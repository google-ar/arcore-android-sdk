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

#ifndef C_ARCORE_COMPUTER_VISION_BACKGROUND_RENDERER_H_
#define C_ARCORE_COMPUTER_VISION_BACKGROUND_RENDERER_H_

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <media/NdkImage.h>
#include <memory>

#include "arcore_c_api.h"
#include "util.h"

namespace computer_vision {

// This class renders both the pass through camera image and the post-processed
// cpu image.
class CpuImageRenderer {
 public:
  CpuImageRenderer() = default;
  ~CpuImageRenderer() = default;

  // Sets up OpenGL state.  Must be called on the OpenGL thread and before any
  // other methods below.
  void InitializeGlContent();

  // Draws the pass through camera image and CPU image.
  void Draw(const ArSession* session, const ArFrame* frame,
            const AImage* ndk_image, float screen_aspect_ratio,
            int display_rotation, float splitter_pos);

  void UpdateTextureCoordinates(int32_t image_width, int32_t image_height,
                                float screen_aspect_ratio,
                                int display_rotation);

  // Returns the generated texture name for the GL_TEXTURE_EXTERNAL_OES target.
  GLuint GetTextureId() const;

 private:
  static constexpr int kNumVertices = 4;

  GLuint shader_program_;

  GLuint texture_id_;
  GLuint overlay_texture_id_;
  GLuint u_texture_id_;
  GLuint v_texture_id_;

  GLuint attribute_position_;
  GLuint attribute_tex_coord_;
  GLuint attribute_img_coord_;
  GLuint uniform_spliter_;

  float transformed_tex_coord_[kNumVertices * 2];
  float transformed_img_coord_[kNumVertices * 2];
  std::unique_ptr<uint8_t[]> processed_image_bytes_grayscale_;
  int cpu_image_buffer_size_ = 0;
};
}  // namespace computer_vision
#endif  // C_ARCORE_COMPUTER_VISION_BACKGROUND_RENDERER_H_
