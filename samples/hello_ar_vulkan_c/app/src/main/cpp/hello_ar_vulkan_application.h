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

#ifndef C_ARCORE_HELLO_AR_VULKAN_APPLICATION_H_
#define C_ARCORE_HELLO_AR_VULKAN_APPLICATION_H_

#include <android/asset_manager.h>
#include <android/native_window.h>
#include <jni.h>

#include <deque>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>

#include "arcore_c_api.h"
#include "android_vulkan_loader.h"
#include "background_renderer.h"
#include "depth_map.h"
#include "model_renderer.h"
#include "plane_renderer.h"
#include "point_cloud_renderer.h"
#include "util.h"
#include "vulkan_handler.h"

namespace hello_ar_vulkan {

// HelloArVulkanApplication handles all application logic.
class HelloArVulkanApplication {
 public:
  const int MAX_FRAMES_IN_FLIGHT = 4;

  // Constructor and deconstructor.
  explicit HelloArVulkanApplication(AAssetManager* asset_manager);
  ~HelloArVulkanApplication();

  // OnPause is called on the UI thread from the Activity's onPause method.
  void OnPause();

  // OnResume is called on the UI thread from the Activity's onResume method.
  void OnResume(JNIEnv* env, void* context, void* activity);

  // OnSurfaceCreated is called when Surface View is created.
  void OnSurfaceCreated(JNIEnv* env, jobject surface_obj);

  // OnSurfaceDestroyed is called when Surface View is destroyed.
  void OnSurfaceDestroyed();

  // OnDisplayGeometryChanged is called on the OpenGL thread when the
  // render surface size or display rotation changes.
  //
  // @param display_rotation: current display rotation.
  // @param width: width of the changed surface view.
  // @param height: height of the changed surface view.
  void OnDisplayGeometryChanged(int display_rotation, int width, int height);

  // OnDrawFrame is called on the OpenGL thread to render the next frame.
  void OnDrawFrame(bool depth_color_visualization_enabled,
                   bool use_depth_for_occlusion);

  // OnTouched is called on the OpenGL thread after the user touches the screen.
  // @param x: x position on the screen (pixels).
  // @param y: y position on the screen (pixels).
  void OnTouched(float x, float y);

  // Returns true if any planes have been detected.  Used for hiding the
  // "searching for planes" snackbar.
  bool HasDetectedPlanes() const { return plane_count_ > 0; }

  // Returns true if depth is supported.
  bool IsDepthSupported();

  // Called when the Instant Placement setting is changed.
  // @param is_instant_placement_enabled: whether Instant Placement is enabled.
  void OnSettingsChange(bool is_instant_placement_enabled);

 private:
  /**
   *  Custom deleter for ANativeWindow.
   */
  struct ANativeWindowDeleter {
    void operator()(ANativeWindow* window) { ANativeWindow_release(window); }
  };

  /**
   *  Returns the texture transform matrix for the given frame.
   *
   *  @param session: The AR session.
   *  @param frame: The AR frame.
   *  @return The texture transform matrix.
   */
  glm::mat3 GetTextureTransformMatrix(const ArSession* session,
                                      const ArFrame* frame);

  static constexpr int kNumVertices = 4;
  float transformed_uvs_[kNumVertices * 2];

  ArSession* ar_session_ = nullptr;
  ArFrame* ar_frame_ = nullptr;

  bool install_requested_ = false;
  bool is_instant_placement_enabled_ = true;
  int width_ = 1;
  int height_ = 1;
  int display_rotation_ = 0;
  int current_frame_ = 0;
  int frames_to_set_vertices = 0;
  int plane_count_ = 0;
  glm::mat3 texture_transform_matrix_;

  AAssetManager* const asset_manager_;
  // Parent object for all Vulkan resources.
  std::unique_ptr<VulkanHandler> vulkan_handler_;
  // Individual renderers which use information from vulkan_handler_.
  std::unique_ptr<BackgroundRenderer> background_renderer_;
  std::unique_ptr<PointCloudRenderer> point_cloud_renderer_;
  std::unique_ptr<PlaneRenderer> plane_renderer_;
  std::unique_ptr<DepthMapRenderer> depth_map_renderer_;
  std::unique_ptr<ModelRenderer> model_renderer_;
  std::unique_ptr<ANativeWindow, ANativeWindowDeleter> window_;

  // The anchors we use to draw Android models (with a specific color).
  struct ColoredAnchor {
    ArAnchor* anchor;
    ArTrackable* trackable;
    glm::vec4 color;
  };

  std::deque<ColoredAnchor> anchors_;
  std::vector<util::InstanceData> instance_data_;

  // Configures the ARCore session.
  void ConfigureSession();

  /**
   *  Updates the color of the anchor based on its associated trackable type.
   *
   *  @param colored_anchor: The anchor to update.
   */
  void UpdateAnchorColor(ColoredAnchor* colored_anchor);
};
}  // namespace hello_ar_vulkan

#endif  // C_ARCORE_HELLO_AR_VULKAN_APPLICATION_H_
