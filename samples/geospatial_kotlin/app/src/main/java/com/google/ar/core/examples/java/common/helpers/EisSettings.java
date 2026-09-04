/*
 * Copyright 2023 Google LLC
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

/**
 * A class providing persistent EIS preference across instances using {@code
 * android.content.SharedPreferences}.
 */
public class EisSettings {
  public static final String SHARED_PREFERENCE_ID = "SHARED_PREFERENCE_EIS_OPTIONS";
  public static final String SHARED_PREFERENCE_EIS_ENABLED = "eis_enabled";
  private boolean eisEnabled = false;
  private SharedPreferences sharedPreferences;

  /** Creates shared preference entry for EIS setting. */
  public void onCreate(Context context) {
    sharedPreferences = context.getSharedPreferences(SHARED_PREFERENCE_ID, Context.MODE_PRIVATE);
    eisEnabled = sharedPreferences.getBoolean(SHARED_PREFERENCE_EIS_ENABLED, false);
  }

  /** Returns saved EIS state. */
  public boolean isEisEnabled() {
    return eisEnabled;
  }

  /** Sets and saves the EIS using {@code android.content.SharedPreferences} */
  public void setEisEnabled(boolean enable) {
    if (enable == eisEnabled) {
      return;
    }

    eisEnabled = enable;
    SharedPreferences.Editor editor = sharedPreferences.edit();
    editor.putBoolean(SHARED_PREFERENCE_EIS_ENABLED, eisEnabled);
    editor.apply();
  }
}
