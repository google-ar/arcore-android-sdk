/*
 * Copyright 2020 Google LLC
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

package com.google.ar.core.examples.java.persistentcloudanchor;

import android.content.Intent;
import android.os.Bundle;
import androidx.appcompat.app.AppCompatActivity;
import com.google.android.material.button.MaterialButton;
import com.google.ar.core.examples.java.common.helpers.DisplayRotationHelper;

/** Main Navigation Activity for the Persistent Cloud Anchor Sample. */
public class MainLobbyActivity extends AppCompatActivity {

  private DisplayRotationHelper displayRotationHelper;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.main_lobby);
    displayRotationHelper = new DisplayRotationHelper(this);
    MaterialButton hostButton = findViewById(R.id.host_button);
    hostButton.setOnClickListener((view) -> onHostButtonPress());
    MaterialButton resolveButton = findViewById(R.id.begin_resolve_button);
    resolveButton.setOnClickListener((view) -> onResolveButtonPress());
  }

  @Override
  protected void onResume() {
    super.onResume();
    displayRotationHelper.onResume();
  }

  @Override
  public void onPause() {
    super.onPause();
    displayRotationHelper.onPause();
  }

  private void onHostButtonPress() {
    Intent intent = CloudAnchorActivity.newHostingIntent(this);
    startActivity(intent);
  }

  /** Callback function invoked when the Resolve Button is pressed. */
  private void onResolveButtonPress() {
    Intent intent = ResolveAnchorsLobbyActivity.newIntent(this);
    startActivity(intent);
  }
}
