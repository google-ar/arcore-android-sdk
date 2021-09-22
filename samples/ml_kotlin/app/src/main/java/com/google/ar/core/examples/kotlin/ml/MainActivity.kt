/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.ar.core.examples.kotlin.ml

import android.os.Bundle
import android.util.Log
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.google.ar.core.CameraConfig
import com.google.ar.core.CameraConfigFilter
import com.google.ar.core.Config
import com.google.ar.core.examples.java.common.helpers.FullScreenHelper
import com.google.ar.core.examples.kotlin.common.helpers.ARCoreSessionLifecycleHelper
import com.google.ar.core.exceptions.CameraNotAvailableException
import com.google.ar.core.exceptions.UnavailableApkTooOldException
import com.google.ar.core.exceptions.UnavailableArcoreNotInstalledException
import com.google.ar.core.exceptions.UnavailableDeviceNotCompatibleException
import com.google.ar.core.exceptions.UnavailableSdkTooOldException
import com.google.ar.core.exceptions.UnavailableUserDeclinedInstallationException

/** Main activity for the ML sample. */
class MainActivity : AppCompatActivity() {
  val TAG = "MainActivity"
  lateinit var arCoreSessionHelper: ARCoreSessionLifecycleHelper

  lateinit var renderer: AppRenderer
  lateinit var view: MainActivityView

  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)

    arCoreSessionHelper = ARCoreSessionLifecycleHelper(this)
    // When session creation or session.resume fails, we display a message and log detailed
    // information.
    arCoreSessionHelper.exceptionCallback =
      { exception ->
        val message =
          when (exception) {
            is UnavailableArcoreNotInstalledException,
            is UnavailableUserDeclinedInstallationException -> "Please install ARCore"
            is UnavailableApkTooOldException -> "Please update ARCore"
            is UnavailableSdkTooOldException -> "Please update this app"
            is UnavailableDeviceNotCompatibleException -> "This device does not support AR"
            is CameraNotAvailableException -> "Camera not available. Try restarting the app."
            else -> "Failed to create AR session: $exception"
          }
        Log.e(TAG, message, exception)
        Toast.makeText(this, message, Toast.LENGTH_LONG).show()
      }

    arCoreSessionHelper.beforeSessionResume =
      { session ->
        session.configure(
          session.config.apply {
            // To get the best image of the object in question, enable autofocus.
            focusMode = Config.FocusMode.AUTO
            if (session.isDepthModeSupported(Config.DepthMode.AUTOMATIC)) {
              depthMode = Config.DepthMode.AUTOMATIC
            }
          }
        )

        val filter =
          CameraConfigFilter(session).setFacingDirection(CameraConfig.FacingDirection.BACK)
        val configs = session.getSupportedCameraConfigs(filter)
        val sort =
          compareByDescending<CameraConfig> { it.imageSize.width }.thenByDescending {
            it.imageSize.height
          }
        session.cameraConfig = configs.sortedWith(sort)[0]
      }
    lifecycle.addObserver(arCoreSessionHelper)

    renderer = AppRenderer(this)
    lifecycle.addObserver(renderer)
    view = MainActivityView(this, renderer)
    setContentView(view.root)
    renderer.bindView(view)
    lifecycle.addObserver(view)
  }

  override fun onRequestPermissionsResult(
    requestCode: Int,
    permissions: Array<out String>,
    grantResults: IntArray
  ) {
    super.onRequestPermissionsResult(requestCode, permissions, grantResults)
    arCoreSessionHelper.onRequestPermissionsResult(requestCode, permissions, grantResults)
  }

  override fun onWindowFocusChanged(hasFocus: Boolean) {
    super.onWindowFocusChanged(hasFocus)
    FullScreenHelper.setFullScreenOnWindowFocusChanged(this, hasFocus)
  }
}
