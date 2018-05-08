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

#ifndef C_ARCORE_COMPUTER_VISION_UTIL_H_
#define C_ARCORE_COMPUTER_VISION_UTIL_H_

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <android/log.h>

#include "arcore_c_api.h"

#ifndef LOGI
#define LOGI(...)                                                    \
  __android_log_print(ANDROID_LOG_INFO, "computer_vision_example_c", \
                      __VA_ARGS__)
#endif  // LOGI

#ifndef LOGE
#define LOGE(...)                                                     \
  __android_log_print(ANDROID_LOG_ERROR, "computer_vision_example_c", \
                      __VA_ARGS__)
#endif  // LOGE

#ifndef LOGW
#define LOGW(...)                                                    \
  __android_log_print(ANDROID_LOG_WARN, "computer_vision_example_c", \
                      __VA_ARGS__)
#endif  // LOGW

#ifndef CHECK
#define CHECK(condition)                                                   \
  if (!(condition)) {                                                      \
    LOGE("*** CHECK FAILED at %s:%d: %s", __FILE__, __LINE__, #condition); \
    abort();                                                               \
  }
#endif  // CHECK

namespace computer_vision {

// Utilities for C computer vision project.
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

// Check GL error, and abort if an error is encountered.
//
// @param operation, the name of the GL function call.
void CheckGlError(const char* operation);

// Create a shader program ID.
//
// @param vertex_source, the vertex shader source.
// @param fragment_source, the fragment shader source.
// @return
GLuint CreateProgram(const char* vertex_source, const char* fragment_source);
}  // namespace util
}  // namespace computer_vision

#endif  // C_ARCORE_COMPUTER_VISION_UTIL_H_
