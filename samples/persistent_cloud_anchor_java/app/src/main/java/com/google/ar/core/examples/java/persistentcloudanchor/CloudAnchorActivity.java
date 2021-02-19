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

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.SystemClock;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.TextView;
import android.widget.Toast;
import androidx.annotation.GuardedBy;
import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.DialogFragment;
import com.google.ar.core.Anchor;
import com.google.ar.core.Anchor.CloudAnchorState;
import com.google.ar.core.ArCoreApk;
import com.google.ar.core.Camera;
import com.google.ar.core.Config;
import com.google.ar.core.Config.CloudAnchorMode;
import com.google.ar.core.Frame;
import com.google.ar.core.HitResult;
import com.google.ar.core.Plane;
import com.google.ar.core.PointCloud;
import com.google.ar.core.Pose;
import com.google.ar.core.Session;
import com.google.ar.core.Session.FeatureMapQuality;
import com.google.ar.core.Trackable;
import com.google.ar.core.TrackingState;
import com.google.ar.core.examples.java.common.helpers.CameraPermissionHelper;
import com.google.ar.core.examples.java.common.helpers.DisplayRotationHelper;
import com.google.ar.core.examples.java.common.helpers.FullScreenHelper;
import com.google.ar.core.examples.java.common.helpers.TrackingStateHelper;
import com.google.ar.core.examples.java.common.rendering.BackgroundRenderer;
import com.google.ar.core.examples.java.common.rendering.ObjectRenderer;
import com.google.ar.core.examples.java.common.rendering.PlaneRenderer;
import com.google.ar.core.examples.java.common.rendering.PointCloudRenderer;
import com.google.ar.core.examples.java.persistentcloudanchor.PrivacyNoticeDialogFragment.HostResolveListener;
import com.google.ar.core.exceptions.CameraNotAvailableException;
import com.google.ar.core.exceptions.UnavailableApkTooOldException;
import com.google.ar.core.exceptions.UnavailableArcoreNotInstalledException;
import com.google.ar.core.exceptions.UnavailableSdkTooOldException;
import com.google.common.base.Preconditions;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Main Activity for the Persistent Cloud Anchor Sample.
 *
 * <p>This is a simple example that shows how to host and resolve anchors using ARCore Cloud Anchors
 * API calls. This app only has at most one anchor at a time, to focus more on the cloud aspect of
 * anchors.
 */
public class CloudAnchorActivity extends AppCompatActivity implements GLSurfaceView.Renderer {
  private static final String TAG = CloudAnchorActivity.class.getSimpleName();
  private static final String EXTRA_HOSTING_MODE = "persistentcloudanchor.hosting_mode";
  private static final String EXTRA_ANCHORS_TO_RESOLVE = "persistentcloudanchor.anchors_to_resolve";
  private static final String QUALITY_INSUFFICIENT_STRING = "INSUFFICIENT";
  private static final String QUALITY_SUFFICIENT_GOOD_STRING = "SUFFICIENT-GOOD";
  private static final String ALLOW_SHARE_IMAGES_KEY = "ALLOW_SHARE_IMAGES";
  protected static final String PREFERENCE_FILE_KEY = "CLOUD_ANCHOR_PREFERENCES";
  protected static final String HOSTED_ANCHOR_IDS = "anchor_ids";
  protected static final String HOSTED_ANCHOR_NAMES = "anchor_names";
  protected static final String HOSTED_ANCHOR_MINUTES = "anchor_minutes";
  protected static final double MIN_DISTANCE = 0.2f;
  protected static final double MAX_DISTANCE = 10.0f;

  static Intent newHostingIntent(Context packageContext) {
    Intent intent = new Intent(packageContext, CloudAnchorActivity.class);
    intent.putExtra(EXTRA_HOSTING_MODE, true);
    return intent;
  }

