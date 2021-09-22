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

import static java.lang.Math.max;
import static java.lang.Math.min;

import android.opengl.GLES30;
import android.util.Log;
import com.google.ar.core.ArImage;
import com.google.ar.core.ImageFormat;
import com.google.ar.core.examples.java.common.samplerender.GLError;
import com.google.ar.core.examples.java.common.samplerender.Mesh;
import com.google.ar.core.examples.java.common.samplerender.SampleRender;
import com.google.ar.core.examples.java.common.samplerender.Shader;
import com.google.ar.core.examples.java.common.samplerender.Texture;
import com.google.ar.core.examples.java.common.samplerender.VertexBuffer;
import java.io.Closeable;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;

/**
 * Filters a provided cubemap into a cubemap lookup texture which is a function of the direction of
 * a reflected ray of light and material roughness, i.e. the LD term of the specular IBL
 * calculation.
 *
 * <p>See https://google.github.io/filament/Filament.md.html#lighting/imagebasedlights for a more
 * detailed explanation.
 */
public class SpecularCubemapFilter implements Closeable {
  private static final String TAG = SpecularCubemapFilter.class.getSimpleName();

  private static final int COMPONENTS_PER_VERTEX = 2;
  private static final int NUMBER_OF_VERTICES = 4;
  private static final int FLOAT_SIZE = 4;
  private static final int COORDS_BUFFER_SIZE =
      COMPONENTS_PER_VERTEX * NUMBER_OF_VERTICES * FLOAT_SIZE;

  private static final int NUMBER_OF_CUBE_FACES = 6;

  private static final FloatBuffer COORDS_BUFFER =
      ByteBuffer.allocateDirect(COORDS_BUFFER_SIZE).order(ByteOrder.nativeOrder()).asFloatBuffer();

  static {
    COORDS_BUFFER.put(
        new float[] {
          /*0:*/ -1f, -1f, /*1:*/ +1f, -1f, /*2:*/ -1f, +1f, /*3:*/ +1f, +1f,
        });
  }

  private static final String[] ATTACHMENT_LOCATION_DEFINES = {
    "PX_LOCATION", "NX_LOCATION", "PY_LOCATION", "NY_LOCATION", "PZ_LOCATION", "NZ_LOCATION",
  };

  private static final int[] ATTACHMENT_ENUMS = {
    GLES30.GL_COLOR_ATTACHMENT0,
    GLES30.GL_COLOR_ATTACHMENT1,
    GLES30.GL_COLOR_ATTACHMENT2,
    GLES30.GL_COLOR_ATTACHMENT3,
    GLES30.GL_COLOR_ATTACHMENT4,
    GLES30.GL_COLOR_ATTACHMENT5,
  };

  // We need to create enough shaders and framebuffers to encompass every face of the cubemap. Each
  // color attachment is used by the framebuffer to render to a different face of the cubemap, so we
  // use "chunks" which define as many color attachments as possible for each face. For example, if
  // we have a maximum of 3 color attachments, we must create two shaders with the following color
  // attachments:
  //
  // layout(location = 0) out vec4 o_FragColorPX;
  // layout(location = 1) out vec4 o_FragColorNX;
  // layout(location = 2) out vec4 o_FragColorPY;
  //
  // and
  //
  // layout(location = 0) out vec4 o_FragColorNY;
  // layout(location = 1) out vec4 o_FragColorPZ;
  // layout(location = 2) out vec4 o_FragColorNZ;
  private static class Chunk {
    public final int chunkIndex;
    public final int chunkSize;
    public final int firstFaceIndex;

    public Chunk(int chunkIndex, int maxChunkSize) {
      this.chunkIndex = chunkIndex;
      this.firstFaceIndex = chunkIndex * maxChunkSize;
      this.chunkSize = min(maxChunkSize, NUMBER_OF_CUBE_FACES - this.firstFaceIndex);
    }
  }

  private static class ChunkIterable implements Iterable<Chunk> {
    public final int maxChunkSize;
    public final int numberOfChunks;

    public ChunkIterable(int maxNumberOfColorAttachments) {
      this.maxChunkSize = min(maxNumberOfColorAttachments, NUMBER_OF_CUBE_FACES);
      int numberOfChunks = NUMBER_OF_CUBE_FACES / this.maxChunkSize;
      if (NUMBER_OF_CUBE_FACES % this.maxChunkSize != 0) {
        numberOfChunks++;
      }
      this.numberOfChunks = numberOfChunks;
    }

