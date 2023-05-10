/*
 * Copyright 2022 Google LLC
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

package com.google.ar.core.examples.java.geospatial;

import android.content.Context;
import android.content.SharedPreferences;
import android.location.Location;
import android.opengl.GLSurfaceView;
import android.opengl.Matrix;
import android.os.Bundle;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.PopupMenu;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;
import androidx.annotation.GuardedBy;
import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.DialogFragment;
import com.google.android.gms.location.FusedLocationProviderClient;
import com.google.android.gms.location.LocationServices;
import com.google.android.gms.tasks.OnSuccessListener;
import com.google.ar.core.Anchor;
import com.google.ar.core.Anchor.RooftopAnchorState;
import com.google.ar.core.Anchor.TerrainAnchorState;
import com.google.ar.core.ArCoreApk;
import com.google.ar.core.Camera;
import com.google.ar.core.Config;
import com.google.ar.core.Earth;
import com.google.ar.core.Frame;
import com.google.ar.core.GeospatialPose;
import com.google.ar.core.HitResult;
import com.google.ar.core.Plane;
import com.google.ar.core.Point;
import com.google.ar.core.Point.OrientationMode;
import com.google.ar.core.PointCloud;
import com.google.ar.core.Pose;
import com.google.ar.core.ResolveAnchorOnRooftopFuture;
import com.google.ar.core.ResolveAnchorOnTerrainFuture;
import com.google.ar.core.Session;
import com.google.ar.core.StreetscapeGeometry;
import com.google.ar.core.Trackable;
import com.google.ar.core.TrackingState;
import com.google.ar.core.VpsAvailability;
import com.google.ar.core.VpsAvailabilityFuture;
import com.google.ar.core.examples.java.common.helpers.CameraPermissionHelper;
import com.google.ar.core.examples.java.common.helpers.DisplayRotationHelper;
import com.google.ar.core.examples.java.common.helpers.FullScreenHelper;
import com.google.ar.core.examples.java.common.helpers.LocationPermissionHelper;
import com.google.ar.core.examples.java.common.helpers.SnackbarHelper;
import com.google.ar.core.examples.java.common.helpers.TrackingStateHelper;
import com.google.ar.core.examples.java.common.samplerender.Framebuffer;
import com.google.ar.core.examples.java.common.samplerender.IndexBuffer;
import com.google.ar.core.examples.java.common.samplerender.Mesh;
import com.google.ar.core.examples.java.common.samplerender.SampleRender;
import com.google.ar.core.examples.java.common.samplerender.Shader;
import com.google.ar.core.examples.java.common.samplerender.Shader.BlendFactor;
import com.google.ar.core.examples.java.common.samplerender.Texture;
import com.google.ar.core.examples.java.common.samplerender.VertexBuffer;
import com.google.ar.core.examples.java.common.samplerender.arcore.BackgroundRenderer;
import com.google.ar.core.examples.java.common.samplerender.arcore.PlaneRenderer;
import com.google.ar.core.exceptions.CameraNotAvailableException;
import com.google.ar.core.exceptions.FineLocationPermissionNotGrantedException;
import com.google.ar.core.exceptions.GooglePlayServicesLocationLibraryNotLinkedException;
import com.google.ar.core.exceptions.UnavailableApkTooOldException;
import com.google.ar.core.exceptions.UnavailableArcoreNotInstalledException;
import com.google.ar.core.exceptions.UnavailableDeviceNotCompatibleException;
import com.google.ar.core.exceptions.UnavailableSdkTooOldException;
import com.google.ar.core.exceptions.UnavailableUserDeclinedInstallationException;
import com.google.ar.core.exceptions.UnsupportedConfigurationException;
import java.io.IOException;
import java.nio.FloatBuffer;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.TimeUnit;

/**
 * Main activity for the Geospatial API example.
 *
 * <p>This example shows how to use the Geospatial APIs. Once the device is localized, anchors can
 * be created at the device's geospatial location. Anchor locations are persisted across sessions
 * and will be recreated once localized.
 */
