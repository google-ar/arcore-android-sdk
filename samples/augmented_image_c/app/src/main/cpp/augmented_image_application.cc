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

#include "augmented_image_application.h"

#include <android/asset_manager.h>
#include <array>
#include <cstdint>
#include <utility>

#include "obj_renderer.h"
#include "util.h"

namespace augmented_image {
namespace {
constexpr int32_t kTintColorRgbaSize = 16;

constexpr std::array<uint32_t, kTintColorRgbaSize> kTintColorRgba = {
    {0x000000FF, 0xF44336FF, 0xE91E63FF, 0x9C27B0FF, 0x673AB7FF, 0x3F51B5FF,
     0x2196F3FF, 0x03A9F4FF, 0x00BCD4FF, 0x009688FF, 0x4CAF50FF, 0x8BC34AFF,
     0xCDDC39FF, 0xFFEB3BFF, 0xFFC107FF, 0xFF9800FF}};

constexpr float kTintAlpha = 1.0f;
constexpr float kTintIntensity = 0.1f;

// AugmentedImage configuration and rendering.
// Load a single image (true) or a pre-generated image database (false).
constexpr bool kUseSingleImage = false;

}  // namespace

AugmentedImageApplication::AugmentedImageApplication(
    AAssetManager* asset_manager)
    : asset_manager_(asset_manager) {
  LOGI("OnCreate()");
}

AugmentedImageApplication::~AugmentedImageApplication() {
  if (ar_session_ != nullptr) {
    ArSession_destroy(ar_session_);
    ArFrame_destroy(ar_frame_);
  }
}

void AugmentedImageApplication::OnPause() {
  LOGI("OnPause()");
  if (ar_session_ != nullptr) {
    ArSession_pause(ar_session_);
  }
}

void AugmentedImageApplication::OnResume(void* env, void* context,
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
    // HelloAR Java sample code for reasonable behavior.
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
    // HelloAR Java sample code for reasonable behavior.
    CHECK(ArSession_create(env, context, &ar_session_) == AR_SUCCESS);
    CHECK(ar_session_);

    ArConfig* ar_config = nullptr;
    ArConfig_create(ar_session_, &ar_config);
    CHECK(ar_config);

    ArAugmentedImageDatabase* ar_augmented_image_database =
        CreateAugmentedImageDatabase();
    ArConfig_setAugmentedImageDatabase(ar_session_, ar_config,
                                       ar_augmented_image_database);

    CHECK(ArSession_configure(ar_session_, ar_config) == AR_SUCCESS);

    ArAugmentedImageDatabase_destroy(ar_augmented_image_database);
    ArConfig_destroy(ar_config);

    ArFrame_create(ar_session_, &ar_frame_);
    CHECK(ar_frame_);

    ArSession_setDisplayGeometry(ar_session_, display_rotation_, width_,
                                 height_);
  }

  const ArStatus status = ArSession_resume(ar_session_);
  CHECK(status == AR_SUCCESS);
}

ArAugmentedImageDatabase*
AugmentedImageApplication::CreateAugmentedImageDatabase() const {
  ArAugmentedImageDatabase* ar_augmented_image_database = nullptr;
  // There are two ways to configure a ArAugmentedImageDatabase:
  // 1. Add Bitmap to DB directly
  // 2. Load a pre-built AugmentedImageDatabase
  // Option 2) has
  // * shorter setup time
  // * doesn't require images to be packaged in apk.
  if (kUseSingleImage) {
    ArAugmentedImageDatabase_create(ar_session_, &ar_augmented_image_database);

    int32_t width, height, stride, index;
    uint8_t* image_pixel_buffer;
    constexpr const char kSampleImageName[] = "default.jpg";
    bool load_image_result = util::LoadImageFromAssetManager(
        kSampleImageName, &width, &height, &stride, &image_pixel_buffer);
    CHECK(load_image_result);

    uint8_t* grayscale_buffer;
    util::ConvertRgbaToGrayscale(image_pixel_buffer, width, height, stride,
                                 &grayscale_buffer);

    int32_t grayscale_stride = stride / 4;
    const ArStatus status = ArAugmentedImageDatabase_addImage(
        ar_session_, ar_augmented_image_database, kSampleImageName,
        grayscale_buffer, width, height, grayscale_stride, &index);
    CHECK(status == AR_SUCCESS);
    // If the physical size of the image is known, you can instead use
    //     ArStatus ArAugmentedImageDatabase_addImageWithPhysicalSize
    // This will improve the initial detection speed. ARCore will still actively
    // estimate the physical size of the image as it is viewed from multiple
    // viewpoints.

    delete[] image_pixel_buffer;
    delete[] grayscale_buffer;
  } else {
    std::string database_buffer;
    util::LoadEntireAssetFile(asset_manager_, "sample_database.imgdb",
                              &database_buffer);

    uint8_t* raw_buffer = reinterpret_cast<uint8_t*>(&database_buffer.front());
    const ArStatus status = ArAugmentedImageDatabase_deserialize(
        ar_session_, raw_buffer, database_buffer.size(),
        &ar_augmented_image_database);
    CHECK(status == AR_SUCCESS);
  }

  return ar_augmented_image_database;
}

