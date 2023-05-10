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
package com.google.ar.core.examples.kotlin.common.helpers

import android.app.Activity
import android.widget.Toast
import androidx.lifecycle.DefaultLifecycleObserver
import androidx.lifecycle.LifecycleOwner
import com.google.ar.core.ArCoreApk
import com.google.ar.core.Session
import com.google.ar.core.examples.java.common.helpers.CameraPermissionHelper
import com.google.ar.core.exceptions.CameraNotAvailableException

/**
 * Manages an ARCore Session using the Android Lifecycle API. Before starting a Session, this class
 * requests installation of Google Play Services for AR if it's not installed or not up to date and
 * asks the user for required permissions if necessary.
 */
class ARCoreSessionLifecycleHelper(
  val activity: Activity,
  val features: Set<Session.Feature> = setOf()
) : DefaultLifecycleObserver {
  var installRequested = false
  var session: Session? = null
    private set

  /**
   * Creating a session may fail. In this case, session will remain null, and this function will be
   * called with an exception.
   *
   * See
   * [the `Session` constructor](https://developers.google.com/ar/reference/java/com/google/ar/core/Session#Session(android.content.Context)
   * ) for more details.
   */
  var exceptionCallback: ((Exception) -> Unit)? = null

  /**
   * Before `Session.resume()` is called, a session must be configured. Use
   * [`Session.configure`](https://developers.google.com/ar/reference/java/com/google/ar/core/Session#configure-config)
   * or
   * [`setCameraConfig`](https://developers.google.com/ar/reference/java/com/google/ar/core/Session#setCameraConfig-cameraConfig)
   */
  var beforeSessionResume: ((Session) -> Unit)? = null

  /**
   * Attempts to create a session. If Google Play Services for AR is not installed or not up to
   * date, request installation.
   *
   * @return null when the session cannot be created due to a lack of the CAMERA permission or when
   * Google Play Services for AR is not installed or up to date, or when session creation fails for
   * any reason. In the case of a failure, [exceptionCallback] is invoked with the failure
   * exception.
   */
  private fun tryCreateSession(): Session? {
    // The app must have been given the CAMERA permission. If we don't have it yet, request it.
    if (!CameraPermissionHelper.hasCameraPermission(activity)) {
      CameraPermissionHelper.requestCameraPermission(activity)
      return null
    }

    return try {
      // Request installation if necessary.
      when (ArCoreApk.getInstance().requestInstall(activity, !installRequested)!!) {
        ArCoreApk.InstallStatus.INSTALL_REQUESTED -> {
          installRequested = true
          // tryCreateSession will be called again, so we return null for now.
          return null
        }
        ArCoreApk.InstallStatus.INSTALLED -> {
          // Left empty; nothing needs to be done.
        }
      }

      // Create a session if Google Play Services for AR is installed and up to date.
      Session(activity, features)
    } catch (e: Exception) {
      exceptionCallback?.invoke(e)
      null
    }
  }

  override fun onResume(owner: LifecycleOwner) {
    val session = this.session ?: tryCreateSession() ?: return
    try {
      beforeSessionResume?.invoke(session)
      session.resume()
      this.session = session
    } catch (e: CameraNotAvailableException) {
      exceptionCallback?.invoke(e)
    }
  }

  override fun onPause(owner: LifecycleOwner) {
    session?.pause()
  }

  override fun onDestroy(owner: LifecycleOwner) {
    // Explicitly close the ARCore session to release native resources.
    // Review the API reference for important considerations before calling close() in apps with
    // more complicated lifecycle requirements:
    // https://developers.google.com/ar/reference/java/arcore/reference/com/google/ar/core/Session#close()
    session?.close()
    session = null
  }

  fun onRequestPermissionsResult(
    requestCode: Int,
    permissions: Array<out String>,
    results: IntArray
  ) {
    if (!CameraPermissionHelper.hasCameraPermission(activity)) {
      // Use toast instead of snackbar here since the activity will exit.
      Toast.makeText(
          activity,
          "Camera permission is needed to run this application",
          Toast.LENGTH_LONG
        )
        .show()
      if (!CameraPermissionHelper.shouldShowRequestPermissionRationale(activity)) {
        // Permission denied with checking "Do not ask again".
        CameraPermissionHelper.launchPermissionSettings(activity)
      }
      activity.finish()
    }
  }
}
