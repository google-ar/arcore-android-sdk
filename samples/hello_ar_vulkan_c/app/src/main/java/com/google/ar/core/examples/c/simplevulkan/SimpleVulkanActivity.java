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

package com.google.ar.core.examples.c.simplevulkan;

import android.hardware.display.DisplayManager;
import android.os.Bundle;
import android.util.Log;
import android.view.GestureDetector;
import android.view.Surface;
import android.view.View;
import android.view.WindowManager;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import com.google.android.material.snackbar.Snackbar;

/**
 * This is a simple example that shows how to create a Vulkan rendering application using the ARCore
 * C API.
 */
public class SimpleVulkanActivity extends AppCompatActivity
    implements VulkanSurfaceView.Renderer, DisplayManager.DisplayListener {
  private static final String TAG = SimpleVulkanActivity.class.getSimpleName();
  private static final int SNACKBAR_UPDATE_INTERVAL_MILLIS = 1000; // In milliseconds.

  private VulkanSurfaceView surfaceView;

  private boolean viewportChanged = false;
  private int viewportWidth;
  private int viewportHeight;

  // Opaque native pointer to the native application instance.
  private long nativeApplication;
  private GestureDetector gestureDetector;

  private Snackbar snackbar;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);
    surfaceView = (VulkanSurfaceView) findViewById(R.id.surfaceview);
    surfaceView.setRenderer(this);

    JniInterface.assetManager = getAssets();
    nativeApplication = JniInterface.createNativeApplication(getAssets());
  }

  @Override
  protected void onResume() {
    super.onResume();
    // ARCore requires camera permissions to operate. If we did not yet obtain runtime
    // permission on Android M and above, now is a good time to ask the user for it.
    if (!CameraPermissionHelper.hasCameraPermission(this)) {
      CameraPermissionHelper.requestCameraPermission(this);
      return;
    }

    try {
      JniInterface.onResume(nativeApplication, getApplicationContext(), this);
    } catch (Exception e) {
      Log.e(TAG, "Exception creating session", e);
      displayInSnackbar(e.getMessage());
      return;
    }

    displayInSnackbar("Simple Vulkan Sample");

    // Listen to display changed events to detect 180° rotation, which does not cause a config
    // change or view resize.
    getSystemService(DisplayManager.class).registerDisplayListener(this, null);
  }

  @Override
  public void onPause() {
    super.onPause();
    JniInterface.onPause(nativeApplication);
    getSystemService(DisplayManager.class).unregisterDisplayListener(this);
  }

  @Override
  public void onDestroy() {
    super.onDestroy();

    // Synchronized to avoid racing onDrawFrame.
    synchronized (this) {
      JniInterface.destroyNativeApplication(nativeApplication);
      nativeApplication = 0;
    }
  }

  @Override
  public void onWindowFocusChanged(boolean hasFocus) {
    super.onWindowFocusChanged(hasFocus);
    if (hasFocus) {
      // Standard Android full-screen functionality.
      getWindow()
          .getDecorView()
          .setSystemUiVisibility(
              View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                  | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                  | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                  | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                  | View.SYSTEM_UI_FLAG_FULLSCREEN
                  | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
      getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }
  }

  @Override
  public void onSurfaceCreated(Surface surface) {
    JniInterface.onSurfaceCreated(nativeApplication, surface);
  }

  @Override
  public void onSurfaceChanged(int width, int height) {
    viewportWidth = width;
    viewportHeight = height;
    viewportChanged = true;
  }

  @Override
  public void onDrawFrame() {
    // Synchronized to avoid racing onDestroy.
    synchronized (this) {
      if (nativeApplication == 0) {
        return;
      }
      if (viewportChanged) {
        int displayRotation = getWindowManager().getDefaultDisplay().getRotation();
        JniInterface.onDisplayGeometryChanged(
            nativeApplication, displayRotation, viewportWidth, viewportHeight);
        viewportChanged = false;
      }
      JniInterface.onSurfaceDrawFrame(nativeApplication);
    }
  }

  @Override
  public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] results) {
    super.onRequestPermissionsResult(requestCode, permissions, results);
    if (!CameraPermissionHelper.hasCameraPermission(this)) {
      Toast.makeText(this, "Camera permission is needed to run this application", Toast.LENGTH_LONG)
          .show();
      if (!CameraPermissionHelper.shouldShowRequestPermissionRationale(this)) {
        // Permission denied with checking "Do not ask again".
        CameraPermissionHelper.launchPermissionSettings(this);
      }
      finish();
    }
  }

  /** Display the message in the snackbar. */
  private void displayInSnackbar(String message) {
    snackbar =
        Snackbar.make(
            SimpleVulkanActivity.this.findViewById(android.R.id.content),
            message,
            Snackbar.LENGTH_INDEFINITE);

    // Set the snackbar background to light transparent black color.
    snackbar.getView().setBackgroundColor(0xbf323232);
    snackbar.show();
  }

  // DisplayListener methods
  @Override
  public void onDisplayAdded(int displayId) {}

  @Override
  public void onDisplayRemoved(int displayId) {}

  @Override
  public void onDisplayChanged(int displayId) {
    viewportChanged = true;
  }
}
