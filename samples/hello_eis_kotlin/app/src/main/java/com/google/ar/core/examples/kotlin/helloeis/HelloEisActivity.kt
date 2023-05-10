/*
 * Copyright 2021 Google LLC
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
package com.google.ar.core.examples.kotlin.helloeis

import android.os.Bundle
import android.util.Log
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.google.ar.core.CameraConfig
import com.google.ar.core.CameraConfigFilter
import com.google.ar.core.Config
import com.google.ar.core.Config.InstantPlacementMode
import com.google.ar.core.Session
import com.google.ar.core.examples.java.common.helpers.CameraPermissionHelper
import com.google.ar.core.examples.java.common.helpers.DepthSettings
import com.google.ar.core.examples.java.common.helpers.EisSettings
import com.google.ar.core.examples.java.common.helpers.FullScreenHelper
import com.google.ar.core.examples.java.common.helpers.InstantPlacementSettings
import com.google.ar.core.examples.java.common.samplerender.SampleRender
import com.google.ar.core.examples.kotlin.common.helpers.ARCoreSessionLifecycleHelper
import com.google.ar.core.exceptions.CameraNotAvailableException
import com.google.ar.core.exceptions.UnavailableApkTooOldException
import com.google.ar.core.exceptions.UnavailableDeviceNotCompatibleException
import com.google.ar.core.exceptions.UnavailableSdkTooOldException
import com.google.ar.core.exceptions.UnavailableUserDeclinedInstallationException
import java.util.EnumSet

/**
 * This is a simple example that shows how to create an augmented reality (AR) application using the
 * ARCore API. The application will display any detected planes and will allow the user to tap on a
 * plane to place a 3D model. The app also shows how to enable and use EIS in ARCore.
 */
class HelloEisActivity : AppCompatActivity() {
  companion object {
    private const val TAG = "HelloEisActivity"
  }

  lateinit var arCoreSessionHelper: ARCoreSessionLifecycleHelper
  lateinit var view: HelloEisView
  private lateinit var renderer: HelloEisRenderer

  val instantPlacementSettings = InstantPlacementSettings()
  val depthSettings = DepthSettings()
  val eisSettings = EisSettings()

  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)

    // Setup ARCore session lifecycle helper and configuration.
    arCoreSessionHelper = ARCoreSessionLifecycleHelper(this)
    // If Session creation or Session.resume() fails, display a message and log detailed
    // information.
    arCoreSessionHelper.exceptionCallback = { exception ->
      val message =
        when (exception) {
          is UnavailableUserDeclinedInstallationException ->
            "Please install Google Play Services for AR"
          is UnavailableApkTooOldException -> "Please update ARCore"
          is UnavailableSdkTooOldException -> "Please update this app"
          is UnavailableDeviceNotCompatibleException -> "This device does not support AR"
          is CameraNotAvailableException -> "Camera not available. Try restarting the app."
          else -> "Failed to create AR session: $exception"
        }
      Log.e(TAG, "ARCore threw an exception", exception)
      view.snackbarHelper.showError(this, message)
    }

    // Pick camera configuration and enable session features, including: Lighting Estimation, Depth
    // mode, Instant Placement.
    arCoreSessionHelper.beforeSessionResume = { session ->
      // Use 30 FPS configurations.
      val cameraConfigFilter =
        CameraConfigFilter(session).setTargetFps(EnumSet.of(CameraConfig.TargetFps.TARGET_FPS_30))
      val cameraConfigs = session.getSupportedCameraConfigs(cameraConfigFilter)
      Log.i(
        TAG,
        "Using Camera config with texture size ${cameraConfigs[0].textureSize} & fps ${cameraConfigs[0].fpsRange}"
      )
      session.cameraConfig = cameraConfigs[0]
      configureSession(session)
    }
    lifecycle.addObserver(arCoreSessionHelper)

    // Set up the Hello EIS renderer.
    renderer = HelloEisRenderer(this)
    lifecycle.addObserver(renderer)

    // Set up Hello EIS UI.
    view = HelloEisView(this)
    lifecycle.addObserver(view)
    setContentView(view.root)

    // Sets up an example renderer using our HelloARRenderer.
    SampleRender(view.surfaceView, renderer, assets)

    depthSettings.onCreate(this)
    instantPlacementSettings.onCreate(this)
    eisSettings.onCreate(this)
  }

  // Configure the session, using Lighting Estimation, and Depth mode.
  fun configureSession(session: Session) {
    session.configure(
      session.config.apply {
        lightEstimationMode = Config.LightEstimationMode.ENVIRONMENTAL_HDR

        // Depth API is used if it is configured in Hello EIS's settings.
        depthMode =
          if (session.isDepthModeSupported(Config.DepthMode.AUTOMATIC)) {
            Config.DepthMode.AUTOMATIC
          } else {
            Config.DepthMode.DISABLED
          }

        // Instant Placement is used if it is configured in Hello EIS's settings.
        instantPlacementMode =
          if (instantPlacementSettings.isInstantPlacementEnabled) {
            InstantPlacementMode.LOCAL_Y_UP
          } else {
            InstantPlacementMode.DISABLED
          }

        // Image Stabilization is used if it is configured in Hello EIS's settings.
        imageStabilizationMode =
          if (
            session.isImageStabilizationModeSupported(Config.ImageStabilizationMode.EIS) &&
              eisSettings.isEisEnabled
          )
            Config.ImageStabilizationMode.EIS
          else Config.ImageStabilizationMode.OFF
      }
    )
  }

  override fun onRequestPermissionsResult(
    requestCode: Int,
    permissions: Array<String>,
    results: IntArray
  ) {
    super.onRequestPermissionsResult(requestCode, permissions, results)
    if (!CameraPermissionHelper.hasCameraPermission(this)) {
      // Use toast instead of snackbar here since the activity will exit.
      Toast.makeText(this, "Camera permission is needed to run this application", Toast.LENGTH_LONG)
        .show()
      if (!CameraPermissionHelper.shouldShowRequestPermissionRationale(this)) {
        // Permission denied with checking "Do not ask again".
        CameraPermissionHelper.launchPermissionSettings(this)
      }
      finish()
    }
  }

  override fun onWindowFocusChanged(hasFocus: Boolean) {
    super.onWindowFocusChanged(hasFocus)
    FullScreenHelper.setFullScreenOnWindowFocusChanged(this, hasFocus)
  }
}