  static Intent newResolvingIntent(Context packageContext, ArrayList<String> anchorsToResolve) {
    Intent intent = new Intent(packageContext, CloudAnchorActivity.class);
    intent.putExtra(EXTRA_HOSTING_MODE, false);
    intent.putExtra(EXTRA_ANCHORS_TO_RESOLVE, anchorsToResolve);
    return intent;
  }

  private enum HostResolveMode {
    NONE,
    HOSTING,
    RESOLVING,
  }

  // Rendering. The Renderers are created here, and initialized when the GL surface is created.
  private GLSurfaceView surfaceView;
  private final BackgroundRenderer backgroundRenderer = new BackgroundRenderer();
  private final ObjectRenderer anchorObject = new ObjectRenderer();
  private final ObjectRenderer featureMapQualityBarObject = new ObjectRenderer();
  private final PlaneRenderer planeRenderer = new PlaneRenderer();
  private final PointCloudRenderer pointCloudRenderer = new PointCloudRenderer();

  private boolean installRequested;

  // Temporary matrices allocated here to reduce number of allocations for each frame.
  private final float[] anchorMatrix = new float[16];
  private final float[] viewMatrix = new float[16];
  private final float[] projectionMatrix = new float[16];
  private final float[] anchorTranslation = new float[4];

  // Locks needed for synchronization
  private final Object singleTapLock = new Object();
  private final Object anchorLock = new Object();

  // Tap handling and UI.
  private GestureDetector gestureDetector;
  private DisplayRotationHelper displayRotationHelper;
  private final TrackingStateHelper trackingStateHelper = new TrackingStateHelper(this);
  private TextView debugText;
  private TextView userMessageText;
  private SharedPreferences sharedPreferences;

  // Feature Map Quality Indicator UI
  private FeatureMapQualityUi featureMapQualityUi;
  private static final float QUALITY_THRESHOLD = 0.6f;
  private Pose anchorPose;
  private boolean hostedAnchor;
  private long lastEstimateTimestampMillis;

  @GuardedBy("singleTapLock")
  private MotionEvent queuedSingleTap;

  private Session session;

  @GuardedBy("anchorLock")
  private Anchor anchor;

  @GuardedBy("anchorLock")
  private List<Anchor> resolvedAnchors = new ArrayList<>();

  @GuardedBy("anchorLock")
  private List<String> unresolvedAnchorIds = new ArrayList<>();

  private CloudAnchorManager cloudAnchorManager;
  private HostResolveMode currentMode;

  private static void saveAnchorToStorage(
      String anchorId, String anchorNickname, SharedPreferences anchorPreferences) {
    String hostedAnchorIds = anchorPreferences.getString(HOSTED_ANCHOR_IDS, "");
    String hostedAnchorNames = anchorPreferences.getString(HOSTED_ANCHOR_NAMES, "");
    String hostedAnchorMinutes = anchorPreferences.getString(HOSTED_ANCHOR_MINUTES, "");
    hostedAnchorIds += anchorId + ";";
    hostedAnchorNames += anchorNickname + ";";
    hostedAnchorMinutes += TimeUnit.MILLISECONDS.toMinutes(System.currentTimeMillis()) + ";";
    anchorPreferences.edit().putString(HOSTED_ANCHOR_IDS, hostedAnchorIds).apply();
    anchorPreferences.edit().putString(HOSTED_ANCHOR_NAMES, hostedAnchorNames).apply();
    anchorPreferences.edit().putString(HOSTED_ANCHOR_MINUTES, hostedAnchorMinutes).apply();
  }

  private static int getNumStoredAnchors(SharedPreferences anchorPreferences) {
    String hostedAnchorIds = anchorPreferences.getString(CloudAnchorActivity.HOSTED_ANCHOR_IDS, "");
    return hostedAnchorIds.split(";", -1).length - 1;
  }

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    setContentView(R.layout.cloud_anchor);
    surfaceView = findViewById(R.id.surfaceview);
    displayRotationHelper = new DisplayRotationHelper(this);
    setUpTapListener();
    // Set up renderer.
    surfaceView.setPreserveEGLContextOnPause(true);
    surfaceView.setEGLContextClientVersion(2);
    surfaceView.setEGLConfigChooser(8, 8, 8, 8, 16, 0); // Alpha used for plane blending.
    surfaceView.setRenderer(this);
    surfaceView.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
    surfaceView.setWillNotDraw(false);
    installRequested = false;

