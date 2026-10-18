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
package com.google.ar.core.examples.java.common.samplerender;

import static java.nio.charset.StandardCharsets.UTF_8;

import android.content.res.AssetManager;
import android.opengl.GLES30;
import android.opengl.GLException;
import android.util.Log;
import java.io.Closeable;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.regex.Matcher;

/**
 * Represents a GPU shader, the state of its associated uniforms, and some additional draw state.
 */
public class Shader implements Closeable {
  private static final String TAG = Shader.class.getSimpleName();

  /**
   * A factor to be used in a blend function.
   *
   * @see <a
   *     href="https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glBlendFunc.xhtml">glBlendFunc</a>
   */
  public static enum BlendFactor {
    ZERO(GLES30.GL_ZERO),
    ONE(GLES30.GL_ONE),
    SRC_COLOR(GLES30.GL_SRC_COLOR),
    ONE_MINUS_SRC_COLOR(GLES30.GL_ONE_MINUS_SRC_COLOR),
    DST_COLOR(GLES30.GL_DST_COLOR),
    ONE_MINUS_DST_COLOR(GLES30.GL_ONE_MINUS_DST_COLOR),
    SRC_ALPHA(GLES30.GL_SRC_ALPHA),
    ONE_MINUS_SRC_ALPHA(GLES30.GL_ONE_MINUS_SRC_ALPHA),
    DST_ALPHA(GLES30.GL_DST_ALPHA),
    ONE_MINUS_DST_ALPHA(GLES30.GL_ONE_MINUS_DST_ALPHA),
    CONSTANT_COLOR(GLES30.GL_CONSTANT_COLOR),
    ONE_MINUS_CONSTANT_COLOR(GLES30.GL_ONE_MINUS_CONSTANT_COLOR),
    CONSTANT_ALPHA(GLES30.GL_CONSTANT_ALPHA),
    ONE_MINUS_CONSTANT_ALPHA(GLES30.GL_ONE_MINUS_CONSTANT_ALPHA);

    /* package-private */
    final int glesEnum;

    private BlendFactor(int glesEnum) {
      this.glesEnum = glesEnum;
    }
  }

  private int programId = 0;
  private final Map<Integer, Uniform> uniforms = new HashMap<>();
  private int maxTextureUnit = 0;

  private final Map<String, Integer> uniformLocations = new HashMap<>();
  private final Map<Integer, String> uniformNames = new HashMap<>();

  private boolean depthTest = true;
  private boolean depthWrite = true;
  private boolean cullFace = true;
  private BlendFactor sourceRgbBlend = BlendFactor.ONE;
  private BlendFactor destRgbBlend = BlendFactor.ZERO;
  private BlendFactor sourceAlphaBlend = BlendFactor.ONE;
  private BlendFactor destAlphaBlend = BlendFactor.ZERO;

