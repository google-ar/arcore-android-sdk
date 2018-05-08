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

#ifndef C_ARCORE_AUGMENTED_IMAGE_UTIL_H_
#define C_ARCORE_AUGMENTED_IMAGE_UTIL_H_

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <android/asset_manager.h>
#include <android/log.h>
#include <errno.h>
#include <jni.h>
#include <cstdint>
#include <cstdlib>
#include <vector>

#include "arcore_c_api.h"
#include "glm.h"

#ifndef LOGI
#define LOGI(...)                                                    \
  __android_log_print(ANDROID_LOG_INFO, "augmented_image_example_c", \
                      __VA_ARGS__)
#endif  // LOGI

#ifndef LOGE
#define LOGE(...)                                                     \
  __android_log_print(ANDROID_LOG_ERROR, "augmented_image_example_c", \
                      __VA_ARGS__)
#endif  // LOGE

#ifndef CHECK
#define CHECK(condition)                                                   \
  if (!(condition)) {                                                      \
    LOGE("*** CHECK FAILED at %s:%d: %s", __FILE__, __LINE__, #condition); \
    abort();                                                               \
  }
#endif  // CHECK

namespace augmented_image {

// Utilities for C AugmentedImage AR project.
namespace util {

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

// Looks up Java class IDs and Method IDs and cache them.
void InitializeJavaMethodIDs();

// Clear the Java class IDs and Method IDs.
void ReleaseJavaMethodIDs();

// Checks GL error, and abort if an error is encountered.
//
// @param operation, the name of the GL function call.
void CheckGlError(const char* operation);

// Creates a shader program ID.
//
// @param vertex_source, the vertex shader source.
// @param fragment_source, the fragment shader source.
// @return
GLuint CreateProgram(const char* vertex_source, const char* fragment_source);

// Loads png file from assets folder and then assign it to the OpenGL target.
// This method must be called from the renderer thread since it will result in
// OpenGL calls to assign the image to the texture target.
//
// @param target, openGL texture target to load the image into.
// @param path, path to the file, relative to the assets folder.
// @return true if png is loaded correctly, otherwise false.
bool LoadPngFromAssetManager(int target, const std::string& path);

// Loads image file from assets folder, then return raw pixel content.
// Support any images (png, jpg, etc) supported by BitmapFactory.decodeStream.
//
// @param path, file path in asset directory
// @param out_width, the output image width
// @param out_height, the output image height
// @param out_stride, the output image stride (bytes per line)
// @param out_pixel_buffer, the function that will allocate memory to store
//   image content, it is the caller's responsibility to release the memory
//   using delete[].
// @return true if image is loaded correctly, otherwise false.
bool LoadImageFromAssetManager(const std::string& path, int* out_width,
                               int* out_height, int* out_stride,
                               uint8_t** out_pixel_buffer);

// Hides the fit-to-scan image.
//
// @param activity a jobject value, pointing to the AugmentedImageActivity
// instance.
// @return true if function is successful, otherwise false
bool HideFitToScanImage(void* activity);

// Loads a file from asset.
//
// @param mgr, the pointer to AAssetManager
// @param file_name, the filename to load
// @param out_file_buffer, a pointer to a std::string object, it will
// be resized to the file content size, and populated with file content.
// @return true if file is loaded successfully, otherwise false.
bool LoadEntireAssetFile(AAssetManager* mgr, const std::string& file_name,
                         std::string* out_file_buffer);

// Loads obj file from assets folder from the app.
//
// @param mgr, AAssetManager pointer.
// @param file_name, name of the obj file.
// @param out_vertices, output vertices.
// @param out_normals, output normals.
// @param out_uv, output texture UV coordinates.
// @param out_indices, output triangle indices.
// @return true if obj is loaded correctly, otherwise false.
bool LoadObjFile(AAssetManager* mgr, const std::string& file_name,
                 std::vector<GLfloat>* out_vertices,
                 std::vector<GLfloat>* out_normals,
                 std::vector<GLfloat>* out_uv,
                 std::vector<GLushort>* out_indices);

// Formats and outputs the matrix to logcat file.
// Note that this function output matrix in row major.
void Log4x4Matrix(float raw_matrix[16]);

// Gets transformation matrix from ArAnchor.
void GetTransformMatrixFromAnchor(const ArSession* ar_session,
                                  const ArAnchor* ar_anchor,
                                  glm::mat4* out_model_mat);

// Converts image to grayscale.
// The AugmentedImage API takes a grayscale image as input,
// so we need to manually convert the image to grayscale
// Here we are using the luma component
// with luma = 0.213 * R + 0.715 * G + 0.072 * B
// ref: https://en.wikipedia.org/wiki/Grayscale
//
// @param image_pixel_buffer image raw pixel data, must be RGBA_8888 format.
// @param width, the image width.
// @param height, the image height
// @param stride, the image size in bytes per line.
// @param out_grayscale_buffer, the output image buffer in grayscale; it is
// the caller's responsibility to release the memory using delete[].
void ConvertRgbaToGrayscale(const uint8_t* image_pixel_buffer, int32_t width,
                            int32_t height, int32_t stride,
                            uint8_t** out_grayscale_buffer);

}  // namespace util
}  // namespace augmented_image

#endif  // C_ARCORE_AUGMENTED_IMAGE_UTIL_H_
