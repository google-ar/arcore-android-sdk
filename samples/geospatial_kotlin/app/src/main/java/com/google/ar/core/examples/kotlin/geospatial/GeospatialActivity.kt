package com.google.ar.core.examples.kotlin.geospatial

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


import android.content.Context
import android.content.SharedPreferences
import android.location.Location
import android.opengl.GLSurfaceView
import android.opengl.Matrix
import android.os.Bundle
import android.util.Log
import android.view.GestureDetector
import android.view.MenuItem
import android.view.MotionEvent
import android.view.View
import android.widget.Button
import android.widget.CompoundButton
import android.widget.PopupMenu
import android.widget.Switch
import android.widget.TextView
import android.widget.Toast
import androidx.annotation.GuardedBy
import androidx.appcompat.app.AppCompatActivity
import androidx.fragment.app.DialogFragment
import com.google.android.gms.location.FusedLocationProviderClient
import com.google.android.gms.location.LocationServices
import com.google.ar.core.Anchor
import com.google.ar.core.Anchor.RooftopAnchorState
import com.google.ar.core.Anchor.TerrainAnchorState
import com.google.ar.core.ArCoreApk
import com.google.ar.core.Camera
import com.google.ar.core.Config
import com.google.ar.core.Earth
import com.google.ar.core.Frame
import com.google.ar.core.GeospatialPose
import com.google.ar.core.HitResult
import com.google.ar.core.Plane
import com.google.ar.core.Point
import com.google.ar.core.Point.OrientationMode
import com.google.ar.core.Pose
import com.google.ar.core.Session
import com.google.ar.core.StreetscapeGeometry
import com.google.ar.core.Trackable
import com.google.ar.core.TrackingState
import com.google.ar.core.VpsAvailability
import com.google.ar.core.examples.java.common.helpers.CameraPermissionHelper
import com.google.ar.core.examples.java.common.helpers.DisplayRotationHelper
import com.google.ar.core.examples.java.common.helpers.FullScreenHelper
import com.google.ar.core.examples.java.common.helpers.LocationPermissionHelper
import com.google.ar.core.examples.java.common.helpers.SnackbarHelper
import com.google.ar.core.examples.java.common.helpers.TrackingStateHelper
import com.google.ar.core.examples.java.common.samplerender.Framebuffer
import com.google.ar.core.examples.java.common.samplerender.IndexBuffer
import com.google.ar.core.examples.java.common.samplerender.Mesh
import com.google.ar.core.examples.java.common.samplerender.SampleRender
import com.google.ar.core.examples.java.common.samplerender.Shader
import com.google.ar.core.examples.java.common.samplerender.Shader.BlendFactor
import com.google.ar.core.examples.java.common.samplerender.Texture
import com.google.ar.core.examples.java.common.samplerender.VertexBuffer
import com.google.ar.core.examples.java.common.samplerender.arcore.BackgroundRenderer
import com.google.ar.core.examples.java.common.samplerender.arcore.PlaneRenderer
import com.google.ar.core.exceptions.CameraNotAvailableException
import com.google.ar.core.exceptions.FineLocationPermissionNotGrantedException
import com.google.ar.core.exceptions.GooglePlayServicesLocationLibraryNotLinkedException
import com.google.ar.core.exceptions.UnavailableApkTooOldException
import com.google.ar.core.exceptions.UnavailableArcoreNotInstalledException
import com.google.ar.core.exceptions.UnavailableDeviceNotCompatibleException
import com.google.ar.core.exceptions.UnavailableSdkTooOldException
import com.google.ar.core.exceptions.UnavailableUserDeclinedInstallationException
import com.google.ar.core.exceptions.UnsupportedConfigurationException
import java.io.IOException
import java.util.concurrent.TimeUnit

/**
 * Main activity for the Geospatial API example.
 *
 * This example shows how to use the Geospatial APIs. Once the device is localized, anchors can
 * be created at the device's geospatial location. Anchor locations are persisted across sessions
 * and will be recreated once localized.
 */
