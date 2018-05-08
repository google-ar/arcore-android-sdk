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

package com.google.ar.core.examples.c.cloudanchor;

import android.hardware.display.DisplayManager;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.Handler;
import android.support.design.widget.BaseTransientBottomBar;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * This is a simple example that shows how to create an augmented reality (AR) application using the
 * ARCore C API.
 */
public class CloudAnchorActivity extends AppCompatActivity
    implements GLSurfaceView.Renderer, DisplayManager.DisplayListener {
  private static final String TAG = CloudAnchorActivity.class.getSimpleName();
  private static final int SNACKBAR_UPDATE_INTERVAL_MILLIS = 1000; // In milliseconds.

  private GLSurfaceView mSurfaceView;

  private boolean mViewportChanged = false;
  private int mViewportWidth;
  private int mViewportHeight;

  // Opaque native pointer to the native application instance.
  private long mNativeApplication;
  private GestureDetector mGestureDetector;

  // Persistence members.
  private Button hostButton;
  private Button resolveButton;
  private TextView roomCodeText;
  private boolean showingLoadingMessage;
  private FirebaseManager firebaseManager;
  private FirebaseRoomCodeListener firebaseListener;

  private Snackbar mLoadingMessageSnackbar;
  private Handler mPlaneStatusCheckingHandler;
  private final Runnable mPlaneStatusCheckingRunnable =
      new Runnable() {
        @Override
        public void run() {
          // The runnable is executed on main UI thread.
          try {
            if (JniInterface.hasDetectedPlanes(mNativeApplication)) {
              if (mLoadingMessageSnackbar != null) {
                mLoadingMessageSnackbar.dismiss();
              }
              mLoadingMessageSnackbar = null;
            } else {
              mPlaneStatusCheckingHandler.postDelayed(
                  mPlaneStatusCheckingRunnable, SNACKBAR_UPDATE_INTERVAL_MILLIS);
            }
          } catch (Exception e) {
            Log.e(TAG, e.getMessage());
          }
        }
      };

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);
    mSurfaceView = (GLSurfaceView) findViewById(R.id.surfaceview);

    // Set up tap listener.
    mGestureDetector =
        new GestureDetector(
            this,
            new GestureDetector.SimpleOnGestureListener() {
              @Override
              public boolean onSingleTapUp(final MotionEvent e) {
                mSurfaceView.queueEvent(
                    new Runnable() {
                      @Override
                      public void run() {
                        JniInterface.onTouched(mNativeApplication, e.getX(), e.getY());
                      }
                    });
                return true;
              }

              @Override
              public boolean onDown(MotionEvent e) {
                return true;
              }
            });

    mSurfaceView.setOnTouchListener(
        new View.OnTouchListener() {
          @Override
          public boolean onTouch(View v, MotionEvent event) {
            return mGestureDetector.onTouchEvent(event);
          }
        });

    // Set up renderer.
    mSurfaceView.setPreserveEGLContextOnPause(true);
    mSurfaceView.setEGLContextClientVersion(2);
    mSurfaceView.setEGLConfigChooser(8, 8, 8, 8, 16, 0); // Alpha used for plane blending.
    mSurfaceView.setRenderer(this);
    mSurfaceView.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);

    // Set up persistence related members.
    hostButton = findViewById(R.id.host_button);
    hostButton.setOnClickListener((view) -> onHostButtonPress());

    resolveButton = findViewById(R.id.resolve_button);
    resolveButton.setOnClickListener((view) -> onResolveButtonPress());

    roomCodeText = findViewById(R.id.room_code_text);
    roomCodeText.setText(R.string.initial_room_code);

    firebaseListener = new FirebaseRoomCodeListener();
    firebaseManager = new FirebaseManager(this, firebaseListener);

    JniInterface.setCallbackInterface(
        new JniInterface.CallbackInterface() {
          @Override
          public void setHostAndResolveVisibility(
              boolean hostRoomEnabled, boolean resolveRoomEnabled) {
            updateHostAndResolveVisibilityButtons(hostRoomEnabled, resolveRoomEnabled);
          }

          @Override
          public void showResolveDialog() {
            ResolveDialogFragment dialogFragment = new ResolveDialogFragment();
            dialogFragment.setResultListener(
                new ResolveDialogFragment.ResultListener() {
                  @Override
                  public void onOkPressed(Long roomCode) {
                    JniInterface.onResolveOkPress(mNativeApplication, roomCode);
                    firebaseManager.registerNewListenerForRoom(roomCode);
                  }

                  @Override
                  public void onCancelPressed() {
                    JniInterface.onResolveCancelPress(mNativeApplication);
                  }
                });

            dialogFragment.show(getSupportFragmentManager(), "ResolveDialog");
          }

          public void displayMessageOnLowerSnackbar(String message) {
            showMessage(message);
          }

          @Override
          public void maybeUpdateFirebase(String anchorId) {
            firebaseListener.maybeShareCloudAnchorId(anchorId);
          }

          @Override
          public void updateFirebaseRoomCode(boolean update, long optionalNewRoomCode) {
            if (update) {
              firebaseManager.getNewRoomCode();
            } else {
              if (optionalNewRoomCode != 0) {
                roomCodeText.setText(Long.toString(optionalNewRoomCode));
              } else {
                roomCodeText.setText(R.string.initial_room_code);
              }
            }
          }
        });
    JniInterface.assetManager = getAssets();
    mNativeApplication = JniInterface.createNativeApplication(getAssets());

    mPlaneStatusCheckingHandler = new Handler();
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

    JniInterface.onResume(mNativeApplication, getApplicationContext(), this);
    mSurfaceView.onResume();

    mLoadingMessageSnackbar =
        Snackbar.make(
            CloudAnchorActivity.this.findViewById(android.R.id.content),
            "Searching for surfaces...",
            Snackbar.LENGTH_INDEFINITE);
    // Set the snackbar background to light transparent black color.
    mLoadingMessageSnackbar.getView().setBackgroundColor(0xbf323232);
    mLoadingMessageSnackbar.show();
    mPlaneStatusCheckingHandler.postDelayed(
        mPlaneStatusCheckingRunnable, SNACKBAR_UPDATE_INTERVAL_MILLIS);

    // Listen to display changed events to detect 180° rotation, which does not cause a config
    // change or view resize.
    getSystemService(DisplayManager.class).registerDisplayListener(this, null);
  }

  @Override
  public void onPause() {
    super.onPause();
    mSurfaceView.onPause();
    JniInterface.onPause(mNativeApplication);

    mPlaneStatusCheckingHandler.removeCallbacks(mPlaneStatusCheckingRunnable);

    getSystemService(DisplayManager.class).unregisterDisplayListener(this);
  }

  @Override
  public void onDestroy() {
    super.onDestroy();

    // Synchronized to avoid racing onDrawFrame.
    synchronized (this) {
      JniInterface.destroyNativeApplication(mNativeApplication);
      mNativeApplication = 0;
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
    JniInterface.onGlSurfaceCreated(mNativeApplication);
  }

  @Override
  public void onSurfaceChanged(GL10 gl, int width, int height) {
    mViewportWidth = width;
    mViewportHeight = height;
    mViewportChanged = true;
  }

  @Override
  public void onDrawFrame(GL10 gl) {
    // Synchronized to avoid racing onDestroy.
    synchronized (this) {
      if (mNativeApplication == 0) {
        return;
      }
      if (mViewportChanged) {
        int displayRotation = getWindowManager().getDefaultDisplay().getRotation();
        JniInterface.onDisplayGeometryChanged(
            mNativeApplication, displayRotation, mViewportWidth, mViewportHeight);
        mViewportChanged = false;
      }
      JniInterface.onGlSurfaceDrawFrame(mNativeApplication);
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
    mViewportChanged = true;
  }

   private void updateHostAndResolveVisibilityButtons(
      boolean hostingEnabled, boolean resolvingEnabled) {
    hostButton.setEnabled(hostingEnabled);
    resolveButton.setEnabled(resolvingEnabled);
    if (hostingEnabled && resolvingEnabled) {
      hostButton.setText(R.string.host_button_text);
      resolveButton.setText(R.string.resolve_button_text);
    } else if (hostingEnabled && !resolvingEnabled) {
      hostButton.setText(R.string.cancel);
      resolveButton.setText(R.string.resolve_button_text);
    } else if (!hostingEnabled && resolvingEnabled) {
      hostButton.setText(R.string.host_button_text);
      resolveButton.setText(R.string.cancel);
    } else {
      Log.e(TAG, "setHostAndResolveVisibility was called with all vals false, which is invalid.");
    }
  }

  private void showSnackbarMessage(String message, boolean finishOnDismiss) {
    mLoadingMessageSnackbar =
        Snackbar.make(
            CloudAnchorActivity.this.findViewById(android.R.id.content),
            message,
            Snackbar.LENGTH_INDEFINITE);
    mLoadingMessageSnackbar.getView().setBackgroundColor(0xbf323232);
    mLoadingMessageSnackbar.setAction(
        "Dismiss",
        (v) -> {
          showingLoadingMessage = false;
          mLoadingMessageSnackbar.dismiss();
        });
    if (finishOnDismiss) {
      mLoadingMessageSnackbar.addCallback(
          new BaseTransientBottomBar.BaseCallback<Snackbar>() {
            @Override
            public void onDismissed(Snackbar transientBottomBar, int event) {
              super.onDismissed(transientBottomBar, event);
              finish();
            }
          });
    }
    mLoadingMessageSnackbar.show();
  }

  private void showMessage(String messageStr) {
    runOnUiThread(() -> showSnackbarMessage(messageStr, false));
  }

  private synchronized void onHostButtonPress() {
    JniInterface.onHostButtonPress(mNativeApplication);
  }

  private synchronized void onResolveButtonPress() {
    JniInterface.onResolveButtonPress(mNativeApplication);
  }

  private final class FirebaseRoomCodeListener implements FirebaseManager.RoomCodeListener {
    private Long roomCode;

    @Override
    public synchronized void onNewRoomCode(Long newRoomCode) {
      roomCode = newRoomCode;
      roomCodeText.setText(String.valueOf(roomCode));
      showMessage("The room code is now available.");
    }

    @Override
    public void onError() {
      JniInterface.onFirebaseError(mNativeApplication);
    }

    @Override
    public void onCloudAnchorIdMadeAvailable(String anchorId) {
      JniInterface.onCloudAnchorIdMadeAvailable(mNativeApplication, anchorId);
    }

    public void maybeShareCloudAnchorId(String cloudAnchorId) {
      if (roomCode == null) {
        return;
      }

      firebaseManager.storeAnchorIdInRoom(roomCode, cloudAnchorId);
      showMessage("The anchor ID was shared.");
    }
  }
}
