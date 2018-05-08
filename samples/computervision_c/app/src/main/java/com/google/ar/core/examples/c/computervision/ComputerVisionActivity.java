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

package com.google.ar.core.examples.c.computervision;

import android.hardware.Camera;
import android.hardware.Camera.CameraInfo;
import android.hardware.display.DisplayManager;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.View;
import android.view.WindowManager;
import android.widget.Toast;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * This is a simple example that shows how to create an augmented reality (AR) application using the
 * ARCore C API.
 */
public class ComputerVisionActivity extends AppCompatActivity
    implements GLSurfaceView.Renderer, DisplayManager.DisplayListener {
  private static final float SWIPE_SCALING_FACTOR = 1.15f;
  private static final float MIN_DELTA = .01f;

  // Opaque native pointer to the native application instance.
  private long nativeApplication;

  private GLSurfaceView surfaceView;
  private boolean viewportChanged = false;
  private int viewportWidth;
  private int viewportHeight;
  // Using float value to set the splitter position in shader in native code.
  private float splitterPosition = 0.5f;
  private float startCoordX = 0;
  private float startCoordY = 0;
  private float startPosition = 0;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);
    surfaceView = findViewById(R.id.surfaceview);

    surfaceView.setOnTouchListener(
        new View.OnTouchListener() {
          @Override
          public boolean onTouch(View view, MotionEvent motionEvent) {
            if (motionEvent.getAction() == MotionEvent.ACTION_DOWN) {
              startCoordX = motionEvent.getX();
              startCoordY = motionEvent.getY();
              startPosition = splitterPosition;

            } else if (motionEvent.getAction() == MotionEvent.ACTION_MOVE) {
              float delta = 0;
              switch (getWindowManager().getDefaultDisplay().getRotation()) {
                case Surface.ROTATION_90:
                  delta = (motionEvent.getX() - startCoordX) / view.getWidth();
                  break;
                case Surface.ROTATION_180:
                  delta = -(motionEvent.getY() - startCoordY) / view.getHeight();
                  break;
                case Surface.ROTATION_270:
                  delta = -(motionEvent.getX() - startCoordX) / view.getWidth();
                  break;
                case Surface.ROTATION_0:
                default:
                  // Other cases, use Surface.ROTATION_0's delta value.
                  delta = (motionEvent.getY() - startCoordY) / view.getHeight();
                  break;
              }
              if (Math.abs(delta) > MIN_DELTA) {
                float newPosition = startPosition + delta * SWIPE_SCALING_FACTOR;
                newPosition = Math.min(1.f, Math.max(0.f, newPosition));
                splitterPosition = newPosition;
              }
            }

            return true;
          }
        });

    // Set up renderer.
    surfaceView.setPreserveEGLContextOnPause(true);
    surfaceView.setEGLContextClientVersion(2);
    surfaceView.setEGLConfigChooser(8, 8, 8, 8, 16, 0); // Alpha used for plane blending.
    surfaceView.setRenderer(this);
    surfaceView.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);

    nativeApplication = JniInterface.createNativeApplication();
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

    JniInterface.onResume(nativeApplication, getApplicationContext(), this);
    surfaceView.onResume();

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
    }
  }

  @Override
  public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] results) {
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
}