  /**
   * Constructs a {@link Shader} given the shader code.
   *
   * @param defines A map of shader precompiler symbols to be defined with the given names and
   *     values
   */
  public Shader(
      SampleRender render,
      String vertexShaderCode,
      String fragmentShaderCode,
      Map<String, String> defines) {
    int vertexShaderId = 0;
    int fragmentShaderId = 0;
    String definesCode = createShaderDefinesCode(defines);
    try {
      vertexShaderId =
          createShader(
              GLES30.GL_VERTEX_SHADER, insertShaderDefinesCode(vertexShaderCode, definesCode));
      fragmentShaderId =
          createShader(
              GLES30.GL_FRAGMENT_SHADER, insertShaderDefinesCode(fragmentShaderCode, definesCode));

      programId = GLES30.glCreateProgram();
      GLError.maybeThrowGLException("Shader program creation failed", "glCreateProgram");
      GLES30.glAttachShader(programId, vertexShaderId);
      GLError.maybeThrowGLException("Failed to attach vertex shader", "glAttachShader");
      GLES30.glAttachShader(programId, fragmentShaderId);
      GLError.maybeThrowGLException("Failed to attach fragment shader", "glAttachShader");
      GLES30.glLinkProgram(programId);
      GLError.maybeThrowGLException("Failed to link shader program", "glLinkProgram");

      final int[] linkStatus = new int[1];
      GLES30.glGetProgramiv(programId, GLES30.GL_LINK_STATUS, linkStatus, 0);
      if (linkStatus[0] == GLES30.GL_FALSE) {
        String infoLog = GLES30.glGetProgramInfoLog(programId);
        GLError.maybeLogGLError(
            Log.WARN, TAG, "Failed to retrieve shader program info log", "glGetProgramInfoLog");
        throw new GLException(0, "Shader link failed: " + infoLog);
      }
    } catch (Throwable t) {
      close();
      throw t;
    } finally {
      // Shader objects can be flagged for deletion immediately after program creation.
      if (vertexShaderId != 0) {
        GLES30.glDeleteShader(vertexShaderId);
        GLError.maybeLogGLError(Log.WARN, TAG, "Failed to free vertex shader", "glDeleteShader");
      }
      if (fragmentShaderId != 0) {
        GLES30.glDeleteShader(fragmentShaderId);
        GLError.maybeLogGLError(Log.WARN, TAG, "Failed to free fragment shader", "glDeleteShader");
      }
    }
  }

  /**
   * Creates a {@link Shader} from the given asset file names.
   *
   * <p>The file contents are interpreted as UTF-8 text.
   *
   * @param defines A map of shader precompiler symbols to be defined with the given names and
   *     values
   */
  public static Shader createFromAssets(
      SampleRender render,
      String vertexShaderFileName,
      String fragmentShaderFileName,
      Map<String, String> defines)
      throws IOException {
    AssetManager assets = render.getAssets();
    return new Shader(
        render,
        inputStreamToString(assets.open(vertexShaderFileName)),
        inputStreamToString(assets.open(fragmentShaderFileName)),
        defines);
  }

  @Override
  public void close() {
    if (programId != 0) {
      GLES30.glDeleteProgram(programId);
      programId = 0;
    }
  }

  /**
   * Sets depth test state.
   *
   * @see <a
   *     href="https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glEnable.xhtml">glEnable(GL_DEPTH_TEST)</a>.
   */
  public Shader setDepthTest(boolean depthTest) {
    this.depthTest = depthTest;
    return this;
  }

  /**
   * Sets depth write state.
   *
   * @see <a
   *     href="https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glDepthMask.xhtml">glDepthMask</a>.
   */
  public Shader setDepthWrite(boolean depthWrite) {
    this.depthWrite = depthWrite;
    return this;
  }

  /**
   * Sets cull face state.
   *
   * @see <a
   *     href="https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glEnable.xhtml">glEnable(GL_CULL_FACE)</a>.
   */
  public Shader setCullFace(boolean cullFace) {
    this.cullFace = cullFace;
    return this;
  }

  /**
   * Sets blending function.
   *
   * @see <a
   *     href="https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBlendFunc.xhtml">glBlendFunc</a>
   */
  public Shader setBlend(BlendFactor sourceBlend, BlendFactor destBlend) {
    this.sourceRgbBlend = sourceBlend;
    this.destRgbBlend = destBlend;
    this.sourceAlphaBlend = sourceBlend;
    this.destAlphaBlend = destBlend;
    return this;
  }

  /**
   * Sets blending functions separately for RGB and alpha channels.
   *
   * @see <a
   *     href="https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glBlendFuncSeparate.xhtml">glBlendFunc</a>
   */
  public Shader setBlend(
      BlendFactor sourceRgbBlend,
      BlendFactor destRgbBlend,
      BlendFactor sourceAlphaBlend,
      BlendFactor destAlphaBlend) {
    this.sourceRgbBlend = sourceRgbBlend;
    this.destRgbBlend = destRgbBlend;
    this.sourceAlphaBlend = sourceAlphaBlend;
    this.destAlphaBlend = destAlphaBlend;
    return this;
  }

