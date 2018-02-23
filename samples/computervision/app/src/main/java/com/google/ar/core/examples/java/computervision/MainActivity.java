/*
 * Copyright 2017 Google Inc. All Rights Reserved.
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

package com.google.ar.core.examples.java.computervision;

import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.support.design.widget.BaseTransientBottomBar;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.View;
import android.view.WindowManager;
import android.widget.Toast;
import com.google.ar.core.Frame;
import com.google.ar.core.Session;
import com.google.ar.core.examples.java.computervision.rendering.BackgroundRenderer;
import com.google.ar.core.examples.java.computervision.utility.CameraImageBuffer;
import com.google.ar.core.examples.java.computervision.utility.CameraPermissionHelper;
import com.google.ar.core.examples.java.computervision.utility.DisplayRotationHelper;
import com.google.ar.core.examples.java.computervision.utility.EdgeDetector;
import com.google.ar.core.examples.java.computervision.utility.TextureReader;
import com.google.ar.core.exceptions.UnavailableApkTooOldException;
import com.google.ar.core.exceptions.UnavailableArcoreNotInstalledException;
import com.google.ar.core.exceptions.UnavailableSdkTooOldException;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/** This is a simple example that demonstrates texture reading with ARCore. */
public class MainActivity extends AppCompatActivity implements GLSurfaceView.Renderer {
  private static final String TAG = MainActivity.class.getSimpleName();

  // Rendering. The Renderers are created here, and initialized when the GL surface is created.
  private GLSurfaceView surfaceView;

  private Session session;
  private Snackbar messageSnackbar;
  private DisplayRotationHelper displayRotationHelper;

  private final BackgroundRenderer backgroundRenderer = new BackgroundRenderer();
  private final CameraImageBuffer edgeImage = new CameraImageBuffer();
  private final TextureReader textureReader = new TextureReader();

  // ArCore full resolution texture has a size of 1920 x 1080.
  private static final int TEXTURE_WIDTH = 1920;
  private static final int TEXTURE_HEIGHT = 1080;

  // We choose a lower sampling resolution.
  private static final int IMAGE_WIDTH = 1024;
  private static final int IMAGE_HEIGHT = 512;

