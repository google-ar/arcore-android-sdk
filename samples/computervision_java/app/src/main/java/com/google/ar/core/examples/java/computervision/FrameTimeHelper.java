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
package com.google.ar.core.examples.java.computervision;

import androidx.annotation.NonNull;
import androidx.lifecycle.DefaultLifecycleObserver;
import androidx.lifecycle.LifecycleOwner;

/** Helper to measure frame-to-frame timing and frame rate. */
public class FrameTimeHelper implements DefaultLifecycleObserver {

  // Number of milliseconds in one second.
  private static final float MILLISECONDS_PER_SECOND = 1000f;

  // Rate by which smoothed frame rate should approach momentary frame rate.
  private static final float SMOOTHING_FACTOR = .03f;

  // System time of last frame, or zero if no time has been recorded.
  private long previousFrameTime;

  // Smoothed frame time, or zero if frame time has not yet been recorded.
  private float smoothedFrameTime;

  @Override
  public void onResume(@NonNull LifecycleOwner owner) {
    // Reset timing data during initialization and after app pause.
    previousFrameTime = 0;
    smoothedFrameTime = 0f;
  }

  /** Capture current frame timestamp and calculate smoothed frame-to-frame time. */
  public void nextFrame() {
    long now = System.currentTimeMillis();

    // Is nextFrame() being called for the first time?
    if (previousFrameTime == 0) {
      previousFrameTime = now;

      // Unable to calculate frame time based on single timestamp.
      smoothedFrameTime = 0f;
      return;
    }

    // Determine momentary frame-to-frame time.
    long frameTime = now - previousFrameTime;

    // Use current frame time as previous frame time during next invocation.
    previousFrameTime = now;

    // Is nextFrame() being called for the second time, in which case we have only one measurement.
    if (smoothedFrameTime == 0f) {
      smoothedFrameTime = frameTime;
      return;
    }

    // In all subsequent calls to nextFrame(), calculated a smoothed frame rate.
    smoothedFrameTime += SMOOTHING_FACTOR * (frameTime - smoothedFrameTime);
  }

  /** Determine smoothed frame-to-frame time. Returns zero if frame time cannot be determined. */
  public float getSmoothedFrameTime() {
    return smoothedFrameTime;
  }

  /** Determine smoothed frame rate. Returns zero if frame rate cannot be determined. */
  public float getSmoothedFrameRate() {
    return smoothedFrameTime == 0f ? 0f : MILLISECONDS_PER_SECOND / smoothedFrameTime;
  }
}
