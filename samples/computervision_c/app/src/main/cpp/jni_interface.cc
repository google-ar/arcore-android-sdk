/*
 * Copyright 2018 Google LLC
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

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <jni.h>

#include "computer_vision_application.h"

#define JNI_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
    Java_com_google_ar_core_examples_c_computervision_JniInterface_##method_name

extern "C" {

namespace {
// Maintain a reference to the JVM so we can use it later.
static JavaVM *g_vm = nullptr;

inline jlong jptr(computer_vision::ComputerVisionApplication
                      *native_computer_vision_application) {
  return reinterpret_cast<intptr_t>(native_computer_vision_application);
}

inline computer_vision::ComputerVisionApplication *native(jlong ptr) {
  return reinterpret_cast<computer_vision::ComputerVisionApplication *>(ptr);
}

}  // namespace

jint JNI_OnLoad(JavaVM *vm, void *) {
  g_vm = vm;
  return JNI_VERSION_1_6;
}

JNI_METHOD(jlong, createNativeApplication)
(JNIEnv *env, jclass, jobject j_asset_manager) {
  AAssetManager *asset_manager = AAssetManager_fromJava(env, j_asset_manager);
  return jptr(new computer_vision::ComputerVisionApplication(asset_manager));
}

JNI_METHOD(void, destroyNativeApplication)
(JNIEnv *, jclass, jlong native_application) {
  delete native(native_application);
}

JNI_METHOD(void, onPause)
(JNIEnv *, jclass, jlong native_application) {
  native(native_application)->OnPause();
}

JNI_METHOD(void, onResume)
(JNIEnv *env, jclass, jlong native_application, jobject context,
 jobject activity) {
  native(native_application)->OnResume(env, context, activity);
}

JNI_METHOD(void, onGlSurfaceCreated)
(JNIEnv *, jclass, jlong native_application) {
  native(native_application)->OnSurfaceCreated();
}

JNI_METHOD(void, onDisplayGeometryChanged)
(JNIEnv *, jobject, jlong native_application, int display_rotation,
 int camera_to_display_rotation, int width, int height) {
  native(native_application)
      ->OnDisplayGeometryChanged(display_rotation, camera_to_display_rotation,
                                 width, height);
}

JNI_METHOD(void, onGlSurfaceDrawFrame)
(JNIEnv *, jclass, jlong native_application, jfloat split_position) {
  native(native_application)->OnDrawFrame(split_position);
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

JNI_METHOD(jstring, getCameraConfigLabel)
(JNIEnv *env, jclass, jlong native_application, jboolean is_low_resolution) {
  auto label =
      native(native_application)->getCameraConfigLabel(is_low_resolution);
  return env->NewStringUTF(label.c_str());
}

JNI_METHOD(jint, setCameraConfig)
(JNIEnv *env, jclass, jlong native_application, jboolean is_low_resolution) {
  ArStatus status =
      native(native_application)->setCameraConfig(is_low_resolution);
  return static_cast<jint>(status);
}

JNI_METHOD(jstring, getCameraIntrinsicsText)
(JNIEnv *env, jclass, jlong native_application, jboolean for_gpu_texture) {
  auto label =
      native(native_application)->GetCameraIntrinsicsText(for_gpu_texture);
  return env->NewStringUTF(label.c_str());
}

JNI_METHOD(void, setFocusMode)
(JNIEnv *, jclass, jlong native_application, jboolean enable_auto_focus) {
  native(native_application)->SetFocusMode(enable_auto_focus);
}

JNI_METHOD(jboolean, getFocusMode)
(JNIEnv *, jclass, jlong native_application) {
  return native(native_application)->GetFocusMode();
}

}  // extern "C"