void AugmentedImageApplication::OnSurfaceCreated() {
  LOGI("OnSurfaceCreated()");

  background_renderer_.InitializeGlContent();
  image_renderer_.InitializeGlContent(asset_manager_);
}

void AugmentedImageApplication::OnDisplayGeometryChanged(int display_rotation,
                                                         int width,
                                                         int height) {
  LOGI("OnSurfaceChanged(%d, %d)", width, height);
  glViewport(0, 0, width, height);
  display_rotation_ = display_rotation;
  width_ = width;
  height_ = height;
  if (ar_session_ != nullptr) {
    ArSession_setDisplayGeometry(ar_session_, display_rotation, width, height);
  }
}

void AugmentedImageApplication::OnDrawFrame(void* activity) {
  // Render the scene.
  glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  if (ar_session_ == nullptr) return;

  ArSession_setCameraTextureName(ar_session_,
                                 background_renderer_.GetTextureId());

  // Update session to get current frame and render camera background.
  if (ArSession_update(ar_session_, ar_frame_) != AR_SUCCESS) {
    LOGE("AugmentedImageApplication::OnDrawFrame ArSession_update error");
  }

  ArCamera* ar_camera;
  ArFrame_acquireCamera(ar_session_, ar_frame_, &ar_camera);

  glm::mat4 view_mat;
  glm::mat4 projection_mat;
  ArCamera_getViewMatrix(ar_session_, ar_camera, glm::value_ptr(view_mat));
  ArCamera_getProjectionMatrix(ar_session_, ar_camera,
                               /*near=*/0.1f, /*far=*/100.f,
                               glm::value_ptr(projection_mat));

  ArTrackingState camera_tracking_state;
  ArCamera_getTrackingState(ar_session_, ar_camera, &camera_tracking_state);
  ArCamera_release(ar_camera);

  background_renderer_.Draw(ar_session_, ar_frame_);

  // If the camera isn't tracking don't bother rendering other objects.
  if (camera_tracking_state != AR_TRACKING_STATE_TRACKING) {
    return;
  }

  // Get light estimation value.
  ArLightEstimate* ar_light_estimate;
  ArLightEstimateState ar_light_estimate_state;
  ArLightEstimate_create(ar_session_, &ar_light_estimate);

  ArFrame_getLightEstimate(ar_session_, ar_frame_, ar_light_estimate);
  ArLightEstimate_getState(ar_session_, ar_light_estimate,
                           &ar_light_estimate_state);

  // Set light intensity to default. Intensity value ranges from 0.0f to 1.0f.
  // The first three components are color scaling factors.
  // The last one is the average pixel intensity in gamma space.
  float color_correction[4] = {1.f, 1.f, 1.f, 1.f};
  if (ar_light_estimate_state == AR_LIGHT_ESTIMATE_STATE_VALID) {
    ArLightEstimate_getColorCorrection(ar_session_, ar_light_estimate,
                                       color_correction);
  }

  ArLightEstimate_destroy(ar_light_estimate);
  ar_light_estimate = nullptr;

  bool found_ar_image =
      DrawAugmentedImage(view_mat, projection_mat, color_correction);

  // Once we found the first image, hide the scan animation
  if (found_ar_image) {
    util::HideFitToScanImage(activity);
  }
}

