/*
 * Copyright 2020 Google LLC
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

import android.content.Context;
import android.content.SharedPreferences;

/** Manages the Instant Placement option setting and shared preferences. */
public class InstantPlacementSettings {
  public static final String SHARED_PREFERENCES_ID = "SHARED_PREFERENCES_INSTANT_PLACEMENT_OPTIONS";
  public static final String SHARED_PREFERENCES_INSTANT_PLACEMENT_ENABLED =
      "instant_placement_enabled";
  private boolean instantPlacementEnabled = true;
  private SharedPreferences sharedPreferences;

  /** Initializes the current settings based on the saved value. */
  public void onCreate(Context context) {
    sharedPreferences = context.getSharedPreferences(SHARED_PREFERENCES_ID, Context.MODE_PRIVATE);
    instantPlacementEnabled =
        sharedPreferences.getBoolean(SHARED_PREFERENCES_INSTANT_PLACEMENT_ENABLED, false);
  }

  /** Retrieves whether Instant Placement is enabled, */
  public boolean isInstantPlacementEnabled() {
    return instantPlacementEnabled;
  }

  public void setInstantPlacementEnabled(boolean enable) {
    if (enable == instantPlacementEnabled) {
      return; // No change.
    }

    // Updates the stored default settings.
    instantPlacementEnabled = enable;
    SharedPreferences.Editor editor = sharedPreferences.edit();
    editor.putBoolean(SHARED_PREFERENCES_INSTANT_PLACEMENT_ENABLED, instantPlacementEnabled);
    editor.apply();
  }
}
