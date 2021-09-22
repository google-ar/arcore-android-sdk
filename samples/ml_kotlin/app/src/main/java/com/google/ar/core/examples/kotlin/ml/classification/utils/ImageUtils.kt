/*
 * Copyright 2021 Google LLC
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

package com.google.ar.core.examples.kotlin.ml.classification.utils

import android.graphics.Bitmap
import android.graphics.Matrix
import java.io.ByteArrayOutputStream

object ImageUtils {
  /**
   * Creates a new [Bitmap] by rotating the input bitmap [rotation] degrees. If [rotation] is 0, the
   * input bitmap is returned.
   */
  fun rotateBitmap(bitmap: Bitmap, rotation: Int): Bitmap {
    if (rotation == 0) return bitmap

    val matrix = Matrix()
    matrix.postRotate(rotation.toFloat())
    return Bitmap.createBitmap(bitmap, 0, 0, bitmap.width, bitmap.height, matrix, false)
  }

  /** Converts a [Bitmap] to [ByteArray] using [Bitmap.compress]. */
  fun Bitmap.toByteArray(): ByteArray =
    ByteArrayOutputStream().use { stream ->
      this.compress(Bitmap.CompressFormat.JPEG, 100, stream)
      stream.toByteArray()
    }
}