    @Override
    public Iterator<Chunk> iterator() {
      return new Iterator<Chunk>() {
        private Chunk chunk = new Chunk(/*chunkIndex=*/ 0, maxChunkSize);

        @Override
        public boolean hasNext() {
          return chunk.chunkIndex < numberOfChunks;
        }

        @Override
        public Chunk next() {
          Chunk result = this.chunk;
          this.chunk = new Chunk(result.chunkIndex + 1, maxChunkSize);
          return result;
        }
      };
    }
  }

  private static class ImportanceSampleCacheEntry {
    public float[] direction;
    public float contribution;
    public float level;
  }

  private final int resolution;
  private final int numberOfImportanceSamples;
  private final int numberOfMipmapLevels;

  private final Texture radianceCubemap;
  private final Texture ldCubemap;
  // Indexed by attachment chunk.
  private final Shader[] shaders;
  private final Mesh mesh;

  // Using OpenGL directly here since cubemap framebuffers are very involved. Indexed by
  // [mipmapLevel][attachmentChunk].
  private final int[][] framebuffers;

  /**
   * Constructs a {@link SpecularCubemapFilter}.
   *
   * <p>The provided resolution refers to both the width and height of the input resolution and the
   * resolution of the highest mipmap level of the filtered cubemap texture.
   *
   * <p>Ideally, the cubemap would need to be filtered by computing a function of every sample over
   * the hemisphere for every texel. Since this is not practical to compute, a limited, discrete
   * number of importance samples are selected instead. A larger number of importance samples will
   * generally provide more accurate results, but in the case of ARCore, the cubemap estimations are
   * already very low resolution, and higher values provide rapidly diminishing returns.
   */
  public SpecularCubemapFilter(SampleRender render, int resolution, int numberOfImportanceSamples)
      throws IOException {
    this.resolution = resolution;
    this.numberOfImportanceSamples = numberOfImportanceSamples;
    this.numberOfMipmapLevels = log2(resolution) + 1;

    try {
      radianceCubemap =
          new Texture(render, Texture.Target.TEXTURE_CUBE_MAP, Texture.WrapMode.CLAMP_TO_EDGE);
      ldCubemap =
          new Texture(render, Texture.Target.TEXTURE_CUBE_MAP, Texture.WrapMode.CLAMP_TO_EDGE);

      ChunkIterable chunks = new ChunkIterable(getMaxColorAttachments());
      initializeLdCubemap();
      shaders = createShaders(render, chunks);
      framebuffers = createFramebuffers(chunks);

      // Create the quad mesh that encompasses the entire view.
      VertexBuffer coordsBuffer = new VertexBuffer(render, COMPONENTS_PER_VERTEX, COORDS_BUFFER);
      mesh =
          new Mesh(
              render,
              Mesh.PrimitiveMode.TRIANGLE_STRIP,
              /*indexBuffer=*/ null,
              new VertexBuffer[] {coordsBuffer});
    } catch (Throwable t) {
      close();
      throw t;
    }
  }

  @Override
  public void close() {
    if (framebuffers != null) {
      for (int[] framebufferChunks : framebuffers) {
        GLES30.glDeleteFramebuffers(framebufferChunks.length, framebufferChunks, 0);
        GLError.maybeLogGLError(
            Log.WARN, TAG, "Failed to free framebuffers", "glDeleteFramebuffers");
      }
    }
    if (radianceCubemap != null) {
      radianceCubemap.close();
    }
    if (ldCubemap != null) {
      ldCubemap.close();
    }
    if (shaders != null) {
      for (Shader shader : shaders) {
        shader.close();
      }
    }
  }