  /** Sets a texture uniform. */
  public Shader setTexture(String name, Texture texture) {
    // Special handling for Textures. If replacing an existing texture uniform, reuse the texture
    // unit.
    int location = getUniformLocation(name);
    Uniform uniform = uniforms.get(location);
    int textureUnit;
    if (!(uniform instanceof UniformTexture)) {
      textureUnit = maxTextureUnit++;
    } else {
      UniformTexture uniformTexture = (UniformTexture) uniform;
      textureUnit = uniformTexture.getTextureUnit();
    }
    uniforms.put(location, new UniformTexture(textureUnit, texture));
    return this;
  }

  /** Sets a {@code bool} uniform. */
  public Shader setBool(String name, boolean v0) {
    int[] values = {v0 ? 1 : 0};
    uniforms.put(getUniformLocation(name), new UniformInt(values));
    return this;
  }

  /** Sets an {@code int} uniform. */
  public Shader setInt(String name, int v0) {
    int[] values = {v0};
    uniforms.put(getUniformLocation(name), new UniformInt(values));
    return this;
  }

  /** Sets a {@code float} uniform. */
  public Shader setFloat(String name, float v0) {
    float[] values = {v0};
    uniforms.put(getUniformLocation(name), new Uniform1f(values));
    return this;
  }

  /** Sets a {@code vec2} uniform. */
  public Shader setVec2(String name, float[] values) {
    if (values.length != 2) {
      throw new IllegalArgumentException("Value array length must be 2");
    }
    uniforms.put(getUniformLocation(name), new Uniform2f(values.clone()));
    return this;
  }
  /** Sets a {@code vec3} uniform. */
  public Shader setVec3(String name, float[] values) {
    if (values.length != 3) {
      throw new IllegalArgumentException("Value array length must be 3");
    }
    uniforms.put(getUniformLocation(name), new Uniform3f(values.clone()));
    return this;
  }

  /** Sets a {@code vec4} uniform. */
  public Shader setVec4(String name, float[] values) {
    if (values.length != 4) {
      throw new IllegalArgumentException("Value array length must be 4");
    }
    uniforms.put(getUniformLocation(name), new Uniform4f(values.clone()));
    return this;
  }

  /** Sets a {@code mat2} uniform. */
  public Shader setMat2(String name, float[] values) {
    if (values.length != 4) {
      throw new IllegalArgumentException("Value array length must be 4 (2x2)");
    }
    uniforms.put(getUniformLocation(name), new UniformMatrix2f(values.clone()));
    return this;
  }

  /** Sets a {@code mat3} uniform. */
  public Shader setMat3(String name, float[] values) {
    if (values.length != 9) {
      throw new IllegalArgumentException("Value array length must be 9 (3x3)");
    }
    uniforms.put(getUniformLocation(name), new UniformMatrix3f(values.clone()));
    return this;
  }

  /** Sets a {@code mat4} uniform. */
  public Shader setMat4(String name, float[] values) {
    if (values.length != 16) {
      throw new IllegalArgumentException("Value array length must be 16 (4x4)");
    }
    uniforms.put(getUniformLocation(name), new UniformMatrix4f(values.clone()));
    return this;
  }

  /** Sets a {@code bool} array uniform. */
  public Shader setBoolArray(String name, boolean[] values) {
    int[] intValues = new int[values.length];
    for (int i = 0; i < values.length; ++i) {
      intValues[i] = values[i] ? 1 : 0;
    }
    uniforms.put(getUniformLocation(name), new UniformInt(intValues));
    return this;
  }

  /** Sets an {@code int} array uniform. */
  public Shader setIntArray(String name, int[] values) {
    uniforms.put(getUniformLocation(name), new UniformInt(values.clone()));
    return this;
  }

