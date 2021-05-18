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

package com.google.ar.core.examples.java.rawdepth;

import android.media.Image;
import android.media.Image.Plane;
import com.google.ar.core.CameraIntrinsics;
import com.google.ar.core.Coordinates2d;
import com.google.ar.core.Frame;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.ShortBuffer;

/** Static utilities for depth data transformations. */
public final class PointCloudHelper {

  private PointCloudHelper() {}

  /**
   * Creates a linear buffer of 3D point positions in the world space and the corresponding
   * confidence values.
   */
  public static FloatBuffer convertRawDepthImagesTo3dPointBuffer(
      Image depth, Image confidence, CameraIntrinsics cameraTextureIntrinsics, int pointLimit) {
    Plane depthImagePlane = depth.getPlanes()[0];
    // Set the endianess to ensure we extract depth data in the correct byte order.
    ShortBuffer depthBuffer =
        depthImagePlane.getBuffer().order(ByteOrder.nativeOrder()).asShortBuffer();

    Plane confidenceImagePlane = confidence.getPlanes()[0];
    ByteBuffer confidenceBuffer = confidenceImagePlane.getBuffer().order(ByteOrder.nativeOrder());

    // To transform 2D depth pixels into 3D points we retrieve the intrinsic camera parameters
    // corresponding to the depth image. See more information about the depth values at
    // https://developers.google.com/ar/develop/java/depth/overview#understand-depth-values.
    int[] intrinsicsDimensions = cameraTextureIntrinsics.getImageDimensions();
    int depthWidth = depth.getWidth();
    int depthHeight = depth.getHeight();
    float fx = cameraTextureIntrinsics.getFocalLength()[0] * depthWidth / intrinsicsDimensions[0];
    float fy = cameraTextureIntrinsics.getFocalLength()[1] * depthHeight / intrinsicsDimensions[1];
    float cx =
        cameraTextureIntrinsics.getPrincipalPoint()[0] * depthWidth / intrinsicsDimensions[0];
    float cy =
        cameraTextureIntrinsics.getPrincipalPoint()[1] * depthHeight / intrinsicsDimensions[1];

    // Allocate the destination point buffer. If the number of depth pixels is larger than
    // `pointLimit` we do uniform image subsampling. Alternatively we could reduce the number of
    // points based on depth confidence at this stage.
    int step = calculateImageSubsamplingStep(depthWidth, depthHeight, pointLimit);
    FloatBuffer points =
        FloatBuffer.allocate(
            depthWidth / step * depthHeight / step * Renderer.POSITION_FLOATS_PER_POINT);

    for (int y = 0; y < depthHeight; y += step) {
      for (int x = 0; x < depthWidth; x += step) {
        // Depth images are tightly packed, so it's OK to not use row and pixel strides.
        int depthMillimeters = depthBuffer.get(y * depthWidth + x); // Depth image pixels are in mm.
        if (depthMillimeters == 0) {
          // A pixel that has a value of zero has a missing depth estimate at this location.
          continue;
        }

        float depthMeters = depthMillimeters / 1000.0f;

        points.put(depthMeters * (x - cx) / fx); // X.
        points.put(depthMeters * (cy - y) / fy); // Y.
        points.put(-depthMeters); // Z.

        // Depth confidence value for this pixel, stored as an unsigned byte in range [0, 255].
        byte confidencePixelValue =
            confidenceBuffer.get(
                y * confidenceImagePlane.getRowStride()
                    + x * confidenceImagePlane.getPixelStride());
        // Normalize depth confidence to [0.0, 1.0] float range.
        float confidenceNormalized = ((float) (confidencePixelValue & 0xff)) / 255.0f;
        points.put(confidenceNormalized);
      }
    }

    points.rewind();

    return points;
  }

  /** Calculates the CPU image region that corresponds to the area covered by the depth image. */
  public static FloatBuffer getImageCoordinatesForFullTexture(Frame frame) {
    FloatBuffer textureCoords =
        ByteBuffer.allocateDirect(TEXTURE_COORDS.length * Float.SIZE)
            .order(ByteOrder.nativeOrder())
            .asFloatBuffer()
            .put(TEXTURE_COORDS);
    textureCoords.position(0);
    FloatBuffer imageCoords =
        ByteBuffer.allocateDirect(TEXTURE_COORDS.length * Float.SIZE)
            .order(ByteOrder.nativeOrder())
            .asFloatBuffer();
    frame.transformCoordinates2d(
        Coordinates2d.TEXTURE_NORMALIZED, textureCoords, Coordinates2d.IMAGE_PIXELS, imageCoords);
    return imageCoords;
  }

