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

#include "hello_ar_application.h"

#include <android/asset_manager.h>
#include <array>
#include <utility>

#include "plane_renderer.h"
#include "util.h"

namespace cloud_anchor {
namespace {
constexpr int32_t kPlaneColorRgbaSize = 16;

const glm::vec3 kWhite = {255, 255, 255};

constexpr std::array<uint32_t, kPlaneColorRgbaSize> kPlaneColorRgba = {
    {0xFFFFFFFF, 0xF44336FF, 0xE91E63FF, 0x9C27B0FF, 0x673AB7FF, 0x3F51B5FF,
     0x2196F3FF, 0x03A9F4FF, 0x00BCD4FF, 0x009688FF, 0x4CAF50FF, 0x8BC34AFF,
     0xCDDC39FF, 0xFFEB3BFF, 0xFFC107FF, 0xFF9800FF}};

inline glm::vec3 GetRandomPlaneColor() {
  const int32_t colorRgba = kPlaneColorRgba[std::rand() % kPlaneColorRgbaSize];
  return glm::vec3(((colorRgba >> 24) & 0xff) / 255.0f,
                   ((colorRgba >> 16) & 0xff) / 255.0f,
                   ((colorRgba >> 8) & 0xff) / 255.0f);
}
}  // namespace

HelloArApplication::HelloArApplication(
    AAssetManager* asset_manager,
    std::shared_ptr<CloudAnchorManager> cloud_anchor_manager)
    : cloud_anchor_manager_(std::move(cloud_anchor_manager)),
      asset_manager_(asset_manager) {
  LOGI("OnCreate()");
}

HelloArApplication::~HelloArApplication() {
  if (ar_session_ != nullptr) {
    ArSession_destroy(ar_session_);
    ArFrame_destroy(ar_frame_);
  }
}

void HelloArApplication::OnPause() {
  LOGI("OnPause()");
  if (ar_session_ != nullptr) {
    ArSession_pause(ar_session_);
  }
}

void HelloArApplication::OnResume(void* env, void* context, void* activity) {
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
    ArConfig_setCloudAnchorMode(
        ar_session_, ar_config,
        ArCloudAnchorMode::AR_CLOUD_ANCHOR_MODE_ENABLED);
    CHECK(ar_config);

    CHECK(ArSession_configure(ar_session_, ar_config) == AR_SUCCESS);
    ArConfig_destroy(ar_config);

    ArFrame_create(ar_session_, &ar_frame_);
    CHECK(ar_frame_);

    ArSession_setDisplayGeometry(ar_session_, display_rotation_, width_,
                                 height_);

    cloud_anchor_manager_->SetArSession(ar_session_);
  }

  const ArStatus status = ArSession_resume(ar_session_);
  CHECK(status == AR_SUCCESS);
}

void HelloArApplication::OnSurfaceCreated() {
  LOGI("OnSurfaceCreated()");

  background_renderer_.InitializeGlContent();
  point_cloud_renderer_.InitializeGlContent();
  andy_renderer_.InitializeGlContent(asset_manager_, "models/andy.obj",
                                     "models/andy.png");
  plane_renderer_.InitializeGlContent(asset_manager_);
}

void HelloArApplication::OnDisplayGeometryChanged(int display_rotation,
                                                  int width, int height) {
  LOGI("OnSurfaceChanged(%d, %d)", width, height);
  glViewport(0, 0, width, height);
  display_rotation_ = display_rotation;
  width_ = width;
  height_ = height;
  if (ar_session_ != nullptr) {
    ArSession_setDisplayGeometry(ar_session_, display_rotation, width, height);
  }
}

