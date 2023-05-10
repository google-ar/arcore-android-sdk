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

#ifndef C_ARCORE_HELLOE_AR_UTIL_H_
#define C_ARCORE_HELLOE_AR_UTIL_H_

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
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
  __android_log_print(ANDROID_LOG_INFO, "hello_ar_example_c", __VA_ARGS__)
#endif  // LOGI

#ifndef LOGE
#define LOGE(...) \
  __android_log_print(ANDROID_LOG_ERROR, "hello_ar_example_c", __VA_ARGS__)
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

namespace hello_ar {

// Utilities for C hello AR project.
namespace util {

// Check GL error, and abort if an error is encountered.
//
// @param operation, the name of the GL function call.
void CheckGlError(const char* operation);

// Throw a Java exception.
//
// @param env, the JNIEnv.
// @param msg, the message of this exception.
void ThrowJavaException(JNIEnv* env, const char* msg);

}  // namespace util
}  // namespace hello_ar

#endif  // C_ARCORE_HELLOE_AR_UTIL_H_
