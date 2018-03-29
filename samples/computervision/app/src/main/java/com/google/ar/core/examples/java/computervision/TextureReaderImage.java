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
package com.google.ar.core.examples.java.computervision;

import java.nio.ByteBuffer;

/** Image Buffer Class. */
public class TextureReaderImage {
  /** The id corresponding to RGBA8888. */
  public static final int IMAGE_FORMAT_RGBA = 0;

  /** The id corresponding to grayscale. */
  public static final int IMAGE_FORMAT_I8 = 1;

  /** The width of the image, in pixels. */
  public int width;

  /** The height of the image, in pixels. */
  public int height;

  /** The image buffer. */
  public ByteBuffer buffer;

  /** Pixel format. Can be either IMAGE_FORMAT_RGBA or IMAGE_FORMAT_I8. */
  public int format;

  /** Default constructor. */
  public TextureReaderImage() {
    width = 1;
    height = 1;
    format = IMAGE_FORMAT_RGBA;
    buffer = ByteBuffer.allocateDirect(4);
  }

  /**
   * Constructor.
   *
   * @param imgWidth the width of the image, in pixels.
   * @param imgHeight the height of the image, in pixels.
   * @param imgFormat the format of the image.
   * @param imgBuffer the buffer of the image pixels.
   */
  public TextureReaderImage(int imgWidth, int imgHeight, int imgFormat, ByteBuffer imgBuffer) {
    if (imgWidth == 0 || imgHeight == 0) {
      throw new RuntimeException("Invalid image size.");
    }

    if (imgFormat != IMAGE_FORMAT_RGBA && imgFormat != IMAGE_FORMAT_I8) {
      throw new RuntimeException("Invalid image format.");
    }

    if (imgBuffer == null) {
      throw new RuntimeException("Pixel buffer cannot be null.");
    }

    width = imgWidth;
    height = imgHeight;
    format = imgFormat;
    buffer = imgBuffer;
  }
}
