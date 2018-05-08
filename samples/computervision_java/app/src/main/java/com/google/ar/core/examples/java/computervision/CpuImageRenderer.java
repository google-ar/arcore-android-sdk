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

import android.content.Context;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.view.Surface;
import com.google.ar.core.Frame;
import com.google.ar.core.Session;
import com.google.ar.core.examples.java.common.rendering.ShaderUtil;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * This class renders the screen with images from both GPU and CPU. The top half of the screen shows
 * the GPU image, while the bottom half of the screen shows the CPU image.
 */
public class CpuImageRenderer {
  private static final String TAG = CpuImageRenderer.class.getSimpleName();

  private static final int COORDS_PER_VERTEX = 3;
  private static final int TEXCOORDS_PER_VERTEX = 2;
  private static final int FLOAT_SIZE = 4;
  private static final int TEXTURE_COUNT = 4;

  private FloatBuffer quadVertices;
  private FloatBuffer quadTexCoord;
  private FloatBuffer quadTexCoordTransformed;
  private FloatBuffer quadImgCoordTransformed;

  private int quadProgram;

  private int quadPositionAttrib;
  private int quadTexCoordAttrib;
  private int quadImgCoordAttrib;
  private int quadSplitterUniform;
  private int backgroundTextureId = -1;
  private int overlayTextureId = -1;
  private int uTextureId = -1;
  private int vTextureId = -1;
  private float splitterPosition = 0.0f;

  public int getTextureId() {
    return backgroundTextureId;
  }

  /**
   * Allocates and initializes OpenGL resources needed by the background renderer. Must be called on
   * the OpenGL thread, typically in {@link GLSurfaceView.Renderer#onSurfaceCreated(GL10,
   * EGLConfig)}.
   *
   * @param context Needed to access shader source.
   */
  public void createOnGlThread(Context context) throws IOException {
    // Generate the background texture.
    int[] textures = new int[TEXTURE_COUNT];
    GLES20.glGenTextures(TEXTURE_COUNT, textures, 0);

    int index = 0;
    backgroundTextureId = textures[index++];
    GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, backgroundTextureId);
    GLES20.glTexParameteri(
        GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
    GLES20.glTexParameteri(
        GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
    GLES20.glTexParameteri(
        GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_NEAREST);
    GLES20.glTexParameteri(
        GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_NEAREST);

    overlayTextureId = textures[index++];
    GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, overlayTextureId);
    GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
    GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
    GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_NEAREST);