  private int frameBufferIndex = -1;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);
    surfaceView = (GLSurfaceView) findViewById(R.id.surfaceview);
    displayRotationHelper = new DisplayRotationHelper(/*context=*/ this);

    // Setup a touch listener to control the texture splitter position.
    surfaceView.setOnTouchListener(
        new View.OnTouchListener() {
          private static final float SWIPE_SCALING_FACTOR = 1.15f;
          private static final float MIN_DELTA = .01f;
          private float startPosition = 0;
          private float startCoordX = 0;
          private float startCoordY = 0;
          private int displayRotation = 0;

          @Override
          public boolean onTouch(View v, MotionEvent e) {
            switch (e.getAction()) {
              case MotionEvent.ACTION_DOWN:
                {
                  startCoordX = e.getX();
                  startCoordY = e.getY();
                  displayRotation = displayRotationHelper.getRotation();
                  startPosition = backgroundRenderer.getSplitterPosition();
                  break;
                }
                case MotionEvent.ACTION_MOVE:
                {
                  float delta = 0;
                  switch (displayRotation) {
                    case Surface.ROTATION_90:
                      delta = (e.getX() - startCoordX) / surfaceView.getWidth();
                      break;
                    case Surface.ROTATION_180:
                      delta = -(e.getY() - startCoordY) / surfaceView.getHeight();
                      break;
                    case Surface.ROTATION_270:
                      delta = -(e.getX() - startCoordX) / surfaceView.getWidth();
                      break;
                    case Surface.ROTATION_0:
                    default:
                      delta = (e.getY() - startCoordY) / surfaceView.getHeight();
                      break;
                  }
                  if (Math.abs(delta) > MIN_DELTA) {
                    float newPosition = startPosition + delta * SWIPE_SCALING_FACTOR;
                    newPosition = Math.min(1.f, Math.max(0.f, newPosition));
                    backgroundRenderer.setSplitterPosition(newPosition);
                  }
                  break;
                }
              default:
                break;
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
  }

  @Override
  protected void onResume() {
    super.onResume();

    if (session == null) {
      Exception exception = null;
      String message = null;
      try {
        // ARCore requires camera permissions to operate. If we did not yet obtain runtime
        // permission on Android M and above, now is a good time to ask the user for it.
        if (!CameraPermissionHelper.hasCameraPermission(this)) {
          CameraPermissionHelper.requestCameraPermission(this);
          return;
        }

        session = new Session(/* context= */ this);
      } catch (UnavailableArcoreNotInstalledException e) {
        message = "Please install ARCore";
        exception = e;
      } catch (UnavailableApkTooOldException e) {
        message = "Please update ARCore";
        exception = e;
      } catch (UnavailableSdkTooOldException e) {
        message = "Please update this app";
        exception = e;
      } catch (Exception e) {
        message = "This device does not support AR";
        exception = e;
      }

      if (message != null) {
        showSnackbarMessage(message, true);
        Log.e(TAG, "Exception creating session", exception);
        return;
      }
    }

    // Note that order matters - see the note in onPause(), the reverse applies here.
    session.resume();
    surfaceView.onResume();
    displayRotationHelper.onResume();
  }

  @Override
  public void onPause() {
    super.onPause();
    // Note that the order matters - GLSurfaceView is paused first so that it does not try
    // to query the session. If Session is paused before GLSurfaceView, GLSurfaceView may
    // still call session.update() and get a SessionPausedException.
    displayRotationHelper.onPause();
    surfaceView.onPause();
    if (session != null) {
      session.pause();
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

    // Create the texture and pass it to ARCore session to be filled during update().
    backgroundRenderer.createOnGlThread(/*context=*/ this);

    if (session != null) {
      session.setCameraTextureName(backgroundRenderer.getTextureId());
    }

    // The image format can be either IMAGE_FORMAT_RGBA or IMAGE_FORMAT_I8.
    // Set keepAspectRatio to false so that the output image covers the whole viewport.
    textureReader.create(CameraImageBuffer.IMAGE_FORMAT_I8, IMAGE_WIDTH, IMAGE_HEIGHT, false);
  }

  @Override
  public void onSurfaceChanged(GL10 gl, int width, int height) {
    displayRotationHelper.onSurfaceChanged(width, height);
    GLES20.glViewport(0, 0, width, height);
  }

  @Override
  public void onDrawFrame(GL10 gl) {
    // Clear screen to notify driver it should not load any pixels from previous frame.
    GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT | GLES20.GL_DEPTH_BUFFER_BIT);
    if (session == null) {
      return;
    }
    // Notify ARCore session that the view size changed so that the perspective matrix and
    // the video background can be properly adjusted.
    displayRotationHelper.updateSessionIfNeeded(session);

    try {
      Frame frame = session.update();

      // If there is a frame being requested previously, acquire the pixels and process it.
      if (frameBufferIndex >= 0) {
        CameraImageBuffer imageBuffer = textureReader.acquireFrame(frameBufferIndex);

        // Detect the edges from the captured grayscale image.
        if (EdgeDetector.detect(edgeImage, imageBuffer)) {
          // Set the edge image to renderer as overlay.
          backgroundRenderer.setOverlayImage(edgeImage);
        }

        // You should always release frame buffer after using. Otherwise the next call to
        // submitFrame() may fail.
        textureReader.releaseFrame(frameBufferIndex);
      }

      // Submit request for the texture from the current frame.
      frameBufferIndex =
          textureReader.submitFrame(
              backgroundRenderer.getTextureId(), TEXTURE_WIDTH, TEXTURE_HEIGHT);

      // Draw background video.
      backgroundRenderer.draw(frame);

    } catch (Throwable t) {
      // Avoid crashing the application due to unhandled exceptions.
      Log.e(TAG, "Exception on the OpenGL thread", t);
    }
  }

  private void showSnackbarMessage(String message, boolean finishOnDismiss) {
    messageSnackbar =
        Snackbar.make(
            MainActivity.this.findViewById(android.R.id.content),
            message,
            Snackbar.LENGTH_INDEFINITE);
    messageSnackbar.getView().setBackgroundColor(0xbf323232);
    if (finishOnDismiss) {
      messageSnackbar.setAction(
          "Dismiss",
          new View.OnClickListener() {
            @Override
            public void onClick(View v) {
              messageSnackbar.dismiss();
            }
          });
      messageSnackbar.addCallback(
          new BaseTransientBottomBar.BaseCallback<Snackbar>() {
            @Override
            public void onDismissed(Snackbar transientBottomBar, int event) {
              super.onDismissed(transientBottomBar, event);
              finish();
            }
          });
    }
    messageSnackbar.show();
  }
}
