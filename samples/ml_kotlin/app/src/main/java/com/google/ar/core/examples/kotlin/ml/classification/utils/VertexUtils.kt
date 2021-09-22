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

import android.graphics.Point
import android.graphics.PointF

object VertexUtils {
  /** Convert a normalized [PointF] to an absolute [Point]. */
  fun PointF.toAbsoluteCoordinates(
    imageWidth: Int,
    imageHeight: Int,
  ): Point {
    return Point((x * imageWidth).toInt(), (y * imageHeight).toInt())
  }

  /** Rotates a [Point] according to [imageRotation]. */
  fun Point.rotateCoordinates(
    imageWidth: Int,
    imageHeight: Int,
    imageRotation: Int,
  ): Point {
    return when (imageRotation) {
      0 -> Point(x, y)
      180 -> Point(imageWidth - x, imageHeight - y)
      90 -> Point(y, imageWidth - x)
      270 -> Point(imageHeight - y, x)
      else -> error("Invalid imageRotation $imageRotation")
    }
  }

  /** Calculate a center point using the average positions of points in the bounding polygon. */
  fun calculateCenterOfPoly(boundingPoly: List<PointF>): PointF {
    var averageX = 0f
    var averageY = 0f
    for (vertex in boundingPoly) {
      averageX += vertex.x / boundingPoly.size
      averageY += vertex.y / boundingPoly.size
    }
    return PointF(averageX, averageY)
  }
}
