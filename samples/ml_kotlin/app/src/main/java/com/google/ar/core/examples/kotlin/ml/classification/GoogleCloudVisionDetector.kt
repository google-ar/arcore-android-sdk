/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.ar.core.examples.kotlin.ml.classification

import android.graphics.PointF
import android.media.Image
import android.util.Base64
import android.util.Log
import com.google.ar.core.examples.kotlin.ml.MainActivity
import com.google.ar.core.examples.kotlin.ml.classification.utils.ImageUtils
import com.google.ar.core.examples.kotlin.ml.classification.utils.ImageUtils.toByteArray
import com.google.ar.core.examples.kotlin.ml.classification.utils.VertexUtils
import com.google.ar.core.examples.kotlin.ml.classification.utils.VertexUtils.rotateCoordinates
import com.google.ar.core.examples.kotlin.ml.classification.utils.VertexUtils.toAbsoluteCoordinates
import com.google.gson.JsonArray
import com.google.gson.JsonObject
import com.google.gson.JsonParser
import okhttp3.MediaType
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.RequestBody

/**
 * Finds detected objects ([DetectedObjectResult]s) given an [android.media.Image]. See
 * [Cloud Vision API's detect multiple objects developer guide](https://cloud.google.com/vision/docs/object-localizer)
 * .
 */
class GoogleCloudVisionDetector(val activity: MainActivity, val apiKey: String) :
  ObjectDetector(activity) {
  companion object {
    val TAG = "GoogleCloudVisionDetector"
  }

  val httpClient = OkHttpClient.Builder().build()

  override suspend fun analyze(image: Image, imageRotation: Int): List<DetectedObjectResult> {
    // `image` is in YUV
    // (https://developers.google.com/ar/reference/java/com/google/ar/core/Frame#acquireCameraImage()),
    val convertYuv = convertYuv(image)

    // The model performs best on upright images, so rotate it.
    val rotatedImage = ImageUtils.rotateBitmap(convertYuv, imageRotation)

    // Create Google Cloud Vision request.
    val body = createRequestBody(rotatedImage.toByteArray())
    val req =
      httpClient.newCall(
        Request.Builder()
          .url("https://vision.googleapis.com/v1/images:annotate?key=$apiKey")
          .post(RequestBody.create(MediaType.parse("text/json"), body.toString()))
          .build()
      )

    // Execute Google Cloud Vision request and parse response body.
    req.execute().use { response ->
      val responseBody = response.body()?.string()
      if (responseBody == null) {
        Log.e(TAG, "Failed to parse result body.")
        return emptyList()
      }
      Log.i(TAG, "Received response body: $responseBody")
      val jsonBody = JsonParser.parseString(responseBody).asJsonObject

      // Process result and map to DetectedObjectResult.
      // https://cloud.google.com/vision/docs/reference/rest/v1/AnnotateImageResponse
      val responseObject = jsonBody.getAsJsonArray("responses").first().asJsonObject
      // https://cloud.google.com/vision/docs/reference/rest/v1/AnnotateImageResponse#LocalizedObjectAnnotation
      val localisedObjectAnnotationsList =
        responseObject.getAsJsonArray("localizedObjectAnnotations") ?: return emptyList()
      return localisedObjectAnnotationsList.map { it.asJsonObject }.map { annotation ->
        // https://cloud.google.com/vision/docs/reference/rest/v1/projects.locations.products.referenceImages#BoundingPoly
        val boundingPoly = annotation.get("boundingPoly").asJsonObject
        val centerCoordinateNormalized =
          VertexUtils.calculateCenterOfPoly(boundingPolyToCoordinateList(boundingPoly))
        val centerCoordinateAbsolute =
          centerCoordinateNormalized.toAbsoluteCoordinates(rotatedImage.width, rotatedImage.height)
        val rotatedCoordinates =
          centerCoordinateAbsolute.rotateCoordinates(
            rotatedImage.width,
            rotatedImage.height,
            imageRotation
          )
        DetectedObjectResult(
          confidence = annotation.get("score").asFloat,
          label = annotation.get("name").asString,
          centerCoordinate = rotatedCoordinates
        )
      }
    }
  }

  /**
   * Creates an
   * [`images.annotate` request body](https://cloud.google.com/vision/docs/reference/rest/v1/images/annotate#request-body)
   * @param image a [ByteArray] representation of the image to upload to Google Cloud Vision API.
   */
  fun createRequestBody(image: ByteArray): JsonObject {

    return JsonObject().apply {
      add(
        "requests",
        JsonArray().apply {
          add(
            JsonObject().apply {
              add(
                "image",
                JsonObject().apply {
                  addProperty("content", Base64.encodeToString(image, Base64.DEFAULT))
                }
              )
              add(
                "features",
                JsonArray().apply {
                  add(JsonObject().apply { addProperty("type", "OBJECT_LOCALIZATION") })
                }
              )
            }
          )
        }
      )
    }
  }

  /**
   * Transforms a [JsonObject] of
   * [BoundingPoly](https://cloud.google.com/vision/docs/reference/rest/v1/projects.locations.products.referenceImages#BoundingPoly)
   * to [List<PointF>].
   */
  fun boundingPolyToCoordinateList(boundingPoly: JsonObject): List<PointF> {
    return boundingPoly.get("normalizedVertices").asJsonArray.map { it.asJsonObject }.mapNotNull {
      if (it["x"] == null || it["y"] == null) null else PointF(it["x"].asFloat, it["y"].asFloat)
    }
  }
}
