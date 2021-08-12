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

#ifndef C_ARCORE_COMPUTER_VISION_APPLICATION_H_
#define C_ARCORE_COMPUTER_VISION_APPLICATION_H_

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <android/asset_manager.h>
#include <jni.h>
#include <memory>
#include <mutex>  // NOLINT
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "arcore_c_api.h"
#include "cpu_image_renderer.h"
#include "util.h"

namespace computer_vision {

// ComputerVisionApplication handles all application logics.
class ComputerVisionApplication {
 public:
  // Constructor and deconstructor.
  explicit ComputerVisionApplication(AAssetManager* asset_manager);
  ~ComputerVisionApplication();

  // OnPause is called on the UI thread from the Activity's onPause method.
  void OnPause();

  // OnResume is called on the UI thread from the Activity's onResume method.
  void OnResume(JNIEnv* env, jobject context, jobject activity);

  // OnSurfaceCreated is called on the OpenGL thread when GLSurfaceView
  // is created.
  void OnSurfaceCreated();

  // OnDisplayGeometryChanged is called on the OpenGL thread when the
  // render surface size or display rotation changes.
  //
  // @param display_rotation: current display rotation.
  // @param width: width of the changed surface view.
  // @param height: height of the changed surface view.
  void OnDisplayGeometryChanged(int display_rotation,
                                int camera_to_display_rotation, int width,
                                int height);

  // OnDrawFrame is called on the OpenGL thread to render the next frame.
  void OnDrawFrame(float split_position);

  // Get the text label of a camera config.
  // Return an empty string if the camera config is not available.
  std::string getCameraConfigLabel(bool is_low_resolution);

  // Set camera config with low or high resolution.
  ArStatus setCameraConfig(bool is_low_resolution);

  void SetFocusMode(bool enable_auto_focus);
  bool GetFocusMode();

  // Get the text logs for the camera intrinsics.
  std::string GetCameraIntrinsicsText(bool for_gpu_texture);

 private:
  ArSession* ar_session_ = nullptr;
  ArConfig* ar_config_ = nullptr;
  ArFrame* ar_frame_ = nullptr;
  ArCameraIntrinsics* ar_camera_intrinsics_ = nullptr;

  bool install_requested_ = false;
  int width_ = 1;
  int height_ = 1;
  int display_rotation_ = 0;
  int camera_to_display_rotation_ = 0;
  float aspect_ratio_ = 0.0;

  AAssetManager* const asset_manager_;

  CpuImageRenderer cpu_image_renderer_;

  struct CameraConfig {
    int32_t width = 0;
    int32_t height = 0;
    std::string config_label;
    ArCameraConfig* config = nullptr;
  };

  // This lock prevents changing resolution as the frame is being rendered.
  // ARCore requires all cpu images to be released before changing resolution.
  std::mutex frame_image_in_use_mutex_;

  std::vector<CameraConfig> camera_configs_;
  CameraConfig* cpu_low_resolution_camera_config_ptr_ = nullptr;
  CameraConfig* cpu_high_resolution_camera_config_ptr_ = nullptr;

  // Obtain all camera configs (and update camera_configs_) and sort out the
  // configs with lowest and highest image resolutions.
  void obtainCameraConfigs();

  // Copy one camera config from ArCameraConfigList to a CameraConfig.
  void copyCameraConfig(const ArSession* ar_session,
                        const ArCameraConfigList* all_configs, int index,
                        int num_configs, CameraConfig* camera_config);

  // Release memory in camera_configs_.
  void destroyCameraConfigs();

  // Help function (called by obtainCameraConfigs()) to return pointers to
  // camera_configs_ the lowest and highest resolutions configs.
  void getCameraConfigLowestAndHighestResolutions(
      CameraConfig** lowest_resolution_config,
      CameraConfig** highest_resolution_config);
};
}  // namespace computer_vision

#endif  // C_ARCORE_COMPUTER_VISION_APPLICATION_H_
