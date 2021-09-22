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
package com.google.ar.core.examples.kotlin.ml

import android.content.pm.PackageManager
import android.opengl.Matrix
import android.util.Log
import androidx.lifecycle.DefaultLifecycleObserver
import androidx.lifecycle.LifecycleOwner
import com.google.ar.core.Anchor
import com.google.ar.core.Coordinates2d
import com.google.ar.core.Frame
import com.google.ar.core.TrackingState
import com.google.ar.core.examples.java.common.helpers.DisplayRotationHelper
import com.google.ar.core.examples.java.common.samplerender.SampleRender
import com.google.ar.core.examples.java.common.samplerender.arcore.BackgroundRenderer
import com.google.ar.core.examples.kotlin.ml.classification.DetectedObjectResult
import com.google.ar.core.examples.kotlin.ml.classification.GoogleCloudVisionDetector
import com.google.ar.core.examples.kotlin.ml.classification.MLKitObjectDetector
import com.google.ar.core.examples.kotlin.ml.classification.ObjectDetector
import com.google.ar.core.examples.kotlin.ml.render.LabelRender
import com.google.ar.core.examples.kotlin.ml.render.PointCloudRender
import com.google.ar.core.exceptions.CameraNotAvailableException
import com.google.ar.core.exceptions.NotYetAvailableException
import java.util.Collections
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.MainScope
import kotlinx.coroutines.launch

/** Renders the ML application into using our sample Renderer. */
class AppRenderer(val activity: MainActivity) : DefaultLifecycleObserver, SampleRender.Renderer {
  companion object {
    val TAG = "MLAppRenderer"
  }

  lateinit var view: MainActivityView
  private val coroutineScope = MainScope()

  val displayRotationHelper = DisplayRotationHelper(activity)

  // Rendering components
  lateinit var backgroundRenderer: BackgroundRenderer
  val pointCloudRender = PointCloudRender()
  val labelRenderer = LabelRender()

  // Matrices for reuse in order to prevent reallocations every frame.
  val viewMatrix = FloatArray(16)
  val projectionMatrix = FloatArray(16)
  val viewProjectionMatrix = FloatArray(16)

  val arLabeledAnchors = Collections.synchronizedList(mutableListOf<ARLabeledAnchor>())
  var scanButtonWasPressed = false

  val mlKitAnalyzer = MLKitObjectDetector(activity)
  val gcpAnalyzer = run {
    // API key used to authenticate with Google Cloud Vision API. See README for steps on how to
    // obtain a valid API key.
    val applicationInfo =
      activity.packageManager.getApplicationInfo(activity.packageName, PackageManager.GET_META_DATA)
    val apiKey = applicationInfo.metaData.getString("com.google.ar.core.examples.kotlin.ml.API_KEY")
    if (apiKey == null) null else GoogleCloudVisionDetector(activity, apiKey)
  }

  var currentAnalyzer: ObjectDetector = gcpAnalyzer ?: mlKitAnalyzer

  override fun onResume(owner: LifecycleOwner) {
    displayRotationHelper.onResume()
  }

  override fun onPause(owner: LifecycleOwner) {
    displayRotationHelper.onPause()
  }

  /** Binds UI elements for ARCore interactions. */
  fun bindView(view: MainActivityView) {
    this.view = view

    view.scanButton.setOnClickListener {
      // frame.acquireCameraImage is dependent on an ARCore Frame, which is only available in
      // onDrawFrame.
      // Use a boolean and check its state in onDrawFrame to interact with the camera image.
      scanButtonWasPressed = true
      view.setScanningActive(true)
      hideSnackbar()
    }

    val gcpConfigured = gcpAnalyzer != null
    val configuredAnalyzer = gcpAnalyzer ?: mlKitAnalyzer
    view.useCloudMlSwitch.setOnCheckedChangeListener { _, isChecked ->
      currentAnalyzer = if (isChecked) configuredAnalyzer else mlKitAnalyzer
    }

    view.useCloudMlSwitch.isChecked = gcpConfigured
    view.useCloudMlSwitch.isEnabled = gcpConfigured
    currentAnalyzer = if (gcpConfigured) configuredAnalyzer else mlKitAnalyzer

    if (!gcpConfigured) {
      showSnackbar(
        "Google Cloud Vision isn't configured (see README). The Cloud ML switch will be disabled."
      )
    }

    view.resetButton.setOnClickListener {
      synchronized(arLabeledAnchors) { arLabeledAnchors.clear() }
      view.resetButton.isEnabled = false
      hideSnackbar()
    }
  }

  override fun onSurfaceCreated(render: SampleRender) {
    backgroundRenderer =
      BackgroundRenderer(render).apply { setUseDepthVisualization(render, false) }
    pointCloudRender.onSurfaceCreated(render)
    labelRenderer.onSurfaceCreated(render)
  }

  override fun onSurfaceChanged(render: SampleRender?, width: Int, height: Int) {
    displayRotationHelper.onSurfaceChanged(width, height)
  }

  var objectResults: List<DetectedObjectResult>? = null

