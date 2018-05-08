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

#ifndef C_ARCORE_AUGMENTED_IMAGE_AUGMENTED_IMAGE_RENDERER_H_
#define C_ARCORE_AUGMENTED_IMAGE_AUGMENTED_IMAGE_RENDERER_H_

#include <android/asset_manager.h>

#include "arcore_c_api.h"
#include "glm.h"
#include "obj_renderer.h"

namespace augmented_image {

// AugmentedImageRenderer handles the display of image frame on ArAugmentedImage
class AugmentedImageRenderer {
 public:
  AugmentedImageRenderer() = default;
  ~AugmentedImageRenderer() = default;

  // Sets up OpenGL state.  Must be called on the OpenGL thread and before any
  // other methods below.
  void InitializeGlContent(AAssetManager* asset_manager);

  // Draws frames on ArAugmentedImage, with center location at ArAnchor.
  void Draw(const glm::mat4& projection_mat, const glm::mat4& view_mat,
            const float* color_correction4, const float* color_tint_rgba,
            const ArSession* ar_session, const ArAugmentedImage* ar_image,
            const ArAnchor* ar_anchor) const;

 private:
  ObjRenderer image_frame_upper_left;
  ObjRenderer image_frame_upper_right;
  ObjRenderer image_frame_lower_left;
  ObjRenderer image_frame_lower_right;
};

}  // namespace augmented_image
#endif  // C_ARCORE_AUGMENTED_IMAGE_AUGMENTED_IMAGE_RENDERER_H_
