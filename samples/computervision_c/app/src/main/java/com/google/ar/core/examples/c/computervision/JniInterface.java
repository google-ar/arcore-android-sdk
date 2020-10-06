/*
 * Copyright 2019 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.google.ar.core.examples.c.computervision;

import android.app.Activity;
import android.content.Context;
import android.content.res.AssetManager;

/** JNI interface to native layer. */
public class JniInterface {
  static {
    System.loadLibrary("computer_vision_native");
  }

  /** Creates the native application to demo CPU Image access from ARCore. */
  static native long createNativeApplication(AssetManager assetManager);

  /**
   * Destroys the native application, clean up data and states.
   *
   * @param nativeApplication the native application handle.
   */
  static native void destroyNativeApplication(long nativeApplication);

  static native void onPause(long nativeApplication);

  static native void onResume(long nativeApplication, Context context, Activity activity);

  /**
   * Allocates OpenGL resources for rendering.
   *
   * @param nativeApplication the native application handle.
   */
  static native void onGlSurfaceCreated(long nativeApplication);

  /**
   * Pass display geometry change event into native application to handle GL view port change.
   *
   * <p>Called on the OpenGL thread before onGlSurfaceDrawFrame when the view port width, height, or
   * display rotation may have changed.
   *
   * @param nativeApplication the native application handle.
   * @param displayRotation the display's rotation.
   * @param cameraToDisplayRotation the camera's rotation relative to display.
   * @param width the window's width.
   * @param height the window's height.
   */
  static native void onDisplayGeometryChanged(
      long nativeApplication,
      int displayRotation,
      int cameraToDisplayRotation,
      int width,
      int height);

  /**
   * Main render loop, called on the OpenGL thread.
   *
   * @param nativeApplication the native application handle.
   * @param splitPosition the current screen split position in proportion to the whole screen.
   */
  static native void onGlSurfaceDrawFrame(long nativeApplication, float splitPosition);

  static native String getCameraConfigLabel(
      long nativeApplication, boolean isLowResolutionSelected);

  static native int setCameraConfig(long nativeApplication, boolean isLowResolutionSelected);

  /**
   * Retrieves the text for the intrinsic values of the current camera configuration.
   *
   * @param nativeApplication the native application handle.
   * @param forGpuTexture is the intrinsic text required for GPU texture or for CPU image.
   */
  static native String getCameraIntrinsicsText(long nativeApplication, boolean forGpuTexture);

  static native void setFocusMode(long nativeApplication, boolean isFixedFocus);

  static native boolean getFocusMode(long nativeApplication);
}
