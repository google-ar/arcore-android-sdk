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

package com.google.ar.core.examples.c.computervision;

import android.hardware.Camera;
import android.hardware.Camera.CameraInfo;
import android.hardware.display.DisplayManager;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.View;
import android.view.WindowManager;
import android.widget.CompoundButton;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import com.google.android.material.snackbar.Snackbar;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * This is a simple example that shows how to create an augmented reality (AR) application using the
 * ARCore C API.
 */
public class ComputerVisionActivity extends AppCompatActivity
    implements GLSurfaceView.Renderer, DisplayManager.DisplayListener {
  private static final String TAG = ComputerVisionActivity.class.getSimpleName();

  // Opaque native pointer to the native application instance.
  private long nativeApplication;

  private GLSurfaceView surfaceView;
  private boolean viewportChanged = false;
  private int viewportWidth;
  private int viewportHeight;
  // Using float value to set the splitter position in shader in native code.
  private float splitterPosition = 0.0f;
  private boolean isLowResolutionSelected = true;

  // Camera intrinsics text elements.
  private TextView cameraIntrinsicsTextView;

  private Switch focusModeSwitch;
  private GestureDetector gestureDetector;
  private Snackbar snackbar;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);
    cameraIntrinsicsTextView = findViewById(R.id.camera_intrinsics_view);
    focusModeSwitch = (Switch) findViewById(R.id.switch_focus_mode);
    focusModeSwitch.setOnCheckedChangeListener(this::onFocusModeChanged);

    surfaceView = findViewById(R.id.surfaceview);
    gestureDetector =
        new GestureDetector(
            this,
            new GestureDetector.SimpleOnGestureListener() {
              @Override
              public boolean onSingleTapUp(MotionEvent e) {
                splitterPosition = (splitterPosition < 0.5f) ? 1.0f : 0.0f;

                // Turn off the CPU resolution radio buttons if CPU image is not displayed.
                showCameraConfigMenu(splitterPosition < 0.5f);
                return true;
              }

              @Override
              public boolean onDown(MotionEvent e) {
                return true;
              }
            });
    surfaceView.setOnTouchListener((unusedView, event) -> gestureDetector.onTouchEvent(event));

    // Set up renderer.
    surfaceView.setPreserveEGLContextOnPause(true);
    surfaceView.setEGLContextClientVersion(2);
    surfaceView.setEGLConfigChooser(8, 8, 8, 8, 16, 0); // Alpha used for plane blending.
    surfaceView.setRenderer(this);
    surfaceView.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
    surfaceView.setWillNotDraw(false);

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
      surfaceView.onResume();
    } catch (Exception e) {
      Log.e(TAG, "Exception resuming session", e);
      displayInSnackbar(e.getMessage());
      return;
    }

    // Update the radio buttons with the resolution info.
    String lowResLabel = JniInterface.getCameraConfigLabel(nativeApplication, true);
    String highResLabel = JniInterface.getCameraConfigLabel(nativeApplication, false);
    RadioButton lowResolutionRadioButton = (RadioButton) findViewById(R.id.radio_low_res);
    RadioButton highResolutionRadioButton = (RadioButton) findViewById(R.id.radio_high_res);
    if (!lowResLabel.isEmpty()) {
      lowResolutionRadioButton.setText(lowResLabel);
    } else {
      lowResolutionRadioButton.setVisibility(View.INVISIBLE);
    }
    if (!highResLabel.isEmpty()) {
      highResolutionRadioButton.setText(highResLabel);
    } else {
      highResolutionRadioButton.setVisibility(View.INVISIBLE);
    }

    focusModeSwitch.setChecked(JniInterface.getFocusMode(nativeApplication));

    // Listen to display changed events to detect 180° rotation, which does not cause a config
    // change or view resize.
    getSystemService(DisplayManager.class).registerDisplayListener(this, null);
  }

  @Override
  public void onPause() {
    super.onPause();
    surfaceView.onPause();
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
  public void onSurfaceCreated(GL10 gl, EGLConfig config) {
    GLES20.glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    JniInterface.onGlSurfaceCreated(nativeApplication);
  }

  @Override
  public void onSurfaceChanged(GL10 gl, int width, int height) {
    viewportWidth = width;
    viewportHeight = height;
    viewportChanged = true;
  }

  private int convertDeviceRotationInDegress() {
    int screenDegrees = 0;
    switch (getWindowManager().getDefaultDisplay().getRotation()) {
      case Surface.ROTATION_0:
        screenDegrees = 0;
        break;
      case Surface.ROTATION_90:
        screenDegrees = 90;
        break;
      case Surface.ROTATION_180:
        screenDegrees = 180;
        break;
      case Surface.ROTATION_270:
        screenDegrees = 270;
        break;
      default:
        break;
    }
    return screenDegrees;
  }

  private int getCameraToDisplayRotation() {
    CameraInfo cameraInfo = new CameraInfo();
    Camera.getCameraInfo(CameraInfo.CAMERA_FACING_BACK, cameraInfo);

    int cameraToScreenDegrees =
        (cameraInfo.orientation - convertDeviceRotationInDegress() + 360) % 360;

    // Convert degrees to rotation ids.
    int cameraToScreenRotation = Surface.ROTATION_0;
    switch (cameraToScreenDegrees) {
      case 0:
        cameraToScreenRotation = Surface.ROTATION_0;
        break;
      case 90:
        cameraToScreenRotation = Surface.ROTATION_90;
        break;
      case 180:
        cameraToScreenRotation = Surface.ROTATION_180;
        break;
      case 270:
        cameraToScreenRotation = Surface.ROTATION_270;
        break;
      default:
        break;
    }

    return cameraToScreenRotation;
  }

  @Override
  public void onDrawFrame(GL10 gl) {
    // Synchronized to avoid racing onDestroy.
    synchronized (this) {
      if (nativeApplication == 0) {
        return;
      }
      if (viewportChanged) {
        int displayRotation = getWindowManager().getDefaultDisplay().getRotation();
        JniInterface.onDisplayGeometryChanged(
            nativeApplication,
            displayRotation,
            getCameraToDisplayRotation(),
            viewportWidth,
            viewportHeight);
        viewportChanged = false;
      }

      JniInterface.onGlSurfaceDrawFrame(nativeApplication, splitterPosition);
      final String cameraIntrinsicsText =
          JniInterface.getCameraIntrinsicsText(
              nativeApplication, /*forGpuTexture=*/ (splitterPosition > 0.5f));

      runOnUiThread(() -> cameraIntrinsicsTextView.setText(cameraIntrinsicsText));
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

  // DisplayListener methods
  @Override
  public void onDisplayAdded(int displayId) {}

  @Override
  public void onDisplayRemoved(int displayId) {}

  @Override
  public void onDisplayChanged(int displayId) {
    viewportChanged = true;
  }

  public void onLowResolutionRadioButtonClicked(View view) {
    boolean checked = ((RadioButton) view).isChecked();
    if (checked && !isLowResolutionSelected) {
      // Display low resolution.
      isLowResolutionSelected = true;
      String label = (String) ((RadioButton) view).getText();
      onCameraConfigChanged(isLowResolutionSelected, label);
    }
  }

  public void onHighResolutionRadioButtonClicked(View view) {
    boolean checked = ((RadioButton) view).isChecked();
    if (checked && isLowResolutionSelected) {
      // Display high resolution
      isLowResolutionSelected = false;
      String label = (String) ((RadioButton) view).getText();
      onCameraConfigChanged(isLowResolutionSelected, label);
    }
  }

  private void onFocusModeChanged(CompoundButton unusedButton, boolean isChecked) {
    JniInterface.setFocusMode(nativeApplication, isChecked);
  }

  private void onCameraConfigChanged(boolean isLowResolution, String label) {
    int status = JniInterface.setCameraConfig(nativeApplication, isLowResolution);
    if (status == 0) {
      // Let the user know that the camera config is set.
      String message = "Set the camera config with " + label;
      Toast.makeText(this, message, Toast.LENGTH_SHORT).show();
    }
  }

  private void showCameraConfigMenu(boolean show) {
    RadioGroup radioGroup = (RadioGroup) findViewById(R.id.radio_camera_configs);
    radioGroup.setVisibility(show ? View.VISIBLE : View.INVISIBLE);
  }

  /** Display the message in the snackbar. */
  private void displayInSnackbar(String message) {
    snackbar =
        Snackbar.make(findViewById(android.R.id.content), message, Snackbar.LENGTH_INDEFINITE);

    snackbar.show();
  }
}
