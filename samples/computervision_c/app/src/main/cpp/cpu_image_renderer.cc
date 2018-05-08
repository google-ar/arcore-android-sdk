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

// This modules handles drawing the passthrough camera image into the OpenGL
// scene.
#include "cpu_image_renderer.h"

#include <stdint.h>
#include <algorithm>

namespace computer_vision {
namespace {
// Positions of the quad vertices in clip space (X, Y, Z).
const GLfloat kVertices[] = {
    -1.0f, -1.0f, 0.0f, -1.0f, +1.0f, 0.0f,
    +1.0f, -1.0f, 0.0f, +1.0f, +1.0f, 0.0f,
};

// UVs of the quad vertices (S, T)
const GLfloat kUvs[] = {
    0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f,
};

constexpr int kSobelEdgeThreshold = 128 * 128;
constexpr int kCoordsPerVertex = 3;
constexpr int kTexCoordsPerVertex = 2;
constexpr int kTextureCount = 4;

constexpr char kVertexShader[] = R"(
      attribute vec4 a_Position;
      attribute vec2 a_TexCoord;
      attribute vec2 a_ImgCoord;

      varying vec2 v_TexCoord;
      varying vec2 v_ImgCoord;

      void main() {
         gl_Position = a_Position;
         v_TexCoord = a_TexCoord;
         v_ImgCoord = a_ImgCoord;
    })";

constexpr char kFragmentShader[] = R"(
    #extension GL_OES_EGL_image_external : require
    precision mediump float;
    varying vec2 v_TexCoord;
    varying vec2 v_ImgCoord;
    uniform float s_SplitterPosition;
    uniform samplerExternalOES TexVideo;
    uniform sampler2D TexCpuImageGrayscale;

    void main() {
      if (v_TexCoord.x < s_SplitterPosition)
      {
        gl_FragColor = texture2D(TexVideo, v_TexCoord);
      }
      else
      {
        gl_FragColor= texture2D(TexCpuImageGrayscale, v_ImgCoord);
      }
    })";

bool GetNdkImageProperties(const AImage* ndk_image, int32_t* out_format,
                           int32_t* out_width, int32_t* out_height,
                           int32_t* out_plane_num, int32_t* out_stride) {
  if (ndk_image == nullptr) {
    return false;
  }
  media_status_t status = AImage_getFormat(ndk_image, out_format);
  if (status != AMEDIA_OK) {
    return false;
  }

  status = AImage_getWidth(ndk_image, out_width);
  if (status != AMEDIA_OK) {
    return false;
  }

  status = AImage_getHeight(ndk_image, out_height);
  if (status != AMEDIA_OK) {
    return false;
  }

  status = AImage_getNumberOfPlanes(ndk_image, out_plane_num);
  if (status != AMEDIA_OK) {
    return false;
  }

  status = AImage_getPlaneRowStride(ndk_image, 0, out_stride);
  if (status != AMEDIA_OK) {
    return false;
  }

  return true;
}

