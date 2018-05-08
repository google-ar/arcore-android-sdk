/*
 * Copyright 2017 Google Inc. All Rights Reserved.
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

#include "cloud_anchor_manager.h"
#include "hello_ar_application.h"
#include "util.h"

#define _PREPEND_CLASS_NAME(_m) \
  Java_com_google_ar_core_examples_c_cloudanchor_JniInterface_##_m

#define JNI_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL _PREPEND_CLASS_NAME(method_name)

extern "C" {

namespace {

struct ApplicationContext {
  std::shared_ptr<cloud_anchor::CloudAnchorManager> cloud_anchor_manager;
  std::unique_ptr<cloud_anchor::HelloArApplication> cloud_anchor_application;
};

// maintain a reference to the JVM so we can use it later.
static JavaVM *g_vm = nullptr;

inline jlong jptr(ApplicationContext *native_hello_ar_application) {
  return reinterpret_cast<intptr_t>(native_hello_ar_application);
}

inline ApplicationContext *native(jlong ptr) {
  return reinterpret_cast<ApplicationContext *>(ptr);
}

}  // namespace

jint JNI_OnLoad(JavaVM *vm, void *) {
  g_vm = vm;
  return JNI_VERSION_1_6;
}

JNI_METHOD(jlong, createNativeApplication)
(JNIEnv *env, jclass, jobject j_asset_manager) {
  AAssetManager *asset_manager = AAssetManager_fromJava(env, j_asset_manager);

  auto application_context = new ApplicationContext();
  auto new_cloud_manager = std::make_shared<cloud_anchor::CloudAnchorManager>();

  application_context->cloud_anchor_manager = new_cloud_manager;
  application_context->cloud_anchor_application =
      std::unique_ptr<cloud_anchor::HelloArApplication>(
          new cloud_anchor::HelloArApplication(asset_manager,
                                               new_cloud_manager));

  return jptr(application_context);
}

JNI_METHOD(void, destroyNativeApplication)
(JNIEnv *, jclass, jlong native_application) {
  delete native(native_application);
}

JNI_METHOD(void, onPause)
(JNIEnv *, jclass, jlong native_application) {
  native(native_application)->cloud_anchor_manager->OnPause();
  native(native_application)->cloud_anchor_application->OnPause();
}

JNI_METHOD(void, onResume)
(JNIEnv *env, jclass, jlong native_application, jobject context,
 jobject activity) {
  native(native_application)
      ->cloud_anchor_application->OnResume(env, context, activity);
}

JNI_METHOD(void, onGlSurfaceCreated)
(JNIEnv *, jclass, jlong native_application) {
  native(native_application)->cloud_anchor_application->OnSurfaceCreated();
}

JNI_METHOD(void, onDisplayGeometryChanged)
(JNIEnv *, jobject, jlong native_application, int display_rotation, int width,
 int height) {
  native(native_application)
      ->cloud_anchor_application->OnDisplayGeometryChanged(display_rotation,
                                                           width, height);
}

JNI_METHOD(void, onGlSurfaceDrawFrame)
(JNIEnv *, jclass, jlong native_application) {
  native(native_application)->cloud_anchor_application->OnDrawFrame();
}

JNI_METHOD(void, onTouched)
(JNIEnv *, jclass, jlong native_application, jfloat x, jfloat y) {
  native(native_application)->cloud_anchor_application->OnTouched(x, y);
}

JNI_METHOD(void, onHostButtonPress)
(JNIEnv *, jclass, jlong native_application) {
  native(native_application)->cloud_anchor_manager->OnHostButtonPress();
}

JNI_METHOD(void, onResolveButtonPress)
(JNIEnv *, jclass, jlong native_application) {
  native(native_application)->cloud_anchor_manager->OnResolveButtonPress();
}

JNI_METHOD(void, onResolveOkPress)
(JNIEnv *, jclass, jlong native_application, jlong room_code) {
  native(native_application)
      ->cloud_anchor_manager->OnResolveDialogOkPress(room_code);
}

JNI_METHOD(void, onResolveCancelPress)
(JNIEnv *, jclass, jlong native_application) {
  native(native_application)
      ->cloud_anchor_manager->OnResolveDialogCancelPress();
}

JNI_METHOD(void, onFirebaseError)
(JNIEnv *, jclass, jlong native_application) {
  native(native_application)->cloud_anchor_manager->OnFirebaseError();
}

JNI_METHOD(void, onCloudAnchorIdMadeAvailable)
(JNIEnv *env, jclass, jlong native_application, jstring j_anchor_id) {
  const char *nativeString = env->GetStringUTFChars(j_anchor_id, nullptr);
  std::string anchor_id(nativeString);
  native(native_application)
      ->cloud_anchor_manager->OnCloudAnchorIdMadeAvailable(anchor_id);
  env->ReleaseStringUTFChars(j_anchor_id, nativeString);
}

JNI_METHOD(jboolean, hasDetectedPlanes)
(JNIEnv *, jclass, jlong native_application) {
  return static_cast<jboolean>(
      native(native_application)->cloud_anchor_application->HasDetectedPlanes()
          ? JNI_TRUE
          : JNI_FALSE);
}

JNI_METHOD(jstring, getCloudAnchorId)
(JNIEnv *env, jclass, jlong native_application) {
  auto cloud_anchor_manager = native(native_application)->cloud_anchor_manager;
  if (cloud_anchor_manager->GetCloudAnchor() == nullptr) {
    return env->NewStringUTF(nullptr);
  } else {
    return env->NewStringUTF(cloud_anchor_manager->GetCloudAnchorId().c_str());
  }
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
