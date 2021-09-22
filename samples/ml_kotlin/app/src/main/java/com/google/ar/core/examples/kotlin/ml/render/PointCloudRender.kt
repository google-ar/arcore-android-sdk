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

package com.google.ar.core.examples.kotlin.ml.render

import com.google.ar.core.PointCloud
import com.google.ar.core.examples.java.common.samplerender.Mesh
import com.google.ar.core.examples.java.common.samplerender.SampleRender
import com.google.ar.core.examples.java.common.samplerender.Shader
import com.google.ar.core.examples.java.common.samplerender.VertexBuffer

/** Renderer to display [PointCloud]s. */
class PointCloudRender {
  lateinit var pointCloudVertexBuffer: VertexBuffer
  lateinit var pointCloudMesh: Mesh
  lateinit var pointCloudShader: Shader

  // Keep track of the last point cloud rendered to avoid updating the VBO if point cloud
  // was not changed.  Do this using the timestamp since we can't compare PointCloud objects.
  var lastPointCloudTimestamp: Long = 0

  fun onSurfaceCreated(render: SampleRender) {
    // Point cloud
    pointCloudShader =
      Shader.createFromAssets(
          render,
          "shaders/point_cloud.vert",
          "shaders/point_cloud.frag",
          /*defines=*/ null
        )
        .setVec4("u_Color", floatArrayOf(31.0f / 255.0f, 188.0f / 255.0f, 210.0f / 255.0f, 1.0f))
        .setFloat("u_PointSize", 5.0f)

    // four entries per vertex: X, Y, Z, confidence
    pointCloudVertexBuffer = VertexBuffer(render, 4, null)
    val pointCloudVertexBuffers = arrayOf(pointCloudVertexBuffer)
    pointCloudMesh = Mesh(render, Mesh.PrimitiveMode.POINTS, null, pointCloudVertexBuffers)
  }

  fun drawPointCloud(
    render: SampleRender,
    pointCloud: PointCloud,
    modelViewProjectionMatrix: FloatArray
  ) {
    if (pointCloud.timestamp > lastPointCloudTimestamp) {
      pointCloudVertexBuffer.set(pointCloud.points)
      lastPointCloudTimestamp = pointCloud.timestamp
    }
    pointCloudShader.setMat4("u_ModelViewProjection", modelViewProjectionMatrix)
    render.draw(pointCloudMesh, pointCloudShader)
  }
}
