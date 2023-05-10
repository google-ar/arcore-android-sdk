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

#ifndef C_ARCORE_SIMPLE_VULKAN_SIMPLE_VULKAN_APPLICATION_H_
#define C_ARCORE_SIMPLE_VULKAN_SIMPLE_VULKAN_APPLICATION_H_

#include <android/asset_manager.h>
#include <android/native_window.h>
#include <jni.h>

#include <memory>
#include <set>
#include <string>
#include <unordered_map>

#include "arcore_c_api.h"
#include "android_vulkan_loader.h"
#include "util.h"
#include "vulkan_handler.h"

namespace simple_vulkan {

// SimpleVulkanApplication handles all application logics.
class SimpleVulkanApplication {
 public:
  const int MAX_FRAMES_IN_FLIGHT = 4;

  // Constructor and deconstructor.
  explicit SimpleVulkanApplication(AAssetManager* asset_manager);
  ~SimpleVulkanApplication();

  // OnPause is called on the UI thread from the Activity's onPause method.
  void OnPause();

  // OnResume is called on the UI thread from the Activity's onResume method.
  void OnResume(JNIEnv* env, void* context, void* activity);

  // OnSurfaceCreated is called when Surface View is created.
  void OnSurfaceCreated(JNIEnv* env, jobject surface_obj);

  // OnDisplayGeometryChanged is called on the OpenGL thread when the
  // render surface size or display rotation changes.
  //
  // @param display_rotation: current display rotation.
  // @param width: width of the changed surface view.
  // @param height: height of the changed surface view.
  void OnDisplayGeometryChanged(int display_rotation, int width, int height);

  // OnDrawFrame is called on the OpenGL thread to render the next frame.
  void OnDrawFrame();

 private:
  /**
   *  Custom deleter for ANativeWindow.
   */
  struct ANativeWindowDeleter {
    void operator()(ANativeWindow* window) { ANativeWindow_release(window); }
  };

  static constexpr int kNumVertices = 4;
  float transformed_uvs_[kNumVertices * 2];

  ArSession* ar_session_ = nullptr;
  ArFrame* ar_frame_ = nullptr;

  bool install_requested_ = false;
  int width_ = 1;
  int height_ = 1;
  int display_rotation_ = 0;
  int current_frame_ = 0;
  int frames_to_set_vertices = 0;

  AAssetManager* const asset_manager_;
  std::unique_ptr<VulkanHandler> vulkan_handler_;
  std::unique_ptr<ANativeWindow, ANativeWindowDeleter> window_;

  void ConfigureSession();
};
}  // namespace simple_vulkan

#endif  // C_ARCORE_SIMPLE_VULKAN_SIMPLE_VULKAN_APPLICATION_H_