void HelloArApplication::OnDrawFrame() {
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
    LOGE("HelloArApplication::OnDrawFrame ArSession_update error");
  }

  ArCamera* ar_camera;
  ArFrame_acquireCamera(ar_session_, ar_frame_, &ar_camera);

  cloud_anchor_manager_->OnUpdate(ar_frame_);

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

  // Render Andy objects.
  glm::mat4 model_mat(1.0f);
  const ArAnchor* cloud_anchor = cloud_anchor_manager_->GetCloudAnchor();
  if (cloud_anchor != nullptr) {
    ArTrackingState tracking_state;
    ArAnchor_getTrackingState(ar_session_, cloud_anchor, &tracking_state);
    if (tracking_state == AR_TRACKING_STATE_TRACKING) {
      // Render object only if the tracking state is AR_TRACKING_STATE_TRACKING.
      util::GetTransformMatrixFromAnchor(ar_session_, cloud_anchor, &model_mat);
      andy_renderer_.Draw(projection_mat, view_mat, model_mat,
                          color_correction);
    }
  }

  // Update and render planes.
  ArTrackableList* plane_list = nullptr;
  ArTrackableList_create(ar_session_, &plane_list);
  CHECK(plane_list != nullptr);

  ArTrackableType plane_tracked_type = AR_TRACKABLE_PLANE;
  ArSession_getAllTrackables(ar_session_, plane_tracked_type, plane_list);

  int32_t plane_list_size = 0;
  ArTrackableList_getSize(ar_session_, plane_list, &plane_list_size);
  plane_count_ = plane_list_size;

  for (int i = 0; i < plane_list_size; ++i) {
    ArTrackable* ar_trackable = nullptr;
    ArTrackableList_acquireItem(ar_session_, plane_list, i, &ar_trackable);
    ArPlane* ar_plane = ArAsPlane(ar_trackable);
    ArTrackingState out_tracking_state;
    ArTrackable_getTrackingState(ar_session_, ar_trackable,
                                 &out_tracking_state);

    ArPlane* subsume_plane;
    ArPlane_acquireSubsumedBy(ar_session_, ar_plane, &subsume_plane);
    if (subsume_plane != nullptr) {
      ArTrackable_release(ArAsTrackable(subsume_plane));
      continue;
    }

    if (ArTrackingState::AR_TRACKING_STATE_TRACKING != out_tracking_state) {
      continue;
    }

    ArTrackingState plane_tracking_state;
    ArTrackable_getTrackingState(ar_session_, ArAsTrackable(ar_plane),
                                 &plane_tracking_state);
    if (plane_tracking_state == AR_TRACKING_STATE_TRACKING) {
      const auto iter = plane_color_map_.find(ar_plane);
      glm::vec3 color;
      if (iter != plane_color_map_.end()) {
        color = iter->second;

        // If this is an already observed trackable release it so it doesn't
        // leave an additional reference dangling.
        ArTrackable_release(ar_trackable);
      } else {
        // The first plane is always white.
        if (!first_plane_has_been_found_) {
          first_plane_has_been_found_ = true;
          color = kWhite;
        } else {
          color = GetRandomPlaneColor();
        }
        plane_color_map_.insert({ar_plane, color});
      }

      plane_renderer_.Draw(projection_mat, view_mat, ar_session_, ar_plane,
                           color);
    }
  }

  ArTrackableList_destroy(plane_list);
  plane_list = nullptr;

  // Update and render point cloud.
  ArPointCloud* ar_point_cloud = nullptr;
  ArStatus point_cloud_status =
      ArFrame_acquirePointCloud(ar_session_, ar_frame_, &ar_point_cloud);
  if (point_cloud_status == AR_SUCCESS) {
    point_cloud_renderer_.Draw(projection_mat * view_mat, ar_session_,
                               ar_point_cloud);
    ArPointCloud_release(ar_point_cloud);
  }
}

