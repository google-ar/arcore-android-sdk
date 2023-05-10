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

#include "simple_vulkan_application.h"

#include <android/asset_manager.h>
#include <android/native_window_jni.h>

#include <array>
#include <cstdint>

#include "arcore_c_api.h"
#include "android_vulkan_loader.h"
#include "util.h"
#include "vulkan_handler.h"

namespace simple_vulkan {
namespace {

constexpr size_t kMaxNumberOfAndroidsToRender = 20;

// The coordinates of vertices in the Android view.
const float kVertices[] = {
    0.0f, 0.0f,  // Top Left
    1.0f, 0.0f,  // Top Right
    1.0f, 1.0f,  // Bottom Right
    0.0f, 1.0f,  // Bottom Left
};

// Assumed distance from the device camera to the surface on which user will
// try to place objects. This value affects the apparent scale of objects
// while the tracking method of the Instant Placement point is
// SCREENSPACE_WITH_APPROXIMATE_DISTANCE. Values in the [0.2, 2.0] meter
// range are a good choice for most AR experiences. Use lower values for AR
// experiences where users are expected to place objects on surfaces close
// to the camera. Use larger values for experiences where the user will
// likely be standing and trying to place an object on the ground or floor
// in front of them.
constexpr float kApproximateDistanceMeters = 1.0f;

void SetColor(float r, float g, float b, float a, float* color4f) {
  color4f[0] = r;
  color4f[1] = g;
  color4f[2] = b;
  color4f[3] = a;
}

}  // namespace

SimpleVulkanApplication::SimpleVulkanApplication(AAssetManager* asset_manager)
    : asset_manager_(asset_manager) {}

SimpleVulkanApplication::~SimpleVulkanApplication() {
  if (ar_session_ != nullptr) {
    ArSession_destroy(ar_session_);
    ArFrame_destroy(ar_frame_);
  }
}

void SimpleVulkanApplication::OnPause() {
  if (ar_session_ != nullptr) {
    ArSession_pause(ar_session_);
  }
}

void SimpleVulkanApplication::OnResume(JNIEnv* env, void* context,
                                       void* activity) {
  if (ar_session_ == nullptr) {
    ArInstallStatus install_status;
    // If install was not yet requested, that means that we are resuming the
    // activity first time because of explicit user interaction (such as
    // launching the application)
    bool user_requested_install = !install_requested_;

    // === ATTENTION!  ATTENTION!  ATTENTION! ===
    // This method can and will fail in user-facing situations.  Your
    // application must handle these cases at least somewhat gracefully.  See
    // HelloAR Java sample code for reasonable behavior.
    CHECKANDTHROW(
        ArCoreApk_requestInstall(env, activity, user_requested_install,
                                 &install_status) == AR_SUCCESS,
        env, "Please install Google Play Services for AR (ARCore).");

    switch (install_status) {
      case AR_INSTALL_STATUS_INSTALLED:
        break;
      case AR_INSTALL_STATUS_INSTALL_REQUESTED:
        install_requested_ = true;
        return;
    }

    // === ATTENTION!  ATTENTION!  ATTENTION! ===
    // This method can and will fail in user-facing situations.  Your
    // application must handle these cases at least somewhat gracefully.  See
    // HelloAR Java sample code for reasonable behavior.
    CHECKANDTHROW(ArSession_create(env, context, &ar_session_) == AR_SUCCESS,
                  env, "Failed to create AR session.");

    ConfigureSession();
    ArFrame_create(ar_session_, &ar_frame_);

    ArSession_setDisplayGeometry(ar_session_, display_rotation_, width_,
                                 height_);
  }

  const ArStatus status = ArSession_resume(ar_session_);
  CHECKANDTHROW(status == AR_SUCCESS, env, "Failed to resume AR session.");
}

void SimpleVulkanApplication::OnSurfaceCreated(JNIEnv* env,
                                               jobject surface_obj) {
  window_.reset(ANativeWindow_fromSurface(env, surface_obj));
  vulkan_handler_ =
      std::make_unique<VulkanHandler>(window_.get(), MAX_FRAMES_IN_FLIGHT);
}

void SimpleVulkanApplication::OnDisplayGeometryChanged(int display_rotation,
                                                       int width, int height) {
  display_rotation_ = display_rotation;
  width_ = width;
  height_ = height;
  if (ar_session_ != nullptr) {
    ArSession_setDisplayGeometry(ar_session_, display_rotation, width, height);
  }
}

void SimpleVulkanApplication::OnDrawFrame() {
  if (ar_session_ == nullptr) return;

  if (ArSession_update(ar_session_, ar_frame_) != AR_SUCCESS) {
    LOGE("SimpleVulkanApplication::OnDrawFrame ArSession_update error");
    return;
  }

  void* native_hardware_buffer = nullptr;
  ArFrame_getHardwareBuffer(ar_session_, ar_frame_, &native_hardware_buffer);

  if ((int64_t)native_hardware_buffer == 0) {
    LOGE("HardwareBuffer isn't ready.");
    return;
  }

  vulkan_handler_->WaitForFrame(current_frame_);

  const uint32_t next_swapchain_image_index =
      vulkan_handler_->AcquireNextImage(current_frame_);
  // If next index is invalid, return directly.
  if (next_swapchain_image_index < 0) {
    return;
  }

  // If display rotation changed (also includes view size change), we need to
  // re-query the uv coordinates for the on-screen portion of the camera image.
  // Since there is slight difference between the return results of
  // `ArFrame_transformCoordinates2d`, if geometry changed, we will refresh
  // all the frames in the flight.
  int32_t geometry_changed = 0;
  ArFrame_getDisplayGeometryChanged(ar_session_, ar_frame_, &geometry_changed);
  if (geometry_changed != 0 ||
      (!vulkan_handler_->IsVerticesSetForFrame(current_frame_) &&
       frames_to_set_vertices == 0)) {
    ArFrame_transformCoordinates2d(
        ar_session_, ar_frame_, AR_COORDINATES_2D_VIEW_NORMALIZED, kNumVertices,
        kVertices, AR_COORDINATES_2D_TEXTURE_NORMALIZED, transformed_uvs_);
    frames_to_set_vertices = MAX_FRAMES_IN_FLIGHT;
  }

  if (frames_to_set_vertices > 0) {
    // These vertices represent 4 corners of the screen. One tuple is one
    // vertex. The first two floats in the tuple represent the screen
    // coordinates in vulkan (Details: http://vulkano.rs/guide/vertex-input).
    // The later two represent the texture coordinates which is fetched from
    // ARCore `ArFrame_transformCoordinates2d`.
    const std::vector<VulkanHandler::VertexInfo> vertices = {
        {-1.0f, -1.0f, transformed_uvs_[0], transformed_uvs_[1]},  // Top Left
        {1.0f, -1.0f, transformed_uvs_[2], transformed_uvs_[3]},   // Top Right
        {1.0f, 1.0f, transformed_uvs_[4], transformed_uvs_[5]},  // Bottom Right
        {-1.0f, 1.0f, transformed_uvs_[6], transformed_uvs_[7]},  // Bottom Left
    };

    // The indices of above vertices. The first 3 and the later 3 integer
    // represents two triangles covering the whole screen.
    std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0};
    vulkan_handler_->SetVerticesAndIndicesForFrame(current_frame_, vertices,
                                                   indices);
    frames_to_set_vertices--;
  }