  override fun onDrawFrame(render: SampleRender) {
    val session = activity.arCoreSessionHelper.session ?: return
    session.setCameraTextureNames(intArrayOf(backgroundRenderer.cameraColorTexture.textureId))

    // Notify ARCore session that the view size changed so that the perspective matrix and
    // the video background can be properly adjusted.
    displayRotationHelper.updateSessionIfNeeded(session)

    val frame =
      try {
        session.update()
      } catch (e: CameraNotAvailableException) {
        Log.e(TAG, "Camera not available during onDrawFrame", e)
        showSnackbar("Camera not available. Try restarting the app.")
        return
      }

    backgroundRenderer.updateDisplayGeometry(frame)
    backgroundRenderer.drawBackground(render)

    // Get camera and projection matrices.
    val camera = frame.camera
    camera.getViewMatrix(viewMatrix, 0)
    camera.getProjectionMatrix(projectionMatrix, 0, 0.01f, 100.0f)
    Matrix.multiplyMM(viewProjectionMatrix, 0, projectionMatrix, 0, viewMatrix, 0)

    // Handle tracking failures.
    if (camera.trackingState != TrackingState.TRACKING) {
      return
    }

    // Draw point cloud.
    frame.acquirePointCloud().use { pointCloud ->
      pointCloudRender.drawPointCloud(render, pointCloud, viewProjectionMatrix)
    }

    // Frame.acquireCameraImage must be used on the GL thread.
    // Check if the button was pressed last frame to start processing the camera image.
    if (scanButtonWasPressed) {
      scanButtonWasPressed = false
      val cameraImage = frame.tryAcquireCameraImage()
      if (cameraImage != null) {
        // Call our ML model on an IO thread.
        coroutineScope.launch(Dispatchers.IO) {
          val cameraId = session.cameraConfig.cameraId
          val imageRotation = displayRotationHelper.getCameraSensorToDisplayRotation(cameraId)
          objectResults =
            try {
              currentAnalyzer.analyze(cameraImage, imageRotation)
            } catch (exception: Exception) {
              showSnackbar(
                "Exception thrown analyzing input frame: " +
                  exception.message +
                  "\n" +
                  "See adb log for details."
              )
              Log.e(TAG, "Exception thrown analyzing input frame", exception)
              null
            }
          cameraImage.close()
        }
      }
    }

    /** If results were completed this frame, create [Anchor]s from model results. */
    val objects = objectResults
    if (objects != null) {
      objectResults = null
      Log.i(TAG, "$currentAnalyzer got objects: $objects")
      val anchors =
        objects.mapNotNull { obj ->
          val anchor =
            createAnchor(obj.centerCoordinate.x.toFloat(), obj.centerCoordinate.y.toFloat(), frame)
              ?: return@mapNotNull null
          Log.i(TAG, "Created anchor ${anchor.pose} from hit test")
          ARLabeledAnchor(anchor, obj.label)
        }
      arLabeledAnchors.addAll(anchors)
      view.post {
        view.resetButton.isEnabled = arLabeledAnchors.isNotEmpty()
        view.setScanningActive(false)
        when {
          objects.isEmpty() &&
            currentAnalyzer == mlKitAnalyzer &&
            !mlKitAnalyzer.hasCustomModel() ->
            showSnackbar(
              "Default ML Kit classification model returned no results. " +
                "For better classification performance, see the README to configure a custom model."
            )
          objects.isEmpty() -> showSnackbar("Classification model returned no results.")
          anchors.size != objects.size ->
            showSnackbar(
              "Objects were classified, but could not be attached to an anchor. " +
                "Try moving your device around to obtain a better understanding of the environment."
            )
        }
      }
    }

    // Draw labels at their anchor position.
    synchronized(arLabeledAnchors) {
      for (arDetectedObject in arLabeledAnchors) {
        val anchor = arDetectedObject.anchor
        if (anchor.trackingState != TrackingState.TRACKING) continue
        labelRenderer.draw(
          render,
          viewProjectionMatrix,
          anchor.pose,
          camera.pose,
          arDetectedObject.label
        )
      }
    }
  }

  /**
   * Utility method for [Frame.acquireCameraImage] that maps [NotYetAvailableException] to `null`.
   */
  fun Frame.tryAcquireCameraImage() =
    try {
      acquireCameraImage()
    } catch (e: NotYetAvailableException) {
      null
    } catch (e: Throwable) {
      throw e
    }

  private fun showSnackbar(message: String): Unit =
    activity.view.snackbarHelper.showMessageWithDismiss(activity, message)

  private fun hideSnackbar() = activity.view.snackbarHelper.hide(activity)

  /** Temporary arrays to prevent allocations in [createAnchor]. */
  private val convertFloats = FloatArray(4)
  private val convertFloatsOut = FloatArray(4)

  /**
   * Create an anchor using (x, y) coordinates in the [Coordinates2d.IMAGE_PIXELS] coordinate space.
   */
  fun createAnchor(xImage: Float, yImage: Float, frame: Frame): Anchor? {
    // IMAGE_PIXELS -> VIEW
    convertFloats[0] = xImage
    convertFloats[1] = yImage
    frame.transformCoordinates2d(
      Coordinates2d.IMAGE_PIXELS,
      convertFloats,
      Coordinates2d.VIEW,
      convertFloatsOut
    )

    // Conduct a hit test using the VIEW coordinates
    val hits = frame.hitTest(convertFloatsOut[0], convertFloatsOut[1])
    val result = hits.getOrNull(0) ?: return null
    return result.trackable.createAnchor(result.hitPose)
  }
}

data class ARLabeledAnchor(val anchor: Anchor, val label: String)
