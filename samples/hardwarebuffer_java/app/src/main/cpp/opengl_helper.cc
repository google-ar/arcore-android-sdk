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

#include "opengl_helper.h"

#define EGL_EGLEXT_PROTOTYPES

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#include <android/asset_manager.h>

#include <array>
#include <cstdint>

#include "util.h"

namespace hello_ar {
namespace {

// Load hardware buffer symbols at runtime.
using PFeglGetNativeClientBufferANDROID =
    EGLClientBuffer (*)(const AHardwareBuffer* buffer);

PFeglGetNativeClientBufferANDROID eglGetNativeClientBufferANDROID = nullptr;

}  // namespace

OpenGlHelper::OpenGlHelper() {
  if (eglGetNativeClientBufferANDROID == nullptr) {
    eglGetNativeClientBufferANDROID =
        reinterpret_cast<PFeglGetNativeClientBufferANDROID>(
            eglGetProcAddress("eglGetNativeClientBufferANDROID"));
    if (eglGetNativeClientBufferANDROID == nullptr) {
      LOGE("eglGetNativeClientBufferANDROID symbol does not exist.");
      return;
    }
  }
}

OpenGlHelper::~OpenGlHelper() {}

EGLImageKHR OpenGlHelper::CreateEglImage(AHardwareBuffer* buffer) {
  if (eglGetNativeClientBufferANDROID == nullptr) {
    LOGE("eglGetNativeClientBufferANDROID symbol does not exist.");
    return EGL_NO_IMAGE;
  }

  EGLClientBuffer egl_client_buffer = eglGetNativeClientBufferANDROID(buffer);

  EGLint attr[] = {EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE};
  EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

  return eglCreateImageKHR(display, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID,
                           egl_client_buffer, attr);
}

void OpenGlHelper::DestroyEglImage(EGLImageKHR image) {
  EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  eglDestroyImageKHR(display, image);
}

void OpenGlHelper::BindEglImageToTexture(EGLImageKHR image,
                                         uint32_t texture_oes_id) {
  glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_oes_id);
  util::CheckGlError("glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_oes_id)");
  glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, image);
  util::CheckGlError("glEGLImageTargetTexture2DOES");
  glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
  util::CheckGlError("glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0)");
}

}  // namespace hello_ar
