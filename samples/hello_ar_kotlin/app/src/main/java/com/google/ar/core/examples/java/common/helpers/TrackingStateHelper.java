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
package com.google.ar.core.examples.java.common.helpers;

import android.app.Activity;
import android.view.WindowManager;
import com.google.ar.core.Camera;
import com.google.ar.core.TrackingFailureReason;
import com.google.ar.core.TrackingState;

/** Gets human readibly tracking failure reasons and suggested actions. */
public final class TrackingStateHelper {
  private static final String INSUFFICIENT_FEATURES_MESSAGE =
      "Can't find anything. Aim device at a surface with more texture or color.";
  private static final String EXCESSIVE_MOTION_MESSAGE = "Moving too fast. Slow down.";
  private static final String INSUFFICIENT_LIGHT_MESSAGE =
      "Too dark. Try moving to a well-lit area.";
  private static final String INSUFFICIENT_LIGHT_ANDROID_S_MESSAGE =
      "Too dark. Try moving to a well-lit area."
      + " Also, make sure the Block Camera is set to off in system settings.";
  private static final String BAD_STATE_MESSAGE =
      "Tracking lost due to bad internal state. Please try restarting the AR experience.";
  private static final String CAMERA_UNAVAILABLE_MESSAGE =
      "Another app is using the camera. Tap on this app or try closing the other one.";
  private static final int ANDROID_S_SDK_VERSION = 31;

  private final Activity activity;

  private TrackingState previousTrackingState;

  public TrackingStateHelper(Activity activity) {
    this.activity = activity;
  }

  /** Keep the screen unlocked while tracking, but allow it to lock when tracking stops. */
  public void updateKeepScreenOnFlag(TrackingState trackingState) {
    if (trackingState == previousTrackingState) {
      return;
    }

    previousTrackingState = trackingState;
    switch (trackingState) {
      case PAUSED:
      case STOPPED:
        activity.runOnUiThread(
            () -> activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON));
        break;
      case TRACKING:
        activity.runOnUiThread(
            () -> activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON));
        break;
    }
  }

  public static String getTrackingFailureReasonString(Camera camera) {
    TrackingFailureReason reason = camera.getTrackingFailureReason();
    switch (reason) {
      case NONE:
        return "";
      case BAD_STATE:
        return BAD_STATE_MESSAGE;
      case INSUFFICIENT_LIGHT:
        if (android.os.Build.VERSION.SDK_INT < ANDROID_S_SDK_VERSION) {
            return INSUFFICIENT_LIGHT_MESSAGE;
        } else {
            return INSUFFICIENT_LIGHT_ANDROID_S_MESSAGE;
        }
      case EXCESSIVE_MOTION:
        return EXCESSIVE_MOTION_MESSAGE;
      case INSUFFICIENT_FEATURES:
        return INSUFFICIENT_FEATURES_MESSAGE;
      case CAMERA_UNAVAILABLE:
        return CAMERA_UNAVAILABLE_MESSAGE;
    }
    return "Unknown tracking failure reason: " + reason;
  }
}
