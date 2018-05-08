/*
 * Copyright 2017 Google Inc. All Rights Reserved.
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

package com.google.ar.core.examples.java.computervision;

import android.content.Context;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.opengl.GLES30;
import com.google.ar.core.examples.java.common.rendering.ShaderUtil;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;

/**
 * Helper class for ARCore apps to read camera image from an OpenGL OES texture.
 *
 * <p>This class provides two methods for reading pixels from a texture:
 *
 * <p>(A) All-in-one method: this method utilizes two frame buffers. It does not block the caller
 * thread. Instead it submits a reading request to read pixel to back buffer from the current
 * texture, and returns pixels from the front buffer bund to texture supplied to the previous call
 * to this function. This can be done by calling submitAndAcquire() function.
 *
 * <p>(B) Asychronous method: this method utilizes multiple frame buffers and it does not block the
 * caller thread. This method allows you to read a texture in a lower frequency than rendering
 * frequency(Calling submitAndAcquire() in a lower frequency will result in an "old" image buffer
 * that was submitted a few frames ago). This method contains three routines: submitFrame(),
 * acquireFrame() and releaseFrame().
 *
 * <p>First, you call submitFrame() to submit a frame reading request. GPU will start the reading
 * process in background:
 *
 * <p>bufferIndex = submitFrame(textureId, textureWidth, textureHeight);
 *
 * <p>Second, you call acquireFrame() to get the actual image frame:
 *
 * <p>imageBuffer = acquireFrame(bufferIndex);
 *
 * <p>Last, when you finish using of the imageBuffer retured from acquireFrame(), you need to
 * release the associated frame buffer so that you can reuse it in later frame:
 *
 * <p>releaseFrame(bufferIndex);
 *
 * <p>Note: To use any of the above two methods, you need to call create() routine to initialize the
 * reader before calling any of the reading routine. You will also need to call destroy() method to
 * release the internal resource when you are done with the reader.
 */
public class TextureReader {
  private static final String TAG = TextureReader.class.getSimpleName();

  // By default, we create only two internal buffers. So you can only hold more than one buffer
  // index in your app without releasing it. If you need to hold more than one buffers, you can
  // increase the bufferCount member.
  private final int bufferCount = 2;
  private int[] frameBuffer;
  private int[] texture;
  private int[] pbo;
  private Boolean[] bufferUsed;
  private int frontIndex = -1;
  private int backIndex = -1;

  // By default, the output image format is set to RGBA. You can also set it to IMAGE_FORMAT_I8.
  private int imageFormat = TextureReaderImage.IMAGE_FORMAT_RGBA;
  private int imageWidth = 0;
  private int imageHeight = 0;
  private int pixelBufferSize = 0;
  private Boolean keepAspectRatio = false;

  private FloatBuffer quadVertices;
  private FloatBuffer quadTexCoord;
  private int quadProgram;
  private int quadPositionAttrib;
  private int quadTexCoordAttrib;
  private static final int COORDS_PER_VERTEX = 3;
  private static final int TEXCOORDS_PER_VERTEX = 2;
  private static final int FLOAT_SIZE = 4;
  private static final float[] QUAD_COORDS =
      new float[] {
        -1.0f, -1.0f,
         0.0f, -1.0f,
        +1.0f,  0.0f,
        +1.0f, -1.0f,
         0.0f, +1.0f,
        +1.0f,  0.0f,
      };

  private static final float[] QUAD_TEXCOORDS =
      new float[] {
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
      };