  /** Sets a {@code float} array uniform. */
  public Shader setFloatArray(String name, float[] values) {
    uniforms.put(getUniformLocation(name), new Uniform1f(values.clone()));
    return this;
  }

  /** Sets a {@code vec2} array uniform. */
  public Shader setVec2Array(String name, float[] values) {
    if (values.length % 2 != 0) {
      throw new IllegalArgumentException("Value array length must be divisible by 2");
    }
    uniforms.put(getUniformLocation(name), new Uniform2f(values.clone()));
    return this;
  }
  /** Sets a {@code vec3} array uniform. */
  public Shader setVec3Array(String name, float[] values) {
    if (values.length % 3 != 0) {
      throw new IllegalArgumentException("Value array length must be divisible by 3");
    }
    uniforms.put(getUniformLocation(name), new Uniform3f(values.clone()));
    return this;
  }

  /** Sets a {@code vec4} array uniform. */
  public Shader setVec4Array(String name, float[] values) {
    if (values.length % 4 != 0) {
      throw new IllegalArgumentException("Value array length must be divisible by 4");
    }
    uniforms.put(getUniformLocation(name), new Uniform4f(values.clone()));
    return this;
  }

  /** Sets a {@code mat2} array uniform. */
  public Shader setMat2Array(String name, float[] values) {
    if (values.length % 4 != 0) {
      throw new IllegalArgumentException("Value array length must be divisible by 4 (2x2)");
    }
    uniforms.put(getUniformLocation(name), new UniformMatrix2f(values.clone()));
    return this;
  }

  /** Sets a {@code mat3} array uniform. */
  public Shader setMat3Array(String name, float[] values) {
    if (values.length % 9 != 0) {
      throw new IllegalArgumentException("Values array length must be divisible by 9 (3x3)");
    }
    uniforms.put(getUniformLocation(name), new UniformMatrix3f(values.clone()));
    return this;
  }

  /** Sets a {@code mat4} uniform. */
  public Shader setMat4Array(String name, float[] values) {
    if (values.length % 16 != 0) {
      throw new IllegalArgumentException("Value array length must be divisible by 16 (4x4)");
    }
    uniforms.put(getUniformLocation(name), new UniformMatrix4f(values.clone()));
    return this;
  }

  /**
   * Activates the shader. Don't call this directly unless you are doing low level OpenGL code;
   * instead, prefer {@link SampleRender#draw}.
   */
  public void lowLevelUse() {
    // Make active shader/set uniforms
    if (programId == 0) {
      throw new IllegalStateException("Attempted to use freed shader");
    }
    GLES30.glUseProgram(programId);
    GLError.maybeThrowGLException("Failed to use shader program", "glUseProgram");
    GLES30.glBlendFuncSeparate(
        sourceRgbBlend.glesEnum,
        destRgbBlend.glesEnum,
        sourceAlphaBlend.glesEnum,
        destAlphaBlend.glesEnum);
    GLError.maybeThrowGLException("Failed to set blend mode", "glBlendFuncSeparate");
    GLES30.glDepthMask(depthWrite);
    GLError.maybeThrowGLException("Failed to set depth write mask", "glDepthMask");
    if (depthTest) {
      GLES30.glEnable(GLES30.GL_DEPTH_TEST);
      GLError.maybeThrowGLException("Failed to enable depth test", "glEnable");
    } else {
      GLES30.glDisable(GLES30.GL_DEPTH_TEST);
      GLError.maybeThrowGLException("Failed to disable depth test", "glDisable");
    }
    if (cullFace) {
      GLES30.glEnable(GLES30.GL_CULL_FACE);
      GLError.maybeThrowGLException("Failed to enable backface culling", "glEnable");
    } else {
      GLES30.glDisable(GLES30.GL_CULL_FACE);
      GLError.maybeThrowGLException("Failed to disable backface culling", "glDisable");
    }
    try {
      // Remove all non-texture uniforms from the map after setting them, since they're stored as
      // part of the program.
      ArrayList<Integer> obsoleteEntries = new ArrayList<>(uniforms.size());
      for (Map.Entry<Integer, Uniform> entry : uniforms.entrySet()) {
        try {
          entry.getValue().use(entry.getKey());
          if (!(entry.getValue() instanceof UniformTexture)) {
            obsoleteEntries.add(entry.getKey());
          }
        } catch (GLException e) {
          String name = uniformNames.get(entry.getKey());
          throw new IllegalArgumentException("Error setting uniform `" + name + "'", e);
        }
      }
      uniforms.keySet().removeAll(obsoleteEntries);
    } finally {
      GLES30.glActiveTexture(GLES30.GL_TEXTURE0);
      GLError.maybeLogGLError(Log.WARN, TAG, "Failed to set active texture", "glActiveTexture");
    }
  }

