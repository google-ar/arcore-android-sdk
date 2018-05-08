/*
 * Copyright 2018 Google Inc. All Rights Reserved.
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

#include "augmented_image_renderer.h"
#include "util.h"

namespace augmented_image {

void AugmentedImageRenderer::InitializeGlContent(AAssetManager* asset_manager) {
  image_frame_upper_left.InitializeGlContent(
      asset_manager, "models/frame_upper_left.obj", "models/frame_base.png");
  image_frame_upper_right.InitializeGlContent(
      asset_manager, "models/frame_upper_right.obj", "models/frame_base.png");
  image_frame_lower_left.InitializeGlContent(
      asset_manager, "models/frame_lower_left.obj", "models/frame_base.png");
  image_frame_lower_right.InitializeGlContent(
      asset_manager, "models/frame_lower_right.obj", "models/frame_base.png");
}

void AugmentedImageRenderer::Draw(const glm::mat4& projection_mat,
                                  const glm::mat4& view_mat,
                                  const float* color_correction4,
                                  const float* color_tint_rgba,
                                  const ArSession* ar_session,
                                  const ArAugmentedImage* ar_image,
                                  const ArAnchor* ar_anchor) const {
  // Get image extents.
  float extent_x, extent_z;
  ArAugmentedImage_getExtentX(ar_session, ar_image, &extent_x);
  ArAugmentedImage_getExtentZ(ar_session, ar_image, &extent_z);

  glm::mat4 local_upper_left_matrix = glm::translate(
      glm::mat4(1.0), glm::vec3(-0.5f * extent_x, 0.0f, -0.5f * extent_z));
  glm::mat4 local_upper_right_matrix = glm::translate(
      glm::mat4(1.0), glm::vec3(0.5f * extent_x, 0.0f, -0.5f * extent_z));
  glm::mat4 local_lower_right_matrix = glm::translate(
      glm::mat4(1.0), glm::vec3(0.5f * extent_x, 0.0f, 0.5f * extent_z));
  glm::mat4 local_lower_left_matrix = glm::translate(
      glm::mat4(1.0), glm::vec3(-0.5f * extent_x, 0.0f, 0.5f * extent_z));

  glm::mat4 center_matrix;
  util::GetTransformMatrixFromAnchor(ar_session, ar_anchor, &center_matrix);

  image_frame_upper_left.Draw(projection_mat, view_mat,
                              center_matrix * local_upper_left_matrix,
                              color_correction4, color_tint_rgba);
  image_frame_upper_right.Draw(projection_mat, view_mat,
                               center_matrix * local_upper_right_matrix,
                               color_correction4, color_tint_rgba);
  image_frame_lower_left.Draw(projection_mat, view_mat,
                              center_matrix * local_lower_left_matrix,
                              color_correction4, color_tint_rgba);
  image_frame_lower_right.Draw(projection_mat, view_mat,
                               center_matrix * local_lower_right_matrix,
                               color_correction4, color_tint_rgba);
}

}  // namespace augmented_image
