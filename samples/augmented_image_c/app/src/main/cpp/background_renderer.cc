/*
 * Copyright 2018 Google LLC
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

// This modules handles drawing the passthrough camera image into the OpenGL
// scene.

#include <type_traits>

#include "background_renderer.h"

namespace augmented_image {
namespace {
// Positions of the quad vertices in clip space (X, Y).
const GLfloat kVertices[] = {
    -1.0f, -1.0f, +1.0f, -1.0f, -1.0f, +1.0f, +1.0f, +1.0f,
};

constexpr char kVertexShaderFilename[] = "shaders/screenquad.vert";
constexpr char kFragmentShaderFilename[] = "shaders/screenquad.frag";
}  // namespace

void BackgroundRenderer::InitializeGlContent(AAssetManager* asset_manager) {
  shader_program_ = util::CreateProgram(asset_manager, kVertexShaderFilename,
                                        kFragmentShaderFilename);
  if (!shader_program_) {
    LOGE("Could not create program.");
  }

  glGenTextures(1, &texture_id_);
  glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_id_);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  uniform_texture_ = glGetUniformLocation(shader_program_, "sTexture");
  attribute_vertices_ = glGetAttribLocation(shader_program_, "a_Position");
  attribute_uvs_ = glGetAttribLocation(shader_program_, "a_TexCoord");
}

void BackgroundRenderer::Draw(const ArSession* session, const ArFrame* frame) {
  static_assert(std::extent<decltype(kVertices)>::value == kNumVertices * 2,
                "Incorrect kVertices length");

  // If display rotation changed (also includes view size change), we need to
  // re-query the uv coordinates for the on-screen portion of the camera image.
  int32_t geometry_changed = 0;
  ArFrame_getDisplayGeometryChanged(session, frame, &geometry_changed);
  if (geometry_changed != 0 || !uvs_initialized_) {
    ArFrame_transformCoordinates2d(
        session, frame, AR_COORDINATES_2D_OPENGL_NORMALIZED_DEVICE_COORDINATES,
        kNumVertices, kVertices, AR_COORDINATES_2D_TEXTURE_NORMALIZED,
        transformed_uvs_);
    uvs_initialized_ = true;
  }

  int64_t frame_timestamp;
  ArFrame_getTimestamp(session, frame, &frame_timestamp);
  if (frame_timestamp == 0) {
    // Suppress rendering if the camera did not produce the first frame yet.
    // This is to avoid drawing possible leftover data from previous sessions if
    // the texture is reused.
    return;
  }

  glUseProgram(shader_program_);
  glDepthMask(GL_FALSE);

  glUniform1i(uniform_texture_, 1);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_id_);

  glEnableVertexAttribArray(attribute_vertices_);
  glVertexAttribPointer(attribute_vertices_, 2, GL_FLOAT, GL_FALSE, 0,
                        kVertices);

  glEnableVertexAttribArray(attribute_uvs_);
  glVertexAttribPointer(attribute_uvs_, 2, GL_FLOAT, GL_FALSE, 0,
                        transformed_uvs_);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  glUseProgram(0);
  glDepthMask(GL_TRUE);
  util::CheckGlError("BackgroundRenderer::Draw() error");
}

GLuint BackgroundRenderer::GetTextureId() const { return texture_id_; }

}  // namespace augmented_image