bool DetectEdge(const AImage* ndk_image, int32_t width, int32_t height,
                int32_t stride, uint8_t* output_pixels) {
  if (ndk_image == nullptr || output_pixels == nullptr) {
    return false;
  }

  uint8_t* input_pixels = nullptr;
  int length = 0;
  media_status_t status =
      AImage_getPlaneData(ndk_image, 0, &input_pixels, &length);
  if (status != AMEDIA_OK) {
    return false;
  }

  // Detect edges.
  for (int j = 1; j < height - 1; j++) {
    for (int i = 1; i < width - 1; i++) {
      // Offset of the pixel at [i, j] of the input image.
      int offset = (j * stride) + i;

      // Neighbour pixels around the pixel at [i, j].
      int a00 = input_pixels[offset - width - 1];
      int a01 = input_pixels[offset - width];
      int a02 = input_pixels[offset - width + 1];
      int a10 = input_pixels[offset - 1];
      int a12 = input_pixels[offset + 1];
      int a20 = input_pixels[offset + width - 1];
      int a21 = input_pixels[offset + width];
      int a22 = input_pixels[offset + width + 1];

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
  return true;
}

}  // namespace

void CpuImageRenderer::InitializeGlContent() {
  GLuint textures[kTextureCount];
  glGenTextures(kTextureCount, textures);

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

  u_texture_id_ = textures[2];
  glBindTexture(GL_TEXTURE_2D, u_texture_id_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  v_texture_id_ = textures[3];
  glBindTexture(GL_TEXTURE_2D, v_texture_id_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  shader_program_ = util::CreateProgram(kVertexShader, kFragmentShader);
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
                            const AImage* ndk_image, float screen_aspect_ratio,
                            int display_rotation, float splitter_pos) {
  // Try to get the NDK image and get the post-processed edge detection image.
  int32_t format = 0, width = 0, height = 0, num_plane = 0, stride = 0;
  bool is_valid_cpu_image = false;
  if (ndk_image != nullptr) {
    if (GetNdkImageProperties(ndk_image, &format, &width, &height, &num_plane,
                              &stride)) {
      if (format == AIMAGE_FORMAT_YUV_420_888) {
        if (width > 0 || height > 0 || num_plane > 0 || stride > 0) {
          if (processed_image_bytes_grayscale_ == nullptr ||
              stride * height > cpu_image_buffer_size_) {
            cpu_image_buffer_size_ = stride * height;
            processed_image_bytes_grayscale_ =
                std::unique_ptr<uint8_t[]>(new uint8_t[cpu_image_buffer_size_]);
          }
          DetectEdge(ndk_image, width, height, stride,
                     processed_image_bytes_grayscale_.get());
          is_valid_cpu_image = true;
        }
      } else {
        LOGE("Expected image in YUV_420_888 format.");
      }
    }
  }

  // No need to test or write depth, the screen quad has arbitrary depth, and is
  // expected to be drawn first.
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_id_);

  UpdateTextureCoordinates(width, height, screen_aspect_ratio,
                           display_rotation);
  // Update GPU image texture coordinates.
  ArFrame_transformDisplayUvCoords(session, frame, kNumVertices * 2, kUvs,
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

void CpuImageRenderer::UpdateTextureCoordinates(int32_t image_width,
                                                int32_t image_height,
                                                float screen_aspect_ratio,
                                                int display_rotation) {
  // Crop the CPU image to fit the screen aspect ratio.
  float image_aspect_ratio = static_cast<float>(image_width) / image_height;
  float cropped_width, cropped_height;
  if (screen_aspect_ratio < image_aspect_ratio) {
    cropped_width = image_height * screen_aspect_ratio;
    cropped_height = image_height;
  } else {
    cropped_width = image_width;
    cropped_height = image_width / screen_aspect_ratio;
  }

  float u = (image_width - cropped_width) / image_width / 2.f;
  float v = (image_height - cropped_height) / image_height / 2.f;
  // 4 possible display rotation.
  float tex_coords[4][kNumVertices * 2] = {
      {u, 1 - v, u, v, 1 - u, 1 - v, 1 - u, v},  // Surface.ROTATION_0
      {1 - u, 1 - v, u, 1 - v, 1 - u, v, u, v},  // Surface.ROTATION_90
      {1 - u, v, 1 - u, 1 - v, u, v, u, 1 - v},  // Surface.ROTATION_180
      {u, v, 1 - u, v, u, 1 - v, 1 - u, 1 - v}   // Surface.ROTATION_270
  };

  if (display_rotation < 0 || display_rotation > 3) {
    display_rotation = 0;  // default;
  }
  std::copy(tex_coords[display_rotation], tex_coords[display_rotation] + 8,
            transformed_img_coord_);
}

GLuint CpuImageRenderer::GetTextureId() const { return texture_id_; }

}  // namespace computer_vision
