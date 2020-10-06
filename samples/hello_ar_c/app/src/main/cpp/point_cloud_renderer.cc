/*
 * Copyright 2017 Google LLC
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

#include "point_cloud_renderer.h"
#include "util.h"

namespace hello_ar {
namespace {
constexpr char kVertexShaderFilename[] = "shaders/point_cloud.vert";
constexpr char kFragmentShaderFilename[] = "shaders/point_cloud.frag";
}  // namespace

void PointCloudRenderer::InitializeGlContent(AAssetManager* asset_manager) {
  shader_program_ = util::CreateProgram(kVertexShaderFilename,
                                        kFragmentShaderFilename, asset_manager);
  if (!shader_program_) {
    LOGE("Could not create program.");
  }

  attribute_vertices_ = glGetAttribLocation(shader_program_, "a_Position");
  uniform_mvp_mat_ =
      glGetUniformLocation(shader_program_, "u_ModelViewProjection");
  uniform_color_ = glGetUniformLocation(shader_program_, "u_Color");
  uniform_point_size_ = glGetUniformLocation(shader_program_, "u_PointSize");

  util::CheckGlError("point_cloud_renderer::InitializeGlContent()");
}

void PointCloudRenderer::Draw(const glm::mat4& mvp_matrix,
                              ArSession* ar_session,
                              ArPointCloud* ar_point_cloud) const {
  CHECK(shader_program_);

  glUseProgram(shader_program_);

  int32_t number_of_points = 0;
  ArPointCloud_getNumberOfPoints(ar_session, ar_point_cloud, &number_of_points);
  if (number_of_points <= 0) {
    return;
  }

  const float* point_cloud_data;
  ArPointCloud_getData(ar_session, ar_point_cloud, &point_cloud_data);

  glUniformMatrix4fv(uniform_mvp_mat_, 1, GL_FALSE, glm::value_ptr(mvp_matrix));

  glEnableVertexAttribArray(attribute_vertices_);
  glVertexAttribPointer(attribute_vertices_, 4, GL_FLOAT, GL_FALSE, 0,
                        point_cloud_data);

  // Set cyan color to the point cloud.
  glUniform4f(uniform_color_, 31.0f / 255.0f, 188.0f / 255.0f, 210.0f / 255.0f,
              1.0f);
  glUniform1f(uniform_point_size_, 5.0f);

  glDrawArrays(GL_POINTS, 0, number_of_points);

  glUseProgram(0);
  util::CheckGlError("PointCloudRenderer::Draw");
}

}  // namespace hello_ar
