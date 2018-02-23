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
package com.google.ar.core.examples.java.helloar.rendering;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.opengl.GLES20;
import android.opengl.GLUtils;
import android.opengl.Matrix;
import com.google.ar.core.examples.java.helloar.R;
import de.javagl.obj.Obj;
import de.javagl.obj.ObjData;
import de.javagl.obj.ObjReader;
import de.javagl.obj.ObjUtils;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;
import java.nio.ShortBuffer;

/** Renders an object loaded from an OBJ file in OpenGL. */
public class ObjectRenderer {
  private static final String TAG = ObjectRenderer.class.getSimpleName();

  /**
   * Blend mode.
   *
   * @see #setBlendMode(BlendMode)
   */
  public enum BlendMode {
    /** Multiplies the destination color by the source alpha. */
    Shadow,
    /** Normal alpha blending. */
    Grid
  }

  private static final int COORDS_PER_VERTEX = 3;

  // Note: the last component must be zero to avoid applying the translational part of the matrix.
  private static final float[] LIGHT_DIRECTION = new float[] {0.250f, 0.866f, 0.433f, 0.0f};
  private final float[] viewLightDirection = new float[4];

  // Object vertex buffer variables.
  private int vertexBufferId;
  private int verticesBaseAddress;
  private int texCoordsBaseAddress;
  private int normalsBaseAddress;
  private int indexBufferId;
  private int indexCount;

  private int program;
  private final int[] textures = new int[1];

  // Shader location: model view projection matrix.
  private int modelViewUniform;
  private int modelViewProjectionUniform;

  // Shader location: object attributes.
  private int positionAttribute;
  private int normalAttribute;
  private int texCoordAttribute;

  // Shader location: texture sampler.
  private int textureUniform;

  // Shader location: environment properties.
  private int lightingParametersUniform;

  // Shader location: material properties.
  private int materialParametersUniform;

  private BlendMode blendMode = null;

  // Temporary matrices allocated here to reduce number of allocations for each frame.
  private final float[] modelMatrix = new float[16];
  private final float[] modelViewMatrix = new float[16];
  private final float[] modelViewProjectionMatrix = new float[16];

  // Set some default material properties to use for lighting.
  private float ambient = 0.3f;
  private float diffuse = 1.0f;
  private float specular = 1.0f;
  private float specularPower = 6.0f;

  public ObjectRenderer() {}

  /**
   * Creates and initializes OpenGL resources needed for rendering the model.
   *
   * @param context Context for loading the shader and below-named model and texture assets.
   * @param objAssetName Name of the OBJ file containing the model geometry.
   * @param diffuseTextureAssetName Name of the PNG file containing the diffuse texture map.
   */
  public void createOnGlThread(Context context, String objAssetName, String diffuseTextureAssetName)
      throws IOException {
    // Read the texture.
    Bitmap textureBitmap =
        BitmapFactory.decodeStream(context.getAssets().open(diffuseTextureAssetName));

    GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
    GLES20.glGenTextures(textures.length, textures, 0);
    GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textures[0]);