    uTextureId = textures[index++];
    GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, uTextureId);
    GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
    GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
    GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_NEAREST);

    vTextureId = textures[index++];
    GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, vTextureId);
    GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
    GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
    GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_NEAREST);

    int numVertices = QUAD_COORDS.length / COORDS_PER_VERTEX;
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

    ByteBuffer bbTexCoordsTransformed =
        ByteBuffer.allocateDirect(numVertices * TEXCOORDS_PER_VERTEX * FLOAT_SIZE);
    bbTexCoordsTransformed.order(ByteOrder.nativeOrder());
    quadTexCoordTransformed = bbTexCoordsTransformed.asFloatBuffer();

    ByteBuffer bbImgCoordsTransformed =
        ByteBuffer.allocateDirect(numVertices * TEXCOORDS_PER_VERTEX * FLOAT_SIZE);
    bbImgCoordsTransformed.order(ByteOrder.nativeOrder());
    quadImgCoordTransformed = bbImgCoordsTransformed.asFloatBuffer();

    int vertexShader =
        ShaderUtil.loadGLShader(
            TAG, context, GLES20.GL_VERTEX_SHADER, "shaders/cpu_screenquad.vert");
    int fragmentShader =
        ShaderUtil.loadGLShader(
            TAG, context, GLES20.GL_FRAGMENT_SHADER, "shaders/cpu_screenquad.frag");

    quadProgram = GLES20.glCreateProgram();
    GLES20.glAttachShader(quadProgram, vertexShader);
    GLES20.glAttachShader(quadProgram, fragmentShader);
    GLES20.glLinkProgram(quadProgram);
    GLES20.glUseProgram(quadProgram);

    ShaderUtil.checkGLError(TAG, "Program creation");

    quadPositionAttrib = GLES20.glGetAttribLocation(quadProgram, "a_Position");
    quadTexCoordAttrib = GLES20.glGetAttribLocation(quadProgram, "a_TexCoord");
    quadImgCoordAttrib = GLES20.glGetAttribLocation(quadProgram, "a_ImgCoord");
    quadSplitterUniform = GLES20.glGetUniformLocation(quadProgram, "s_SplitterPosition");

    int texLoc = GLES20.glGetUniformLocation(quadProgram, "TexVideo");
    GLES20.glUniform1i(texLoc, 0);

    texLoc = GLES20.glGetUniformLocation(quadProgram, "TexCpuImageGrayscale");
    GLES20.glUniform1i(texLoc, 1);

    ShaderUtil.checkGLError(TAG, "Program parameters");
  }

  /**
   * Gets the texture splitter position.
   *
   * @return the splitter position.
   */
  public float getSplitterPosition() {
    return splitterPosition;
  }

  /**
   * Sets the splitter position. This position defines the splitting position between the background
   * video and the image.
   *
   * @param position the new splitter position.
   */
  public void setSplitterPosition(float position) {
    splitterPosition = position;
  }

  /**
   * Draws the AR background image. The image will be drawn such that virtual content rendered with
   * the matrices provided by {@link Frame#getViewMatrix(float[], int)} and {@link
   * Session#getProjectionMatrix(float[], int, float, float)} will accurately follow static physical
   * objects. This must be called <b>before</b> drawing virtual content.
   *
   * @param frame The last {@code Frame} returned by {@link Session#update()}.
   * @param imageWidth The processed image width.
   * @param imageHeight The processed image height.
   * @param processedImageBytesGrayscale the processed bytes of the image, grayscale par only. Can
   *     be null.
   * @param screenAspectRatio The aspect ratio of the screen.
   * @param cameraToDisplayRotation The rotation of camera with respect to the display. The value is
   *     one of android.view.Surface.ROTATION_#(0, 90, 180, 270).
   */
  public void drawWithCpuImage(
      Frame frame,
      int imageWidth,
      int imageHeight,
      ByteBuffer processedImageBytesGrayscale,
      float screenAspectRatio,
      int cameraToDisplayRotation) {

    // Apply overlay image buffer
    if (processedImageBytesGrayscale != null) {
      GLES20.glActiveTexture(GLES20.GL_TEXTURE1);
      GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, overlayTextureId);
      GLES20.glTexImage2D(
          GLES20.GL_TEXTURE_2D,
          0,
          GLES20.GL_LUMINANCE,
          imageWidth,
          imageHeight,
          0,
          GLES20.GL_LUMINANCE,
          GLES20.GL_UNSIGNED_BYTE,
          processedImageBytesGrayscale);
    }

    updateTextureCoordinates(
        frame, imageWidth, imageHeight, screenAspectRatio, cameraToDisplayRotation);

    // Rest of the draw code is shared between the two functions.
    drawWithoutCpuImage();
  }

  /**
   * Same as above, but will not update the CPU image drawn. Should be used when a CPU image is
   * unavailable for any reason, and only background should be drawn.
   */
  public void drawWithoutCpuImage() {
    // No need to test or write depth, the screen quad has arbitrary depth, and is expected
    // to be drawn first.
    GLES20.glDisable(GLES20.GL_DEPTH_TEST);
    GLES20.glDepthMask(false);

    GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
    GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, backgroundTextureId);

    GLES20.glUseProgram(quadProgram);

    // Set the vertex positions.
    GLES20.glVertexAttribPointer(
        quadPositionAttrib, COORDS_PER_VERTEX, GLES20.GL_FLOAT, false, 0, quadVertices);

    // Set splitter position.
    GLES20.glUniform1f(quadSplitterUniform, splitterPosition);

    // Set the GPU image texture coordinates.
    GLES20.glVertexAttribPointer(
        quadTexCoordAttrib,
        TEXCOORDS_PER_VERTEX,
        GLES20.GL_FLOAT,
        false,
        0,
        quadTexCoordTransformed);

    // Set the CPU image texture coordinates.
    GLES20.glVertexAttribPointer(
        quadImgCoordAttrib,
        TEXCOORDS_PER_VERTEX,
        GLES20.GL_FLOAT,
        false,
        0,
        quadImgCoordTransformed);

    // Enable vertex arrays
    GLES20.glEnableVertexAttribArray(quadPositionAttrib);
    GLES20.glEnableVertexAttribArray(quadTexCoordAttrib);
    GLES20.glEnableVertexAttribArray(quadImgCoordAttrib);

    GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);

    // Disable vertex arrays
    GLES20.glDisableVertexAttribArray(quadPositionAttrib);
    GLES20.glDisableVertexAttribArray(quadTexCoordAttrib);
    GLES20.glDisableVertexAttribArray(quadImgCoordAttrib);

    // Restore the depth state for further drawing.
    GLES20.glDepthMask(true);
    GLES20.glEnable(GLES20.GL_DEPTH_TEST);

    ShaderUtil.checkGLError(TAG, "Draw");
  }

  private void updateTextureCoordinates(
      Frame frame,
      int imageWidth,
      int imageHeight,
      float screenAspectRatio,
      int cameraToDisplayRotation) {
    if (frame == null) {
      return;
    }

    // Crop the CPU image to fit the screen aspect ratio.
    float imageAspectRatio = (float) (imageWidth) / imageHeight;
    float croppedWidth = 0.f;
    float croppedHeight = 0.f;
    if (screenAspectRatio < imageAspectRatio) {
      croppedWidth = imageHeight * screenAspectRatio;
      croppedHeight = imageHeight;
    } else {
      croppedWidth = imageWidth;
      croppedHeight = imageWidth / screenAspectRatio;
    }

    float u = (imageWidth - croppedWidth) / imageWidth / 2.f;
    float v = (imageHeight - croppedHeight) / imageHeight / 2.f;

    float[] texCoords;
    switch (cameraToDisplayRotation) {
      case Surface.ROTATION_90:
        texCoords = new float[] {1 - u, 1 - v, u, 1 - v, 1 - u, v, u, v};
        break;
      case Surface.ROTATION_180:
        texCoords = new float[] {1 - u, v, 1 - u, 1 - v, u, v, u, 1 - v};
        break;
      case Surface.ROTATION_270:
        texCoords = new float[] {u, v, 1 - u, v, u, 1 - v, 1 - u, 1 - v};
        break;
      case Surface.ROTATION_0:

      default:
        texCoords = new float[] {u, 1 - v, u, v, 1 - u, 1 - v, 1 - u, v};
        break;
    }

    // Save CPU image texture coordinates.
    quadImgCoordTransformed.put(texCoords);
    quadImgCoordTransformed.position(0);

    // Update GPU image texture coordinates.
    frame.transformDisplayUvCoords(quadTexCoord, quadTexCoordTransformed);
  }

  private static final float[] QUAD_COORDS =
      new float[] {
        -1.0f, -1.0f, 0.0f, -1.0f, +1.0f, 0.0f, +1.0f, -1.0f, 0.0f, +1.0f, +1.0f, 0.0f,
      };

  private static final float[] QUAD_TEXCOORDS =
      new float[] {
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,
      };
}
