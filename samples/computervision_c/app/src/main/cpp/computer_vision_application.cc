/*
 * Copyright 2018 Google Inc. All Rights Reserved.
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
#include <media/NdkImage.h>
#include <array>

#include "util.h"

namespace computer_vision {
namespace {

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

ComputerVisionApplication::~ComputerVisionApplication() {
  if (ar_session_ != nullptr) {
    ArSession_destroy(ar_session_);
    ArFrame_destroy(ar_frame_);
  }
}

void ComputerVisionApplication::OnPause() {
  LOGI("OnPause()");
  if (ar_session_ != nullptr) {
    ArSession_pause(ar_session_);
  }
}

void ComputerVisionApplication::OnResume(void* env, void* context,
                                         void* activity) {
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
    CHECK(ArCoreApk_requestInstall(env, activity, user_requested_install,
                                   &install_status) == AR_SUCCESS);

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
    CHECK(ArSession_create(env, context, &ar_session_) == AR_SUCCESS);
    CHECK(ar_session_);

    ArFrame_create(ar_session_, &ar_frame_);
    CHECK(ar_frame_);

    ArSession_setDisplayGeometry(ar_session_, display_rotation_, width_,
                                 height_);
  }

  const ArStatus status = ArSession_resume(ar_session_);
  CHECK(status == AR_SUCCESS);
}

void ComputerVisionApplication::OnSurfaceCreated() {
  LOGI("OnSurfaceCreated()");
  cpu_image_renderer_.InitializeGlContent();
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

  ArImage* ar_image;
  const AImage* ndk_image = nullptr;
  ArStatus status =
      ArFrame_acquireCameraImage(ar_session_, ar_frame_, &ar_image);
  if (status == AR_SUCCESS) {
    ArImage_getNdkImage(ar_image, &ndk_image);
    ArImage_release(ar_image);
  } else {
    LOGW(
        "ComputerVisionApplication::OnDrawFrame acquire camera image not "
        "ready.");
  }
  cpu_image_renderer_.Draw(ar_session_, ar_frame_, ndk_image, aspect_ratio_,
                           camera_to_display_rotation_, split_position);
}

}  // namespace computer_vision