  /**
   * Creates a linear buffer of RGB color values corresponding to the values in the depth image.
   * Pixels with the depth value equal to zero are not included in the output.
   */
  public static FloatBuffer convertImageToColorBuffer(
      Image color, Image depth, FloatBuffer imageCoords, int pointLimit) {
    int depthWidth = depth.getWidth();
    int depthHeight = depth.getHeight();
    int colorWidth = color.getWidth();
    int colorHeight = color.getHeight();
    Plane imagePlaneY = color.getPlanes()[0];
    Plane imagePlaneU = color.getPlanes()[1];
    Plane imagePlaneV = color.getPlanes()[2];
    int rowStrideY = imagePlaneY.getRowStride();
    int rowStrideU = imagePlaneU.getRowStride();
    int rowStrideV = imagePlaneV.getRowStride();
    int pixelStrideY = imagePlaneY.getPixelStride();
    int pixelStrideU = imagePlaneU.getPixelStride();
    int pixelStrideV = imagePlaneV.getPixelStride();
    ByteBuffer colorBufferY = imagePlaneY.getBuffer();
    ByteBuffer colorBufferU = imagePlaneU.getBuffer();
    ByteBuffer colorBufferV = imagePlaneV.getBuffer();

    // The first CPU image row overlapping with the depth image region.
    int colorMinY = Math.round(imageCoords.get(1));
    // The last CPU image row overlapping with the depth image region.
    int colorMaxY = Math.round(imageCoords.get(3));
    int colorRegionHeight = colorMaxY - colorMinY;

    Plane depthImagePlane = depth.getPlanes()[0];
    ShortBuffer depthBuffer =
        depthImagePlane.getBuffer().order(ByteOrder.nativeOrder()).asShortBuffer();

    // Allocate the destination color buffer.
    int step = calculateImageSubsamplingStep(depthWidth, depthHeight, pointLimit);
    FloatBuffer colors =
        FloatBuffer.allocate(
            depthWidth / step * depthHeight / step * Renderer.COLOR_FLOATS_PER_POINT);

    float rgb[] = new float[3]; // Reusable space for 3-channel color values.

    for (int y = 0; y < depthHeight; y += step) {
      for (int x = 0; x < depthWidth; x += step) {
        if (depthBuffer.get(y * depthWidth + x) == 0) {
          // A pixel that has a value of zero has a missing depth estimate at this location.
          continue;
        }

        // Retrieve the color at this point.
        int colorX = x * colorWidth / depthWidth;
        int colorY = colorMinY + y * colorRegionHeight / depthHeight;
        int colorHalfX = colorX / 2;
        int colorHalfY = colorY / 2;

        // Each channel value is an unsigned byte, so we need to apply `0xff` to convert the sign.
        int channelValueY = colorBufferY.get(colorY * rowStrideY + colorX * pixelStrideY) & 0xff;
        int channelValueU =
            colorBufferU.get(colorHalfY * rowStrideU + colorHalfX * pixelStrideU) & 0xff;
        int channelValueV =
            colorBufferV.get(colorHalfY * rowStrideV + colorHalfX * pixelStrideV) & 0xff;

        convertYuvToRgb(channelValueY, channelValueU, channelValueV, rgb);
        colors.put(rgb[0]);
        colors.put(rgb[1]);
        colors.put(rgb[2]);
      }
    }

    colors.rewind();

    return colors;
  }

  /** Returs the increment in rows and columns to sample the image n times. */
  private static int calculateImageSubsamplingStep(int imageWidth, int imageHeight, int n) {
    return (int) Math.ceil(Math.sqrt((float) imageWidth * imageHeight / n));
  }

  /**
   * Converts a YUV color value into RGB. Input YUV values are expected in the range [0, 255].
   * Output RGB values are in the range [0.0, 1.0].
   */
  private static void convertYuvToRgb(int yInt, int uInt, int vInt, float[] rgb) {
    // See https://en.wikipedia.org/wiki/YUV.
    float yFloat = yInt / 255.0f; // Range [0.0, 1.0].
    float uFloat = uInt * 0.872f / 255.0f - 0.436f; // Range [-0.436, 0.436].
    float vFloat = vInt * 1.230f / 255.0f - 0.615f; // Range [-0.615, 0.615].
    rgb[0] = clamp(yFloat + 1.13983f * vFloat);
    rgb[1] = clamp(yFloat - 0.39465f * uFloat - 0.58060f * vFloat);
    rgb[2] = clamp(yFloat + 2.03211f * uFloat);
  }

  /**
   * Clamps the value to [0, 1] range (inclusive).
   *
   * <p>If the value passed in is between 0 and 1, then it is returned unchanged.
   *
   * <p>If the value passed in is less than 0, 0 is returned.
   *
   * <p>If the value passed in is greater than 1, 1 is returned.
   */
  private static float clamp(float val) {
    return Math.max(0.0f, Math.min(1.0f, val));
  }

  private static final float[] TEXTURE_COORDS =
      new float[] {
        0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
      };
}