bool AugmentedImageApplication::DrawAugmentedImage(
    const glm::mat4& view_mat, const glm::mat4& projection_mat,
    const float* color_correction) {
  bool found_ar_image = false;

  ArTrackableList* updated_image_list = nullptr;
  ArTrackableList_create(ar_session_, &updated_image_list);
  CHECK(updated_image_list != nullptr);
  ArFrame_getUpdatedTrackables(
      ar_session_, ar_frame_, AR_TRACKABLE_AUGMENTED_IMAGE, updated_image_list);

  int32_t image_list_size;
  ArTrackableList_getSize(ar_session_, updated_image_list, &image_list_size);

  // Find newly detected image, add it to map
  for (int i = 0; i < image_list_size; ++i) {
    ArTrackable* ar_trackable = nullptr;
    ArTrackableList_acquireItem(ar_session_, updated_image_list, i,
                                &ar_trackable);
    ArAugmentedImage* image = ArAsAugmentedImage(ar_trackable);

    ArTrackingState tracking_state;
    ArTrackable_getTrackingState(ar_session_, ar_trackable, &tracking_state);

    int image_index;
    ArAugmentedImage_getIndex(ar_session_, image, &image_index);

    switch (tracking_state) {
      case AR_TRACKING_STATE_PAUSED:
        // When an image is in PAUSED state but the camera is not PAUSED,
        // that means the image has been detected but not yet tracked.
        LOGI("Detected Image %d", image_index);
        break;
      case AR_TRACKING_STATE_TRACKING:
        found_ar_image = true;

        if (augmented_image_map.find(image_index) ==
            augmented_image_map.end()) {
          // Record the image and its anchor.
          util::ScopedArPose scopedArPose(ar_session_);
          ArAugmentedImage_getCenterPose(ar_session_, image,
                                         scopedArPose.GetArPose());

          ArAnchor* image_anchor = nullptr;
          const ArStatus status = ArTrackable_acquireNewAnchor(
              ar_session_, ar_trackable, scopedArPose.GetArPose(),
              &image_anchor);
          CHECK(status == AR_SUCCESS);

          // Now we have an Anchor, record this image.
          augmented_image_map[image_index] =
              std::pair<ArAugmentedImage*, ArAnchor*>(image, image_anchor);
        }
        break;

      case AR_TRACKING_STATE_STOPPED: {
        std::pair<ArAugmentedImage*, ArAnchor*> record =
            augmented_image_map[image_index];
        ArTrackable_release(ArAsTrackable(record.first));
        ArAnchor_release(record.second);
        augmented_image_map.erase(image_index);
      } break;

      default:
        break;
    }  // End of switch (tracking_state)
  }    // End of for (int i = 0; i < image_list_size; ++i) {

  ArTrackableList_destroy(updated_image_list);
  updated_image_list = nullptr;

  // Display all augmented images in augmented_image_map.
  for (const auto& it : augmented_image_map) {
    const std::pair<ArAugmentedImage*, ArAnchor*>& record = it.second;
    ArAugmentedImage* ar_image = record.first;
    ArAnchor* ar_anchor = record.second;
    ArTrackingState tracking_state;
    ArTrackable_getTrackingState(ar_session_, ArAsTrackable(ar_image),
                                 &tracking_state);

    // Draw this image frame.
    if (tracking_state == AR_TRACKING_STATE_TRACKING) {
      // Use Index to get tint color.
      int index;
      ArAugmentedImage_getIndex(ar_session_, ar_image, &index);
      int tint_index = index % kTintColorRgba.size();
      uint32_t tint_color_hex = kTintColorRgba[tint_index];
      float tint_color_rgba[4] = {
          ((tint_color_hex & 0xFF000000) >> 24) / 255.0f * kTintIntensity,
          ((tint_color_hex & 0x00FF0000) >> 16) / 255.0f * kTintIntensity,
          ((tint_color_hex & 0x0000FF00) >> 8) / 255.0f * kTintIntensity,
          kTintAlpha};

      image_renderer_.Draw(projection_mat, view_mat, color_correction,
                           tint_color_rgba, ar_session_, ar_image, ar_anchor);
    }
  }

  return found_ar_image;
}

}  // namespace augmented_image