  /**
   * Updates and filters the provided cubemap textures from ARCore.
   *
   * <p>This method should be called every frame with the result of {@link
   * com.google.ar.core.LightEstimate.acquireEnvironmentalHdrCubeMap()} to update the filtered
   * cubemap texture, accessible via {@link getFilteredCubemapTexture()}.
   *
   * <p>The given {@link ArImage}s will be closed by this method, even if an exception occurs.
   */
  public void update(ArImage[] images) {
    try {
      GLES30.glBindTexture(GLES30.GL_TEXTURE_CUBE_MAP, radianceCubemap.getTextureId());
      GLError.maybeThrowGLException("Failed to bind radiance cubemap texture", "glBindTexture");

      if (images.length != NUMBER_OF_CUBE_FACES) {
        throw new IllegalArgumentException(
            "Number of images differs from the number of sides of a cube.");
      }

      for (int i = 0; i < NUMBER_OF_CUBE_FACES; ++i) {
        ArImage image = images[i];
        // Sanity check for the format of the cubemap.
        if (image.getFormat() != ImageFormat.RGBA_FP16) {
          throw new IllegalArgumentException(
              "Unexpected image format for cubemap: " + image.getFormat());
        }
        if (image.getHeight() != image.getWidth()) {
          throw new IllegalArgumentException("Cubemap face is not square.");
        }
        if (image.getHeight() != resolution) {
          throw new IllegalArgumentException(
              "Cubemap face resolution ("
                  + image.getHeight()
                  + ") does not match expected value ("
                  + resolution
                  + ").");
        }

        GLES30.glTexImage2D(
            GLES30.GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            /*level=*/ 0,
            GLES30.GL_RGBA16F,
            /*width=*/ resolution,
            /*height=*/ resolution,
            /*border=*/ 0,
            GLES30.GL_RGBA,
            GLES30.GL_HALF_FLOAT,
            image.getPlanes()[0].getBuffer());
        GLError.maybeThrowGLException("Failed to populate cubemap face", "glTexImage2D");
      }

      GLES30.glGenerateMipmap(GLES30.GL_TEXTURE_CUBE_MAP);
      GLError.maybeThrowGLException("Failed to generate cubemap mipmaps", "glGenerateMipmap");

      // Do the filtering operation, filling the mipmaps of ldTexture with the roughness filtered
      // cubemap.
      for (int level = 0; level < numberOfMipmapLevels; ++level) {
        int mipmapResolution = resolution >> level;
        GLES30.glViewport(0, 0, mipmapResolution, mipmapResolution);
        GLError.maybeThrowGLException("Failed to set viewport dimensions", "glViewport");
        for (int chunkIndex = 0; chunkIndex < shaders.length; ++chunkIndex) {
          GLES30.glBindFramebuffer(GLES30.GL_FRAMEBUFFER, framebuffers[level][chunkIndex]);
          GLError.maybeThrowGLException("Failed to bind cubemap framebuffer", "glBindFramebuffer");
          shaders[chunkIndex].setInt("u_RoughnessLevel", level);
          shaders[chunkIndex].lowLevelUse();
          mesh.lowLevelDraw();
        }
      }
    } finally {
      for (ArImage image : images) {
        image.close();
      }
    }
  }

  /** Returns the number of mipmap levels in the filtered cubemap texture. */
  public int getNumberOfMipmapLevels() {
    return numberOfMipmapLevels;
  }

  /**
   * Returns the filtered cubemap texture whose contents are updated with each call to {@link
   * #update(ArImage[])}.
   */
  public Texture getFilteredCubemapTexture() {
    return ldCubemap;
  }

  private void initializeLdCubemap() {
    // Initialize mipmap levels of LD cubemap.
    GLES30.glBindTexture(GLES30.GL_TEXTURE_CUBE_MAP, ldCubemap.getTextureId());
    GLError.maybeThrowGLException("Could not bind LD cubemap texture", "glBindTexture");
    for (int level = 0; level < numberOfMipmapLevels; ++level) {
      int mipmapResolution = resolution >> level;
      for (int face = 0; face < NUMBER_OF_CUBE_FACES; ++face) {
        GLES30.glTexImage2D(
            GLES30.GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
            level,
            GLES30.GL_RGB16F,
            /*width=*/ mipmapResolution,
            /*height=*/ mipmapResolution,
            /*border=*/ 0,
            GLES30.GL_RGB,
            GLES30.GL_HALF_FLOAT,
            /*data=*/ null);
        GLError.maybeThrowGLException("Could not initialize LD cubemap mipmap", "glTexImage2D");
      }
    }
  }

