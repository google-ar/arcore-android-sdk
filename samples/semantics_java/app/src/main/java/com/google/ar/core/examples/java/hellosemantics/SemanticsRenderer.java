/*
 * Copyright 2023 Google LLC
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

package com.google.ar.core.examples.java.hellosemantics;

import android.media.Image;
import android.opengl.GLES30;
import com.google.ar.core.Coordinates2d;
import com.google.ar.core.Frame;
import com.google.ar.core.examples.java.common.samplerender.Framebuffer;
import com.google.ar.core.examples.java.common.samplerender.Mesh;
import com.google.ar.core.examples.java.common.samplerender.SampleRender;
import com.google.ar.core.examples.java.common.samplerender.Shader;
import com.google.ar.core.examples.java.common.samplerender.Texture;
import com.google.ar.core.examples.java.common.samplerender.VertexBuffer;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

/** This class renders semantic Images to a framebuffer or the screen. */
public class SemanticsRenderer {
  private static final String TAG = SemanticsRenderer.class.getSimpleName();

  // components_per_vertex * number_of_vertices * float_size
  private static final int COORDS_BUFFER_SIZE = 2 * 4 * 4;

  private static final FloatBuffer NDC_QUAD_COORDS_BUFFER =
      ByteBuffer.allocateDirect(COORDS_BUFFER_SIZE).order(ByteOrder.nativeOrder()).asFloatBuffer();

  private static final FloatBuffer VIRTUAL_SCENE_TEX_COORDS_BUFFER =
      ByteBuffer.allocateDirect(COORDS_BUFFER_SIZE).order(ByteOrder.nativeOrder()).asFloatBuffer();

  static {
    NDC_QUAD_COORDS_BUFFER.put(
        new float[] {
          /*0:*/ -1f, -1f, /*1:*/ +1f, -1f, /*2:*/ -1f, +1f, /*3:*/ +1f, +1f,
        });
    VIRTUAL_SCENE_TEX_COORDS_BUFFER.put(
        new float[] {
          /*0:*/ 0f, 0f, /*1:*/ 1f, 0f, /*2:*/ 0f, 1f, /*3:*/ 1f, 1f,
        });
  }

  private final FloatBuffer cameraTexCoords =
      ByteBuffer.allocateDirect(COORDS_BUFFER_SIZE).order(ByteOrder.nativeOrder()).asFloatBuffer();

  private final Texture cameraSemanticsTexture;
  private Texture semanticsColorPaletteTexture;
  private Shader backgroundSemanticsShader;
  private final VertexBuffer cameraTexCoordsVertexBuffer;
  private final Mesh mesh;

  /**
   * Allocates and initializes OpenGL resources needed by the semantics renderer. Must be called
   * during a {@link SampleRender.Renderer} callback, typically in {@link
   * SampleRender.Renderer#onSurfaceCreated()}.
   */
  public SemanticsRenderer(SampleRender render) {
    cameraSemanticsTexture =
        new Texture(
            render,
            Texture.Target.TEXTURE_2D,
            Texture.WrapMode.CLAMP_TO_EDGE,
            /* useMipmaps= */ false);

    // Create a Mesh with three vertex buffers: one for the screen coordinates (normalized device
    // coordinates), one for the camera texture coordinates (to be populated with proper data later
    // before drawing), and one for the virtual scene texture coordinates (unit texture quad)
    VertexBuffer screenCoordsVertexBuffer =
        new VertexBuffer(render, /* numberOfEntriesPerVertex= */ 2, NDC_QUAD_COORDS_BUFFER);
    cameraTexCoordsVertexBuffer =
        new VertexBuffer(render, /* numberOfEntriesPerVertex= */ 2, /* entries= */ null);
    VertexBuffer virtualSceneTexCoordsVertexBuffer =
        new VertexBuffer(
            render, /* numberOfEntriesPerVertex= */ 2, VIRTUAL_SCENE_TEX_COORDS_BUFFER);
    VertexBuffer[] vertexBuffers = {
      screenCoordsVertexBuffer, cameraTexCoordsVertexBuffer, virtualSceneTexCoordsVertexBuffer,
    };
    mesh =
        new Mesh(render, Mesh.PrimitiveMode.TRIANGLE_STRIP, /* indexBuffer= */ null, vertexBuffers);
  }

  /** Sets up the shader responsible for drawing the semantic image. */
  public void setupSemanticsShader(SampleRender render) throws IOException {
    semanticsColorPaletteTexture =
        Texture.createFromAsset(
            render,
            "textures/semantics_sv12_colormap.png",
            Texture.WrapMode.CLAMP_TO_EDGE,
            Texture.ColorFormat.LINEAR);
    backgroundSemanticsShader =
        Shader.createFromAssets(
                render,
                "shaders/background_semantics.vert",
                "shaders/background_semantics.frag",
                /* defines= */ null)
            .setTexture("uSemanticImage", cameraSemanticsTexture)
            .setTexture("uSemanticColorMap", semanticsColorPaletteTexture)
            .setDepthTest(false)
            .setDepthWrite(false);
  }

  /**
   * Updates the display geometry. This must be called every frame before calling either of
   * SemanticsRenderer's draw methods.
   *
   * @param frame The current {@code Frame} as returned by {@link Session#update()}.
   */
  public void updateDisplayGeometry(Frame frame) {
    if (frame.hasDisplayGeometryChanged()) {
      // If display rotation changed (also includes view size change), we need to re-query the UV
      // coordinates for the screen rect, as they may have changed as well.
      frame.transformCoordinates2d(
          Coordinates2d.OPENGL_NORMALIZED_DEVICE_COORDINATES,
          NDC_QUAD_COORDS_BUFFER,
          Coordinates2d.TEXTURE_NORMALIZED,
          cameraTexCoords);
      cameraTexCoordsVertexBuffer.set(cameraTexCoords);
    }
  }

  /** Update semantics texture with Image contents. */
  public void updateCameraSemanticsTexture(Image image) {
    // SampleRender abstraction leaks here
    GLES30.glBindTexture(GLES30.GL_TEXTURE_2D, cameraSemanticsTexture.getTextureId());
    GLES30.glTexImage2D(
        GLES30.GL_TEXTURE_2D,
        0,
        GLES30.GL_R8,
        image.getWidth(),
        image.getHeight(),
        0,
        GLES30.GL_RED,
        GLES30.GL_UNSIGNED_BYTE,
        image.getPlanes()[0].getBuffer());
  }

  /**
   * Draws semantic image.
   *
   * @param render the render to use.
   */
  public void draw(SampleRender render) {
    render.draw(mesh, backgroundSemanticsShader);
  }

  /**
   * Draws semantic image to a given framebuffer.
   *
   * @param render the render to use.
   * @param framebuffer the destination framebuffer
   */
  public void draw(SampleRender render, Framebuffer framebuffer) {
    render.draw(mesh, backgroundSemanticsShader, framebuffer);
  }
}
