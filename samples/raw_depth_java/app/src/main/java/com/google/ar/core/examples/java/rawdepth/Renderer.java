/*
 * Copyright 2021 Google LLC
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

package com.google.ar.core.examples.java.rawdepth;

import android.content.Context;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.opengl.Matrix;
import com.google.ar.core.examples.java.common.rendering.ShaderUtil;
import java.io.IOException;
import java.util.ArrayList;

/** Renders the data from Raw Depth API as 3D points. */
final class Renderer {
  private static final String TAG = Renderer.class.getSimpleName();

  public static final int POSITION_FLOATS_PER_POINT = 4; // X, Y, Z, confidence.
  public static final int COLOR_FLOATS_PER_POINT = 3; // Red, green, blue channels.

  public static final int BYTES_PER_FLOAT = Float.SIZE / 8;
  private static final int POSITION_BYTES_PER_POINT = BYTES_PER_FLOAT * POSITION_FLOATS_PER_POINT;
  private static final int COLOR_BYTES_PER_POINT = BYTES_PER_FLOAT * COLOR_FLOATS_PER_POINT;
  private static final int INITIAL_BUFFER_POINTS = 1000;

  // Shader names.
  private static final String VERTEX_SHADER_NAME = "shaders/depth_point_cloud.vert";
  private static final String FRAGMENT_SHADER_NAME = "shaders/depth_point_cloud.frag";

  /**
   * The list of pointcloud frames to render. Each item in the list represents a single raw depth
   * frame, taken at different times and poses.
   */
  private ArrayList<DepthData> depthFrames = new ArrayList<DepthData>();

  private int positionAttribute;
  private int positionBuffer;
  private int positionBufferSize;

  private int colorAttribute;
  private int colorBuffer;
  private int colorBufferSize;

  private int programName;
  private int modelViewProjectionUniform;
  private int pointSizeUniform;
  private int confidenceThresholdUniform;

  private int numPoints = 0;

  /**
   * The minimum confidence value of a depth image pixel to be rendered as a point. The initial
   * value is selected to remove only the most unreliable depth values. Low confidence points are
   * discarded in the vertex shader when they fall below this threshold value.
   */
  private float minConfidence = 0.1f;

  public Renderer() {}

  /**
   * Allocates and initializes OpenGL resources needed by the plane renderer. Must be called on the
   * OpenGL thread, typically in {@link GLSurfaceView.Renderer#onSurfaceCreated(GL10, EGLConfig)}.
   *
   * @param context Needed to access shader source.
   */
  public void createOnGlThread(Context context) throws IOException {
    ShaderUtil.checkGLError(TAG, "Bind");

    int[] buffers = new int[2];
    GLES20.glGenBuffers(2, buffers, 0);
    positionBuffer = buffers[0];
    colorBuffer = buffers[1];

    GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, positionBuffer);
    positionBufferSize = INITIAL_BUFFER_POINTS * POSITION_BYTES_PER_POINT;
    GLES20.glBufferData(GLES20.GL_ARRAY_BUFFER, positionBufferSize, null, GLES20.GL_DYNAMIC_DRAW);

    GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, colorBuffer);
    colorBufferSize = INITIAL_BUFFER_POINTS * COLOR_BYTES_PER_POINT;
    GLES20.glBufferData(GLES20.GL_ARRAY_BUFFER, colorBufferSize, null, GLES20.GL_DYNAMIC_DRAW);

    GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, 0);

    ShaderUtil.checkGLError(TAG, "Create");

    int vertexShader =
        ShaderUtil.loadGLShader(TAG, context, GLES20.GL_VERTEX_SHADER, VERTEX_SHADER_NAME);
    int passthroughShader =
        ShaderUtil.loadGLShader(TAG, context, GLES20.GL_FRAGMENT_SHADER, FRAGMENT_SHADER_NAME);

    programName = GLES20.glCreateProgram();
    GLES20.glAttachShader(programName, vertexShader);
    GLES20.glAttachShader(programName, passthroughShader);
    GLES20.glLinkProgram(programName);
    GLES20.glUseProgram(programName);

    ShaderUtil.checkGLError(TAG, "Program");

    positionAttribute = GLES20.glGetAttribLocation(programName, "a_Position");
    colorAttribute = GLES20.glGetAttribLocation(programName, "a_Color");
    modelViewProjectionUniform = GLES20.glGetUniformLocation(programName, "u_ModelViewProjection");
    pointSizeUniform = GLES20.glGetUniformLocation(programName, "u_PointSize");
    confidenceThresholdUniform = GLES20.glGetUniformLocation(programName, "u_ConfidenceThreshold");

    ShaderUtil.checkGLError(TAG, "Init complete");
  }

  /**
   * Updates the OpenGL buffer contents to the provided point. Repeated calls with the same point
   * cloud will be ignored.
   */
  public void update(DepthData depth) {
    depthFrames.add(depth);

    // We store and visualize multiple previous frames to create a denser map of the environment.
    // Cap the total number of depth frames and anchors to avoid memory exhaustion (each depth frame
    // consumes memory). ARCore consumes CPU cycles for each anchor that it tracks, and this cap
    // avoids overloading the app rendering system.
    final int maxFramesStored = 60;
    while (depthFrames.size() > maxFramesStored) {
      depthFrames.get(0).getAnchor().detach();
      depthFrames.remove(0);
    }
  }

  /**
   * Renders the point cloud. ARCore point cloud is given in world space.
   *
   * @param viewMatrix The camera view matrix for this frame, typically from {@link
   *     com.google.ar.core.Camera#getViewMatrix(float[], int)}.
   * @param projectionMatrix The camera projection matrix for this frame, typically from {@link
   *     com.google.ar.core.Camera#getProjectionMatrix(float[], int, float, float)}.
   */
  public void draw(float[] viewMatrix, float[] projectionMatrix) {
    if (depthFrames.isEmpty()) {
      return;
    }

    // Move the camera backwards by 1 meter, to help convey 3D depth of the point cloud.
    moveCameraAlongLocalZAxis(viewMatrix, -1f);

    float[] modelMatrix = new float[16];
    float[] modelView = new float[16];
    float[] modelViewProjection = new float[16];

    ShaderUtil.checkGLError(TAG, "Draw");

    for (DepthData depthFrame : depthFrames) {
      numPoints = depthFrame.getPoints().remaining() / POSITION_FLOATS_PER_POINT;

      // Resize the position buffer if needed.
      while (numPoints * POSITION_BYTES_PER_POINT > positionBufferSize) {
        positionBufferSize *= 2;
      }
      GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, positionBuffer);
      GLES20.glBufferData(GLES20.GL_ARRAY_BUFFER, positionBufferSize, null, GLES20.GL_DYNAMIC_DRAW);

      GLES20.glBufferSubData(
          GLES20.GL_ARRAY_BUFFER, 0, numPoints * POSITION_BYTES_PER_POINT, depthFrame.getPoints());

      // Resize the color buffer if needed.
      while (numPoints * COLOR_BYTES_PER_POINT > colorBufferSize) {
        colorBufferSize *= 2;
      }
      GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, colorBuffer);
      GLES20.glBufferData(GLES20.GL_ARRAY_BUFFER, colorBufferSize, null, GLES20.GL_DYNAMIC_DRAW);
      GLES20.glBufferSubData(
          GLES20.GL_ARRAY_BUFFER, 0, numPoints * COLOR_BYTES_PER_POINT, depthFrame.getColors());

      depthFrame.getModelMatrix(modelMatrix);

      Matrix.multiplyMM(modelView, 0, viewMatrix, 0, modelMatrix, 0);
      Matrix.multiplyMM(modelViewProjection, 0, projectionMatrix, 0, modelView, 0);

      GLES20.glUseProgram(programName);

      GLES20.glEnableVertexAttribArray(positionAttribute);
      GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, positionBuffer);
      GLES20.glVertexAttribPointer(
          positionAttribute, 4, GLES20.GL_FLOAT, false, POSITION_BYTES_PER_POINT, 0);

      GLES20.glEnableVertexAttribArray(colorAttribute);
      GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, colorBuffer);
      GLES20.glVertexAttribPointer(
          colorAttribute, 4, GLES20.GL_FLOAT, false, COLOR_BYTES_PER_POINT, 0);

      GLES20.glUniformMatrix4fv(modelViewProjectionUniform, 1, false, modelViewProjection, 0);
      GLES20.glUniform1f(pointSizeUniform, 5.0f);
      GLES20.glUniform1f(confidenceThresholdUniform, minConfidence);

      GLES20.glDrawArrays(GLES20.GL_POINTS, 0, numPoints);
      GLES20.glDisableVertexAttribArray(positionAttribute);
      GLES20.glDisableVertexAttribArray(colorAttribute);
      GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, 0);
    }

    ShaderUtil.checkGLError(TAG, "Draw complete");
  }

  /** Returns the fraction of points that is currently rendered based on confidence. */
  float getPointAmount() {
    // Higher confidence threshold means lower point amount.
    return 1f - minConfidence;
  }

  /**
   * Configures the fraction of points that should be rendered based on their depth confidence.
   *
   * @param pointAmount How many depth points should be rendered. The value must be in [0; 1] range
   *     (inclusive).
   */
  void setPointAmount(float pointAmount) {
    minConfidence = 1f - pointAmount;
  }

  /**
   * Translates the virtual camera along its local forward axis by a specified amount in meters.
   *
   * @param viewMatrix The camera view matrix for this frame, typically from {@link
   *     com.google.ar.core.Camera#getViewMatrix(float[], int)}.
   * @param translation The distance in meters that the camera should be moved.
   */
  private static void moveCameraAlongLocalZAxis(float[] viewMatrix, float translation) {
    // Construct a 4x4 matrix that transforms a point in the camera space into the world space.
    float[] cameraToWorld = new float[16];
    Matrix.invertM(cameraToWorld, 0, viewMatrix, 0);

    // Calculate the current and the modified camera position in the world space.
    float[] originalCameraPositionCS = new float[] {0f, 0f, 0f, 1f};
    float[] modifiedCameraPositionCS = new float[] {0f, 0f, translation, 1f};
    float[] originalCameraPositionWS = new float[4];
    float[] modifiedCameraPositionWS = new float[4];
    Matrix.multiplyMV(originalCameraPositionWS, 0, cameraToWorld, 0, originalCameraPositionCS, 0);
    Matrix.multiplyMV(modifiedCameraPositionWS, 0, cameraToWorld, 0, modifiedCameraPositionCS, 0);

    // Move the camera from the current position into `modifiedCameraPositionWS`.
    float deltaX = modifiedCameraPositionWS[0] - originalCameraPositionWS[0];
    float deltaY = modifiedCameraPositionWS[1] - originalCameraPositionWS[1];
    float deltaZ = modifiedCameraPositionWS[2] - originalCameraPositionWS[2];
    Matrix.translateM(viewMatrix, 0, deltaX, deltaY, deltaZ);
  }
}
