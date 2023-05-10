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

#define EGL_EGLEXT_PROTOTYPES

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl32.h>
#include <android/api-level.h>
#include <android/hardware_buffer_jni.h>

#include <cstdint>

#include "opengl_helper.h"

#define JNI_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
Java_com_google_ar_core_examples_java_helloarhardwarebuffer_JniInterface_##method_name

extern "C" {

namespace {
// maintain a reference to the JVM and OpenGlHelper so we can use it later.
static JavaVM *g_vm = nullptr;
static hello_ar::OpenGlHelper *opengl_helper = nullptr;
}  // namespace

jint JNI_OnLoad(JavaVM *vm, void *) {
  g_vm = vm;
  opengl_helper = new hello_ar::OpenGlHelper();
  return JNI_VERSION_1_6;
}

JNI_METHOD(jlong, createEglImage)
(JNIEnv *env, jclass, jobject hardware_buffer) {
#if (__ANDROID_API__ >= 26)
  EGLImageKHR image = opengl_helper->CreateEglImage(
      AHardwareBuffer_fromHardwareBuffer(env, hardware_buffer));

  return image == EGL_NO_IMAGE ? 0 : reinterpret_cast<intptr_t>(image);
#else
  jclass jcls = env->FindClass("java/lang/UnsupportedOperationException");
  env->ThrowNew(jcls,
                "Hardware Buffer is not supported on compiled NDK level.");
  return 0;
#endif
}

JNI_METHOD(void, destroyEglImage)
(JNIEnv *env, jclass, jlong image_address) {
  if (image_address == 0) return;
  opengl_helper->DestroyEglImage(reinterpret_cast<EGLImageKHR>(image_address));
}

JNI_METHOD(void, bindEglImageToTexture)
(JNIEnv *env, jclass, jlong image_address, jlong texture_id) {
  opengl_helper->BindEglImageToTexture(
      reinterpret_cast<EGLImageKHR>(image_address), texture_id);
}

JNIEnv *GetJniEnv() {
  JNIEnv *env;
  jint result = g_vm->AttachCurrentThread(&env, nullptr);
  return result == JNI_OK ? env : nullptr;
}

jclass FindClass(const char *classname) {
  JNIEnv *env = GetJniEnv();
  return env->FindClass(classname);
}

}  // extern "C"