public class GeospatialActivity extends AppCompatActivity
    implements SampleRender.Renderer,
        VpsAvailabilityNoticeDialogFragment.NoticeDialogListener,
        PrivacyNoticeDialogFragment.NoticeDialogListener {

  private static final String TAG = GeospatialActivity.class.getSimpleName();

  private static final String SHARED_PREFERENCES_SAVED_ANCHORS = "SHARED_PREFERENCES_SAVED_ANCHORS";
  private static final String ALLOW_GEOSPATIAL_ACCESS_KEY = "ALLOW_GEOSPATIAL_ACCESS";
  private static final String ANCHOR_MODE = "ANCHOR_MODE";

  private static final float Z_NEAR = 0.1f;
  private static final float Z_FAR = 1000f;

  // The thresholds that are required for horizontal and orientation accuracies before entering into
  // the LOCALIZED state. Once the accuracies are equal or less than these values, the app will
  // allow the user to place anchors.
  private static final double LOCALIZING_HORIZONTAL_ACCURACY_THRESHOLD_METERS = 10;
  private static final double LOCALIZING_ORIENTATION_YAW_ACCURACY_THRESHOLD_DEGREES = 15;

  // Once in the LOCALIZED state, if either accuracies degrade beyond these amounts, the app will
  // revert back to the LOCALIZING state.
  private static final double LOCALIZED_HORIZONTAL_ACCURACY_HYSTERESIS_METERS = 10;
  private static final double LOCALIZED_ORIENTATION_YAW_ACCURACY_HYSTERESIS_DEGREES = 10;

  private static final int LOCALIZING_TIMEOUT_SECONDS = 180;
  private static final int MAXIMUM_ANCHORS = 20;
  private static final long DURATION_FOR_NO_TERRAIN_ANCHOR_RESULT_MS = 10000;

  // Rendering. The Renderers are created here, and initialized when the GL surface is created.
  private GLSurfaceView surfaceView;

  private boolean installRequested;
  private Integer clearedAnchorsAmount = null;

  /** Timer to keep track of how much time has passed since localizing has started. */
  private long localizingStartTimestamp;
  /** Deadline for showing resolving terrain anchors no result yet message. */
  private long deadlineForMessageMillis;

  enum State {
    /** The Geospatial API has not yet been initialized. */
    UNINITIALIZED,
    /** The Geospatial API is not supported. */
    UNSUPPORTED,
    /** The Geospatial API has encountered an unrecoverable error. */
    EARTH_STATE_ERROR,
    /** The Session has started, but {@link Earth} isn't {@link TrackingState.TRACKING} yet. */
    PRETRACKING,
    /**
     * {@link Earth} is {@link TrackingState.TRACKING}, but the desired positioning confidence
     * hasn't been reached yet.
     */
    LOCALIZING,
    /** The desired positioning confidence wasn't reached in time. */
    LOCALIZING_FAILED,
    /**
     * {@link Earth} is {@link TrackingState.TRACKING} and the desired positioning confidence has
     * been reached.
     */
    LOCALIZED
  }

  private State state = State.UNINITIALIZED;

  enum AnchorType {
    // Set WGS84 anchor.
    GEOSPATIAL,
    // Set Terrain anchor.
    TERRAIN,
    // Set Rooftop anchor.
    ROOFTOP
  }

  private AnchorType anchorType = AnchorType.GEOSPATIAL;

  private Session session;
  private final SnackbarHelper messageSnackbarHelper = new SnackbarHelper();
  private DisplayRotationHelper displayRotationHelper;
  private final TrackingStateHelper trackingStateHelper = new TrackingStateHelper(this);
  private SampleRender render;
  private SharedPreferences sharedPreferences;

  private String lastStatusText;
  private TextView geospatialPoseTextView;
  private TextView statusTextView;
  private TextView tapScreenTextView;
  private Button setAnchorButton;
  private Button clearAnchorsButton;
  private Switch streetscapeGeometrySwitch;

  private PlaneRenderer planeRenderer;
  private BackgroundRenderer backgroundRenderer;
  private Framebuffer virtualSceneFramebuffer;
  private boolean hasSetTextureNames = false;
  // Set rendering Streetscape Geometry.
  private boolean isRenderStreetscapeGeometry = false;

  // Virtual object (ARCore geospatial)
  private Mesh virtualObjectMesh;
  private Shader geospatialAnchorVirtualObjectShader;
  // Virtual object (ARCore geospatial terrain)
  private Shader terrainAnchorVirtualObjectShader;

  private final Object anchorsLock = new Object();

  @GuardedBy("anchorsLock")
  private final List<Anchor> anchors = new ArrayList<>();

  private final Set<Anchor> terrainAnchors = new HashSet<>();
  private final Set<Anchor> rooftopAnchors = new HashSet<>();

  // Temporary matrix allocated here to reduce number of allocations for each frame.
  private final float[] modelMatrix = new float[16];
  private final float[] viewMatrix = new float[16];
  private final float[] projectionMatrix = new float[16];
  private final float[] modelViewMatrix = new float[16]; // view x model
  private final float[] modelViewProjectionMatrix = new float[16]; // projection x view x model

  private final float[] identityQuaternion = {0, 0, 0, 1};

  // Locks needed for synchronization
  private final Object singleTapLock = new Object();

  @GuardedBy("singleTapLock")
  private MotionEvent queuedSingleTap;
  // Tap handling and UI.
  private GestureDetector gestureDetector;

  // Point Cloud
  private VertexBuffer pointCloudVertexBuffer;
  private Mesh pointCloudMesh;
  private Shader pointCloudShader;
  // Keep track of the last point cloud rendered to avoid updating the VBO if point cloud
  // was not changed.  Do this using the timestamp since we can't compare PointCloud objects.
  private long lastPointCloudTimestamp = 0;

  // Provides device location.
  private FusedLocationProviderClient fusedLocationClient;

  // Streetscape geometry.
  private final ArrayList<float[]> wallsColor = new ArrayList<float[]>();

  private Shader streetscapeGeometryTerrainShader;
  private Shader streetscapeGeometryBuildingShader;
  // A set of planes representing building outlines and floors.
  private final Map<StreetscapeGeometry, Mesh> streetscapeGeometryToMeshes = new HashMap<>();

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    sharedPreferences = getPreferences(Context.MODE_PRIVATE);

    setContentView(R.layout.activity_main);
    surfaceView = findViewById(R.id.surfaceview);
    geospatialPoseTextView = findViewById(R.id.geospatial_pose_view);
    statusTextView = findViewById(R.id.status_text_view);
    tapScreenTextView = findViewById(R.id.tap_screen_text_view);
    setAnchorButton = findViewById(R.id.set_anchor_button);
    clearAnchorsButton = findViewById(R.id.clear_anchors_button);

    setAnchorButton.setOnClickListener(
        new View.OnClickListener() {
          @Override
          public void onClick(View v) {
            PopupMenu popup = new PopupMenu(GeospatialActivity.this, v);
            popup.setOnMenuItemClickListener(GeospatialActivity.this::settingsMenuClick);
            popup.inflate(R.menu.setting_menu);
            popup.show();
            popup
                .getMenu()
                .findItem(sharedPreferences.getInt(ANCHOR_MODE, R.id.geospatial))
                .setChecked(true);
          }
        });

    clearAnchorsButton.setOnClickListener(view -> handleClearAnchorsButton());

    streetscapeGeometrySwitch = findViewById(R.id.streetscape_geometry_switch);
    // Initial terrain anchor mode is DISABLED.
    streetscapeGeometrySwitch.setChecked(false);
    streetscapeGeometrySwitch.setOnCheckedChangeListener(this::onRenderStreetscapeGeometryChanged);

    displayRotationHelper = new DisplayRotationHelper(/* activity= */ this);

    // Set up renderer.
    render = new SampleRender(surfaceView, this, getAssets());

    installRequested = false;
    clearedAnchorsAmount = null;

    // Set up touch listener.
    gestureDetector =
        new GestureDetector(
            this,
            new GestureDetector.SimpleOnGestureListener() {
              @Override
              public boolean onSingleTapUp(MotionEvent e) {
                synchronized (singleTapLock) {
                  queuedSingleTap = e;
                }
                return true;
              }

              @Override
              public boolean onDown(MotionEvent e) {
                return true;
              }
            });
    surfaceView.setOnTouchListener((v, event) -> gestureDetector.onTouchEvent(event));
    fusedLocationClient = LocationServices.getFusedLocationProviderClient(/* context= */ this);
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
    if (sharedPreferences.getBoolean(ALLOW_GEOSPATIAL_ACCESS_KEY, /* defValue= */ false)) {
      createSession();
    } else {
      showPrivacyNoticeDialog();
    }

    surfaceView.onResume();
    displayRotationHelper.onResume();
  }

  private void showPrivacyNoticeDialog() {
    DialogFragment dialog = PrivacyNoticeDialogFragment.createDialog();
    dialog.show(getSupportFragmentManager(), PrivacyNoticeDialogFragment.class.getName());
  }

  private void createSession() {
    Exception exception = null;
    String message = null;
    if (session == null) {

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
        if (!LocationPermissionHelper.hasFineLocationPermission(this)) {
          LocationPermissionHelper.requestFineLocationPermission(this);
          return;
        }

        // Create the session.
        // Plane finding mode is default on, which will help the dynamic alignment of terrain
        // anchors on ground.
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
    // Check VPS availability before configure and resume session.
    if (session != null) {
      getLastLocation();
    }

    // Note that order matters - see the note in onPause(), the reverse applies here.
    try {
      configureSession();
      // To record a live camera session for later playback, call
      // `session.startRecording(recordingConfig)` at anytime. To playback a previously recorded AR
      // session instead of using the live camera feed, call
      // `session.setPlaybackDatasetUri(Uri)` before calling `session.resume()`. To
      // learn more about recording and playback, see:
      // https://developers.google.com/ar/develop/java/recording-and-playback
      session.resume();
    } catch (CameraNotAvailableException e) {
      message = "Camera not available. Try restarting the app.";
      exception = e;
    } catch (GooglePlayServicesLocationLibraryNotLinkedException e) {
      message = "Google Play Services location library not linked or obfuscated with Proguard.";
      exception = e;
    } catch (FineLocationPermissionNotGrantedException e) {
      message = "The Android permission ACCESS_FINE_LOCATION was not granted.";
      exception = e;
    } catch (UnsupportedConfigurationException e) {
      message = "This device does not support GeospatialMode.ENABLED.";
      exception = e;
    } catch (SecurityException e) {
      message = "Camera failure or the internet permission has not been granted.";
      exception = e;
    }

    if (message != null) {
      session = null;
      messageSnackbarHelper.showError(this, message);
      Log.e(TAG, "Exception configuring and resuming the session", exception);
      return;
    }
  }

  private void getLastLocation() {
    try {
      fusedLocationClient
          .getLastLocation()
          .addOnSuccessListener(
              new OnSuccessListener<Location>() {
                @Override
                public void onSuccess(Location location) {
                  double latitude = 0;
                  double longitude = 0;
                  if (location != null) {
                    latitude = location.getLatitude();
                    longitude = location.getLongitude();
                  } else {
                    Log.e(TAG, "Error location is null");
                  }
                  checkVpsAvailability(latitude, longitude);
                }
              });
    } catch (SecurityException e) {
      Log.e(TAG, "No location permissions granted by User!");
    }
  }

  private void checkVpsAvailability(double latitude, double longitude) {
    final VpsAvailabilityFuture future =
        session.checkVpsAvailabilityAsync(
            latitude,
            longitude,
            availability -> {
              if (availability != VpsAvailability.AVAILABLE) {
                showVpsNotAvailabilityNoticeDialog();
              }
            });
  }

  private void showVpsNotAvailabilityNoticeDialog() {
    DialogFragment dialog = VpsAvailabilityNoticeDialogFragment.createDialog();
    dialog.show(getSupportFragmentManager(), VpsAvailabilityNoticeDialogFragment.class.getName());
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
      // Use toast instead of snackbar here since the activity will exit.
      Toast.makeText(this, "Camera permission is needed to run this application", Toast.LENGTH_LONG)
          .show();
      if (!CameraPermissionHelper.shouldShowRequestPermissionRationale(this)) {
        // Permission denied with checking "Do not ask again".
        CameraPermissionHelper.launchPermissionSettings(this);
      }
      finish();
    }
    // Check if this result pertains to the location permission.
    if (LocationPermissionHelper.hasFineLocationPermissionsResponseInResult(permissions)
        && !LocationPermissionHelper.hasFineLocationPermission(this)) {
      // Use toast instead of snackbar here since the activity will exit.
      Toast.makeText(
              this,
              "Precise location permission is needed to run this application",
              Toast.LENGTH_LONG)
          .show();
      if (!LocationPermissionHelper.shouldShowRequestPermissionRationale(this)) {
        // Permission denied with checking "Do not ask again".
        LocationPermissionHelper.launchPermissionSettings(this);
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
    try {
      planeRenderer = new PlaneRenderer(render);
      backgroundRenderer = new BackgroundRenderer(render);
      virtualSceneFramebuffer = new Framebuffer(render, /* width= */ 1, /* height= */ 1);

      // Virtual object to render (ARCore geospatial)
      Texture virtualObjectTexture =
          Texture.createFromAsset(
              render,
              "models/spatial_marker_baked.png",
              Texture.WrapMode.CLAMP_TO_EDGE,
              Texture.ColorFormat.SRGB);

      virtualObjectMesh = Mesh.createFromAsset(render, "models/geospatial_marker.obj");
      geospatialAnchorVirtualObjectShader =
          Shader.createFromAssets(
                  render,
                  "shaders/ar_unlit_object.vert",
                  "shaders/ar_unlit_object.frag",
                  /* defines= */ null)
              .setTexture("u_Texture", virtualObjectTexture);

      // Virtual object to render (Terrain anchor marker)
      Texture terrainAnchorVirtualObjectTexture =
          Texture.createFromAsset(
              render,
              "models/spatial_marker_yellow.png",
              Texture.WrapMode.CLAMP_TO_EDGE,
              Texture.ColorFormat.SRGB);
      terrainAnchorVirtualObjectShader =
          Shader.createFromAssets(
                  render,
                  "shaders/ar_unlit_object.vert",
                  "shaders/ar_unlit_object.frag",
                  /* defines= */ null)
              .setTexture("u_Texture", terrainAnchorVirtualObjectTexture);

      backgroundRenderer.setUseDepthVisualization(render, false);
      backgroundRenderer.setUseOcclusion(render, false);

      // Point cloud
      pointCloudShader =
          Shader.createFromAssets(
                  render,
                  "shaders/point_cloud.vert",
                  "shaders/point_cloud.frag",
                  /* defines= */ null)
              .setVec4(
                  "u_Color", new float[] {31.0f / 255.0f, 188.0f / 255.0f, 210.0f / 255.0f, 1.0f})
              .setFloat("u_PointSize", 5.0f);
      // four entries per vertex: X, Y, Z, confidence
      pointCloudVertexBuffer =
          new VertexBuffer(render, /* numberOfEntriesPerVertex= */ 4, /* entries= */ null);
      final VertexBuffer[] pointCloudVertexBuffers = {pointCloudVertexBuffer};
      pointCloudMesh =
          new Mesh(
              render, Mesh.PrimitiveMode.POINTS, /* indexBuffer= */ null, pointCloudVertexBuffers);

      streetscapeGeometryBuildingShader =
          Shader.createFromAssets(
                  render,
                  "shaders/streetscape_geometry.vert",
                  "shaders/streetscape_geometry.frag",
                  /* defines= */ null)
              .setBlend(
                  BlendFactor.DST_ALPHA, // RGB (src)
                  BlendFactor.ONE); // ALPHA (dest)

      streetscapeGeometryTerrainShader =
          Shader.createFromAssets(
                  render,
                  "shaders/streetscape_geometry.vert",
                  "shaders/streetscape_geometry.frag",
                  /* defines= */ null)
              .setBlend(
                  BlendFactor.DST_ALPHA, // RGB (src)
                  BlendFactor.ONE); // ALPHA (dest)
      wallsColor.add(new float[] {0.5f, 0.0f, 0.5f, 0.3f});
      wallsColor.add(new float[] {0.5f, 0.5f, 0.0f, 0.3f});
      wallsColor.add(new float[] {0.0f, 0.5f, 0.5f, 0.3f});
    } catch (IOException e) {
      Log.e(TAG, "Failed to read a required asset file", e);
      messageSnackbarHelper.showError(this, "Failed to read a required asset file: " + e);
    }
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
    updateStreetscapeGeometries(session.getAllTrackables(StreetscapeGeometry.class));

    // Obtain the current frame from ARSession. When the configuration is set to
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

    // BackgroundRenderer.updateDisplayGeometry must be called every frame to update the coordinates
    // used to draw the background camera image.
    backgroundRenderer.updateDisplayGeometry(frame);

    // Keep the screen unlocked while tracking, but allow it to lock when tracking stops.
    trackingStateHelper.updateKeepScreenOnFlag(camera.getTrackingState());

    Earth earth = session.getEarth();
    if (earth != null) {
      updateGeospatialState(earth);
    }

    // Show a message based on whether tracking has failed, if planes are detected, and if the user
    // has placed any objects.
    String message = null;
    switch (state) {
      case UNINITIALIZED:
        break;
      case UNSUPPORTED:
        message = getResources().getString(R.string.status_unsupported);
        break;
      case PRETRACKING:
        message = getResources().getString(R.string.status_pretracking);
        break;
      case EARTH_STATE_ERROR:
        message = getResources().getString(R.string.status_earth_state_error);
        break;
      case LOCALIZING:
        message = getResources().getString(R.string.status_localize_hint);
        break;
      case LOCALIZING_FAILED:
        message = getResources().getString(R.string.status_localize_timeout);
        break;
      case LOCALIZED:
        if (lastStatusText.equals(getResources().getString(R.string.status_localize_hint))) {
          message = getResources().getString(R.string.status_localize_complete);
        }
        break;
    }

    if (message != null && lastStatusText != message) {
      lastStatusText = message;
      runOnUiThread(
          () -> {
            statusTextView.setVisibility(View.VISIBLE);
            statusTextView.setText(lastStatusText);
          });
    }
    synchronized (anchorsLock) {
      if (anchors.size() >= MAXIMUM_ANCHORS) {
        runOnUiThread(
            () -> {
              setAnchorButton.setVisibility(View.INVISIBLE);
              tapScreenTextView.setVisibility(View.INVISIBLE);
            });
      }
    }

    // Handle user input.
    handleTap(frame, camera.getTrackingState());

    // -- Draw background

    if (frame.getTimestamp() != 0) {
      // Suppress rendering if the camera did not produce the first frame yet. This is to avoid
      // drawing possible leftover data from previous sessions if the texture is reused.
      backgroundRenderer.drawBackground(render);
    }

    // If not tracking, don't draw 3D objects.
    if (camera.getTrackingState() != TrackingState.TRACKING || state != State.LOCALIZED) {
      return;
    }

    // -- Draw virtual objects

    // Get projection matrix.
    camera.getProjectionMatrix(projectionMatrix, 0, Z_NEAR, Z_FAR);

    // Get camera matrix and draw.
    camera.getViewMatrix(viewMatrix, 0);

    // Visualize tracked points.
    // Use try-with-resources to automatically release the point cloud.
    try (PointCloud pointCloud = frame.acquirePointCloud()) {
      if (pointCloud.getTimestamp() > lastPointCloudTimestamp) {
        pointCloudVertexBuffer.set(pointCloud.getPoints());
        lastPointCloudTimestamp = pointCloud.getTimestamp();
      }
      Matrix.multiplyMM(modelViewProjectionMatrix, 0, projectionMatrix, 0, viewMatrix, 0);
      pointCloudShader.setMat4("u_ModelViewProjection", modelViewProjectionMatrix);
      render.draw(pointCloudMesh, pointCloudShader);
    }

    // Visualize planes.
    planeRenderer.drawPlanes(
        render,
        session.getAllTrackables(Plane.class),
        camera.getDisplayOrientedPose(),
        projectionMatrix);

    // Visualize anchors created by touch.
    render.clear(virtualSceneFramebuffer, 0f, 0f, 0f, 0f);

    // -- Draw Streetscape Geometries.
    if (isRenderStreetscapeGeometry) {
      int index = 0;
      for (Map.Entry<StreetscapeGeometry, Mesh> set : streetscapeGeometryToMeshes.entrySet()) {
        StreetscapeGeometry streetscapeGeometry = set.getKey();
        if (streetscapeGeometry.getTrackingState() != TrackingState.TRACKING) {
          continue;
        }
        Mesh mesh = set.getValue();
        Pose pose = streetscapeGeometry.getMeshPose();
        pose.toMatrix(modelMatrix, 0);

        // Calculate model/view/projection matrices
        Matrix.multiplyMM(modelViewMatrix, 0, viewMatrix, 0, modelMatrix, 0);
        Matrix.multiplyMM(modelViewProjectionMatrix, 0, projectionMatrix, 0, modelViewMatrix, 0);

        if (streetscapeGeometry.getType() == StreetscapeGeometry.Type.BUILDING) {
          float[] color = wallsColor.get(index % wallsColor.size());
          index += 1;
          streetscapeGeometryBuildingShader
              .setVec4(
                  "u_Color",
                  new float[] {/* r= */ color[0], /* g= */ color[1], /* b= */ color[2], color[3]})
              .setMat4("u_ModelViewProjection", modelViewProjectionMatrix);
          render.draw(mesh, streetscapeGeometryBuildingShader);
        } else if (streetscapeGeometry.getType() == StreetscapeGeometry.Type.TERRAIN) {
          streetscapeGeometryTerrainShader
              .setVec4("u_Color", new float[] {/* r= */ 0f, /* g= */ .5f, /* b= */ 0f, 0.3f})
              .setMat4("u_ModelViewProjection", modelViewProjectionMatrix);
          render.draw(mesh, streetscapeGeometryTerrainShader);
        }
      }
    }
    render.clear(virtualSceneFramebuffer, 0f, 0f, 0f, 0f);
    synchronized (anchorsLock) {
      for (Anchor anchor : anchors) {
        // Get the current pose of an Anchor in world space. The Anchor pose is updated
        // during calls to session.update() as ARCore refines its estimate of the world.
        // Only render resolved Terrain & Rooftop anchors and Geospatial anchors.
        if (anchor.getTrackingState() != TrackingState.TRACKING) {
          continue;
        }
        anchor.getPose().toMatrix(modelMatrix, 0);
        float[] scaleMatrix = new float[16];
        Matrix.setIdentityM(scaleMatrix, 0);
        float scale = getScale(anchor.getPose(), camera.getDisplayOrientedPose());
        scaleMatrix[0] = scale;
        scaleMatrix[5] = scale;
        scaleMatrix[10] = scale;
        Matrix.multiplyMM(modelMatrix, 0, modelMatrix, 0, scaleMatrix, 0);
        // Rotate the virtual object 180 degrees around the Y axis to make the object face the GL
        // camera -Z axis, since camera Z axis faces toward users.
        float[] rotationMatrix = new float[16];
        Matrix.setRotateM(rotationMatrix, 0, 180, 0.0f, 1.0f, 0.0f);
        float[] rotationModelMatrix = new float[16];
        Matrix.multiplyMM(rotationModelMatrix, 0, modelMatrix, 0, rotationMatrix, 0);
        // Calculate model/view/projection matrices
        Matrix.multiplyMM(modelViewMatrix, 0, viewMatrix, 0, rotationModelMatrix, 0);
        Matrix.multiplyMM(modelViewProjectionMatrix, 0, projectionMatrix, 0, modelViewMatrix, 0);

        // Update shader properties and draw
        if (terrainAnchors.contains(anchor) || rooftopAnchors.contains(anchor)) {
          terrainAnchorVirtualObjectShader.setMat4(
              "u_ModelViewProjection", modelViewProjectionMatrix);

          render.draw(virtualObjectMesh, terrainAnchorVirtualObjectShader, virtualSceneFramebuffer);
        } else {
          geospatialAnchorVirtualObjectShader.setMat4(
              "u_ModelViewProjection", modelViewProjectionMatrix);
          render.draw(
              virtualObjectMesh, geospatialAnchorVirtualObjectShader, virtualSceneFramebuffer);
        }
      }
      if (anchors.size() > 0) {
        String anchorMessage =
            getResources()
                .getQuantityString(
                    R.plurals.status_anchors_set, anchors.size(), anchors.size(), MAXIMUM_ANCHORS);
        runOnUiThread(
            () -> {
              statusTextView.setVisibility(View.VISIBLE);
              statusTextView.setText(anchorMessage);
            });
      }
    }

    // Compose the virtual scene with the background.
    backgroundRenderer.drawVirtualScene(render, virtualSceneFramebuffer, Z_NEAR, Z_FAR);
  }

  /**
   * Updates all the StreetscapeGeometries. Existing StreetscapeGeometries will have pose updated,
   * and non-existing StreetscapeGeometries will be removed from the scene.
   */
  private void updateStreetscapeGeometries(Collection<StreetscapeGeometry> streetscapeGeometries) {
    for (StreetscapeGeometry streetscapeGeometry : streetscapeGeometries) {
      // If the Streetscape Geometry node is already added to the scene, then we'll simply update
      // the pose.
      if (streetscapeGeometryToMeshes.containsKey(streetscapeGeometry)) {
      } else {
        // Otherwise, we create a StreetscapeGeometry mesh and add it to the scene.
        Mesh mesh = getSampleRenderMesh(streetscapeGeometry);
        streetscapeGeometryToMeshes.put(streetscapeGeometry, mesh);
      }
    }
  }

  private Mesh getSampleRenderMesh(StreetscapeGeometry streetscapeGeometry) {
    FloatBuffer streetscapeGeometryBuffer = streetscapeGeometry.getMesh().getVertexList();
    streetscapeGeometryBuffer.rewind();
    VertexBuffer meshVertexBuffer =
        new VertexBuffer(
            render, /* numberOfEntriesPerVertex= */ 3, /* entries= */ streetscapeGeometryBuffer);
    IndexBuffer meshIndexBuffer =
        new IndexBuffer(render, streetscapeGeometry.getMesh().getIndexList());
    final VertexBuffer[] meshVertexBuffers = {meshVertexBuffer};
    return new Mesh(
        render,
        Mesh.PrimitiveMode.TRIANGLES,
        /* indexBuffer= */ meshIndexBuffer,
        meshVertexBuffers);
  }

  /** Configures the session with feature settings. */
  private void configureSession() {
    // Earth mode may not be supported on this device due to insufficient sensor quality.
    if (!session.isGeospatialModeSupported(Config.GeospatialMode.ENABLED)) {
      state = State.UNSUPPORTED;
      return;
    }

    Config config = session.getConfig();
    config =
        config
            .setGeospatialMode(Config.GeospatialMode.ENABLED)
            .setStreetscapeGeometryMode(Config.StreetscapeGeometryMode.ENABLED);
    session.configure(config);
    state = State.PRETRACKING;
    localizingStartTimestamp = System.currentTimeMillis();
  }

  /** Change behavior depending on the current {@link State} of the application. */
  private void updateGeospatialState(Earth earth) {
    if (earth.getEarthState() != Earth.EarthState.ENABLED) {
      state = State.EARTH_STATE_ERROR;
      return;
    }
    if (earth.getTrackingState() != TrackingState.TRACKING) {
      state = State.PRETRACKING;
      return;
    }
    if (state == State.PRETRACKING) {
      updatePretrackingState(earth);
    } else if (state == State.LOCALIZING) {
      updateLocalizingState(earth);
    } else if (state == State.LOCALIZED) {
      updateLocalizedState(earth);
    }
  }

  /**
   * Handles the updating for {@link State.PRETRACKING}. In this state, wait for {@link Earth} to
   * have {@link TrackingState.TRACKING}. If it hasn't been enabled by now, then we've encountered
   * an unrecoverable {@link State.EARTH_STATE_ERROR}.
   */
  private void updatePretrackingState(Earth earth) {
    if (earth.getTrackingState() == TrackingState.TRACKING) {
      state = State.LOCALIZING;
      return;
    }

    runOnUiThread(() -> geospatialPoseTextView.setText(R.string.geospatial_pose_not_tracking));
  }

  /**
   * Handles the updating for {@link State.LOCALIZING}. In this state, wait for the horizontal and
   * orientation threshold to improve until it reaches your threshold.
   *
   * <p>If it takes too long for the threshold to be reached, this could mean that GPS data isn't
   * accurate enough, or that the user is in an area that can't be localized with StreetView.
   */
  private void updateLocalizingState(Earth earth) {
    GeospatialPose geospatialPose = earth.getCameraGeospatialPose();
    if (geospatialPose.getHorizontalAccuracy() <= LOCALIZING_HORIZONTAL_ACCURACY_THRESHOLD_METERS
        && geospatialPose.getOrientationYawAccuracy()
            <= LOCALIZING_ORIENTATION_YAW_ACCURACY_THRESHOLD_DEGREES) {
      state = State.LOCALIZED;
      synchronized (anchorsLock) {
        final int anchorNum = anchors.size();
        if (anchorNum == 0) {
          createAnchorFromSharedPreferences(earth);
        }
        if (anchorNum < MAXIMUM_ANCHORS) {
          runOnUiThread(
              () -> {
                setAnchorButton.setVisibility(View.VISIBLE);
                tapScreenTextView.setVisibility(View.VISIBLE);
                if (anchorNum > 0) {
                  clearAnchorsButton.setVisibility(View.VISIBLE);
                }
              });
        }
      }
      return;
    }

    if (TimeUnit.MILLISECONDS.toSeconds(System.currentTimeMillis() - localizingStartTimestamp)
        > LOCALIZING_TIMEOUT_SECONDS) {
      state = State.LOCALIZING_FAILED;
      return;
    }

    updateGeospatialPoseText(geospatialPose);
  }

  /**
   * Handles the updating for {@link State.LOCALIZED}. In this state, check the accuracy for
   * degradation and return to {@link State.LOCALIZING} if the position accuracies have dropped too
   * low.
   */
  private void updateLocalizedState(Earth earth) {
    GeospatialPose geospatialPose = earth.getCameraGeospatialPose();
    // Check if either accuracy has degraded to the point we should enter back into the LOCALIZING
    // state.
    if (geospatialPose.getHorizontalAccuracy()
            > LOCALIZING_HORIZONTAL_ACCURACY_THRESHOLD_METERS
                + LOCALIZED_HORIZONTAL_ACCURACY_HYSTERESIS_METERS
        || geospatialPose.getOrientationYawAccuracy()
            > LOCALIZING_ORIENTATION_YAW_ACCURACY_THRESHOLD_DEGREES
                + LOCALIZED_ORIENTATION_YAW_ACCURACY_HYSTERESIS_DEGREES) {
      // Accuracies have degenerated, return to the localizing state.
      state = State.LOCALIZING;
      localizingStartTimestamp = System.currentTimeMillis();
      runOnUiThread(
          () -> {
            setAnchorButton.setVisibility(View.INVISIBLE);
            tapScreenTextView.setVisibility(View.INVISIBLE);
            clearAnchorsButton.setVisibility(View.INVISIBLE);
          });
      return;
    }

    updateGeospatialPoseText(geospatialPose);
  }

  private void updateGeospatialPoseText(GeospatialPose geospatialPose) {
    float[] quaternion = geospatialPose.getEastUpSouthQuaternion();
    String poseText =
        getResources()
            .getString(
                R.string.geospatial_pose,
                geospatialPose.getLatitude(),
                geospatialPose.getLongitude(),
                geospatialPose.getHorizontalAccuracy(),
                geospatialPose.getAltitude(),
                geospatialPose.getVerticalAccuracy(),
                quaternion[0],
                quaternion[1],
                quaternion[2],
                quaternion[3],
                geospatialPose.getOrientationYawAccuracy());
    runOnUiThread(
        () -> {
          geospatialPoseTextView.setText(poseText);
        });
  }

  // Return the scale in range [1, 2] after mapping a distance between camera and anchor to [2, 20].
  private float getScale(Pose anchorPose, Pose cameraPose) {
    double distance =
        Math.sqrt(
            Math.pow(anchorPose.tx() - cameraPose.tx(), 2.0)
                + Math.pow(anchorPose.ty() - cameraPose.ty(), 2.0)
                + Math.pow(anchorPose.tz() - cameraPose.tz(), 2.0));
    double mapDistance = Math.min(Math.max(2, distance), 20);
    return (float) (mapDistance - 2) / (20 - 2) + 1;
  }

  /**
   * Handles the button that creates an anchor.
   *
   * <p>Ensure Earth is in the proper state, then create the anchor. Persist the parameters used to
   * create the anchors so that the anchors will be loaded next time the app is launched.
   */
  private void handleSetAnchorButton() {}

  /** Menu button to choose anchor type. */
  protected boolean settingsMenuClick(MenuItem item) {
    int itemId = item.getItemId();
    if (itemId == R.id.anchor_reset) {
      return true;
    }
    item.setChecked(!item.isChecked());
    sharedPreferences.edit().putInt(ANCHOR_MODE, itemId).commit();
    if (itemId == R.id.geospatial) {
      anchorType = AnchorType.GEOSPATIAL;
      return true;
    } else if (itemId == R.id.terrain) {
      anchorType = AnchorType.TERRAIN;
      return true;
    } else if (itemId == R.id.rooftop) {
      anchorType = AnchorType.ROOFTOP;
      return true;
    }
    return false;
  }

  /** Creates anchor with the provided GeospatialPose, either from camera or HitResult. */
  private void createAnchorWithGeospatialPose(Earth earth, GeospatialPose geospatialPose) {
    double latitude = geospatialPose.getLatitude();
    double longitude = geospatialPose.getLongitude();
    double altitude = geospatialPose.getAltitude();
    float[] quaternion = geospatialPose.getEastUpSouthQuaternion();
    switch (anchorType) {
      case TERRAIN:
        createTerrainAnchor(earth, latitude, longitude, identityQuaternion);
        storeAnchorParameters(latitude, longitude, 0, identityQuaternion);
        break;
      case GEOSPATIAL:
        createAnchor(earth, latitude, longitude, altitude, quaternion);
        storeAnchorParameters(latitude, longitude, altitude, quaternion);
        break;
      case ROOFTOP:
        createRooftopAnchor(earth, latitude, longitude, identityQuaternion);
        storeAnchorParameters(latitude, longitude, 0, identityQuaternion);
        break;
    }
    runOnUiThread(
        () -> {
          clearAnchorsButton.setVisibility(View.VISIBLE);
        });
    if (clearedAnchorsAmount != null) {
      clearedAnchorsAmount = null;
    }
  }

  private void handleClearAnchorsButton() {
    synchronized (anchorsLock) {
      clearedAnchorsAmount = anchors.size();
      String message =
          getResources()
              .getQuantityString(
                  R.plurals.status_anchors_cleared, clearedAnchorsAmount, clearedAnchorsAmount);

      statusTextView.setVisibility(View.VISIBLE);
      statusTextView.setText(message);

      for (Anchor anchor : anchors) {
        anchor.detach();
      }
      anchors.clear();
    }
    clearAnchorsFromSharedPreferences();
    clearAnchorsButton.setVisibility(View.INVISIBLE);
    setAnchorButton.setVisibility(View.VISIBLE);
    tapScreenTextView.setVisibility(View.VISIBLE);
  }

  /** Create an anchor at a specific geodetic location using a EUS quaternion. */
  private void createAnchor(
      Earth earth, double latitude, double longitude, double altitude, float[] quaternion) {
    Anchor anchor =
        earth.createAnchor(
            latitude,
            longitude,
            altitude,
            quaternion[0],
            quaternion[1],
            quaternion[2],
            quaternion[3]);
    synchronized (anchorsLock) {
      anchors.add(anchor);
    }
  }

  /** Create a terrain anchor at a specific geodetic location using a EUS quaternion. */
  private void createTerrainAnchor(
      Earth earth, double latitude, double longitude, float[] quaternion) {
    final ResolveAnchorOnTerrainFuture future =
        earth.resolveAnchorOnTerrainAsync(
            latitude,
            longitude,
            /* altitudeAboveTerrain= */ 0.0f,
            quaternion[0],
            quaternion[1],
            quaternion[2],
            quaternion[3],
            (anchor, state) -> {
              if (state == TerrainAnchorState.SUCCESS) {
                synchronized (anchorsLock) {
                  anchors.add(anchor);
                  terrainAnchors.add(anchor);
                }
              } else {
                statusTextView.setVisibility(View.VISIBLE);
                statusTextView.setText(getString(R.string.status_terrain_anchor, state));
              }
            });
  }

  /** Create a rooftop anchor at a specific geodetic location using a EUS quaternion. */
  private void createRooftopAnchor(
      Earth earth, double latitude, double longitude, float[] quaternion) {
    final ResolveAnchorOnRooftopFuture future =
        earth.resolveAnchorOnRooftopAsync(
            latitude,
            longitude,
            /* altitudeAboveRooftop= */ 0.0f,
            quaternion[0],
            quaternion[1],
            quaternion[2],
            quaternion[3],
            (anchor, state) -> {
              if (state == RooftopAnchorState.SUCCESS) {
                synchronized (anchorsLock) {
                  anchors.add(anchor);
                  rooftopAnchors.add(anchor);
                }
              } else {
                statusTextView.setVisibility(View.VISIBLE);
                statusTextView.setText(getString(R.string.status_rooftop_anchor, state));
              }
            });
  }

  /**
   * Helper function to store the parameters used in anchor creation in {@link SharedPreferences}.
   */
  private void storeAnchorParameters(
      double latitude, double longitude, double altitude, float[] quaternion) {
    Set<String> anchorParameterSet =
        sharedPreferences.getStringSet(SHARED_PREFERENCES_SAVED_ANCHORS, new HashSet<>());
    HashSet<String> newAnchorParameterSet = new HashSet<>(anchorParameterSet);

    SharedPreferences.Editor editor = sharedPreferences.edit();
    String type = "";
    switch (anchorType) {
      case TERRAIN:
        type = "Terrain";
        break;
      case ROOFTOP:
        type = "Rooftop";
        break;
      default:
        type = "";
        break;
    }
    newAnchorParameterSet.add(
        String.format(
            type + "%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f",
            latitude,
            longitude,
            altitude,
            quaternion[0],
            quaternion[1],
            quaternion[2],
            quaternion[3]));
    editor.putStringSet(SHARED_PREFERENCES_SAVED_ANCHORS, newAnchorParameterSet);
    editor.commit();
  }

  private void clearAnchorsFromSharedPreferences() {
    SharedPreferences.Editor editor = sharedPreferences.edit();
    editor.putStringSet(SHARED_PREFERENCES_SAVED_ANCHORS, null);
    editor.commit();
  }

  /** Creates all anchors that were stored in the {@link SharedPreferences}. */
  private void createAnchorFromSharedPreferences(Earth earth) {
    Set<String> anchorParameterSet =
        sharedPreferences.getStringSet(SHARED_PREFERENCES_SAVED_ANCHORS, null);
    if (anchorParameterSet == null) {
      return;
    }

    for (String anchorParameters : anchorParameterSet) {
      AnchorType type = AnchorType.GEOSPATIAL;
      if (anchorParameters.contains("Terrain")) {
        type = AnchorType.TERRAIN;
        anchorParameters = anchorParameters.replace("Terrain", "");
      } else if (anchorParameters.contains("Rooftop")) {
        type = AnchorType.ROOFTOP;
        anchorParameters = anchorParameters.replace("Rooftop", "");
      }
      String[] parameters = anchorParameters.split(",");
      if (parameters.length != 7) {
        Log.d(
            TAG, "Invalid number of anchor parameters. Expected four, found " + parameters.length);
        continue;
      }
      double latitude = Double.parseDouble(parameters[0]);
      double longitude = Double.parseDouble(parameters[1]);
      double altitude = Double.parseDouble(parameters[2]);
      float[] quaternion =
          new float[] {
            Float.parseFloat(parameters[3]),
            Float.parseFloat(parameters[4]),
            Float.parseFloat(parameters[5]),
            Float.parseFloat(parameters[6])
          };
      switch (type) {
        case TERRAIN:
          createTerrainAnchor(earth, latitude, longitude, quaternion);
          break;
        case ROOFTOP:
          createRooftopAnchor(earth, latitude, longitude, quaternion);
          break;
        default:
          createAnchor(earth, latitude, longitude, altitude, quaternion);
          break;
      }
    }

    runOnUiThread(() -> clearAnchorsButton.setVisibility(View.VISIBLE));
  }

  @Override
  public void onDialogPositiveClick(DialogFragment dialog) {
    if (!sharedPreferences.edit().putBoolean(ALLOW_GEOSPATIAL_ACCESS_KEY, true).commit()) {
      throw new AssertionError("Could not save the user preference to SharedPreferences!");
    }
    createSession();
  }

  @Override
  public void onDialogContinueClick(DialogFragment dialog) {
    dialog.dismiss();
  }

  private void onRenderStreetscapeGeometryChanged(CompoundButton button, boolean isChecked) {
    if (session == null) {
      return;
    }
    isRenderStreetscapeGeometry = isChecked;
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
      synchronized (anchorsLock) {
        if (queuedSingleTap == null
            || anchors.size() >= MAXIMUM_ANCHORS
            || cameraTrackingState != TrackingState.TRACKING) {
          queuedSingleTap = null;
          return;
        }
      }
      Earth earth = session.getEarth();
      if (earth == null || earth.getTrackingState() != TrackingState.TRACKING) {
        queuedSingleTap = null;
        return;
      }

      for (HitResult hit : frame.hitTest(queuedSingleTap)) {
        if (shouldCreateAnchorWithHit(hit)) {
          Pose hitPose = hit.getHitPose();
          GeospatialPose geospatialPose = earth.getGeospatialPose(hitPose);
          createAnchorWithGeospatialPose(earth, geospatialPose);
          break; // Only handle the first valid hit.
        }
      }
      queuedSingleTap = null;
    }
  }

  /** Returns {@code true} if and only if the hit can be used to create an Anchor reliably. */
  private boolean shouldCreateAnchorWithHit(HitResult hit) {
    Trackable trackable = hit.getTrackable();
    if (isRenderStreetscapeGeometry) {
      if (trackable instanceof StreetscapeGeometry) {
        return true;
      }
    }
    if (trackable instanceof Plane) {
      // Check if the hit was within the plane's polygon.
      return ((Plane) trackable).isPoseInPolygon(hit.getHitPose());
    } else if (trackable instanceof Point) {
      // Check if the hit was against an oriented point.
      return ((Point) trackable).getOrientationMode() == OrientationMode.ESTIMATED_SURFACE_NORMAL;
    }
    return false;
  }

}
