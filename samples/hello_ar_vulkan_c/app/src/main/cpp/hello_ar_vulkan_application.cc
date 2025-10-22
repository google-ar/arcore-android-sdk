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

#include "hello_ar_vulkan_application.h"

#include <android/asset_manager.h>
#include <android/native_window_jni.h>

#include <array>
#include <cstdint>

#include "arcore_c_api.h"
#include "android_vulkan_loader.h"
#include "background_renderer.h"
#include "model_renderer.h"
#include "point_cloud_renderer.h"
#include "util.h"
#include "vulkan_handler.h"

namespace hello_ar_vulkan {
namespace {

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

void SetColor(float r, float g, float b, float a, glm::vec4& color) {
  color = glm::vec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}

}  // namespace

HelloArVulkanApplication::HelloArVulkanApplication(AAssetManager* asset_manager)
    : asset_manager_(asset_manager) {}

HelloArVulkanApplication::~HelloArVulkanApplication() {
  if (ar_session_ != nullptr) {
    ArSession_destroy(ar_session_);
    ArFrame_destroy(ar_frame_);
  }
  for (auto& anchor : anchors_) {
    ArAnchor_release(anchor.anchor);
    ArTrackable_release(anchor.trackable);
  }
}

void HelloArVulkanApplication::OnPause() {
  if (ar_session_ != nullptr) {
    ArSession_pause(ar_session_);
  }
}

void HelloArVulkanApplication::OnResume(JNIEnv* env, void* context,
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

void HelloArVulkanApplication::OnSurfaceCreated(JNIEnv* env,
                                               jobject surface_obj) {
  // Get the window handle and check if it is null.
  ANativeWindow* new_window = ANativeWindow_fromSurface(env, surface_obj);
  if (new_window == nullptr) {
    LOGE("Failed to get ANativeWindow from surface. Aborting.");
    return;
  }

  // This prevents the window from being destroyed while we initialize Vulkan.
  ANativeWindow_acquire(new_window);

  // Now, reset the unique_ptr. Its deleter will call ANativeWindow_release(),
  // correctly pairing with the acquire() call.
  window_.reset(new_window);

  vulkan_handler_ =
      std::make_unique<VulkanHandler>(window_.get(), MAX_FRAMES_IN_FLIGHT);
  background_renderer_ =
      std::make_unique<BackgroundRenderer>(vulkan_handler_.get());
  plane_renderer_ = std::make_unique<PlaneRenderer>(vulkan_handler_.get());
  depth_map_renderer_ =
      std::make_unique<DepthMapRenderer>(vulkan_handler_.get());
  if (ar_session_ != nullptr) {
    point_cloud_renderer_ = std::make_unique<PointCloudRenderer>(
        vulkan_handler_.get(), ar_session_);
  }
  model_renderer_ =
      std::make_unique<ModelRenderer>(vulkan_handler_.get(), asset_manager_);
}

void HelloArVulkanApplication::OnSurfaceDestroyed() {
  model_renderer_.reset();
  point_cloud_renderer_.reset();
  plane_renderer_.reset();
  background_renderer_.reset();
  depth_map_renderer_.reset();
  vulkan_handler_.reset();
  window_.reset();
}

void HelloArVulkanApplication::OnDisplayGeometryChanged(int display_rotation,
                                                       int width, int height) {
  display_rotation_ = display_rotation;
  width_ = width;
  height_ = height;
  if (ar_session_ != nullptr) {
    ArSession_setDisplayGeometry(ar_session_, display_rotation, width, height);
  }
}

void HelloArVulkanApplication::OnDrawFrame(
    bool depth_color_visualization_enabled, bool use_depth_for_occlusion) {
  if (ar_session_ == nullptr) return;

  if (point_cloud_renderer_ == nullptr) {
    point_cloud_renderer_ = std::make_unique<PointCloudRenderer>(
        vulkan_handler_.get(), ar_session_);
  }

  if (ArSession_update(ar_session_, ar_frame_) != AR_SUCCESS) {
    LOGE("HelloArVulkanApplication::OnDrawFrame ArSession_update error");
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

  texture_transform_matrix_ = GetTextureTransformMatrix(ar_session_, ar_frame_);
  if (frames_to_set_vertices > 0) {
    // These vertices represent 4 corners of the screen. One tuple is one
    // vertex. The first two floats in the tuple represent the screen
    // coordinates in vulkan (Details: http://vulkano.rs/guide/vertex-input).
    // The later two represent the texture coordinates which is fetched from
    // ARCore `ArFrame_transformCoordinates2d`.
    const std::vector<util::VertexInfo> vertices = {
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
  ArCamera* ar_camera;
  ArFrame_acquireCamera(ar_session_, ar_frame_, &ar_camera);

  glm::mat4 view_matrix;
  ArCamera_getViewMatrix(ar_session_, ar_camera, glm::value_ptr(view_matrix));
  glm::mat4 projection_matrix;
  ArCamera_getProjectionMatrix(ar_session_, ar_camera,
                               /*near=*/0.1f, /*far=*/100.0f,
                               glm::value_ptr(projection_matrix));
  projection_matrix[1][1] *= -1.0f;  // Vulkan uses an inverted y-axis.

  vulkan_handler_->BeginRecordingCommandBuffer(current_frame_);
  vulkan_handler_->BeginRenderPass(current_frame_, next_swapchain_image_index);

  vulkan_handler_->UpdateDepthTexture(ar_session_, ar_frame_, current_frame_);

  if (depth_color_visualization_enabled) {
    depth_map_renderer_->DrawDepthMap(current_frame_);
  } else {
    background_renderer_->DrawBackground(
        current_frame_,
        reinterpret_cast<AHardwareBuffer*>(native_hardware_buffer));
  }

  ArTrackingState camera_tracking_state;
  ArCamera_getTrackingState(ar_session_, ar_camera, &camera_tracking_state);

  if (camera_tracking_state == AR_TRACKING_STATE_TRACKING) {
    ArPointCloud* point_cloud;
    ArStatus status =
        ArFrame_acquirePointCloud(ar_session_, ar_frame_, &point_cloud);
    if (status == AR_SUCCESS) {
      point_cloud_renderer_->Draw(current_frame_, point_cloud, view_matrix,
                                  projection_matrix);
      ArPointCloud_release(point_cloud);  // Only release if status is success.
    }

    ArTrackableList* plane_list = nullptr;
    ArTrackableList_create(ar_session_, &plane_list);
    CHECK(plane_list != nullptr);
    ArSession_getAllTrackables(ar_session_, AR_TRACKABLE_PLANE, plane_list);
    plane_count_ = plane_renderer_->Draw(
        current_frame_, *ar_session_, plane_list, view_matrix,
        projection_matrix, texture_transform_matrix_, use_depth_for_occlusion);
    ArTrackableList_destroy(plane_list);
    plane_list = nullptr;

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

    // Render all the anchors.
    instance_data_.clear();
    glm::mat4 model_matrix(1.0f);
    for (auto& anchor : anchors_) {
      ArTrackingState tracking_state = AR_TRACKING_STATE_STOPPED;
      ArAnchor_getTrackingState(ar_session_, anchor.anchor, &tracking_state);
      if (tracking_state == AR_TRACKING_STATE_TRACKING) {
        // Only render the anchors that are tracking.
        util::GetTransformMatrixFromAnchor(*anchor.anchor, ar_session_,
                                           &model_matrix);
        util::InstanceData instance_data;
        instance_data.model_view = view_matrix * model_matrix;
        instance_data.color = anchor.color;
        instance_data_.push_back(instance_data);
      }
    }
    model_renderer_->DrawModel(current_frame_, instance_data_,
                               projection_matrix, texture_transform_matrix_,
                               use_depth_for_occlusion, color_correction);
  }

  // End render pass and submit the recording command buffer.
  vulkan_handler_->EndRenderPass(current_frame_);
  vulkan_handler_->FinishRecordingCommandBuffer(current_frame_);
  vulkan_handler_->SubmitRecordingCommandBuffer(current_frame_);
  vulkan_handler_->PresentRecordingCommandBuffer(current_frame_,
                                                 next_swapchain_image_index);

  ArCamera_release(ar_camera);

  current_frame_ = (current_frame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

bool HelloArVulkanApplication::IsDepthSupported() {
  int32_t is_supported = 0;
  ArSession_isDepthModeSupported(ar_session_, AR_DEPTH_MODE_AUTOMATIC,
                                 &is_supported);
  return is_supported;
}

void HelloArVulkanApplication::ConfigureSession() {
  ArConfig* ar_config = nullptr;
  ArConfig_create(ar_session_, &ar_config);
  CHECK(ar_config);

  if (IsDepthSupported()) {
    ArConfig_setDepthMode(ar_session_, ar_config, AR_DEPTH_MODE_AUTOMATIC);
  } else {
    ArConfig_setDepthMode(ar_session_, ar_config, AR_DEPTH_MODE_DISABLED);
  }

  if (is_instant_placement_enabled_) {
    ArConfig_setInstantPlacementMode(ar_session_, ar_config,
                                     AR_INSTANT_PLACEMENT_MODE_LOCAL_Y_UP);
  } else {
    ArConfig_setInstantPlacementMode(ar_session_, ar_config,
                                     AR_INSTANT_PLACEMENT_MODE_DISABLED);
  }

  ArConfig_setTextureUpdateMode(ar_session_, ar_config,
                                AR_TEXTURE_UPDATE_MODE_EXPOSE_HARDWARE_BUFFER);

  CHECK(ArSession_configure(ar_session_, ar_config) == AR_SUCCESS);
  ArConfig_destroy(ar_config);
}

void HelloArVulkanApplication::OnTouched(float x, float y) {
  if (ar_frame_ == nullptr || ar_session_ == nullptr) {
    return;
  }

  // --- Perform Hit Test ---
  ArHitResultList* hit_result_list = nullptr;
  ArHitResultList_create(ar_session_, &hit_result_list);
  CHECK(hit_result_list);

  if (is_instant_placement_enabled_) {
    ArFrame_hitTestInstantPlacement(ar_session_, ar_frame_, x, y,
                                    kApproximateDistanceMeters,
                                    hit_result_list);
  } else {
    ArFrame_hitTest(ar_session_, ar_frame_, x, y, hit_result_list);
  }
  int32_t hit_result_list_size = 0;
  ArHitResultList_getSize(ar_session_, hit_result_list, &hit_result_list_size);

  ArHitResult* ar_hit_result = nullptr;
  for (int32_t i = 0; i < hit_result_list_size; ++i) {
    ArHitResult* ar_hit = nullptr;
    ArHitResult_create(ar_session_, &ar_hit);

    if (ar_hit == nullptr) {
      LOGE("Failed to allocate ArHitResult for item %d, skipping.", i);
      continue;  // Skip this item due to allocation failure
    }

    ArHitResultList_getItem(ar_session_, hit_result_list, i, ar_hit);

    ArTrackable* ar_trackable = nullptr;
    ArHitResult_acquireTrackable(ar_session_, ar_hit, &ar_trackable);
    ArTrackableType ar_trackable_type = AR_TRACKABLE_NOT_VALID;
    ArTrackable_getType(ar_session_, ar_trackable, &ar_trackable_type);

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
          util::CalculateDistanceToPlane(*ar_session_, *hit_pose, *camera_pose);

      ArPose_destroy(hit_pose);
      ArPose_destroy(camera_pose);

      if (!in_polygon || normal_distance_to_plane < 0) {
        ArTrackable_release(ar_trackable);
        ArHitResult_destroy(ar_hit);
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
    } else if (AR_TRACKABLE_INSTANT_PLACEMENT_POINT == ar_trackable_type ||
               AR_TRACKABLE_DEPTH_POINT == ar_trackable_type) {
      ar_hit_result = ar_hit;
      break;
    }

    // If we reach here, the trackable wasn't suitable, so clean up and
    // continue.
    ArTrackable_release(ar_trackable);
    ArHitResult_destroy(ar_hit);
  }

  // This is the anchor creation logic.
  if (ar_hit_result) {
    ArAnchor* anchor = nullptr;
    if (ArHitResult_acquireNewAnchor(ar_session_, ar_hit_result, &anchor) !=
        AR_SUCCESS) {
      ArHitResult_destroy(ar_hit_result);
      ArHitResultList_destroy(hit_result_list);
      return;
    }

    if (anchors_.size() >= util::kMaxNumberOfAndroidsToRender) {
      ArAnchor_release(anchors_.front().anchor);
      ArTrackable_release(anchors_.front().trackable);
      anchors_.pop_front();
    }

    ArTrackable* ar_trackable = nullptr;
    ArHitResult_acquireTrackable(ar_session_, ar_hit_result, &ar_trackable);

    // Assign a color to the object based on its anchor's associated trackable
    // type. Blue: AR_TRACKABLE_POINT; Green: AR_TRACKABLE_PLANE
    ColoredAnchor colored_anchor;
    colored_anchor.anchor = anchor;
    colored_anchor.trackable = ar_trackable;

    UpdateAnchorColor(&colored_anchor);
    anchors_.push_back(colored_anchor);

    ArHitResult_destroy(ar_hit_result);
    ar_hit_result = nullptr;

    ArHitResultList_destroy(hit_result_list);
    hit_result_list = nullptr;
  }
}

void HelloArVulkanApplication::OnSettingsChange(
    bool is_instant_placement_enabled) {
  is_instant_placement_enabled_ = is_instant_placement_enabled;

  if (ar_session_ != nullptr) {
    ConfigureSession();
  }
}

// This method is used to update the color of the anchor.
// The color of the anchor is used to indicate the type of the anchor.
void HelloArVulkanApplication::UpdateAnchorColor(
    ColoredAnchor* colored_anchor) {
  ArTrackable* ar_trackable = colored_anchor->trackable;
  glm::vec4& color = colored_anchor->color;

  ArTrackableType ar_trackable_type;
  ArTrackable_getType(ar_session_, ar_trackable, &ar_trackable_type);

  if (ar_trackable_type == AR_TRACKABLE_POINT) {
    SetColor(66.0f, 133.0f, 244.0f, 255.0f, color);
    return;
  }

  if (ar_trackable_type == AR_TRACKABLE_PLANE) {
    SetColor(139.0f, 195.0f, 74.0f, 255.0f, color);
    return;
  }

  if (ar_trackable_type == AR_TRACKABLE_DEPTH_POINT) {
    SetColor(199.0f, 8.0f, 65.0f, 255.0f, color);
    return;
  }

  if (ar_trackable_type == AR_TRACKABLE_INSTANT_PLACEMENT_POINT) {
    ArInstantPlacementPoint* ar_instant_placement_point =
        ArAsInstantPlacementPoint(ar_trackable);
    ArInstantPlacementPointTrackingMethod tracking_method;
    ArInstantPlacementPoint_getTrackingMethod(
        ar_session_, ar_instant_placement_point, &tracking_method);
    if (tracking_method ==
        AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_FULL_TRACKING) {
      SetColor(255.0f, 255.0f, 137.0f, 255.0f, color);
      return;
    } else if (
        tracking_method ==
        AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_SCREENSPACE_WITH_APPROXIMATE_DISTANCE) {  // NOLINT
      SetColor(255.0f, 255.0f, 255.0f, 255.0f, color);
      return;
    }
  }

  // Fallback color
  SetColor(0.0f, 0.0f, 0.0f, 0.0f, color);
}

// This method returns a transformation matrix that when applied to screen space
// uvs makes them match correctly with the quad texture coords used to render
// the camera feed. It takes into account device orientation.
glm::mat3 HelloArVulkanApplication::GetTextureTransformMatrix(
    const ArSession* session, const ArFrame* frame) {
  float frameTransform[6];
  float uvTransform[9];
  // XY pairs of coordinates in NDC space that constitute the origin and points
  // along the two principal axes.
  const float ndcBasis[6] = {0, 0, 1, 0, 0, 1};
  ArFrame_transformCoordinates2d(
      session, frame, AR_COORDINATES_2D_VIEW_NORMALIZED, 3, ndcBasis,
      AR_COORDINATES_2D_TEXTURE_NORMALIZED, frameTransform);

  // Convert the transformed points into an affine transform and transpose it.
  float ndcOriginX = frameTransform[0];
  float ndcOriginY = frameTransform[1];
  uvTransform[0] = frameTransform[2] - ndcOriginX;
  uvTransform[1] = frameTransform[3] - ndcOriginY;
  uvTransform[2] = 0;
  uvTransform[3] = frameTransform[4] - ndcOriginX;
  uvTransform[4] = frameTransform[5] - ndcOriginY;
  uvTransform[5] = 0;
  uvTransform[6] = ndcOriginX;
  uvTransform[7] = ndcOriginY;
  uvTransform[8] = 1;

  return glm::make_mat3(uvTransform);
}

}  // namespace hello_ar_vulkan