  private Shader[] createShaders(SampleRender render, ChunkIterable chunks) throws IOException {
    ImportanceSampleCacheEntry[][] importanceSampleCaches = generateImportanceSampleCaches();

    HashMap<String, String> commonDefines = new HashMap<>();
    commonDefines.put("NUMBER_OF_IMPORTANCE_SAMPLES", Integer.toString(numberOfImportanceSamples));
    commonDefines.put("NUMBER_OF_MIPMAP_LEVELS", Integer.toString(numberOfMipmapLevels));

    Shader[] shaders = new Shader[chunks.numberOfChunks];
    for (Chunk chunk : chunks) {
      HashMap<String, String> defines = new HashMap<>(commonDefines);
      for (int location = 0; location < chunk.chunkSize; ++location) {
        defines.put(
            ATTACHMENT_LOCATION_DEFINES[chunk.firstFaceIndex + location],
            Integer.toString(location));
      }

      // Create the shader and populate its uniforms with the importance sample cache entries.
      shaders[chunk.chunkIndex] =
          Shader.createFromAssets(
                  render, "shaders/cubemap_filter.vert", "shaders/cubemap_filter.frag", defines)
              .setTexture("u_Cubemap", radianceCubemap)
              .setDepthTest(false)
              .setDepthWrite(false);
    }

    for (Shader shader : shaders) {
      for (int i = 0; i < importanceSampleCaches.length; ++i) {
        ImportanceSampleCacheEntry[] cache = importanceSampleCaches[i];
        String cacheName = "u_ImportanceSampleCaches[" + i + "]";
        shader.setInt(cacheName + ".number_of_entries", cache.length);
        for (int j = 0; j < cache.length; ++j) {
          ImportanceSampleCacheEntry entry = cache[j];
          String entryName = cacheName + ".entries[" + j + "]";
          shader
              .setVec3(entryName + ".direction", entry.direction)
              .setFloat(entryName + ".contribution", entry.contribution)
              .setFloat(entryName + ".level", entry.level);
        }
      }
    }

    return shaders;
  }

  private int[][] createFramebuffers(ChunkIterable chunks) {
    // Create the framebuffers for each mipmap level.
    int[][] framebuffers = new int[numberOfMipmapLevels][];
    for (int level = 0; level < numberOfMipmapLevels; ++level) {
      int[] framebufferChunks = new int[chunks.numberOfChunks];
      GLES30.glGenFramebuffers(framebufferChunks.length, framebufferChunks, 0);
      GLError.maybeThrowGLException("Could not create cubemap framebuffers", "glGenFramebuffers");
      for (Chunk chunk : chunks) {
        // Set the drawbuffers
        GLES30.glBindFramebuffer(GLES30.GL_FRAMEBUFFER, framebufferChunks[chunk.chunkIndex]);
        GLError.maybeThrowGLException("Could not bind framebuffer", "glBindFramebuffer");
        GLES30.glDrawBuffers(chunk.chunkSize, ATTACHMENT_ENUMS, 0);
        GLError.maybeThrowGLException("Could not bind draw buffers", "glDrawBuffers");
        // Since GLES doesn't support glFramebufferTexture, we will use each cubemap face as a
        // different color attachment.
        for (int attachment = 0; attachment < chunk.chunkSize; ++attachment) {
          GLES30.glFramebufferTexture2D(
              GLES30.GL_FRAMEBUFFER,
              GLES30.GL_COLOR_ATTACHMENT0 + attachment,
              GLES30.GL_TEXTURE_CUBE_MAP_POSITIVE_X + chunk.firstFaceIndex + attachment,
              ldCubemap.getTextureId(),
              level);
          GLError.maybeThrowGLException(
              "Could not attach LD cubemap mipmap to framebuffer", "glFramebufferTexture");
        }
      }
      framebuffers[level] = framebufferChunks;
    }

    return framebuffers;
  }

