/*
 * Copyright 2017 Google Inc. All Rights Reserved.
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

package com.google.ar.core.examples.java.computervision.utility;

import android.util.Log;
import java.nio.ByteBuffer;

/** Detects edges from input grayscale image. */
public class EdgeDetector {
  private static final String TAG = EdgeDetector.class.getSimpleName();

  private static byte[] s_ImageBuffer = new byte[0];
  private static int s_ImageBufferSize = 0;

  /**
   * Detects edges from the input grayscale image.
   *
   * @param outputImage Output image buffer, which has a size of width * height.
   * @param inputImage Input image.
   * @return False if the outputImage buffer is too small, True otherwise.
   */
  public static boolean detect(CameraImageBuffer outputImage, CameraImageBuffer inputImage) {
    if (inputImage == null || inputImage.format != CameraImageBuffer.IMAGE_FORMAT_I8) {
      Log.e(TAG, "Invalid input image!");
      return false;
    }

    if (outputImage == null) {
      Log.e(TAG, "Invalid output image!");
      return false;
    }

    // Recreate output image buffer if it is different from input image buffer.
    if (outputImage.width != inputImage.width
        || outputImage.height != inputImage.height
        || outputImage.format != inputImage.format
        || outputImage.buffer == null) {
      outputImage.width = inputImage.width;
      outputImage.height = inputImage.height;
      outputImage.format = inputImage.format;
      outputImage.buffer = ByteBuffer.allocate(inputImage.width * inputImage.height);
    }

    sobel(outputImage.buffer, inputImage.buffer, inputImage.width, inputImage.height);

    return true;
  }

  private static void sobel(
      ByteBuffer outputBuffer, ByteBuffer inputBuffer, int width, int height) {
    // Adjust buffer size if necessary.
    final int bufferSize = width * height;
    if (s_ImageBuffer.length < bufferSize || bufferSize < s_ImageBufferSize) {
      s_ImageBuffer = new byte[bufferSize];
      s_ImageBufferSize = bufferSize;
    }

    inputBuffer.position(0);
    inputBuffer.get(s_ImageBuffer);

    outputBuffer.position(0);
    byte[] outputPixel = outputBuffer.array();

    // Detect edges.
    int threshold = 128 * 128;

    for (int j = 1; j < height - 1; j++) {
      for (int i = 1; i < width - 1; i++) {
        // Offset of the pixel at [i, j] of the input image.
        int offset = (j * width) + i;

        // Neighbour pixels around the pixel at [i, j].
        int a00 = s_ImageBuffer[offset - width - 1];
        int a01 = s_ImageBuffer[offset - width];
        int a02 = s_ImageBuffer[offset - width + 1];
        int a10 = s_ImageBuffer[offset - 1];
        int a12 = s_ImageBuffer[offset + 1];
        int a20 = s_ImageBuffer[offset + width - 1];
        int a21 = s_ImageBuffer[offset + width];
        int a22 = s_ImageBuffer[offset + width + 1];

        // Sobel X filter:
        //   -1, 0, 1,
        //   -2, 0, 2,
        //   -1, 0, 1
        int xSum = -a00 - (2 * a10) - a20 + a02 + (2 * a12) + a22;

        // Sobel Y filter:
        //    1, 2, 1,
        //    0, 0, 0,
        //   -1, -2, -1
        int ySum = a00 + (2 * a01) + a02 - a20 - (2 * a21) - a22;

        if ((xSum * xSum) + (ySum * ySum) > threshold) {
          outputPixel[(j * width) + i] = (byte) 0xFF;
        } else {
          outputPixel[(j * width) + i] = (byte) 0x1F;
        }
      }
    }
  }
}
