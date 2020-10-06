/*
 * Copyright 2020 Google LLC
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
package com.google.ar.core.examples.java.common.samplerender.arcore;

import com.google.ar.core.Coordinates2d;
import com.google.ar.core.Frame;
import com.google.ar.core.examples.java.common.samplerender.Mesh;
import com.google.ar.core.examples.java.common.samplerender.SampleRender;
import com.google.ar.core.examples.java.common.samplerender.Shader;
import com.google.ar.core.examples.java.common.samplerender.Texture;
import com.google.ar.core.examples.java.common.samplerender.VertexBuffer;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

/**
 * This class renders the AR background from camera feed. It creates and hosts the texture given to
 * ARCore to be filled with the camera image.
 */
public class BackgroundRenderer {
  private static final String TAG = BackgroundRenderer.class.getSimpleName();

  // Shader names.
  private static final String CAMERA_VERTEX_SHADER_NAME = "shaders/background_show_camera.vert";
  private static final String CAMERA_FRAGMENT_SHADER_NAME = "shaders/background_show_camera.frag";
  private static final String DEPTH_VISUALIZER_VERTEX_SHADER_NAME =
      "shaders/background_show_depth_color_visualization.vert";
  private static final String DEPTH_VISUALIZER_FRAGMENT_SHADER_NAME =
      "shaders/background_show_depth_color_visualization.frag";

  private static final int FLOAT_SIZE = 4;

  private static final float[] QUAD_COORDS_ARRAY = {
    /*0:*/ -1f, -1f, /*1:*/ +1f, -1f, /*2:*/ -1f, +1f, /*3:*/ +1f, +1f,
  };

  private static final FloatBuffer QUAD_COORDS_BUFFER =
      ByteBuffer.allocateDirect(QUAD_COORDS_ARRAY.length * FLOAT_SIZE)
          .order(ByteOrder.nativeOrder())
          .asFloatBuffer();

  static {
    QUAD_COORDS_BUFFER.put(QUAD_COORDS_ARRAY);
  }

  private final FloatBuffer texCoords =
      ByteBuffer.allocateDirect(QUAD_COORDS_ARRAY.length * FLOAT_SIZE)
          .order(ByteOrder.nativeOrder())
          .asFloatBuffer();

  private final Mesh mesh;
  private final VertexBuffer texCoordsVertexBuffer;
  private final Shader cameraShader;
  private final Shader depthShader;
  private final Texture cameraTexture;

  /**
   * Allocates and initializes OpenGL resources needed by the background renderer. Must be called
   * during a {@link SampleRender.Renderer} callback, typically in {@link
   * SampleRender.Renderer#onSurfaceCreated()}.
   */
  public BackgroundRenderer(SampleRender render, Texture depthTexture) throws IOException {
    cameraTexture =
        new Texture(render, Texture.Target.TEXTURE_EXTERNAL_OES, Texture.WrapMode.CLAMP_TO_EDGE);

    cameraShader =
        Shader.createFromAssets(
                render, CAMERA_VERTEX_SHADER_NAME, CAMERA_FRAGMENT_SHADER_NAME, /*defines=*/ null)
            .setTexture("u_Texture", cameraTexture)
            .setDepthTest(false)
            .setDepthWrite(false);

    if (depthTexture == null) {
      depthShader = null;
    } else {
      depthShader =
          Shader.createFromAssets(
                  render,
                  DEPTH_VISUALIZER_VERTEX_SHADER_NAME,
                  DEPTH_VISUALIZER_FRAGMENT_SHADER_NAME,
                  /*defines=*/ null)
              .setTexture("u_DepthTexture", depthTexture)
              .setDepthTest(false)
              .setDepthWrite(false);
    }

    VertexBuffer localCoordsVertexBuffer = new VertexBuffer(render, 2, QUAD_COORDS_BUFFER);
    texCoordsVertexBuffer =
        new VertexBuffer(render, /*numberOfEntriesPerVertex=*/ 2, /*entries=*/ null);
    VertexBuffer[] vertexBuffers = {
      localCoordsVertexBuffer, texCoordsVertexBuffer,
    };
    mesh =
        new Mesh(render, Mesh.PrimitiveMode.TRIANGLE_STRIP, /*indexBuffer=*/ null, vertexBuffers);
  }

  /**
   * Draws the AR background image. The image will be drawn such that virtual content rendered with
   * the matrices provided by {@link com.google.ar.core.Camera#getViewMatrix(float[], int)} and
   * {@link com.google.ar.core.Camera#getProjectionMatrix(float[], int, float, float)} will
   * accurately follow static physical objects. This must be called <b>before</b> drawing virtual
   * content.
   *
   * <p>Virtual content should be rendered using the matrices provided by {@link
   * com.google.ar.core.Camera#getViewMatrix(float[], int)} and {@link
   * com.google.ar.core.Camera#getProjectionMatrix(float[], int, float, float)}.
   *
   * @param frame The current {@code Frame} as returned by {@link Session#update()}.
   * @param showDebugDepthMap Toggles whether to show the live camera feed or latest depth image.
   */
  public void draw(SampleRender render, Frame frame, boolean showDebugDepthMap) {
    // If display rotation changed (also includes view size change), we need to re-query the uv
    // coordinates for the screen rect, as they may have changed as well.
    if (frame.hasDisplayGeometryChanged()) {
      QUAD_COORDS_BUFFER.rewind();
      frame.transformCoordinates2d(
          Coordinates2d.OPENGL_NORMALIZED_DEVICE_COORDINATES,
          QUAD_COORDS_BUFFER,
          Coordinates2d.TEXTURE_NORMALIZED,
          texCoords);
      texCoordsVertexBuffer.set(texCoords);
    }

    if (frame.getTimestamp() == 0) {
      // Suppress rendering if the camera did not produce the first frame yet. This is to avoid
      // drawing possible leftover data from previous sessions if the texture is reused.
      return;
    }

    if (showDebugDepthMap) {
      render.draw(mesh, depthShader);
    } else {
      render.draw(mesh, cameraShader);
    }
  }

  /** Return the texture ID generated by this object. */
  public int getTextureId() {
    return cameraTexture.getTextureId();
  }
}
