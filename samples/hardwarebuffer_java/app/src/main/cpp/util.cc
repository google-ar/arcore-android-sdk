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
#include "util.h"

#include <unistd.h>

#include <sstream>
#include <string>

#include "jni_interface.h"

namespace hello_ar {
namespace util {

void CheckGlError(const char* operation) {
  bool anyError = false;
  for (GLint error = glGetError(); error; error = glGetError()) {
    LOGE("after %s() glError (0x%x)\n", operation, error);
    anyError = true;
  }
  if (anyError) {
    abort();
  }
}

void ThrowJavaException(JNIEnv* env, const char* msg) {
  LOGE("Throw Java exception: %s", msg);
  jclass c = env->FindClass("java/lang/RuntimeException");
  env->ThrowNew(c, msg);
}

}  // namespace util
}  // namespace hello_ar
