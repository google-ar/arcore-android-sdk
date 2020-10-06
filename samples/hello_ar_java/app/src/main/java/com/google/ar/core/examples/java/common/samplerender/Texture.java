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

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.opengl.GLES11Ext;
import android.opengl.GLES30;
import android.opengl.GLUtils;
import android.util.Log;
import java.io.Closeable;
import java.io.IOException;

/** A GPU-side texture. */
public class Texture implements Closeable {
  private static final String TAG = Texture.class.getSimpleName();

  private final int[] textureId = {0};
  private final Target target;

  /**
   * Describes the way the texture's edges are rendered.
   *
   * @see <a
   *     href="https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glTexParameter.xhtml">GL_TEXTURE_WRAP_S</a>.
   */
  public enum WrapMode {
    CLAMP_TO_EDGE(GLES30.GL_CLAMP_TO_EDGE),
    MIRRORED_REPEAT(GLES30.GL_MIRRORED_REPEAT),
    REPEAT(GLES30.GL_REPEAT);

    /* package-private */
    final int glesEnum;

    private WrapMode(int glesEnum) {
      this.glesEnum = glesEnum;
    }
  }

  /**
   * Describes the target this texture is bound to.
   *
   * @see <a
   *     href="https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glBindTexture.xhtml">glBindTexture</a>.
   */
  public enum Target {
    TEXTURE_2D(GLES30.GL_TEXTURE_2D),
    TEXTURE_EXTERNAL_OES(GLES11Ext.GL_TEXTURE_EXTERNAL_OES),
    TEXTURE_CUBE_MAP(GLES30.GL_TEXTURE_CUBE_MAP);

    final int glesEnum;

    private Target(int glesEnum) {
      this.glesEnum = glesEnum;
    }
  }

  /**
   * Construct an empty {@link Texture}.
   *
   * <p>Since {@link Texture}s created in this way are not populated with data, this method is
   * mostly only useful for creating {@link Target.TEXTURE_EXTERNAL_OES} textures. See {@link
   * #createFromAsset} if you want a texture with data.
   */
  public Texture(SampleRender render, Target target, WrapMode wrapMode) {
    this.target = target;

    GLES30.glGenTextures(1, textureId, 0);
    GLError.maybeThrowGLException("Texture creation failed", "glGenTextures");

    try {
      GLES30.glBindTexture(target.glesEnum, textureId[0]);
      GLError.maybeThrowGLException("Failed to bind texture", "glBindTexture");
      GLES30.glTexParameteri(target.glesEnum, GLES30.GL_TEXTURE_MIN_FILTER, GLES30.GL_LINEAR);
      GLError.maybeThrowGLException("Failed to set texture parameter", "glTexParameteri");
      GLES30.glTexParameteri(target.glesEnum, GLES30.GL_TEXTURE_MAG_FILTER, GLES30.GL_LINEAR);
      GLError.maybeThrowGLException("Failed to set texture parameter", "glTexParameteri");

      GLES30.glTexParameteri(target.glesEnum, GLES30.GL_TEXTURE_WRAP_S, wrapMode.glesEnum);
      GLError.maybeThrowGLException("Failed to set texture parameter", "glTexParameteri");
      GLES30.glTexParameteri(target.glesEnum, GLES30.GL_TEXTURE_WRAP_T, wrapMode.glesEnum);
      GLError.maybeThrowGLException("Failed to set texture parameter", "glTexParameteri");
    } catch (Throwable t) {
      close();
      throw t;
    }
  }

  /** Create a texture from the given asset file name. */
  public static Texture createFromAsset(
      SampleRender render, String assetFileName, WrapMode wrapMode) throws IOException {
    Texture texture = new Texture(render, Target.TEXTURE_2D, wrapMode);
    try {
      GLES30.glBindTexture(GLES30.GL_TEXTURE_2D, texture.getTextureId());
      Bitmap bitmap = BitmapFactory.decodeStream(render.getAssets().open(assetFileName));
      GLUtils.texImage2D(GLES30.GL_TEXTURE_2D, 0, bitmap, 0);
      bitmap.recycle();
      GLError.maybeThrowGLException("Failed to populate texture data", "GLUtils.texImage2D");
      GLES30.glGenerateMipmap(GLES30.GL_TEXTURE_2D);
      GLError.maybeThrowGLException("Failed to generate mipmaps", "glGenerateMipmap");
    } catch (Throwable t) {
      texture.close();
      throw t;
    }
    return texture;
  }

  @Override
  public void close() {
    if (textureId[0] != 0) {
      GLES30.glDeleteTextures(1, textureId, 0);
      GLError.maybeLogGLError(Log.WARN, TAG, "Failed to free texture", "glDeleteTextures");
      textureId[0] = 0;
    }
  }

  /** Retrieve the native texture ID. */
  public int getTextureId() {
    return textureId[0];
  }

  /* package-private */
  Target getTarget() {
    return target;
  }
}
