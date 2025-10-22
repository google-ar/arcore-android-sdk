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

#ifndef C_ARCORE_HELLO_AR_VULKAN_UTIL_H_
#define C_ARCORE_HELLO_AR_VULKAN_UTIL_H_

#include <android/asset_manager.h>
#include <android/log.h>
#include <errno.h>
#include <jni.h>
#include <vulkan/vulkan.h>

#include <cstdint>
#include <cstdlib>
#include <map>
#include <vector>

#include "arcore_c_api.h"
#include "glm.h"

#ifndef LOGI
#define LOGI(...)                                                    \
  __android_log_print(ANDROID_LOG_INFO, "hello_ar_vulkan_example_c", \
                      __VA_ARGS__)
#endif  // LOGI

#ifndef LOGE
#define LOGE(...)                                                     \
  __android_log_print(ANDROID_LOG_ERROR, "hello_ar_vulkan_example_c", \
                      __VA_ARGS__)
#endif  // LOGE

#ifndef CHECK
#define CHECK(condition)                                                   \
  if (!(condition)) {                                                      \
    LOGE("*** CHECK FAILED at %s:%d: %s", __FILE__, __LINE__, #condition); \
    abort();                                                               \
  }
#endif  // CHECK

#ifndef CHECKANDTHROW
#define CHECKANDTHROW(condition, env, msg, ...)                            \
  if (!(condition)) {                                                      \
    LOGE("*** CHECK FAILED at %s:%d: %s", __FILE__, __LINE__, #condition); \
    util::ThrowJavaException(env, msg);                                    \
    return ##__VA_ARGS__;                                                  \
  }
#endif  // CHECKANDTHROW

namespace hello_ar_vulkan {

// Utilities for C vulkan project.
namespace util {

// @struct Vertex Information to be processed in the vertex shader.
struct VertexInfo {
  // pos_x: x-axis position in the screen where the vertex of the image will
  // be rendered. Normalized between -1 (left) and 1 (right).
  float pos_x;
  // pos_y: y-axis position in the screen where the vertex of the image will
  // be rendered. Normalized between -1 (top) and 1 (bottom).
  float pos_y;
  // tex_u: x-axis position of the texture to be rendered. Normalized between
  // 0 (left) and 1 (right).
  float tex_u;
  // tex_v: y-axis position of the texture to be rendered. Normalized between
  // 0 (top) and 1 (bottom).
  float tex_v;
};

// Structure with texture image.
struct TextureImageInfo {
  VkImageView texture_image_view;
  VkDeviceMemory texture_memory;
  VkImage texture_image;
};

// Structure with swapchain image's relative.
struct SwapchainImageRelative {
  VkImageView swapchain_view;
  VkFramebuffer frame_buffer;
};

struct InstanceData {
  glm::mat4 model_view;
  glm::vec4 color;
};

constexpr size_t kMaxNumberOfAndroidsToRender = 20;

// Provides a scoped allocated instance of Anchor.
// Can be treated as an ArAnchor*.
class ScopedArPose {
 public:
  explicit ScopedArPose(const ArSession* session) {
    ArPose_create(session, nullptr, &pose_);
  }
  ~ScopedArPose() { ArPose_destroy(pose_); }
  ArPose* GetArPose() { return pose_; }
  // Delete copy constructors.
  ScopedArPose(const ScopedArPose&) = delete;
  void operator=(const ScopedArPose&) = delete;

 private:
  ArPose* pose_;
};

// Throw a Java exception.
//
// @param env, the JNIEnv.
// @param msg, the message of this exception.
void ThrowJavaException(JNIEnv* env, const char* msg);

// Calculate the distance between the plane and the camera.
//
// @param ar_session, ArSession pointer.
// @param plane_pose, ArPose of the plane.
// @param camera_pose, ArPose of the camera.
// @return the distance between the plane and the camera.
float CalculateDistanceToPlane(const ArSession& ar_session,
                               const ArPose& plane_pose,
                               const ArPose& camera_pose);

// Load a text file from assets folder.
//
// @param asset_manager, AAssetManager pointer.
// @param file_name, path to the file, relative to the assets folder.
// @param out_string, output string.
// @return true if the file is loaded correctly, otherwise false.
bool LoadTextFileFromAssetManager(const char* file_name,
                                  AAssetManager* asset_manager,
                                  std::string* out_file_text_string);

// Load a PNG file from the assets folder and return the image data.
//
// @param path, path to the file, relative to the assets folder.
// @param width, output width of the image.
// @param height, output height of the image.
// @param image_size, output size of the image.
// @param pixel_data, output pixel data of the image.
// @return true if png is loaded correctly, otherwise false.
bool LoadPngFromAssetManager(const char* path, uint32_t& width,
                             uint32_t& height, VkDeviceSize& image_size,
                             void*& pixel_data);

// Format and output the matrix to logcat file.
// Note that this function output matrix in row major.
void Log4x4Matrix(const float raw_matrix[16]);

// Get the transform matrix from the anchor.
//
// @param ar_anchor, ArAnchor pointer.
// @param ar_session, ArSession pointer.
// @param out_model_mat, output model matrix.
void GetTransformMatrixFromAnchor(const ArAnchor& ar_anchor,
                                  ArSession* ar_session,
                                  glm::mat4* out_model_mat);

// Get the normal vector of the plane.
//
// @param ar_session, ArSession pointer.
// @param plane_pose, ArPose of the plane.
// @return the normal vector of the plane.
glm::vec3 GetPlaneNormal(const ArSession& ar_session, const ArPose& plane_pose);

// Helper function to parse float data from a line
bool ParseVertexAttribute(const char* line_header, const char* format,
                          std::vector<float>& out_data, int expected_matches);

// Load obj file from assets folder from the app.
//
// @param asset_manager, AAssetManager pointer.
// @param file_name, name of the obj file.
// @param out_vertices, output vertices.
// @param out_normals, output normals.
// @param out_uv, output texture UV coordinates.
// @param out_indices, output triangle indices.
// @return true if obj is loaded correctly, otherwise false.
bool LoadObjFile(const std::string& file_name, AAssetManager* asset_manager,
                 std::vector<float>& out_vertices,
                 std::vector<float>& out_normals, std::vector<float>& out_uv,
                 std::vector<uint16_t>& out_indices);

}  // namespace util
}  // namespace hello_ar_vulkan

#endif  // C_ARCORE_HELLO_AR_VULKAN_UTIL_H_