  private static interface Uniform {
    public void use(int location);
  }

  private static class UniformTexture implements Uniform {
    private final int textureUnit;
    private final Texture texture;

    public UniformTexture(int textureUnit, Texture texture) {
      this.textureUnit = textureUnit;
      this.texture = texture;
    }

    public int getTextureUnit() {
      return textureUnit;
    }

    @Override
    public void use(int location) {
      if (texture.getTextureId() == 0) {
        throw new IllegalStateException("Tried to draw with freed texture");
      }
      GLES30.glActiveTexture(GLES30.GL_TEXTURE0 + textureUnit);
      GLError.maybeThrowGLException("Failed to set active texture", "glActiveTexture");
      GLES30.glBindTexture(texture.getTarget().glesEnum, texture.getTextureId());
      GLError.maybeThrowGLException("Failed to bind texture", "glBindTexture");
      GLES30.glUniform1i(location, textureUnit);
      GLError.maybeThrowGLException("Failed to set shader texture uniform", "glUniform1i");
    }
  }

  private static class UniformInt implements Uniform {
    private final int[] values;

    public UniformInt(int[] values) {
      this.values = values;
    }

    @Override
    public void use(int location) {
      GLES30.glUniform1iv(location, values.length, values, 0);
      GLError.maybeThrowGLException("Failed to set shader uniform 1i", "glUniform1iv");
    }
  }

  private static class Uniform1f implements Uniform {
    private final float[] values;

    public Uniform1f(float[] values) {
      this.values = values;
    }

    @Override
    public void use(int location) {
      GLES30.glUniform1fv(location, values.length, values, 0);
      GLError.maybeThrowGLException("Failed to set shader uniform 1f", "glUniform1fv");
    }
  }

  private static class Uniform2f implements Uniform {
    private final float[] values;

    public Uniform2f(float[] values) {
      this.values = values;
    }

    @Override
    public void use(int location) {
      GLES30.glUniform2fv(location, values.length / 2, values, 0);
      GLError.maybeThrowGLException("Failed to set shader uniform 2f", "glUniform2fv");
    }
  }

  private static class Uniform3f implements Uniform {
    private final float[] values;

    public Uniform3f(float[] values) {
      this.values = values;
    }

    @Override
    public void use(int location) {
      GLES30.glUniform3fv(location, values.length / 3, values, 0);
      GLError.maybeThrowGLException("Failed to set shader uniform 3f", "glUniform3fv");
    }
  }

  private static class Uniform4f implements Uniform {
    private final float[] values;

    public Uniform4f(float[] values) {
      this.values = values;
    }

    @Override
    public void use(int location) {
      GLES30.glUniform4fv(location, values.length / 4, values, 0);
      GLError.maybeThrowGLException("Failed to set shader uniform 4f", "glUniform4fv");
    }
  }

  private static class UniformMatrix2f implements Uniform {
    private final float[] values;

    public UniformMatrix2f(float[] values) {
      this.values = values;
    }

