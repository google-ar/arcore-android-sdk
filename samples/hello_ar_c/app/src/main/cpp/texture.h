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

#ifndef THIRD_PARTY_ARCORE_JAVA_COM_GOOGLE_AR_CORE_EXAMPLES_C_HELLOAR_CPP_TEXTURE_H_
#define THIRD_PARTY_ARCORE_JAVA_COM_GOOGLE_AR_CORE_EXAMPLES_C_HELLOAR_CPP_TEXTURE_H_

#include "arcore_c_api.h"

namespace hello_ar {

/**
 * Handle the creation and update of a GPU texture.
 **/
class Texture {
 public:
  Texture() = default;
  ~Texture() = default;

  void CreateOnGlThread();
  void UpdateWithDepthImageOnGlThread(const ArSession& session,
                                      const ArFrame& frame);
  unsigned int GetTextureId() { return texture_id_; }

  unsigned int GetWidth() { return width_; }

  unsigned int GetHeight() { return height_; }

 private:
  unsigned int texture_id_ = 0;
  unsigned int width_ = 1;
  unsigned int height_ = 1;
};
}  // namespace hello_ar

#endif  // THIRD_PARTY_ARCORE_JAVA_COM_GOOGLE_AR_CORE_EXAMPLES_C_HELLOAR_CPP_TEXTURE_H_