class GeospatialActivity : AppCompatActivity(),
    SampleRender.Renderer,
    VpsAvailabilityNoticeDialogFragment.NoticeDialogListener,
    PrivacyNoticeDialogFragment.NoticeDialogListener {

    companion object {
        private val TAG = GeospatialActivity::class.java.simpleName

        private const val SHARED_PREFERENCES_SAVED_ANCHORS = "SHARED_PREFERENCES_SAVED_ANCHORS"
        private const val ALLOW_GEOSPATIAL_ACCESS_KEY = "ALLOW_GEOSPATIAL_ACCESS"
        private const val ANCHOR_MODE = "ANCHOR_MODE"

        private const val Z_NEAR = 0.1f
        private const val Z_FAR = 1000f

        private const val LOCALIZING_HORIZONTAL_ACCURACY_THRESHOLD_METERS = 10.0
        private const val LOCALIZING_ORIENTATION_YAW_ACCURACY_THRESHOLD_DEGREES = 15.0

        private const val LOCALIZED_HORIZONTAL_ACCURACY_HYSTERESIS_METERS = 10.0
        private const val LOCALIZED_ORIENTATION_YAW_ACCURACY_HYSTERESIS_DEGREES = 10.0

        private const val LOCALIZING_TIMEOUT_SECONDS = 180
        private const val MAXIMUM_ANCHORS = 20
        private const val DURATION_FOR_NO_TERRAIN_ANCHOR_RESULT_MS = 10000L
    }

    enum class State {
        UNINITIALIZED,
        UNSUPPORTED,
        EARTH_STATE_ERROR,
        PRETRACKING,
        LOCALIZING,
        LOCALIZING_FAILED,
        LOCALIZED
    }

    enum class AnchorType {
        GEOSPATIAL,
        TERRAIN,
        ROOFTOP
    }

    // Rendering
    private lateinit var surfaceView: GLSurfaceView

    private var installRequested = false
    private var clearedAnchorsAmount: Int? = null

    private var localizingStartTimestamp = 0L
    private var deadlineForMessageMillis = 0L

    private var state = State.UNINITIALIZED
    private var anchorType = AnchorType.GEOSPATIAL

    private var session: Session? = null
    private val messageSnackbarHelper = SnackbarHelper()
    private lateinit var displayRotationHelper: DisplayRotationHelper
    private val trackingStateHelper = TrackingStateHelper(this)
    private lateinit var render: SampleRender
    private lateinit var sharedPreferences: SharedPreferences

    private var lastStatusText: String = ""
    private lateinit var geospatialPoseTextView: TextView
    private lateinit var statusTextView: TextView
    private lateinit var tapScreenTextView: TextView
    private lateinit var setAnchorButton: Button
    private lateinit var clearAnchorsButton: Button
    private lateinit var streetscapeGeometrySwitch: Switch

    private lateinit var planeRenderer: PlaneRenderer
    private lateinit var backgroundRenderer: BackgroundRenderer
    private lateinit var virtualSceneFramebuffer: Framebuffer
    private var hasSetTextureNames = false
    private var isRenderStreetscapeGeometry = false

    private lateinit var virtualObjectMesh: Mesh
    private lateinit var geospatialAnchorVirtualObjectShader: Shader
    private lateinit var terrainAnchorVirtualObjectShader: Shader

    private val anchorsLock = Any()

    @GuardedBy("anchorsLock")
    private val anchors = mutableListOf<Anchor>()

    private val terrainAnchors = mutableSetOf<Anchor>()
    private val rooftopAnchors = mutableSetOf<Anchor>()

    // Temporary matrices to reduce allocations per frame
    private val modelMatrix = FloatArray(16)
    private val viewMatrix = FloatArray(16)
    private val projectionMatrix = FloatArray(16)
    private val modelViewMatrix = FloatArray(16)
    private val modelViewProjectionMatrix = FloatArray(16)

    private val identityQuaternion = floatArrayOf(0f, 0f, 0f, 1f)

    private val singleTapLock = Any()

    @GuardedBy("singleTapLock")
    private var queuedSingleTap: MotionEvent? = null

    private lateinit var gestureDetector: GestureDetector

    // Point Cloud
    private lateinit var pointCloudVertexBuffer: VertexBuffer
    private lateinit var pointCloudMesh: Mesh
    private lateinit var pointCloudShader: Shader
    private var lastPointCloudTimestamp = 0L

    private lateinit var fusedLocationClient: FusedLocationProviderClient

    // Streetscape geometry
    private val wallsColor = ArrayList<FloatArray>()
    private lateinit var streetscapeGeometryTerrainShader: Shader
    private lateinit var streetscapeGeometryBuildingShader: Shader
    private val streetscapeGeometryToMeshes = HashMap<StreetscapeGeometry, Mesh>()

    // Lifecycle

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        sharedPreferences = getPreferences(Context.MODE_PRIVATE)

        setContentView(R.layout.activity_main)
        surfaceView = findViewById(R.id.surfaceview)
        geospatialPoseTextView = findViewById(R.id.geospatial_pose_view)
        statusTextView = findViewById(R.id.status_text_view)
        tapScreenTextView = findViewById(R.id.tap_screen_text_view)
        setAnchorButton = findViewById(R.id.set_anchor_button)
        clearAnchorsButton = findViewById(R.id.clear_anchors_button)

        setAnchorButton.setOnClickListener { v ->
            PopupMenu(this, v).apply {
                setOnMenuItemClickListener { item -> settingsMenuClick(item) }
                inflate(R.menu.settings_menu)
                show()
                menu.findItem(sharedPreferences.getInt(ANCHOR_MODE, R.id.geospatial))
                    ?.isChecked = true
            }
        }

        clearAnchorsButton.setOnClickListener { handleClearAnchorsButton() }

        streetscapeGeometrySwitch = findViewById(R.id.streetscape_geometry_switch)
        streetscapeGeometrySwitch.isChecked = false
        streetscapeGeometrySwitch.setOnCheckedChangeListener { button, isChecked ->
            onRenderStreetscapeGeometryChanged(button, isChecked)
        }

        displayRotationHelper = DisplayRotationHelper(this)
        render = SampleRender(surfaceView, this, assets)

        installRequested = false
        clearedAnchorsAmount = null

        gestureDetector = GestureDetector(this, object : GestureDetector.SimpleOnGestureListener() {
            override fun onSingleTapUp(e: MotionEvent): Boolean {
                synchronized(singleTapLock) { queuedSingleTap = e }
                return true
            }

            override fun onDown(e: MotionEvent): Boolean = true
        })
        surfaceView.setOnTouchListener { _, event -> gestureDetector.onTouchEvent(event) }

        fusedLocationClient = LocationServices.getFusedLocationProviderClient(this)
    }

    override fun onDestroy() {
        session?.close()
        session = null
        super.onDestroy()
    }

    override fun onResume() {
        super.onResume()
        if (sharedPreferences.getBoolean(ALLOW_GEOSPATIAL_ACCESS_KEY, false)) {
            createSession()
        } else {
            showPrivacyNoticeDialog()
        }
        surfaceView.onResume()
        displayRotationHelper.onResume()
    }

    override fun onPause() {
        super.onPause()
        session?.let {
            displayRotationHelper.onPause()
            surfaceView.onPause()
            it.pause()
        }
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<String>, results: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, results)
        if (!CameraPermissionHelper.hasCameraPermission(this)) {
            Toast.makeText(this, "Camera permission is needed to run this application", Toast.LENGTH_LONG).show()
            if (!CameraPermissionHelper.shouldShowRequestPermissionRationale(this)) {
                CameraPermissionHelper.launchPermissionSettings(this)
            }
            finish()
        }
        if (LocationPermissionHelper.hasFineLocationPermissionsResponseInResult(permissions)
            && !LocationPermissionHelper.hasFineLocationPermission(this)
        ) {
            Toast.makeText(this, "Precise location permission is needed to run this application", Toast.LENGTH_LONG).show()
            if (!LocationPermissionHelper.shouldShowRequestPermissionRationale(this)) {
                LocationPermissionHelper.launchPermissionSettings(this)
            }
            finish()
        }
    }

    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)
        FullScreenHelper.setFullScreenOnWindowFocusChanged(this, hasFocus)
    }

    // Session setup

    private fun showPrivacyNoticeDialog() {
        PrivacyNoticeDialogFragment.createDialog()
            .show(supportFragmentManager, PrivacyNoticeDialogFragment::class.java.name)
    }

    private fun createSession() {
        var exception: Exception? = null
        var message: String? = null

        if (session == null) {
            try {
                when (ArCoreApk.getInstance().requestInstall(this, !installRequested)) {
                    ArCoreApk.InstallStatus.INSTALL_REQUESTED -> {
                        installRequested = true
                        return
                    }
                    ArCoreApk.InstallStatus.INSTALLED -> Unit
                }

                if (!CameraPermissionHelper.hasCameraPermission(this)) {
                    CameraPermissionHelper.requestCameraPermission(this)
                    return
                }
                if (!LocationPermissionHelper.hasFineLocationPermission(this)) {
                    LocationPermissionHelper.requestFineLocationPermission(this)
                    return
                }

                session = Session(this)
            } catch (e: UnavailableArcoreNotInstalledException) {
                message = "Please install ARCore"; exception = e
            } catch (e: UnavailableUserDeclinedInstallationException) {
                message = "Please install ARCore"; exception = e
            } catch (e: UnavailableApkTooOldException) {
                message = "Please update ARCore"; exception = e
            } catch (e: UnavailableSdkTooOldException) {
                message = "Please update this app"; exception = e
            } catch (e: UnavailableDeviceNotCompatibleException) {
                message = "This device does not support AR"; exception = e
            } catch (e: Exception) {
                message = "Failed to create AR session"; exception = e
            }

            if (message != null) {
                messageSnackbarHelper.showError(this, message)
                Log.e(TAG, "Exception creating session", exception)
                return
            }
        }

        if (session != null) {
            getLastLocation()
        }

        try {
            configureSession()
            session?.resume()
        } catch (e: CameraNotAvailableException) {
            message = "Camera not available. Try restarting the app."; exception = e
        } catch (e: GooglePlayServicesLocationLibraryNotLinkedException) {
            message = "Google Play Services location library not linked or obfuscated with Proguard."; exception = e
        } catch (e: FineLocationPermissionNotGrantedException) {
            message = "The Android permission ACCESS_FINE_LOCATION was not granted."; exception = e
        } catch (e: UnsupportedConfigurationException) {
            message = "This device does not support GeospatialMode.ENABLED."; exception = e
        } catch (e: SecurityException) {
            message = "Camera failure or the internet permission has not been granted."; exception = e
        }

        if (message != null) {
            session = null
            messageSnackbarHelper.showError(this, message)
            Log.e(TAG, "Exception configuring and resuming the session", exception)
        }
    }

    private fun getLastLocation() {
        try {
            fusedLocationClient.lastLocation.addOnSuccessListener { location: Location? ->
                val latitude = location?.latitude ?: run {
                    Log.e(TAG, "Error location is null"); 0.0
                }
                val longitude = location?.longitude ?: 0.0
                checkVpsAvailability(latitude, longitude)
            }
        } catch (e: SecurityException) {
            Log.e(TAG, "No location permissions granted by User!")
        }
    }

    private fun checkVpsAvailability(latitude: Double, longitude: Double) {
        session?.checkVpsAvailabilityAsync(latitude, longitude) { availability ->
            if (availability != VpsAvailability.AVAILABLE) {
                showVpsNotAvailabilityNoticeDialog()
            }
        }
    }

    private fun showVpsNotAvailabilityNoticeDialog() {
        VpsAvailabilityNoticeDialogFragment.createDialog()
            .show(supportFragmentManager, VpsAvailabilityNoticeDialogFragment::class.java.name)
    }

    private fun configureSession() {
        val currentSession = session ?: return
        if (!currentSession.isGeospatialModeSupported(Config.GeospatialMode.ENABLED)) {
            state = State.UNSUPPORTED
            return
        }
        val config = currentSession.config
            .setGeospatialMode(Config.GeospatialMode.ENABLED)
            .setStreetscapeGeometryMode(Config.StreetscapeGeometryMode.ENABLED)
        currentSession.configure(config)
        state = State.PRETRACKING
        localizingStartTimestamp = System.currentTimeMillis()
    }

    // SampleRender.Renderer callbacks

    override fun onSurfaceCreated(render: SampleRender) {
        try {
            planeRenderer = PlaneRenderer(render)
            backgroundRenderer = BackgroundRenderer(render)
            virtualSceneFramebuffer = Framebuffer(render, 1, 1)

            val virtualObjectTexture = Texture.createFromAsset(
                render,
                "models/spatial_marker_baked.png",
                Texture.WrapMode.CLAMP_TO_EDGE,
                Texture.ColorFormat.SRGB
            )
            virtualObjectMesh = Mesh.createFromAsset(render, "models/geospatial_marker.obj")
            geospatialAnchorVirtualObjectShader = Shader.createFromAssets(
                render,
                "shaders/ar_unlit_object.vert",
                "shaders/ar_unlit_object.frag",
                null
            ).setTexture("u_Texture", virtualObjectTexture)

            val terrainAnchorVirtualObjectTexture = Texture.createFromAsset(
                render,
                "models/spatial_marker_yellow.png",
                Texture.WrapMode.CLAMP_TO_EDGE,
                Texture.ColorFormat.SRGB
            )
            terrainAnchorVirtualObjectShader = Shader.createFromAssets(
                render,
                "shaders/ar_unlit_object.vert",
                "shaders/ar_unlit_object.frag",
                null
            ).setTexture("u_Texture", terrainAnchorVirtualObjectTexture)

            backgroundRenderer.setUseDepthVisualization(render, false)
            backgroundRenderer.setUseOcclusion(render, false)

            pointCloudShader = Shader.createFromAssets(
                render,
                "shaders/point_cloud.vert",
                "shaders/point_cloud.frag",
                null
            ).setVec4("u_Color", floatArrayOf(31f / 255f, 188f / 255f, 210f / 255f, 1f))
                .setFloat("u_PointSize", 5f)

            pointCloudVertexBuffer = VertexBuffer(render, 4, null)
            pointCloudMesh = Mesh(render, Mesh.PrimitiveMode.POINTS, null, arrayOf(pointCloudVertexBuffer))

            streetscapeGeometryBuildingShader = Shader.createFromAssets(
                render,
                "shaders/streetscape_geometry.vert",
                "shaders/streetscape_geometry.frag",
                null
            ).setBlend(BlendFactor.DST_ALPHA, BlendFactor.ONE)

            streetscapeGeometryTerrainShader = Shader.createFromAssets(
                render,
                "shaders/streetscape_geometry.vert",
                "shaders/streetscape_geometry.frag",
                null
            ).setBlend(BlendFactor.DST_ALPHA, BlendFactor.ONE)

            wallsColor.add(floatArrayOf(0.5f, 0f, 0.5f, 0.3f))
            wallsColor.add(floatArrayOf(0.5f, 0.5f, 0f, 0.3f))
            wallsColor.add(floatArrayOf(0f, 0.5f, 0.5f, 0.3f))
        } catch (e: IOException) {
            Log.e(TAG, "Failed to read a required asset file", e)
            messageSnackbarHelper.showError(this, "Failed to read a required asset file: $e")
        }
    }

    override fun onSurfaceChanged(render: SampleRender, width: Int, height: Int) {
        displayRotationHelper.onSurfaceChanged(width, height)
        virtualSceneFramebuffer.resize(width, height)
    }

    override fun onDrawFrame(render: SampleRender) {
        val currentSession = session ?: return

        if (!hasSetTextureNames) {
            currentSession.setCameraTextureNames(intArrayOf(backgroundRenderer.cameraColorTexture.textureId))
            hasSetTextureNames = true
        }

        displayRotationHelper.updateSessionIfNeeded(currentSession)
        updateStreetscapeGeometries(currentSession.getAllTrackables(StreetscapeGeometry::class.java))

        val frame: Frame = try {
            currentSession.update()
        } catch (e: CameraNotAvailableException) {
            Log.e(TAG, "Camera not available during onDrawFrame", e)
            messageSnackbarHelper.showError(this, "Camera not available. Try restarting the app.")
            return
        }
        val camera: Camera = frame.camera

        backgroundRenderer.updateDisplayGeometry(frame)
        trackingStateHelper.updateKeepScreenOnFlag(camera.trackingState)

        val earth = currentSession.earth
        if (earth != null) {
            updateGeospatialState(earth)
        }

        val message: String? = when (state) {
            State.UNINITIALIZED -> null
            State.UNSUPPORTED -> resources.getString(R.string.status_unsupported)
            State.PRETRACKING -> resources.getString(R.string.status_pretracking)
            State.EARTH_STATE_ERROR -> resources.getString(R.string.status_earth_state_error)
            State.LOCALIZING -> resources.getString(R.string.status_localize_hint)
            State.LOCALIZING_FAILED -> resources.getString(R.string.status_localize_timeout)
            State.LOCALIZED -> {
                if (lastStatusText == resources.getString(R.string.status_localize_hint)) {
                    resources.getString(R.string.status_localize_complete)
                } else null
            }
        }

        if (message != null && lastStatusText != message) {
            lastStatusText = message
            runOnUiThread {
                statusTextView.visibility = View.VISIBLE
                statusTextView.text = lastStatusText
            }
        }

        synchronized(anchorsLock) {
            if (anchors.size >= MAXIMUM_ANCHORS) {
                runOnUiThread {
                    setAnchorButton.visibility = View.INVISIBLE
                    tapScreenTextView.visibility = View.INVISIBLE
                }
            }
        }

        handleTap(frame, camera.trackingState)

        if (frame.timestamp != 0L) {
            backgroundRenderer.drawBackground(render)
        }

        if (camera.trackingState != TrackingState.TRACKING || state != State.LOCALIZED) return

        camera.getProjectionMatrix(projectionMatrix, 0, Z_NEAR, Z_FAR)
        camera.getViewMatrix(viewMatrix, 0)

        frame.acquirePointCloud().use { pointCloud ->
            if (pointCloud.timestamp > lastPointCloudTimestamp) {
                pointCloudVertexBuffer.set(pointCloud.points)
                lastPointCloudTimestamp = pointCloud.timestamp
            }
            Matrix.multiplyMM(modelViewProjectionMatrix, 0, projectionMatrix, 0, viewMatrix, 0)
            pointCloudShader.setMat4("u_ModelViewProjection", modelViewProjectionMatrix)
            render.draw(pointCloudMesh, pointCloudShader)
        }

        planeRenderer.drawPlanes(
            render,
            currentSession.getAllTrackables(Plane::class.java),
            camera.displayOrientedPose,
            projectionMatrix
        )

        render.clear(virtualSceneFramebuffer, 0f, 0f, 0f, 0f)

        if (isRenderStreetscapeGeometry) {
            var index = 0
            for ((streetscapeGeometry, mesh) in streetscapeGeometryToMeshes) {
                if (streetscapeGeometry.trackingState != TrackingState.TRACKING) continue
                val pose: Pose = streetscapeGeometry.meshPose
                pose.toMatrix(modelMatrix, 0)
                Matrix.multiplyMM(modelViewMatrix, 0, viewMatrix, 0, modelMatrix, 0)
                Matrix.multiplyMM(modelViewProjectionMatrix, 0, projectionMatrix, 0, modelViewMatrix, 0)

                if (streetscapeGeometry.type == StreetscapeGeometry.Type.BUILDING) {
                    val color = wallsColor[index % wallsColor.size]
                    index++
                    streetscapeGeometryBuildingShader
                        .setVec4("u_Color", floatArrayOf(color[0], color[1], color[2], color[3]))
                        .setMat4("u_ModelViewProjection", modelViewProjectionMatrix)
                    render.draw(mesh, streetscapeGeometryBuildingShader)
                } else if (streetscapeGeometry.type == StreetscapeGeometry.Type.TERRAIN) {
                    streetscapeGeometryTerrainShader
                        .setVec4("u_Color", floatArrayOf(0f, 0.5f, 0f, 0.3f))
                        .setMat4("u_ModelViewProjection", modelViewProjectionMatrix)
                    render.draw(mesh, streetscapeGeometryTerrainShader)
                }
            }
        }

        render.clear(virtualSceneFramebuffer, 0f, 0f, 0f, 0f)
        synchronized(anchorsLock) {
            for (anchor in anchors) {
                if (anchor.trackingState != TrackingState.TRACKING) continue

                anchor.pose.toMatrix(modelMatrix, 0)
                val scaleMatrix = FloatArray(16)
                Matrix.setIdentityM(scaleMatrix, 0)
                val scale = getScale(anchor.pose, camera.displayOrientedPose)
                scaleMatrix[0] = scale; scaleMatrix[5] = scale; scaleMatrix[10] = scale
                Matrix.multiplyMM(modelMatrix, 0, modelMatrix, 0, scaleMatrix, 0)

                val rotationMatrix = FloatArray(16)
                Matrix.setRotateM(rotationMatrix, 0, 180f, 0f, 1f, 0f)
                val rotationModelMatrix = FloatArray(16)
                Matrix.multiplyMM(rotationModelMatrix, 0, modelMatrix, 0, rotationMatrix, 0)

                Matrix.multiplyMM(modelViewMatrix, 0, viewMatrix, 0, rotationModelMatrix, 0)
                Matrix.multiplyMM(modelViewProjectionMatrix, 0, projectionMatrix, 0, modelViewMatrix, 0)

                if (terrainAnchors.contains(anchor) || rooftopAnchors.contains(anchor)) {
                    terrainAnchorVirtualObjectShader.setMat4("u_ModelViewProjection", modelViewProjectionMatrix)
                    render.draw(virtualObjectMesh, terrainAnchorVirtualObjectShader, virtualSceneFramebuffer)
                } else {
                    geospatialAnchorVirtualObjectShader.setMat4("u_ModelViewProjection", modelViewProjectionMatrix)
                    render.draw(virtualObjectMesh, geospatialAnchorVirtualObjectShader, virtualSceneFramebuffer)
                }
            }

            if (anchors.size > 0) {
                val anchorMessage = resources.getQuantityString(
                    R.plurals.status_anchors_set, anchors.size, anchors.size, MAXIMUM_ANCHORS
                )
                runOnUiThread {
                    statusTextView.visibility = View.VISIBLE
                    statusTextView.text = anchorMessage
                }
            }
        }

        backgroundRenderer.drawVirtualScene(render, virtualSceneFramebuffer, Z_NEAR, Z_FAR)
    }

    // Streetscape geometry

    private fun updateStreetscapeGeometries(streetscapeGeometries: Collection<StreetscapeGeometry>) {
        for (streetscapeGeometry in streetscapeGeometries) {
            if (!streetscapeGeometryToMeshes.containsKey(streetscapeGeometry)) {
                streetscapeGeometryToMeshes[streetscapeGeometry] = getSampleRenderMesh(streetscapeGeometry)
            }
        }
    }

    private fun getSampleRenderMesh(streetscapeGeometry: StreetscapeGeometry): Mesh {
        val buffer = streetscapeGeometry.mesh.vertexList.also { it.rewind() }
        val meshVertexBuffer = VertexBuffer(render, 3, buffer)
        val meshIndexBuffer = IndexBuffer(render, streetscapeGeometry.mesh.indexList)
        return Mesh(render, Mesh.PrimitiveMode.TRIANGLES, meshIndexBuffer, arrayOf(meshVertexBuffer))
    }

    // Geospatial state machine

    private fun updateGeospatialState(earth: Earth) {
        if (earth.earthState != Earth.EarthState.ENABLED) {
            state = State.EARTH_STATE_ERROR
            return
        }
        if (earth.trackingState != TrackingState.TRACKING) {
            state = State.PRETRACKING
            return
        }
        when (state) {
            State.PRETRACKING -> updatePretrackingState(earth)
            State.LOCALIZING -> updateLocalizingState(earth)
            State.LOCALIZED -> updateLocalizedState(earth)
            else -> Unit
        }
    }

    private fun updatePretrackingState(earth: Earth) {
        if (earth.trackingState == TrackingState.TRACKING) {
            state = State.LOCALIZING
            return
        }
        runOnUiThread { geospatialPoseTextView.setText(R.string.geospatial_pose_not_tracking) }
    }

    private fun updateLocalizingState(earth: Earth) {
        val geospatialPose = earth.cameraGeospatialPose
        if (geospatialPose.horizontalAccuracy <= LOCALIZING_HORIZONTAL_ACCURACY_THRESHOLD_METERS
            && geospatialPose.orientationYawAccuracy <= LOCALIZING_ORIENTATION_YAW_ACCURACY_THRESHOLD_DEGREES
        ) {
            state = State.LOCALIZED
            synchronized(anchorsLock) {
                val anchorNum = anchors.size
                if (anchorNum == 0) createAnchorFromSharedPreferences(earth)
                if (anchorNum < MAXIMUM_ANCHORS) {
                    runOnUiThread {
                        setAnchorButton.visibility = View.VISIBLE
                        tapScreenTextView.visibility = View.VISIBLE
                        if (anchorNum > 0) clearAnchorsButton.visibility = View.VISIBLE
                    }
                }
            }
            return
        }

        if (TimeUnit.MILLISECONDS.toSeconds(System.currentTimeMillis() - localizingStartTimestamp)
            > LOCALIZING_TIMEOUT_SECONDS
        ) {
            state = State.LOCALIZING_FAILED
            return
        }

        updateGeospatialPoseText(geospatialPose)
    }

    private fun updateLocalizedState(earth: Earth) {
        val geospatialPose = earth.cameraGeospatialPose
        if (geospatialPose.horizontalAccuracy > LOCALIZING_HORIZONTAL_ACCURACY_THRESHOLD_METERS + LOCALIZED_HORIZONTAL_ACCURACY_HYSTERESIS_METERS
            || geospatialPose.orientationYawAccuracy > LOCALIZING_ORIENTATION_YAW_ACCURACY_THRESHOLD_DEGREES + LOCALIZED_ORIENTATION_YAW_ACCURACY_HYSTERESIS_DEGREES
        ) {
            state = State.LOCALIZING
            localizingStartTimestamp = System.currentTimeMillis()
            runOnUiThread {
                setAnchorButton.visibility = View.INVISIBLE
                tapScreenTextView.visibility = View.INVISIBLE
                clearAnchorsButton.visibility = View.INVISIBLE
            }
            return
        }
        updateGeospatialPoseText(geospatialPose)
    }

    private fun updateGeospatialPoseText(geospatialPose: GeospatialPose) {
        val quaternion = geospatialPose.eastUpSouthQuaternion
        val poseText = resources.getString(
            R.string.geospatial_pose,
            geospatialPose.latitude,
            geospatialPose.longitude,
            geospatialPose.horizontalAccuracy,
            geospatialPose.altitude,
            geospatialPose.verticalAccuracy,
            quaternion[0], quaternion[1], quaternion[2], quaternion[3],
            geospatialPose.orientationYawAccuracy
        )
        runOnUiThread { geospatialPoseTextView.text = poseText }
    }

    // Tap / anchor handling

    private fun handleTap(frame: Frame, cameraTrackingState: TrackingState) {
        synchronized(singleTapLock) {
            synchronized(anchorsLock) {
                if (queuedSingleTap == null || anchors.size >= MAXIMUM_ANCHORS
                    || cameraTrackingState != TrackingState.TRACKING
                ) {
                    queuedSingleTap = null
                    return
                }
            }
            val earth = session?.earth
            if (earth == null || earth.trackingState != TrackingState.TRACKING) {
                queuedSingleTap = null
                return
            }

            for (hit in frame.hitTest(queuedSingleTap)) {
                if (shouldCreateAnchorWithHit(hit)) {
                    val geospatialPose = earth.getGeospatialPose(hit.hitPose)
                    createAnchorWithGeospatialPose(earth, geospatialPose)
                    break
                }
            }
            queuedSingleTap = null
        }
    }

    private fun shouldCreateAnchorWithHit(hit: HitResult): Boolean {
        val trackable: Trackable = hit.trackable
        if (isRenderStreetscapeGeometry && trackable is StreetscapeGeometry) return true
        return when (trackable) {
            is Plane -> trackable.isPoseInPolygon(hit.hitPose)
            is Point -> trackable.orientationMode == OrientationMode.ESTIMATED_SURFACE_NORMAL
            else -> false
        }
    }


    protected fun settingsMenuClick(item: MenuItem): Boolean {
        val itemId = item.itemId
        if (itemId == R.id.anchor_reset) return true
        item.isChecked = !item.isChecked
        sharedPreferences.edit().putInt(ANCHOR_MODE, itemId).apply()
        anchorType = when (itemId) {
            R.id.geospatial -> AnchorType.GEOSPATIAL
            R.id.terrain -> AnchorType.TERRAIN
            R.id.rooftop -> AnchorType.ROOFTOP
            else -> return false
        }
        return true
    }

    private fun createAnchorWithGeospatialPose(earth: Earth, geospatialPose: GeospatialPose) {
        val latitude = geospatialPose.latitude
        val longitude = geospatialPose.longitude
        val altitude = geospatialPose.altitude
        val quaternion = geospatialPose.eastUpSouthQuaternion
        when (anchorType) {
            AnchorType.TERRAIN -> {
                createTerrainAnchor(earth, latitude, longitude, identityQuaternion)
                storeAnchorParameters(latitude, longitude, 0.0, identityQuaternion)
            }
            AnchorType.GEOSPATIAL -> {
                createAnchor(earth, latitude, longitude, altitude, quaternion)
                storeAnchorParameters(latitude, longitude, altitude, quaternion)
            }
            AnchorType.ROOFTOP -> {
                createRooftopAnchor(earth, latitude, longitude, identityQuaternion)
                storeAnchorParameters(latitude, longitude, 0.0, identityQuaternion)
            }
        }
        runOnUiThread { clearAnchorsButton.visibility = View.VISIBLE }
        clearedAnchorsAmount = null
    }

    private fun handleClearAnchorsButton() {
        synchronized(anchorsLock) {
            clearedAnchorsAmount = anchors.size
            val message = resources.getQuantityString(
                R.plurals.status_anchors_cleared, clearedAnchorsAmount!!, clearedAnchorsAmount
            )
            statusTextView.visibility = View.VISIBLE
            statusTextView.text = message
            anchors.forEach { it.detach() }
            anchors.clear()
        }
        clearAnchorsFromSharedPreferences()
        clearAnchorsButton.visibility = View.INVISIBLE
        setAnchorButton.visibility = View.VISIBLE
        tapScreenTextView.visibility = View.VISIBLE
    }

    // Anchor creation helpers

    private fun createAnchor(earth: Earth, latitude: Double, longitude: Double, altitude: Double, quaternion: FloatArray) {
        val anchor = earth.createAnchor(latitude, longitude, altitude, quaternion[0], quaternion[1], quaternion[2], quaternion[3])
        synchronized(anchorsLock) { anchors.add(anchor) }
    }

    private fun createTerrainAnchor(earth: Earth, latitude: Double, longitude: Double, quaternion: FloatArray) {
        earth.resolveAnchorOnTerrainAsync(
            latitude, longitude, 0.0,
            quaternion[0], quaternion[1], quaternion[2], quaternion[3]
        ) { anchor, anchorState ->
            if (anchorState == TerrainAnchorState.SUCCESS) {
                synchronized(anchorsLock) { anchors.add(anchor); terrainAnchors.add(anchor) }
            } else {
                statusTextView.visibility = View.VISIBLE
                statusTextView.text = getString(R.string.status_terrain_anchor, anchorState)
            }
        }
    }

    private fun createRooftopAnchor(earth: Earth, latitude: Double, longitude: Double, quaternion: FloatArray) {
        earth.resolveAnchorOnRooftopAsync(
            latitude, longitude, 0.0,
            quaternion[0], quaternion[1], quaternion[2], quaternion[3]
        ) { anchor, anchorState ->
            if (anchorState == RooftopAnchorState.SUCCESS) {
                synchronized(anchorsLock) { anchors.add(anchor); rooftopAnchors.add(anchor) }
            } else {
                statusTextView.visibility = View.VISIBLE
                statusTextView.text = getString(R.string.status_rooftop_anchor, anchorState)
            }
        }
    }

    // SharedPreferences helpers

    private fun storeAnchorParameters(latitude: Double, longitude: Double, altitude: Double, quaternion: FloatArray) {
        val existing = sharedPreferences.getStringSet(SHARED_PREFERENCES_SAVED_ANCHORS, emptySet())!!
        val updated = HashSet<String>(existing)
        val prefix = when (anchorType) {
            AnchorType.TERRAIN -> "Terrain"
            AnchorType.ROOFTOP -> "Rooftop"
            else -> ""
        }
        updated.add(
            String.format(
                "${prefix}%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f",
                latitude, longitude, altitude,
                quaternion[0], quaternion[1], quaternion[2], quaternion[3]
            )
        )
        sharedPreferences.edit().putStringSet(SHARED_PREFERENCES_SAVED_ANCHORS, updated).apply()
    }

    private fun clearAnchorsFromSharedPreferences() {
        sharedPreferences.edit().putStringSet(SHARED_PREFERENCES_SAVED_ANCHORS, null).apply()
    }

    private fun createAnchorFromSharedPreferences(earth: Earth) {
        val anchorParameterSet = sharedPreferences.getStringSet(SHARED_PREFERENCES_SAVED_ANCHORS, null) ?: return
        for (rawParams in anchorParameterSet) {
            var params = rawParams
            val type = when {
                params.contains("Terrain") -> { params = params.replace("Terrain", ""); AnchorType.TERRAIN }
                params.contains("Rooftop") -> { params = params.replace("Rooftop", ""); AnchorType.ROOFTOP }
                else -> AnchorType.GEOSPATIAL
            }
            val parts = params.split(",")
            if (parts.size != 7) {
                Log.d(TAG, "Invalid number of anchor parameters. Expected four, found ${parts.size}")
                continue
            }
            val latitude = parts[0].toDouble()
            val longitude = parts[1].toDouble()
            val altitude = parts[2].toDouble()
            val quaternion = floatArrayOf(parts[3].toFloat(), parts[4].toFloat(), parts[5].toFloat(), parts[6].toFloat())
            when (type) {
                AnchorType.TERRAIN -> createTerrainAnchor(earth, latitude, longitude, quaternion)
                AnchorType.ROOFTOP -> createRooftopAnchor(earth, latitude, longitude, quaternion)
                else -> createAnchor(earth, latitude, longitude, altitude, quaternion)
            }
        }
        runOnUiThread { clearAnchorsButton.visibility = View.VISIBLE }
    }

    // Scale helper

    private fun getScale(anchorPose: Pose, cameraPose: Pose): Float {
        val distance = Math.sqrt(
            Math.pow((anchorPose.tx() - cameraPose.tx()).toDouble(), 2.0)
                    + Math.pow((anchorPose.ty() - cameraPose.ty()).toDouble(), 2.0)
                    + Math.pow((anchorPose.tz() - cameraPose.tz()).toDouble(), 2.0)
        )
        val mapDistance = distance.coerceIn(2.0, 20.0)
        return ((mapDistance - 2) / (20 - 2) + 1).toFloat()
    }

    // Dialog callbacks

    override fun onDialogPositiveClick(dialog: DialogFragment) {
        if (!sharedPreferences.edit().putBoolean(ALLOW_GEOSPATIAL_ACCESS_KEY, true).commit()) {
            throw AssertionError("Could not save the user preference to SharedPreferences!")
        }
        createSession()
    }

    override fun onDialogContinueClick(dialog: DialogFragment) {
        dialog.dismiss()
    }

    private fun onRenderStreetscapeGeometryChanged(button: CompoundButton, isChecked: Boolean) {
        if (session == null) return
        isRenderStreetscapeGeometry = isChecked
    }
}