    // Initialize UI components.
    debugText = findViewById(R.id.debug_message);
    userMessageText = findViewById(R.id.user_message);

    // Initialize Cloud Anchor variables.
    if (this.getIntent().getBooleanExtra(EXTRA_HOSTING_MODE, false)) {
      currentMode = HostResolveMode.HOSTING;
    } else {
      currentMode = HostResolveMode.RESOLVING;
    }
    showPrivacyDialog();
  }

  private void setUpTapListener() {
    gestureDetector =
        new GestureDetector(
            this,
            new GestureDetector.SimpleOnGestureListener() {
              @Override
              public boolean onSingleTapUp(MotionEvent e) {
                synchronized (singleTapLock) {
                  if (currentMode == HostResolveMode.HOSTING) {
                    queuedSingleTap = e;
                  }
                }
                return true;
              }

              @Override
              public boolean onDown(MotionEvent e) {
                return true;
              }
            });
    surfaceView.setOnTouchListener((v, event) -> gestureDetector.onTouchEvent(event));
  }

  private void showPrivacyDialog() {
    sharedPreferences = getSharedPreferences(PREFERENCE_FILE_KEY, Context.MODE_PRIVATE);

    if (currentMode == HostResolveMode.HOSTING) {
      if (!sharedPreferences.getBoolean(ALLOW_SHARE_IMAGES_KEY, false)) {
        showNoticeDialog(this::onPrivacyAcceptedForHost);
      } else {
        onPrivacyAcceptedForHost();
      }
    } else {
      if (!sharedPreferences.getBoolean(ALLOW_SHARE_IMAGES_KEY, false)) {
        showNoticeDialog(this::onPrivacyAcceptedForResolve);
      } else {
        onPrivacyAcceptedForResolve();
      }
    }
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
    if (sharedPreferences.getBoolean(ALLOW_SHARE_IMAGES_KEY, false)) {
      createSession();
    }
    surfaceView.onResume();
    displayRotationHelper.onResume();
  }

  private void createSession() {
    if (session == null) {
      Exception exception = null;
      int messageId = -1;
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
        session = new Session(this);
        cloudAnchorManager = new CloudAnchorManager(session);
      } catch (UnavailableArcoreNotInstalledException e) {
        messageId = R.string.arcore_unavailable;
        exception = e;
      } catch (UnavailableApkTooOldException e) {
        messageId = R.string.arcore_too_old;
        exception = e;
      } catch (UnavailableSdkTooOldException e) {
        messageId = R.string.arcore_sdk_too_old;
        exception = e;
      } catch (Exception e) {
        messageId = R.string.arcore_exception;
        exception = e;
      }

      if (exception != null) {
        userMessageText.setText(messageId);
        debugText.setText(messageId);
        Log.e(TAG, "Exception creating session", exception);
        return;
      }

      // Create default config and check if supported.
      Config config = new Config(session);
      config.setCloudAnchorMode(CloudAnchorMode.ENABLED);
      session.configure(config);
    }

    // Note that order matters - see the note in onPause(), the reverse applies here.
    try {
      session.resume();
    } catch (CameraNotAvailableException e) {
      userMessageText.setText(R.string.camera_unavailable);
      debugText.setText(R.string.camera_unavailable);
      session = null;
      cloudAnchorManager = null;
    }
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

  /**
   * Handles the most recent user tap.
   *
   * <p>We only ever handle one tap at a time, since this app only allows for a single anchor.
   *
   * @param frame the current AR frame
   * @param cameraTrackingState the current camera tracking state
   */
  private void handleTap(Frame frame, TrackingState cameraTrackingState) {
    // Handle taps. Handling only one tap per frame, as taps are usually low frequency
    // compared to frame rate.
    synchronized (singleTapLock) {
      synchronized (anchorLock) {
        // Only handle a tap if the anchor is currently null, the queued tap is non-null and the
        // camera is currently tracking.
        if (anchor == null
            && queuedSingleTap != null
            && cameraTrackingState == TrackingState.TRACKING) {
          Preconditions.checkState(
              currentMode == HostResolveMode.HOSTING,
              "We should only be creating an anchor in hosting mode.");
          for (HitResult hit : frame.hitTest(queuedSingleTap)) {
            if (shouldCreateAnchorWithHit(hit)) {
              debugText.setText(
                  getString(R.string.debug_hosting_save, QUALITY_INSUFFICIENT_STRING));
              // Create an anchor using a hit test with plane
              Anchor newAnchor = hit.createAnchor();
              Plane plane = (Plane) hit.getTrackable();
              if (plane.getType() == Plane.Type.VERTICAL) {
                featureMapQualityUi =
                    FeatureMapQualityUi.createVerticalFeatureMapQualityUi(
                        featureMapQualityBarObject);
              } else {
                featureMapQualityUi =
                    FeatureMapQualityUi.createHorizontalFeatureMapQualityUi(
                        featureMapQualityBarObject);
              }
              setNewAnchor(newAnchor);
              break; // Only handle the first valid hit.
            }
          }
        }
      }
      queuedSingleTap = null;
    }
  }

  /** Returns {@code true} if and only if the hit can be used to create an Anchor reliably. */
  private static boolean shouldCreateAnchorWithHit(HitResult hit) {
    Trackable trackable = hit.getTrackable();
    if (trackable instanceof Plane) {
      // Check if the hit was within the plane's polygon.
      return ((Plane) trackable).isPoseInPolygon(hit.getHitPose());
    }
    return false;
  }

  @Override
  public void onSurfaceCreated(GL10 gl, EGLConfig config) {
    GLES20.glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    // Prepare the rendering objects. This involves reading shaders, so may throw an IOException.
    try {
      // Create the texture and pass it to ARCore session to be filled during update().
      backgroundRenderer.createOnGlThread(this);
      planeRenderer.createOnGlThread(this, "models/trigrid.png");
      pointCloudRenderer.createOnGlThread(this);

      anchorObject.createOnGlThread(this, "models/anchor.obj", "models/anchor.png");
      anchorObject.setMaterialProperties(0.0f, 0.75f, 0.1f, 0.5f);

      featureMapQualityBarObject.createOnGlThread(
          this, "models/map_quality_bar.obj", "models/map_quality_bar.png");
      featureMapQualityBarObject.setMaterialProperties(0.0f, 2.0f, 0.02f, 0.5f);

    } catch (IOException ex) {
      Log.e(TAG, "Failed to read an asset file", ex);
    }
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
      session.setCameraTextureName(backgroundRenderer.getTextureId());

      // Obtain the current frame from ARSession. When the configuration is set to
      // UpdateMode.BLOCKING (it is by default), this will throttle the rendering to the
      // camera framerate.
      Frame frame = session.update();
      Camera camera = frame.getCamera();
      TrackingState cameraTrackingState = camera.getTrackingState();

      // Notify the cloudAnchorManager of all the updates.
      cloudAnchorManager.onUpdate();

      // Handle user input.
      handleTap(frame, cameraTrackingState);

      // If frame is ready, render camera preview image to the GL surface.
      backgroundRenderer.draw(frame);

      // Keep the screen unlocked while tracking, but allow it to lock when tracking stops.
      trackingStateHelper.updateKeepScreenOnFlag(camera.getTrackingState());

      // If not tracking, don't draw 3d objects.
      if (cameraTrackingState == TrackingState.PAUSED) {
        return;
      }

      // Get camera and projection matrices.
      camera.getViewMatrix(viewMatrix, 0);
      camera.getProjectionMatrix(projectionMatrix, 0, 0.1f, 100.0f);

      // Visualize tracked points.
      // Use try-with-resources to automatically release the point cloud.
      try (PointCloud pointCloud = frame.acquirePointCloud()) {
        pointCloudRenderer.update(pointCloud);
        pointCloudRenderer.draw(viewMatrix, projectionMatrix);
      }

      float[] colorCorrectionRgba = new float[4];
      float scaleFactor = 1.0f;
      frame.getLightEstimate().getColorCorrection(colorCorrectionRgba, 0);
      boolean shouldDrawFeatureMapQualityUi = false;
      synchronized (anchorLock) {
        for (Anchor resolvedAnchor : resolvedAnchors) {
          // Update the poses of resolved anchors that can be drawn and render them.
          if (resolvedAnchor != null
              && resolvedAnchor.getTrackingState() == TrackingState.TRACKING) {
            // Get the current pose of an Anchor in world space. The Anchor pose is updated
            // during calls to session.update() as ARCore refines its estimate of the world.
            anchorPose = resolvedAnchor.getPose();
            anchorPose.toMatrix(anchorMatrix, 0);
            // Update and draw the model and its shadow.
            drawAnchor(anchorMatrix, scaleFactor, colorCorrectionRgba);
          }
        }
        if (anchor == null) {
          // Visualize planes.
          planeRenderer.drawPlanes(
              session.getAllTrackables(Plane.class),
              camera.getDisplayOrientedPose(),
              projectionMatrix);
        }
        // Update the pose of the anchor (to be) hosted if it can be drawn and render the anchor.
        if (anchor != null && anchor.getTrackingState() == TrackingState.TRACKING) {
          anchorPose = anchor.getPose();
          anchorPose.toMatrix(anchorMatrix, 0);
          anchorPose.getTranslation(anchorTranslation, 0);
          anchorTranslation[3] = 1.0f;
          drawAnchor(anchorMatrix, scaleFactor, colorCorrectionRgba);

          if (!hostedAnchor && featureMapQualityUi != null) {
            shouldDrawFeatureMapQualityUi = true;
          }
        }
      }

      // Render the Feature Map Quality Indicator UI.
      // Adaptive UI is drawn here, using the values from the mapping quality API.
      if (shouldDrawFeatureMapQualityUi) {
        updateFeatureMapQualityUi(camera, colorCorrectionRgba);
      }

    } catch (Throwable t) {
      // Avoid crashing the application due to unhandled exceptions.
      Log.e(TAG, "Exception on the OpenGL thread", t);
    }
  }

  private void updateFeatureMapQualityUi(Camera camera, float[] colorCorrectionRgba) {
    Pose featureMapQualityUiPose = anchorPose.compose(featureMapQualityUi.getUiTransform());
    float[] cameraUiFrame =
        featureMapQualityUiPose.inverse().compose(camera.getPose()).getTranslation();
    double distance = Math.hypot(/*dx=*/ cameraUiFrame[0], /*dz=*/ cameraUiFrame[2]);
    runOnUiThread(
        () -> {
          if (distance < MIN_DISTANCE) {
            userMessageText.setText(R.string.too_close);
          } else if (distance > MAX_DISTANCE) {
            userMessageText.setText(R.string.too_far);
          } else {
            userMessageText.setText(R.string.hosting_save);
          }
        });

    long now = SystemClock.uptimeMillis();
    // Call estimateFeatureMapQualityForHosting() every 500ms.
    if (now - lastEstimateTimestampMillis > 500
        // featureMapQualityUi.updateQualityForViewpoint() calculates the angle (and intersected
        // quality bar) using the vector going from the phone to the anchor. If the person is
        // looking away from the anchor and we would incorrectly update the intersected angle with
        // the FeatureMapQuality from their current view. So we check isAnchorInView() here.
        && FeatureMapQualityUi.isAnchorInView(anchorTranslation, viewMatrix, projectionMatrix)) {
      lastEstimateTimestampMillis = now;
      // Update the FeatureMapQuality for the current camera viewpoint. Can pass in ANY valid camera
      // pose to estimateFeatureMapQualityForHosting(). Ideally, the pose should represent users’
      // expected perspectives.
      FeatureMapQuality currentQuality =
          session.estimateFeatureMapQualityForHosting(camera.getPose());
      featureMapQualityUi.updateQualityForViewpoint(cameraUiFrame, currentQuality);
      float averageQuality = featureMapQualityUi.computeOverallQuality();
      Log.i(TAG, "History of average mapping quality calls: " + averageQuality);

      if (averageQuality >= QUALITY_THRESHOLD) {
        // Host the anchor automatically if the FeatureMapQuality threshold is reached.
        Log.i(TAG, "FeatureMapQuality has reached SUFFICIENT-GOOD, triggering hostCloudAnchor()");
        synchronized (anchorLock) {
          hostedAnchor = true;
          cloudAnchorManager.hostCloudAnchor(anchor, new HostListener());
        }
        runOnUiThread(
            () -> {
              userMessageText.setText(R.string.hosting_processing);
              debugText.setText(R.string.debug_hosting_processing);
            });
      }
    }

    // Render the mapping quality UI.
    featureMapQualityUi.drawUi(anchorPose, viewMatrix, projectionMatrix, colorCorrectionRgba);
  }

  private void drawAnchor(float[] anchorMatrix, float scaleFactor, float[] colorCorrectionRgba) {
    anchorObject.updateModelMatrix(anchorMatrix, scaleFactor);
    anchorObject.draw(viewMatrix, projectionMatrix, colorCorrectionRgba);
  }

  /** Sets the new value of the current anchor. Detaches the old anchor, if it was non-null. */
  private void setNewAnchor(Anchor newAnchor) {
    synchronized (anchorLock) {
      if (anchor != null) {
        anchor.detach();
      }
      anchor = newAnchor;
    }
  }

  /** Adds a new anchor to the set of resolved anchors. */
  private void setAnchorAsResolved(Anchor newAnchor) {
    synchronized (anchorLock) {
      if (unresolvedAnchorIds.contains(newAnchor.getCloudAnchorId())) {
        resolvedAnchors.add(newAnchor);
        unresolvedAnchorIds.remove(newAnchor.getCloudAnchorId());
      }
    }
  }

  /** Callback function invoked when the privacy notice is accepted. */
  private void onPrivacyAcceptedForHost() {
    if (!sharedPreferences.edit().putBoolean(ALLOW_SHARE_IMAGES_KEY, true).commit()) {
      throw new AssertionError("Could not save the user preference to SharedPreferences!");
    }
    createSession();
    debugText.setText(R.string.debug_hosting_place_anchor);
    userMessageText.setText(R.string.hosting_place_anchor);
  }

  private void onPrivacyAcceptedForResolve() {
    if (!sharedPreferences.edit().putBoolean(ALLOW_SHARE_IMAGES_KEY, true).commit()) {
      throw new AssertionError("Could not save the user preference to SharedPreferences!");
    }
    createSession();
    ResolveListener resolveListener = new ResolveListener();
    synchronized (anchorLock) {
      unresolvedAnchorIds = getIntent().getStringArrayListExtra(EXTRA_ANCHORS_TO_RESOLVE);
      debugText.setText(getString(R.string.debug_resolving_processing, unresolvedAnchorIds.size()));
      // Encourage the user to look at a previously mapped area.
      userMessageText.setText(R.string.resolving_processing);
      Log.i(
          TAG,
          String.format(
              "Attempting to resolve %d anchor(s): %s",
              unresolvedAnchorIds.size(), unresolvedAnchorIds));
      for (String cloudAnchorId : unresolvedAnchorIds) {
        cloudAnchorManager.resolveCloudAnchor(cloudAnchorId, resolveListener);
      }
    }
  }

  /* Listens for a resolved anchor. */
  private final class ResolveListener implements CloudAnchorManager.CloudAnchorListener {

    @Override
    public void onComplete(Anchor anchor) {
      runOnUiThread(
          () -> {
            CloudAnchorState state = anchor.getCloudAnchorState();
            if (state.isError()) {
              Log.e(TAG, "Error hosting a cloud anchor, state " + state);
              userMessageText.setText(getString(R.string.resolving_error, state));
              return;
            }
            setAnchorAsResolved(anchor);
            userMessageText.setText(getString(R.string.resolving_success));
            synchronized (anchorLock) {
              if (unresolvedAnchorIds.isEmpty()) {
                debugText.setText(getString(R.string.debug_resolving_success));
              } else {
                Log.i(
                    TAG,
                    String.format(
                        "Attempting to resolve %d anchor(s): %s",
                        unresolvedAnchorIds.size(), unresolvedAnchorIds));
                debugText.setText(
                    getString(R.string.debug_resolving_processing, unresolvedAnchorIds.size()));
              }
            }
          });
    }
  }

  /* Listens for a hosted anchor. */
  private final class HostListener implements CloudAnchorManager.CloudAnchorListener {
    private String cloudAnchorId;

    @Override
    public void onComplete(Anchor anchor) {
      runOnUiThread(
          () -> {
            CloudAnchorState state = anchor.getCloudAnchorState();
            if (state.isError()) {
              Log.e(TAG, "Error hosting a cloud anchor, state " + state);
              userMessageText.setText(getString(R.string.hosting_error, state));
              return;
            }
            Preconditions.checkState(
                cloudAnchorId == null, "The cloud anchor ID cannot have been set before.");
            cloudAnchorId = anchor.getCloudAnchorId();
            setNewAnchor(anchor);
            Log.i(TAG, "Anchor " + cloudAnchorId + " created.");
            userMessageText.setText(getString(R.string.hosting_success));
            debugText.setText(getString(R.string.debug_hosting_success, cloudAnchorId));
            saveAnchorWithNickname();
          });
    }

    /** Callback function invoked when the user presses the OK button in the Save Anchor Dialog. */
    private void onAnchorNameEntered(String anchorNickname) {
      saveAnchorToStorage(cloudAnchorId, anchorNickname, sharedPreferences);
      userMessageText.setVisibility(View.GONE);
      debugText.setText(getString(R.string.debug_hosting_success, cloudAnchorId));
      Intent sendIntent = new Intent();
      sendIntent.setAction(Intent.ACTION_SEND);
      sendIntent.putExtra(Intent.EXTRA_TEXT, cloudAnchorId);
      sendIntent.setType("text/plain");
      Intent shareIntent = Intent.createChooser(sendIntent, null);
      startActivity(shareIntent);
    }

    private void saveAnchorWithNickname() {
      if (cloudAnchorId == null) {
        return;
      }
      HostDialogFragment hostDialogFragment = new HostDialogFragment();
      // Supply num input as an argument.
      Bundle args = new Bundle();
      args.putString(
          "nickname", getString(R.string.nickname_default, getNumStoredAnchors(sharedPreferences)));
      hostDialogFragment.setOkListener(this::onAnchorNameEntered);
      hostDialogFragment.setArguments(args);
      hostDialogFragment.show(getSupportFragmentManager(), "HostDialog");
    }
  }

  public void showNoticeDialog(HostResolveListener listener) {
    DialogFragment dialog = PrivacyNoticeDialogFragment.createDialog(listener);
    dialog.show(getSupportFragmentManager(), PrivacyNoticeDialogFragment.class.getName());
  }
}
