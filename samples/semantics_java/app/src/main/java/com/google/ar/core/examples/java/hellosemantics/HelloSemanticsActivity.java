/*
 * Copyright 2023 Google LLC
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

package com.google.ar.core.examples.java.hellosemantics;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.media.Image;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import com.google.ar.core.ArCoreApk;
import com.google.ar.core.Camera;
import com.google.ar.core.Config;
import com.google.ar.core.Frame;
import com.google.ar.core.SemanticLabel;
import com.google.ar.core.Session;
import com.google.ar.core.TrackingFailureReason;
import com.google.ar.core.TrackingState;
import com.google.ar.core.examples.java.common.helpers.CameraPermissionHelper;
import com.google.ar.core.examples.java.common.helpers.DisplayRotationHelper;
import com.google.ar.core.examples.java.common.helpers.FullScreenHelper;
import com.google.ar.core.examples.java.common.helpers.SnackbarHelper;
import com.google.ar.core.examples.java.common.helpers.TapHelper;
import com.google.ar.core.examples.java.common.helpers.TrackingStateHelper;
import com.google.ar.core.examples.java.common.samplerender.Framebuffer;
import com.google.ar.core.examples.java.common.samplerender.SampleRender;
import com.google.ar.core.examples.java.common.samplerender.arcore.BackgroundRenderer;
import com.google.ar.core.exceptions.CameraNotAvailableException;
import com.google.ar.core.exceptions.NotYetAvailableException;
import com.google.ar.core.exceptions.UnavailableApkTooOldException;
import com.google.ar.core.exceptions.UnavailableArcoreNotInstalledException;
import com.google.ar.core.exceptions.UnavailableDeviceNotCompatibleException;
import com.google.ar.core.exceptions.UnavailableSdkTooOldException;
import com.google.ar.core.exceptions.UnavailableUserDeclinedInstallationException;
import java.io.IOException;
import java.io.InputStream;

/**
 * This is a simple example that shows how to create an augmented reality (AR) application using the
 * ARCore API. The application will overlay a semantic image of top of the camera image to showcase
 * ARCore's Semantics API.
 */
public class HelloSemanticsActivity extends AppCompatActivity implements SampleRender.Renderer {

  private static final String TAG = HelloSemanticsActivity.class.getSimpleName();

  private static final float Z_NEAR = 0.1f;
  private static final float Z_FAR = 100f;

  // Rendering. The Renderers are created here, and initialized when the GL surface is created.
  private GLSurfaceView surfaceView;

  private boolean installRequested;

  private Session session;
  private final SnackbarHelper messageSnackbarHelper = new SnackbarHelper();
  private DisplayRotationHelper displayRotationHelper;
  private final TrackingStateHelper trackingStateHelper = new TrackingStateHelper(this);
  private TapHelper tapHelper;
  private SampleRender render;

  private BackgroundRenderer backgroundRenderer;
  private SemanticsRenderer semanticsRenderer;
  private Framebuffer virtualSceneFramebuffer;
  private boolean hasSetTextureNames = false;
  private boolean semanticsEnabled = true;

