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

#ifndef C_ARCORE_COMPUTER_VISION_APPLICATION_H_
#define C_ARCORE_COMPUTER_VISION_APPLICATION_H_

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <jni.h>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>

#include "arcore_c_api.h"
#include "cpu_image_renderer.h"
#include "util.h"

namespace computer_vision {

// ComputerVisionApplication handles all application logics.
class ComputerVisionApplication {
 public:
  // Constructor and deconstructor.
  ~ComputerVisionApplication();

  // OnPause is called on the UI thread from the Activity's onPause method.
  void OnPause();

  // OnResume is called on the UI thread from the Activity's onResume method.
  void OnResume(void* env, void* context, void* activity);

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

 private:
  ArSession* ar_session_ = nullptr;
  ArFrame* ar_frame_ = nullptr;

  bool install_requested_ = false;
  int width_ = 1;
  int height_ = 1;
  int display_rotation_ = 0;
  int camera_to_display_rotation_ = 0;
  float aspect_ratio_ = 0.0;

  CpuImageRenderer cpu_image_renderer_;
};
}  // namespace computer_vision

#endif  // C_ARCORE_COMPUTER_VISION_APPLICATION_H_
