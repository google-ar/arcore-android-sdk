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

#ifndef C_ARCORE_SIMPLE_VULKAN_UTIL_H_
#define C_ARCORE_SIMPLE_VULKAN_UTIL_H_

#include <android/asset_manager.h>
#include <android/log.h>
#include <errno.h>
#include <jni.h>

#include <cstdint>
#include <cstdlib>
#include <map>
#include <vector>

#include "arcore_c_api.h"

#ifndef LOGI
#define LOGI(...) \
  __android_log_print(ANDROID_LOG_INFO, "simple_vulkan_example_c", __VA_ARGS__)
#endif  // LOGI

#ifndef LOGE
#define LOGE(...) \
  __android_log_print(ANDROID_LOG_ERROR, "simple_vulkan_example_c", __VA_ARGS__)
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

namespace simple_vulkan {

// Utilities for C simple vulkan project.
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

// Throw a Java exception.
//
// @param env, the JNIEnv.
// @param msg, the message of this exception.
void ThrowJavaException(JNIEnv* env, const char* msg);

// Load a text file from assets folder.
//
// @param asset_manager, AAssetManager pointer.
// @param file_name, path to the file, relative to the assets folder.
// @param out_string, output string.
// @return true if the file is loaded correctly, otherwise false.
bool LoadTextFileFromAssetManager(const char* file_name,
                                  AAssetManager* asset_manager,
                                  std::string* out_file_text_string);

// Load png file from assets folder and then assign it to the OpenGL target.
// This method must be called from the renderer thread since it will result in
// OpenGL calls to assign the image to the texture target.
//
// @param target, openGL texture target to load the image into.
// @param path, path to the file, relative to the assets folder.
// @return true if png is loaded correctly, otherwise false.
bool LoadPngFromAssetManager(int target, const char* path);

// Format and output the matrix to logcat file.
// Note that this function output matrix in row major.
void Log4x4Matrix(const float raw_matrix[16]);

}  // namespace util
}  // namespace simple_vulkan

#endif  // C_ARCORE_SIMPLE_VULKAN_UTIL_H_