    GLES20.glTexParameteri(
        GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR_MIPMAP_LINEAR);
    GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
    GLUtils.texImage2D(GLES20.GL_TEXTURE_2D, 0, textureBitmap, 0);
    GLES20.glGenerateMipmap(GLES20.GL_TEXTURE_2D);
    GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);

    textureBitmap.recycle();

    ShaderUtil.checkGLError(TAG, "Texture loading");

    // Read the obj file.
    InputStream objInputStream = context.getAssets().open(objAssetName);
    Obj obj = ObjReader.read(objInputStream);

    // Prepare the Obj so that its structure is suitable for
    // rendering with OpenGL:
    // 1. Triangulate it
    // 2. Make sure that texture coordinates are not ambiguous
    // 3. Make sure that normals are not ambiguous
    // 4. Convert it to single-indexed data
    obj = ObjUtils.convertToRenderable(obj);

    // OpenGL does not use Java arrays. ByteBuffers are used instead to provide data in a format
    // that OpenGL understands.

    // Obtain the data from the OBJ, as direct buffers:
    IntBuffer wideIndices = ObjData.getFaceVertexIndices(obj, 3);
    FloatBuffer vertices = ObjData.getVertices(obj);
    FloatBuffer texCoords = ObjData.getTexCoords(obj, 2);
    FloatBuffer normals = ObjData.getNormals(obj);

    // Convert int indices to shorts for GL ES 2.0 compatibility
    ShortBuffer indices =
        ByteBuffer.allocateDirect(2 * wideIndices.limit())
            .order(ByteOrder.nativeOrder())
            .asShortBuffer();
    while (wideIndices.hasRemaining()) {
      indices.put((short) wideIndices.get());
    }
    indices.rewind();

    int[] buffers = new int[2];
    GLES20.glGenBuffers(2, buffers, 0);
    vertexBufferId = buffers[0];
    indexBufferId = buffers[1];

    // Load vertex buffer
    verticesBaseAddress = 0;
    texCoordsBaseAddress = verticesBaseAddress + 4 * vertices.limit();
    normalsBaseAddress = texCoordsBaseAddress + 4 * texCoords.limit();
    final int totalBytes = normalsBaseAddress + 4 * normals.limit();

    GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, vertexBufferId);
    GLES20.glBufferData(GLES20.GL_ARRAY_BUFFER, totalBytes, null, GLES20.GL_STATIC_DRAW);
    GLES20.glBufferSubData(
        GLES20.GL_ARRAY_BUFFER, verticesBaseAddress, 4 * vertices.limit(), vertices);
    GLES20.glBufferSubData(
        GLES20.GL_ARRAY_BUFFER, texCoordsBaseAddress, 4 * texCoords.limit(), texCoords);
    GLES20.glBufferSubData(
        GLES20.GL_ARRAY_BUFFER, normalsBaseAddress, 4 * normals.limit(), normals);
    GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, 0);

    // Load index buffer
    GLES20.glBindBuffer(GLES20.GL_ELEMENT_ARRAY_BUFFER, indexBufferId);
    indexCount = indices.limit();
    GLES20.glBufferData(
        GLES20.GL_ELEMENT_ARRAY_BUFFER, 2 * indexCount, indices, GLES20.GL_STATIC_DRAW);
    GLES20.glBindBuffer(GLES20.GL_ELEMENT_ARRAY_BUFFER, 0);

    ShaderUtil.checkGLError(TAG, "OBJ buffer load");

    final int vertexShader =
        ShaderUtil.loadGLShader(TAG, context, GLES20.GL_VERTEX_SHADER, R.raw.object_vertex);
    final int fragmentShader =
        ShaderUtil.loadGLShader(TAG, context, GLES20.GL_FRAGMENT_SHADER, R.raw.object_fragment);

    program = GLES20.glCreateProgram();
    GLES20.glAttachShader(program, vertexShader);
    GLES20.glAttachShader(program, fragmentShader);
    GLES20.glLinkProgram(program);
    GLES20.glUseProgram(program);

    ShaderUtil.checkGLError(TAG, "Program creation");

    modelViewUniform = GLES20.glGetUniformLocation(program, "u_ModelView");
    modelViewProjectionUniform = GLES20.glGetUniformLocation(program, "u_ModelViewProjection");

    positionAttribute = GLES20.glGetAttribLocation(program, "a_Position");
    normalAttribute = GLES20.glGetAttribLocation(program, "a_Normal");
    texCoordAttribute = GLES20.glGetAttribLocation(program, "a_TexCoord");

    textureUniform = GLES20.glGetUniformLocation(program, "u_Texture");

    lightingParametersUniform = GLES20.glGetUniformLocation(program, "u_LightingParameters");
    materialParametersUniform = GLES20.glGetUniformLocation(program, "u_MaterialParameters");

    ShaderUtil.checkGLError(TAG, "Program parameters");

    Matrix.setIdentityM(modelMatrix, 0);
  }

  /**
   * Selects the blending mode for rendering.
   *
   * @param blendMode The blending mode. Null indicates no blending (opaque rendering).
   */
  public void setBlendMode(BlendMode blendMode) {
    this.blendMode = blendMode;
  }

  /**
   * Updates the object model matrix and applies scaling.
   *
   * @param modelMatrix A 4x4 model-to-world transformation matrix, stored in column-major order.
   * @param scaleFactor A separate scaling factor to apply before the {@code modelMatrix}.
   * @see android.opengl.Matrix
   */
  public void updateModelMatrix(float[] modelMatrix, float scaleFactor) {
    float[] scaleMatrix = new float[16];
    Matrix.setIdentityM(scaleMatrix, 0);
    scaleMatrix[0] = scaleFactor;
    scaleMatrix[5] = scaleFactor;
    scaleMatrix[10] = scaleFactor;
    Matrix.multiplyMM(this.modelMatrix, 0, modelMatrix, 0, scaleMatrix, 0);
  }

  /**
   * Sets the surface characteristics of the rendered model.
   *
   * @param ambient Intensity of non-directional surface illumination.
   * @param diffuse Diffuse (matte) surface reflectivity.
   * @param specular Specular (shiny) surface reflectivity.
   * @param specularPower Surface shininess. Larger values result in a smaller, sharper specular
   *     highlight.
   */
  public void setMaterialProperties(
      float ambient, float diffuse, float specular, float specularPower) {
    this.ambient = ambient;
    this.diffuse = diffuse;
    this.specular = specular;
    this.specularPower = specularPower;
  }

  /**
   * Draws the model.
   *
   * @param cameraView A 4x4 view matrix, in column-major order.
   * @param cameraPerspective A 4x4 projection matrix, in column-major order.
   * @param lightIntensity Illumination intensity. Combined with diffuse and specular material
   *     properties.
   * @see #setBlendMode(BlendMode)
   * @see #updateModelMatrix(float[], float)
   * @see #setMaterialProperties(float, float, float, float)
   * @see android.opengl.Matrix
   */
  public void draw(float[] cameraView, float[] cameraPerspective, float lightIntensity) {

    ShaderUtil.checkGLError(TAG, "Before draw");

    // Build the ModelView and ModelViewProjection matrices
    // for calculating object position and light.
    Matrix.multiplyMM(modelViewMatrix, 0, cameraView, 0, modelMatrix, 0);
    Matrix.multiplyMM(modelViewProjectionMatrix, 0, cameraPerspective, 0, modelViewMatrix, 0);

    GLES20.glUseProgram(program);

    // Set the lighting environment properties.
    Matrix.multiplyMV(viewLightDirection, 0, modelViewMatrix, 0, LIGHT_DIRECTION, 0);
    normalizeVec3(viewLightDirection);
    GLES20.glUniform4f(
        lightingParametersUniform,
        viewLightDirection[0],
        viewLightDirection[1],
        viewLightDirection[2],
        lightIntensity);

    // Set the object material properties.
    GLES20.glUniform4f(materialParametersUniform, ambient, diffuse, specular, specularPower);

    // Attach the object texture.
    GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
    GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textures[0]);
    GLES20.glUniform1i(textureUniform, 0);

    // Set the vertex attributes.
    GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, vertexBufferId);

    GLES20.glVertexAttribPointer(
        positionAttribute, COORDS_PER_VERTEX, GLES20.GL_FLOAT, false, 0, verticesBaseAddress);
    GLES20.glVertexAttribPointer(normalAttribute, 3, GLES20.GL_FLOAT, false, 0, normalsBaseAddress);
    GLES20.glVertexAttribPointer(
        texCoordAttribute, 2, GLES20.GL_FLOAT, false, 0, texCoordsBaseAddress);

    GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, 0);

    // Set the ModelViewProjection matrix in the shader.
    GLES20.glUniformMatrix4fv(modelViewUniform, 1, false, modelViewMatrix, 0);
    GLES20.glUniformMatrix4fv(modelViewProjectionUniform, 1, false, modelViewProjectionMatrix, 0);

    // Enable vertex arrays
    GLES20.glEnableVertexAttribArray(positionAttribute);
    GLES20.glEnableVertexAttribArray(normalAttribute);
    GLES20.glEnableVertexAttribArray(texCoordAttribute);

    if (blendMode != null) {
      GLES20.glDepthMask(false);
      GLES20.glEnable(GLES20.GL_BLEND);
      switch (blendMode) {
        case Shadow:
          // Multiplicative blending function for Shadow.
          GLES20.glBlendFunc(GLES20.GL_ZERO, GLES20.GL_ONE_MINUS_SRC_ALPHA);
          break;
        case Grid:
          // Grid, additive blending function.
          GLES20.glBlendFunc(GLES20.GL_SRC_ALPHA, GLES20.GL_ONE_MINUS_SRC_ALPHA);
          break;
      }
    }

    GLES20.glBindBuffer(GLES20.GL_ELEMENT_ARRAY_BUFFER, indexBufferId);
    GLES20.glDrawElements(GLES20.GL_TRIANGLES, indexCount, GLES20.GL_UNSIGNED_SHORT, 0);
    GLES20.glBindBuffer(GLES20.GL_ELEMENT_ARRAY_BUFFER, 0);

    if (blendMode != null) {
      GLES20.glDisable(GLES20.GL_BLEND);
      GLES20.glDepthMask(true);
    }

    // Disable vertex arrays
    GLES20.glDisableVertexAttribArray(positionAttribute);
    GLES20.glDisableVertexAttribArray(normalAttribute);
    GLES20.glDisableVertexAttribArray(texCoordAttribute);

    GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);

    ShaderUtil.checkGLError(TAG, "After draw");
  }

  private static void normalizeVec3(float[] v) {
    float reciprocalLength = 1.0f / (float) Math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    v[0] *= reciprocalLength;
    v[1] *= reciprocalLength;
    v[2] *= reciprocalLength;
  }
}
