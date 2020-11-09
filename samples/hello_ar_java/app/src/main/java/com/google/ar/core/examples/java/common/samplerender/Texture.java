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
import android.util.Log;
import java.io.Closeable;
import java.io.IOException;
import java.nio.ByteBuffer;

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
   * Describes the color format of the texture.
   *
   * @see <a
   *     href="https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glTexImage2D.xhtml">glTexImage2d</a>.
   */
  public enum ColorFormat {
    LINEAR(GLES30.GL_RGBA8),
    SRGB(GLES30.GL_SRGB8_ALPHA8);

    final int glesEnum;

    private ColorFormat(int glesEnum) {
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
      SampleRender render, String assetFileName, WrapMode wrapMode, ColorFormat colorFormat)
      throws IOException {
    Texture texture = new Texture(render, Target.TEXTURE_2D, wrapMode);
    Bitmap bitmap = null;
    try {
      // The following lines up to glTexImage2D could technically be replaced with
      // GLUtils.texImage2d, but this method does not allow for loading sRGB images.

      // Load and convert the bitmap and copy its contents to a direct ByteBuffer. Despite its name,
      // the ARGB_8888 config is actually stored in RGBA order.
      bitmap =
          convertBitmapToConfig(
              BitmapFactory.decodeStream(render.getAssets().open(assetFileName)),
              Bitmap.Config.ARGB_8888);
      ByteBuffer buffer = ByteBuffer.allocateDirect(bitmap.getByteCount());
      bitmap.copyPixelsToBuffer(buffer);
      buffer.rewind();

      GLES30.glBindTexture(GLES30.GL_TEXTURE_2D, texture.getTextureId());
      GLError.maybeThrowGLException("Failed to bind texture", "glBindTexture");
      GLES30.glTexImage2D(
          GLES30.GL_TEXTURE_2D,
          /*level=*/ 0,
          colorFormat.glesEnum,
          bitmap.getWidth(),
          bitmap.getHeight(),
          /*border=*/ 0,
          GLES30.GL_RGBA,
          GLES30.GL_UNSIGNED_BYTE,
          buffer);
      GLError.maybeThrowGLException("Failed to populate texture data", "glTexImage2D");
      GLES30.glGenerateMipmap(GLES30.GL_TEXTURE_2D);
      GLError.maybeThrowGLException("Failed to generate mipmaps", "glGenerateMipmap");
    } catch (Throwable t) {
      texture.close();
      throw t;
    } finally {
      if (bitmap != null) {
        bitmap.recycle();
      }
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

  private static Bitmap convertBitmapToConfig(Bitmap bitmap, Bitmap.Config config) {
    // We use this method instead of BitmapFactory.Options.outConfig to support a minimum of Android
    // API level 24.
    if (bitmap.getConfig() == config) {
      return bitmap;
    }
    Bitmap result = bitmap.copy(config, /*isMutable=*/ false);
    bitmap.recycle();
    return result;
  }
}
