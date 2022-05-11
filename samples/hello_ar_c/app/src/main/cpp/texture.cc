/*
 * Copyright 2020 Google LLC
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

#include "texture.h"

// clang-format off
#include <GLES3/gl31.h>
#include <GLES2/gl2ext.h>
// clang-format on
#include "util.h"

namespace hello_ar {

void Texture::CreateOnGlThread() {
  GLuint texture_id_array[1];
  glGenTextures(1, texture_id_array);
  texture_id_ = texture_id_array[0];

  glBindTexture(GL_TEXTURE_2D, texture_id_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void Texture::UpdateWithDepthImageOnGlThread(const ArSession& session,
                                             const ArFrame& frame) {
  ArImage* depth_image = nullptr;
  if (ArFrame_acquireDepthImage16Bits(&session, &frame, &depth_image) !=
      AR_SUCCESS) {
    // No depth image received for this frame.
    return;
  }
  // Checks that the format is as expected.
  ArImageFormat image_format;
  ArImage_getFormat(&session, depth_image, &image_format);
  if (image_format != AR_IMAGE_FORMAT_D_16) {
    LOGE("Unexpected image format 0x%x", image_format);
    ArImage_release(depth_image);
    abort();
    return;
  }

  const uint8_t* depth_data = nullptr;
  int plane_size_bytes = 0;
  ArImage_getPlaneData(&session, depth_image, /*plane_index=*/0, &depth_data,
                       &plane_size_bytes);

  // Bails out if there's no depth_data.
  if (depth_data == nullptr) {
    ArImage_release(depth_image);
    return;
  }

  // Sets texture sizes.
  int image_width = 0;
  int image_height = 0;
  int image_pixel_stride = 0;
  int image_row_stride = 0;
  ArImage_getWidth(&session, depth_image, &image_width);
  ArImage_getHeight(&session, depth_image, &image_height);
  ArImage_getPlanePixelStride(&session, depth_image, 0, &image_pixel_stride);
  ArImage_getPlaneRowStride(&session, depth_image, 0, &image_row_stride);
  ArImage_release(depth_image);
  glBindTexture(GL_TEXTURE_2D, texture_id_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, image_width, image_height, 0, GL_RG,
               GL_UNSIGNED_BYTE, depth_data);
  width_ = image_width;
  height_ = image_height;
}

}  // namespace hello_ar