  vulkan_handler_->BeginRecordingCommandBuffer(current_frame_);
  vulkan_handler_->BeginRenderPass(current_frame_, next_swapchain_image_index);
  vulkan_handler_->RenderFromHardwareBuffer(
      current_frame_,
      reinterpret_cast<AHardwareBuffer*>(native_hardware_buffer));
  vulkan_handler_->EndRenderPass(current_frame_);
  vulkan_handler_->FinishRecordingCommandBuffer(current_frame_);
  vulkan_handler_->SubmitRecordingCommandBuffer(current_frame_);
  vulkan_handler_->PresentRecordingCommandBuffer(current_frame_,
                                                 next_swapchain_image_index);

  ArCamera* ar_camera;
  ArFrame_acquireCamera(ar_session_, ar_frame_, &ar_camera);

  ArTrackingState camera_tracking_state;
  ArCamera_getTrackingState(ar_session_, ar_camera, &camera_tracking_state);
  ArCamera_release(ar_camera);

  current_frame_ = (current_frame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

void SimpleVulkanApplication::ConfigureSession() {
  ArConfig* ar_config = nullptr;
  ArConfig_create(ar_session_, &ar_config);

  ArConfig_setTextureUpdateMode(ar_session_, ar_config,
                                AR_TEXTURE_UPDATE_MODE_EXPOSE_HARDWARE_BUFFER);

  CHECK(ar_config);
  CHECK(ArSession_configure(ar_session_, ar_config) == AR_SUCCESS);
  ArConfig_destroy(ar_config);
}

}  // namespace simple_vulkan
