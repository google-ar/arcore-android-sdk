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

#include "computer_vision_application.h"
#include <array>
#include <cmath>
#include <iomanip>
#include <sstream>

#include "util.h"

namespace computer_vision {
namespace {
constexpr float kRadiansToDegrees = static_cast<float>(180 / M_PI);

float GetViewportAspectRatio(int display_rotation, int viewport_width,
                             int viewport_height) {
  float aspect_ratio;
  switch (display_rotation) {
    case 1:  // Surface.ROTATION_90:
    case 3:  // Surface.ROTATION_270:
      aspect_ratio = static_cast<float>(viewport_height) /
                     static_cast<float>(viewport_width);
      break;
    case 0:  // Surface.ROTATION_0:
    case 2:  // Surface.ROTATION_180:
    default:
      aspect_ratio = static_cast<float>(viewport_width) /
                     static_cast<float>(viewport_height);
      break;
  }
  return aspect_ratio;
}

}  // namespace

ComputerVisionApplication::ComputerVisionApplication(
    AAssetManager* asset_manager)
    : asset_manager_(asset_manager) {}

ComputerVisionApplication::~ComputerVisionApplication() {
  if (ar_session_ != nullptr) {
    destroyCameraConfigs();
    ArSession_destroy(ar_session_);
    ArConfig_destroy(ar_config_);
  }
  if (ar_frame_ != nullptr) {
    ArFrame_destroy(ar_frame_);
  }
  if (ar_camera_intrinsics_ != nullptr) {
    ArCameraIntrinsics_destroy(ar_camera_intrinsics_);
  }
}

void ComputerVisionApplication::OnPause() {
  LOGI("OnPause()");
  if (ar_session_ != nullptr) {
    ArSession_pause(ar_session_);
  }
}

void ComputerVisionApplication::OnResume(JNIEnv* env, jobject context,
                                         jobject activity) {
  LOGI("OnResume()");

  if (ar_session_ == nullptr) {
    ArInstallStatus install_status;
    // If install was not yet requested, that means that we are resuming the
    // activity first time because of explicit user interaction (such as
    // launching the application)
    bool user_requested_install = !install_requested_;

    // === ATTENTION!  ATTENTION!  ATTENTION! ===
    // This method can and will fail in user-facing situations.  Your
    // application must handle these cases at least somewhat gracefully.  See
    // ComputerVision Java sample code for reasonable behavior.
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
    // ComputerVision Java sample code for reasonable behavior.
    CHECKANDTHROW(ArSession_create(env, context, &ar_session_) == AR_SUCCESS,
                  env, "Failed to create AR session.");
    CHECK(ar_session_);

    ArConfig_create(ar_session_, &ar_config_);
    CHECK(ar_config_);

    ArFrame_create(ar_session_, &ar_frame_);
    CHECK(ar_frame_);

    obtainCameraConfigs();

    ArCameraIntrinsics_create(ar_session_, &ar_camera_intrinsics_);
    CHECK(ar_camera_intrinsics_);

    ArSession_setDisplayGeometry(ar_session_, display_rotation_, width_,
                                 height_);
  }

  const ArStatus status = ArSession_resume(ar_session_);
  CHECKANDTHROW(status == AR_SUCCESS, env, "Failed to resume AR session.");
}

void ComputerVisionApplication::OnSurfaceCreated() {
  LOGI("OnSurfaceCreated()");
  cpu_image_renderer_.InitializeGlContent(asset_manager_);
}

void ComputerVisionApplication::OnDisplayGeometryChanged(
    int display_rotation, int camera_to_display_rotation, int width,
    int height) {
  LOGI("OnSurfaceChanged(%d, %d)", width, height);
  glViewport(0, 0, width, height);
  display_rotation_ = display_rotation;
  camera_to_display_rotation_ = camera_to_display_rotation;
  width_ = width;
  height_ = height;
  aspect_ratio_ =
      GetViewportAspectRatio(camera_to_display_rotation_, width_, height_);
  if (ar_session_ != nullptr) {
    ArSession_setDisplayGeometry(ar_session_, display_rotation, width, height);
  }
}

void ComputerVisionApplication::OnDrawFrame(float split_position) {
  // Render the scene.
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

  if (ar_session_ == nullptr) return;

  ArSession_setCameraTextureName(ar_session_,
                                 cpu_image_renderer_.GetTextureId());

  // Update session to get current frame and render camera background and cpu
  // image.
  if (ArSession_update(ar_session_, ar_frame_) != AR_SUCCESS) {
    LOGE("ComputerVisionApplication::OnDrawFrame ArSession_update error");
  }

  // Lock the image use to avoid pausing & resuming session when the image is in
  // use. This is because switching resolutions requires all images to be
  // released before session.resume() is called.
  std::lock_guard<std::mutex> lock(frame_image_in_use_mutex_);

  ArImage* image = nullptr;
  ArStatus status = ArFrame_acquireCameraImage(ar_session_, ar_frame_, &image);
  if (status != AR_SUCCESS) {
    LOGW(
        "ComputerVisionApplication::OnDrawFrame acquire camera image not "
        "ready.");
  }

  cpu_image_renderer_.Draw(ar_session_, ar_frame_, image, aspect_ratio_,
                           camera_to_display_rotation_, split_position);
  ArImage_release(image);
}

std::string ComputerVisionApplication::getCameraConfigLabel(
    bool is_low_resolution) {
  if (is_low_resolution && cpu_low_resolution_camera_config_ptr_ != nullptr) {
    return "Low Resolution" +
           cpu_low_resolution_camera_config_ptr_->config_label;
  } else if (!is_low_resolution &&
             cpu_high_resolution_camera_config_ptr_ != nullptr) {
    return "High Resolution" +
           cpu_high_resolution_camera_config_ptr_->config_label;
  } else {
    return "";
  }
}

ArStatus ComputerVisionApplication::setCameraConfig(bool is_low_resolution) {
  // To change the AR camera config - first we pause the AR session, set the
  // desired camera config and then resume the AR session.
  CHECK(ar_session_)

  // Block here if the image is still being used.
  std::lock_guard<std::mutex> lock(frame_image_in_use_mutex_);

  ArSession_pause(ar_session_);

  if (is_low_resolution) {
    ArSession_setCameraConfig(ar_session_,
                              cpu_low_resolution_camera_config_ptr_->config);
  } else {
    ArSession_setCameraConfig(ar_session_,
                              cpu_high_resolution_camera_config_ptr_->config);
  }

  ArStatus status = ArSession_resume(ar_session_);
  if (status != ArStatus::AR_SUCCESS) {
    // In a rare case (such as another camera app launching) the camera may be
    // given to a different app and so may not be available to this app. Handle
    // this properly and recreate the session at the next iteration.
    ArSession_destroy(ar_session_);
    ArConfig_destroy(ar_config_);
    ArFrame_destroy(ar_frame_);
  }

  return status;
}

void ComputerVisionApplication::SetFocusMode(bool enable_auto_focus) {
  CHECK(ar_session_);
  CHECK(ar_config_);

  ArConfig_setFocusMode(
      ar_session_, ar_config_,
      enable_auto_focus ? AR_FOCUS_MODE_AUTO : AR_FOCUS_MODE_FIXED);

  CHECK(ArSession_configure(ar_session_, ar_config_) == AR_SUCCESS);
}

bool ComputerVisionApplication::GetFocusMode() {
  CHECK(ar_session_);
  CHECK(ar_config_);

  ArFocusMode focus_mode;
  ArConfig_getFocusMode(ar_session_, ar_config_, &focus_mode);

  return (focus_mode == AR_FOCUS_MODE_AUTO);
}

void ComputerVisionApplication::obtainCameraConfigs() {
  // Retrieve supported camera configs.
  ArCameraConfigList* all_camera_configs = nullptr;
  int32_t num_configs = 0;
  ArCameraConfigList_create(ar_session_, &all_camera_configs);
  // Create filter first to get both 30 and 60 fps.
  ArCameraConfigFilter* camera_config_filter = nullptr;
  ArCameraConfigFilter_create(ar_session_, &camera_config_filter);
  ArCameraConfigFilter_setTargetFps(
      ar_session_, camera_config_filter,
      AR_CAMERA_CONFIG_TARGET_FPS_30 | AR_CAMERA_CONFIG_TARGET_FPS_60);
  ArSession_getSupportedCameraConfigsWithFilter(
      ar_session_, camera_config_filter, all_camera_configs);
  ArCameraConfigList_getSize(ar_session_, all_camera_configs, &num_configs);

  if (num_configs < 1) {
    LOGE("No camera config found");
    return;
  }

  camera_configs_.resize(num_configs);
  for (int i = 0; i < num_configs; ++i) {
    copyCameraConfig(ar_session_, all_camera_configs, i, num_configs,
                     &camera_configs_[i]);
  }

  // Determine the highest and lowest CPU resolutions.
  cpu_low_resolution_camera_config_ptr_ = nullptr;
  cpu_high_resolution_camera_config_ptr_ = nullptr;
  getCameraConfigLowestAndHighestResolutions(
      &cpu_low_resolution_camera_config_ptr_,
      &cpu_high_resolution_camera_config_ptr_);

  // Cleanup the list obtained as it is safe to destroy the list as camera
  // config instances were explicitly created and copied. Refer to the
  // previous comment.
  ArCameraConfigList_destroy(all_camera_configs);
}

void ComputerVisionApplication::getCameraConfigLowestAndHighestResolutions(
    CameraConfig** lowest_resolution_config,
    CameraConfig** highest_resolution_config) {
  if (camera_configs_.empty()) {
    return;
  }

  int low_resolution_config_idx = 0;
  int high_resolution_config_idx = 0;
  int32_t smallest_height = camera_configs_[0].height;
  int32_t largest_height = camera_configs_[0].height;

  for (int i = 1; i < camera_configs_.size(); ++i) {
    int32_t image_height = camera_configs_[i].height;
    if (image_height < smallest_height) {
      smallest_height = image_height;
      low_resolution_config_idx = i;
    } else if (image_height > largest_height) {
      largest_height = image_height;
      high_resolution_config_idx = i;
    }
  }

  if (low_resolution_config_idx == high_resolution_config_idx) {
    *lowest_resolution_config = &camera_configs_[low_resolution_config_idx];
  } else {
    *lowest_resolution_config = &camera_configs_[low_resolution_config_idx];
    *highest_resolution_config = &camera_configs_[high_resolution_config_idx];
  }
}

void ComputerVisionApplication::copyCameraConfig(
    const ArSession* ar_session, const ArCameraConfigList* all_configs,
    int index, int num_configs, CameraConfig* camera_config) {
  if (camera_config != nullptr && index >= 0 && index < num_configs) {
    ArCameraConfig_create(ar_session, &camera_config->config);
    ArCameraConfigList_getItem(ar_session, all_configs, index,
                               camera_config->config);
    ArCameraConfig_getImageDimensions(ar_session, camera_config->config,
                                      &camera_config->width,
                                      &camera_config->height);
    camera_config->config_label = "(" + std::to_string(camera_config->width) +
                                  "x" + std::to_string(camera_config->height) +
                                  ")";
  }
}

void ComputerVisionApplication::destroyCameraConfigs() {
  for (int i = 0; i < camera_configs_.size(); ++i) {
    if (camera_configs_[i].config != nullptr) {
      ArCameraConfig_destroy(camera_configs_[i].config);
    }
  }
}

std::string ComputerVisionApplication::GetCameraIntrinsicsText(
    bool for_gpu_texture) {
  if (ar_session_ == nullptr) return "";

  ArCamera* ar_camera;
  ArFrame_acquireCamera(ar_session_, ar_frame_, &ar_camera);
  if (for_gpu_texture) {
    ArCamera_getTextureIntrinsics(ar_session_, ar_camera,
                                  ar_camera_intrinsics_);
  } else {
    ArCamera_getImageIntrinsics(ar_session_, ar_camera, ar_camera_intrinsics_);
  }

  float fx;
  float fy;
  float cx;
  float cy;
  int image_width;
  int image_height;

  ArCameraIntrinsics_getFocalLength(ar_session_, ar_camera_intrinsics_, &fx,
                                    &fy);
  ArCameraIntrinsics_getPrincipalPoint(ar_session_, ar_camera_intrinsics_, &cx,
                                       &cy);
  ArCameraIntrinsics_getImageDimensions(ar_session_, ar_camera_intrinsics_,
                                        &image_width, &image_height);

  ArCamera_release(ar_camera);

  float fov_x = 2.0f * std::atan2(image_width, 2.0f * fx);
  float fov_y = 2.0f * std::atan2(image_height, 2.0f * fy);
  fov_x *= kRadiansToDegrees;
  fov_y *= kRadiansToDegrees;

  std::ostringstream intrinsics_text;
  intrinsics_text << std::fixed << std::setprecision(2) << "Unrotated Camera "
                  << (for_gpu_texture ? "GPU Texture" : "CPU Image")
                  << " Intrinsics:\n\tFocal Length: (" << fx << ", " << fy
                  << ")\n\tPrincipal Point: (" << cx << ", " << cy << ")\n\t"
                  << (for_gpu_texture ? "GPU" : "CPU") << " Image Dimensions: ("
                  << image_width << ", " << image_height
                  << ")\n\tUnrotated Field of View: (" << fov_x << "º, "
                  << fov_y << "º)";

  return intrinsics_text.str();
}

}  // namespace computer_vision
