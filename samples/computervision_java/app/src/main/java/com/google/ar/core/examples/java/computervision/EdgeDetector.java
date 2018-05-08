/*
 * Copyright 2018 Google Inc. All Rights Reserved.
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
package com.google.ar.core.examples.java.computervision;

import java.nio.ByteBuffer;

/** Detects edges from input YUV image. */
public class EdgeDetector {
  private byte[] inputPixels = new byte[0]; // Reuse java byte array to avoid multiple allocations.

  private static final int SOBEL_EDGE_THRESHOLD = 128 * 128;

  /**
   * Process a grayscale image using the Sobel edge detector.
   *
   * @param width image width.
   * @param height image height.
   * @param stride image stride (number of bytes per row, equals to width if no row padding).
   * @param input bytes of the image, assumed single channel grayscale of size [stride * height].
   * @return bytes of the processed image, where the byte value is the strength of the edge at that
   *     pixel. Number of bytes is width * height, row padding (if any) is removed.
   */
  public synchronized ByteBuffer detect(int width, int height, int stride, ByteBuffer input) {
    // Reallocate input byte array if its size is different from the required size.
    if (stride * height != inputPixels.length) {
      inputPixels = new byte[stride * height];
    }

    // Allocate a new output byte array.
    byte[] outputPixels = new byte[width * height];

    // Copy input buffer into a java array for ease of access. This is not the most optimal
    // way to process an image, but used here for simplicity.
    input.position(0);
    input.get(inputPixels);

    // Detect edges.
    for (int j = 1; j < height - 1; j++) {
      for (int i = 1; i < width - 1; i++) {
        // Offset of the pixel at [i, j] of the input image.
        int offset = (j * stride) + i;

        // Neighbour pixels around the pixel at [i, j].
        int a00 = inputPixels[offset - width - 1];
        int a01 = inputPixels[offset - width];
        int a02 = inputPixels[offset - width + 1];
        int a10 = inputPixels[offset - 1];
        int a12 = inputPixels[offset + 1];
        int a20 = inputPixels[offset + width - 1];
        int a21 = inputPixels[offset + width];
        int a22 = inputPixels[offset + width + 1];

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

        if ((xSum * xSum) + (ySum * ySum) > SOBEL_EDGE_THRESHOLD) {
          outputPixels[(j * width) + i] = (byte) 0xFF;
        } else {
          outputPixels[(j * width) + i] = (byte) 0x1F;
        }
      }
    }

    return ByteBuffer.wrap(outputPixels);
  }
}