  /**
   * Generate a cache of importance sampling terms in tangent space, indexed by {@code
   * [roughnessLevel-1][sampleIndex]}.
   */
  private ImportanceSampleCacheEntry[][] generateImportanceSampleCaches() {
    ImportanceSampleCacheEntry[][] result =
        new ImportanceSampleCacheEntry[numberOfMipmapLevels - 1][];
    for (int i = 0; i < numberOfMipmapLevels - 1; ++i) {
      int mipmapLevel = i + 1;
      float perceptualRoughness = mipmapLevel / (float) (numberOfMipmapLevels - 1);
      float roughness = perceptualRoughness * perceptualRoughness;
      int resolution = this.resolution >> mipmapLevel;
      float log4omegaP = log4((4.0f * PI_F) / (6 * resolution * resolution));
      float inverseNumberOfSamples = 1f / numberOfImportanceSamples;

      ArrayList<ImportanceSampleCacheEntry> cache = new ArrayList<>(numberOfImportanceSamples);
      float weight = 0f;
      for (int sampleIndex = 0; sampleIndex < numberOfImportanceSamples; ++sampleIndex) {
        float[] u = hammersley(sampleIndex, inverseNumberOfSamples);
        float[] h = hemisphereImportanceSampleDggx(u, roughness);
        float noh = h[2];
        float noh2 = noh * noh;
        float nol = 2f * noh2 - 1f;
        if (nol > 0) {
          ImportanceSampleCacheEntry entry = new ImportanceSampleCacheEntry();
          entry.direction = new float[] {2f * noh * h[0], 2 * noh * h[1], nol};
          float pdf = distributionGgx(noh, roughness) / 4f;
          float log4omegaS = log4(1f / (numberOfImportanceSamples * pdf));
          // K is a LOD bias that allows a bit of overlapping between samples
          float log4K = 1f; // K = 4
          float l = log4omegaS - log4omegaP + log4K;
          entry.level = min(max(l, 0f), (float) (numberOfMipmapLevels - 1));
          entry.contribution = nol;

          cache.add(entry);
          weight += nol;
        }
      }
      for (ImportanceSampleCacheEntry entry : cache) {
        entry.contribution /= weight;
      }
      result[i] = new ImportanceSampleCacheEntry[cache.size()];
      cache.toArray(result[i]);
    }
    return result;
  }

  private static int getMaxColorAttachments() {
    int[] result = new int[1];
    GLES30.glGetIntegerv(GLES30.GL_MAX_COLOR_ATTACHMENTS, result, 0);
    GLError.maybeThrowGLException("Failed to get max color attachments", "glGetIntegerv");
    return result[0];
  }

  // Math!
  private static final float PI_F = (float) Math.PI;

  private static int log2(int value) {
    if (value <= 0) {
      throw new IllegalArgumentException("value must be positive");
    }
    value >>= 1;
    int result = 0;
    while (value != 0) {
      ++result;
      value >>= 1;
    }
    return result;
  }

  private static float log4(float value) {
    return (float) (Math.log((double) value) / Math.log(4.0));
  }

  private static float sqrt(float value) {
    return (float) Math.sqrt((double) value);
  }

  private static float sin(float value) {
    return (float) Math.sin((double) value);
  }

  private static float cos(float value) {
    return (float) Math.cos((double) value);
  }

  private static float[] hammersley(int i, float iN) {
    float tof = 0.5f / 0x80000000L;
    long bits = i;
    bits = (bits << 16) | (bits >>> 16);
    bits = ((bits & 0x55555555L) << 1) | ((bits & 0xAAAAAAAAL) >>> 1);
    bits = ((bits & 0x33333333L) << 2) | ((bits & 0xCCCCCCCCL) >>> 2);
    bits = ((bits & 0x0F0F0F0FL) << 4) | ((bits & 0xF0F0F0F0L) >>> 4);
    bits = ((bits & 0x00FF00FFL) << 8) | ((bits & 0xFF00FF00L) >>> 8);
    return new float[] {i * iN, bits * tof};
  }

  private static float[] hemisphereImportanceSampleDggx(float[] u, float a) {
    // GGX - Trowbridge-Reitz importance sampling
    float phi = 2.0f * PI_F * u[0];
    // NOTE: (aa-1) == (a-1)(a+1) produces better fp accuracy
    float cosTheta2 = (1f - u[1]) / (1f + (a + 1f) * ((a - 1f) * u[1]));
    float cosTheta = sqrt(cosTheta2);
    float sinTheta = sqrt(1f - cosTheta2);
    return new float[] {sinTheta * cos(phi), sinTheta * sin(phi), cosTheta};
  }

  private static float distributionGgx(float noh, float a) {
    // NOTE: (aa-1) == (a-1)(a+1) produces better fp accuracy
    float f = (a - 1f) * ((a + 1f) * (noh * noh)) + 1f;
    return (a * a) / (PI_F * f * f);
  }
}