  private LinearLayout semanticsColorLegend;
  private RecyclerView semanticLabelsRecyclerView;
  private SemanticLabelAdapter adapter;
  private RecyclerView.LayoutManager layoutManager;
  private TextView toggleSemanticsLegend;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);
    surfaceView = findViewById(R.id.surfaceview);
    displayRotationHelper = new DisplayRotationHelper(/* context= */ this);

    // Set up touch listener.
    tapHelper = new TapHelper(/* context= */ this);
    surfaceView.setOnTouchListener(tapHelper);

    // Set up renderer.
    render = new SampleRender(surfaceView, this, getAssets());

    setupSemanticsColorMapLegend();

    installRequested = false;
  }

  @Override
  protected void onDestroy() {
    if (session != null) {
      // Explicitly close ARCore Session to release native resources.
      // Review the API reference for important considerations before calling close() in apps with
      // more complicated lifecycle requirements:
      // https://developers.google.com/ar/reference/java/arcore/reference/com/google/ar/core/Session#close()
      session.close();
      session = null;
    }

    super.onDestroy();
  }

  @Override
  protected void onResume() {
    super.onResume();

    if (session == null) {
      Exception exception = null;
      String message = null;
      try {
        switch (ArCoreApk.getInstance().requestInstall(this, !installRequested)) {
          case INSTALL_REQUESTED:
            installRequested = true;
            return;
          case INSTALLED:
            break;
        }

        // ARCore requires camera permissions to operate. If we did not yet obtain runtime
        // permission on Android M and above, now is a good time to ask the user for it.
        if (!CameraPermissionHelper.hasCameraPermission(this)) {
          CameraPermissionHelper.requestCameraPermission(this);
          return;
        }

        // Create the session.
        session = new Session(/* context= */ this);
      } catch (UnavailableArcoreNotInstalledException
          | UnavailableUserDeclinedInstallationException e) {
        message = "Please install ARCore";
        exception = e;
      } catch (UnavailableApkTooOldException e) {
        message = "Please update ARCore";
        exception = e;
      } catch (UnavailableSdkTooOldException e) {
        message = "Please update this app";
        exception = e;
      } catch (UnavailableDeviceNotCompatibleException e) {
        message = "This device does not support AR";
        exception = e;
      } catch (Exception e) {
        message = "Failed to create AR session";
        exception = e;
      }

      if (message != null) {
        messageSnackbarHelper.showError(this, message);
        Log.e(TAG, "Exception creating session", exception);
        return;
      }
    }

    // Note that order matters - see the note in onPause(), the reverse applies here.
    try {
      if (session.isSemanticModeSupported(Config.SemanticMode.ENABLED)) {
        Log.i(TAG, "Semantics API is supported");
        configureSession();
      } else {
        session = null;
        this.runOnUiThread(this::showSemanticsApiDialog);
        return;
      }
      // To record a live camera session for later playback, call
      // `session.startRecording(recordingConfig)` at anytime. To playback a previously recorded AR
      // session instead of using the live camera feed, call
      // `session.setPlaybackDatasetUri(Uri)` before calling `session.resume()`. To
      // learn more about recording and playback, see:
      // https://developers.google.com/ar/develop/java/recording-and-playback
      session.resume();
    } catch (CameraNotAvailableException e) {
      messageSnackbarHelper.showError(this, "Camera not available. Try restarting the app.");
      session = null;
      return;
    }

    surfaceView.onResume();
    displayRotationHelper.onResume();
  }

  @Override
  public void onPause() {
    super.onPause();
    if (session != null) {
      // Note that the order matters - GLSurfaceView is paused first so that it does not try
      // to query the session. If Session is paused before GLSurfaceView, GLSurfaceView may
      // still call session.update() and get a SessionPausedException.
      displayRotationHelper.onPause();
      surfaceView.onPause();
      session.pause();
    }
  }

  @Override
  public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] results) {
    super.onRequestPermissionsResult(requestCode, permissions, results);
    if (!CameraPermissionHelper.hasCameraPermission(this)) {
      session = null;
      // Use toast instead of snackbar here since the activity will exit.
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
    FullScreenHelper.setFullScreenOnWindowFocusChanged(this, hasFocus);
  }

  @Override
  public void onSurfaceCreated(SampleRender render) {
    // Prepare the rendering objects. This involves reading shaders and 3D model files, so may throw
    // an IOException.
    backgroundRenderer = new BackgroundRenderer(render);
    semanticsRenderer = new SemanticsRenderer(render);
    try {
      semanticsRenderer.setupSemanticsShader(render);
    } catch (IOException e) {
      Log.e(TAG, "Failed to read a required asset file", e);
      messageSnackbarHelper.showError(this, "Failed to read a required asset file: " + e);
    }
    virtualSceneFramebuffer = new Framebuffer(render, /* width= */ 1, /* height= */ 1);
  }

  @Override
  public void onSurfaceChanged(SampleRender render, int width, int height) {
    displayRotationHelper.onSurfaceChanged(width, height);
    virtualSceneFramebuffer.resize(width, height);
  }

  @Override
  public void onDrawFrame(SampleRender render) {
    if (session == null) {
      return;
    }

    // Texture names should only be set once on a GL thread unless they change. This is done during
    // onDrawFrame rather than onSurfaceCreated since the session is not guaranteed to have been
    // initialized during the execution of onSurfaceCreated.
    if (!hasSetTextureNames) {
      session.setCameraTextureNames(
          new int[] {backgroundRenderer.getCameraColorTexture().getTextureId()});
      hasSetTextureNames = true;
    }

    // -- Update per-frame state

    // Notify ARCore session that the view size changed so that the perspective matrix and
    // the video background can be properly adjusted.
    displayRotationHelper.updateSessionIfNeeded(session);

    // Obtain the current frame from the AR Session. When the configuration is set to
    // UpdateMode.BLOCKING (it is by default), this will throttle the rendering to the
    // camera framerate.
    Frame frame;
    try {
      frame = session.update();
    } catch (CameraNotAvailableException e) {
      Log.e(TAG, "Camera not available during onDrawFrame", e);
      messageSnackbarHelper.showError(this, "Camera not available. Try restarting the app.");
      return;
    }
    Camera camera = frame.getCamera();

    // Update BackgroundRenderer state to match the depth settings.
    try {
      backgroundRenderer.setUseDepthVisualization(render, false);
      backgroundRenderer.setUseOcclusion(render, false);
    } catch (IOException e) {
      Log.e(TAG, "Failed to read a required asset file", e);
      messageSnackbarHelper.showError(this, "Failed to read a required asset file: " + e);
      return;
    }
    // BackgroundRenderer.updateDisplayGeometry must be called every frame to update the coordinates
    // used to draw the background camera image.
    backgroundRenderer.updateDisplayGeometry(frame);
    semanticsRenderer.updateDisplayGeometry(frame);

    if (camera.getTrackingState() == TrackingState.TRACKING) {
      try (Image semanticImage = frame.acquireSemanticImage()) {
        semanticsRenderer.updateCameraSemanticsTexture(semanticImage);
        updateSemanticLabelsFraction(frame);
      } catch (NotYetAvailableException e) {
        // This normally means that semantics data is not available yet. This is normal so we will
        // not spam the logcat with this.
      }
    }

    // Handle one tap per frame.
    handleTap(frame, camera);

    // Keep the screen unlocked while tracking, but allow it to lock when tracking stops.
    trackingStateHelper.updateKeepScreenOnFlag(camera.getTrackingState());

    // Show a message based on whether tracking has failed, if planes are detected, and if the user
    // has placed any objects.
    String message = null;
    if (camera.getTrackingState() == TrackingState.PAUSED) {
      if (camera.getTrackingFailureReason() != TrackingFailureReason.NONE) {
        message = TrackingStateHelper.getTrackingFailureReasonString(camera);
      }
    }
    if (message == null) {
      messageSnackbarHelper.hide(this);
    } else {
      messageSnackbarHelper.showMessage(this, message);
    }

    // -- Draw background

    if (frame.getTimestamp() != 0) {
      // Suppress rendering if the camera did not produce the first frame yet. This is to avoid
      // drawing possible leftover data from previous sessions if the texture is reused.
      backgroundRenderer.drawBackground(render);
    }

    // If not tracking, don't draw 3D objects.
    if (camera.getTrackingState() == TrackingState.PAUSED) {
      return;
    }

    render.clear(virtualSceneFramebuffer, 0f, 0f, 0f, 0f);
    if (semanticsEnabled) {
      semanticsRenderer.draw(render, virtualSceneFramebuffer);
    }

    // Compose the virtual scene with the background.
    backgroundRenderer.drawVirtualScene(render, virtualSceneFramebuffer, Z_NEAR, Z_FAR);
  }

  /** Configures the session with feature settings. */
  @SuppressWarnings("CheckReturnValue")
  private void configureSession() {
    Config config = session.getConfig();
    config.setSemanticMode(
        semanticsEnabled ? Config.SemanticMode.ENABLED : Config.SemanticMode.DISABLED);
    session.configure(config);
  }

  /**
   * Tapping the screen toggles ARCore Semantics on and off. Handles only one tap per frame, as taps
   * are usually low frequency compared to frame rate.
   */
  private void handleTap(Frame frame, Camera camera) {
    // Retrieves a user tap on the screen. The position of the tap is ignored.
    MotionEvent tap = tapHelper.poll();
    if (tap == null) {
      return;
    }

    // Toggles semantics on and off.
    semanticsEnabled = !semanticsEnabled;
    configureSession();
    messageSnackbarHelper.showMessageForLongDuration(
        this, "Semantic segmentation: " + semanticsEnabled + ". Tap again to toggle.");
  }

  /**
   * Shows a pop-up dialog that warns the user if semantics API is supported on the device. If the
   * device doesn't support the API the app will be finished.
   */
  private void showSemanticsApiDialog() {
    new AlertDialog.Builder(this)
        .setCancelable(false)
        .setTitle(R.string.title_semantics_api)
        .setMessage(R.string.message_semantics_not_supported)
        .setPositiveButton(
            R.string.quit_app,
            (DialogInterface dialog, int which) -> {
              finish();
            })
        .show();
  }

  private void setupSemanticsColorMapLegend() {
    semanticsColorLegend = findViewById(R.id.semantic_labels_legend);
    toggleSemanticsLegend = (TextView) findViewById(R.id.toggle_semantics_legend);
    Bitmap semanticsColorMap = null;
    try (InputStream inputStream = getAssets().open("textures/semantics_sv12_colormap.png")) {
      semanticsColorMap = BitmapFactory.decodeStream(inputStream);
    } catch (IOException e1) {
      Log.w(TAG, "Error loading semantics color map.");
    }

    if (semanticsColorMap == null) {
      semanticsColorLegend.setVisibility(View.GONE);
    } else {
      semanticLabelsRecyclerView = (RecyclerView) findViewById(R.id.semantic_labels_recycler_view);
      layoutManager = new LinearLayoutManager(this);
      semanticLabelsRecyclerView.setLayoutManager(layoutManager);
      semanticLabelsRecyclerView.scrollToPosition(0);

      adapter = new SemanticLabelAdapter();
      adapter.setupLabels(semanticsColorMap);
      semanticLabelsRecyclerView.setAdapter(adapter);
    }

    toggleSemanticsLegend.setOnClickListener(
        view -> {
          if (semanticLabelsRecyclerView.getVisibility() == View.VISIBLE) {
            resetToggleSemanticsLegend();
          } else {
            semanticLabelsRecyclerView.setVisibility(View.VISIBLE);
            toggleSemanticsLegend.setText(R.string.hide_semantics_legend);
          }
        });
  }

  private void resetToggleSemanticsLegend() {
    semanticLabelsRecyclerView.setVisibility(View.GONE);
    toggleSemanticsLegend.setText(R.string.show_semantics_legend);
  }

  private void updateSemanticLabelsFraction(Frame frame) {
    for (SemanticLabel label : SemanticLabel.values()) {
      float fraction;
      try {
        fraction = frame.getSemanticLabelFraction(label);
      } catch (NotYetAvailableException e) {
        fraction = 0.0f;
      }

      float finalFraction = fraction;
      runOnUiThread(() -> adapter.updateLabelFraction(label, finalFraction));
    }
  }
}