  /**
   * Creates the texture reader. This function needs to be called from the OpenGL rendering thread.
   *
   * @param format the format of the output pixel buffer. It can be one of the two values:
   *     TextureReaderImage.IMAGE_FORMAT_RGBA or TextureReaderImage.IMAGE_FORMAT_I8.
   * @param width the width of the output image.
   * @param height the height of the output image.
   * @param keepAspectRatio whether or not to keep aspect ratio. If true, the output image may be
   *     cropped if the image aspect ratio is different from the texture aspect ratio. If false, the
   *     output image covers the entire texture scope and no cropping is applied.
   */
  public void create(Context context, int format, int width, int height, Boolean keepAspectRatio)
      throws IOException {
    if (format != TextureReaderImage.IMAGE_FORMAT_RGBA
        && format != TextureReaderImage.IMAGE_FORMAT_I8) {
      throw new RuntimeException("Image format not supported.");
    }

    this.keepAspectRatio = keepAspectRatio;
    imageFormat = format;
    imageWidth = width;
    imageHeight = height;
    frontIndex = -1;
    backIndex = -1;

    if (imageFormat == TextureReaderImage.IMAGE_FORMAT_RGBA) {
      pixelBufferSize = imageWidth * imageHeight * 4;
    } else if (imageFormat == TextureReaderImage.IMAGE_FORMAT_I8) {
      pixelBufferSize = imageWidth * imageHeight;
    }

    // Create framebuffers and PBOs.
    pbo = new int[bufferCount];
    frameBuffer = new int[bufferCount];
    texture = new int[bufferCount];
    bufferUsed = new Boolean[bufferCount];
    GLES30.glGenBuffers(bufferCount, pbo, 0);
    GLES20.glGenFramebuffers(bufferCount, frameBuffer, 0);
    GLES20.glGenTextures(bufferCount, texture, 0);

    for (int i = 0; i < bufferCount; i++) {
      bufferUsed[i] = false;
      GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, frameBuffer[i]);

      GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texture[i]);
      GLES30.glTexImage2D(
          GLES30.GL_TEXTURE_2D,
          0,
          imageFormat == TextureReaderImage.IMAGE_FORMAT_I8 ? GLES30.GL_R8 : GLES30.GL_RGBA,
          imageWidth,
          imageHeight,
          0,
          imageFormat == TextureReaderImage.IMAGE_FORMAT_I8 ? GLES30.GL_RED : GLES30.GL_RGBA,
          GLES30.GL_UNSIGNED_BYTE,
          null);
      GLES20.glTexParameteri(
          GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
      GLES20.glTexParameteri(
          GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
      GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
      GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
      GLES20.glFramebufferTexture2D(
          GLES20.GL_FRAMEBUFFER, GLES20.GL_COLOR_ATTACHMENT0, GLES20.GL_TEXTURE_2D, texture[i], 0);

      int status = GLES20.glCheckFramebufferStatus(GLES20.GL_FRAMEBUFFER);
      if (status != GLES20.GL_FRAMEBUFFER_COMPLETE) {
        throw new RuntimeException(
            this
                + ": Failed to set up render buffer with status "
                + status
                + " and error "
                + GLES20.glGetError());
      }

      // Setup PBOs
      GLES30.glBindBuffer(GLES30.GL_PIXEL_PACK_BUFFER, pbo[i]);
      GLES30.glBufferData(
          GLES30.GL_PIXEL_PACK_BUFFER, pixelBufferSize, null, GLES30.GL_DYNAMIC_READ);
      GLES30.glBindBuffer(GLES30.GL_PIXEL_PACK_BUFFER, 0);
    }

    GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);

    // Load shader program.
    int numVertices = 4;
    if (numVertices != QUAD_COORDS.length / COORDS_PER_VERTEX) {
      throw new RuntimeException("Unexpected number of vertices in BackgroundRenderer.");
    }

    ByteBuffer bbVertices = ByteBuffer.allocateDirect(QUAD_COORDS.length * FLOAT_SIZE);
    bbVertices.order(ByteOrder.nativeOrder());
    quadVertices = bbVertices.asFloatBuffer();
    quadVertices.put(QUAD_COORDS);
    quadVertices.position(0);

    ByteBuffer bbTexCoords =
        ByteBuffer.allocateDirect(numVertices * TEXCOORDS_PER_VERTEX * FLOAT_SIZE);
    bbTexCoords.order(ByteOrder.nativeOrder());
    quadTexCoord = bbTexCoords.asFloatBuffer();
    quadTexCoord.put(QUAD_TEXCOORDS);
    quadTexCoord.position(0);

    int vertexShader =
        ShaderUtil.loadGLShader(TAG, context, GLES20.GL_VERTEX_SHADER, "shaders/gpu_download.vert");
    int fragmentShader =
        ShaderUtil.loadGLShader(
            TAG,
            context,
            GLES20.GL_FRAGMENT_SHADER,
            imageFormat == TextureReaderImage.IMAGE_FORMAT_I8
                ? "shaders/gpu_download_i8.frag"
                : "shaders/gpu_download_rgba.frag");

    quadProgram = GLES20.glCreateProgram();
    GLES20.glAttachShader(quadProgram, vertexShader);
    GLES20.glAttachShader(quadProgram, fragmentShader);
    GLES20.glLinkProgram(quadProgram);
    GLES20.glUseProgram(quadProgram);