void HelloArApplication::OnTouched(float x, float y) {
  if (ar_frame_ == nullptr || ar_session_ == nullptr) {
    return;
  }

  // New Androids are only placed when in the cloud anchor hosting mode.
  if (!cloud_anchor_manager_->AllowHostingAnchor()) {
    return;
  }

  ArHitResultList* hit_result_list = nullptr;
  ArHitResultList_create(ar_session_, &hit_result_list);
  CHECK(hit_result_list);
  ArFrame_hitTest(ar_session_, ar_frame_, x, y, hit_result_list);

  int32_t hit_result_list_size = 0;
  ArHitResultList_getSize(ar_session_, hit_result_list, &hit_result_list_size);

  // The hitTest method sorts the resulting list by distance from the camera,
  // increasing.  The first hit result will usually be the most relevant when
  // responding to user input.

  ArHitResult* ar_hit_result = nullptr;
  for (int32_t i = 0; i < hit_result_list_size; ++i) {
    ArHitResult* ar_hit = nullptr;
    ArHitResult_create(ar_session_, &ar_hit);
    ArHitResultList_getItem(ar_session_, hit_result_list, i, ar_hit);


    ArTrackable* ar_trackable = nullptr;
    ArHitResult_acquireTrackable(ar_session_, ar_hit, &ar_trackable);
    ArTrackableType ar_trackable_type = AR_TRACKABLE_NOT_VALID;
    ArTrackable_getType(ar_session_, ar_trackable, &ar_trackable_type);
    // Creates an anchor if a plane or an oriented point was hit.
    if (AR_TRACKABLE_PLANE == ar_trackable_type) {
      ArPose* hit_pose = nullptr;
      ArPose_create(ar_session_, nullptr, &hit_pose);
      ArHitResult_getHitPose(ar_session_, ar_hit, hit_pose);
      int32_t in_polygon = 0;
      ArPlane* ar_plane = ArAsPlane(ar_trackable);
      ArPlane_isPoseInPolygon(ar_session_, ar_plane, hit_pose, &in_polygon);

      // Use hit pose and camera pose to check if hittest is from the
      // back of the plane, if it is, no need to create the anchor.
      ArPose* camera_pose = nullptr;
      ArPose_create(ar_session_, nullptr, &camera_pose);
      ArCamera* ar_camera;
      ArFrame_acquireCamera(ar_session_, ar_frame_, &ar_camera);
      ArCamera_getPose(ar_session_, ar_camera, camera_pose);
      ArCamera_release(ar_camera);
      float normal_distance_to_plane =
          util::CalculateDistanceToPlane(ar_session_, hit_pose, *camera_pose);

      ArPose_destroy(hit_pose);
      ArPose_destroy(camera_pose);

      if (!in_polygon || normal_distance_to_plane < 0) {
        continue;
      }

      ar_hit_result = ar_hit;
      break;
    } else if (AR_TRACKABLE_POINT == ar_trackable_type) {
      ArPoint* ar_point = ArAsPoint(ar_trackable);
      ArPointOrientationMode mode;
      ArPoint_getOrientationMode(ar_session_, ar_point, &mode);
      if (AR_POINT_ORIENTATION_ESTIMATED_SURFACE_NORMAL == mode) {
        ar_hit_result = ar_hit;
        break;
      }
    }
  }

  if (!ar_hit_result) {
    return;
  }

  // Note that the application is responsible for releasing the anchor
  // pointer after using it. Call ArAnchor_release(anchor) to release.
  ArAnchor* anchor = nullptr;
  if (ArHitResult_acquireNewAnchor(ar_session_, ar_hit_result, &anchor) !=
      AR_SUCCESS) {
    LOGE("HelloArApplication::OnTouched ArHitResult_acquireNewAnchor error");
    return;
  }

  // Register with the cloud anchor manager.
  cloud_anchor_manager_->RegisterLocalAnchorAsCloudAnchor(anchor);

  ArTrackingState tracking_state = AR_TRACKING_STATE_STOPPED;
  ArAnchor_getTrackingState(ar_session_, anchor, &tracking_state);
  ArAnchor_release(anchor);

  if (tracking_state != AR_TRACKING_STATE_TRACKING) {
    return;
  }

  ArHitResult_destroy(ar_hit_result);
  ar_hit_result = nullptr;

  ArHitResultList_destroy(hit_result_list);
  hit_result_list = nullptr;
}

}  // namespace cloud_anchor
