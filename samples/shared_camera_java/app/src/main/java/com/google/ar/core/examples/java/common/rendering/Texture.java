/*
 * Copyright 2020 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.ar.core.examples.java.common.rendering;

import static android.opengl.GLES20.GL_CLAMP_TO_EDGE;
import static android.opengl.GLES20.GL_TEXTURE_2D;
import static android.opengl.GLES20.GL_TEXTURE_MAG_FILTER;
import static android.opengl.GLES20.GL_TEXTURE_MIN_FILTER;
import static android.opengl.GLES20.GL_TEXTURE_WRAP_S;
import static android.opengl.GLES20.GL_TEXTURE_WRAP_T;
import static android.opengl.GLES20.GL_UNSIGNED_BYTE;
import static android.opengl.GLES20.glBindTexture;
import static android.opengl.GLES20.glGenTextures;
import static android.opengl.GLES20.glTexImage2D;
import static android.opengl.GLES20.glTexParameteri;
import static android.opengl.GLES30.GL_LINEAR;
import static android.opengl.GLES30.GL_RG;
import static android.opengl.GLES30.GL_RG8;

import android.media.Image;
import com.google.ar.core.Frame;
import com.google.ar.core.exceptions.NotYetAvailableException;

/** Handle the creation and update of a GPU texture. */
public final class Texture {
  // Stores the latest provided texture id.
  private int textureId = -1;
  private int width = -1;
  private int height = -1;

  /**
   * Creates and initializes the texture. This method needs to be called on a thread with a EGL
   * context attached.
   */
  public void createOnGlThread() {
    int[] textureIdArray = new int[1];
    glGenTextures(1, textureIdArray, 0);
    textureId = textureIdArray[0];
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }

  /**
   * Updates the texture with the content from acquireDepthImage, which provides an image in DEPTH16
   * format, representing each pixel as a depth measurement in millimeters. This method needs to be
   * called on a thread with a EGL context attached.
   */
  public void updateWithDepthImageOnGlThread(final Frame frame) {
    try {
      Image depthImage = frame.acquireDepthImage();
      width = depthImage.getWidth();
      height = depthImage.getHeight();
      glBindTexture(GL_TEXTURE_2D, textureId);
      glTexImage2D(
          GL_TEXTURE_2D,
          0,
          GL_RG8,
          width,
          height,
          0,
          GL_RG,
          GL_UNSIGNED_BYTE,
          depthImage.getPlanes()[0].getBuffer());
      depthImage.close();
    } catch (NotYetAvailableException e) {
      // This normally means that depth data is not available yet. This is normal so we will not
      // spam the logcat with this.
    }
  }

  public int getTextureId() {
    return textureId;
  }

  public int getWidth() {
    return width;
  }

  public int getHeight() {
    return height;
  }
}