    quadPositionAttrib = GLES20.glGetAttribLocation(quadProgram, "a_Position");
    quadTexCoordAttrib = GLES20.glGetAttribLocation(quadProgram, "a_TexCoord");
    int texLoc = GLES20.glGetUniformLocation(quadProgram, "sTexture");
    GLES20.glUniform1i(texLoc, 0);
  }

  /** Destroy the texture reader. */
  public void destroy() {
    if (frameBuffer != null) {
      GLES20.glDeleteFramebuffers(bufferCount, frameBuffer, 0);
      frameBuffer = null;
    }
    if (texture != null) {
      GLES20.glDeleteTextures(bufferCount, texture, 0);
      texture = null;
    }
    if (pbo != null) {
      GLES30.glDeleteBuffers(bufferCount, pbo, 0);
      pbo = null;
    }
  }

  /**
   * Submits a frame reading request. This routine does not return the result frame buffer
   * immediately. Instead, it returns a frame buffer index, which can be used to acquire the frame
   * buffer later through acquireFrame().
   *
   * <p>If there is no enough frame buffer available, an exception will be thrown.
   *
   * @param textureId the id of the input OpenGL texture.
   * @param textureWidth width of the texture in pixels.
   * @param textureHeight height of the texture in pixels.
   * @return the index to the frame buffer this request is associated to. You should use this index
   *     to acquire the frame using acquireFrame(); and you should release the frame buffer using
   *     releaseBuffer() routine after using of the frame.
   */
  public int submitFrame(int textureId, int textureWidth, int textureHeight) {
    // Find next buffer.
    int bufferIndex = -1;
    for (int i = 0; i < bufferCount; i++) {
      if (!bufferUsed[i]) {
        bufferIndex = i;
        break;
      }
    }
    if (bufferIndex == -1) {
      throw new RuntimeException("No buffer available.");
    }

    // Bind both read and write to framebuffer.
    GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, frameBuffer[bufferIndex]);

    // Save and setup viewport
    IntBuffer viewport = IntBuffer.allocate(4);
    GLES20.glGetIntegerv(GLES20.GL_VIEWPORT, viewport);
    GLES20.glViewport(0, 0, imageWidth, imageHeight);

    // Draw texture to framebuffer.
    drawTexture(textureId, textureWidth, textureHeight);

    // Start reading into PBO
    GLES30.glBindBuffer(GLES30.GL_PIXEL_PACK_BUFFER, pbo[bufferIndex]);
    GLES30.glReadBuffer(GLES30.GL_COLOR_ATTACHMENT0);

    GLES30.glReadPixels(
        0,
        0,
        imageWidth,
        imageHeight,
        imageFormat == TextureReaderImage.IMAGE_FORMAT_I8 ? GLES30.GL_RED : GLES20.GL_RGBA,
        GLES20.GL_UNSIGNED_BYTE,
        0);

    // Restore viewport.
    GLES20.glViewport(viewport.get(0), viewport.get(1), viewport.get(2), viewport.get(3));

    GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
    GLES30.glBindBuffer(GLES30.GL_PIXEL_PACK_BUFFER, 0);

    bufferUsed[bufferIndex] = true;
    return bufferIndex;
  }

  /**
   * Acquires the frame requested earlier. This routine returns a TextureReaderImage object that
   * contains the pixels mapped to the frame buffer requested previously through submitFrame().
   *
   * <p>If input buffer index is invalid, an exception will be thrown.
   *
   * @param bufferIndex the index to the frame buffer to be acquired. It has to be a frame index
   *     returned from submitFrame().
   * @return a TextureReaderImage object if succeed. Null otherwise.
   */
  public TextureReaderImage acquireFrame(int bufferIndex) {
    if (bufferIndex < 0 || bufferIndex >= bufferCount || !bufferUsed[bufferIndex]) {
      throw new RuntimeException("Invalid buffer index.");
    }

    // Bind the current PB and acquire the pixel buffer.
    GLES30.glBindBuffer(GLES30.GL_PIXEL_PACK_BUFFER, pbo[bufferIndex]);
    ByteBuffer mapped =
        (ByteBuffer)
            GLES30.glMapBufferRange(
                GLES30.GL_PIXEL_PACK_BUFFER, 0, pixelBufferSize, GLES30.GL_MAP_READ_BIT);

    // Wrap the mapped buffer into TextureReaderImage object.
    TextureReaderImage buffer =
        new TextureReaderImage(imageWidth, imageHeight, imageFormat, mapped);

    return buffer;
  }

  /**
   * Releases a previously requested frame buffer. If input buffer index is invalid, an exception
   * will be thrown.
   *
   * @param bufferIndex the index to the frame buffer to be acquired. It has to be a frame index
   *     returned from submitFrame().
   */
  public void releaseFrame(int bufferIndex) {
    if (bufferIndex < 0 || bufferIndex >= bufferCount || !bufferUsed[bufferIndex]) {
      throw new RuntimeException("Invalid buffer index.");
    }
    GLES30.glBindBuffer(GLES30.GL_PIXEL_PACK_BUFFER, pbo[bufferIndex]);
    GLES30.glUnmapBuffer(GLES30.GL_PIXEL_PACK_BUFFER);
    GLES30.glBindBuffer(GLES30.GL_PIXEL_PACK_BUFFER, 0);
    bufferUsed[bufferIndex] = false;
  }

  /**
   * Reads pixels using dual buffers. This function sends the reading request to GPU and returns the
   * result from the previous call. Thus, the first call always returns null. The pixelBuffer member
   * in the returned object maps to the internal buffer. This buffer cannot be overrode, and it
   * becomes invalid after next call to submitAndAcquire().
   *
   * @param textureId the OpenGL texture Id.
   * @param textureWidth width of the texture in pixels.
   * @param textureHeight height of the texture in pixels.
   * @return a TextureReaderImage object that contains the pixels read from the texture.
   */
  public TextureReaderImage submitAndAcquire(int textureId, int textureWidth, int textureHeight) {
    // Release previously used front buffer.
    if (frontIndex != -1) {
      releaseFrame(frontIndex);
    }

    // Move previous back buffer to front buffer.
    frontIndex = backIndex;

    // Submit new request on back buffer.
    backIndex = submitFrame(textureId, textureWidth, textureHeight);

    // Acquire frame from the new front buffer.
    if (frontIndex != -1) {
      return acquireFrame(frontIndex);
    }

    return null;
  }

  /** Draws texture to full screen. */
  private void drawTexture(int textureId, int textureWidth, int textureHeight) {
    // Disable features that we don't use.
    GLES20.glDisable(GLES20.GL_DEPTH_TEST);
    GLES20.glDisable(GLES20.GL_CULL_FACE);
    GLES20.glDisable(GLES20.GL_SCISSOR_TEST);
    GLES20.glDisable(GLES20.GL_STENCIL_TEST);
    GLES20.glDisable(GLES20.GL_BLEND);
    GLES20.glDepthMask(false);
    GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, 0);
    GLES20.glBindBuffer(GLES20.GL_ELEMENT_ARRAY_BUFFER, 0);
    GLES30.glBindVertexArray(0);

    // Clear buffers.
    GLES20.glClearColor(0, 0, 0, 0);
    GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT | GLES20.GL_DEPTH_BUFFER_BIT);

    // Set the vertex positions.
    GLES20.glVertexAttribPointer(
        quadPositionAttrib, COORDS_PER_VERTEX, GLES20.GL_FLOAT, false, 0, quadVertices);

    // Calculate the texture coordinates.
    if (keepAspectRatio) {
      int renderWidth = 0;
      int renderHeight = 0;
      float textureAspectRatio = (float) (textureWidth) / textureHeight;
      float imageAspectRatio = (float) (imageWidth) / imageHeight;
      if (textureAspectRatio < imageAspectRatio) {
        renderWidth = imageWidth;
        renderHeight = textureHeight * imageWidth / textureWidth;
      } else {
        renderWidth = textureWidth * imageHeight / textureHeight;
        renderHeight = imageHeight;
      }
      float offsetU = (float) (renderWidth - imageWidth) / renderWidth / 2;
      float offsetV = (float) (renderHeight - imageHeight) / renderHeight / 2;

      float[] texCoords =
          new float[] {
            offsetU, offsetV, offsetU, 1 - offsetV, 1 - offsetU, offsetV, 1 - offsetU, 1 - offsetV
          };

      quadTexCoord.put(texCoords);
      quadTexCoord.position(0);
    } else {
      quadTexCoord.put(QUAD_TEXCOORDS);
      quadTexCoord.position(0);
    }

    // Set the texture coordinates.
    GLES20.glVertexAttribPointer(
        quadTexCoordAttrib, TEXCOORDS_PER_VERTEX, GLES20.GL_FLOAT, false, 0, quadTexCoord);

    // Enable vertex arrays
    GLES20.glEnableVertexAttribArray(quadPositionAttrib);
    GLES20.glEnableVertexAttribArray(quadTexCoordAttrib);

    GLES20.glUseProgram(quadProgram);

    // Select input texture.
    GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
    GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, textureId);

    // Draw a quad with texture.
    GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);

    // Disable vertex arrays
    GLES20.glDisableVertexAttribArray(quadPositionAttrib);
    GLES20.glDisableVertexAttribArray(quadTexCoordAttrib);

    // Reset texture binding.
    GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, 0);
  }
}