    @Override
    public void use(int location) {
      GLES30.glUniformMatrix2fv(location, values.length / 4, /*transpose=*/ false, values, 0);
      GLError.maybeThrowGLException("Failed to set shader uniform matrix 2f", "glUniformMatrix2fv");
    }
  }

  private static class UniformMatrix3f implements Uniform {
    private final float[] values;

    public UniformMatrix3f(float[] values) {
      this.values = values;
    }

    @Override
    public void use(int location) {
      GLES30.glUniformMatrix3fv(location, values.length / 9, /*transpose=*/ false, values, 0);
      GLError.maybeThrowGLException("Failed to set shader uniform matrix 3f", "glUniformMatrix3fv");
    }
  }

  private static class UniformMatrix4f implements Uniform {
    private final float[] values;

    public UniformMatrix4f(float[] values) {
      this.values = values;
    }

    @Override
    public void use(int location) {
      GLES30.glUniformMatrix4fv(location, values.length / 16, /*transpose=*/ false, values, 0);
      GLError.maybeThrowGLException("Failed to set shader uniform matrix 4f", "glUniformMatrix4fv");
    }
  }

  private int getUniformLocation(String name) {
    Integer locationObject = uniformLocations.get(name);
    if (locationObject != null) {
      return locationObject;
    }
    int location = GLES30.glGetUniformLocation(programId, name);
    GLError.maybeThrowGLException("Failed to find uniform", "glGetUniformLocation");
    if (location == -1) {
      throw new IllegalArgumentException("Shader uniform does not exist: " + name);
    }
    uniformLocations.put(name, Integer.valueOf(location));
    uniformNames.put(Integer.valueOf(location), name);
    return location;
  }

  private static int createShader(int type, String code) {
    int shaderId = GLES30.glCreateShader(type);
    GLError.maybeThrowGLException("Shader creation failed", "glCreateShader");
    GLES30.glShaderSource(shaderId, code);
    GLError.maybeThrowGLException("Shader source failed", "glShaderSource");
    GLES30.glCompileShader(shaderId);
    GLError.maybeThrowGLException("Shader compilation failed", "glCompileShader");

    final int[] compileStatus = new int[1];
    GLES30.glGetShaderiv(shaderId, GLES30.GL_COMPILE_STATUS, compileStatus, 0);
    if (compileStatus[0] == GLES30.GL_FALSE) {
      String infoLog = GLES30.glGetShaderInfoLog(shaderId);
      GLError.maybeLogGLError(
          Log.WARN, TAG, "Failed to retrieve shader info log", "glGetShaderInfoLog");
      GLES30.glDeleteShader(shaderId);
      GLError.maybeLogGLError(Log.WARN, TAG, "Failed to free shader", "glDeleteShader");
      throw new GLException(0, "Shader compilation failed: " + infoLog);
    }

    return shaderId;
  }

  private static String createShaderDefinesCode(Map<String, String> defines) {
    if (defines == null) {
      return "";
    }
    StringBuilder builder = new StringBuilder();
    for (Map.Entry<String, String> entry : defines.entrySet()) {
      builder.append("#define " + entry.getKey() + " " + entry.getValue() + "\n");
    }
    return builder.toString();
  }

  private static String insertShaderDefinesCode(String sourceCode, String definesCode) {
    String result =
        sourceCode.replaceAll(
            "(?m)^(\\s*#\\s*version\\s+.*)$", "$1\n" + Matcher.quoteReplacement(definesCode));
    if (result.equals(sourceCode)) {
      // No #version specified, so just prepend source
      return definesCode + sourceCode;
    }
    return result;
  }

  private static String inputStreamToString(InputStream stream) throws IOException {
    InputStreamReader reader = new InputStreamReader(stream, UTF_8.name());
    char[] buffer = new char[1024 * 4];
    StringBuilder builder = new StringBuilder();
    int amount = 0;
    while ((amount = reader.read(buffer)) != -1) {
      builder.append(buffer, 0, amount);
    }
    reader.close();
    return builder.toString();
  }
}
