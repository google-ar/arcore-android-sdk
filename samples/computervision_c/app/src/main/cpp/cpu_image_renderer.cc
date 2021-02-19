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
#include "cpu_image_renderer.h"

#include <stdint.h>

#include <algorithm>

namespace computer_vision {
namespace {
// Positions of the quad vertices in clip space (X, Y).
const GLfloat kVertices[] = {
    -1.0f, -1.0f, +1.0f, -1.0f, -1.0f, +1.0f, +1.0f, +1.0f,
};

constexpr int kSobelEdgeThreshold = 128 * 128;
constexpr int kCoordsPerVertex = 2;
constexpr int kTexCoordsPerVertex = 2;
constexpr char kVertexShaderFilename[] = "shaders/cpu_image.vert";
constexpr char kFragmentShaderFilename[] = "shaders/cpu_image.frag";

void DetectEdge(const ArSession* session, const ArImage* image, int32_t width,
                int32_t height, int32_t stride, uint8_t* output_pixels) {
  const uint8_t* input_pixels = nullptr;
  int32_t length = 0;
  ArImage_getPlaneData(session, image, 0, &input_pixels, &length);

  // Detect edges.
  for (int j = 1; j < height - 1; j++) {
    for (int i = 1; i < width - 1; i++) {
      // Offset of the pixel at [i, j] of the input image.
      int offset = (j * stride) + i;

      // Neighbour pixels around the pixel at [i, j].
      int a00 = input_pixels[offset - stride - 1];
      int a01 = input_pixels[offset - stride];
      int a02 = input_pixels[offset - stride + 1];
      int a10 = input_pixels[offset - 1];
      int a12 = input_pixels[offset + 1];
      int a20 = input_pixels[offset + stride - 1];
      int a21 = input_pixels[offset + stride];
      int a22 = input_pixels[offset + stride + 1];

      // Sobel X filter:
      //   -1, 0, 1,
      //   -2, 0, 2,
      //   -1, 0, 1
      int x_sum = -a00 - (2 * a10) - a20 + a02 + (2 * a12) + a22;

      // Sobel Y filter:
      //    1, 2, 1,
      //    0, 0, 0,
      //   -1, -2, -1
      int y_sum = a00 + (2 * a01) + a02 - a20 - (2 * a21) - a22;

      if ((x_sum * x_sum) + (y_sum * y_sum) > kSobelEdgeThreshold) {
        output_pixels[(j * width) + i] = static_cast<uint8_t>(0xFF);
      } else {
        output_pixels[(j * width) + i] = static_cast<uint8_t>(0x1F);
      }
    }
  }
}

}  // namespace

void CpuImageRenderer::InitializeGlContent(AAssetManager* asset_manager) {
  GLuint textures[2];
  glGenTextures(2, textures);

  texture_id_ = textures[0];
  glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_id_);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  overlay_texture_id_ = textures[1];
  glBindTexture(GL_TEXTURE_2D, overlay_texture_id_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  shader_program_ = util::CreateProgram(asset_manager, kVertexShaderFilename,
                                        kFragmentShaderFilename);
  if (!shader_program_) {
    LOGE("Could not create program.");
  }
  glUseProgram(shader_program_);
  attribute_position_ = glGetAttribLocation(shader_program_, "a_Position");
  attribute_tex_coord_ = glGetAttribLocation(shader_program_, "a_TexCoord");
  attribute_img_coord_ = glGetAttribLocation(shader_program_, "a_ImgCoord");

  uniform_spliter_ =
      glGetUniformLocation(shader_program_, "s_SplitterPosition");
  int tex_loc = glGetUniformLocation(shader_program_, "TexVideo");
  glUniform1i(tex_loc, 0);
  tex_loc = glGetUniformLocation(shader_program_, "TexCpuImageGrayscale");
  glUniform1i(tex_loc, 1);
}

void CpuImageRenderer::Draw(const ArSession* session, const ArFrame* frame,
                            const ArImage* image, float screen_aspect_ratio,
                            int display_rotation, float splitter_pos) {
  // Try to get the ar image and get the post-processed edge detection image.
  ArImageFormat format;
  int32_t width = 0, height = 0, num_plane = 0, stride = 0;
  bool is_valid_cpu_image = false;
  // No need to compute edge detection as it is not being displayed if the
  // splitter position is one.
  if ((image != nullptr) && (splitter_pos < 1.0)) {
    ArImage_getFormat(session, image, &format);
    ArImage_getWidth(session, image, &width);
    ArImage_getHeight(session, image, &height);
    ArImage_getNumberOfPlanes(session, image, &num_plane);
    ArImage_getPlaneRowStride(session, image, 0, &stride);

    if (format == AR_IMAGE_FORMAT_YUV_420_888) {
      if (width > 0 || height > 0 || num_plane > 0 || stride > 0) {
        if (processed_image_bytes_grayscale_ == nullptr ||
            stride * height > cpu_image_buffer_size_) {
          cpu_image_buffer_size_ = stride * height;
          processed_image_bytes_grayscale_ =
              std::unique_ptr<uint8_t[]>(new uint8_t[cpu_image_buffer_size_]);
        }
        DetectEdge(session, image, width, height, stride,
                   processed_image_bytes_grayscale_.get());
        is_valid_cpu_image = true;
      }
    } else {
      LOGE("Expected image in YUV_420_888 format.");
    }
  }

  // No need to test or write depth, the screen quad has arbitrary depth, and is
  // expected to be drawn first.
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_id_);

  // Update CPU image & GPU texture coordinates.
  ArFrame_transformCoordinates2d(
      session, frame, AR_COORDINATES_2D_OPENGL_NORMALIZED_DEVICE_COORDINATES,
      kNumVertices, kVertices, AR_COORDINATES_2D_IMAGE_NORMALIZED,
      transformed_img_coord_);
  ArFrame_transformCoordinates2d(
      session, frame, AR_COORDINATES_2D_OPENGL_NORMALIZED_DEVICE_COORDINATES,
      kNumVertices, kVertices, AR_COORDINATES_2D_TEXTURE_NORMALIZED,
      transformed_tex_coord_);

  if (is_valid_cpu_image) {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, overlay_texture_id_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE,
                 GL_UNSIGNED_BYTE, processed_image_bytes_grayscale_.get());
  }
  glUseProgram(shader_program_);

  // Set the vertex positions.
  glVertexAttribPointer(attribute_position_, kCoordsPerVertex, GL_FLOAT,
                        GL_FALSE, 0, kVertices);

  // Set splitter position.
  glUniform1f(uniform_spliter_, splitter_pos);
  glVertexAttribPointer(attribute_tex_coord_, kTexCoordsPerVertex, GL_FLOAT,
                        GL_FALSE, 0, transformed_tex_coord_);

  glVertexAttribPointer(attribute_img_coord_, kTexCoordsPerVertex, GL_FLOAT,
                        GL_FALSE, 0, transformed_img_coord_);

  // Enable vertex arrays
  glEnableVertexAttribArray(attribute_position_);
  glEnableVertexAttribArray(attribute_tex_coord_);
  glEnableVertexAttribArray(attribute_img_coord_);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  // Disable vertex arrays
  glDisableVertexAttribArray(attribute_position_);
  glDisableVertexAttribArray(attribute_tex_coord_);
  glDisableVertexAttribArray(attribute_img_coord_);

  // Restore the depth state for further drawing.
  glDepthMask(GL_TRUE);
  glEnable(GL_DEPTH_TEST);
  util::CheckGlError("CpuImageRenderer::Draw() error");
}

GLuint CpuImageRenderer::GetTextureId() const { return texture_id_; }

}  // namespace computer_vision
