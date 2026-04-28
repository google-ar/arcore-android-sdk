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

import android.opengl.GLES30;
import java.io.Closeable;
import java.nio.IntBuffer;

/**
 * A list of vertex indices stored GPU-side.
 *
 * <p>When constructing a {@link Mesh}, an {@link IndexBuffer} may be passed to describe the
 * ordering of vertices when drawing each primitive.
 *
 * @see <a
 *     href="https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glDrawElements.xhtml">glDrawElements</a>
 */
public class IndexBuffer implements Closeable {
  private final GpuBuffer buffer;

  /**
   * Construct an {@link IndexBuffer} populated with initial data.
   *
   * <p>The GPU buffer will be filled with the data in the <i>direct</i> buffer {@code entries},
   * starting from the beginning of the buffer (not the current cursor position). The cursor will be
   * left in an undefined position after this function returns.
   *
   * <p>The {@code entries} buffer may be null, in which case an empty buffer is constructed
   * instead.
   */
  public IndexBuffer(SampleRender render, IntBuffer entries) {
    buffer = new GpuBuffer(GLES30.GL_ELEMENT_ARRAY_BUFFER, GpuBuffer.INT_SIZE, entries);
  }

  /**
   * Populate with new data.
   *
   * <p>The entire buffer is replaced by the contents of the <i>direct</i> buffer {@code entries}
   * starting from the beginning of the buffer, not the current cursor position. The cursor will be
   * left in an undefined position after this function returns.
   *
   * <p>The GPU buffer is reallocated automatically if necessary.
   *
   * <p>The {@code entries} buffer may be null, in which case the buffer will become empty.
   */
  public void set(IntBuffer entries) {
    buffer.set(entries);
  }

  @Override
  public void close() {
    buffer.free();
  }

  /* package-private */
  int getBufferId() {
    return buffer.getBufferId();
  }

  /* package-private */
  int getSize() {
    return buffer.getSize();
  }
}
