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
  if (ArFrame_acquireDepthImage(&session, &frame, &depth_image) != AR_SUCCESS) {
    // No depth image received for this frame.
    return;
  }
  // Checks that the format is as expected.
  ArImageFormat image_format;
  ArImage_getFormat(&session, depth_image, &image_format);
  if (image_format != AR_IMAGE_FORMAT_DEPTH16) {
    LOGE("Unexpected image format 0x%x", image_format);
    abort();
    return;
  }

  const uint8_t* depth_data = nullptr;
  int plane_size_bytes = 0;
  ArImage_getPlaneData(&session, depth_image, /*plane_index=*/0, &depth_data,
                       &plane_size_bytes);

  // Bails out if there's no depth_data.
  if (depth_data == nullptr) return;

  // Sets texture sizes.
  int image_width = 0;
  int image_height = 0;
  int image_pixel_stride = 0;
  int image_row_stride = 0;
  ArImage_getWidth(&session, depth_image, &image_width);
  ArImage_getHeight(&session, depth_image, &image_height);
  ArImage_getPlanePixelStride(&session, depth_image, 0, &image_pixel_stride);
  ArImage_getPlaneRowStride(&session, depth_image, 0, &image_row_stride);
  glBindTexture(GL_TEXTURE_2D, texture_id_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, image_width, image_height, 0, GL_RG,
               GL_UNSIGNED_BYTE, depth_data);
  width_ = image_width;
  height_ = image_height;
}

}  // namespace hello_ar
