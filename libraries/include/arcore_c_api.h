/*
 * Copyright 2017 Google LLC
 *
 * Licensed for use under "ARCore Additional Terms of Service". You may obtain
 * a copy of the license at https://developers.google.com/ar/develop/terms.
 */
#ifndef ARCORE_C_API_H_
#define ARCORE_C_API_H_

#include <stddef.h>
#include <stdint.h>

/// @defgroup concepts Concepts
/// High-Level ARCore concepts.
///
/// @section ownership Object ownership
///
/// ARCore objects fall into two categories:
///
/// - <b>Value types</b> are owned by application. They are created and
///   destroyed using the @c create / @c destroy functions, and are populated by
///   ARCore using functions with @c get in the function name.
///
/// - <b>Reference types</b> are owned by ARCore. A reference is acquired by one
///   of the @c acquire functions.  For each call to the @c acquire function,
///   the application must call the matching @c release function. Note that even
///   if the last reference is released, ARCore may continue to hold a reference
///   to the object at ARCore's discretion.
///
/// Reference types are further split into:
///
/// - <b>Long-lived objects</b>. These objects persist across frames, possibly
///   for the life span of the application or session. Acquire may fail if
///   ARCore is in an incorrect state, e.g. not tracking. Acquire from list
///   always succeeds, as the object already exists.
///
/// - <b>Transient large data</b>. These objects are usually acquired per-frame
///   and are a limited resource. The @c acquire call may fail if the resource
///   is exhausted (too many are currently held), deadline exceeded (the target
///   of the acquire was already released), or the resource is not yet
///   available.
///
/// Note: Lists are value types (owned by application), but can hold references
/// to long-lived objects. This means that the references held by a list are not
/// released until either the list is destroyed, or is re-populated by another
/// api call.
///
/// For example, @c ::ArAnchorList, which is a value type, will hold references
/// to anchors, which are long-lived objects.
///
/// @section spaces Poses and coordinate spaces
///
/// An @c ::ArPose describes an rigid transformation from one coordinate space
/// to another. As provided from all ARCore APIs, poses always describe the
/// transformation from object's local coordinate space to the <b>world
/// coordinate space</b> (see below). That is, poses from ARCore APIs can be
/// thought of as equivalent to OpenGL model matrices.
///
/// The transformation is defined using a quaternion rotation about the origin
/// followed by a translation.
///
/// The coordinate system is right-handed, like OpenGL conventions.
///
/// Translation units are meters.
///
/// @section worldcoordinates World coordinate space
///
/// As ARCore's understanding of the environment changes, it adjusts its model
/// of the world to keep things consistent. When this happens, the numerical
/// location (coordinates) of the camera and anchors can change significantly to
/// maintain appropriate relative positions of the physical locations they
/// represent.
///
/// These changes mean that every frame should be considered to be in a
/// completely unique world coordinate space. The numerical coordinates of
/// anchors and the camera should never be used outside the rendering frame
/// during which they were retrieved. If a position needs to be considered
/// beyond the scope of a single rendering frame, either an anchor should be
/// created or a position relative to a nearby existing anchor should be used.

/// @defgroup shared_types Shared types and enums
/// Shared types and constants.

/// @defgroup utility_functions Utility functions
/// Utility functions for releasing data.

#ifdef __cplusplus
/// @defgroup type_conversions C++ type conversions
/// These functions expose allowable type conversions as C++ helper functions.
/// This avoids having to explicitly @c reinterpret_cast in most cases.
///
/// Note: These functions only change the type of a pointer; they do not change
/// the reference count of the referenced objects.
///
/// Note: There is no runtime checking that casts are correct. When downcasting
/// @c ::ArTrackable, call @c ::ArTrackable_getType beforehand to figure out the
/// correct cast.
#endif  // __cplusplus

/// @defgroup ArAnchor ArAnchor
/// Describes a fixed location and orientation in the real world, representing
/// local and Cloud Anchors.

/// @defgroup ArAugmentedFace ArAugmentedFace
/// Describes a face detected by ARCore and provides functions to access
/// additional center and face region poses as well as face mesh related data.
///
/// Augmented Faces supports front-facing (selfie) camera only, and does not
/// support attaching anchors nor raycast hit testing. Calling
/// @c ::ArTrackable_acquireNewAnchor will return @c #AR_ERROR_ILLEGAL_STATE.

/// @defgroup ArAugmentedImage ArAugmentedImage
/// An image being detected and tracked by ARCore.

/// @defgroup ArAugmentedImageDatabase ArAugmentedImageDatabase
/// Database containing a list of images to be detected and tracked by ARCore.

/// @defgroup ArCamera ArCamera
/// Provides information about the camera that is used to capture images.

/// @defgroup ArCameraConfig ArCameraConfig
/// Camera configuration.

/// @defgroup ArCameraConfigFilter ArCameraConfigFilter
/// Filters available camera configurations.

/// @defgroup ArCameraIntrinsics ArCameraIntrinsics
/// Provides information about the physical characteristics of the device
/// camera.

/// @defgroup ArFuture ArFuture
/// Futures in ARCore.
///
/// @section future_concept Futures in ARCore
///
/// Futures represent the eventual completion of an asynchronous
/// operation. A future has one of three states, @c ::ArFutureState, which can
/// be obtained with @c ::ArFuture_getState:
///
/// - @c #AR_FUTURE_STATE_PENDING - The operation is still pending. The result
///   of the operation isn't available yet and any associated callback hasn't
///   yet been invoked.
/// - @c #AR_FUTURE_STATE_DONE - The operation is complete, and a result is
///   available.
/// - @c #AR_FUTURE_STATE_CANCELLED - The operation has been cancelled.
///
/// An @c ::ArFuture starts in the @c #AR_FUTURE_STATE_PENDING state and
/// transitions to @c #AR_FUTURE_STATE_DONE upon completion. If the
/// future is cancelled using @c ::ArFuture_cancel, then its state may become @c
/// #AR_FUTURE_STATE_CANCELLED (see @ref future_cancellation
/// "cancelling a future" for caveats).
///
/// Futures must eventually be released using @c ::ArFuture_release.
/// (@ref ownership "reference type, long-lived").
///
/// @section future_results Obtaining results from a Future
///
/// There are two ways of obtaining results from an @c ::ArFuture:
///
/// @subsection future_polling Polling a Future
///
/// When the @c ::ArFuture is created, its @c ::ArFutureState is set to @c
/// #AR_FUTURE_STATE_PENDING. You may poll the future using @c
/// ::ArFuture_getState to query the state of the asynchronous operation. When
/// its state is @c #AR_FUTURE_STATE_DONE, you can obtain the operation's
/// result. The future must eventually be released using @c ::ArFuture_release.
///
/// @subsection future_callback Using a callback to obtain Future results
///
/// The operation's result can be reported via a callback. When providing a
/// callback, ARCore will invoke the given function when the operation is
/// complete, unless the future has been cancelled using @c ::ArFuture_cancel.
/// This callback will be invoked on the <a
/// href="https://developer.android.com/guide/components/processes-and-threads#Threads">main
/// thread</a>.
///
/// When providing a callback, you may provide a
/// @c context, which will be passed as the first parameter to the callback.
/// It is a best practice to free the memory of @c context at the end of the
/// callback and when @c ::ArFuture_cancel successfully cancels the callback.
///
/// @section future_cancellation Cancelling a Future
///
/// You can try to cancel an @c ::ArFuture by calling @c ::ArFuture_cancel.
/// Due to multi-threading, it is possible that the cancel operation is not
/// successful. The @c out_was_cancelled parameter indicates if the cancellation
/// was successful.
///
/// If the cancellation is successful, then any
/// @ref future_callback "associated callback" will never be called. It is a
/// best practice to free context memory
/// provided to the callback, if any.
///
/// @defgroup ArConfig ArConfig
/// Session configuration.
///
/// To configure an @c ::ArSession:
///
/// 1. Use @c ::ArConfig_create to create an @c ::ArConfig object.
/// 2. Call any number of configuration functions on the newly created object.
/// 3. To apply the configuration to the session, use @c ::ArSession_configure.
/// 4. To release the memory used by the @c ::ArConfig object, use
///    @c ::ArConfig_destroy.
///
/// Note: None of the `ArConfig_set*()` functions will actually affect the state
/// of the given @c ::ArSession until @c ::ArSession_configure is called.

/// @defgroup ArCoreApk ArCoreApk
/// Functions for installing and updating "Google Play Services for AR" (ARCore)
/// and determining whether the current device is an
/// <a href="https://developers.google.com/ar/devices">ARCore
/// supported device</a>.

/// @defgroup ArDepthPoint ArDepthPoint
/// Hit Depth API

/// @defgroup ArEarth ArEarth
/// A @c ::ArTrackable implementation representing the Earth. Provides
/// localization ability in geospatial coordinates.
///
/// To access @c ::ArEarth, configure the session with an appropriate @c
/// ::ArGeospatialMode and use @c ::ArSession_acquireEarth.
///
/// Not all devices support @c #AR_GEOSPATIAL_MODE_ENABLED, use
/// @c ::ArSession_isGeospatialModeSupported to check if the current device and
/// selected camera support enabling this mode.
///
/// @c ::ArEarth should only be used when its @c ::ArTrackingState is @c
/// #AR_TRACKING_STATE_TRACKING, and otherwise should not be used. Use @c
/// ::ArTrackable_getTrackingState to obtain the current @c ::ArTrackingState.
/// If the @c ::ArTrackingState does not become @c #AR_TRACKING_STATE_TRACKING,
/// then @c ::ArEarth_getEarthState may contain more information as @c
/// ::ArEarthState.
///
/// Use @c ::ArEarth_getCameraGeospatialPose to obtain the Earth-relative
/// virtual camera pose for the latest frame.
///
/// Use @c ::ArEarth_acquireNewAnchor to attach anchors to Earth. Calling @c
/// ::ArTrackable_acquireNewAnchor with an @c ::ArEarth instance will
/// fail to create a new anchor and will return the @c
/// #AR_ERROR_INVALID_ARGUMENT error code.
///
/// @c ::ArEarth does not support hit testing. Because @c ::ArEarth
/// is a type of @c ::ArTrackable, the singleton @c ::ArEarth instance may also
/// be returned by @c ::ArSession_getAllTrackables when enabled.

/// @defgroup ArGeospatialPose ArGeospatialPose
/// Describes a specific location, elevation, and compass heading relative to
/// Earth (@ref ownership "value type"). It is comprised of:
///
/// - Latitude and longitude are specified in degrees, with positive values
///   being north of the equator and east of the prime meridian as defined by
///   the <a href="https://en.wikipedia.org/wiki/World_Geodetic_System">WGS84
///   specification</a>.
/// - Altitude is specified in meters above the WGS84 ellipsoid, which is
///   roughly equivalent to meters above sea level.
/// - Orientation approximates the direction the user is facing in the EUS
///   coordinate system. The EUS coordinate system has X+ pointing east, Y+
///   pointing up, and Z+ pointing south.
/// - Accuracy of the latitude, longitude, altitude, and heading are available
///   as numeric confidence intervals where a large value (large interval) means
///   low confidence and small value (small interval) means high confidence.
///
/// An @c ::ArGeospatialPose can be retrieved from @c
/// ::ArEarth_getCameraGeospatialPose.
///

/// @defgroup ArSemanticLabel ArSemanticLabel
/// Scene Semantics API. See the
/// <a href="https://developers.google.com/ar/develop/c/scene-semantics">Scene
/// Semantics Developer Guide</a> for more information.

/// @defgroup ArStreetscapeGeometry ArStreetscapeGeometry
/// ARCore Geospatial Streetscape Geometry APIs. See the <a
/// href="https://developers.google.com/ar/develop/c/geospatial/streetscape-geometry">Streetscape
/// Geometry Developer Guide</a> for additional information.

/// @defgroup ArFrame ArFrame
/// Per-frame state.

/// @defgroup ArHitResult ArHitResult
/// Defines an intersection between a ray and estimated real-world geometry.

/// @defgroup ArImage ArImage
/// Provides access to CPU camera images.

/// @defgroup ArImageMetadata ArImageMetadata
/// Provides access to CPU image camera metadata.

/// @defgroup ArInstantPlacementPoint ArInstantPlacementPoint
/// Trackable Instant Placement point returned by
/// @c ::ArFrame_hitTestInstantPlacement.
///
/// If ARCore has an accurate 3D pose for the @c ::ArInstantPlacementPoint
/// returned by @c ::ArFrame_hitTestInstantPlacement it will start with tracking
/// method @c #AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_FULL_TRACKING.
/// Otherwise, it will start with tracking method
/// @c
/// #AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_SCREENSPACE_WITH_APPROXIMATE_DISTANCE,<!--NOLINT-->
/// and will transition to
/// @c #AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_FULL_TRACKING
/// once ARCore has an accurate 3D pose. Once the tracking method is
/// @c #AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_FULL_TRACKING it will not
/// revert to @c
/// #AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_SCREENSPACE_WITH_APPROXIMATE_DISTANCE.<!--NOLINT-->
///
/// When the tracking method changes from
/// @c
/// #AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_SCREENSPACE_WITH_APPROXIMATE_DISTANCE<!--NOLINT-->
/// in one frame to @c #AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_FULL_TRACKING
/// in the next frame, the pose will jump from its initial location based on the
/// provided approximate distance to a new location at an accurate distance.
///
/// This instantaneous change in pose will change the apparent scale of any
/// objects that are anchored to the @c ::ArInstantPlacementPoint. That is, an
/// object will suddenly appear larger or smaller than it was in the previous
/// frame.
///
/// To avoid the visual jump due to the sudden change in apparent object scale,
/// use the following procedure:
/// 1. Keep track of the pose and tracking method of the
///    @c ::ArInstantPlacementPoint in each frame.
/// 2. Wait for the tracking method to change to
///    @c #AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_FULL_TRACKING.
/// 3. Use the pose from the previous frame and the pose in the current frame to
///    determine the object's distance to the device in both frames.
/// 4. Calculate the apparent change in scale due to the change in distance
///    from the camera.
/// 5. Adjust the scale of the object to counteract the perceived change in
///    scale, so that visually the object does not appear to change in size.
/// 6. Optionally, smoothly adjust the scale of the object back to its original
///    value over several frames.

/// @defgroup ArLightEstimate ArLightEstimate
/// Holds information about the estimated lighting of the real scene.

/// @defgroup ArPlane ArPlane
/// Describes the current best knowledge of a real-world planar surface.

/// @defgroup ArPoint ArPoint
/// Represents a point in space that ARCore is tracking.

/// @defgroup ArPointCloud ArPointCloud
/// Contains a set of observed 3D points and confidence values.

/// @defgroup ArPose ArPose
/// Represents an immutable rigid transformation from one coordinate
/// space to another.

/// @defgroup ArRecordingConfig ArRecordingConfig
/// Session recording management.

/// @defgroup ArTrack ArTrack
/// External data recording.

/// @defgroup ArTrackData ArTrackData
/// External data track, used by Recording and Playback API.

/// @defgroup ArSession ArSession
/// Session management.

/// @defgroup ArMesh ArMesh
/// Represents a polygon mesh describing geometry.
///
/// Obtained by @c ::ArStreetscapeGeometry_acquireMesh.

/// @defgroup ArTrackable ArTrackable
/// Something that can be tracked and that anchors can be attached to.

/// @defgroup ArVpsAvailabilityFuture ArVpsAvailabilityFuture
/// An asynchronous operation checking VPS availability. The availability of VPS
/// in a given location helps to improve the quality of Geospatial localization
/// and tracking accuracy. See @c ::ArSession_checkVpsAvailabilityAsync for more
/// details.

/// @defgroup ArHostCloudAnchorFuture ArHostCloudAnchorFuture
/// An asynchronous operation for hosting a Cloud Anchor launched by @c
/// ::ArSession_hostCloudAnchorAsync. See the <a
/// href="https://developers.google.com/ar/develop/c/cloud-anchors/developer-guide">Cloud
/// Anchors developer guide</a> for more information.

/// @defgroup ArResolveCloudAnchorFuture ArResolveCloudAnchorFuture
/// An asynchronous operation for resolving a Cloud Anchor launched by @c
/// ::ArSession_resolveCloudAnchorAsync. See the <a
/// href="https://developers.google.com/ar/develop/c/cloud-anchors/developer-guide">Cloud
/// Anchors developer guide</a> for more information.

/// @defgroup ArResolveAnchorOnTerrainFuture ArResolveAnchorOnTerrainFuture
/// An asynchronous operation for resolving a terrain anchor launched by
/// @c ::ArEarth_resolveAnchorOnTerrainAsync. See the <a
/// href="https://developers.google.com/ar/develop/geospatial/c/anchors#terrain-anchors">Terrain
/// anchors developer guide</a> for more information.

/// @defgroup ArResolveAnchorOnRooftopFuture ArResolveAnchorOnRooftopFuture
/// Handle to an async operation launched by @c
/// ::ArEarth_resolveAnchorOnRooftopAsync. See the <a
/// href="https://developers.google.com/ar/develop/geospatial/c/anchors#rooftop-anchors">Rooftop
/// anchors developer guide</a> for more information.

/// @ingroup ArConfig
/// An opaque session configuration object (@ref ownership "value type").
///
/// - Create with: @c ::ArConfig_create
/// - Release with: @c ::ArConfig_destroy
typedef struct ArConfig_ ArConfig;

// CameraConfig objects and list.

/// @ingroup ArCameraConfig
/// A camera config struct that contains the config supported by
/// the physical camera obtained from the low level device profiles.
/// (@ref ownership "value type").
///
/// - Create with: @c ::ArCameraConfig_create
/// - Release with: @c ::ArCameraConfig_destroy
typedef struct ArCameraConfig_ ArCameraConfig;

/// @ingroup ArCameraConfig
/// A list of camera config (@ref ownership "value type").
///
/// - Create with: @c ::ArCameraConfigList_create
/// - Release with: @c ::ArCameraConfigList_destroy
typedef struct ArCameraConfigList_ ArCameraConfigList;

// Shared Camera objects definition.
// Excluded from generated docs (// vs ///) since it's a detail of the Java SDK.
//
// A shared camera contains functions that require sending Java objects over
// the c/c++ interface. To avoid using void* and making code clarity that the
// Java object is being just transmitted we define a new typedef.
typedef void *ArJavaObject;

// Camera config filters and camera config filters objects.

/// @ingroup ArCameraConfigFilter
/// A camera config filter struct contains the filters that are desired
/// by the application. (@ref ownership "value type").
///
/// - Create with: @c ::ArCameraConfigFilter_create
/// - Release with: @c ::ArCameraConfigFilter_destroy
typedef struct ArCameraConfigFilter_ ArCameraConfigFilter;

/// @ingroup ArRecordingConfig
/// A recording config struct that contains the config to set the recorder.
///
/// (@ref ownership "value type").
///
/// - Create with: @c ::ArRecordingConfig_create
/// - Release with: @c ::ArRecordingConfig_destroy
typedef struct ArRecordingConfig_ ArRecordingConfig;

/// @ingroup ArTrack
/// A track recording configuration struct of type data.
///
/// (@ref ownership "value type").
///
/// - Create with: @c ::ArTrack_create
/// - Release with: @c ::ArTrack_destroy
typedef struct ArTrack_ ArTrack;

/// @ingroup ArSession
/// The ARCore session (@ref ownership "value type").
///
/// - Create with: @c ::ArSession_create
/// - Release with: @c ::ArSession_destroy
typedef struct ArSession_ ArSession;

/// @ingroup ArPose
/// A structured rigid transformation (@ref ownership "value type").
///
/// - Allocate with: @c ::ArPose_create
/// - Release with: @c ::ArPose_destroy
typedef struct ArPose_ ArPose;

// Camera.

/// @ingroup ArCamera
/// The virtual and physical camera
/// (@ref ownership "reference type, long-lived").
///
/// - Acquire with: @c ::ArFrame_acquireCamera
/// - Release with: @c ::ArCamera_release
typedef struct ArCamera_ ArCamera;

// === Camera intrinstics types and functions ===

/// @ingroup ArCameraIntrinsics
/// The physical characteristics of a given camera.
///
/// - Allocate with: @c ::ArCameraIntrinsics_create
/// - Release with: @c ::ArCameraIntrinsics_destroy
typedef struct ArCameraIntrinsics_ ArCameraIntrinsics;

// Frame and frame objects.

/// @ingroup ArFrame
/// The world state resulting from an update (@ref ownership "value type").
///
/// - Create with: @c ::ArFrame_create
/// - Allocate with: @c ::ArSession_update
/// - Release with: @c ::ArFrame_destroy
typedef struct ArFrame_ ArFrame;

// LightEstimate.

/// @ingroup ArLightEstimate
/// An estimate of the real-world lighting (@ref ownership "value type").
///
/// - Create with: @c ::ArLightEstimate_create
/// - Allocate with: @c ::ArFrame_getLightEstimate
/// - Release with: @c ::ArLightEstimate_destroy
typedef struct ArLightEstimate_ ArLightEstimate;

// PointCloud.

/// @ingroup ArPointCloud
/// A cloud of tracked 3D visual feature points
/// (@ref ownership "reference type, large data").
///
/// - Acquire with: @c ::ArFrame_acquirePointCloud
/// - Release with: @c ::ArPointCloud_release
typedef struct ArPointCloud_ ArPointCloud;

// ImageMetadata.

/// @ingroup ArImageMetadata
/// Camera capture metadata (@ref ownership "reference type, large data").
///
/// - Acquire with: @c ::ArFrame_acquireImageMetadata
/// - Release with: @c ::ArImageMetadata_release
typedef struct ArImageMetadata_ ArImageMetadata;

/// @ingroup ArImage
/// Accessing CPU image from the camera
/// (@ref ownership "reference type, large data").
///
/// - Acquire with: @c ::ArFrame_acquireCameraImage
/// - Release with: @c ::ArImage_release.
typedef struct ArImage_ ArImage;

/// @ingroup ArImage
/// Convenient definition for cubemap image storage where it is a fixed size
/// array of 6 @c ::ArImage.
typedef ArImage *ArImageCubemap[6];

// Trackables.

/// @ingroup ArTrackable
/// Trackable base type (@ref ownership "reference type, long-lived").
typedef struct ArTrackable_ ArTrackable;

/// @ingroup ArTrackable
/// A list of @c ::ArTrackable's (@ref ownership "value type").
///
/// - Create with: @c ::ArTrackableList_create
/// - Release with: @c ::ArTrackableList_destroy
typedef struct ArTrackableList_ ArTrackableList;

/// @ingroup ArTrackData
/// Data that has been recorded to an external track.
typedef struct ArTrackData_ ArTrackData;

/// @ingroup ArTrackData
/// A list of @c ::ArTrackData
typedef struct ArTrackDataList_ ArTrackDataList;

// Planes.

/// @ingroup ArPlane
/// A detected planar surface (@ref ownership "reference type, long-lived").
///
/// - Trackable type: @c #AR_TRACKABLE_PLANE
/// - Release with: @c ::ArTrackable_release
typedef struct ArPlane_ ArPlane;

// Points.

/// @ingroup ArPoint
/// An arbitrary point in space (@ref ownership "reference type, long-lived").
///
/// - Trackable type: @c #AR_TRACKABLE_POINT
/// - Release with: @c ::ArTrackable_release
typedef struct ArPoint_ ArPoint;

// Depth Points.

/// @ingroup ArDepthPoint
/// A point measurement taken from a depth image
/// (@ref ownership "reference type, long-lived").
///
/// - Trackable type: @c #AR_TRACKABLE_DEPTH_POINT
/// - Release with: @c ::ArTrackable_release
typedef struct ArDepthPoint_ ArDepthPoint;

// Instant Placement points.

/// @ingroup ArInstantPlacementPoint
/// (@ref ownership "reference type, long-lived").
///
/// - Trackable type: @c #AR_TRACKABLE_INSTANT_PLACEMENT_POINT
/// - Release with: @c ::ArTrackable_release
typedef struct ArInstantPlacementPoint_ ArInstantPlacementPoint;

// Augmented Images.

/// @ingroup ArAugmentedImage
/// An image that has been detected and tracked
/// (@ref ownership "reference type, long-lived").
///
/// - Trackable type: @c #AR_TRACKABLE_AUGMENTED_IMAGE
/// - Release with: @c ::ArTrackable_release
typedef struct ArAugmentedImage_ ArAugmentedImage;

// Augmented Faces.

/// @ingroup ArAugmentedFace
/// A detected face trackable (@ref ownership "reference type, long-lived").
///
/// - Trackable type: @c #AR_TRACKABLE_FACE
/// - Release with: @c ::ArTrackable_release
typedef struct ArAugmentedFace_ ArAugmentedFace;

/// @ingroup ArStreetscapeGeometry
/// A Streetscape Geometry trackable (@ref ownership "reference type,
/// long-lived").
///
/// Defines geometry such as terrain, buildings, or other structures obtained
/// from the Streetscape Geometry API. See the <a
/// href="https://developers.google.com/ar/develop/c/geospatial/streetscape-geometry">Streetscape
/// Geometry Developer Guide</a> for additional information.
///
/// <p>Obtained from a call to @c ::ArSession_getAllTrackables
/// and @c ::ArFrame_getUpdatedTrackables when @c ::ArStreetscapeGeometryMode is
/// set to @c #AR_STREETSCAPE_GEOMETRY_MODE_ENABLED and @c ::ArGeospatialMode is
/// set to @c #AR_GEOSPATIAL_MODE_ENABLED.
///
/// - Trackable type: @c #AR_TRACKABLE_STREETSCAPE_GEOMETRY
/// - Release with: @c ::ArTrackable_release
typedef struct ArStreetscapeGeometry_ ArStreetscapeGeometry;

// Earth.

/// @ingroup ArEarth
/// The Earth trackable (@ref ownership "reference type, long-lived").
///
/// - Trackable type: @c #AR_TRACKABLE_EARTH
/// - Acquire with: @c ::ArSession_acquireEarth
/// - Release with: @c ::ArTrackable_release
typedef struct ArEarth_ ArEarth;

// Geospatial Pose.

/// @ingroup ArGeospatialPose
/// Describes a specific location, elevation, and orientation relative to
/// Earth (@ref ownership "value type").
///
/// - Create with: @c ::ArGeospatialPose_create
/// - Release with: @c ::ArGeospatialPose_destroy
typedef struct ArGeospatialPose_ ArGeospatialPose;

// Augmented Images database

/// @ingroup ArAugmentedImageDatabase
/// A database of images to be detected and tracked by ARCore (@ref ownership
/// "value type").
///
/// An image database supports up to 1000 images. A database can be generated by
/// the `arcoreimg` command-line database generation tool provided in the SDK,
/// or dynamically created at runtime by adding individual images.
///
/// Only one image database can be active in a session. Any images in the
/// currently active image database that have a
/// @c #AR_TRACKING_STATE_TRACKING or @c #AR_TRACKING_STATE_PAUSED state will
/// immediately be set to the @c #AR_TRACKING_STATE_STOPPED state if a different
/// or @c NULL image database is made active in the current session Config.
///
/// - Create with: @c ::ArAugmentedImageDatabase_create or
///   @c ::ArAugmentedImageDatabase_deserialize
/// - Release with: @c ::ArAugmentedImageDatabase_destroy
typedef struct ArAugmentedImageDatabase_ ArAugmentedImageDatabase;

// Anchors.

/// @ingroup ArAnchor
/// A position in space attached to a trackable
/// (@ref ownership "reference type, long-lived").
///
/// - To create a new anchor call @c ::ArSession_acquireNewAnchor or
///   @c ::ArHitResult_acquireNewAnchor.
/// - To have ARCore stop tracking the anchor, call @c ::ArAnchor_detach.
/// - To release the memory associated with this anchor reference, call
///   @c ::ArAnchor_release. Note that this will not cause ARCore to stop
///   tracking the anchor. Other references to the same anchor acquired through
///   @c ::ArAnchorList_acquireItem are unaffected.
typedef struct ArAnchor_ ArAnchor;

/// @ingroup ArAnchor
/// A list of anchors (@ref ownership "value type").
///
/// - Create with: @c ::ArAnchorList_create
/// - Release with: @c ::ArAnchorList_destroy
typedef struct ArAnchorList_ ArAnchorList;

/// @ingroup ArMesh
/// A triangulated mesh representing a surface reconstruction of the scene.
/// (@ref ownership "reference type, large data").
///
/// - Release with: @c ::ArMesh_release
typedef struct ArMesh_ ArMesh;

// Hit result functionality.

/// @ingroup ArHitResult
/// A single trackable hit (@ref ownership "value type").
///
/// - Create with: @c ::ArHitResult_create
/// - Allocate with: @c ::ArHitResultList_getItem
/// - Release with: @c ::ArHitResult_destroy
typedef struct ArHitResult_ ArHitResult;

/// @ingroup ArHitResult
/// A list of hit test results (@ref ownership "value type").
///
/// - Create with: @c ::ArHitResultList_create
/// - Release with: @c ::ArHitResultList_destroy
typedef struct ArHitResultList_ ArHitResultList;

#ifdef __cplusplus
/// @ingroup type_conversions
/// Upcasts to @c ::ArTrackable
inline ArTrackable *ArAsTrackable(ArPlane *plane) {
  return reinterpret_cast<ArTrackable *>(plane);
}

/// @ingroup type_conversions
/// Upcasts to @c ::ArTrackable
inline ArTrackable *ArAsTrackable(ArPoint *point) {
  return reinterpret_cast<ArTrackable *>(point);
}

/// @ingroup type_conversions
/// Upcasts to @c ::ArTrackable
inline ArTrackable *ArAsTrackable(ArAugmentedImage *augmented_image) {
  return reinterpret_cast<ArTrackable *>(augmented_image);
}

/// @ingroup type_conversions
/// Downcasts to @c ::ArPlane.
inline ArPlane *ArAsPlane(ArTrackable *trackable) {
  return reinterpret_cast<ArPlane *>(trackable);
}

/// @ingroup type_conversions
/// Downcasts to @c ::ArPoint.
inline ArPoint *ArAsPoint(ArTrackable *trackable) {
  return reinterpret_cast<ArPoint *>(trackable);
}

/// @ingroup type_conversions
/// Upcasts to @c ::ArTrackable.
inline ArTrackable *ArAsTrackable(
    ArInstantPlacementPoint *instant_placement_point) {
  return reinterpret_cast<ArTrackable *>(instant_placement_point);
}

/// @ingroup type_conversions
/// Downcasts to @c ::ArInstantPlacementPoint.
inline ArInstantPlacementPoint *ArAsInstantPlacementPoint(
    ArTrackable *trackable) {
  return reinterpret_cast<ArInstantPlacementPoint *>(trackable);
}

/// @ingroup type_conversions
/// Downcasts to @c ::ArAugmentedImage.
inline ArAugmentedImage *ArAsAugmentedImage(ArTrackable *trackable) {
  return reinterpret_cast<ArAugmentedImage *>(trackable);
}

/// @ingroup type_conversions
/// Upcasts to @c ::ArTrackable
inline ArTrackable *ArAsTrackable(ArAugmentedFace *face) {
  return reinterpret_cast<ArTrackable *>(face);
}

/// @ingroup type_conversions
/// Downcasts to @c ::ArAugmentedFace
inline ArAugmentedFace *ArAsFace(ArTrackable *trackable) {
  return reinterpret_cast<ArAugmentedFace *>(trackable);
}

/// @ingroup type_conversions
/// Upcasts to @c ::ArTrackable
inline ArTrackable *ArAsTrackable(ArStreetscapeGeometry *streetscape_geometry) {
  return reinterpret_cast<ArTrackable *>(streetscape_geometry);
}

/// @ingroup type_conversions
/// Downcasts to @c ::ArStreetscapeGeometry.
inline ArStreetscapeGeometry *ArAsStreetscapeGeometry(ArTrackable *trackable) {
  return reinterpret_cast<ArStreetscapeGeometry *>(trackable);
}

#endif  // __cplusplus

// If compiling for C++11, use the 'enum underlying type' feature to enforce
// size for ABI compatibility. In pre-C++11, use int32_t for fixed size.
#if __cplusplus >= 201100
#define AR_DEFINE_ENUM(_type) enum _type : int32_t
#else
#define AR_DEFINE_ENUM(_type) \
  typedef int32_t _type;      \
  enum
#endif  // __cplusplus >= 201100

#if defined(__GNUC__) && !defined(AR_DEPRECATED_SUPPRESS)
#define AR_DEPRECATED(_deprecation_string) \
  __attribute__((deprecated(_deprecation_string)))
#else
#define AR_DEPRECATED(_deprecation_string)
#endif  // defined(__GNUC__) && !defined(AR_DEPRECATED_SUPPRESS)

/// @ingroup ArTrackable
/// Object types for heterogeneous query/update lists.
AR_DEFINE_ENUM(ArTrackableType){
    /// The base Trackable type. Can be passed to @c
    /// ::ArSession_getAllTrackables and @c ::ArFrame_getUpdatedTrackables as
    /// the @p filter_type to get all/updated Trackables of all types.
    AR_TRACKABLE_BASE_TRACKABLE = 0x41520100,

    /// The @c ::ArPlane subtype of Trackable.
    AR_TRACKABLE_PLANE = 0x41520101,

    /// The @c ::ArPoint subtype of Trackable.
    AR_TRACKABLE_POINT = 0x41520102,

    /// The @c ::ArAugmentedImage subtype of Trackable.
    AR_TRACKABLE_AUGMENTED_IMAGE = 0x41520104,

    /// Trackable type for faces.
    AR_TRACKABLE_FACE = 0x41520105,

    /// Trackable type for Streetscape Geometry.
    AR_TRACKABLE_STREETSCAPE_GEOMETRY = 0x41520103,

    /// Trackable type for @c ::ArEarth.
    AR_TRACKABLE_EARTH = 0x41520109,

    /// On supported devices, trackable type for depth image based hit results
    /// returned by @c ::ArFrame_hitTest when @c ::ArConfig_setDepthMode has
    /// been set to @c #AR_DEPTH_MODE_AUTOMATIC.
    AR_TRACKABLE_DEPTH_POINT = 0x41520111,

    /// Trackable type for results retrieved from
    /// @c ::ArFrame_hitTestInstantPlacement. This trackable type is only
    /// available when when @c ::ArConfig_setInstantPlacementMode is
    /// @c #AR_INSTANT_PLACEMENT_MODE_LOCAL_Y_UP.
    AR_TRACKABLE_INSTANT_PLACEMENT_POINT = 0x41520112,

    /// An invalid Trackable type.
    AR_TRACKABLE_NOT_VALID = 0};

/// @ingroup ArSession
/// Feature names for use with @c ::ArSession_createWithFeatures
///
/// All currently defined features are mutually compatible.
AR_DEFINE_ENUM(ArSessionFeature){
    /// Indicates the end of a features list. This must be the last entry in the
    /// array passed to @c ::ArSession_createWithFeatures.
    AR_SESSION_FEATURE_END_OF_LIST = 0,

    /// Use the front-facing (selfie) camera. When the front camera is selected,
    /// ARCore's behavior changes in the following ways:
    ///
    /// - The display will be mirrored. Specifically,
    ///   @c ::ArCamera_getProjectionMatrix will include a horizontal flip in
    ///   the generated projection matrix and APIs that reason about things in
    ///   screen space, such as @c ::ArFrame_transformCoordinates2d, will mirror
    ///   screen coordinates. Open GL apps should consider using @c glFrontFace
    ///   to render mirrored assets without changing their winding direction.
    /// - @c ::ArCamera_getTrackingState will always output
    ///   @c #AR_TRACKING_STATE_PAUSED.
    /// - @c ::ArFrame_hitTest will always output an empty list.
    /// - @c ::ArCamera_getDisplayOrientedPose will always output an identity
    ///   pose.
    /// - @c ::ArSession_acquireNewAnchor will always return
    ///   @c #AR_ERROR_NOT_TRACKING.
    /// - Planes will never be detected.
    /// - @c ::ArSession_configure will fail if the supplied configuration
    /// requests
    ///   Cloud Anchors, Augmented Images, or Environmental HDR Lighting
    ///   Estimation mode.
    ///
    /// @deprecated To create a session using the front-facing (selfie)
    /// camera, use @c ::ArSession_setCameraConfig with the desired config
    /// retrieved from @c ::ArSession_getSupportedCameraConfigsWithFilter.
    AR_SESSION_FEATURE_FRONT_CAMERA AR_DEPRECATED(
        "To create a session using the front-facing (selfie) camera, use "
        "@c ::ArSession_setCameraConfig with the desired config retrieved "
        "from @c ::ArSession_getSupportedCameraConfigsWithFilter.") = 1,

};

/// @ingroup shared_types
/// Return code indicating success or failure of a function.
AR_DEFINE_ENUM(ArStatus){
    /// The operation was successful.
    AR_SUCCESS = 0,

    /// One of the arguments was invalid; either @c NULL or not appropriate for
    /// the operation requested.
    AR_ERROR_INVALID_ARGUMENT = -1,

    /// An internal error occurred that the application should not attempt to
    /// recover from.
    AR_ERROR_FATAL = -2,

    /// An operation was attempted that requires the session be running, but the
    /// session was paused.
    AR_ERROR_SESSION_PAUSED = -3,

    /// An operation was attempted that requires the session be paused, but the
    /// session was running.
    AR_ERROR_SESSION_NOT_PAUSED = -4,

    /// An operation was attempted that the session be in the
    /// @c #AR_TRACKING_STATE_TRACKING state, but the session was not.
    AR_ERROR_NOT_TRACKING = -5,

    /// A texture name was not set by calling @c
    /// ::ArSession_setCameraTextureName before the first call to @c
    /// ::ArSession_update.
    AR_ERROR_TEXTURE_NOT_SET = -6,

    /// An operation required GL context but one was not available.
    AR_ERROR_MISSING_GL_CONTEXT = -7,

    /// The configuration supplied to @c ::ArSession_configure is unsupported.
    /// To avoid this error, ensure that Session_checkSupported() returns true.
    AR_ERROR_UNSUPPORTED_CONFIGURATION = -8,

    /// The application does not have Android camera permission.
    AR_ERROR_CAMERA_PERMISSION_NOT_GRANTED = -9,

    /// Acquire failed because the object being acquired was already released.
    /// For example, this happens if the application holds an @c ::ArFrame
    /// beyond the next call to @c ::ArSession_update, and then tries to acquire
    /// its Point Cloud.
    AR_ERROR_DEADLINE_EXCEEDED = -10,

    /// There are no available resources to complete the operation. In cases of
    /// @c acquire functions returning this error, this can be avoided by
    /// releasing previously acquired objects before acquiring new ones.
    AR_ERROR_RESOURCE_EXHAUSTED = -11,

    /// Acquire failed because the data isn't available yet for the current
    /// frame. For example, acquiring image metadata may fail with this error
    /// because the camera hasn't fully started.
    AR_ERROR_NOT_YET_AVAILABLE = -12,

    /// The Android camera has been reallocated to a higher priority application
    /// or is otherwise unavailable.
    AR_ERROR_CAMERA_NOT_AVAILABLE = -13,

    /// The host/resolve function call failed because the Session is not
    /// configured for Cloud Anchors.
    AR_ERROR_CLOUD_ANCHORS_NOT_CONFIGURED = -14,

    /// @c ::ArSession_configure failed because the specified configuration
    /// required the Android INTERNET permission, which the application did not
    /// have.
    AR_ERROR_INTERNET_PERMISSION_NOT_GRANTED = -15,

    /// Hosting a Cloud Anchor failed because the anchor is not a type of anchor
    /// that is currently supported for hosting.
    AR_ERROR_ANCHOR_NOT_SUPPORTED_FOR_HOSTING = -16,

    /// Attempted to add an image with insufficient quality (e.g., too few
    /// features) to the image database.
    AR_ERROR_IMAGE_INSUFFICIENT_QUALITY = -17,

    /// The data passed in for this operation was not in a valid format.
    AR_ERROR_DATA_INVALID_FORMAT = -18,

    /// The data passed in for this operation is not supported by this version
    /// of the SDK.
    AR_ERROR_DATA_UNSUPPORTED_VERSION = -19,

    /// A function has been invoked at an illegal or inappropriate time. A
    /// message will be printed to logcat with additional details for the
    /// developer. For example, @c ::ArSession_resume will return this status if
    /// the camera configuration was changed and there is at least one
    /// unreleased image.
    AR_ERROR_ILLEGAL_STATE = -20,

    /// The Android precise location permission (@c ACCESS_FINE_LOCATION) is
    /// required, but has not been granted prior to calling @c
    /// ::ArSession_configure when @c ::ArGeospatialMode is set to @c
    /// #AR_GEOSPATIAL_MODE_ENABLED.
    ///
    /// See <a
    /// href="https://developer.android.com/training/location/permissions#request-location-access-runtime">Android
    /// documentation on Request location access at runtime</a> for more
    /// information on how to request the @c ACCESS_FINE_LOCATION permission.
    AR_ERROR_FINE_LOCATION_PERMISSION_NOT_GRANTED = -21,

    /// The <a
    /// href="https://developers.google.com/android/guides/setup#declare-dependencies">Fused
    /// Location Provider for Android library</a> is required, but wasn't found
    /// in the client app's binary prior to calling @c ::ArSession_configure
    /// when @c ::ArGeospatialMode is set to @c #AR_GEOSPATIAL_MODE_ENABLED.
    ///
    /// <p>Ensure that your app <a
    /// href="https://developers.google.com/ar/develop/c/geospatial/developer-guide#include-required-libraries">includes
    /// the Fused Location Provider of Android library</a>, and that your app's
    /// <a
    /// href="https://developers.google.com/ar/develop/c/geospatial/developer-guide#include-required-proguard-rules">ProGuard
    /// rules are correctly set up for the Geospatial API</a>.
    ///
    /// <p>When building your app, check the resulting binary with the <a
    /// href="https://developer.android.com/studio/debug/apk-analyzer">APK
    /// Analyzer</a>, and ensure that the built application binary includes the
    /// @c com.google.android.gms.location package, and that its contents are
    /// not renamed or minified.

    AR_ERROR_GOOGLE_PLAY_SERVICES_LOCATION_LIBRARY_NOT_LINKED = -22,

    /// When recording failed.
    AR_ERROR_RECORDING_FAILED = -23,

    /// When playback failed.
    AR_ERROR_PLAYBACK_FAILED = -24,
    /// Operation is unsupported with the current session.
    AR_ERROR_SESSION_UNSUPPORTED = -25,

    /// The requested metadata tag cannot be found in input metadata.
    AR_ERROR_METADATA_NOT_FOUND = -26,

    /// The ARCore APK is not installed on this device.
    AR_UNAVAILABLE_ARCORE_NOT_INSTALLED = -100,

    /// The device is not currently compatible with ARCore.
    AR_UNAVAILABLE_DEVICE_NOT_COMPATIBLE = -101,

    /// The ARCore APK currently installed on device is too old and needs to be
    /// updated.
    AR_UNAVAILABLE_APK_TOO_OLD = -103,

    /// The ARCore APK currently installed no longer supports the ARCore SDK
    /// that the application was built with.
    AR_UNAVAILABLE_SDK_TOO_OLD = -104,

    /// The user declined installation of the ARCore APK during this run of the
    /// application and the current request was not marked as user-initiated.
    AR_UNAVAILABLE_USER_DECLINED_INSTALLATION = -105};

/// @ingroup shared_types
/// Describes the tracking state of an @c ::ArTrackable, an @c ::ArAnchor or the
/// @c ::ArCamera.
AR_DEFINE_ENUM(ArTrackingState){
    /// The object is currently tracked and its pose is current.
    AR_TRACKING_STATE_TRACKING = 0,

    /// ARCore has paused tracking this object, but may resume tracking it in
    /// the future. This can happen if device tracking is lost, if the user
    /// enters a new space, or if the session is currently paused. When in this
    /// state, the positional properties of the object may be wildly inaccurate
    /// and should not be used.
    AR_TRACKING_STATE_PAUSED = 1,

    /// ARCore has stopped tracking this Trackable and will never resume
    /// tracking it.
    AR_TRACKING_STATE_STOPPED = 2};

/// @ingroup shared_types
/// Describes possible tracking failure reasons of an @c ::ArCamera.
AR_DEFINE_ENUM(ArTrackingFailureReason){
    /// Indicates expected motion tracking behavior. Always returned when
    /// @c ::ArCamera_getTrackingState is @c #AR_TRACKING_STATE_TRACKING. When
    /// @c ::ArCamera_getTrackingState is @c #AR_TRACKING_STATE_PAUSED,
    /// indicates that
    /// the session is initializing normally.
    AR_TRACKING_FAILURE_REASON_NONE = 0,
    /// Motion tracking lost due to bad internal state. No specific user action
    /// is likely to resolve this issue.
    AR_TRACKING_FAILURE_REASON_BAD_STATE = 1,
    /// Motion tracking lost due to poor lighting conditions. Ask the user to
    /// move to a more brightly lit area.
    /// On Android 12 (API level 31) or later, the user may have <a
    /// href="https://developer.android.com/about/versions/12/behavior-changes-all#mic-camera-toggles">
    /// disabled camera access</a>, causing ARCore to receive a blank camera
    /// feed.
    AR_TRACKING_FAILURE_REASON_INSUFFICIENT_LIGHT = 2,
    /// Motion tracking lost due to excessive motion. Ask the user to move the
    /// device more slowly.
    AR_TRACKING_FAILURE_REASON_EXCESSIVE_MOTION = 3,
    /// Motion tracking lost due to insufficient visual features. Ask the user
    /// to move to a different area and to avoid blank walls and surfaces
    /// without detail.
    AR_TRACKING_FAILURE_REASON_INSUFFICIENT_FEATURES = 4,
    /// Motion tracking paused because the camera is in use by another
    /// application. Tracking will resume once this app regains priority, or
    /// once all apps with higher priority have stopped using the camera. Prior
    /// to ARCore SDK 1.13, @c #AR_TRACKING_FAILURE_REASON_NONE is returned in
    /// this
    /// case instead.
    AR_TRACKING_FAILURE_REASON_CAMERA_UNAVAILABLE = 5};

/// @ingroup ArAnchor
/// Result of a Cloud Anchor hosting or resolving operation.
AR_DEFINE_ENUM(ArCloudAnchorState){
    /// Not a valid value for a Cloud Anchor operation.
    AR_CLOUD_ANCHOR_STATE_NONE = 0,

    /// A hosting/resolving task for the anchor is in progress. Once the task
    /// completes in the background, the anchor will get a new cloud state after
    /// the next @c ::ArSession_update call.
    AR_CLOUD_ANCHOR_STATE_TASK_IN_PROGRESS AR_DEPRECATED(
        "Not returned by async cloud anchor APIs - replaced by "
        "AR_FUTURE_STATE_PENDING.") = 1,

    /// A hosting/resolving task for this anchor completed successfully.
    AR_CLOUD_ANCHOR_STATE_SUCCESS = 2,

    /// A hosting/resolving task for this anchor finished with an internal
    /// error. The app should not attempt to recover from this error.
    AR_CLOUD_ANCHOR_STATE_ERROR_INTERNAL = -1,

    /// The authorization provided by the application is not valid.
    /// - The Google Cloud project may not have enabled the ARCore API.
    /// - It may fail if the operation you are trying to perform is not allowed.
    /// - When using API key authentication, this will happen if the API key in
    ///   the manifest is invalid, unauthorized or missing. It may also fail if
    ///   the API key is restricted to a set of apps not including the current
    ///   one.
    /// - When using keyless authentication, this will happen if the developer
    ///   fails to create OAuth client. It may also fail if Google Play Services
    ///   isn't installed, is too old, or is malfunctioning for some reason (e.g.
    ///   services killed due to memory pressure).
    AR_CLOUD_ANCHOR_STATE_ERROR_NOT_AUTHORIZED = -2,

    AR_CLOUD_ANCHOR_STATE_ERROR_SERVICE_UNAVAILABLE AR_DEPRECATED(
        "AR_CLOUD_ANCHOR_STATE_ERROR_SERVICE_UNAVAILABLE is deprecated in "
        "ARCore SDK 1.12. See release notes to learn more.") = -3,

    /// The application has exhausted the request quota allotted to the given
    /// API key. The developer should request additional quota for the ARCore
    /// API for their API key from the Google Developers Console.
    AR_CLOUD_ANCHOR_STATE_ERROR_RESOURCE_EXHAUSTED = -4,

    /// Hosting failed, because the server could not successfully process the
    /// dataset for the given anchor. The developer should try again after the
    /// device has gathered more data from the environment.
    AR_CLOUD_ANCHOR_STATE_ERROR_HOSTING_DATASET_PROCESSING_FAILED = -5,

    /// Resolving failed, because the ARCore Cloud Anchor service could not find
    /// the provided Cloud Anchor ID.
    AR_CLOUD_ANCHOR_STATE_ERROR_CLOUD_ID_NOT_FOUND = -6,

    AR_CLOUD_ANCHOR_STATE_ERROR_RESOLVING_LOCALIZATION_NO_MATCH AR_DEPRECATED(
        "AR_CLOUD_ANCHOR_STATE_ERROR_RESOLVING_LOCALIZATION_NO_MATCH is "
        "deprecated in ARCore SDK 1.12. See release notes to learn more.") = -7,

    /// The Cloud Anchor could not be resolved because the SDK version used to
    /// resolve the anchor is older than and incompatible with the version used
    /// to host it.
    AR_CLOUD_ANCHOR_STATE_ERROR_RESOLVING_SDK_VERSION_TOO_OLD = -8,

    /// The Cloud Anchor could not be resolved because the SDK version used to
    /// resolve the anchor is newer than and incompatible with the version used
    /// to host it.
    AR_CLOUD_ANCHOR_STATE_ERROR_RESOLVING_SDK_VERSION_TOO_NEW = -9,

    /// The ARCore Cloud Anchor service was unreachable. This can happen for
    /// a number of reasons. The device might be in airplane mode or does not
    /// have a working internet connection. The request sent to the server might
    /// have timed out with no response, or there might be a bad network
    /// connection, DNS unavailability, firewall issues, or anything else that
    /// might affect the device's ability to connect to the ARCore Cloud Anchor
    /// service.
    AR_CLOUD_ANCHOR_STATE_ERROR_HOSTING_SERVICE_UNAVAILABLE = -10,
};

/// @ingroup ArCoreApk
/// Describes the current state of ARCore availability on the device.
AR_DEFINE_ENUM(ArAvailability){
    /// An internal error occurred while determining ARCore availability.
    AR_AVAILABILITY_UNKNOWN_ERROR = 0,
    /// ARCore is not installed, and a query has been issued to check if ARCore
    /// is is supported.
    AR_AVAILABILITY_UNKNOWN_CHECKING = 1,
    /// ARCore is not installed, and the query to check if ARCore is supported
    /// timed out. This may be due to the device being offline.
    AR_AVAILABILITY_UNKNOWN_TIMED_OUT = 2,
    /// ARCore is not supported on this device.
    AR_AVAILABILITY_UNSUPPORTED_DEVICE_NOT_CAPABLE = 100,
    /// The device and Android version are supported, but the ARCore APK is not
    /// installed.
    AR_AVAILABILITY_SUPPORTED_NOT_INSTALLED = 201,
    /// The device and Android version are supported, and a version of the
    /// ARCore APK is installed, but that ARCore APK version is too old.
    AR_AVAILABILITY_SUPPORTED_APK_TOO_OLD = 202,
    /// ARCore is supported, installed, and available to use.
    AR_AVAILABILITY_SUPPORTED_INSTALLED = 203};

/// @ingroup ArCoreApk
/// Indicates the outcome of a call to @c ::ArCoreApk_requestInstall.
AR_DEFINE_ENUM(ArInstallStatus){
    /// The requested resource is already installed.
    AR_INSTALL_STATUS_INSTALLED = 0,
    /// Installation of the resource was requested. The current activity will be
    /// paused.
    AR_INSTALL_STATUS_INSTALL_REQUESTED = 1};

/// @ingroup ArCoreApk
/// Controls the behavior of the installation UI.
AR_DEFINE_ENUM(ArInstallBehavior){
    /// Hide the Cancel button during initial prompt and prevent user from
    /// exiting via tap-outside.
    ///
    /// Note: The BACK button or tapping outside of any marketplace-provided
    /// install dialog will still decline the installation.
    AR_INSTALL_BEHAVIOR_REQUIRED = 0,
    /// Include Cancel button in initial prompt and allow easily backing out
    /// after installation has been initiated.
    AR_INSTALL_BEHAVIOR_OPTIONAL = 1};

/// @ingroup ArCoreApk
/// Controls the message displayed by the installation UI.
AR_DEFINE_ENUM(ArInstallUserMessageType){
    /// Display a localized message like "This application requires ARCore...".
    AR_INSTALL_USER_MESSAGE_TYPE_APPLICATION = 0,
    /// Display a localized message like "This feature requires ARCore...".
    AR_INSTALL_USER_MESSAGE_TYPE_FEATURE = 1,
    /// Application has explained why ARCore is required prior to calling
    /// @c ::ArCoreApk_requestInstall, skip user education dialog.
    AR_INSTALL_USER_MESSAGE_TYPE_USER_ALREADY_INFORMED = 2};

/// @ingroup ArConfig
/// Select the behavior of the Lighting Estimation subsystem.
AR_DEFINE_ENUM(ArLightEstimationMode){
    /// Lighting Estimation is disabled.
    AR_LIGHT_ESTIMATION_MODE_DISABLED = 0,
    /// Lighting Estimation is enabled, generating a single-value intensity
    /// estimate and three (R, G, B) color correction values.
    AR_LIGHT_ESTIMATION_MODE_AMBIENT_INTENSITY = 1,
    /// Lighting Estimation is enabled, generating inferred Environmental HDR
    /// Lighting Estimation in linear color space. Note,
    /// @c #AR_LIGHT_ESTIMATION_MODE_ENVIRONMENTAL_HDR is not supported when
    /// using the front-facing (selfie) camera.
    AR_LIGHT_ESTIMATION_MODE_ENVIRONMENTAL_HDR = 2,
};

/// @ingroup ArConfig
/// Select the behavior of the plane detection subsystem.
AR_DEFINE_ENUM(ArPlaneFindingMode){
    /// Plane detection is disabled.
    AR_PLANE_FINDING_MODE_DISABLED = 0,
    /// Detection of only horizontal planes is enabled.
    AR_PLANE_FINDING_MODE_HORIZONTAL = 1,
    /// Detection of only vertical planes is enabled.
    AR_PLANE_FINDING_MODE_VERTICAL = 2,
    /// Detection of horizontal and vertical planes is enabled.
    AR_PLANE_FINDING_MODE_HORIZONTAL_AND_VERTICAL = 3,
};

/// @ingroup ArRecordingConfig
/// Describe the current recording status.
AR_DEFINE_ENUM(ArRecordingStatus){
    /// The dataset recorder is not recording.
    AR_RECORDING_NONE = 0,
    /// The dataset recorder is recording normally.
    AR_RECORDING_OK = 1,
    /// The dataset recorder encountered an error while recording.
    AR_RECORDING_IO_ERROR = 2,
};

/// @ingroup ArConfig
/// Selects the behavior of @c ::ArSession_update.
AR_DEFINE_ENUM(ArUpdateMode){
    /// @c ::ArSession_update will wait until a new camera image is available,
    /// or until the built-in timeout (currently 66ms) is reached. On most
    /// devices the camera is configured to capture 30 frames per second. If the
    /// camera image does not arrive by the built-in timeout, then @c
    /// ::ArSession_update
    /// will return the most recent @c ::ArFrame object.
    AR_UPDATE_MODE_BLOCKING = 0,
    /// @c ::ArSession_update will return immediately without blocking. If no
    /// new camera image is available, then @c ::ArSession_update will return
    /// the most recent @c ::ArFrame object.
    AR_UPDATE_MODE_LATEST_CAMERA_IMAGE = 1,
};

/// @ingroup ArConfig
/// Selects the behavior of Augmented Faces subsystem.
/// Default value is @c #AR_AUGMENTED_FACE_MODE_DISABLED.
AR_DEFINE_ENUM(ArAugmentedFaceMode){
    /// Disable augmented face mode.
    AR_AUGMENTED_FACE_MODE_DISABLED = 0,

    /// Face 3D mesh is enabled. Augmented Faces is currently only
    /// supported when using the front-facing (selfie) camera.
    AR_AUGMENTED_FACE_MODE_MESH3D = 2,
};

/// @ingroup ArAugmentedImage
/// Defines the current tracking mode for an Augmented Image. To retrieve the
/// tracking mode for an image use @c ::ArAugmentedImage_getTrackingMethod.
AR_DEFINE_ENUM(ArAugmentedImageTrackingMethod){
    /// The Augmented Image is not currently being tracked.
    AR_AUGMENTED_IMAGE_TRACKING_METHOD_NOT_TRACKING = 0,
    /// The Augmented Image is currently being tracked using the camera image.
    AR_AUGMENTED_IMAGE_TRACKING_METHOD_FULL_TRACKING = 1,
    /// The Augmented Image is currently being tracked based on its last known
    /// pose, because it can no longer be tracked using the camera image.
    AR_AUGMENTED_IMAGE_TRACKING_METHOD_LAST_KNOWN_POSE = 2};

/// @ingroup ArAugmentedFace
/// Defines face regions to query the pose for. Left and right are defined
/// relative to the person that the mesh belongs to. To retrieve the center pose
/// use @c ::ArAugmentedFace_getCenterPose.
AR_DEFINE_ENUM(ArAugmentedFaceRegionType){
    /// The region at the tip of the nose.
    AR_AUGMENTED_FACE_REGION_NOSE_TIP = 0,
    /// The region at the detected face's left side of the forehead.
    AR_AUGMENTED_FACE_REGION_FOREHEAD_LEFT = 1,
    /// The region at the detected face's right side of the forehead.
    AR_AUGMENTED_FACE_REGION_FOREHEAD_RIGHT = 2,
};

/// @ingroup ArConfig
/// Selects the desired behavior of the camera focus subsystem.
AR_DEFINE_ENUM(ArFocusMode){/// Focus is fixed.
                            AR_FOCUS_MODE_FIXED = 0,
                            /// Auto-focus is enabled.
                            AR_FOCUS_MODE_AUTO = 1};

/// @ingroup ArConfig
/// Selects the desired behavior of the camera flash subsystem. See the <a
/// href="https://developers.google.com/ar/develop/camera/flash/c">documentation
/// page</a> for more information on using the device's flash.
AR_DEFINE_ENUM(ArFlashMode){/// Flash is off.
                            AR_FLASH_MODE_OFF = 0,
                            /// Flash is on.
                            AR_FLASH_MODE_TORCH = 2};

/// @ingroup ArConfig
/// Describes the behavior of the Electronic Image Stabilization (EIS) API. When
/// enabled, EIS smoothes the camera feed and helps correct video shakes in the
/// camera preview. See the <a
/// href="https://developers.google.com/ar/develop/c/electronic-image-stabilization">Electronic
/// Image Stabilization Developer Guide</a> for more information.
///
/// Not all devices support all modes. Use @c
/// ::ArSession_isImageStabilizationModeSupported to detect if a @c
/// ::ArImageStabilizationMode is supported with the selected camera
/// configuration.
///
/// Attempting to use @c ::ArSession_configure to configure a @c
/// ::ArImageStabilizationMode mode on a device that isn't supported, @c
/// ::ArSession_configure will return #AR_ERROR_UNSUPPORTED_CONFIGURATION.
///
/// The default value is #AR_IMAGE_STABILIZATION_MODE_OFF. Use
/// @c ::ArConfig_setImageStabilizationMode to set the desired mode.
AR_DEFINE_ENUM(ArImageStabilizationMode){
    /// Image stabilization is off.
    ///
    /// This is the default mode.
    AR_IMAGE_STABILIZATION_MODE_OFF = 0,
    /// Image stabilization is on. @c ::ArFrame_transformCoordinates3d can be
    /// use to convert 2D coordinates to @c
    /// #AR_COORDINATES_3D_EIS_TEXTURE_NORMALIZED and @c
    /// #AR_COORDINATES_3D_EIS_NORMALIZED_DEVICE_COORDINATES.
    ///
    /// See the <a
    /// href="https://developers.google.com/ar/develop/c/electronic-image-stabilization">Electronic
    /// Image Stabilization Developer Guide</a> for more information.
    AR_IMAGE_STABILIZATION_MODE_EIS = 1,
};

/// @ingroup ArConfig
/// Selects the desired depth mode. Not all devices support all modes, use
/// @c ::ArSession_isDepthModeSupported to find whether the current device and
/// the selected camera support a particular depth mode.
AR_DEFINE_ENUM(ArDepthMode){
    /// No depth information will be provided. Calling
    /// @c ::ArFrame_acquireDepthImage16Bits or
    /// @c ::ArFrame_acquireRawDepthImage16Bits will return
    /// @c #AR_ERROR_ILLEGAL_STATE.
    AR_DEPTH_MODE_DISABLED = 0,
    /// On supported devices the best possible depth is estimated based on
    /// hardware and software sources. Available sources of automatic depth are:
    ///  - Depth from motion, using the main RGB camera
    ///  - Hardware depth sensor, such as a time-of-flight sensor (or ToF sensor)
    ///
    /// Provides depth estimation for every pixel in the image, and works best for
    /// static scenes. For a list of supported devices, see:
    /// https://developers.google.com/ar/devices
    /// Adds significant computational load.
    ///
    /// With this mode enabled, @c ::ArFrame_hitTest also returns
    /// @c ::ArDepthPoint in the output @c ::ArHitResultList, which are
    /// sampled from the generated depth image for the current frame if available.
    AR_DEPTH_MODE_AUTOMATIC = 1,
    /// @ingroup ArFrame
    /// On <a
    /// href="https://developers.google.com/ar/devices">ARCore
    /// supported devices</a> that also support the Depth API, provides a "raw",
    /// mostly unfiltered, depth image
    /// (@c ::ArFrame_acquireRawDepthImage16Bits) and depth confidence image
    /// (@c ::ArFrame_acquireRawDepthConfidenceImage).
    ///
    /// The raw depth image is sparse and does not provide valid depth for all
    /// pixels. Pixels without a valid depth estimate have a pixel value of 0.
    ///
    /// Raw depth data is also available when @c #AR_DEPTH_MODE_AUTOMATIC is
    /// selected.
    ///
    /// Raw depth is intended to be used in cases that involve understanding of
    /// the geometry in the environment.
    AR_DEPTH_MODE_RAW_DEPTH_ONLY = 3,
};

/// @ingroup ArConfig
/// Describes how ARCore will update the camera texture. See <a
/// href="https://developers.google.com/ar/develop/c/vulkan">Vulkan Rendering
/// developer guide</a> for more information.
///
/// The default value is @c
/// #AR_TEXTURE_UPDATE_MODE_BIND_TO_TEXTURE_EXTERNAL_OES. Use
/// @c ::ArConfig_setTextureUpdateMode to set the desired mode.
AR_DEFINE_ENUM(ArTextureUpdateMode){
    /// ARCore provides the camera image through the @c GL_TEXTURE_EXTERNAL_OES
    /// texture provided to @c ::ArSession_setCameraTextureName or @c
    /// ::ArSession_setCameraTextureNames.
    ///
    /// This is the default mode.
    AR_TEXTURE_UPDATE_MODE_BIND_TO_TEXTURE_EXTERNAL_OES = 0,
    /// ARCore provides the camera image through an @c AHardwareBuffer. The
    /// hardware buffer for an @c ::ArFrame is accessible via
    /// @c ::ArFrame_getHardwareBuffer. See documentation on <a
    /// href="https://developer.android.com/ndk/reference/group/a-hardware-buffer">Native
    /// Hardware Buffer</a> in the Android NDK.
    ///
    /// The client app is responsible for binding it to
    /// a @c GL_TEXTURE_EXTERNAL_OES (OpenGL ES) or @c VkImage (Vulkan).
    ///
    /// When a configuration is active with @c
    /// #AR_TEXTURE_UPDATE_MODE_EXPOSE_HARDWARE_BUFFER,
    /// texture names provided to @c ::ArSession_setCameraTextureName and
    /// @c ::ArSession_setCameraTextureNames are ignored.
    ///
    /// This is only available on Android API levels 27 and above. Using @c
    /// ::ArSession_configure to set
    /// #AR_TEXTURE_UPDATE_MODE_EXPOSE_HARDWARE_BUFFER on an incompatible
    /// device will return @c #AR_ERROR_UNSUPPORTED_CONFIGURATION.
    AR_TEXTURE_UPDATE_MODE_EXPOSE_HARDWARE_BUFFER = 1,
};

/// @ingroup ArConfig
/// Describes the desired behavior of Scene Semantics. Scene Semantics uses a
/// machine learning model to label each pixel from the camera feed with a @c
/// ::ArSemanticLabel. See the
/// <a href="https://developers.google.com/ar/develop/c/scene-semantics">Scene
/// Semantics Developer Guide</a> for more information.
///
/// The Scene Semantics API is currently able to distinguish
/// between outdoor labels specified by @c ::ArSemanticLabel. Usage indoors is
/// currently unsupported and may yield unreliable results.
///
/// A small number of ARCore supported devices do not support the Scene
/// Semantics API. Use  @c ::ArSession_isSemanticModeSupported to query for
/// support for Scene Semantics. Affected devices are also indicated on the <a
/// href="https://developers.google.com/ar/devices">ARCore supported devices
/// page</a>.
///
/// The default value is @c #AR_SEMANTIC_MODE_DISABLED. Use @c
/// ::ArConfig_setSemanticMode to set the desired mode.
AR_DEFINE_ENUM(ArSemanticMode){
    /// The Scene Semantics API is disabled. Calls to
    /// @c ::ArFrame_acquireSemanticImage,
    /// @c ::ArFrame_acquireSemanticConfidenceImage,
    /// and @c ::ArFrame_getSemanticLabelFraction will not return valid results.
    ///
    /// This is the default mode.
    AR_SEMANTIC_MODE_DISABLED = 0,
    /// The Scene Semantics API is enabled. Calls to
    /// @c ::ArFrame_acquireSemanticImage,
    /// @c ::ArFrame_acquireSemanticConfidenceImage,
    /// and @c ::ArFrame_getSemanticLabelFraction will return valid results.
    ///
    /// Use @c ::ArConfig_setSemanticMode to set this mode.
    AR_SEMANTIC_MODE_ENABLED = 1,
};

/// @ingroup ArSemanticLabel
/// Defines the labels the Scene Semantics API is able to detect and maps
/// human-readable names to per-pixel semantic labels. See the
/// <a href="https://developers.google.com/ar/develop/c/scene-semantics">Scene
/// Semantics Developer Guide</a> for more information.
///
/// Use @c ::ArFrame_acquireSemanticImage to obtain an image containing these
/// pixels and @c ::ArFrame_getSemanticLabelFraction to query what percentage of
/// the image contains these pixels.
AR_DEFINE_ENUM(ArSemanticLabel){
    /// Pixels with no semantic label available in the API output.
    AR_SEMANTIC_LABEL_UNLABELED = 0,
    /// Pixels of the open sky, including clouds. Thin electrical wires in front
    /// of the sky are included, but leaves/vegetation are not included.
    AR_SEMANTIC_LABEL_SKY = 1,
    /// Pixels of buildings, including houses, garages, etc. Includes all
    /// structures attached to the building, such as signs, solar panels,
    /// scaffolding, etc.
    AR_SEMANTIC_LABEL_BUILDING = 2,
    /// Pixels of non-walkable vegetation, like trees and shrubs. In contrast,
    /// 'terrain' specifies walkable vegetation, like grass.
    AR_SEMANTIC_LABEL_TREE = 3,
    /// Pixels of drivable surfaces for vehicles, including paved, unpaved,
    /// dirt, driveways, crosswalks, etc.
    AR_SEMANTIC_LABEL_ROAD = 4,
    /// Pixels of sidewalks for pedestrians and cyclists, including associated
    /// curbs.
    AR_SEMANTIC_LABEL_SIDEWALK = 5,
    /// Pixels of walkable vegetation areas, including grass, soil, sand,
    /// mountains, etc. In contrast, 'tree' specifies non-walkable vegetation,
    /// like trees and bushes.
    AR_SEMANTIC_LABEL_TERRAIN = 6,
    /// Pixels of structures that are not buildings, including fences,
    /// guardrails,
    /// stand-alone walls, tunnels, bridges, etc.
    AR_SEMANTIC_LABEL_STRUCTURE = 7,
    /// Pixels of general temporary and permanent objects and obstacles,
    /// including street signs, traffic signs, free-standing business signs,
    /// billboards, poles, mailboxes, fire hydrants, street lights, phone
    /// booths, bus stop enclosures, cones, parking meters, animals, etc.
    AR_SEMANTIC_LABEL_OBJECT = 8,
    /// Pixels of vehicles, including cars, vans, buses, trucks, motorcycles,
    /// bicycles, trains, etc.
    AR_SEMANTIC_LABEL_VEHICLE = 9,
    /// Pixels of humans, including pedestrians and bicycle/motorcycle riders.
    AR_SEMANTIC_LABEL_PERSON = 10,
    /// Pixels of ground surfaces covered by water, including lakes, rivers,
    /// etc.
    AR_SEMANTIC_LABEL_WATER = 11,
};

/// @ingroup ArPlane
/// Simple summary of the normal vector of a plane, for filtering purposes.
AR_DEFINE_ENUM(ArPlaneType){
    /// A horizontal plane facing upward (for example a floor or tabletop).
    AR_PLANE_HORIZONTAL_UPWARD_FACING = 0,
    /// A horizontal plane facing downward (for example a ceiling).
    AR_PLANE_HORIZONTAL_DOWNWARD_FACING = 1,
    /// A vertical plane (for example a wall).
    AR_PLANE_VERTICAL = 2};

/// @ingroup ArLightEstimate
/// Tracks the validity of an @c ::ArLightEstimate object.
AR_DEFINE_ENUM(ArLightEstimateState){
    /// The @c ::ArLightEstimate is not valid this frame and should not be used
    /// for rendering.
    AR_LIGHT_ESTIMATE_STATE_NOT_VALID = 0,
    /// The @c ::ArLightEstimate is valid this frame.
    AR_LIGHT_ESTIMATE_STATE_VALID = 1};

/// @ingroup ArPoint
/// Indicates the orientation mode of the @c ::ArPoint.
AR_DEFINE_ENUM(ArPointOrientationMode){
    /// The orientation of the @c ::ArPoint is initialized to identity but may
    /// adjust slightly over time.
    AR_POINT_ORIENTATION_INITIALIZED_TO_IDENTITY = 0,
    /// The orientation of the @c ::ArPoint will follow the behavior described
    /// in
    /// @c ::ArHitResult_getHitPose.
    AR_POINT_ORIENTATION_ESTIMATED_SURFACE_NORMAL = 1};

/// @ingroup ArInstantPlacementPoint
/// Tracking methods for @c ::ArInstantPlacementPoint.
AR_DEFINE_ENUM(ArInstantPlacementPointTrackingMethod){
    /// The @c ::ArInstantPlacementPoint is not currently being tracked. The
    /// @c ::ArTrackingState is @c #AR_TRACKING_STATE_PAUSED or
    /// @c #AR_TRACKING_STATE_STOPPED.
    AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_NOT_TRACKING = 0,
    /// The @c ::ArInstantPlacementPoint is currently being tracked in screen
    /// space and the pose returned by @c ::ArInstantPlacementPoint_getPose is
    /// being estimated using the approximate distance provided to
    /// @c ::ArFrame_hitTestInstantPlacement.
    ///
    /// ARCore concurrently tracks at most 20 Instant Placement points that are
    /// @c
    /// #AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_SCREENSPACE_WITH_APPROXIMATE_DISTANCE.<!--NOLINT-->
    /// As additional Instant Placement points with
    /// @c
    /// #AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_SCREENSPACE_WITH_APPROXIMATE_DISTANCE<!--NOLINT-->
    /// are created, the oldest points will become permanently
    /// @c #AR_TRACKING_STATE_STOPPED in order to maintain the maximum number of
    /// concurrently tracked points.
    AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_SCREENSPACE_WITH_APPROXIMATE_DISTANCE =  // NOLINT
    1,
    /// The @c ::ArInstantPlacementPoint is being tracked normally and
    /// @c ::ArInstantPlacementPoint_getPose is using a pose fully determined by
    /// ARCore.
    ///
    /// ARCore doesn't limit the number of Instant Placement points with
    /// @c #AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_FULL_TRACKING that are
    /// being tracked concurrently.
    AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_FULL_TRACKING = 2};

/// @ingroup ArConfig
/// Describes the desired behavior of the ARCore Cloud Anchor API. The Cloud
/// Anchor API uses feature maps to persist an anchor throughout sessions and
/// across devices. See the <a
/// href="https://developers.google.com/ar/develop/c/cloud-anchors/developer-guide">Cloud
/// Anchors developer guide</a> for more information.
///
/// The default value is @c #AR_CLOUD_ANCHOR_MODE_DISABLED. Use @c
/// ::ArConfig_setCloudAnchorMode to set the Cloud Anchor API mode and
/// @c ::ArSession_configure to configure the session.
AR_DEFINE_ENUM(ArCloudAnchorMode){
    /// The Cloud Anchor API is disabled. Calling @c
    /// ::ArSession_hostCloudAnchorAsync and @c
    /// ::ArSession_resolveCloudAnchorAsync will return
    /// @c #AR_ERROR_CLOUD_ANCHORS_NOT_CONFIGURED.
    ///
    /// This is the default value.
    AR_CLOUD_ANCHOR_MODE_DISABLED = 0,

    /// The Cloud Anchor API is enabled. @c
    /// ::ArSession_hostCloudAnchorAsync and @c
    /// ::ArSession_resolveCloudAnchorAsync can be used to host and resolve
    /// Cloud Anchors.
    ///
    /// Using this mode requires your app to do the following:
    ///
    /// - Include the <a
    ///   href="https://developer.android.com/training/basics/network-ops/connecting">@c
    ///    ACCESS_INTERNET </a> permission to the app's AndroidManifest;
    ///    otherwise, @c ::ArSession_configure returns @c
    ///    #AR_ERROR_INTERNET_PERMISSION_NOT_GRANTED.
    /// - Configure <a
    ///   href="https://developers.google.com/ar/develop/c/cloud-anchors/developer-guide#authorize_your_app_to_call_the_arcore_api">Keyless
    ///   or API Key authorization</a>.
    ///
    /// Use @c
    /// ::ArConfig_setCloudAnchorMode to set the Cloud Anchor API mode and
    /// @c ::ArSession_configure to configure the session.
    AR_CLOUD_ANCHOR_MODE_ENABLED = 1,
};

/// @ingroup ArConfig
/// Describes the desired behavior of the ARCore Geospatial API.
/// The Geospatial API uses a combination of Google's Visual Positioning System
/// (VPS) and GPS to determine the geospatial pose.
///
/// The Geospatial API is able to provide the best user experience when it is
/// able to generate high accuracy poses. However, the Geospatial API can be
/// used anywhere, as long as the device is able to determine its location, even
/// if the available location information has low accuracy.
///
/// - In areas with VPS coverage, the Geospatial API is able to generate high
///   accuracy poses. This can work even where GPS accuracy is low, such as
///   dense urban environments. Under typical conditions, VPS can be expected to
///   provide positional accuracy typically better than 5 meters and often
///   around 1 meter, and a rotational accuracy of better than 5 degrees. Use @c
///   ::ArSession_checkVpsAvailabilityAsync to determine if a given location
///   has VPS coverage.
/// - In outdoor environments with few or no overhead obstructions, GPS may be
///   sufficient to generate high accuracy poses. GPS accuracy may be low in
///   dense urban environments and indoors.
///
/// A small number of ARCore supported devices do not support the Geospatial
/// API. Use @c ::ArSession_isGeospatialModeSupported to determine if the
/// current device is supported. Affected devices are also indicated on the <a
/// href="https://developers.google.com/ar/devices"> ARCore supported devices
/// page</a>.
///
/// When the Geospatial API and the Depth API are enabled, output images
/// from the Depth API will include terrain and building geometry when in a
/// location with VPS coverage. See the <a
/// href="https://developers.google.com/ar/develop/c/depth/geospatial-depth">Geospatial
/// Depth Developer Guide</a> for more information.
///
/// The default value is @c #AR_GEOSPATIAL_MODE_DISABLED. Use @c
/// ::ArConfig_setGeospatialMode to set the desired mode.
AR_DEFINE_ENUM(ArGeospatialMode){
    /// The Geospatial API is disabled. When a configuration with @c
    /// #AR_GEOSPATIAL_MODE_DISABLED becomes active on the @c ::ArSession,
    /// current @c ::ArEarth and @c ::ArAnchor objects created from @c
    /// ::ArEarth_acquireNewAnchor will stop updating; have their @c
    /// ::ArTrackingState set to @c #AR_TRACKING_STATE_STOPPED and should be
    /// released; and @c ::ArSession_acquireEarth will return @c NULL. If
    /// re-enabled, you will need to call @c ::ArSession_acquireEarth to acquire
    /// @c ::ArEarth again.
    AR_GEOSPATIAL_MODE_DISABLED = 0,

    /// The Geospatial API is enabled. @c ::ArSession_acquireEarth will create
    /// valid @c ::ArEarth objects when a configuration becomes active with this
    /// mode through @c ::ArSession_configure.
    ///
    /// Using this mode requires your app do the following:
    ///
    /// - Add the <a
    /// href="https://developer.android.com/training/basics/network-ops/connecting">ACCESS_INTERNET</a>
    /// permission to the AndroidManifest; otherwise,
    ///     @c ::ArSession_configure returns
    ///     @c #AR_ERROR_INTERNET_PERMISSION_NOT_GRANTED.
    /// - Include the Google Play Services Location Library as a dependency for
    ///     your app. See <a
    ///     href="https://developers.google.com/android/guides/setup#declare-dependencies">Declare
    ///     dependencies for Google Play services</a> for instructions on how to
    ///     include this library in your app. If this library is not linked, @c
    ///     ::ArSession_configure returns
    ///     @c #AR_ERROR_GOOGLE_PLAY_SERVICES_LOCATION_LIBRARY_NOT_LINKED.
    /// - Request and be granted the <a
    /// href="https://developer.android.com/training/location/permissions">Android
    /// ACCESS_FINE_LOCATION permission</a>;
    ///     otherwise, @c ::ArSession_configure returns
    ///     @c #AR_ERROR_FINE_LOCATION_PERMISSION_NOT_GRANTED.
    ///
    /// Location is tracked only while @c ::ArSession is resumed. While
    /// @c ::ArSession is paused, @c ::ArEarth's @c ::ArTrackingState will be
    /// @c #AR_TRACKING_STATE_PAUSED.
    ///
    /// For more information, see documentation on <a
    ///     href="https://developers.google.com/ar/develop/c/geospatial/developer-guide">the
    ///     Geospatial API on Google Developers</a>.
    ///
    /// When the Geospatial API and the Depth API are enabled, output images
    /// from the Depth API will include terrain and building geometry when in a
    /// location with VPS coverage. See the <a
    /// href="https://developers.google.com/ar/develop/c/depth/geospatial-depth">Geospatial
    /// Depth Developer Guide</a> for more information.
    ///
    /// This mode is not compatible with the front-facing
    /// (selfie) camera. If @c ::ArGeospatialMode is @c
    /// #AR_GEOSPATIAL_MODE_ENABLED on a session using
    /// @c #AR_CAMERA_CONFIG_FACING_DIRECTION_FRONT, @c ::ArSession_configure
    /// will return @c #AR_ERROR_UNSUPPORTED_CONFIGURATION.
    ///
    /// Not all devices support @c #AR_GEOSPATIAL_MODE_ENABLED, use
    /// @c ::ArSession_isGeospatialModeSupported to check if the current device
    /// and selected camera support enabling this mode.
    AR_GEOSPATIAL_MODE_ENABLED = 2,
};

/// @ingroup ArConfig
/// Describes the desired behavior of the Geospatial Streetscape Geometry API.
/// The Streetscape Geometry API provides polygon meshes of terrain, buildings,
/// and other structures in a radius surrounding the device. See the <a
/// href="https://developers.google.com/ar/develop/c/geospatial/streetscape-geometry">Streetscape
/// Geometry Developer Guide</a> for additional information.
///
/// <p>When Streetscape Geometry is enabled, @c ::ArSession_getAllTrackables
/// and @c ::ArFrame_getUpdatedTrackables may additionally return
/// @c ::ArStreetscapeGeometry trackables.
///
/// <p>The Streetscape Geometry API requires both @c ::ArStreetscapeGeometryMode
/// to be set to @c #AR_STREETSCAPE_GEOMETRY_MODE_ENABLED and @c
/// ::ArGeospatialMode to be set to @c #AR_GEOSPATIAL_MODE_ENABLED.
///
/// <p>The default value is @c #AR_STREETSCAPE_GEOMETRY_MODE_DISABLED. Use
/// @c ::ArConfig_setStreetscapeGeometryMode to set the desired mode.

AR_DEFINE_ENUM(ArStreetscapeGeometryMode){
    /// The Streetscape Geometry API is disabled.
    ///
    /// This is the default mode.
    AR_STREETSCAPE_GEOMETRY_MODE_DISABLED = 0,
    /// The Streetscape Geometry API is enabled. @c ::ArSession_getAllTrackables
    /// and @c ::ArFrame_getUpdatedTrackables may additionally return
    /// @c ::ArStreetscapeGeometry trackables. This mode requires @c
    /// ::ArGeospatialMode to be set to @c #AR_GEOSPATIAL_MODE_ENABLED.
    ///
    /// Use @c ::ArConfig_setStreetscapeGeometryMode to set this mode.
    AR_STREETSCAPE_GEOMETRY_MODE_ENABLED = 1,
};

// === ArStreetscapeGeometry types ===

/// @ingroup ArStreetscapeGeometry
/// Describes the type of a Streetscape Geometry. Obtained by @c
/// ::ArStreetscapeGeometry_getType.
AR_DEFINE_ENUM(ArStreetscapeGeometryType){
    /// This geometry represents the ground or floor.
    AR_STREETSCAPE_GEOMETRY_TYPE_TERRAIN = 0,
    /// This geometry represents a building or other structure.
    AR_STREETSCAPE_GEOMETRY_TYPE_BUILDING = 1,
};

/// @ingroup ArStreetscapeGeometry
/// Describes the quality of the mesh data. The values correspond to the levels
/// of detail (LOD) defined by the <a
/// href="https://portal.ogc.org/files/?artifact_id=16675">CityGML 2.0
/// standard</a>.
///
/// Obtained by @c ::ArStreetscapeGeometry_getQuality.
AR_DEFINE_ENUM(ArStreetscapeGeometryQuality){
    /// The quality of the geometry is not defined, e.g. when the @c
    /// ::ArStreetscapeGeometryType is @c #AR_STREETSCAPE_GEOMETRY_TYPE_TERRAIN.
    AR_STREETSCAPE_GEOMETRY_QUALITY_NONE = 0,
    /// The @c #AR_STREETSCAPE_GEOMETRY_TYPE_BUILDING geometry is the building
    /// footprint extruded up to a single flat top. The building contains empty
    /// space above any angled roofs.
    AR_STREETSCAPE_GEOMETRY_QUALITY_BUILDING_LOD_1 = 1,
    /// The @c #AR_STREETSCAPE_GEOMETRY_TYPE_BUILDING geometry is the building
    /// footprint with rough heightmap. The geometry will closely follow simple
    /// angled roofs. Chimneys and roof vents on top of roofs will poke outside
    /// of the Streetscape Geometry.
    AR_STREETSCAPE_GEOMETRY_QUALITY_BUILDING_LOD_2 = 2,
};

/// @ingroup ArInstantPlacementPoint
/// Used in @c ::ArConfig to indicate whether Instant Placement should be
/// enabled or disabled. Default value is @c
/// #AR_INSTANT_PLACEMENT_MODE_DISABLED.
AR_DEFINE_ENUM(ArInstantPlacementMode){
    /// Instant Placement is disabled.

    /// When Instant Placement is disabled, any @c ::ArInstantPlacementPoint
    /// that has
    /// @c
    /// #AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_SCREENSPACE_WITH_APPROXIMATE_DISTANCE<!--NOLINT-->
    /// tracking method will result in tracking state becoming permanently
    /// @c #AR_TRACKING_STATE_STOPPED.
    AR_INSTANT_PLACEMENT_MODE_DISABLED = 0,

    /// Enable Instant Placement. If the hit test is successful,
    /// @c ::ArFrame_hitTestInstantPlacement will return a single
    /// @c ::ArInstantPlacementPoint with the +Y pointing upward, against
    /// gravity. Otherwise, returns an empty result set.
    ///
    /// This mode is currently intended to be used with hit tests against
    /// horizontal surfaces.
    ///
    /// Hit tests may also be performed against surfaces with any orientation,
    /// however:
    ///  - The resulting Instant Placement point will always have a pose
    ///    with +Y pointing upward, against gravity.
    ///  - No guarantees are made with respect to orientation of +X and +Z.
    ///    Specifically, a hit test against a vertical surface, such as a wall,
    ///    will not result in a pose that's in any way aligned to the plane of
    ///    the wall, other than +Y being up, against gravity.
    ///  - The @c ::ArInstantPlacementPoint's tracking method may
    ///    never become
    ///    @c #AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_FULL_TRACKING or may
    ///    take a long time to reach this state. The tracking method remains
    ///    @c
    ///    #AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_SCREENSPACE_WITH_APPROXIMATE_DISTANCE.<!--NOLINT-->
    ///    until a (tiny) horizontal plane is fitted at the point of the hit
    ///    test.
    AR_INSTANT_PLACEMENT_MODE_LOCAL_Y_UP = 2,
};

/// @ingroup ArFrame
/// 2d coordinate systems supported by ARCore.
AR_DEFINE_ENUM(ArCoordinates2dType){
    /// GPU texture, (x,y) in pixels.
    AR_COORDINATES_2D_TEXTURE_TEXELS = 0,
    /// GPU texture coordinates, (s,t) normalized to [0.0f, 1.0f] range.
    AR_COORDINATES_2D_TEXTURE_NORMALIZED = 1,
    /// CPU image, (x,y) in pixels. The range of x and y is determined by the
    /// CPU image resolution.
    AR_COORDINATES_2D_IMAGE_PIXELS = 2,
    /// CPU image, (x,y) normalized to [0.0f, 1.0f] range.
    AR_COORDINATES_2D_IMAGE_NORMALIZED = 3,
    /// OpenGL Normalized Device Coordinates, display-rotated, (x,y) normalized
    /// to [-1.0f, 1.0f] range.
    AR_COORDINATES_2D_OPENGL_NORMALIZED_DEVICE_COORDINATES = 6,
    /// Android view, display-rotated, (x,y) in pixels.
    AR_COORDINATES_2D_VIEW = 7,
    /// Android view, display-rotated, (x,y) normalized to [0.0f, 1.0f] range.
    AR_COORDINATES_2D_VIEW_NORMALIZED = 8,

};

/// @ingroup ArFrame
/// 3d coordinate systems supported by ARCore.
AR_DEFINE_ENUM(ArCoordinates3dType){
    /// GPU texture coordinates, using the Z component to compensate for
    /// perspective shift when using Electronic Image Stabilization (EIS).
    ///
    /// <p>Use with @c ::ArFrame_transformCoordinates3d. See the <a
    /// href="https://developers.google.com/ar/develop/c/electronic-image-stabilization">Electronic
    /// Image Stabilization developer guide</a> for more information.
    AR_COORDINATES_3D_EIS_TEXTURE_NORMALIZED = 0,
    /// Normalized Device Coordinates (NDC), display-rotated, (x,y) normalized
    /// to [-1.0f, 1.0f] range to compensate for perspective shift for EIS.
    ///
    /// <p>Use with @c ::ArFrame_transformCoordinates3d. See the <a
    /// href="https://developers.google.com/ar/develop/c/electronic-image-stabilization">Electronic
    /// Image Stabilization developer guide</a> for more information.
    AR_COORDINATES_3D_EIS_NORMALIZED_DEVICE_COORDINATES = 1,
};

/// @ingroup ArCameraConfig
/// Describes the direction a camera is facing relative to the device.  Used by
/// @c ::ArCameraConfig_getFacingDirection.
AR_DEFINE_ENUM(ArCameraConfigFacingDirection){
    /// Camera looks out the back of the device (away from the user).
    AR_CAMERA_CONFIG_FACING_DIRECTION_BACK = 0,
    /// Camera looks out the front of the device (towards the user).
    /// To create a session using the front-facing (selfie) camera, use
    /// @c ::ArSession_setCameraConfig to set a front-facing (selfie) camera
    /// config retrieved from @c
    /// ::ArSession_getSupportedCameraConfigsWithFilter.
    ///
    /// When the front camera is selected,
    /// ARCore's behavior changes in the following ways:
    ///
    /// - The display will be mirrored. Specifically,
    ///   @c ::ArCamera_getProjectionMatrix will include a horizontal flip in
    ///   the generated projection matrix and APIs that reason about things in
    ///   screen space, such as @c ::ArFrame_transformCoordinates2d, will mirror
    ///   screen coordinates. Open GL apps should consider using @c glFrontFace
    ///   to render mirrored assets without changing their winding direction.
    /// - @c ::ArCamera_getTrackingState will always output
    ///   @c #AR_TRACKING_STATE_PAUSED.
    /// - @c ::ArFrame_hitTest will always output an empty list.
    /// - @c ::ArCamera_getDisplayOrientedPose will always output an identity
    ///   pose.
    /// - @c ::ArSession_acquireNewAnchor will always return
    ///   @c #AR_ERROR_NOT_TRACKING.
    /// - Planes will never be detected.
    /// - @c ::ArSession_configure will fail if the supplied configuration
    ///   requests Cloud Anchors, Augmented Images, or Environmental HDR
    ///   Lighting Estimation mode.
    AR_CAMERA_CONFIG_FACING_DIRECTION_FRONT = 1};

/// @ingroup ArFuture
/// Base type for asynchronous operations. See
/// @ref future_concept "Futures in ARCore" for more details.
///
/// An acquired future must eventually be released using @c ::ArFuture_release.
/// (@ref ownership "reference type, long-lived").
typedef struct ArFuture_ ArFuture;

/// @ingroup ArFuture
/// The state of an asynchronous operation.
AR_DEFINE_ENUM(ArFutureState){
    /// The operation is still pending. The result of the operation isn't
    /// available yet and any associated callback hasn't yet been dispatched or
    /// invoked. Do not use this to check if the operation can be cancelled as
    /// the state can change from another thread between the calls to
    /// @c ::ArFuture_getState and @c ::ArFuture_cancel.
    AR_FUTURE_STATE_PENDING = 0,

    /// The operation has been cancelled. Any associated callback will never be
    /// invoked.
    AR_FUTURE_STATE_CANCELLED = 1,

    /// The operation is complete and the result is available. If a callback was
    /// associated with this future, it will soon be invoked with the result on
    /// the main thread, if it hasn't been invoked already.
    AR_FUTURE_STATE_DONE = 2,
};

// Describe the Geospatial anchor type. Do not use. For internal use only.
AR_DEFINE_ENUM(ArGeospatialAnchorType){
    AR_GEOSPATIAL_ANCHOR_TYPE_UNKNOWN = 0,
    // Anchor with altitude relative to WGS84 Earth ellipsoid's mean sea level.
    AR_GEOSPATIAL_ANCHOR_TYPE_WGS84 = 1,
    // Anchor with altitude relative to ground floors or ground terrain.
    AR_GEOSPATIAL_ANCHOR_TYPE_TERRAIN = 2,
    // Anchor with altitude relative to building rooftops.
    AR_GEOSPATIAL_ANCHOR_TYPE_ROOFTOP = 3,
};

// Describe the Geospatial Creator Platform. Do not use. For internal use only.
AR_DEFINE_ENUM(ArGeospatialCreatorPlatform){
    // The platform cannot be determined, because the client did not explicitly
    // report this field. This is the default value.
    AR_GEOSPATIAL_CREATOR_PLATFORM_UNKNOWN = 0,

    // Geospatial Creator is available on the platform, but was not used for
    // this AR application.
    AR_GEOSPATIAL_CREATOR_PLATFORM_DISABLED = 1,

    // Geospatial Creator was used, but the platform is not defined in an enum.
    // This is useful for implementing backwards compatibility for 3rd party
    // partners, though subsequent ARCore releases should have an appropriate
    // enum value exposed.
    AR_GEOSPATIAL_CREATOR_PLATFORM_OTHER = 2,

    // Geospatial Creator for the ARCore Extensions SDK was used to build this
    // this AR application.
    AR_GEOSPATIAL_CREATOR_PLATFORM_ARCORE_EXTENSIONS_UNITY = 3,

    // Geospatial Creator for Adobe Aero was used to build this AR application.
    AR_GEOSPATIAL_CREATOR_PLATFORM_ADOBE_AERO = 4,
};

/// @ingroup ArHostCloudAnchorFuture
/// Handle to an asynchronous operation launched by
/// @c ::ArSession_hostCloudAnchorAsync. See the <a
///  href="https://developers.google.com/ar/develop/c/cloud-anchors/developer-guide">Cloud
///  Anchors developer guide</a> for more information.
///
/// Release with @c ::ArFuture_release.
/// (@ref ownership "reference type, long-lived").
typedef struct ArHostCloudAnchorFuture_ ArHostCloudAnchorFuture;

#ifdef __cplusplus
/// @ingroup type_conversions
/// Upcasts to @c ::ArFuture
inline ArFuture *ArAsFuture(ArHostCloudAnchorFuture *future) {
  return reinterpret_cast<ArFuture *>(future);
}
#endif  // __cplusplus

/// @ingroup ArResolveCloudAnchorFuture
/// Handle to an asynchronous operation launched by
/// @c ::ArSession_resolveCloudAnchorAsync.
/// Release with @c ::ArFuture_release.
/// (@ref ownership "reference type, long-lived").
typedef struct ArResolveCloudAnchorFuture_ ArResolveCloudAnchorFuture;

#ifdef __cplusplus
/// @ingroup type_conversions
/// Upcasts to @c ::ArFuture
inline ArFuture *ArAsFuture(ArResolveCloudAnchorFuture *future) {
  return reinterpret_cast<ArFuture *>(future);
}
#endif  // __cplusplus

/// @ingroup ArResolveAnchorOnTerrainFuture
/// Handle to an asynchronous operation launched by
/// @c ::ArEarth_resolveAnchorOnTerrainAsync.
///
/// Release with @c ::ArFuture_release.
/// (@ref ownership "reference type, long-lived").
typedef struct ArResolveAnchorOnTerrainFuture_ ArResolveAnchorOnTerrainFuture;

#ifdef __cplusplus
/// @ingroup type_conversions
/// Upcasts to @c ::ArFuture
inline ArFuture *ArAsFuture(ArResolveAnchorOnTerrainFuture *future) {
  return reinterpret_cast<ArFuture *>(future);
}
#endif  // __cplusplus

/// @ingroup ArResolveAnchorOnRooftopFuture
/// Handle to an asynchronous operation launched by
/// @c ::ArEarth_resolveAnchorOnRooftopAsync.
/// Release with @c ::ArFuture_release.
/// (@ref ownership "reference type, long-lived").
typedef struct ArResolveAnchorOnRooftopFuture_ ArResolveAnchorOnRooftopFuture;

#ifdef __cplusplus
/// @ingroup type_conversions
/// Upcasts to @c ::ArFuture
inline ArFuture *ArAsFuture(ArResolveAnchorOnRooftopFuture *future) {
  return reinterpret_cast<ArFuture *>(future);
}
#endif  // __cplusplus

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

// Note: Destroy functions do not take ArSession* to allow late destruction in
// finalizers of garbage-collected languages such as Java.

/// @ingroup ArCoreApk
/// Determines if ARCore is supported on this device. This may initiate a query
/// with a remote service to determine if the device is compatible, in which
/// case it will return immediately with @p out_availability set to
/// @c #AR_AVAILABILITY_UNKNOWN_CHECKING.
///
/// For ARCore-required apps (as indicated by the <a
/// href="https://developers.google.com/ar/develop/c/enable-arcore#ar_required">manifest
/// @c meta-data </a>) this function will assume device compatibility and will
/// always immediately return one of @c #AR_AVAILABILITY_SUPPORTED_INSTALLED,
/// @c #AR_AVAILABILITY_SUPPORTED_APK_TOO_OLD, or
/// @c #AR_AVAILABILITY_SUPPORTED_NOT_INSTALLED.
///
/// Note: A result @c #AR_AVAILABILITY_SUPPORTED_INSTALLED only indicates
/// presence of a suitably versioned ARCore APK. Session creation may still fail
/// if the ARCore APK has been side-loaded onto an unsupported device.
///
/// May be called prior to @c ::ArSession_create.
///
/// @param[in] env The application's @c JNIEnv object
/// @param[in] context A @c JOBject for an Android @c Context.
/// @param[out] out_availability A pointer to an @c ::ArAvailability to receive
///     the result.
void ArCoreApk_checkAvailability(void *env,
                                 void *context,
                                 ArAvailability *out_availability);

/// @ingroup ArCoreApk
/// Callback definition for @c ::ArCoreApk_checkAvailabilityAsync. The
/// @p callback_context argument will be the same as that passed to
/// @c ::ArCoreApk_checkAvailabilityAsync.
///
/// It is a best practice to free @c callback_context memory provided to
/// @c ::ArCoreApk_checkAvailabilityAsync at the end of the callback
/// implementation.
typedef void (*ArAvailabilityCallback)(void *callback_context,
                                       ArAvailability availability);

/// @ingroup ArCoreApk
/// Asynchronously determines if ARCore is supported on this device. This may
/// initiate a query with a remote service to determine if the device is
/// compatible.
///
/// The callback will be invoked asynchronously on the Main thread.
/// Unlike the synchronous function @c ::ArCoreApk_checkAvailability, the result
/// will never be @c #AR_AVAILABILITY_UNKNOWN_CHECKING.
///
/// For ARCore-required apps (as indicated by the <a
/// href="https://developers.google.com/ar/develop/c/enable-arcore#ar_required">manifest
/// @c meta-data </a>) this function will assume device compatibility and the
/// callback will be asynchronously invoked on the Main thread with one of
/// @c #AR_AVAILABILITY_SUPPORTED_INSTALLED,
/// @c #AR_AVAILABILITY_SUPPORTED_APK_TOO_OLD, or
/// @c #AR_AVAILABILITY_SUPPORTED_NOT_INSTALLED.
///
/// Note: A result @c #AR_AVAILABILITY_SUPPORTED_INSTALLED only indicates
/// presence of a suitably versioned ARCore APK. Session creation may still fail
/// if the ARCore APK has been side-loaded onto an unsupported device.
///
/// May be called prior to @c ::ArSession_create.
///
/// @param[in] env The current thread's @c JNIEnv object
/// @param[in] application_context A @c JOBject for an Android @c Context.
/// @param[in] callback_context An arbitrary pointer which will be passed as the
///     first argument to the callback.
/// @param[in] callback The callback to invoke with the result on the Main
///     thread. Must not be @c NULL.
void ArCoreApk_checkAvailabilityAsync(void *env,
                                      void *application_context,
                                      void *callback_context,
                                      ArAvailabilityCallback callback);

/// @ingroup ArCoreApk
/// On supported devices initiates download and installation of
/// Google Play Services for AR (ARCore) and ARCore device profile data, see
/// https://developers.google.com/ar/develop/c/enable-arcore.
///
/// Do not call this function unless @c ::ArCoreApk_checkAvailability has
/// returned either @c #AR_AVAILABILITY_SUPPORTED_APK_TOO_OLD or
/// @c #AR_AVAILABILITY_SUPPORTED_NOT_INSTALLED.
///
/// When your application launches or wishes to enter AR mode, call this
/// function with @p user_requested_install = 1.
///
/// If Google Play Services for AR and device profile data are fully installed
/// and up to date, this function will set @p out_install_status to
/// @c #AR_INSTALL_STATUS_INSTALLED.
///
/// If Google Play Services for AR or device profile data is not installed or
/// not up to date, the function will set @p out_install_status to
/// @c #AR_INSTALL_STATUS_INSTALL_REQUESTED and return immediately. The current
/// activity will then pause while the user is prompted to install
/// Google Play Services for AR (market://details?id=com.google.ar.core) and/or
/// ARCore downloads required device profile data.
///
/// When your activity resumes, call this function again, this time with
/// @p user_requested_install = 0. This will either set @p out_install_status
/// to @c #AR_INSTALL_STATUS_INSTALLED or return an error code indicating the
/// reason that installation could not be completed.
///
/// Once this function returns with @p out_install_status set to
/// @c #AR_INSTALL_STATUS_INSTALLED, it is safe to call @c ::ArSession_create.
///
/// Side-loading Google Play Services for AR (ARCore) on unsupported devices
/// will not work. Although @c ::ArCoreApk_checkAvailability may return
/// @c #AR_AVAILABILITY_SUPPORTED_APK_TOO_OLD or
/// @c #AR_AVAILABILITY_SUPPORTED_INSTALLED after side-loading the ARCore APK,
/// the device will still fail to create an AR session, because it is unable to
/// locate the required ARCore device profile data.
///
/// For more control over the message displayed and ease of exiting the process,
/// see @c ::ArCoreApk_requestInstallCustom.
///
/// <b>Caution:</b> The value of @p *out_install_status is only valid when
/// @c #AR_SUCCESS is returned.  Otherwise this value must be ignored.
///
/// @param[in] env The application's @c JNIEnv object
/// @param[in] application_activity A @c JObject referencing the application's
///     current Android @c Activity.
/// @param[in] user_requested_install if set, override the previous installation
///     failure message and always show the installation interface.
/// @param[out] out_install_status A pointer to an @c ::ArInstallStatus to
///     receive the resulting install status, if successful. Value is only valid
///     when the return value is @c #AR_SUCCESS.
/// @return @c #AR_SUCCESS, or any of:
/// - @c #AR_ERROR_FATAL if an error occurs while checking for or requesting
///   installation
/// - @c #AR_UNAVAILABLE_DEVICE_NOT_COMPATIBLE if ARCore is not supported
///   on this device.
/// - @c #AR_UNAVAILABLE_USER_DECLINED_INSTALLATION if the user previously
///   declined installation.
ArStatus ArCoreApk_requestInstall(void *env,
                                  void *application_activity,
                                  int32_t user_requested_install,
                                  ArInstallStatus *out_install_status);

/// @ingroup ArCoreApk
/// Initiates installation of Google Play Services for AR (ARCore) and required
/// device profile data, with configurable behavior.
///
/// This is a more flexible version of @c ::ArCoreApk_requestInstall, allowing
/// the application control over the initial informational dialog and ease of
/// exiting or cancelling the installation.
///
/// Refer to @c ::ArCoreApk_requestInstall for correct use and expected runtime
/// behavior.
///
/// @param[in] env The application's @c JNIEnv object
/// @param[in] application_activity A @c JObject referencing the application's
///     current Android @c Activity.
/// @param[in] user_requested_install if set, override the previous installation
///     failure message and always show the installation interface.
/// @param[in] install_behavior controls the presence of the cancel button at
///     the user education screen and if tapping outside the education screen or
///     install-in-progress screen causes them to dismiss.
/// @param[in] message_type controls the text of the of message displayed
///     before showing the install prompt, or disables display of this message.
/// @param[out] out_install_status A pointer to an @c ::ArInstallStatus to
///     receive the resulting install status, if successful. Value is only valid
///     when the return value is @c #AR_SUCCESS.
/// @return @c #AR_SUCCESS, or any of:
/// - @c #AR_ERROR_FATAL if an error occurs while checking for or requesting
///   installation
/// - @c #AR_UNAVAILABLE_DEVICE_NOT_COMPATIBLE if ARCore is not supported
///   on this device.
/// - @c #AR_UNAVAILABLE_USER_DECLINED_INSTALLATION if the user previously
///   declined installation.
ArStatus ArCoreApk_requestInstallCustom(void *env,
                                        void *application_activity,
                                        int32_t user_requested_install,
                                        ArInstallBehavior install_behavior,
                                        ArInstallUserMessageType message_type,
                                        ArInstallStatus *out_install_status);

/// @ingroup ArSession
/// Creates a new ARCore session.  Prior to calling this function, your app must
/// check that ARCore is installed by verifying that either:
///
/// - @c ::ArCoreApk_requestInstall or @c ::ArCoreApk_requestInstallCustom
/// returns
///   @c #AR_INSTALL_STATUS_INSTALLED, or
/// - @c ::ArCoreApk_checkAvailability returns
/// @c #AR_AVAILABILITY_SUPPORTED_INSTALLED.
///
/// This check must be performed prior to creating an @c ::ArSession, otherwise
/// @c ::ArSession creation will fail, and subsequent installation or upgrade of
/// ARCore will require an app restart and might cause Android to kill your app.
///
/// @param[in]  env                 The application's @c JNIEnv object
/// @param[in]  context A @c JObject for an Android @c Context
/// @param[out] out_session_pointer A pointer to an @c ::ArSession* to receive
///     the address of the newly allocated session.
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_FATAL if an internal error occurred while creating the
///   session. `adb logcat` may contain useful information.
/// - @c #AR_ERROR_CAMERA_PERMISSION_NOT_GRANTED if your app does not have the
///   [CAMERA](https://developer.android.com/reference/android/Manifest.permission.html#CAMERA)
///   permission.
/// - @c #AR_UNAVAILABLE_ARCORE_NOT_INSTALLED if the ARCore APK is not present.
///   This can be prevented by the installation check described above.
/// - @c #AR_UNAVAILABLE_DEVICE_NOT_COMPATIBLE if the device is not compatible
///   with ARCore.  If encountered after completing the installation check, this
///   usually indicates a user has side-loaded ARCore onto an incompatible
///   device.
/// - @c #AR_UNAVAILABLE_APK_TOO_OLD if the installed ARCore APK is too old for
///   the ARCore SDK with which this application was built. This can be
///   prevented by the installation check described above.
/// - @c #AR_UNAVAILABLE_SDK_TOO_OLD if the ARCore SDK that this app was built
///   with is too old and no longer supported by the installed ARCore APK.
ArStatus ArSession_create(void *env,
                          void *context,
                          ArSession **out_session_pointer);

/// @ingroup ArSession
/// Creates a new ARCore session requesting additional features.  Prior to
/// calling this function, your app must check that ARCore is installed by
/// verifying that either:
///
/// - @c ::ArCoreApk_requestInstall or @c ::ArCoreApk_requestInstallCustom
/// returns
///   @c #AR_INSTALL_STATUS_INSTALLED, or
/// - @c ::ArCoreApk_checkAvailability returns
///   @c #AR_AVAILABILITY_SUPPORTED_INSTALLED.
///
/// This check must be performed prior to creating an @c ::ArSession, otherwise
/// @c ::ArSession creation will fail, and subsequent installation or upgrade of
/// ARCore will require an app restart and might cause Android to kill your app.
///
/// @param[in]  env                 The application's @c JNIEnv object
/// @param[in]  context A @c JObject for an Android @c Context
/// @param[in]  features            The list of requested features, terminated
///     by with @c #AR_SESSION_FEATURE_END_OF_LIST.
/// @param[out] out_session_pointer A pointer to an @c ::ArSession* to receive
///     the address of the newly allocated session.
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_FATAL if an internal error occurred while creating the
///   session. `adb logcat` may contain useful information.
/// - @c #AR_ERROR_CAMERA_PERMISSION_NOT_GRANTED if your app does not have the
///   [CAMERA](https://developer.android.com/reference/android/Manifest.permission.html#CAMERA)
///   permission.
/// - @c #AR_ERROR_INVALID_ARGUMENT if the requested features are mutually
///   incompatible.  See @c ::ArSessionFeature for details.
/// - @c #AR_UNAVAILABLE_ARCORE_NOT_INSTALLED if the ARCore APK is not present.
///   This can be prevented by the installation check described above.
/// - @c #AR_UNAVAILABLE_DEVICE_NOT_COMPATIBLE if the device is not compatible
///   with ARCore.  If encountered after completing the installation check, this
///   usually indicates a user has side-loaded ARCore onto an incompatible
///   device.
/// - @c #AR_UNAVAILABLE_APK_TOO_OLD if the installed ARCore APK is too old for
///   the ARCore SDK with which this application was built. This can be
///   prevented by the installation check described above.
/// - @c #AR_UNAVAILABLE_SDK_TOO_OLD if the ARCore SDK that this app was built
///   with is too old and no longer supported by the installed ARCore APK.
ArStatus ArSession_createWithFeatures(void *env,
                                      void *context,
                                      const ArSessionFeature *features,
                                      ArSession **out_session_pointer);

// === ArFuture functions ===

/// @ingroup ArFuture
/// Gets the state of an asynchronous operation. See @c ::ArFutureState and
/// @ref future_concept "Futures in ARCore" for more details.
///
/// @param[in] session    The ARCore session.
/// @param[in] future     The handle for the asynchronous operation.
/// @param[out] out_state The state of the operation.
void ArFuture_getState(const ArSession *session,
                       const ArFuture *future,
                       ArFutureState *out_state);

/// @ingroup ArFuture
/// Tries to cancel execution of this operation. @p out_was_cancelled will be
/// set to 1 if the operation was cancelled by this invocation, and in that case
/// it is a best practice to free @c context memory provided to the
/// @ref future_callback "callback", if any.
///
/// @param[in] session The ARCore session.
/// @param[in] future The handle for the asynchronous operation.
/// @param[out] out_was_cancelled Set to 1 if this invocation successfully
///     cancelled the operation, 0 otherwise. You may pass @c NULL.
void ArFuture_cancel(const ArSession *session,
                     ArFuture *future,
                     int32_t *out_was_cancelled);

/// @ingroup ArFuture
/// Releases a reference to a future. This does not mean that the operation will
/// be terminated - see @ref future_cancellation "cancelling a future".
///
/// This function may safely be called with @c NULL - it will do nothing.
void ArFuture_release(ArFuture *future);

// === ArConfig functions ===

/// @ingroup ArConfig
/// Creates a new configuration object and initializes it to a sensible default
/// configuration. Plane detection and Lighting Estimation are enabled, and
/// blocking update is selected. This configuration is guaranteed to be
/// supported on all devices that support ARCore.
void ArConfig_create(const ArSession *session, ArConfig **out_config);

/// @ingroup ArConfig
/// Releases memory used by the provided configuration object.
void ArConfig_destroy(ArConfig *config);

/// @ingroup ArConfig
/// Stores the currently configured @c ::ArLightEstimationMode mode into
/// @p *light_estimation_mode.
void ArConfig_getLightEstimationMode(
    const ArSession *session,
    const ArConfig *config,
    ArLightEstimationMode *light_estimation_mode);

/// @ingroup ArConfig
/// Sets the desired @c ::ArLightEstimationMode. See @c ::ArLightEstimationMode
/// for available options.
void ArConfig_setLightEstimationMode(
    const ArSession *session,
    ArConfig *config,
    ArLightEstimationMode light_estimation_mode);

/// @ingroup ArConfig
/// Stores the currently configured plane finding mode into
/// @p *plane_finding_mode.
void ArConfig_getPlaneFindingMode(const ArSession *session,
                                  const ArConfig *config,
                                  ArPlaneFindingMode *plane_finding_mode);

/// @ingroup ArConfig
/// Sets the desired plane finding mode. See @c ::ArPlaneFindingMode for
/// available options.
void ArConfig_setPlaneFindingMode(const ArSession *session,
                                  ArConfig *config,
                                  ArPlaneFindingMode plane_finding_mode);

/// @ingroup ArConfig
/// Stores the currently configured behavior of @c ::ArSession_update into
/// @p *update_mode.
void ArConfig_getUpdateMode(const ArSession *session,
                            const ArConfig *config,
                            ArUpdateMode *update_mode);

/// @ingroup ArConfig
/// Sets the behavior of @c ::ArSession_update. See
/// @c ::ArUpdateMode for available options.
void ArConfig_setUpdateMode(const ArSession *session,
                            ArConfig *config,
                            ArUpdateMode update_mode);

/// @ingroup ArConfig
/// Gets the current Cloud Anchor mode from the @c ::ArConfig.
void ArConfig_getCloudAnchorMode(const ArSession *session,
                                 const ArConfig *config,
                                 ArCloudAnchorMode *out_cloud_anchor_mode);

/// @ingroup ArConfig
/// Sets the desired cloud configuration. See @c ::ArCloudAnchorMode for
/// available options.
void ArConfig_setCloudAnchorMode(const ArSession *session,
                                 ArConfig *config,
                                 ArCloudAnchorMode cloud_anchor_mode);

/// @ingroup ArConfig
/// Sets the image database in the session configuration.
///
/// Any images in the currently active image database that have a
/// @c #AR_TRACKING_STATE_TRACKING or @c #AR_TRACKING_STATE_PAUSED state will
/// immediately be set to the @c #AR_TRACKING_STATE_STOPPED state if a different
/// or @c NULL image database is set.
///
/// This function makes a copy of the image database.
void ArConfig_setAugmentedImageDatabase(
    const ArSession *session,
    ArConfig *config,
    const ArAugmentedImageDatabase *augmented_image_database);

/// @ingroup ArConfig
/// Returns the image database from the session configuration.
///
/// This function returns a copy of the internally stored image database, so any
/// changes to the copy will not affect the current configuration or session.
///
/// If no @c ::ArAugmentedImageDatabase has been configured, a new empty
/// database will be constructed using @c ::ArAugmentedImageDatabase_create.
void ArConfig_getAugmentedImageDatabase(
    const ArSession *session,
    const ArConfig *config,
    ArAugmentedImageDatabase *out_augmented_image_database);

/// @ingroup ArConfig
/// Stores the desired texture update mode from the given @p config into
/// @p out_texture_update_mode.
void ArConfig_getTextureUpdateMode(
    const ArSession *session,
    const ArConfig *config,
    ArTextureUpdateMode *out_texture_update_mode);

/// @ingroup ArConfig
/// Sets the desired texture update mode. See @c ::ArTextureUpdateMode for
/// available options.
///
/// Some values for @p texture_update_mode may not be supported on the current
/// device. If an unsupported value is set, @c ::ArSession_configure will throw
/// @c #AR_ERROR_UNSUPPORTED_CONFIGURATION.
void ArConfig_setTextureUpdateMode(const ArSession *session,
                                   ArConfig *config,
                                   ArTextureUpdateMode texture_update_mode);

/// @ingroup ArConfig
/// Stores the currently configured augmented face mode into
/// @p *augmented_face_mode.
void ArConfig_getAugmentedFaceMode(const ArSession *session,
                                   const ArConfig *config,
                                   ArAugmentedFaceMode *augmented_face_mode);

/// @ingroup ArConfig
/// Sets the desired face mode. See @c ::ArAugmentedFaceMode for
/// available options. Augmented Faces is currently only supported when using
/// the front-facing (selfie) camera.
void ArConfig_setAugmentedFaceMode(const ArSession *session,
                                   ArConfig *config,
                                   ArAugmentedFaceMode augmented_face_mode);

/// @ingroup ArConfig
/// On supported devices, selects the desired camera focus mode. On these
/// devices, the default desired focus mode is currently @c
/// #AR_FOCUS_MODE_FIXED, although this default might change in the future. See
/// the <a
/// href="https://developers.google.com/ar/devices">ARCore
/// supported devices</a> page for a list of devices on which ARCore does not
/// support changing the desired focus mode.
///
/// For optimal AR tracking performance, use the focus mode provided by the
/// default session config. While capturing pictures or video, use
/// @c #AR_FOCUS_MODE_AUTO. For optimal AR  tracking, revert to the default
/// focus mode once auto focus behavior is no longer needed. If your app
/// requires fixed focus camera, set @c #AR_FOCUS_MODE_FIXED before enabling the
/// AR session. This ensures that your app always uses fixed focus, even if the
/// default camera config focus mode changes in a future release.
///
/// To determine whether the configured ARCore camera supports auto focus, check
/// @c ACAMERA_LENS_INFO_MINIMUM_FOCUS_DISTANCE, which is 0 for fixed-focus
/// cameras.
///
/// The desired focus mode is ignored while an MP4 dataset file is being played
/// back.
void ArConfig_setFocusMode(const ArSession *session,
                           ArConfig *config,
                           ArFocusMode focus_mode);

/// @ingroup ArConfig
/// Stores the currently configured desired focus mode into @p *focus_mode.
/// Note: The desired focus mode may be different from the actual focus
/// mode. See the
/// <a href="https://developers.google.com/ar/devices">ARCore
/// supported devices page</a> for a list of affected devices.
void ArConfig_getFocusMode(const ArSession *session,
                           ArConfig *config,
                           ArFocusMode *focus_mode);

/// @ingroup ArConfig
/// Gets the current camera flash mode set on this config object.
///
/// @param[in]    session          The ARCore session.
/// @param[in]    config           The configuration object.
/// @param[inout] out_flash_mode   The current camera flash mode.
void ArConfig_getFlashMode(const ArSession *session,
                           const ArConfig *config,
                           ArFlashMode *out_flash_mode);

/// @ingroup ArConfig
/// Sets the camera flash mode.
///
/// See @c ::ArFlashMode for available options. The default flash
/// mode is @c #AR_FLASH_MODE_OFF. Note, on devices where camera flash hardware
/// is not supported, calling this function will do nothing.
/// See the <a
/// href="https://developers.google.com/ar/develop/camera/flash/c">documentation
/// page</a> for more information on how to query device flash capability.
///
/// @param[in]   session      The ARCore session.
/// @param[in]   config       The configuration object.
/// @param[in]   flash_mode   The desired camera flash mode.
void ArConfig_setFlashMode(const ArSession *session,
                           ArConfig *config,
                           ArFlashMode flash_mode);

/// @ingroup ArConfig
/// Gets the camera image stabilization mode set on this config
/// object.
///
/// @param[in]    session          The ARCore session.
/// @param[in]    config           The configuration object.
/// @param[inout] out_image_stabilization_mode     The current EIS mode.
void ArConfig_getImageStabilizationMode(
    const ArSession *session,
    const ArConfig *config,
    ArImageStabilizationMode *out_image_stabilization_mode);

/// @ingroup ArConfig
/// Sets the camera image stabilization mode.
///
/// See @c ::ArImageStabilizationMode for available options. Currently, the
/// default image stabilization mode is @c #AR_IMAGE_STABILIZATION_MODE_OFF.
///
/// @param[in]   session      The ARCore session.
/// @param[in]   config       The configuration object.
/// @param[in]   image_stabilization_mode     The desired camera image
/// stabilization mode.
void ArConfig_setImageStabilizationMode(
    const ArSession *session,
    ArConfig *config,
    ArImageStabilizationMode image_stabilization_mode);

/// @ingroup ArConfig
/// Sets the Geospatial mode. See @c ::ArGeospatialMode for available options.
void ArConfig_setGeospatialMode(const ArSession *session,
                                ArConfig *config,
                                ArGeospatialMode geospatial_mode);

/// @ingroup ArConfig
/// Gets the current geospatial mode set on the configuration.
///
/// @param[in]   session             The ARCore session.
/// @param[in]   config              The configuration object.
/// @param[out]  out_geospatial_mode The current geospatial mode set on the
///     config.
void ArConfig_getGeospatialMode(const ArSession *session,
                                const ArConfig *config,
                                ArGeospatialMode *out_geospatial_mode);

/// @ingroup ArConfig
/// Sets the desired configuration for the Streetscape Geometry API. See @c
/// ::ArStreetscapeGeometryMode for available options and usage information.
void ArConfig_setStreetscapeGeometryMode(
    const ArSession *session,
    ArConfig *config,
    ArStreetscapeGeometryMode streetscape_geometry_mode);

/// @ingroup ArConfig
/// Gets the current Streetscape Geometry mode set on the configuration.
///
/// @param[in]   session             The ARCore session.
/// @param[in]   config              The configuration object.
/// @param[out]  out_streetscape_geometry_mode The current @c
///   ::ArStreetscapeGeometry mode set on the config.
void ArConfig_getStreetscapeGeometryMode(
    const ArSession *session,
    const ArConfig *config,
    ArStreetscapeGeometryMode *out_streetscape_geometry_mode);

/// @ingroup ArConfig
/// Gets the currently configured desired @c ::ArDepthMode.
void ArConfig_getDepthMode(const ArSession *session,
                           const ArConfig *config,
                           ArDepthMode *out_depth_mode);

/// @ingroup ArConfig
/// Gets the currently configured @c ::ArSemanticMode.
void ArConfig_getSemanticMode(const ArSession *session,
                              const ArConfig *config,
                              ArSemanticMode *out_semantic_mode);

/// @ingroup ArConfig
/// Sets the desired @c ::ArDepthMode.
///
/// Notes:
/// - Not all devices support all modes. Use @c ::ArSession_isDepthModeSupported
///   to determine whether the current device and the selected camera support a
///   particular depth mode.
/// - With depth enabled through this call, calls to
///   @c ::ArFrame_acquireDepthImage16Bits and
///   @c ::ArFrame_acquireRawDepthImage16Bits can be made to acquire the latest
///   computed depth image.
/// - With depth enabled through this call, calling @c ::ArFrame_hitTest
///   generates an @c ::ArHitResultList that also includes @c ::ArDepthPoint
///   values that are sampled from the latest computed depth image.
void ArConfig_setDepthMode(const ArSession *session,
                           ArConfig *config,
                           ArDepthMode mode);

/// @ingroup ArConfig
/// Sets the desired configuration for the Scene Semantics API. See
/// @c ::ArSemanticMode for available options and usage information.
///
/// Not all devices support all modes. Use
/// @c ::ArSession_isSemanticModeSupported to determine whether the current
/// device and the selected camera support a particular semantic mode.
void ArConfig_setSemanticMode(const ArSession *session,
                              ArConfig *config,
                              ArSemanticMode semantic_mode);

/// @ingroup ArConfig
/// Sets the current Instant Placement mode from the @c ::ArConfig.
void ArConfig_setInstantPlacementMode(
    const ArSession *session,
    ArConfig *config,
    ArInstantPlacementMode instant_placement_mode);

/// @ingroup ArConfig
/// Gets the current Instant Placement Region mode from the @c ::ArConfig.
void ArConfig_getInstantPlacementMode(
    const ArSession *session,
    const ArConfig *config,
    ArInstantPlacementMode *instant_placement_mode);

// === ArCameraConfigList and ArCameraConfig functions ===

// === ArCameraConfigList functions ===

/// @ingroup ArCameraConfig
/// Creates a camera config list object.
///
/// @param[in]   session      The ARCore session
/// @param[out]  out_list     A pointer to an @c ::ArCameraConfigList* to
///     receive the address of the newly allocated @c ::ArCameraConfigList.
void ArCameraConfigList_create(const ArSession *session,
                               ArCameraConfigList **out_list);

/// @ingroup ArCameraConfig
/// Releases the memory used by a camera config list object,
/// along with all the camera config references it holds.
void ArCameraConfigList_destroy(ArCameraConfigList *list);

/// @ingroup ArCameraConfig
/// Retrieves the number of camera configs in this list.
void ArCameraConfigList_getSize(const ArSession *session,
                                const ArCameraConfigList *list,
                                int32_t *out_size);

/// @ingroup ArCameraConfig
/// Retrieves the specific camera config based on the position in this list.
void ArCameraConfigList_getItem(const ArSession *session,
                                const ArCameraConfigList *list,
                                int32_t index,
                                ArCameraConfig *out_camera_config);

// === ArCameraConfig functions ===

/// @ingroup ArCameraConfig
/// Creates a camera config object.
///
/// @param[in]   session            The ARCore session
/// @param[out]  out_camera_config  Pointer to an @c ::ArCameraConfig* to
///     receive the address of the newly allocated @c ::ArCameraConfig.
void ArCameraConfig_create(const ArSession *session,
                           ArCameraConfig **out_camera_config);

/// @ingroup ArCameraConfig
/// Releases the memory used by a camera config object.
void ArCameraConfig_destroy(ArCameraConfig *camera_config);

/// @ingroup ArCameraConfig
/// Obtains the CPU camera image dimensions for the given camera config.
void ArCameraConfig_getImageDimensions(const ArSession *session,
                                       const ArCameraConfig *camera_config,
                                       int32_t *out_width,
                                       int32_t *out_height);

/// @ingroup ArCameraConfig
/// Obtains the GPU texture dimensions for the given camera config.
void ArCameraConfig_getTextureDimensions(const ArSession *session,
                                         const ArCameraConfig *camera_config,
                                         int32_t *out_width,
                                         int32_t *out_height);

/// @ingroup ArCameraConfig
/// Obtains the minimum and maximum camera capture rate in frames per second
/// (fps) for the current camera config. Actual capture frame rate will vary
/// within this range, depending on lighting conditions. Frame rates will
/// generally be lower under poor lighting conditions to accommodate longer
/// exposure times.
void ArCameraConfig_getFpsRange(const ArSession *session,
                                const ArCameraConfig *camera_config,
                                int32_t *out_min_fps,
                                int32_t *out_max_fps);

/// @ingroup ArCameraConfig
/// Gets the depth sensor usage settings. @p out_depth_sensor_usage will contain
/// one of the values from @c ::ArCameraConfigDepthSensorUsage enum.
void ArCameraConfig_getDepthSensorUsage(const ArSession *session,
                                        const ArCameraConfig *camera_config,
                                        uint32_t *out_depth_sensor_usage);

/// @ingroup ArCameraConfig
/// Obtains the camera id for the given camera config which is obtained from the
/// list of ARCore compatible camera configs. The acquired ID must be released
/// after use by the @c ::ArString_release function.
void ArCameraConfig_getCameraId(const ArSession *session,
                                const ArCameraConfig *camera_config,
                                char **out_camera_id);

/// @ingroup ArCameraConfig
/// Obtains the facing direction of the camera selected by this config.
void ArCameraConfig_getFacingDirection(
    const ArSession *session,
    const ArCameraConfig *camera_config,
    ArCameraConfigFacingDirection *out_facing);

// Camera config filters and camera config filters objects.

/// @ingroup ArCameraConfigFilter
/// Target camera capture frame rates.
/// The target frame rate represents the maximum or desired frame rate. Actual
/// camera capture frame rates can be lower than the target frame rate under low
/// light conditions in order to accommodate longer exposure times.
AR_DEFINE_ENUM(ArCameraConfigTargetFps){
    /// Target 30fps camera capture frame rate.
    ///
    /// Available on all ARCore supported devices.
    ///
    /// Used as a camera filter, via @c ::ArCameraConfigFilter_setTargetFps.
    AR_CAMERA_CONFIG_TARGET_FPS_30 = 0x0001,

    /// Target 60fps camera capture frame rate.
    ///
    /// Increases power consumption and may increase app memory usage.
    ///
    /// See the ARCore supported devices
    /// (https://developers.google.com/ar/devices)
    /// page for a list of devices that currently support 60fps.
    ///
    /// Used as a camera filter, via @c ::ArCameraConfigFilter_setTargetFps.
    AR_CAMERA_CONFIG_TARGET_FPS_60 = 0x0002,
};

/// @ingroup ArCameraConfigFilter
/// Depth sensor usage.
AR_DEFINE_ENUM(ArCameraConfigDepthSensorUsage){
    /// When used as a camera filter, via
    /// @c ::ArCameraConfigFilter_setDepthSensorUsage, filters for camera
    /// configs that require a depth sensor to be present on the device, and
    /// that will be used by ARCore.
    ///
    /// See the ARCore supported devices
    /// (https://developers.google.com/ar/devices)
    /// page for a list of devices that currently have supported depth sensors.
    ///
    /// When returned by @c ::ArCameraConfig_getDepthSensorUsage, indicates
    /// that a depth sensor is present, and that the camera config will use the
    /// available depth sensor.
    AR_CAMERA_CONFIG_DEPTH_SENSOR_USAGE_REQUIRE_AND_USE = 0x0001,

    /// When used as a camera filter, via
    /// @c ::ArCameraConfigFilter_setDepthSensorUsage, filters for camera
    /// configs
    /// where a depth sensor is not present, or is present but will not be used
    /// by ARCore.
    ///
    /// Most commonly used to filter camera configurations when the app requires
    /// exclusive access to the depth sensor outside of ARCore, for example to
    /// support 3D mesh reconstruction. Available on all ARCore supported
    /// devices.
    ///
    /// When returned by @c ::ArCameraConfig_getDepthSensorUsage, indicates that
    /// the camera config will not use a depth sensor, even if it is present.
    AR_CAMERA_CONFIG_DEPTH_SENSOR_USAGE_DO_NOT_USE = 0x0002,
};

/// @ingroup ArCameraConfigFilter
/// Stereo camera usage.
// TODO(b/166280987) Finalize documentation
AR_DEFINE_ENUM(ArCameraConfigStereoCameraUsage){
    /// When used as a camera filter, via
    /// @c ::ArCameraConfigFilter_setStereoCameraUsage, indicates that a stereo
    /// camera must be present on the device, and the stereo multi-camera
    /// (https://source.android.com/devices/camera/multi-camera) must be used by
    /// ARCore. Increases CPU and device power consumption. Not supported on all
    /// devices.
    ///
    /// See the ARCore supported devices
    /// (https://developers.google.com/ar/devices)
    /// page for a list of devices that currently have supported stereo camera
    /// capability.
    ///
    /// When returned by @c ::ArCameraConfig_getStereoCameraUsage, indicates
    /// that a
    /// stereo camera is present on the device and that the camera config will
    /// use the available stereo camera.
    AR_CAMERA_CONFIG_STEREO_CAMERA_USAGE_REQUIRE_AND_USE = 0x0001,

    /// When used as a camera filter, via
    /// @c ::ArCameraConfigFilter_setStereoCameraUsage, indicates that ARCore
    /// will
    /// not attempt to use a stereo multi-camera
    /// (https://source.android.com/devices/camera/multi-camera), even if one is
    /// present. Can be used to limit power consumption. Available on all ARCore
    /// supported devices.
    ///
    /// When returned by @c ::ArCameraConfig_getStereoCameraUsage, indicates
    /// that
    /// the camera config will not use a stereo camera, even if one is present
    /// on the device.
    AR_CAMERA_CONFIG_STEREO_CAMERA_USAGE_DO_NOT_USE = 0x0002,
};

/// @ingroup ArCameraConfig
/// Gets the stereo multi-camera
/// (https://source.android.com/devices/camera/multi-camera) usage settings. @p
/// out_stereo_camera_usage will contain one of the values from
/// @c ::ArCameraConfigStereoCameraUsage enum.
// TODO(b/166280987) Finalize documentation
void ArCameraConfig_getStereoCameraUsage(
    const ArSession *session,
    const ArCameraConfig *camera_config,
    ArCameraConfigStereoCameraUsage *out_stereo_camera_usage);

/// @ingroup ArCameraConfigFilter
/// Creates a camera config filter object.
///
/// @param[in]   session     The ARCore session
/// @param[out]  out_filter  A pointer to an @c ::ArCameraConfigFilter* to
///     receive the address of the newly allocated @c ::ArCameraConfigFilter
void ArCameraConfigFilter_create(const ArSession *session,
                                 ArCameraConfigFilter **out_filter);

/// @ingroup ArCameraConfigFilter
/// Releases memory used by the provided camera config filter object.
///
/// @param[in] filter The filter to release memory for.
void ArCameraConfigFilter_destroy(ArCameraConfigFilter *filter);

/// @ingroup ArCameraConfigFilter
/// Sets the desired framerates to allow.
///
/// @param[in] session     The ARCore session
/// @param[in, out] filter The filter object to change
/// @param[in] fps_filters A 32bit integer representing multiple
///     @c ::ArCameraConfigTargetFps values, bitwise-or'd together
void ArCameraConfigFilter_setTargetFps(const ArSession *session,
                                       ArCameraConfigFilter *filter,
                                       uint32_t fps_filters);

/// @ingroup ArCameraConfigFilter
/// Gets the desired framerates to allow.
///
/// @param[in]  session         The ARCore session
/// @param[in]  filter          The filter object to query
/// @param[out] out_fps_filters To be filled in with the desired framerates
///      allowed
void ArCameraConfigFilter_getTargetFps(const ArSession *session,
                                       ArCameraConfigFilter *filter,
                                       uint32_t *out_fps_filters);

/// @ingroup ArCameraConfigFilter
/// Sets the desired depth sensor usages to allow.
///
/// @param[in]      session                    The ARCore session
/// @param[in, out] filter                     The filter object to change
/// @param[in]      depth_sensor_usage_filters A 32bit integer representing
///     multiple @c ::ArCameraConfigDepthSensorUsage values, bitwise-or'd
///     together
void ArCameraConfigFilter_setDepthSensorUsage(
    const ArSession *session,
    ArCameraConfigFilter *filter,
    uint32_t depth_sensor_usage_filters);

/// @ingroup ArCameraConfigFilter
/// Gets the desired depth sensor usages to allow.
///
/// @param[in]  session                The ARCore session
/// @param[in]  filter                 The filter object to query
/// @param[out] out_depth_sensor_usage To be filled in with the desired depth
///     sensor usages allowed
void ArCameraConfigFilter_getDepthSensorUsage(const ArSession *session,
                                              ArCameraConfigFilter *filter,
                                              uint32_t *out_depth_sensor_usage);

/// @ingroup ArCameraConfigFilter
/// Sets the stereo multi-camera
/// (https://source.android.com/devices/camera/multi-camera) usage filter.
/// Default is to not filter.
///
/// @param[in]      session                      The ARCore session
/// @param[in, out] filter                       The filter object to change
/// @param[in]      stereo_camera_usage_filters  A 32bit integer representing
///     multiple @c ::ArCameraConfigStereoCameraUsage values, bitwise-or'd
///     together
// TODO(b/166280987) Finalize documentation
void ArCameraConfigFilter_setStereoCameraUsage(
    const ArSession *session,
    ArCameraConfigFilter *filter,
    uint32_t stereo_camera_usage_filters);

/// @ingroup ArCameraConfigFilter
/// Get the stereo multi-camera
/// (https://source.android.com/devices/camera/multi-camera) usage filter state.
///
/// @param[in]  session                  The ARCore session
/// @param[in]  filter                   The filter object to query
/// @param[out] out_stereo_camera_usage  To be filled in with the desired stereo
///     camera usages allowed
// TODO(b/166280987) Finalize documentation
void ArCameraConfigFilter_getStereoCameraUsage(
    const ArSession *session,
    ArCameraConfigFilter *filter,
    uint32_t *out_stereo_camera_usage);

/// @ingroup ArCameraConfigFilter
/// Sets the desired camera facing direction to allow.
///
/// @param[in] session     The ARCore session
/// @param[in, out] filter The filter object to change
/// @param[in] facing_direction_filter A valid
/// @c ::ArCameraConfigFacingDirection enum value
void ArCameraConfigFilter_setFacingDirection(
    const ArSession *session,
    ArCameraConfigFilter *filter,
    ArCameraConfigFacingDirection facing_direction_filter);

/// @ingroup ArCameraConfigFilter
/// Gets the desired camera facing direction to allow.
///
/// @param[in]  session         The ARCore session
/// @param[in]  filter          The filter object to query
/// @param[out] out_facing_direction_filter To be filled in with the desired
///     camera facing direction allowed
void ArCameraConfigFilter_getFacingDirection(
    const ArSession *session,
    ArCameraConfigFilter *filter,
    ArCameraConfigFacingDirection *out_facing_direction_filter);

/// @ingroup ArRecordingConfig
/// Creates a dataset recording config object.
///
/// @param[in]   session     The ARCore session
/// @param[out]  out_config  Pointer to an @c ::ArRecordingConfig* to receive
///     the address of the newly allocated @c ::ArRecordingConfig
void ArRecordingConfig_create(const ArSession *session,
                              ArRecordingConfig **out_config);

/// @ingroup ArRecordingConfig
/// Releases memory used by the provided recording config object.
///
/// @param[in] config The config to release memory for.
void ArRecordingConfig_destroy(ArRecordingConfig *config);

/// @ingroup ArRecordingConfig
/// Gets the file path to save an MP4 dataset file for the recording.
///
/// This will be null if @c ::ArRecordingConfig_setMp4DatasetUri was used to set
/// the output.
///
/// @param[in]  session                   The ARCore session
/// @param[in]  config                    The config object to query
/// @param[out] out_mp4_dataset_file_path Pointer to an @c char* to receive
///     the address of the newly allocated file path.
/// @deprecated Deprecated in release 1.26.0. Use
/// @c ::ArRecordingConfig_getMp4DatasetUri instead.
void ArRecordingConfig_getMp4DatasetFilePath(const ArSession *session,
                                             const ArRecordingConfig *config,
                                             char **out_mp4_dataset_file_path)
    AR_DEPRECATED(
        "Deprecated in release 1.26.0. Please see function documentation");

/// @ingroup ArRecordingConfig
/// Sets the file path to save an MP4 dataset file for the recording.
///
/// @param[in] session               The ARCore session
/// @param[in, out] config           The config object to change
/// @param[in] mp4_dataset_file_path A string representing the file path
/// @deprecated Deprecated in release 1.26.0. Use
/// @c ::ArRecordingConfig_setMp4DatasetUri instead.
void ArRecordingConfig_setMp4DatasetFilePath(const ArSession *session,
                                             ArRecordingConfig *config,
                                             const char *mp4_dataset_file_path)
    AR_DEPRECATED(
        "Deprecated in release 1.26.0. Please see function documentation");

/// @ingroup ArRecordingConfig
/// Gets the URI that the MP4 dataset will be recorded to.
///
/// This will be null if @c ::ArRecordingConfig_setMp4DatasetFilePath was used
/// to set the output.
///
/// @param[in]  session             The ARCore session
/// @param[in]  config              The config object to query
/// @param[out] out_mp4_dataset_uri Pointer to a @c char* to receive
///     the address of the newly allocated URI.
void ArRecordingConfig_getMp4DatasetUri(const ArSession *session,
                                        const ArRecordingConfig *config,
                                        char **out_mp4_dataset_uri);

/// @ingroup ArRecordingConfig
/// Sets the file path to save an MP4 dataset file for the recording.
///
/// The URI must be able to be opened as a file descriptor for reading and
/// writing that supports @c lseek or @c #AR_ERROR_INVALID_ARGUMENT will be
/// returned when @c ::ArSession_startRecording is called.
///
/// @param[in] session         The ARCore session
/// @param[in, out] config     The config object to change
/// @param[in] mp4_dataset_uri The percent encoded, null terminated, URI to
///     write data to.
void ArRecordingConfig_setMp4DatasetUri(const ArSession *session,
                                        ArRecordingConfig *config,
                                        const char *mp4_dataset_uri);

/// @ingroup ArRecordingConfig
/// Gets the setting that indicates whether the recording should stop
/// automatically when the ARCore session is paused.
///
/// @param[in]  session            The ARCore session
/// @param[in]  config             The config object to query
/// @param[out] out_config_enabled To be filled in with the state used (1 for
///     enabled, 0 for disabled)
void ArRecordingConfig_getAutoStopOnPause(const ArSession *session,
                                          const ArRecordingConfig *config,
                                          int32_t *out_config_enabled);

/// @ingroup ArRecordingConfig
/// Specifies whether recording should stop automatically when the ARCore
/// session is paused.
///
/// @param[in] session        The ARCore session
/// @param[in, out] config    The config object to change
/// @param[in] config_enabled Desired state (1 to enable, 0 to disable)
void ArRecordingConfig_setAutoStopOnPause(const ArSession *session,
                                          ArRecordingConfig *config,
                                          int32_t config_enabled);

/// @ingroup ArRecordingConfig
/// Gets the clockwise rotation in degrees that should be applied to the
/// recorded image.
///
/// @param[in]  session                The ARCore session
/// @param[in]  config                 The config object to query
/// @param[out] out_recording_rotation To be filled in with the clockwise
///     rotation in degrees (0, 90, 180, 270, or -1 for unspecified)
void ArRecordingConfig_getRecordingRotation(const ArSession *session,
                                            const ArRecordingConfig *config,
                                            int32_t *out_recording_rotation);

/// @ingroup ArRecordingConfig
/// Specifies the clockwise rotation in degrees that should be applied to the
/// recorded image.
///
/// @param[in] session            The ARCore session
/// @param[in, out] config        The config object to change
/// @param[in] recording_rotation The clockwise rotation in degrees (0, 90, 180,
///     or 270).
void ArRecordingConfig_setRecordingRotation(const ArSession *session,
                                            ArRecordingConfig *config,
                                            int32_t recording_rotation);

/// @ingroup ArRecordingConfig
/// Configures and adds a track.
///
/// @param[in] session               The ARCore session
/// @param[in, out] config           The @c ::ArRecordingConfig object to change
/// @param[in] track                 The @c ::ArTrack being added to the
/// recording config
void ArRecordingConfig_addTrack(const ArSession *session,
                                ArRecordingConfig *config,
                                const ArTrack *track);

/// @ingroup ArTrack
/// Initializes an @c ::ArTrack.
///
/// @param[in] session                   The ARCore session
/// @param[out] out_track                Pointer to an @c ::ArTrack to receive
///     the address of the newly allocated @c ::ArTrack
void ArTrack_create(const ArSession *session, ArTrack **out_track);

/// @ingroup ArTrack
/// Sets the  Track Id.
///
/// @param[in] session               The ARCore session
/// @param[in, out] track            The track object to change
/// @param[in] track_id_uuid_16      The track ID as UUID as a byte array of 16
/// bytes in size
void ArTrack_setId(const ArSession *session,
                   ArTrack *track,
                   const uint8_t *track_id_uuid_16);

/// @ingroup ArTrack
/// Sets the Track Metadata.
///
/// @param[in] session                    The ARCore session
/// @param[in, out] track                 The external track object to change
/// @param[in] track_metadata_buffer      bytes describing arbitrary data about
///     the track.
/// @param[in] track_metadata_buffer_size size of the @p track_metadata_buffer
void ArTrack_setMetadata(const ArSession *session,
                         ArTrack *track,
                         const uint8_t *track_metadata_buffer,
                         size_t track_metadata_buffer_size);

/// @ingroup ArTrack
/// Sets the MIME type for the Data Track. Default value is
/// "application/text".
///
/// @param[in] session               The ARCore session
/// @param[in, out] track            The track object to change
/// @param[in] mime_type             The string representing the MIME type label
/// as a null-terminated string in UTF-8 format. This will be the MIME type set
/// on the additional stream created for this @c ::ArTrack, another convenient
/// means of identification, particularly for applications that do not support
/// UUID as a means of identification.
void ArTrack_setMimeType(const ArSession *session,
                         ArTrack *track,
                         const char *mime_type);

/// @ingroup ArTrack
/// Releases memory used by the provided data track config object.
///
/// @param[in] track The track config object to release
void ArTrack_destroy(ArTrack *track);

// === ArSession functions ===

/// @ingroup ArSession
/// Releases resources used by an ARCore session.
/// This function will take several seconds to complete. To prevent blocking
/// the main thread, call @c ::ArSession_pause on the main thread, and then call
/// @c ::ArSession_destroy on a background thread.
///
void ArSession_destroy(ArSession *session);

/// @ingroup ArSession
/// Before release 1.2.0: Checks if the provided configuration is usable on the
/// this device. If this function returns @c
/// #AR_ERROR_UNSUPPORTED_CONFIGURATION, calls to @c ::ArSession_configure with
/// this configuration will fail.
///
/// This function now always returns true. See documentation for each
/// configuration entry to know which configuration options & combinations are
/// supported.
///
/// @param[in] session The ARCore session
/// @param[in] config  The configuration to test
/// @return @c #AR_SUCCESS or:
///  - @c #AR_ERROR_INVALID_ARGUMENT if any of the arguments are @c NULL.
/// @deprecated Deprecated in release 1.2.0. Please refer to the release notes
/// (<a
/// href="https://github.com/google-ar/arcore-android-sdk/releases/tag/v1.2.0">release
/// notes 1.2.0</a>)
///
ArStatus ArSession_checkSupported(const ArSession *session,
                                  const ArConfig *config)
    AR_DEPRECATED(
        "Deprecated in release 1.2.0. Please see function documentation");

/// @ingroup ArSession
/// Configures the session and verifies that the enabled features in the
/// specified session config are supported with the currently set camera config.
///
/// Should be called after @c ::ArSession_setCameraConfig to verify that all
/// requested session config features are supported. Features not supported with
/// the current camera config will otherwise be silently disabled when the
/// session is resumed by calling @c ::ArSession_resume.
///
/// The following configurations are unsupported and will return
/// @c #AR_ERROR_UNSUPPORTED_CONFIGURATION:
///
/// - When using the (default) back-facing camera:
///   - @c #AR_AUGMENTED_FACE_MODE_MESH3D.
///   - @c #AR_GEOSPATIAL_MODE_ENABLED on devices that do not support this
///   Geospatial mode. See @c ::ArSession_isGeospatialModeSupported.
/// - When using the front-facing (selfie) camera:
///   - Any config using @c ::ArConfig_setAugmentedImageDatabase.
///   - @c #AR_CLOUD_ANCHOR_MODE_ENABLED.
///   - @c #AR_LIGHT_ESTIMATION_MODE_ENVIRONMENTAL_HDR.
///   - @c #AR_GEOSPATIAL_MODE_ENABLED.
/// - When on an Android device with API level 26 or below:
///   - @c #AR_TEXTURE_UPDATE_MODE_EXPOSE_HARDWARE_BUFFER.
///
/// @param[in] session The ARCore session.
/// @param[in] config The new configuration setting for the session.
///
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_FATAL
/// - @c #AR_ERROR_UNSUPPORTED_CONFIGURATION if the requested session config is
///   not supported.
///   See above restrictions.
/// - @c #AR_ERROR_INTERNET_PERMISSION_NOT_GRANTED if
///   @c #AR_GEOSPATIAL_MODE_ENABLED or
///   @c #AR_CLOUD_ANCHOR_MODE_ENABLED is set and the
///   Android <a
///   href="https://developer.android.com/training/basics/network-ops/connecting">
///   INTERNET</a> permission has not been granted.
/// - @c #AR_ERROR_FINE_LOCATION_PERMISSION_NOT_GRANTED if
///   @c #AR_GEOSPATIAL_MODE_ENABLED is set and the @c ACCESS_FINE_LOCATION
///   permission has not been granted.
/// - @c #AR_ERROR_GOOGLE_PLAY_SERVICES_LOCATION_LIBRARY_NOT_LINKED if
///   @c #AR_GEOSPATIAL_MODE_ENABLED is set and the <a
///   href="https://developers.google.com/android/guides/setup#declare-dependencies">Fused
///   Location Provider for Android library</a> could not be found. See @c
///   #AR_ERROR_GOOGLE_PLAY_SERVICES_LOCATION_LIBRARY_NOT_LINKED for additional
///   troubleshooting steps.
ArStatus ArSession_configure(ArSession *session, const ArConfig *config);

/// @ingroup ArSession
/// Gets the current config. More specifically, fills the given @c ::ArConfig
/// object with the copy of the configuration most recently set by
/// @c ::ArSession_configure. Note: if the session was not explicitly
/// configured, a default configuration is returned (same as
/// @c ::ArConfig_create).
void ArSession_getConfig(ArSession *session, ArConfig *out_config);

/// @ingroup ArSession
/// Starts or resumes the ARCore Session.
///
/// Typically this should be called from <a
/// href="https://developer.android.com/reference/android/app/Activity.html#onResume()"
/// >@c Activity.onResume() </a>.
///
/// Note that if the camera configuration has been changed by
/// @c ::ArSession_setCameraConfig since the last call to @c ::ArSession_resume,
/// all images previously acquired using @c ::ArFrame_acquireCameraImage must be
/// released by calling @c ::ArImage_release before calling
/// @c ::ArSession_resume.  If there are open images, @c ::ArSession_resume will
/// return @c #AR_ERROR_ILLEGAL_STATE and the session will not resume.
///
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_FATAL
/// - @c #AR_ERROR_CAMERA_PERMISSION_NOT_GRANTED
/// - @c #AR_ERROR_CAMERA_NOT_AVAILABLE
/// - @c #AR_ERROR_ILLEGAL_STATE
ArStatus ArSession_resume(ArSession *session);

/// @ingroup ArSession
/// Pause the current session. This function will stop the camera feed and
/// release resources. The session can be restarted again by calling
/// @c ::ArSession_resume.
///
/// Typically this should be called from <a
/// href="https://developer.android.com/reference/android/app/Activity.html#onPause()"
/// >@c Activity.onPause() </a>.
///
/// Note that ARCore might continue consuming substantial computing resources
/// for up to 10 seconds after calling this function.
///
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_FATAL
ArStatus ArSession_pause(ArSession *session);

/// @ingroup ArSession
/// Sets the OpenGL texture names (IDs) that will be assigned to incoming camera
/// frames in sequence in a ring buffer. The textures must be bound to the
/// @c GL_TEXTURE_EXTERNAL_OES target for use. Shaders accessing these textures
/// must use a @c samplerExternalOES sampler.
///
/// The texture contents are not guaranteed to remain valid after another call
/// to @c ::ArSession_setCameraTextureName or
/// @c ::ArSession_setCameraTextureNames, and additionally are not guaranteed to
/// remain valid after a call to
/// @c ::ArSession_pause or @c ::ArSession_destroy.
///
/// Passing multiple textures allows for a multithreaded rendering pipeline,
/// unlike @c ::ArSession_setCameraTextureName.
///
/// Note: this function doesn't fail. If given invalid input, it logs an error
/// without setting the texture names.
///
/// When a configuration is active using @c
/// #AR_TEXTURE_UPDATE_MODE_EXPOSE_HARDWARE_BUFFER, values provided to this
/// function are ignored.
///
/// @param[in] session The ARCore session
/// @param[in] number_of_textures The number of textures being passed. This
///     must always be at least 1.
/// @param[in] texture_ids Pointer to the array of textures names (IDs)
void ArSession_setCameraTextureNames(ArSession *session,
                                     int32_t number_of_textures,
                                     const uint32_t *texture_ids);

/// @ingroup ArSession
/// Sets the OpenGL texture name (ID) that will allow GPU access to the camera
/// image. The texture must be bound to the @c GL_TEXTURE_EXTERNAL_OES target
/// for use. Shaders accessing this texture must use a @c samplerExternalOES
/// sampler.
///
/// The texture contents are not guaranteed to remain valid after another call
/// to @c ::ArSession_setCameraTextureName or
/// @c ::ArSession_setCameraTextureNames, and additionally are not guaranteed to
/// remain valid after a call to @c ::ArSession_pause or @c ::ArSession_destroy.
///
/// When a configuration is active using @c
/// #AR_TEXTURE_UPDATE_MODE_EXPOSE_HARDWARE_BUFFER, values provided to this
/// function are ignored.
void ArSession_setCameraTextureName(ArSession *session, uint32_t texture_id);

/// @ingroup ArSession
/// Sets the aspect ratio, coordinate scaling, and display rotation. This data
/// is used by UV conversion, projection matrix generation, and hit test logic.
///
/// Note: this function always returns successfully. If given invalid input, it
/// logs a error and doesn't apply the changes.
///
/// @param[in] session   The ARCore session
/// @param[in] rotation  Display rotation specified by @c android.view.Surface
///     constants: @c ROTATION_0, @c ROTATION_90, @c ROTATION_180 and
///     @c ROTATION_270
/// @param[in] width     Width of the view, in pixels
/// @param[in] height    Height of the view, in pixels
void ArSession_setDisplayGeometry(ArSession *session,
                                  int32_t rotation,
                                  int32_t width,
                                  int32_t height);

/// @ingroup ArSession
/// Updates the state of the ARCore system. This includes: receiving a new
/// camera frame, updating the location of the device, updating the location of
/// tracking anchors, updating detected planes, etc.
///
/// This call may cause off-screen OpenGL activity. Because of this, to avoid
/// unnecessary frame buffer flushes and reloads, this call should not be made
/// in the middle of rendering a frame or offscreen buffer.
///
/// This call may update the pose of all created anchors and detected planes.
/// The set of updated objects is accessible through
/// @c ::ArFrame_getUpdatedTrackables.
///
/// @c ::ArSession_update in blocking mode (see @c ::ArUpdateMode) will wait
/// until a new camera image is available, or until the built-in timeout
/// (currently 66ms) is reached.
/// If the camera image does not arrive by the built-in timeout, then
/// @c ::ArSession_update will return the most recent @c ::ArFrame object. For
/// some applications it may be important to know if a new frame was actually
/// obtained (for example, to avoid redrawing if the camera did not produce a
/// new frame). To do that, compare the current frame's timestamp, obtained via
/// @c ::ArFrame_getTimestamp, with the previously recorded frame timestamp. If
/// they are different, this is a new frame.
///
/// During startup the camera system may not produce actual images
/// immediately. In this common case, a frame with timestamp = 0 will be
/// returned.
///
/// @param[in]    session   The ARCore session
/// @param[inout] out_frame The Frame object to populate with the updated world
///     state.  This frame must have been previously created using
///     @c ::ArFrame_create.  The same @c ::ArFrame instance may be used when
///     calling this repeatedly.
///
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_FATAL
/// - @c #AR_ERROR_SESSION_PAUSED
/// - @c #AR_ERROR_TEXTURE_NOT_SET
/// - @c #AR_ERROR_MISSING_GL_CONTEXT
/// - @c #AR_ERROR_CAMERA_NOT_AVAILABLE - camera was removed during runtime.
ArStatus ArSession_update(ArSession *session, ArFrame *out_frame);

/// @ingroup ArSession
/// Defines a tracked location in the physical world.
///
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_NOT_TRACKING
/// - @c #AR_ERROR_SESSION_PAUSED
/// - @c #AR_ERROR_RESOURCE_EXHAUSTED
ArStatus ArSession_acquireNewAnchor(ArSession *session,
                                    const ArPose *pose,
                                    ArAnchor **out_anchor);

/// @ingroup ArSession
/// Returns all known anchors, including those not currently tracked. Anchors
/// forgotten by ARCore due to a call to @c ::ArAnchor_detach or entering the
/// @c #AR_TRACKING_STATE_STOPPED state will not be included.
///
/// @param[in]    session         The ARCore session
/// @param[inout] out_anchor_list The list to fill.  This list must have already
///     been allocated with @c ::ArAnchorList_create.  If previously used, the
///     list will first be cleared.
void ArSession_getAllAnchors(const ArSession *session,
                             ArAnchorList *out_anchor_list);

/// @ingroup ArSession
/// Returns the list of all known @c ::ArTrackable objects.  This includes
/// @c ::ArPlane objects if plane detection is enabled, as well as @c ::ArPoint
/// objects created as a side effect of calls to @c ::ArSession_acquireNewAnchor
/// or
/// @c ::ArFrame_hitTest.
///
/// @param[in]    session            The ARCore session
/// @param[in]    filter_type        The type(s) of trackables to return.  See
///     @c ::ArTrackableType for legal values.
/// @param[inout] out_trackable_list The list to fill.  This list must have
///     already been allocated with @c ::ArTrackableList_create.  If previously
///     used, the list will first be cleared.
void ArSession_getAllTrackables(const ArSession *session,
                                ArTrackableType filter_type,
                                ArTrackableList *out_trackable_list);

/// @ingroup ArAnchor
/// Describes the quality of the visual features seen by ARCore in the preceding
/// few seconds and visible from a desired camera @c ::ArPose. A higher quality
/// indicates a Cloud Anchor hosted at the current time with the current set of
/// recently seen features will generally be easier to resolve more accurately.
/// See the <a
/// href="https://developers.google.com/ar/develop/c/cloud-anchors/developer-guide">Cloud
/// Anchors developer guide</a> for more information.
AR_DEFINE_ENUM(ArFeatureMapQuality){
    /// The quality of features seen from the pose in the preceding
    /// seconds is low. This state indicates that ARCore will likely have more
    /// difficulty resolving the Cloud Anchor. Encourage the user to move the
    /// device, so that the desired position of the Cloud Anchor to be hosted is
    /// seen from different angles.
    AR_FEATURE_MAP_QUALITY_INSUFFICIENT = 0,
    /// The quality of features seen from the pose in the preceding few
    /// seconds is likely sufficient for ARCore to successfully resolve a Cloud
    /// Anchor, although the accuracy of the resolved pose will likely be
    /// reduced. Encourage the user to move the device, so that the desired
    /// position of the Cloud Anchor to be hosted is seen from different angles.
    AR_FEATURE_MAP_QUALITY_SUFFICIENT = 1,
    /// The quality of features seen from the pose in the preceding few
    /// seconds is likely sufficient for ARCore to successfully resolve a Cloud
    /// Anchor with a high degree of accuracy.
    AR_FEATURE_MAP_QUALITY_GOOD = 2,
};

/// @ingroup ArSession
/// Estimates the quality of the visual features seen by ARCore in the
/// preceding few seconds and visible from the provided camera pose.
///
/// Cloud Anchors hosted using higher quality features will generally result
/// in easier and more accurately resolved Cloud Anchor poses.
///
/// @param[in] session The ARCore session.
/// @param[in] pose The camera pose to use in estimating the quality.
/// @param[out] out_feature_map_quality The estimated quality of the visual
///     features seen by ARCore in the preceding few seconds and visible from
///     the provided camera pose.
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_INVALID_ARGUMENT
/// - @c #AR_ERROR_NOT_TRACKING
/// - @c #AR_ERROR_SESSION_PAUSED
/// - @c #AR_ERROR_CLOUD_ANCHORS_NOT_CONFIGURED
ArStatus ArSession_estimateFeatureMapQualityForHosting(
    const ArSession *session,
    const ArPose *pose,
    ArFeatureMapQuality *out_feature_map_quality);

/// @ingroup ArSession
/// This creates a new Cloud Anchor using the pose and other metadata from
/// @p anchor.
///
/// If the function returns @c #AR_SUCCESS, the cloud state of @p
/// out_cloud_anchor will be set to @c #AR_CLOUD_ANCHOR_STATE_TASK_IN_PROGRESS
/// and the initial pose will be set to the pose of @p anchor. However, the new
/// @p out_cloud_anchor is completely independent of @p anchor, and the poses
/// may diverge over time. If the return value of this function is not @c
/// #AR_SUCCESS, then @p out_cloud_anchor will be set to @c NULL.
///
/// @param[in]    session          The ARCore session
/// @param[in]    anchor           The anchor to be hosted
/// @param[inout] out_cloud_anchor The new Cloud Anchor
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_NOT_TRACKING
/// - @c #AR_ERROR_SESSION_PAUSED
/// - @c #AR_ERROR_CLOUD_ANCHORS_NOT_CONFIGURED
/// - @c #AR_ERROR_RESOURCE_EXHAUSTED
/// - @c #AR_ERROR_ANCHOR_NOT_SUPPORTED_FOR_HOSTING
/// @deprecated Use @c ::ArSession_hostCloudAnchorAsync with @c ttl_days = 1
/// instead.
ArStatus ArSession_hostAndAcquireNewCloudAnchor(ArSession *session,
                                                const ArAnchor *anchor,
                                                ArAnchor **out_cloud_anchor)
    AR_DEPRECATED(
        "Use ArSession_hostCloudAnchorAsync with ttl_days = 1 instead.");

/// @ingroup ArSession
/// This creates a new Cloud Anchor and schedules a task to resolve the anchor's
/// pose using the given Cloud Anchor ID. You don't need to wait for a call to
/// resolve a Cloud Anchor to complete before initiating another call.
/// A session can be resolving up to 40 Cloud Anchors at a given time.
///
/// If this function returns @c #AR_SUCCESS, the cloud state of @p
/// out_cloud_anchor will be @c #AR_CLOUD_ANCHOR_STATE_TASK_IN_PROGRESS, and its
/// tracking state will be @c #AR_TRACKING_STATE_PAUSED. This anchor will never
/// start tracking until its pose has been successfully resolved. If the
/// resolving task ends in an error, the tracking state will be set to @c
/// #AR_TRACKING_STATE_STOPPED. If the return value is not @c #AR_SUCCESS, then
/// @p out_cloud_anchor will be set to
/// @c NULL.
///
/// @param[in]    session          The ARCore session
/// @param[in]    cloud_anchor_id  The cloud ID of the anchor to be resolved
/// @param[inout] out_cloud_anchor The new Cloud Anchor
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_NOT_TRACKING
/// - @c #AR_ERROR_SESSION_PAUSED
/// - @c #AR_ERROR_CLOUD_ANCHORS_NOT_CONFIGURED
/// - @c #AR_ERROR_RESOURCE_EXHAUSTED
/// @deprecated Use @c ::ArSession_resolveCloudAnchorAsync instead.
ArStatus ArSession_resolveAndAcquireNewCloudAnchor(ArSession *session,
                                                   const char *cloud_anchor_id,
                                                   ArAnchor **out_cloud_anchor)
    AR_DEPRECATED("Use @c ::ArSession_resolveCloudAnchorAsync instead.");

/// @ingroup ArSession
/// This creates a new Cloud Anchor with a given lifetime in days, using the
/// pose of the provided @p anchor.
///
/// The cloud state of the returned anchor will be set to
/// @c #AR_CLOUD_ANCHOR_STATE_TASK_IN_PROGRESS and the initial pose
/// will be set to the pose of the provided @p anchor. However, the returned
/// anchor is completely independent of the original @p anchor, and the two
/// poses might diverge over time.
///
/// Hosting requires an active session for which the @c ::ArTrackingState
/// is @c #AR_TRACKING_STATE_TRACKING, as well as a working internet connection.
/// ARCore will continue to retry silently in the background if it is unable to
/// establish a connection to the ARCore Cloud Anchor service.
///
/// @param[in] session  The ARCore session.
/// @param[in] anchor   The anchor with the desired pose to be used to create a
///     hosted Cloud Anchor.
/// @param[in] ttl_days The lifetime of the anchor in days. Must be positive.
///     The maximum allowed value is 1 if using an API Key to authenticate with
///     the ARCore Cloud Anchor service, otherwise the maximum allowed value is
///     365.
/// @param[inout] out_cloud_anchor The new Cloud Anchor.
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_INVALID_ARGUMENT
/// - @c #AR_ERROR_NOT_TRACKING
/// - @c #AR_ERROR_SESSION_PAUSED
/// - @c #AR_ERROR_CLOUD_ANCHORS_NOT_CONFIGURED
/// - @c #AR_ERROR_RESOURCE_EXHAUSTED
/// - @c #AR_ERROR_ANCHOR_NOT_SUPPORTED_FOR_HOSTING
/// @deprecated Use @c ::ArSession_hostCloudAnchorAsync with @c ttl_days = 1
/// instead.
ArStatus ArSession_hostAndAcquireNewCloudAnchorWithTtl(
    ArSession *session,
    const ArAnchor *anchor,
    int32_t ttl_days,
    ArAnchor **out_cloud_anchor)
    AR_DEPRECATED(
        "Use ArSession_hostCloudAnchorAsync with ttl_days = 1 instead.");

/// @ingroup ArHostCloudAnchorFuture
/// Gets the Cloud Anchor ID of the hosted anchor. If the operation isn't done
/// yet or the operation failed, this will be @c NULL. The caller must release
/// the string using @c ::ArString_release.
///
/// @param[in] session    The ARCore session.
/// @param[in] future     The handle for the asynchronous operation.
/// @param[out] out_cloud_anchor_id The Cloud Anchor ID.
void ArHostCloudAnchorFuture_acquireResultCloudAnchorId(
    const ArSession *session,
    const ArHostCloudAnchorFuture *future,
    char **out_cloud_anchor_id);

/// @ingroup ArHostCloudAnchorFuture
/// Gets the result status of the hosting operation, if the operation is done.
///
/// @param[in] session    The ARCore session.
/// @param[in] future     The handle for the asynchronous operation.
/// @param[out] out_cloud_anchor_state The result status.
void ArHostCloudAnchorFuture_getResultCloudAnchorState(
    const ArSession *session,
    const ArHostCloudAnchorFuture *future,
    ArCloudAnchorState *out_cloud_anchor_state);

/// @ingroup ArHostCloudAnchorFuture
/// Callback definition for @c ::ArSession_hostCloudAnchorAsync. The
/// @p context argument will be the same as that passed to
/// @c ::ArSession_hostCloudAnchorAsync. The @p cloud_anchor_id argument
/// will contain the same value as that returned by
/// @c ::ArHostCloudAnchorFuture_acquireResultCloudAnchorId and must be released
/// using @c ::ArString_release. The @p cloud_anchor_state argument will be the
/// same as that returned by
/// @c ::ArHostCloudAnchorFuture_getResultCloudAnchorState.
///
/// It is a best practice to free @c context memory provided to
/// @c ::ArSession_hostCloudAnchorAsync at the end of the callback
/// implementation.
typedef void (*ArHostCloudAnchorCallback)(
    void *context,
    char *cloud_anchor_id,
    ArCloudAnchorState cloud_anchor_state);

/// @ingroup ArSession
/// Uses the pose and other data from @p anchor to host a new Cloud Anchor.
/// A Cloud Anchor is assigned an identifier that can be used to create an
/// @c ::ArAnchor in the same position in subsequent sessions across devices
/// using @c ::ArSession_resolveCloudAnchorAsync. See the <a
/// href="https://developers.google.com/ar/develop/c/cloud-anchors/developer-guide">Cloud
/// Anchors developer guide</a> for more information.
///
/// The duration that a Cloud Anchor can be resolved for is specified by
/// @p ttl_days. When using <a
/// href="https://developers.google.com/ar/develop/c/cloud-anchors/developer-guide#keyless-authorization">Keyless
/// authorization</a>, the maximum allowed value is 365 days. When using an <a
/// href="https://developers.google.com/ar/develop/c/cloud-anchors/developer-guide#api-key-authorization">API
/// Key</a> to authenticate with the ARCore API, the maximum allowed value is 1
/// day.
///
/// This launches an asynchronous operation used to query the Google Cloud
/// ARCore API. See @c ::ArFuture for information on obtaining results and
/// cancelling the operation.
///
/// Cloud Anchors requires a @c ::ArConfig with @c
/// #AR_CLOUD_ANCHOR_MODE_ENABLED set on this session. Use @c
/// ::ArConfig_setCloudAnchorMode to set the Cloud Anchor API mode and
/// @c ::ArSession_configure to configure the session.
///
/// Hosting a Cloud Anchor works best when ARCore is able to create a good
/// feature map around the @c ::ArAnchor. Use @c
/// ::ArSession_estimateFeatureMapQualityForHosting to determine the quality of
/// visual features seen by ARCore in the preceding few seconds. Cloud Anchors
/// hosted using higher quality features will generally result in quicker and
/// more accurately resolved Cloud Anchor poses.
///
/// ARCore can have up to 40 simultaneous Cloud Anchor operations, including
/// resolved anchors and active hosting operations.
///
/// @param[in] session  The ARCore session.
/// @param[in] anchor   The anchor with the desired pose to be used to host a
///     Cloud Anchor.
/// @param[in] ttl_days The lifetime of the anchor in days. Must be positive.
/// @param[in] context An optional void pointer passed when using a @ref
///   future_callback "callback".
/// @param[in] callback A @ref future_callback "callback function".
/// @param[out] out_future An optional @c ::ArFuture which can be @ref
///   future_polling "polled".
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_INVALID_ARGUMENT if any of the provided arguments are
/// invalid,
/// - @c #AR_ERROR_NOT_TRACKING if the camera is not currently tracking,
/// - @c #AR_ERROR_SESSION_PAUSED if the session is currently paused,
/// - @c #AR_ERROR_CLOUD_ANCHORS_NOT_CONFIGURED if the Cloud Anchor API has not
/// been enabled in the session configuration,
/// - @c #AR_ERROR_RESOURCE_EXHAUSTED if too many Cloud Anchors operations are
/// ongoing.
/// - @c #AR_ERROR_ANCHOR_NOT_SUPPORTED_FOR_HOSTING if the anchor is not
/// supported for hosting.
ArStatus ArSession_hostCloudAnchorAsync(ArSession *session,
                                        const ArAnchor *anchor,
                                        int32_t ttl_days,
                                        void *context,
                                        ArHostCloudAnchorCallback callback,
                                        ArHostCloudAnchorFuture **out_future);

/// @ingroup ArResolveCloudAnchorFuture
/// Gets the resolved Cloud Anchor. If the operation isn't done yet or the
/// operation failed, this will be @c NULL. The caller must release the anchor
/// using @c ::ArAnchor_release.
///
/// @param[in] session    The ARCore session.
/// @param[in] future     The handle for the asynchronous operation.
/// @param[out] out_anchor The anchor.
void ArResolveCloudAnchorFuture_acquireResultAnchor(
    const ArSession *session,
    const ArResolveCloudAnchorFuture *future,
    ArAnchor **out_anchor);

/// @ingroup ArResolveCloudAnchorFuture
/// Gets the result status of the resolving operation, if the operation is done.
///
/// @param[in] session    The ARCore session.
/// @param[in] future     The handle for the asynchronous operation.
/// @param[out] out_cloud_anchor_state The result status.
void ArResolveCloudAnchorFuture_getResultCloudAnchorState(
    const ArSession *session,
    const ArResolveCloudAnchorFuture *future,
    ArCloudAnchorState *out_cloud_anchor_state);

/// @ingroup ArResolveCloudAnchorFuture
/// Callback definition for @c ::ArSession_resolveCloudAnchorAsync. The
/// @p context argument will be the same as that passed to
/// @c ::ArSession_resolveCloudAnchorAsync. The @p anchor argument will be the
/// same as that returned by
/// @c ::ArResolveCloudAnchorFuture_acquireResultAnchor and must be released
/// using @c ::ArAnchor_release. The @p cloud_anchor_state argument will be the
/// same as that returned by
/// @c ::ArResolveCloudAnchorFuture_getResultCloudAnchorState.
///
/// It is a best practice to free @c context memory provided to
/// @c ::ArSession_resolveCloudAnchorAsync at the end of the callback
/// implementation.
typedef void (*ArResolveCloudAnchorCallback)(
    void *context, ArAnchor *anchor, ArCloudAnchorState cloud_anchor_state);

/// @ingroup ArSession
/// Attempts to resolve a Cloud Anchor using the provided @p cloud_anchor_id.
/// The Cloud Anchor must previously have been hosted by @c
/// ::ArSession_hostCloudAnchorAsync or another Cloud Anchor hosting method
/// within the allotted @c ttl_days. See the <a
/// href="https://developers.google.com/ar/develop/c/cloud-anchors/developer-guide">Cloud
/// Anchors developer guide</a> for more information.
///
/// This launches an asynchronous operation used to query the Google Cloud
/// ARCore API. See @c ::ArFuture for information on obtaining results and
/// cancelling the operation.
///
/// When resolving a Cloud Anchor, the ARCore API periodically compares visual
/// features from the scene against the anchor's 3D feature map to pinpoint the
/// user's position and orientation relative to the anchor. When it finds a
/// match, the task completes.
///
/// Cloud Anchors requires a @c ::ArConfig with @c
/// #AR_CLOUD_ANCHOR_MODE_ENABLED set on this session. Use @c
/// ::ArConfig_setCloudAnchorMode to set the Cloud Anchor API mode and
/// @c ::ArSession_configure to configure the session.
///
/// ARCore can have up to 40 simultaneous Cloud Anchor operations, including
/// resolved anchors and active hosting operations.
///
/// @param[in] session          The ARCore session
/// @param[in] cloud_anchor_id  The cloud ID of the anchor to be resolved.
/// @param[in] context An optional void pointer passed when using a @ref
///   future_callback "callback".
/// @param[in] callback A @ref future_callback "callback function".
/// @param[out] out_future An optional @c ::ArFuture which can be @ref
///   future_polling "polled".
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_INVALID_ARGUMENT
/// - @c #AR_ERROR_SESSION_PAUSED
/// - @c #AR_ERROR_CLOUD_ANCHORS_NOT_CONFIGURED
/// - @c #AR_ERROR_RESOURCE_EXHAUSTED if too many Cloud Anchors operations are
/// ongoing.
ArStatus ArSession_resolveCloudAnchorAsync(
    ArSession *session,
    const char *cloud_anchor_id,
    void *context,
    ArResolveCloudAnchorCallback callback,
    ArResolveCloudAnchorFuture **out_future);

/// @ingroup ArSession
/// Gets a list of camera configs supported by the camera being used by the
/// session.
///
/// Can be called at any time. The provided list populated with the camera
/// configs supported by the configured session and camera.
///
/// Each config will contain a different CPU resolution. The GPU texture
/// resolutions will be the same in all configs. Most devices provide a GPU
/// texture resolution of 1920x1080, but the actual resolution will vary with
/// device capabilities.
///
/// When the session camera is a back-facing camera:
/// - The list will always contain three camera configs.
/// - The CPU image resolutions returned will be VGA, a middle resolution, and a
///   large resolution matching the GPU texture resolution. The middle
///   resolution is typically 1280x720, but the actual resolution will vary
///   with device capabilities.
///
/// When the session camera is front-facing (selfie) camera, the list will
/// contain at least one supported camera config.
///
/// Notes:
/// - Prior to ARCore SDK 1.6.0, the middle CPU image resolution was guaranteed
///   to be 1280x720 on all devices.
/// - In ARCore SDK 1.7.0 and 1.8.0, when the session camera was a front-facing
///   (selfie) camera, the list contained three identical camera configs.
///
/// @param[in]    session          The ARCore session
/// @param[inout] list             The list to fill. This list must have already
///      been allocated with @c ::ArCameraConfigList_create.  The list is
///      cleared to remove any existing elements.  Once it is no longer needed,
///      the list must be destroyed using @c ::ArCameraConfigList_destroy to
///      release allocated memory.
/// @deprecated Deprecated in release 1.11.0. Use
/// @c ::ArSession_getSupportedCameraConfigsWithFilter instead.
// TODO(b/146903940): Change ArSession_getSupportedCameraConfigs to return
// ArStatus.
void ArSession_getSupportedCameraConfigs(const ArSession *session,
                                         ArCameraConfigList *list)
    AR_DEPRECATED(
        "Deprecated in release 1.11.0. Please see function documentation.");

/// @ingroup ArSession
/// Sets the @c ::ArCameraConfig that the @c ::ArSession should use. Can only be
/// called while the session is paused. The provided @c ::ArCameraConfig must
/// be one of the configs returned by
/// @c ::ArSession_getSupportedCameraConfigsWithFilter.
///
/// The camera config will be applied once the session is resumed.
/// All previously acquired frame images must be released with
/// @c ::ArImage_release before calling @c resume(). Failure to do so will cause
/// @c resume() to return
/// @c #AR_ERROR_ILLEGAL_STATE error.
///
/// Note: Starting in ARCore 1.12, changing the active camera config may cause
/// the tracking state on certain devices to become permanently
/// @c #AR_TRACKING_STATE_PAUSED. For consistent behavior across all supported
/// devices, release any previously created anchors and trackables when setting
/// a new camera config.
///
/// Changing the camera config for an existing session may affect which ARCore
/// features can be used. Unsupported session features are silently disabled
/// when the session is resumed. Call @c ::ArSession_configure after setting a
/// camera config to verify that all configured session features are supported
/// with the new camera config.
///
/// Changing the current session's camera config to one that uses a different
/// camera will cause all internal session states to be reset when the session
/// is next resumed by calling @c ::ArSession_resume.
///
/// @param[in]    session          The ARCore session
/// @param[in]    camera_config    The provided @c ::ArCameraConfig must be from
///    a list returned by @c ::ArSession_getSupportedCameraConfigsWithFilter.
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_INVALID_ARGUMENT
/// - @c #AR_ERROR_SESSION_NOT_PAUSED
ArStatus ArSession_setCameraConfig(const ArSession *session,
                                   const ArCameraConfig *camera_config);

/// @ingroup ArSession
/// Gets the @c ::ArCameraConfig that the @c ::ArSession is currently using.  If
/// the camera config was not explicitly set then it returns the default camera
/// config.  Use @c ::ArCameraConfig_destroy to release memory associated with
/// the returned camera config once it is no longer needed.
///
/// @param[in]    session           The ARCore session
/// @param[inout] out_camera_config The camera config object to fill. This
///      object must have already been allocated with @c
///      ::ArCameraConfig_create. Use @c ::ArCameraConfig_destroy to release
///      memory associated with @p out_camera_config once it is no longer
///      needed.
void ArSession_getCameraConfig(const ArSession *session,
                               ArCameraConfig *out_camera_config);

/// @ingroup ArSession
/// Gets the list of supported camera configs that satisfy the provided
/// filter settings.
///
/// The returned camera configs might vary at runtime depending on device
/// capabilities. Overly restrictive filtering can result in the returned list
/// being empty on one or more devices.
///
/// Can be called at any time.
///
/// Beginning with ARCore SDK 1.15.0, some devices support additional camera
/// configs with lower GPU texture resolutions than the device's default GPU
/// texture resolution. These additional resolutions are only returned when the
/// filter is not a @c nullptr. See the ARCore supported devices
/// (https://developers.google.com/ar/devices) page for
/// an up to date list of supported devices.
///
/// Beginning with ARCore SDK 1.21.0, some devices will return additional camera
/// configs for supported stereo cameras. See the ARCore supported devices
/// (https://developers.google.com/ar/devices) page for
/// available camera configs by device.
///
/// Beginning with ARCore SDK 1.23.0, the list of returned camera configs will
/// include front-facing (selfie) and back-facing (world) camera configs. In
/// previous SDKs, returned camera configs included only front-facing (selfie)
/// or only back-facing (world) camera configs, depending on whether the
/// deprecated @c ::AR_SESSION_FEATURE_FRONT_CAMERA feature was used.
///
/// Element 0 will contain the camera config that best matches the filter
/// settings, according to the following priority:
///
/// 1. Stereo camera usage: prefer
///    @c #AR_CAMERA_CONFIG_STEREO_CAMERA_USAGE_REQUIRE_AND_USE over
///    @c #AR_CAMERA_CONFIG_STEREO_CAMERA_USAGE_DO_NOT_USE
/// 2. Target FPS: prefer @c #AR_CAMERA_CONFIG_TARGET_FPS_60 over
///    @c #AR_CAMERA_CONFIG_TARGET_FPS_30
/// 3. Depth sensor usage: prefer
///    @c #AR_CAMERA_CONFIG_DEPTH_SENSOR_USAGE_REQUIRE_AND_USE over
///    @c #AR_CAMERA_CONFIG_DEPTH_SENSOR_USAGE_DO_NOT_USE
///
/// No guarantees are made about the order in which the remaining elements are
/// returned.
///
/// @return list of supported camera configs.
// TODO(b/146903940): Change ArSession_getSupportedCameraConfigsWithFilter to
// return ArStatus.
void ArSession_getSupportedCameraConfigsWithFilter(
    const ArSession *session,
    const ArCameraConfigFilter *filter,
    ArCameraConfigList *list);

/// @ingroup ArSession
/// Returns the @c ::ArEarth object for the session. This object is long-lived;
/// it should be used for the entire duration of the ARCore session or until an
/// @c ::ArConfig with @c #AR_GEOSPATIAL_MODE_DISABLED is applied on the @c
/// ::ArSession.
///
/// @c ::ArEarth can only be acquired when an @c ::ArConfig with @c
/// #AR_GEOSPATIAL_MODE_ENABLED is active on the @c ::ArSession. See @c
/// ::ArConfig_setGeospatialMode to enable the Geospatial API.
///
/// @c ::ArEarth must be released by calling @c ::ArTrackable_release.
///
/// @param[in]    session   The ARCore session.
/// @param[out]   out_earth Pointer to @c ::ArEarth to receive the earth object,
///     or @c NULL if the session's config @c ::ArGeospatialMode is set to @c
///     #AR_GEOSPATIAL_MODE_DISABLED.
void ArSession_acquireEarth(const ArSession *session, ArEarth **out_earth);

/// @ingroup ArSession
/// Sets a MP4 dataset file to play back instead of using the live camera feed
/// and IMU sensor data.
///
/// Restrictions:
/// - Can only be called while the session is paused. Playback of the MP4
///   dataset file will start once the session is resumed.
/// - The MP4 dataset file must use the same camera facing direction as is
///   configured in the session.
/// - Due to the way session data is processed, ARCore APIs may sometimes
///   produce different results during playback than during recording and
///   produce different results during subsequent playback sessions.
///   For example, the number of detected planes and other trackables, the
///   precise timing of their detection and their pose over time may be
///   different in subsequent playback sessions.
/// - Once playback has started (due to the first call to @c
///   ::ArSession_resume), pausing the session (by calling @c ::ArSession_pause)
///   will suspend processing of all camera image frames and any other recorded
///   sensor data in the dataset. Camera image frames and sensor frame data that
///   is discarded in this way will not be reprocessed when the session is again
///   resumed (by calling @c ::ArSession_resume). AR tracking for the session
///   will generally suffer due to the gap in processed data.
///
/// When an MP4 dataset file is set:
/// - All existing trackables (@c ::ArAnchor and @c ::ArTrackable) immediately
///   enter tracking state @c #AR_TRACKING_STATE_STOPPED.
/// - The desired focus mode (@c ::ArConfig_setFocusMode) is ignored, and will
///   not affect the previously recorded camera images.
/// - The current camera configuration (@c ::ArCameraConfig) is immediately set
///   to the default for the device the MP4 dataset file was recorded on.
/// - Calls to @c ::ArSession_getSupportedCameraConfigs will return camera
///   configs supported by the device the MP4 dataset file was recorded on.
/// - Setting a previously obtained camera config to
///   @c ::ArSession_setCameraConfig will have no effect.
///
/// @param[in] session               The ARCore session
/// @param[in] mp4_dataset_file_path A string file path to a MP4 dataset file
///     or @c NULL to use the live camera feed.
///
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_SESSION_NOT_PAUSED if called when session is not paused.
/// - @c #AR_ERROR_SESSION_UNSUPPORTED if playback is incompatible with selected
///   features.
/// - @c #AR_ERROR_PLAYBACK_FAILED if an error occurred with the MP4 dataset
///   file such as not being able to open the file or the file is unable to be
///   decoded.
/// @deprecated Deprecated in release 1.26.0. Use
/// @c ::ArRecordingConfig_setMp4DatasetUri instead.
ArStatus ArSession_setPlaybackDataset(ArSession *session,
                                      const char *mp4_dataset_file_path)
    AR_DEPRECATED(
        "Deprecated in release 1.26.0. Please see function documentation");

/// @ingroup ArSession
/// Sets a MP4 dataset file to play back instead of using the live camera feed
/// and IMU sensor data.
///
/// This overrides the last path set by @c ::ArSession_setPlaybackDataset.
//
/// The URI must point to a resource that supports @c lseek.
//
/// See @c ::ArSession_setPlaybackDataset for more restrictions and details.
///
/// @param[in] session         The ARCore session
/// @param[in] mp4_dataset_uri The percent encoded, null terminated URI for the
/// dataset
///
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_INVALID_ARGUMENT if the file descriptor is not seekable.
/// - @c #AR_ERROR_SESSION_NOT_PAUSED if called when session is not paused.
/// - @c #AR_ERROR_SESSION_UNSUPPORTED if playback is incompatible with selected
///   features.
/// - @c #AR_ERROR_PLAYBACK_FAILED if an error occurred with the MP4 dataset
///   such as not being able to open the resource or the resource is unable to
///   be decoded.
ArStatus ArSession_setPlaybackDatasetUri(ArSession *session,
                                         const char *mp4_dataset_uri);

/// @ingroup ArRecordingConfig
/// Describe the current playback status.
AR_DEFINE_ENUM(ArPlaybackStatus){
    /// The session is not playing back an MP4 dataset file.
    AR_PLAYBACK_NONE = 0,
    /// Playback is in process without issues.
    AR_PLAYBACK_OK = 1,
    /// Playback has stopped due to an error.
    AR_PLAYBACK_IO_ERROR = 2,
    /// Playback has finished successfully.
    AR_PLAYBACK_FINISHED = 3,
};

/// @ingroup ArSession
/// Gets the playback status.
///
/// @param[in]  session             The ARCore session.
/// @param[out] out_playback_status The current playback status.
void ArSession_getPlaybackStatus(ArSession *session,
                                 ArPlaybackStatus *out_playback_status);

/// @ingroup ArSession
/// Starts a new MP4 dataset file recording that is written to the specific
/// filesystem path.
///
/// Existing files will be overwritten.
///
///
/// The MP4 video stream (VGA) bitrate is 5Mbps (40Mb per minute).
///
/// Recording introduces additional overhead and may affect app performance.
///
/// Session recordings may contain sensitive information. See <a
/// href="https://developers.google.com/ar/develop/recording-and-playback#what%E2%80%99s_in_a_recording">documentation
/// on Recording and Playback</a> to learn which data is saved in a recording.
///
/// @param[in] session           The ARCore session
/// @param[in] recording_config  The configuration defined for recording.
///
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_ILLEGAL_STATE
/// - @c #AR_ERROR_INVALID_ARGUMENT
/// - @c #AR_ERROR_RECORDING_FAILED
ArStatus ArSession_startRecording(ArSession *session,
                                  const ArRecordingConfig *recording_config);

/// @ingroup ArSession
/// Stops recording and flushes unwritten data to disk. The MP4 dataset file
/// will be ready to read after this call.
///
/// Recording can be stopped automatically when @c ::ArSession_pause is called,
/// if auto stop is enabled via @c ::ArRecordingConfig_setAutoStopOnPause.
/// Recording errors that would be thrown in stopRecording() are silently
/// ignored in @c ::ArSession_pause.
///
/// @param[in] session  The ARCore session
///
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_RECORDING_FAILED
ArStatus ArSession_stopRecording(ArSession *session);

/// @ingroup ArSession
/// Returns the current recording status.
///
/// @param[in] session The ARCore session.
/// @param[out] out_recording_status The current recording status.
void ArSession_getRecordingStatus(ArSession *session,
                                  ArRecordingStatus *out_recording_status);

/// @ingroup ArFrame
/// Writes a data sample in the specified track. The samples recorded using
/// this API will be muxed into the recorded MP4 dataset in a corresponding
/// additional MP4 stream.
///
/// For smooth playback of the MP4 on video players and for future compatibility
/// of the MP4 datasets with ARCore's playback of tracks it is
/// recommended that the samples are recorded at a frequency no higher
/// than 90kHz.
///
/// Additionally, if the samples are recorded at a frequency lower than 1Hz,
/// empty padding samples will be automatically recorded at approximately
/// one second intervals to fill in the gaps.
///
/// Recording samples introduces additional CPU and/or I/O overhead and
/// may affect app performance.

///
/// @param[in] session                     The ARCore session
/// @param[in] frame                       The current @c ::ArFrame
/// @param[in] track_id_uuid_16            The external track ID as UUID as a
/// byte array of 16 bytes in size
/// @param[in] payload                     The byte array payload to record
/// @param[in] payload_size                Size in bytes of the payload
///
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_ILLEGAL_STATE when either @c ::ArSession_getRecordingStatus
///   is not currently @c ::AR_RECORDING_OK or the system is currently under
///   excess load for images to be produced. The system should not be under such
///   excess load for more than a few frames and an app should try to record the
///   data again during the next frame.
/// - @c #AR_ERROR_INVALID_ARGUMENT when any argument is invalid, e.g. null.
/// - @c #AR_ERROR_DEADLINE_EXCEEDED when the frame is not the current frame.
ArStatus ArFrame_recordTrackData(ArSession *session,
                                 const ArFrame *frame,
                                 const uint8_t *track_id_uuid_16,
                                 const void *payload,
                                 size_t payload_size);

/// @ingroup ArSession
/// Checks whether the provided @c ::ArDepthMode is supported on this device
/// with the selected camera configuration. The current list of supported
/// devices is documented on the <a
/// href="https://developers.google.com/ar/devices">ARCore
/// supported devices</a> page.
///
/// @param[in] session The ARCore session.
/// @param[in] depth_mode The desired depth mode to check.
/// @param[out] out_is_supported Non zero if the depth mode is supported on this
///     device.
void ArSession_isDepthModeSupported(const ArSession *session,
                                    ArDepthMode depth_mode,
                                    int32_t *out_is_supported);

/// @ingroup ArSession
/// Checks whether the provided @c ::ArImageStabilizationMode is supported on
/// this device with the selected camera configuration. See <a
/// href="https://developers.google.com/ar/develop/c/electronic-image-stabilization">Enabling
/// Electronic Image Stabilization</a> for more information.
///
/// @param[in] session The ARCore session.
/// @param[in] image_stabilization_mode The desired image stabilization mode to
/// check.
/// @param[out] out_is_supported Non zero if given @c ::ArImageStabilizationMode
/// is supported on this device.
void ArSession_isImageStabilizationModeSupported(
    const ArSession *session,
    ArImageStabilizationMode image_stabilization_mode,
    int32_t *out_is_supported);

/// @ingroup ArSession
/// Checks whether the provided @c ::ArSemanticMode is supported on this device
/// with the selected camera configuration. The current list of supported
/// devices is documented on the <a
/// href="https://developers.google.com/ar/devices">ARCore
/// supported devices</a> page.
///
/// @param[in] session The ARCore session.
/// @param[in] semantic_mode The desired semantic mode to check.
/// @param[out] out_is_supported Non zero if the semantic mode is supported on
/// this device.
void ArSession_isSemanticModeSupported(const ArSession *session,
                                       ArSemanticMode semantic_mode,
                                       int32_t *out_is_supported);

/// @ingroup ArSession
/// Checks whether the provided @c ::ArGeospatialMode is supported on this
/// device. The current list of supported devices is documented on the <a
/// href="https://developers.google.com/ar/devices">ARCore supported devices</a>
/// page. A device may be incompatible with a given @c ::ArGeospatialMode due to
/// insufficient sensor capabilities.
///
/// @param[in] session           The ARCore session.
/// @param[in] geospatial_mode   The desired geospatial mode to check.
/// @param[out] out_is_supported Non-zero if the @c ::ArGeospatialMode is
///     supported on this device.
void ArSession_isGeospatialModeSupported(const ArSession *session,
                                         ArGeospatialMode geospatial_mode,
                                         int32_t *out_is_supported);

// === ArPose functions ===

/// @ingroup ArPose
/// Allocates and initializes a new pose object. @p pose_raw points to an array
/// of 7 floats, describing the rotation (quaternion) and translation of the
/// pose in the same order as the first 7 elements of the Android
/// @c Sensor.TYPE_POSE_6DOF values documented on <a
/// href="https://developer.android.com/reference/android/hardware/SensorEvent.html#values"
/// >@c SensorEvent.values() </a>.
///
/// The order of the values is: qx, qy, qz, qw, tx, ty, tz.
///
/// If @p pose_raw is @c NULL, initializes with the identity pose.
void ArPose_create(const ArSession *session,
                   const float *pose_raw,
                   ArPose **out_pose);

/// @ingroup ArPose
/// Releases memory used by a pose object.
void ArPose_destroy(ArPose *pose);

/// @ingroup ArPose
/// Extracts the quaternion rotation and translation from a pose object.
/// @param[in]  session         The ARCore session
/// @param[in]  pose            The pose to extract
/// @param[out] out_pose_raw_7  Pointer to an array of 7 floats, to be filled
///     with the quaternion rotation and translation as described in
///     @c ::ArPose_create.
void ArPose_getPoseRaw(const ArSession *session,
                       const ArPose *pose,
                       float *out_pose_raw_7);

/// @ingroup ArPose
/// Converts a pose into a 4x4 transformation matrix.
/// @param[in]  session                  The ARCore session
/// @param[in]  pose                     The pose to convert
/// @param[out] out_matrix_col_major_4x4 Pointer to an array of 16 floats, to be
///     filled with a column-major homogenous transformation matrix, as used by
///     OpenGL.
void ArPose_getMatrix(const ArSession *session,
                      const ArPose *pose,
                      float *out_matrix_col_major_4x4);

// === ArCamera functions ===

/// @ingroup ArCamera
/// Sets @p out_pose to the pose of the physical camera in world space for the
/// latest frame. This is an OpenGL camera pose with +X pointing right, +Y
/// pointing right up, -Z pointing in the direction the camera is looking, with
/// "right" and "up" being relative to the image readout in the usual
/// left-to-right top-to-bottom order. Specifically, this is the camera pose at
/// the center of exposure of the center row of the image.
///
/// <b>For applications using the SDK for ARCore 1.5.0 and earlier</b>, the
/// returned pose is rotated around the Z axis by a multiple of 90 degrees so
/// that the axes correspond approximately to those of the <a
/// href="https://developer.android.com/guide/topics/sensors/sensors_overview#sensors-coords">Android
/// Sensor Coordinate System</a>.
///
/// See Also:
///
/// * @c ::ArCamera_getDisplayOrientedPose for the pose of the virtual camera.
/// It will differ by a local rotation about the Z axis by a multiple of 90
///   degrees.
/// * @c ::ArFrame_getAndroidSensorPose for the pose of the Android sensor
/// frame.
///   It will differ in both orientation and location.
/// * @c ::ArFrame_transformCoordinates2d to convert viewport coordinates to
///   texture coordinates.
///
/// Note: This pose is only useful when @c ::ArCamera_getTrackingState returns
/// @c #AR_TRACKING_STATE_TRACKING and otherwise should not be used.
///
/// @param[in]    session  The ARCore session
/// @param[in]    camera   The session's camera (retrieved from any frame).
/// @param[inout] out_pose An already-allocated @c ::ArPose object into which
///     the pose will be stored.
void ArCamera_getPose(const ArSession *session,
                      const ArCamera *camera,
                      ArPose *out_pose);

/// @ingroup ArCamera
/// Sets @p out_pose to the virtual camera pose in world space for rendering AR
/// content onto the latest frame. This is an OpenGL camera pose with +X
/// pointing right, +Y pointing up, and -Z pointing in the direction the camera
/// is looking, with "right" and "up" being relative to current logical display
/// orientation.
///
/// See Also:
///
/// * @c ::ArCamera_getViewMatrix to conveniently compute the OpenGL view
/// matrix.
/// * @c ::ArCamera_getPose for the physical pose of the camera. It will differ
/// by a local rotation about the Z axis by a multiple of 90 degrees.
/// * @c ::ArFrame_getAndroidSensorPose for the pose of the android sensor
/// frame. It will differ in both orientation and location.
/// * @c ::ArSession_setDisplayGeometry to update the display rotation.
///
/// Note: This pose is only useful when @c ::ArCamera_getTrackingState returns
/// @c #AR_TRACKING_STATE_TRACKING and otherwise should not be used.
///
/// @param[in]    session  The ARCore session
/// @param[in]    camera   The session's camera (retrieved from any frame).
/// @param[inout] out_pose An already-allocated @c ::ArPose object into which
///     the pose will be stored.
void ArCamera_getDisplayOrientedPose(const ArSession *session,
                                     const ArCamera *camera,
                                     ArPose *out_pose);

/// @ingroup ArCamera
/// Returns the view matrix for the camera for this frame. This matrix performs
/// the inverse transform as the pose provided by
/// @c ::ArCamera_getDisplayOrientedPose.
///
/// @param[in]    session           The ARCore session
/// @param[in]    camera            The session's camera.
/// @param[inout] out_col_major_4x4 Pointer to an array of 16 floats, to be
///     filled with a column-major homogenous transformation matrix, as used by
///     OpenGL.
void ArCamera_getViewMatrix(const ArSession *session,
                            const ArCamera *camera,
                            float *out_col_major_4x4);

/// @ingroup ArCamera
/// Gets the current motion tracking state of this camera. If this state is
/// anything other than @c #AR_TRACKING_STATE_TRACKING the pose should not be
/// considered useful. Use @c ::ArCamera_getTrackingFailureReason to determine
/// the best recommendation to provide to the user to restore motion tracking.
///
/// Note: Starting in ARCore 1.12, changing the active camera config using
/// @c ::ArSession_setCameraConfig may cause the tracking state on certain
/// devices to become permanently @c #AR_TRACKING_STATE_PAUSED. For consistent
/// behavior across all supported devices, release any previously created
/// anchors and trackables when setting a new camera config.
void ArCamera_getTrackingState(const ArSession *session,
                               const ArCamera *camera,
                               ArTrackingState *out_tracking_state);

/// @ingroup ArCamera
/// Gets the reason that @c ::ArCamera_getTrackingState is
/// @c #AR_TRACKING_STATE_PAUSED.
///
/// Note: This function returns
/// @c #AR_TRACKING_FAILURE_REASON_NONE briefly after
/// @c ::ArSession_resume while the motion tracking is initializing.
/// This function always returns
/// @c #AR_TRACKING_FAILURE_REASON_NONE when
/// @c ::ArCamera_getTrackingState is @c #AR_TRACKING_STATE_TRACKING.
///
/// If multiple potential causes for motion tracking failure are detected,
/// this reports the most actionable failure reason.
void ArCamera_getTrackingFailureReason(
    const ArSession *session,
    const ArCamera *camera,
    ArTrackingFailureReason *out_tracking_failure_reason);

/// @ingroup ArCamera
/// Computes a projection matrix for rendering virtual content on top of the
/// camera image. Note that the projection matrix reflects the current display
/// geometry and display rotation.
///
/// Note: When using the front-facing (selfie) camera, the returned
/// projection matrix will incorporate a horizontal flip.
///
/// @param[in]    session            The ARCore session
/// @param[in]    camera             The session's camera.
/// @param[in]    near               Specifies the near clip plane, in meters
/// @param[in]    far                Specifies the far clip plane, in meters
/// @param[inout] dest_col_major_4x4 Pointer to an array of 16 floats, to
///     be filled with a column-major homogenous transformation matrix, as used
///     by OpenGL.
void ArCamera_getProjectionMatrix(const ArSession *session,
                                  const ArCamera *camera,
                                  float near,
                                  float far,
                                  float *dest_col_major_4x4);

/// @ingroup ArCamera
/// Retrieves the unrotated and uncropped intrinsics for the image (CPU) stream.
/// The intrinsics may change per frame, so this should be called
/// on each frame to get the intrinsics for the current frame.
///
/// @param[in]    session                The ARCore session
/// @param[in]    camera                 The session's camera.
/// @param[inout] out_camera_intrinsics  The @c ::ArCameraIntrinsics data.
void ArCamera_getImageIntrinsics(const ArSession *session,
                                 const ArCamera *camera,
                                 ArCameraIntrinsics *out_camera_intrinsics);

/// @ingroup ArCamera
/// Retrieves the unrotated and uncropped intrinsics for the GPU texture
/// stream. The intrinsics may change per frame, so this should be called
/// on each frame to get the intrinsics for the current frame.
///
/// @param[in]    session                The ARCore session
/// @param[in]    camera                 The session's camera.
/// @param[inout] out_camera_intrinsics  The @c ::ArCameraIntrinsics data.
void ArCamera_getTextureIntrinsics(const ArSession *session,
                                   const ArCamera *camera,
                                   ArCameraIntrinsics *out_camera_intrinsics);

/// @ingroup ArCamera
/// Releases a reference to the camera.  This must match a call to
/// @c ::ArFrame_acquireCamera.
///
/// This function may safely be called with @c NULL - it will do nothing.
void ArCamera_release(ArCamera *camera);

// === ArCameraIntrinsics functions ===
/// @ingroup ArCameraIntrinsics
/// Allocates a camera intrinstics object.
///
/// @param[in]    session                The ARCore session
/// @param[inout] out_camera_intrinsics  The @c ::ArCameraIntrinsics data.
void ArCameraIntrinsics_create(const ArSession *session,
                               ArCameraIntrinsics **out_camera_intrinsics);

/// @ingroup ArCameraIntrinsics
/// Returns the camera's focal length in pixels.
/// The focal length is conventionally represented in pixels. For a detailed
/// explanation, please see https://ksimek.github.io/2013/08/13/intrinsic.
/// Pixels-to-meters conversion can use SENSOR_INFO_PHYSICAL_SIZE and
/// SENSOR_INFO_PIXEL_ARRAY_SIZE in the Android Camera Characteristics API.
void ArCameraIntrinsics_getFocalLength(const ArSession *session,
                                       const ArCameraIntrinsics *intrinsics,
                                       float *out_fx,
                                       float *out_fy);

/// @ingroup ArCameraIntrinsics
/// Returns the principal point in pixels.
void ArCameraIntrinsics_getPrincipalPoint(const ArSession *session,
                                          const ArCameraIntrinsics *intrinsics,
                                          float *out_cx,
                                          float *out_cy);

/// @ingroup ArCameraIntrinsics
/// Returns the image's width and height in pixels.
void ArCameraIntrinsics_getImageDimensions(const ArSession *session,
                                           const ArCameraIntrinsics *intrinsics,
                                           int32_t *out_width,
                                           int32_t *out_height);

/// @ingroup ArCameraIntrinsics
/// Releases the provided camera intrinsics object.
void ArCameraIntrinsics_destroy(ArCameraIntrinsics *camera_intrinsics);

// === ArFrame functions ===

/// @ingroup ArFrame
/// Allocates a new @c ::ArFrame object, storing the pointer into @p *out_frame.
///
/// Note: the same @c ::ArFrame can be used repeatedly when calling
/// @c ::ArSession_update.
void ArFrame_create(const ArSession *session, ArFrame **out_frame);

/// @ingroup ArFrame
/// Releases an @c ::ArFrame and any references it holds.
void ArFrame_destroy(ArFrame *frame);

/// @ingroup ArFrame
/// Checks if the display rotation or viewport geometry changed since the
/// previous call to @c ::ArSession_update. The application should re-query
/// @c ::ArCamera_getProjectionMatrix and @c ::ArFrame_transformCoordinates2d
/// whenever this emits non-zero.
void ArFrame_getDisplayGeometryChanged(const ArSession *session,
                                       const ArFrame *frame,
                                       int32_t *out_geometry_changed);

/// @ingroup ArFrame
/// Returns the timestamp in nanoseconds when this image was captured. This can
/// be used to detect dropped frames or measure the camera frame rate. The time
/// base of this value is specifically <b>not</b> defined, but it is likely
/// similar to @c clock_gettime(CLOCK_BOOTTIME).
void ArFrame_getTimestamp(const ArSession *session,
                          const ArFrame *frame,
                          int64_t *out_timestamp_ns);

/// @ingroup ArFrame
/// Sets @p out_pose to the pose of the <a
/// href="https://developer.android.com/guide/topics/sensors/sensors_overview#sensors-coords">Android
/// Sensor Coordinate System</a> in the world coordinate space for this frame.
/// The orientation follows the device's "native" orientation (it is not
/// affected by display rotation) with all axes corresponding to those of the
/// Android sensor coordinates.
///
/// See Also:
///
/// * @c ::ArCamera_getDisplayOrientedPose for the pose of the virtual camera.
/// * @c ::ArCamera_getPose for the pose of the physical camera.
/// * @c ::ArFrame_getTimestamp for the system time that this pose was estimated
///   for.
///
/// Note: This pose is only useful when @c ::ArCamera_getTrackingState returns
/// @c #AR_TRACKING_STATE_TRACKING and otherwise should not be used.
///
/// @param[in]    session  The ARCore session
/// @param[in]    frame    The current frame.
/// @param[inout] out_pose An already-allocated @c ::ArPose object into which
///     the pose will be stored.
void ArFrame_getAndroidSensorPose(const ArSession *session,
                                  const ArFrame *frame,
                                  ArPose *out_pose);

/// @ingroup ArFrame
/// Transform the given texture coordinates to correctly show the background
/// image. This accounts for the display rotation, and any additional required
/// adjustment. For performance, this function should be called only if
/// @c ::ArFrame_getDisplayGeometryChanged indicates a change.
///
/// @param[in]    session      The ARCore session
/// @param[in]    frame        The current frame.
/// @param[in]    num_elements The number of floats to transform.  Must be
///     a multiple of 2.  @p uvs_in and @p uvs_out must point to arrays of at
///     least this many floats.
/// @param[in]    uvs_in       Input UV coordinates in normalized screen space.
/// @param[inout] uvs_out      Output UV coordinates in texture coordinates.
/// @deprecated Deprecated in release 1.7.0. Use
/// @c ::ArFrame_transformCoordinates2d instead.
void ArFrame_transformDisplayUvCoords(const ArSession *session,
                                      const ArFrame *frame,
                                      int32_t num_elements,
                                      const float *uvs_in,
                                      float *uvs_out)
    AR_DEPRECATED(
        "Deprecated in release 1.7.0. Please see function documentation.");

/// @ingroup ArFrame
/// Transforms a list of 2D coordinates from one 2D coordinate system to another
/// 2D coordinate system.
///
/// For Android view coordinates (@c #AR_COORDINATES_2D_VIEW,
/// @c #AR_COORDINATES_2D_VIEW_NORMALIZED), the view information is taken from
/// the most recent call to @c ::ArSession_setDisplayGeometry.
///
/// Must be called on the most recently obtained @c ::ArFrame object. If this
/// function is called on an older frame, a log message will be printed and
/// @p out_vertices_2d will remain unchanged.
///
/// Some examples of useful conversions:
///  - To transform from [0,1] range to screen-quad coordinates for rendering:
///    @c #AR_COORDINATES_2D_VIEW_NORMALIZED ->
///    @c #AR_COORDINATES_2D_TEXTURE_NORMALIZED
///  - To transform from [-1,1] range to screen-quad coordinates for rendering:
///    @c #AR_COORDINATES_2D_OPENGL_NORMALIZED_DEVICE_COORDINATES ->
///    @c #AR_COORDINATES_2D_TEXTURE_NORMALIZED
///  - To transform a point found by a computer vision algorithm in a cpu image
///    into a point on the screen that can be used to place an Android View
///    (e.g. Button) at that location:
///    @c #AR_COORDINATES_2D_IMAGE_PIXELS -> @c #AR_COORDINATES_2D_VIEW
///  - To transform a point found by a computer vision algorithm in a CPU image
///    into a point to be rendered using GL in clip-space ([-1,1] range):
///    @c #AR_COORDINATES_2D_IMAGE_PIXELS ->
///    @c #AR_COORDINATES_2D_OPENGL_NORMALIZED_DEVICE_COORDINATES
///
/// If @p inputCoordinates is same as @p outputCoordinates, the input vertices
/// will be copied to the output vertices unmodified.
///
/// @param[in]  session         The ARCore session.
/// @param[in]  frame           The current frame.
/// @param[in]  input_coordinates The coordinate system used by @p vectors2d_in.
/// @param[in]  number_of_vertices The number of 2D vertices to transform.
///                             @p vertices_2d and @p out_vertices_2d must
///                             point to arrays of size at least
///                             @p number_of_vertices * 2.
/// @param[in] vertices_2d      Input 2D vertices to transform.
/// @param[in] output_coordinates The coordinate system to convert to.
/// @param[inout] out_vertices_2d Transformed 2d vertices, can be the same array
///                             as @p vertices_2d for in-place transform.
void ArFrame_transformCoordinates2d(const ArSession *session,
                                    const ArFrame *frame,
                                    ArCoordinates2dType input_coordinates,
                                    int32_t number_of_vertices,
                                    const float *vertices_2d,
                                    ArCoordinates2dType output_coordinates,
                                    float *out_vertices_2d);

/// @ingroup ArFrame
/// Transforms a list of 2D coordinates from one 2D coordinate space to 3D
/// coordinate space. See the <a
/// href="https://developers.google.com/ar/develop/c/electronic-image-stabilization">Electronic
/// Image Stabilization Developer Guide</a> for more information.
///
/// The view information is taken from the most recent call to @c
/// ::ArSession_setDisplayGeometry.
///
/// If Electronic Image Stabilization is off, the device coordinates return (-1,
/// -1, 0) -> (1, 1, 0) and texture coordinates return the same coordinates as
/// @c ::ArFrame_transformCoordinates2d with the Z component set to 1.0f.
///
/// In order to use EIS, your app should use EIS compensated screen coordinates
/// and camera texture coordinates to pass on to shaders. Use the 2D NDC space
/// coordinates as input to obtain EIS compensated 3D screen coordinates and
/// matching camera texture coordinates.
///
/// @param[in]  session         The ARCore session.
/// @param[in]  frame           The current frame.
/// @param[in]  input_coordinates The coordinate system used by @p vectors2d_in.
/// @param[in]  number_of_vertices The number of 2D vertices to transform.
///                             @p vertices_2d must
///                             point to arrays of size at least
///                             @p number_of_vertices * 2. And @p
///                             out_vertices_2d must point to arrays of size at
///                             least @p number_of_vertices * 3.
/// @param[in] vertices_2d      Input 2D vertices to transform.
/// @param[in] output_coordinates The 3D coordinate system to convert to.
/// @param[out] out_vertices_3d Transformed 3d vertices.
void ArFrame_transformCoordinates3d(const ArSession *session,
                                    const ArFrame *frame,
                                    ArCoordinates2dType input_coordinates,
                                    int32_t number_of_vertices,
                                    const float *vertices_2d,
                                    ArCoordinates3dType output_coordinates,
                                    float *out_vertices_3d);

/// @ingroup ArFrame
/// Performs a ray cast from the user's device in the direction of the given
/// location in the camera view. Intersections with detected scene geometry are
/// returned, sorted by distance from the device; the nearest intersection is
/// returned first.
///
/// Note: Significant geometric leeway is given when returning hit results. For
/// example, a plane hit may be generated if the ray came close, but did not
/// actually hit within the plane extents or plane bounds
/// (@c ::ArPlane_isPoseInExtents and @c ::ArPlane_isPoseInPolygon can be used
/// to determine these cases). A point (Point Cloud) hit is generated when a
/// point is roughly within one finger-width of the provided screen coordinates.
///
/// The resulting list is ordered by distance, with the nearest hit first
///
/// Note: If not tracking, the @p hit_result_list will be empty.
///
/// Note: If called on an old frame (not the latest produced by
///     @c ::ArSession_update the @p hit_result_list will be empty).
///
/// Note: When using the front-facing (selfie) camera, the returned hit
/// result list will always be empty, as the camera is not
/// @c #AR_TRACKING_STATE_TRACKING. Hit testing against tracked faces is not
/// currently supported.
///
/// Note: In ARCore 1.24.0 or later on supported devices, if the
/// @c ::ArDepthMode is enabled by calling @c ::ArConfig_setDepthMode the
/// @p hit_result_list includes @c ::ArDepthPoint values that are sampled from
/// the latest computed depth image.
///
/// @param[in]    session         The ARCore session.
/// @param[in]    frame           The current frame.
/// @param[in]    pixel_x         Logical X position within the view, as from an
///     Android UI event.
/// @param[in]    pixel_y         Logical Y position within the view, as from an
///     Android UI event.
/// @param[inout] hit_result_list The list to fill.  This list must have been
///     previously allocated using @c ::ArHitResultList_create.  If the list has
///     been previously used, it will first be cleared.
void ArFrame_hitTest(const ArSession *session,
                     const ArFrame *frame,
                     float pixel_x,
                     float pixel_y,
                     ArHitResultList *hit_result_list);

/// @ingroup ArFrame
/// Performs a ray cast that can return a result before ARCore establishes full
/// tracking.
///
/// The pose and apparent scale of attached objects depends on the
/// @c ::ArInstantPlacementPoint tracking method and the provided
/// @p approximate_distance_meters. A discussion of the different tracking
/// methods and the effects of apparent object scale are described in
/// @c ::ArInstantPlacementPoint.
///
/// This function will succeed only if @c ::ArInstantPlacementMode is
/// @c #AR_INSTANT_PLACEMENT_MODE_LOCAL_Y_UP in the ARCore session
/// configuration, the ARCore session tracking state is @c
/// #AR_TRACKING_STATE_TRACKING, and there are sufficient feature points to
/// track the point in screen space.
///
/// @param[in]    session         The ARCore session.
/// @param[in]    frame           The current frame.
/// @param[in]    pixel_x         Logical X position within the view, as from an
///     Android UI event.
/// @param[in]    pixel_y         Logical Y position within the view, as from an
///     Android UI event.
/// @param[in]    approximate_distance_meters The distance at which to create an
///     @c ::ArInstantPlacementPoint. This is only used while the tracking
///     method for the returned point is
///     @c
///     #AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_SCREENSPACE_WITH_APPROXIMATE_DISTANCE.<!--NOLINT-->
/// @param[inout] hit_result_list The list to fill. If successful the list will
///     contain a single @c ::ArHitResult, otherwise it will be cleared. The
///     @c ::ArHitResult will have a trackable of type @c
///     ::ArInstantPlacementPoint. The list must have been previously allocated
///     using @c ::ArHitResultList_create.
void ArFrame_hitTestInstantPlacement(const ArSession *session,
                                     const ArFrame *frame,
                                     float pixel_x,
                                     float pixel_y,
                                     float approximate_distance_meters,
                                     ArHitResultList *hit_result_list);

/// @ingroup ArFrame
/// Similar to @c ::ArFrame_hitTest, but takes an arbitrary ray in world space
/// coordinates instead of a screen space point.
///
/// @param[in]    session         The ARCore session.
/// @param[in]    frame           The current frame.
/// @param[in]    ray_origin_3    A pointer to float[3] array containing ray
///     origin in world space coordinates.
/// @param[in]    ray_direction_3 A pointer to float[3] array containing ray
///     direction in world space coordinates. Does not have to be normalized.
/// @param[inout] hit_result_list The list to fill.  This list must have been
///     previously allocated using @c ::ArHitResultList_create.  If the list has
///     been previously used, it will first be cleared.
void ArFrame_hitTestRay(const ArSession *session,
                        const ArFrame *frame,
                        const float *ray_origin_3,
                        const float *ray_direction_3,
                        ArHitResultList *hit_result_list);

/// @ingroup ArFrame
/// Gets the current @c ::ArLightEstimate, if Lighting Estimation is enabled.
///
/// @param[in]    session            The ARCore session.
/// @param[in]    frame              The current frame.
/// @param[inout] out_light_estimate The @c ::ArLightEstimate to fill. This
///   object must have been previously created with @c ::ArLightEstimate_create.
void ArFrame_getLightEstimate(const ArSession *session,
                              const ArFrame *frame,
                              ArLightEstimate *out_light_estimate);

/// @ingroup ArFrame
/// Acquires the current set of estimated 3d points attached to real-world
/// geometry. A matching call to @c ::ArPointCloud_release must be made when the
/// application is done accessing the Point Cloud.
///
/// Note: This information is for visualization and debugging purposes only. Its
/// characteristics and format are subject to change in subsequent versions of
/// the API.
///
/// @param[in]  session         The ARCore session.
/// @param[in]  frame           The current frame.
/// @param[out] out_point_cloud Pointer to an @c ::ArPointCloud* receive the
///     address of the Point Cloud.
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_DEADLINE_EXCEEDED if @p frame is not the latest frame from
///   by @c ::ArSession_update.
/// - @c #AR_ERROR_RESOURCE_EXHAUSTED if too many Point Clouds are currently
///   held.
ArStatus ArFrame_acquirePointCloud(const ArSession *session,
                                   const ArFrame *frame,
                                   ArPointCloud **out_point_cloud);

/// @ingroup ArFrame
/// Returns the camera object for the session. Note that this Camera instance is
/// long-lived so the same instance is returned regardless of the frame object
/// this function was called on.
void ArFrame_acquireCamera(const ArSession *session,
                           const ArFrame *frame,
                           ArCamera **out_camera);

/// @ingroup ArFrame
/// Gets the camera metadata for the current camera image.
///
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_DEADLINE_EXCEEDED if @p frame is not the latest frame from
///   by @c ::ArSession_update.
/// - @c #AR_ERROR_RESOURCE_EXHAUSTED if too many metadata objects are currently
///   held.
/// - @c #AR_ERROR_NOT_YET_AVAILABLE if the camera failed to produce metadata
/// for
///   the given frame. Note: this commonly happens for few frames right
///   after @c ::ArSession_resume due to the camera stack bringup.
ArStatus ArFrame_acquireImageMetadata(const ArSession *session,
                                      const ArFrame *frame,
                                      ArImageMetadata **out_metadata);

/// @ingroup ArFrame
/// Returns the CPU image for the current frame.
/// Caller is responsible for later releasing the image with
/// @c ::ArImage_release. Not supported on all devices (see
/// https://developers.google.com/ar/devices). Return values:
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_INVALID_ARGUMENT - one more input arguments are invalid.
/// - @c #AR_ERROR_DEADLINE_EXCEEDED - the input frame is not the current frame.
/// - @c #AR_ERROR_RESOURCE_EXHAUSTED - the caller app has exceeded maximum
///   number of images that it can hold without releasing.
/// - @c #AR_ERROR_NOT_YET_AVAILABLE - image with the timestamp of the input
///   frame was not found within a bounded amount of time, or the camera failed
///   to produce the image
ArStatus ArFrame_acquireCameraImage(ArSession *session,
                                    ArFrame *frame,
                                    ArImage **out_image);

/// @ingroup ArFrame
/// Gets the set of anchors that were changed by the @c ::ArSession_update that
/// produced this Frame.
///
/// @param[in]    session            The ARCore session
/// @param[in]    frame              The current frame.
/// @param[inout] out_anchor_list The list to fill.  This list must have already
///     been allocated with @c ::ArAnchorList_create.  If previously used, the
///     list is cleared first.
void ArFrame_getUpdatedAnchors(const ArSession *session,
                               const ArFrame *frame,
                               ArAnchorList *out_anchor_list);

/// @ingroup ArFrame
/// Gets the set of trackables of a particular type that were changed by the
/// @c ::ArSession_update call that produced this Frame.
///
/// @param[in]    session            The ARCore session
/// @param[in]    frame              The current frame.
/// @param[in]    filter_type        The type(s) of trackables to return.  See
///     @c ::ArTrackableType for legal values.
/// @param[inout] out_trackable_list The list to fill.  This list must have
///     already been allocated with @c ::ArTrackableList_create.  If previously
///     used, the list is cleared first.
void ArFrame_getUpdatedTrackables(const ArSession *session,
                                  const ArFrame *frame,
                                  ArTrackableType filter_type,
                                  ArTrackableList *out_trackable_list);

/// @ingroup ArFrame
/// Gets the set of data recorded to the given track available during playback
/// on this @c ::ArFrame. If frames are skipped during playback, which can
/// happen when the device is under load, played back track data will be
/// attached to a later frame in order.
///
/// Note, currently playback continues internally while the session is paused.
/// Track data from frames that were processed while the session was
/// paused will be discarded.
///
/// @param[in]    session            The ARCore session
/// @param[in]    frame              The current frame
/// @param[in]    track_id_uuid_16   The track ID as UUID as a byte
/// array of 16 bytes in size
/// @param[inout] out_track_data_list The list to fill. This list must have
/// already been allocated with @c ::ArTrackDataList_create. If previously
/// used, the list will first be cleared
void ArFrame_getUpdatedTrackData(const ArSession *session,
                                 const ArFrame *frame,
                                 const uint8_t *track_id_uuid_16,
                                 ArTrackDataList *out_track_data_list);

// === ArTrackData functions ===

/// @ingroup ArTrackData
/// Retrieves the timestamp in nanoseconds of the frame the given
/// @c ::ArTrackData was recorded on. This timestamp is equal to the result of
/// @c ::ArFrame_getTimestamp on the frame during which the track data was
/// written.
///
/// @param[in]    session            The ARCore session
/// @param[in]    track_data         The @c ::ArTrackData to pull the timestamp
/// from
/// @param[inout] out_timestamp_ns   The @c int64_t timestamp value to be set
void ArTrackData_getFrameTimestamp(const ArSession *session,
                                   const ArTrackData *track_data,
                                   int64_t *out_timestamp_ns);

/// @ingroup ArTrackData
/// Retrieves the @c ::ArTrackData byte data that was recorded by
/// @c ::ArFrame_recordTrackData.
///
/// The pointer returned by this function is valid until
/// @c ::ArTrackData_release is called.
///
/// @param[in]    session            The ARCore session
/// @param[in]    track_data         The @c ::ArTrackData to pull the data from
/// @param[inout] out_data           The pointer to the @c ::ArTrackDataList
/// byte data being populated. Will point to an empty list if no data present
/// @param[inout] out_size           The number of bytes that were populated in
/// @p out_data, 0 if list is empty
void ArTrackData_getData(const ArSession *session,
                         const ArTrackData *track_data,
                         const uint8_t **out_data,
                         int32_t *out_size);

/// @ingroup ArTrackData
/// Releases a reference to a @c ::ArTrackData. Acquire a @c ::ArTrackData with
/// @c ::ArTrackDataList_acquireItem.
///
/// This function may safely be called with @c NULL - it will do nothing.
///
/// @param[inout] track_data      The @c ::ArTrackData being released
void ArTrackData_release(ArTrackData *track_data);

// === ArTrackDataList functions ===

/// @ingroup ArTrackData
/// Creates an @c ::ArTrackDataList object.
///
/// @param[in]    session             The ARCore session
/// @param[inout] out_track_data_list The pointer to the list to be initialized
void ArTrackDataList_create(const ArSession *session,
                            ArTrackDataList **out_track_data_list);

/// @ingroup ArTrackData
/// Releases the memory used by the @c ::ArTrackData list object
///
/// @param[inout] track_data_list The pointer to the @c ::ArTrackDataList
/// memory being released
void ArTrackDataList_destroy(ArTrackDataList *track_data_list);

/// @ingroup ArTrackData
/// Retrieves the number of @c ::ArTrackData elements in the list.
///
/// @param[in]    session             The ARCore session.
/// @param[in]    track_data_list     The list being checked for size
/// @param[inout] out_size            The size of the list, 0 if
/// @p track_data_list is empty
void ArTrackDataList_getSize(const ArSession *session,
                             const ArTrackDataList *track_data_list,
                             int32_t *out_size);

/// @ingroup ArTrackData
/// Acquires a reference to an indexed entry in the list.  This call must
/// eventually be matched with a call to @c ::ArTrackData_release.
///
/// @param[in]    session             The ARCore session
/// @param[in]    track_data_list     The list being queried
/// @param[in]    index               The index of the list
/// @param[inout] out_track_data      The pointer to the @c ::ArTrackData
/// acquired at the given index in the list
void ArTrackDataList_acquireItem(const ArSession *session,
                                 const ArTrackDataList *track_data_list,
                                 int32_t index,
                                 ArTrackData **out_track_data);

/// @ingroup ArFrame
/// Attempts to acquire a depth image that corresponds to the current frame.
///
/// The depth image has a single 16-bit plane at index 0, stored in
/// little-endian format. Each pixel contains the distance in millimeters along
/// the camera principal axis. Currently, the three most significant bits are
/// always set to 000. The remaining thirteen bits express values from 0 to
/// 8191, representing depth in millimeters. To extract distance from a depth
/// map, see <a
/// href="https://developers.google.com/ar/develop/c/depth/developer-guide#extract-distance">the
/// Depth API developer guide</a>.
///
/// The actual size of the depth image depends on the device and its display
/// aspect ratio. The size of the depth image is typically around 160x120
/// pixels, with higher resolutions up to 640x480 on some devices. These sizes
/// may change in the future. The outputs of
/// @c ::ArFrame_acquireDepthImage, @c ::ArFrame_acquireRawDepthImage and
/// @c ::ArFrame_acquireRawDepthConfidenceImage will all have the exact same
/// size.
///
/// Optimal depth accuracy is achieved between 500 millimeters (50 centimeters)
/// and 5000 millimeters (5 meters) from the camera. Error increases
/// quadratically as distance from the camera increases.
///
/// Depth is estimated using data from the world-facing cameras, user motion,
/// and hardware depth sensors such as a time-of-flight sensor (or ToF sensor)
/// if available. As the user moves their device through the environment, 3D
/// depth data is collected and cached which improves the quality of subsequent
/// depth images and reducing the error introduced by camera distance.
///
/// If an up-to-date depth image isn't ready for the current frame, the most
/// recent depth image available from an earlier frame will be returned instead.
/// This is expected only to occur on compute-constrained devices. An up-to-date
/// depth image should typically become available again within a few frames.
///
/// The image must be released with @c ::ArImage_release once it is no
/// longer needed.
///
/// @param[in]  session                The ARCore session.
/// @param[in]  frame                  The current frame.
/// @param[out] out_depth_image        On successful return, this is filled out
///     with a pointer to an @c ::ArImage. On error return, this is filled out
///     with @c nullptr.
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_INVALID_ARGUMENT if the session, frame, or depth image
///   arguments are invalid.
/// - @c #AR_ERROR_NOT_YET_AVAILABLE if the number of observed camera frames is
///   not yet sufficient for depth estimation; or depth estimation was not
///   possible due to poor lighting, camera occlusion, or insufficient motion
///   observed.
/// - @c #AR_ERROR_NOT_TRACKING The session is not in the
///   @c #AR_TRACKING_STATE_TRACKING state, which is required to acquire depth
///   images.
/// - @c #AR_ERROR_ILLEGAL_STATE if a supported depth mode was not enabled in
///   Session configuration.
/// - @c #AR_ERROR_RESOURCE_EXHAUSTED if the caller app has exceeded maximum
///   number of depth images that it can hold without releasing.
/// - @c #AR_ERROR_DEADLINE_EXCEEDED if the provided Frame is not the current
///   one.
///
/// @deprecated Deprecated in release 1.31.0. Please use
/// @c ::ArFrame_acquireDepthImage16Bits instead, which expands the depth range
/// from 8191mm to 65535mm. This deprecated version may be slower than
/// @c ::ArFrame_acquireDepthImage16Bits due to the clearing of the top 3 bits
/// per pixel.
ArStatus ArFrame_acquireDepthImage(const ArSession *session,
                                   const ArFrame *frame,
                                   ArImage **out_depth_image)
    AR_DEPRECATED(
        "Deprecated in release 1.31.0. Please use "
        "ArFrame_acquireDepthImage16Bits instead, which expands the depth "
        "range from 8191mm to 65535mm. This deprecated version may "
        "be slower than ArFrame_acquireDepthImage16Bits due to the clearing of "
        "the top 3 bits per pixel.");

/// @ingroup ArFrame
/// Attempts to acquire a depth image that corresponds to the current frame.
///
/// The depth image has format <a
/// href="https://developer.android.com/reference/android/hardware/HardwareBuffer#D_16">
/// HardwareBuffer.D_16</a>, which is a single 16-bit plane at index 0,
/// stored in little-endian format. Each pixel contains the distance in
/// millimeters along the camera principal axis, with the representable depth
/// range between 0 millimeters and 65535 millimeters, or about 65 meters.
///
/// To extract distance from a depth map, see <a
/// href="https://developers.google.com/ar/develop/c/depth/developer-guide#extract-distance">the
/// Depth API developer guide</a>.
////
/// The actual size of the depth image depends on the device and its display
/// aspect ratio. The size of the depth image is typically around 160x120
/// pixels, with higher resolutions up to 640x480 on some devices. These sizes
/// may change in the future. The outputs of
/// @c ::ArFrame_acquireDepthImage16Bits,
/// @c ::ArFrame_acquireRawDepthImage16Bits and
/// @c ::ArFrame_acquireRawDepthConfidenceImage will all have the exact same
/// size.
///
/// Optimal depth accuracy is achieved between 500 millimeters (50 centimeters)
/// and 15000 millimeters (15 meters) from the camera, with depth reliably
/// observed up to 25000 millimeters (25 meters). Error increases quadratically
/// as distance from the camera increases.
///
/// Depth is estimated using data from the world-facing cameras, user motion,
/// and hardware depth sensors such as a time-of-flight sensor (or ToF sensor)
/// if available. As the user moves their device through the environment, 3D
/// depth data is collected and cached which improves the quality of subsequent
/// depth images and reducing the error introduced by camera distance.
///
/// If an up-to-date depth image isn't ready for the current frame, the most
/// recent depth image available from an earlier frame will be returned instead.
/// This is expected only to occur on compute-constrained devices. An up-to-date
/// depth image should typically become available again within a few frames.
///
/// When the Geospatial API and the Depth API are enabled, output images
/// from the Depth API will include terrain and building geometry when in a
/// location with VPS coverage. See the <a
/// href="https://developers.google.com/ar/develop/c/depth/geospatial-depth">Geospatial
/// Depth Developer Guide</a> for more information.
///
/// The image must be released with @c ::ArImage_release once it is no
/// longer needed.
///
/// @param[in]  session                The ARCore session.
/// @param[in]  frame                  The current frame.
/// @param[out] out_depth_image        On successful return, this is filled out
///     with a pointer to an @c ::ArImage. On error return, this is filled out
///     with @c nullptr.
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_INVALID_ARGUMENT if the session, frame, or depth image
///   arguments are invalid.
/// - @c #AR_ERROR_NOT_YET_AVAILABLE if the number of observed camera frames is
///   not yet sufficient for depth estimation; or depth estimation was not
///   possible due to poor lighting, camera occlusion, or insufficient motion
///   observed.
/// - @c #AR_ERROR_NOT_TRACKING The session is not in the
///   @c #AR_TRACKING_STATE_TRACKING state, which is required to acquire depth
///   images.
/// - @c #AR_ERROR_ILLEGAL_STATE if a supported depth mode was not enabled in
///   Session configuration.
/// - @c #AR_ERROR_RESOURCE_EXHAUSTED if the caller app has exceeded maximum
///   number of depth images that it can hold without releasing.
/// - @c #AR_ERROR_DEADLINE_EXCEEDED if the provided Frame is not the current
///   one.
ArStatus ArFrame_acquireDepthImage16Bits(const ArSession *session,
                                         const ArFrame *frame,
                                         ArImage **out_depth_image);

/// @ingroup ArFrame
/// Attempts to acquire a "raw", mostly unfiltered, depth image that corresponds
/// to the current frame.
///
/// The raw depth image is sparse and does not provide valid depth for all
/// pixels. Pixels without a valid depth estimate have a pixel value of 0 and a
/// corresponding confidence value of 0 (see
/// @c ::ArFrame_acquireRawDepthConfidenceImage).
///
/// The depth image has a single 16-bit plane at index 0, stored in
/// little-endian format. Each pixel contains the distance in millimeters along
/// the camera principal axis. Currently, the three most significant bits are
/// always set to 000. The remaining thirteen bits express values from 0 to
/// 8191, representing depth in millimeters. To extract distance from a depth
/// map, see <a
/// href="https://developers.google.com/ar/develop/c/depth/developer-guide#extract-distance">the
/// Depth API developer guide</a>.
///
/// The actual size of the depth image depends on the device and its display
/// aspect ratio. The size of the depth image is typically around 160x120
/// pixels, with higher resolutions up to 640x480 on some devices. These sizes
/// may change in the future. The outputs of
/// @c ::ArFrame_acquireDepthImage, @c ::ArFrame_acquireRawDepthImage and
/// @c ::ArFrame_acquireRawDepthConfidenceImage will all have the exact same
/// size.
///
/// Optimal depth accuracy occurs between 500 millimeters (50 centimeters) and
/// 5000 millimeters (5 meters) from the camera. Error increases quadratically
/// as distance from the camera increases.
///
/// Depth is primarily estimated using data from the motion of world-facing
/// cameras. As the user moves their device through the environment, 3D depth
/// data is collected and cached, improving the quality of subsequent depth
/// images and reducing the error introduced by camera distance. Depth accuracy
/// and robustness improves if the device has a hardware depth sensor, such as a
/// time-of-flight (ToF) camera.
///
/// Not every raw depth image contains a new depth estimate. Typically there is
/// about 10 updates to the raw depth data per second. The depth images between
/// those updates are a 3D reprojection which transforms each depth pixel into a
/// 3D point in space and renders those 3D points into a new raw depth image
/// based on the current camera pose. This effectively transforms raw depth
/// image data from a previous frame to account for device movement since the
/// depth data was calculated. For some applications it may be important to know
/// whether the raw depth image contains new depth data or is a 3D reprojection
/// (for example, to reduce the runtime cost of 3D reconstruction). To do that,
/// compare the current raw depth image timestamp, obtained via @c
/// ::ArImage_getTimestamp, with the previously recorded raw depth image
/// timestamp. If they are different, the depth image contains new information.
///
/// The image must be released via @c ::ArImage_release once it is no longer
/// needed.
///
/// @param[in]  session                The ARCore session.
/// @param[in]  frame                  The current frame.
/// @param[out] out_depth_image        On successful return, this is filled out
///   with a pointer to an @c ::ArImage. On error return, this is filled out
///   filled out with @c nullptr.
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_INVALID_ARGUMENT if the session, frame, or depth image
///   arguments are invalid.
/// - @c #AR_ERROR_NOT_YET_AVAILABLE if the number of observed camera frames is
///   not yet sufficient for depth estimation; or depth estimation was not
///   possible due to poor lighting, camera occlusion, or insufficient motion
///   observed.
/// - @c #AR_ERROR_NOT_TRACKING The session is not in the
///   @c #AR_TRACKING_STATE_TRACKING state, which is required to acquire depth
///   images.
/// - @c #AR_ERROR_ILLEGAL_STATE if a supported depth mode was not enabled in
///   Session configuration.
/// - @c #AR_ERROR_RESOURCE_EXHAUSTED if the caller app has exceeded maximum
///   number of depth images that it can hold without releasing.
/// - @c #AR_ERROR_DEADLINE_EXCEEDED if the provided @c ::ArFrame is not the
///   current one.
///
/// @deprecated Deprecated in release 1.31.0. Please use
/// @c ::ArFrame_acquireRawDepthImage16Bits instead, which expands the depth
/// range from 8191mm to 65535mm. This deprecated version may be slower than
/// @c ::ArFrame_acquireRawDepthImage16Bits due to the clearing of the top 3
/// bits per pixel.
ArStatus ArFrame_acquireRawDepthImage(const ArSession *session,
                                      const ArFrame *frame,
                                      ArImage **out_depth_image)
    AR_DEPRECATED(
        "Deprecated in release 1.31.0. Please use "
        "ArFrame_acquireRawDepthImage16Bits instead, which expands the depth "
        "range from 8191mm to 65535mm. This deprecated version may "
        "be slower than ArFrame_acquireRawDepthImage16Bits due to the clearing "
        "of the top 3 bits per pixel.");

/// @ingroup ArFrame
/// Attempts to acquire a "raw", mostly unfiltered, depth image that corresponds
/// to the current frame.
///
/// The raw depth image is sparse and does not provide valid depth for all
/// pixels. Pixels without a valid depth estimate have a pixel value of 0 and a
/// corresponding confidence value of 0 (see
/// @c ::ArFrame_acquireRawDepthConfidenceImage).
///
/// The depth image has format <a
/// href="https://developer.android.com/reference/android/hardware/HardwareBuffer#D_16">
/// HardwareBuffer.D_16</a>, which is a single 16-bit plane at index 0,
/// stored in little-endian format. Each pixel contains the distance in
/// millimeters along the camera principal axis, with the representable depth
/// range between 0 millimeters and 65535 millimeters, or about 65 meters.
///
/// To extract distance from a depth map, see <a
/// href="https://developers.google.com/ar/develop/c/depth/developer-guide#extract-distance">the
/// Depth API developer guide</a>.
///
/// The actual size of the depth image depends on the device and its display
/// aspect ratio. The size of the depth image is typically around 160x120
/// pixels, with higher resolutions up to 640x480 on some devices. These sizes
/// may change in the future. The outputs of
/// @c ::ArFrame_acquireDepthImage16Bits,
/// @c ::ArFrame_acquireRawDepthImage16Bits and
/// @c ::ArFrame_acquireRawDepthConfidenceImage will all have the exact same
/// size.
///
/// Optimal depth accuracy is achieved between 500 millimeters (50 centimeters)
/// and 15000 millimeters (15 meters) from the camera, with depth reliably
/// observed up to 25000 millimeters (25 meters). Error increases quadratically
/// as distance from the camera increases.
///
/// Depth is primarily estimated using data from the motion of world-facing
/// cameras. As the user moves their device through the environment, 3D depth
/// data is collected and cached, improving the quality of subsequent depth
/// images and reducing the error introduced by camera distance. Depth accuracy
/// and robustness improves if the device has a hardware depth sensor, such as a
/// time-of-flight (ToF) camera.
///
/// Not every raw depth image contains a new depth estimate. Typically there are
/// about 10 updates to the raw depth data per second. The depth images between
/// those updates are a 3D reprojection which transforms each depth pixel into a
/// 3D point in space and renders those 3D points into a new raw depth image
/// based on the current camera pose. This effectively transforms raw depth
/// image data from a previous frame to account for device movement since the
/// depth data was calculated. For some applications it may be important to know
/// whether the raw depth image contains new depth data or is a 3D reprojection
/// (for example, to reduce the runtime cost of 3D reconstruction). To do that,
/// compare the current raw depth image timestamp, obtained via @c
/// ::ArImage_getTimestamp, with the previously recorded raw depth image
/// timestamp. If they are different, the depth image contains new information.
///
/// When the Geospatial API and the Depth API are enabled, output images
/// from the Depth API will include terrain and building geometry when in a
/// location with VPS coverage. See the <a
/// href="https://developers.google.com/ar/develop/c/depth/geospatial-depth">Geospatial
/// Depth Developer Guide</a> for more information.
///
/// The image must be released via @c ::ArImage_release once it is no longer
/// needed.
///
/// @param[in]  session                The ARCore session.
/// @param[in]  frame                  The current frame.
/// @param[out] out_depth_image        On successful return, this is filled out
///   with a pointer to an @c ::ArImage. On error return, this is filled out
///   filled out with @c nullptr.
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_INVALID_ARGUMENT if the session, frame, or depth image
///   arguments are invalid.
/// - @c #AR_ERROR_NOT_YET_AVAILABLE if the number of observed camera frames is
///   not yet sufficient for depth estimation; or depth estimation was not
///   possible due to poor lighting, camera occlusion, or insufficient motion
///   observed.
/// - @c #AR_ERROR_NOT_TRACKING The session is not in the
///   @c #AR_TRACKING_STATE_TRACKING state, which is required to acquire depth
///   images.
/// - @c #AR_ERROR_ILLEGAL_STATE if a supported depth mode was not enabled in
///   Session configuration.
/// - @c #AR_ERROR_RESOURCE_EXHAUSTED if the caller app has exceeded maximum
///   number of depth images that it can hold without releasing.
/// - @c #AR_ERROR_DEADLINE_EXCEEDED if the provided @c ::ArFrame is not the
///   current one.
ArStatus ArFrame_acquireRawDepthImage16Bits(const ArSession *session,
                                            const ArFrame *frame,
                                            ArImage **out_depth_image);

/// @ingroup ArFrame
/// Attempts to acquire the confidence image corresponding to the raw depth
/// image of the current frame.
///
/// The image must be released via @c ::ArImage_release once it is no longer
/// needed.
///
/// Each pixel is an 8-bit unsigned integer representing the estimated
/// confidence of the corresponding pixel in the raw depth image. The confidence
/// value is between 0 and 255, inclusive, with 0 representing the lowest
/// confidence and 255 representing the highest confidence in the measured depth
/// value. Pixels without a valid depth estimate have a confidence value of 0
/// and a corresponding depth value of 0 (see @c
/// ::ArFrame_acquireRawDepthImage16Bits).
///
/// The scaling of confidence values is linear and continuous within this range.
/// Expect to see confidence values represented across the full range of 0 to
/// 255, with values increasing as better observations are made of each
/// location.  If an application requires filtering out low-confidence pixels,
/// removing depth pixels below a confidence threshold of half confidence (128)
/// tends to work well.
///
/// The actual size of the depth image depends on the device and its display
/// aspect ratio. The size of the depth image is typically around 160x120
/// pixels, with higher resolutions up to 640x480 on some devices. These sizes
/// may change in the future. The outputs of
/// @c ::ArFrame_acquireDepthImage16Bits,
/// @c ::ArFrame_acquireRawDepthImage16Bits and
/// @c ::ArFrame_acquireRawDepthConfidenceImage will all have the exact same
/// size.
///
/// @param[in]  session                The ARCore session.
/// @param[in]  frame                  The current frame.
/// @param[out] out_confidence_image   On successful return, this is filled out
///   with a pointer to an @c ::ArImage. On error return, this is filled out
///   filled out with @c nullptr.
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_INVALID_ARGUMENT if the session, frame, or depth image
///   arguments are invalid.
/// - @c #AR_ERROR_NOT_YET_AVAILABLE if the number of observed camera frames is
///   not yet sufficient for depth estimation; or depth estimation was not
///   possible due to poor lighting, camera occlusion, or insufficient motion
///   observed.
/// - @c #AR_ERROR_NOT_TRACKING The session is not in the
///   @c #AR_TRACKING_STATE_TRACKING state, which is required to acquire depth
///   images.
/// - @c #AR_ERROR_ILLEGAL_STATE if a supported depth mode was not enabled in
///   Session configuration.
/// - @c #AR_ERROR_RESOURCE_EXHAUSTED if the caller app has exceeded maximum
///   number of depth images that it can hold without releasing.
/// - @c #AR_ERROR_DEADLINE_EXCEEDED if the provided @c ::ArFrame is not the
///   current one.
ArStatus ArFrame_acquireRawDepthConfidenceImage(const ArSession *session,
                                                const ArFrame *frame,
                                                ArImage **out_confidence_image);

/// @ingroup ArFrame
/// Attempts to acquire the semantic image corresponding to the current frame.
/// Each pixel in the image is an 8-bit unsigned integer representing a semantic
/// class label: see @c ::ArSemanticLabel for a list of pixel labels and the
/// <a href="https://developers.google.com/ar/develop/c/scene-semantics">Scene
/// Semantics Developer Guide</a> for more information.
///
/// The image must be released via @c ::ArImage_release once it is no longer
/// needed.
///
/// In order to obtain a valid result from this function, you must set the
/// session's @c ::ArSemanticMode to @c #AR_SEMANTIC_MODE_ENABLED. Use
/// @c ::ArSession_isSemanticModeSupported to query for support for Scene
/// Semantics.
///
/// The width of the semantic image is currently 256 pixels. The height of the
/// image depends on the device and will match its display aspect ratio.
///
/// @param[in]  session                The ARCore session.
/// @param[in]  frame                  The current frame.
/// @param[out] out_semantic_image     On successful return, this is filled out
///   with a pointer to an @c ::ArImage formatted as UINT8, where each pixel
///   denotes the semantic class. On error return, this is filled out
///   with @c NULL.
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_INVALID_ARGUMENT if the @p session, @p frame, or
///   @p out_semantic_image arguments are invalid.
/// - @c #AR_ERROR_NOT_YET_AVAILABLE if no semantic image is available
///   that corresponds to the frame.
/// - @c #AR_ERROR_RESOURCE_EXHAUSTED if the caller app has exceeded maximum
///   number of images that it can hold without releasing.
/// - @c #AR_ERROR_DEADLINE_EXCEEDED if the provided @c ::ArFrame is not the
///   current one.
ArStatus ArFrame_acquireSemanticImage(const ArSession *session,
                                      const ArFrame *frame,
                                      ArImage **out_semantic_image);

/// @ingroup ArFrame
/// Attempts to acquire the semantic confidence image corresponding to the
/// current frame. Each pixel is an 8-bit integer representing the estimated
/// confidence of the corresponding pixel in the semantic image. See the
/// <a href="https://developers.google.com/ar/develop/c/scene-semantics">Scene
/// Semantics Developer Guide</a> for more information.
///
/// The confidence value is between 0 and 255, inclusive, with 0 representing
/// the lowest confidence and 255 representing the highest confidence in the
/// semantic class prediction (see @c ::ArFrame_acquireSemanticImage).
///
/// The image must be released via @c ::ArImage_release once it is no longer
/// needed.
///
/// In order to obtain a valid result from this function, you must set the
/// session's @c ::ArSemanticMode to @c #AR_SEMANTIC_MODE_ENABLED. Use
/// @c ::ArSession_isSemanticModeSupported to query for support for Scene
/// Semantics.
///
/// The size of the semantic confidence image is the same size as the image
/// obtained by @c ::ArFrame_acquireSemanticImage.
///
/// @param[in]  session                          The ARCore session.
/// @param[in]  frame                            The current frame.
/// @param[out] out_semantic_confidence_image    On successful return, this is
///   filled out with a pointer to an @c ::ArImage, where each pixel denotes the
///   confidence corresponding to the semantic label. On error return, this is
///   filled out with @c nullptr.
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_INVALID_ARGUMENT if the @p session, @p frame, or
///   @p out_semantic_confidence_image arguments are invalid.
/// - @c #AR_ERROR_NOT_YET_AVAILABLE if no semantic image is available
///   that corresponds to the frame.
/// - @c #AR_ERROR_RESOURCE_EXHAUSTED if the caller app has exceeded maximum
///   number of images that it can hold without releasing.
/// - @c #AR_ERROR_DEADLINE_EXCEEDED if the provided @c ::ArFrame is not the
///   current one.
ArStatus ArFrame_acquireSemanticConfidenceImage(
    const ArSession *session,
    const ArFrame *frame,
    ArImage **out_semantic_confidence_image);

/// @ingroup ArFrame
/// Retrieves the fraction of the most recent semantics frame that are @p
/// query_label.
///
/// Queries the semantic image provided by @c ::ArFrame_acquireSemanticImage for
/// pixels labeled by @p query_label. This call is more efficient than
/// retrieving the @c ArImage and performing a pixel-wise search for the
/// detected labels.
///
/// @param[in]  session                          The ARCore session.
/// @param[in]  frame                            The current frame.
/// @param[in]  query_label                      The label to search for within
///   the semantic image for this frame.
/// @param[out] out_fraction                     The fraction of pixels in the
///   most recent semantic image that contain the query label.  This value is in
///   the range 0 to 1.  If no pixels are present with that label, or if an
///   invalid label is provided, this call returns 0.
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_INVALID_ARGUMENT if @p session, @p frame, or @p query_label
///   are invalid.
/// - @c #AR_ERROR_NOT_YET_AVAILABLE if no semantic image has been generated
///   yet.
ArStatus ArFrame_getSemanticLabelFraction(const ArSession *session,
                                          const ArFrame *frame,
                                          ArSemanticLabel query_label,
                                          float *out_fraction);

/// @ingroup ArFrame
/// Gets the <a
/// href="https://developer.android.com/ndk/reference/group/a-hardware-buffer">@c
/// AHardwareBuffer </a> for this frame. See <a
/// href="https://developers.google.com/ar/develop/c/vulkan">Vulkan Rendering
/// developer guide</a> for more information.
///
/// The result in @p out_hardware_buffer is
/// only valid when a configuration is active that uses @c
/// #AR_TEXTURE_UPDATE_MODE_EXPOSE_HARDWARE_BUFFER.
///
/// This hardware buffer is only guaranteed to be valid until the next call to
/// @c ::ArSession_update(). If you want to use the hardware buffer beyond that,
/// such as for rendering, you must call <a
/// href="https://developer.android.com/ndk/reference/group/a-hardware-buffer#ahardwarebuffer_acquire">@c
/// AHardwareBuffer_acquire </a> and then call <a
/// href="https://developer.android.com/ndk/reference/group/a-hardware-buffer#ahardwarebuffer_release">@c
/// AHardwareBuffer_release </a> after your rendering is complete.
///
/// @param[in]    session  The ARCore session.
/// @param[in]    frame    The current frame.
/// @param[out]   out_hardware_buffer The destination @c AHardwareBuffer
///              representing a memory chunk of a camera image.
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_INVALID_ARGUMENT - one or more input arguments are invalid.
/// - @c #AR_ERROR_DEADLINE_EXCEEDED - the input frame is not the current frame.
/// - @c #AR_ERROR_NOT_YET_AVAILABLE - the camera failed to produce the image.
ArStatus ArFrame_getHardwareBuffer(const ArSession *session,
                                   const ArFrame *frame,
                                   void **out_hardware_buffer);

/// @ingroup ArFrame
/// Returns the OpenGL ES camera texture name (ID) associated with this frame.
/// This is guaranteed to be one of the texture names previously set via
/// @c ::ArSession_setCameraTextureNames or @c ::ArSession_setCameraTextureName.
/// Texture names (IDs) are returned in a round robin fashion in sequential
/// frames.
///
/// @param[in]   session         The ARCore session.
/// @param[in]   frame           The current frame.
/// @param[out]  out_texture_id  Where to store the texture name (ID).
void ArFrame_getCameraTextureName(const ArSession *session,
                                  const ArFrame *frame,
                                  uint32_t *out_texture_id);

/// @ingroup ArMesh
/// Releases a reference to the Mesh.
/// This must match a call to @c ::ArStreetscapeGeometry_acquireMesh.
///
/// This function may safely be called with @c NULL - it will do nothing.
void ArMesh_release(ArMesh *mesh);

/// @ingroup ArMesh
/// Retrieves the number of vertices in this mesh.
///
/// @param[in]  session                   The ARCore session.
/// @param[in]  mesh                      The mesh object to query.
/// @param[out] out_num_vertices          The number of vertices in this mesh.
void ArMesh_getVertexListSize(const ArSession *session,
                              const ArMesh *mesh,
                              int32_t *out_num_vertices);

/// @ingroup ArMesh
/// Retrieves the vertex coordinate data for this mesh. Each vertex is three
/// @c float values, stored in XYZ order. The total number of @c float values in
/// @p out_vertex_positions_xyz is 3 * @c ::ArMesh_getVertexListSize.
///
/// The array returned in @p out_vertex_positions_xyz will only remain valid as
/// long as @p mesh is valid.
///
/// @param[in]  session                   The ARCore session.
/// @param[in]  mesh                      The mesh object to query.
/// @param[out] out_vertex_positions_xyz  Where to store the vertex positions
///                                       positions.
void ArMesh_getVertexList(const ArSession *session,
                          const ArMesh *mesh,
                          const float **out_vertex_positions_xyz);

/// @ingroup ArMesh
/// Retrieves the number of triangle indices (represented by @c
/// ::ArMesh_getIndexList) in this mesh. The indices should always be used as a
/// triangle list, with three indices per triangle. The result can be passed as
/// the @c indices parameter to <a
/// href="https://registry.khronos.org/OpenGL-Refpages/es3.0/html/glDrawElements.xhtml">@c
/// glDrawElements </a> or copied into a buffer bound via <a
/// href="https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCmdDrawIndexed.html">@c
/// vkCmdDrawIndexed </a>.
///
/// @param[in]  session                The ARCore session.
/// @param[in]  mesh                   The mesh object to query.
/// @param[out] out_num_indices        The number of indices in this mesh. This
/// will always be a multiple of 3.
void ArMesh_getIndexListSize(const ArSession *session,
                             const ArMesh *mesh,
                             int32_t *out_num_indices);

/// @ingroup ArMesh
/// Retrieves the triangle data for this mesh in an array of length @c
/// ::ArMesh_getIndexListSize. Each face is a triplet of indices into the
/// vertices array. The indices should always be used as a triangle list, with
/// three indices per triangle. The result can be passed as the "indices"
/// parameter to @c glDrawElements or copied into a buffer bound via
/// @c vkCmdBindIndexBuffer.
///
/// @param[in]  session                The ARCore session.
/// @param[in]  mesh                   The mesh object to query.
/// @param[out] out_indices            Where to store the concatenated indices
///                                    of mesh vertices that correspond to each
///                                    face.  This length is @c out_num_indices
///                                    from @c ::ArMesh_getIndexListSize().
void ArMesh_getIndexList(const ArSession *session,
                         const ArMesh *mesh,
                         const uint32_t **out_indices);

// === ArPointCloud functions ===

/// @ingroup ArPointCloud
/// Retrieves the number of points in the Point Cloud.
///
void ArPointCloud_getNumberOfPoints(const ArSession *session,
                                    const ArPointCloud *point_cloud,
                                    int32_t *out_number_of_points);

/// @ingroup ArPointCloud
/// Retrieves a pointer to the Point Cloud data.
///
/// Each point is represented by four consecutive values in the array; first the
/// X, Y, Z position coordinates, followed by a confidence value. This is the
/// same format as described in <a
/// href="https://developer.android.com/reference/android/graphics/ImageFormat.html#DEPTH_POINT_CLOUD"
/// >DEPTH_POINT_CLOUD</a>.
///
/// The pointer returned by this function is valid until
/// @c ::ArPointCloud_release is called. If the number of points is zero, then
/// the value of
/// @p *out_point_cloud_data is undefined.
///
/// If your app needs to keep some Point Cloud data, for example to compare
/// Point Cloud data frame to frame, consider copying just the data points your
/// app needs, and then calling @c ::ArPointCloud_release to reduce the amount
/// of memory required.
void ArPointCloud_getData(const ArSession *session,
                          const ArPointCloud *point_cloud,
                          const float **out_point_cloud_data);

/// @ingroup ArPointCloud
/// Retrieves a pointer to the Point Cloud point IDs. The number of IDs is the
/// same as number of points, and is given by
/// @c ::ArPointCloud_getNumberOfPoints.
///
/// Each point has a unique identifier (within a session) that is persistent
/// across frames. That is, if a point from Point Cloud 1 has the same id as the
/// point from Point Cloud 2, then it represents the same point in space.
///
/// The pointer returned by this function is valid until
/// @c ::ArPointCloud_release is called. If the number of points is zero, then
/// the value of
/// @p *out_point_ids is undefined.
///
/// If your app needs to keep some Point Cloud data, for example to compare
/// Point Cloud data frame to frame, consider copying just the data points your
/// app needs, and then calling @c ::ArPointCloud_release to reduce the amount
/// of memory required.
void ArPointCloud_getPointIds(const ArSession *session,
                              const ArPointCloud *point_cloud,
                              const int32_t **out_point_ids);

/// @ingroup ArPointCloud
/// Returns the timestamp in nanoseconds when this Point Cloud was observed.
/// This timestamp uses the same time base as @c ::ArFrame_getTimestamp.
void ArPointCloud_getTimestamp(const ArSession *session,
                               const ArPointCloud *point_cloud,
                               int64_t *out_timestamp_ns);

/// @ingroup ArPointCloud
/// Releases a reference to the Point Cloud.  This must match a call to
/// @c ::ArFrame_acquirePointCloud.
///
/// This function may safely be called with @c NULL - it will do nothing.
void ArPointCloud_release(ArPointCloud *point_cloud);

// === Image Metadata functions ===

/// @ingroup ArImageMetadata
/// Releases a reference to the metadata.  This must match a call to
/// @c ::ArFrame_acquireImageMetadata.
///
/// This function may safely be called with @c NULL - it will do nothing.
void ArImageMetadata_release(ArImageMetadata *metadata);

/// @ingroup ArImage
/// Image formats produced by ARCore.
AR_DEFINE_ENUM(ArImageFormat){
    /// Invalid image format. Produced by ARCore when an invalid session/image
    /// is given to @c ::ArImage_getFormat.
    AR_IMAGE_FORMAT_INVALID = 0,

    /// Produced by @c ::ArFrame_acquireCameraImage.
    /// Integer value equal to Android NDK @c <a
    /// href="https://developer.android.com/ndk/reference/group/media#group___media_1gga9c3dace30485a0f28163a882a5d65a19aea9797f9b5db5d26a2055a43d8491890">AIMAGE_FORMAT_YUV_420_888</a>
    /// and @c <a
    /// href="https://developer.android.com/reference/android/graphics/ImageFormat.html#YUV_420_888">YUV_420_888</a>.
    AR_IMAGE_FORMAT_YUV_420_888 = 0x23,

    /// Produced by @c ::ArFrame_acquireDepthImage and
    /// @c ::ArFrame_acquireRawDepthImage.
    /// Integer value equal to <a
    /// href="https://developer.android.com/reference/android/graphics/ImageFormat.html#DEPTH16">DEPTH16</a>.
    AR_IMAGE_FORMAT_DEPTH16 = 0x44363159,

    /// Produced by @c ::ArFrame_acquireDepthImage16Bits and
    /// @c ::ArFrame_acquireRawDepthImage16Bits.
    /// Depth range values in units of millimeters.
    /// Integer value equal to <a
    /// href="https://developer.android.com/reference/android/hardware/HardwareBuffer#D_16">D_16</a>.
    AR_IMAGE_FORMAT_D_16 = 0x00000030,

    /// Produced by @c ::ArFrame_acquireRawDepthConfidenceImage. Integer value
    /// equal to @c <a
    /// href="https://developer.android.com/reference/android/graphics/ImageFormat.html#Y8">Y8</a>.
    AR_IMAGE_FORMAT_Y8 = 0x20203859,

    /// Produced by @c ::ArLightEstimate_acquireEnvironmentalHdrCubemap.
    /// Integer value equal to Android NDK @c <a
    /// href="https://developer.android.com/ndk/reference/group/media#group___media_1gga9c3dace30485a0f28163a882a5d65a19aa0f5b9a07c9f3dc8a111c0098b18363a">AIMAGE_FORMAT_RGBA_FP16</a>.
    AR_IMAGE_FORMAT_RGBA_FP16 = 0x16,
};

/// @ingroup ArImage
/// Gets the width of the input @c ::ArImage.
///
/// @param[in]    session                The ARCore session.
/// @param[in]    image                  The @c ::ArImage of interest.
/// @param[inout] out_width              The width of the image in pixels.
void ArImage_getWidth(const ArSession *session,
                      const ArImage *image,
                      int32_t *out_width);

/// @ingroup ArImage
/// Gets the height of the input @c ::ArImage.
///
/// @param[in]    session                The ARCore session.
/// @param[in]    image                  The @c ::ArImage of interest.
/// @param[inout] out_height             The height of the image in pixels.
void ArImage_getHeight(const ArSession *session,
                       const ArImage *image,
                       int32_t *out_height);

/// @ingroup ArImage
/// Gets the source-specific timestamp of the provided @c ::ArImage in
/// nanoseconds. The timestamp is normally monotonically increasing. The
/// timestamps for the images from different sources may have different
/// timebases and should not be compared with each other. The specific meaning
/// and timebase of the returned timestamp depends on the source providing
/// images.
///
/// @param[in]    session                The ARCore session.
/// @param[in]    image                  The @c ::ArImage of interest.
/// @param[inout] out_timestamp_ns       The timestamp of the image in
///                                      nanoseconds.
void ArImage_getTimestamp(const ArSession *session,
                          const ArImage *image,
                          int64_t *out_timestamp_ns);

/// @ingroup ArImage
/// Gets the image format of the provided @c ::ArImage.
///
/// @param[in]    session                The ARCore session.
/// @param[in]    image                  The @c ::ArImage of interest.
/// @param[inout] out_format             The image format, one of
/// @c ::ArImageFormat values.
void ArImage_getFormat(const ArSession *session,
                       const ArImage *image,
                       ArImageFormat *out_format);

/// @ingroup ArImage
/// Gets the number of planes in the provided @c ::ArImage. The number of planes
/// and format of data in each plane is format dependent. Use
/// @c ::ArImage_getFormat to determine the format.
///
/// @param[in]    session                The ARCore session.
/// @param[in]    image                  The @c ::ArImage of interest.
/// @param[inout] out_num_planes         The number of planes in the image.
void ArImage_getNumberOfPlanes(const ArSession *session,
                               const ArImage *image,
                               int32_t *out_num_planes);

/// @ingroup ArImage
/// Gets the byte distance between the start of two consecutive pixels in
/// the image. The pixel stride is always greater than 0.
///
/// @param[in]    session                The ARCore session.
/// @param[in]    image                  The @c ::ArImage of interest.
/// @param[in]    plane_index            The index of the plane, between 0 and
///     n-1, where n is number of planes for this image.
/// @param[inout] out_pixel_stride       The plane stride of the image in bytes.
void ArImage_getPlanePixelStride(const ArSession *session,
                                 const ArImage *image,
                                 int32_t plane_index,
                                 int32_t *out_pixel_stride);

/// @ingroup ArImage
/// Gets the number of bytes between the start of two consecutive rows of pixels
/// in the image. The row stride is always greater than 0.
///
/// @param[in]    session                The ARCore session.
/// @param[in]    image                  The @c ::ArImage of interest.
/// @param[in]    plane_index            The index of the plane, between 0 and
///     n-1, where n is number of planes for this image.
/// @param[inout] out_row_stride         The row stride of the image in bytes.
void ArImage_getPlaneRowStride(const ArSession *session,
                               const ArImage *image,
                               int32_t plane_index,
                               int32_t *out_row_stride);

/// @ingroup ArImage
/// Gets the data pointer of the provided image for direct application access.
/// Note that once the @c ::ArImage data is released with @c ::ArImage_release,
/// the data pointer from the corresponding @c ::ArImage_getPlaneData call
/// becomes invalid. Do NOT use it after the @c ::ArImage is released.
///
/// @param[in]    session                The ARCore session.
/// @param[in]    image                  The @c ::ArImage of interest.
/// @param[in]    plane_index            The index of the plane, between 0 and
///     n-1, where n is number of planes for this image.
/// @param[inout] out_data               The data pointer to the image.
/// @param[inout] out_data_length        The length of data in bytes.
void ArImage_getPlaneData(const ArSession *session,
                          const ArImage *image,
                          int32_t plane_index,
                          const uint8_t **out_data,
                          int32_t *out_data_length);

/// @ingroup ArImage
/// Releases an instance of @c ::ArImage returned by
/// @c ::ArFrame_acquireCameraImage.
void ArImage_release(ArImage *image);

// === ArLightEstimate functions ===

/// @ingroup ArLightEstimate
/// Allocates an @c ::ArLightEstimate object.
void ArLightEstimate_create(const ArSession *session,
                            ArLightEstimate **out_light_estimate);

/// @ingroup ArLightEstimate
/// Releases the provided @c ::ArLightEstimate object.
void ArLightEstimate_destroy(ArLightEstimate *light_estimate);

/// @ingroup ArLightEstimate
/// Retrieves the validity state of an @c ::ArLightEstimate.  If the resulting
/// value of
/// @p *out_light_estimate_state is not @c #AR_LIGHT_ESTIMATE_STATE_VALID, the
/// estimate should not be used for rendering.
void ArLightEstimate_getState(const ArSession *session,
                              const ArLightEstimate *light_estimate,
                              ArLightEstimateState *out_light_estimate_state);

/// @ingroup ArLightEstimate
/// Retrieves the pixel intensity, in gamma color space, of the current camera
/// view. Values are in the range [0.0, 1.0], with zero being black and one
/// being white. If @c #AR_LIGHT_ESTIMATION_MODE_AMBIENT_INTENSITY mode is not
/// set, returns zero.
///
/// If rendering in gamma color space, divide this value by 0.466, which is
/// middle gray in gamma color space, and multiply against the final calculated
/// color after rendering. If rendering in linear space, first convert this
/// value to linear space by rising to the power 2.2. Normalize the result by
/// dividing it by 0.18 which is middle gray in linear space. Then multiply by
/// the final calculated color after rendering.
void ArLightEstimate_getPixelIntensity(const ArSession *session,
                                       const ArLightEstimate *light_estimate,
                                       float *out_pixel_intensity);

/// @ingroup ArLightEstimate
/// Gets the color correction values that are uploaded to the fragment shader.
/// Use the RGB scale factors (components 0-2) to match the color of the light
/// in the scene. Use the pixel intensity (component 3) to match the intensity
/// of the light in the scene. If @c #AR_LIGHT_ESTIMATION_MODE_AMBIENT_INTENSITY
/// mode is not set, returns all zeros.
///
/// `out_color_correction_4` components are:
///   - `[0]` Red channel scale factor. This value is larger or equal to zero.
///   - `[1]` Green channel scale factor. This value is always 1.0 as the green
///           channel is the reference baseline.
///   - `[2]` Blue channel scale factor. This value is larger or equal to zero.
///   - `[3]` Pixel intensity. This is the same value as the one return from
///       @c ::ArLightEstimate_getPixelIntensity.
///
///  The RGB scale factors can be used independently from the pixel intensity
///  value. They are put together for the convenience of only having to upload
///  one float4 to the fragment shader.
///
///  The RGB scale factors are not intended to brighten nor dim the scene.  They
///  are only to shift the color of the virtual object towards the color of the
///  light; not intensity of the light. The pixel intensity is used to match the
///  intensity of the light in the scene.
///
///  Color correction values are reported in gamma color space.
///  If rendering in gamma color space, multiply them component-wise against the
///  final calculated color after rendering. If rendering in linear space, first
///  convert the values to linear space by rising to the power 2.2. Then
///  multiply component-wise against the final calculated color after rendering.
void ArLightEstimate_getColorCorrection(const ArSession *session,
                                        const ArLightEstimate *light_estimate,
                                        float *out_color_correction_4);

/// @ingroup ArLightEstimate
/// Returns the timestamp of the given @c ::ArLightEstimate in nanoseconds.
////
/// ARCore returns a different timestamp when the underlying light estimate has
/// changed. Conversely, the same timestamp is returned if the light estimate
/// has not changed.
///
/// This timestamp uses the same time base as @c ::ArFrame_getTimestamp.
void ArLightEstimate_getTimestamp(const ArSession *session,
                                  const ArLightEstimate *light_estimate,
                                  int64_t *out_timestamp_ns);

/// @ingroup ArLightEstimate
/// Returns the direction of the main directional light based on the inferred
/// Environmental HDR Lighting Estimation. If
/// @c #AR_LIGHT_ESTIMATION_MODE_ENVIRONMENTAL_HDR mode is not set, returns
/// [0.0, 1.0, 0.0], representing a light shining straight down from above.
/// @param[in]    session           The ARCore session.
/// @param[in]    light_estimate    The ArLightEstimate.
/// @param[out]   out_direction_3   Output lighting direction.
///   This array stores the normalized output lighting direction as 3 floats [x,
///   y, z].
void ArLightEstimate_getEnvironmentalHdrMainLightDirection(
    const ArSession *session,
    const ArLightEstimate *light_estimate,
    float *out_direction_3);

/// @ingroup ArLightEstimate
/// Returns the intensity of the main directional light based on the inferred
/// Environmental HDR Lighting Estimation. All return values are larger or equal
/// to zero. If @c #AR_LIGHT_ESTIMATION_MODE_ENVIRONMENTAL_HDR mode is not set,
/// returns zero for all elements of the array.
/// @param[in]    session           The ARCore session.
/// @param[in]    light_estimate    The ArLightEstimate.
/// @param[out]   out_intensity_3   Output lighting intensity.
///   This array stores the output lighting intensity as 3 floats [r, g, b].
void ArLightEstimate_getEnvironmentalHdrMainLightIntensity(
    const ArSession *session,
    const ArLightEstimate *light_estimate,
    float *out_intensity_3);

/// @ingroup ArLightEstimate
/// Gets the spherical harmonics coefficients for the ambient illumination based
/// on the inferred Environmental HDR Lighting Estimation.
/// @param[in]    session              The ARCore session.
/// @param[in]    light_estimate       The ArLightEstimate.
/// @param[out]   out_coefficients_27  The output spherical harmonics
///    coefficients for the ambient illumination. This array contains 9 sets of
///    per-channel coefficients, or a total of 27 values of 32-bit floating
///    point type. The coefficients are stored in a channel-major fashion e.g.
///    [r0, g0, b0, r1, g1, b1, ... , r8, g8, b8]. If
///    @c #AR_LIGHT_ESTIMATION_MODE_ENVIRONMENTAL_HDR mode is not set, returns
///    zero for all 27 coefficients.
void ArLightEstimate_getEnvironmentalHdrAmbientSphericalHarmonics(
    const ArSession *session,
    const ArLightEstimate *light_estimate,
    float *out_coefficients_27);

/// @ingroup ArLightEstimate
/// Gets the 6 cubemap textures in OpenGL texture format based on the inferred
/// Environmental HDR lighting.
/// @param[in]    session          The ARCore session.
/// @param[in]    light_estimate   The ArLightEstimate.
/// @param[out]   out_textures_6   The fixed size array for 6 cubemap textures.
///                                @c ::ArImageCubemap type has been created to
///                                facilitate representing the array of
///                                @c ::ArImage pointers.
/// out_textures_6 contains 6 images in @c AIMAGE_FORMAT_RGBA_FP16 format for
/// the HDR cubemap. The memory layout for the image data is identical to
/// @c GL_RGBA16F. The pixel values are in linear color space. The order of the
/// images corresponds to the cubemap order as follows:
///   out_textures_6[0]: GL_TEXTURE_CUBE_MAP_POSITIVE_X
///   out_textures_6[1]: GL_TEXTURE_CUBE_MAP_NEGATIVE_X
///   out_textures_6[2]: GL_TEXTURE_CUBE_MAP_POSITIVE_Y
///   out_textures_6[3]: GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
///   out_textures_6[4]: GL_TEXTURE_CUBE_MAP_POSITIVE_Z
///   out_textures_6[5]: GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
/// If @c #AR_LIGHT_ESTIMATION_MODE_ENVIRONMENTAL_HDR mode is not set, all
/// textures will be assigned with zero pixel values. All 6 acquired images must
/// be released with @c ::ArImage_release once they are no longer needed.
void ArLightEstimate_acquireEnvironmentalHdrCubemap(
    const ArSession *session,
    const ArLightEstimate *light_estimate,
    ArImageCubemap out_textures_6);

// === ArAnchorList functions ===

/// @ingroup ArAnchor
/// Creates an anchor list object.
void ArAnchorList_create(const ArSession *session,
                         ArAnchorList **out_anchor_list);

/// @ingroup ArAnchor
/// Releases the memory used by an anchor list object, along with all the anchor
/// references it holds.
void ArAnchorList_destroy(ArAnchorList *anchor_list);

/// @ingroup ArAnchor
/// Retrieves the number of anchors in this list.
void ArAnchorList_getSize(const ArSession *session,
                          const ArAnchorList *anchor_list,
                          int32_t *out_size);

/// @ingroup ArAnchor
/// Acquires a reference to an indexed entry in the list.  This call must
/// eventually be matched with a call to @c ::ArAnchor_release.
void ArAnchorList_acquireItem(const ArSession *session,
                              const ArAnchorList *anchor_list,
                              int32_t index,
                              ArAnchor **out_anchor);

// === ArAnchor functions ===

/// @ingroup ArAnchor
/// Retrieves the pose of the anchor in the world coordinate space. This pose
/// produced by this call may change each time @c ::ArSession_update is called.
/// This pose should only be used for rendering if
/// @c ::ArAnchor_getTrackingState returns @c #AR_TRACKING_STATE_TRACKING.
///
/// @param[in]    session  The ARCore session.
/// @param[in]    anchor   The anchor to retrieve the pose of.
/// @param[inout] out_pose An already-allocated @c ::ArPose object into which
///     the pose will be stored.
void ArAnchor_getPose(const ArSession *session,
                      const ArAnchor *anchor,
                      ArPose *out_pose);

/// @ingroup ArAnchor
/// Retrieves the current state of the pose of this anchor.
///
/// Note: Starting in ARCore 1.12, changing the active camera config using
/// @c ::ArSession_setCameraConfig may cause the tracking state on certain
/// devices to become permanently @c #AR_TRACKING_STATE_PAUSED. For consistent
/// behavior across all supported devices, release any previously created
/// anchors and trackables when setting a new camera config.
void ArAnchor_getTrackingState(const ArSession *session,
                               const ArAnchor *anchor,
                               ArTrackingState *out_tracking_state);

/// @ingroup ArAnchor
/// Tells ARCore to stop tracking and forget this anchor.  This call does not
/// release any references to the anchor - that must be done separately using
/// @c ::ArAnchor_release.
void ArAnchor_detach(ArSession *session, ArAnchor *anchor);

/// @ingroup ArAnchor
/// Releases a reference to an anchor. To stop tracking for this anchor, call
/// @c ::ArAnchor_detach first.
///
/// This function may safely be called with @c NULL - it will do nothing.
void ArAnchor_release(ArAnchor *anchor);

/// @ingroup ArAnchor
/// Acquires the Cloud Anchor ID of the anchor. The ID acquired is an ASCII
/// null-terminated string. The acquired ID must be released after use by the
/// @c ::ArString_release function. For anchors with cloud state
/// @c #AR_CLOUD_ANCHOR_STATE_NONE or @c
/// #AR_CLOUD_ANCHOR_STATE_TASK_IN_PROGRESS, this will always be an empty
/// string.
///
/// @param[in]    session             The ARCore session.
/// @param[in]    anchor              The anchor to retrieve the cloud ID of.
/// @param[inout] out_cloud_anchor_id A pointer to the acquired ID string.
/// @deprecated Use @c ::ArHostCloudAnchorFuture_acquireResultCloudAnchorId
/// instead.
void ArAnchor_acquireCloudAnchorId(ArSession *session,
                                   ArAnchor *anchor,
                                   char **out_cloud_anchor_id)
    AR_DEPRECATED(
        "For anchors hosted using ArSession_hostCloudAnchorAsync, the cloud "
        "anchor ID can be obtained from the callback or future object. This "
        "function will always return the empty string except for anchors "
        "created using the deprecated functions "
        "ArSession_resolveAndAcquireNewCloudAnchor, "
        "ArSession_hostAndAcquireNewCloudAnchor, and "
        "ArSession_hostAndAcquireNewCloudAnchorWithTtl.");

/// @ingroup ArAnchor
/// Gets the current Cloud Anchor state of the anchor. This state is guaranteed
/// not to change until @c ::ArSession_update is called.
///
/// @param[in]    session   The ARCore session.
/// @param[in]    anchor    The anchor to retrieve the cloud state of.
/// @param[inout] out_state The current cloud state of the anchor.
/// @deprecated Use @c ::ArHostCloudAnchorFuture_getResultCloudAnchorState or
/// @c ::ArResolveCloudAnchorFuture_getResultCloudAnchorState instead.
void ArAnchor_getCloudAnchorState(const ArSession *session,
                                  const ArAnchor *anchor,
                                  ArCloudAnchorState *out_state)
    AR_DEPRECATED(
        "For anchors hosted or resolved using async cloud anchor APIs, the "
        "state can be obtained from the callback or future object. This "
        "function will always return AR_CLOUD_ANCHOR_STATE_NONE except for "
        "anchors created using the deprecated functions "
        "ArSession_resolveAndAcquireNewCloudAnchor, "
        "ArSession_hostAndAcquireNewCloudAnchor, and "
        "ArSession_hostAndAcquireNewCloudAnchorWithTtl.");

// === ArTrackableList functions ===

/// @ingroup ArTrackable
/// Creates a trackable list object.
void ArTrackableList_create(const ArSession *session,
                            ArTrackableList **out_trackable_list);

/// @ingroup ArTrackable
/// Releases the memory used by a trackable list object, along with all the
/// anchor references it holds.
void ArTrackableList_destroy(ArTrackableList *trackable_list);

/// @ingroup ArTrackable
/// Retrieves the number of trackables in this list.
void ArTrackableList_getSize(const ArSession *session,
                             const ArTrackableList *trackable_list,
                             int32_t *out_size);

/// @ingroup ArTrackable
/// Acquires a reference to an indexed entry in the list.  This call must
/// eventually be matched with a call to @c ::ArTrackable_release.
void ArTrackableList_acquireItem(const ArSession *session,
                                 const ArTrackableList *trackable_list,
                                 int32_t index,
                                 ArTrackable **out_trackable);

// === ArTrackable functions ===

/// @ingroup ArTrackable
/// Releases a reference to a trackable. This does not mean that the trackable
/// will necessarily stop tracking. The same trackable may still be included in
/// from other calls, for example @c ::ArSession_getAllTrackables.
///
/// This function may safely be called with @c NULL - it will do nothing.
void ArTrackable_release(ArTrackable *trackable);

/// @ingroup ArTrackable
/// Retrieves the type of the trackable.  See @c ::ArTrackableType for valid
/// types.
void ArTrackable_getType(const ArSession *session,
                         const ArTrackable *trackable,
                         ArTrackableType *out_trackable_type);

/// @ingroup ArTrackable
/// Retrieves the current state of ARCore's knowledge of the pose of this
/// trackable.
///
/// Note: Starting in ARCore 1.12, changing the active camera config using
/// @c ::ArSession_setCameraConfig may cause the tracking state on certain
/// devices to become permanently @c #AR_TRACKING_STATE_PAUSED. For consistent
/// behavior across all supported devices, release any previously created
/// trackables when setting a new camera config.
void ArTrackable_getTrackingState(const ArSession *session,
                                  const ArTrackable *trackable,
                                  ArTrackingState *out_tracking_state);

/// @ingroup ArTrackable
/// Creates an Anchor at the given pose in the world coordinate space, attached
/// to this Trackable, and acquires a reference to it. The type of Trackable
/// will determine the semantics of attachment and how the Anchor's pose will be
/// updated to maintain this relationship. Note that the relative offset between
/// the pose of multiple Anchors attached to a Trackable may adjust slightly
/// over time as ARCore updates its model of the world.
///
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_NOT_TRACKING if the trackable's tracking state was not
///   @c #AR_TRACKING_STATE_TRACKING
/// - @c #AR_ERROR_SESSION_PAUSED if the session was paused
/// - @c #AR_ERROR_RESOURCE_EXHAUSTED if too many anchors exist
/// - @c #AR_ERROR_ILLEGAL_STATE if this trackable doesn't support anchors
ArStatus ArTrackable_acquireNewAnchor(ArSession *session,
                                      ArTrackable *trackable,
                                      ArPose *pose,
                                      ArAnchor **out_anchor);

/// @ingroup ArTrackable
/// Gets the set of anchors attached to this trackable.
///
/// @param[in]    session         The ARCore session
/// @param[in]    trackable       The trackable to query the anchors of.
/// @param[inout] out_anchor_list The list to fill.  This list must have
///     already been allocated with @c ::ArAnchorList_create.  If previously
///     used, the list will first be cleared.
void ArTrackable_getAnchors(const ArSession *session,
                            const ArTrackable *trackable,
                            ArAnchorList *out_anchor_list);

// === ArPlane functions ===

/// @ingroup ArPlane
/// Acquires a reference to the plane subsuming this plane.
///
/// Two or more planes may be automatically merged into a single parent plane,
/// resulting in this function acquiring the parent plane when called with each
/// child plane. A subsumed plane becomes identical to the parent plane, and
/// will continue behaving as if it were independently tracked, for example
/// being included in the output of @c ::ArFrame_getUpdatedTrackables.
///
/// In cases where a subsuming plane is itself subsumed, this function
/// will always return the topmost non-subsumed plane.
///
/// Note: this function will set @p *out_subsumed_by to @c NULL if the plane is
/// not subsumed.
void ArPlane_acquireSubsumedBy(const ArSession *session,
                               const ArPlane *plane,
                               ArPlane **out_subsumed_by);

/// @ingroup ArPlane
/// Retrieves the type (orientation) of the plane.  See @c ::ArPlaneType.
void ArPlane_getType(const ArSession *session,
                     const ArPlane *plane,
                     ArPlaneType *out_plane_type);

/// @ingroup ArPlane
/// Returns the pose of the center position of the plane's bounding rectangle.
/// The pose's transformed +Y axis will be a normal vector pointing out of
/// plane. The transformed +X and +Z axes represent right and up relative to the
/// plane.
///
/// @param[in]    session  The ARCore session.
/// @param[in]    plane    The plane for which to retrieve center pose.
/// @param[inout] out_pose An already-allocated @c ::ArPose object into which
///     the pose will be stored.
void ArPlane_getCenterPose(const ArSession *session,
                           const ArPlane *plane,
                           ArPose *out_pose);

/// @ingroup ArPlane
/// Retrieves the length of this plane's bounding rectangle measured along the
/// local X-axis of the coordinate space defined by the output of
/// @c ::ArPlane_getCenterPose.
void ArPlane_getExtentX(const ArSession *session,
                        const ArPlane *plane,
                        float *out_extent_x);

/// @ingroup ArPlane
/// Retrieves the length of this plane's bounding rectangle measured along the
/// local Z-axis of the coordinate space defined by the output of
/// @c ::ArPlane_getCenterPose.
void ArPlane_getExtentZ(const ArSession *session,
                        const ArPlane *plane,
                        float *out_extent_z);

/// @ingroup ArPlane
/// Retrieves the number of elements (not vertices) in the boundary polygon.
/// The number of vertices is half this size.
void ArPlane_getPolygonSize(const ArSession *session,
                            const ArPlane *plane,
                            int32_t *out_polygon_size);

/// @ingroup ArPlane
/// Returns the 2D vertices of a convex polygon approximating the detected
/// plane, in the form <tt>[x1, z1, x2, z2, ...]</tt>. These @c x and @c z
/// values are in the plane's local X-Z plane (@c y=0) and must be transformed
/// by the pose (@c ::ArPlane_getCenterPose) to get the boundary in world
/// coordinates.
///
/// @param[in]    session        The ARCore session.
/// @param[in]    plane          The plane to retrieve the polygon from.
/// @param[inout] out_polygon_xz A pointer to an array of floats.  The length of
///     this array must be at least that reported by @c
///     ::ArPlane_getPolygonSize.
void ArPlane_getPolygon(const ArSession *session,
                        const ArPlane *plane,
                        float *out_polygon_xz);

/// @ingroup ArPlane
/// Sets @p *out_pose_in_extents to non-zero if the given pose (usually obtained
/// from an @c ::ArHitResult) is in the plane's rectangular extents.
void ArPlane_isPoseInExtents(const ArSession *session,
                             const ArPlane *plane,
                             const ArPose *pose,
                             int32_t *out_pose_in_extents);

/// @ingroup ArPlane
/// Sets @p *out_pose_in_extents to non-zero if the given pose (usually obtained
/// from an @c ::ArHitResult) is in the plane's polygon.
void ArPlane_isPoseInPolygon(const ArSession *session,
                             const ArPlane *plane,
                             const ArPose *pose,
                             int32_t *out_pose_in_polygon);

// === ArPoint functions ===

/// @ingroup ArPoint
/// Returns the pose of the point.
/// If @c ::ArPoint_getOrientationMode returns
/// @c #AR_POINT_ORIENTATION_ESTIMATED_SURFACE_NORMAL, the orientation will
/// follow the behavior described in @c ::ArHitResult_getHitPose. If
/// @c ::ArPoint_getOrientationMode returns
/// @c #AR_POINT_ORIENTATION_INITIALIZED_TO_IDENTITY, then returns an
/// orientation that is identity or close to identity.
/// @param[in]    session  The ARCore session.
/// @param[in]    point    The point to retrieve the pose of.
/// @param[inout] out_pose An already-allocated @c ::ArPose object into which
///     the pose will be stored.
void ArPoint_getPose(const ArSession *session,
                     const ArPoint *point,
                     ArPose *out_pose);

/// @ingroup ArPoint
/// Returns the @c ::ArPointOrientationMode of the point. For @c ::ArPoint
/// objects created by @c ::ArFrame_hitTest. If @c ::ArPointOrientationMode is
/// @c #AR_POINT_ORIENTATION_ESTIMATED_SURFACE_NORMAL, then normal of the
/// surface centered around the @c ::ArPoint was estimated successfully.
///
/// @param[in]    session              The ARCore session.
/// @param[in]    point                The point to retrieve the pose of.
/// @param[inout] out_orientation_mode OrientationMode output result for the
///     point.
void ArPoint_getOrientationMode(const ArSession *session,
                                const ArPoint *point,
                                ArPointOrientationMode *out_orientation_mode);

/// @ingroup ArInstantPlacementPoint
/// Returns the pose of the @c ::ArInstantPlacementPoint.
/// @param[in]    session  The ARCore session.
/// @param[in]    instant_placement_point  The Instant Placement point to
///     retrieve the pose of.
/// @param[inout] out_pose An @c ::ArPose object already-allocated via
///     @c ::ArPose_create into which the pose will be stored.
void ArInstantPlacementPoint_getPose(
    const ArSession *session,
    const ArInstantPlacementPoint *instant_placement_point,
    ArPose *out_pose);

/// @ingroup ArInstantPlacementPoint
/// Returns the tracking method of the @c ::ArInstantPlacementPoint.
/// @param[in]    session  The ARCore session.
/// @param[in]    instant_placement_point  The Instant Placement point to
///     retrieve the tracking method of.
/// @param[inout] out_tracking_method An already-allocated
///     @c ::ArInstantPlacementPointTrackingMethod object into which the
///     tracking  method will be stored.
void ArInstantPlacementPoint_getTrackingMethod(
    const ArSession *session,
    const ArInstantPlacementPoint *instant_placement_point,
    ArInstantPlacementPointTrackingMethod *out_tracking_method);

// === ArAugmentedImage functions ===

/// @ingroup ArAugmentedImage
/// Returns the pose of the center of the Augmented Image, in world coordinates.
/// The pose's transformed +Y axis will be the normal out of the plane. The
/// pose's transformed +X axis points from left to right on the image, and the
/// transformed +Z axis points from top to bottom on the image.
///
/// If the tracking state is @c #AR_TRACKING_STATE_PAUSED or
/// @c #AR_TRACKING_STATE_STOPPED, this returns the pose when the image state
/// was last @c #AR_TRACKING_STATE_TRACKING, or the identity pose if the image
/// state has never been @c #AR_TRACKING_STATE_TRACKING.
void ArAugmentedImage_getCenterPose(const ArSession *session,
                                    const ArAugmentedImage *augmented_image,
                                    ArPose *out_pose);

/// @ingroup ArAugmentedImage
/// Retrieves the estimated width, in metres, of the corresponding physical
/// image, as measured along the local X-axis of the coordinate space with
/// origin and axes as defined by @c ::ArAugmentedImage_getCenterPose.
///
/// ARCore will attempt to estimate the physical image's width and continuously
/// update this estimate based on its understanding of the world. If the
/// optional physical size is specified in the image database, this estimation
/// process will happen more quickly. However, the estimated size may be
/// different from the originally specified size.
///
/// If the tracking state is
/// @c #AR_TRACKING_STATE_PAUSED or @c #AR_TRACKING_STATE_STOPPED, this returns
/// the estimated width when the image state was last @c
/// #AR_TRACKING_STATE_TRACKING. If the image state has never been @c
/// #AR_TRACKING_STATE_TRACKING, this returns 0, even the image has a specified
/// physical size in the image database.
void ArAugmentedImage_getExtentX(const ArSession *session,
                                 const ArAugmentedImage *augmented_image,
                                 float *out_extent_x);

/// @ingroup ArAugmentedImage
/// Retrieves the estimated height, in metres, of the corresponding physical
/// image, as measured along the local Z-axis of the coordinate space with
/// origin and axes as defined by @c ::ArAugmentedImage_getCenterPose.
///
/// ARCore will attempt to estimate the physical image's height and continuously
/// update this estimate based on its understanding of the world. If an optional
/// physical size is specified in the image database, this estimation process
/// will happen more quickly. However, the estimated size may be different from
/// the originally specified size.
///
/// If the tracking state is
/// @c #AR_TRACKING_STATE_PAUSED or @c #AR_TRACKING_STATE_STOPPED, this returns
/// the estimated height when the image state was last @c
/// #AR_TRACKING_STATE_TRACKING. If the image state has never been @c
/// #AR_TRACKING_STATE_TRACKING, this returns 0, even the image has a specified
/// physical size in the image database.
void ArAugmentedImage_getExtentZ(const ArSession *session,
                                 const ArAugmentedImage *augmented_image,
                                 float *out_extent_z);

/// @ingroup ArAugmentedImage
/// Returns the zero-based positional index of this image from its originating
/// image database.
///
/// This index serves as the unique identifier for the image in the database.
void ArAugmentedImage_getIndex(const ArSession *session,
                               const ArAugmentedImage *augmented_image,
                               int32_t *out_index);

/// @ingroup ArAugmentedImage
/// Returns the name of this image.
///
/// The image name is not guaranteed to be unique.
///
/// This function will allocate memory for the name string, and set
/// *out_augmented_image_name to point to that string. The caller must release
/// the string using @c ::ArString_release when the string is no longer needed.
void ArAugmentedImage_acquireName(const ArSession *session,
                                  const ArAugmentedImage *augmented_image,
                                  char **out_augmented_image_name);

/// @ingroup ArAugmentedImage
/// Returns the current method being used to track this Augmented Image.
void ArAugmentedImage_getTrackingMethod(
    const ArSession *session,
    const ArAugmentedImage *image,
    ArAugmentedImageTrackingMethod *out_tracking_method);

// === ArAugmentedFace functions ===

/// @ingroup ArAugmentedFace
/// Returns a pointer to an array of 3D vertices in (x, y, z) packing. These
/// vertices are relative to the center pose of the face with units in meters.
///
/// The pointer returned by this function is valid until
/// @c ::ArTrackable_release or the next @c ::ArSession_update is called. The
/// application must copy the data if they wish to retain it for longer.
///
/// If the face's tracking state is @c #AR_TRACKING_STATE_PAUSED, then the
/// value of the size of the returned array is 0.
///
/// @param[in]  session                The ARCore session.
/// @param[in]  face                   The face for which to retrieve vertices.
/// @param[out] out_vertices           A pointer to an array of 3D vertices in
///                                    (x, y, z) packing.
/// @param[out] out_number_of_vertices The number of vertices in the mesh. The
///     returned pointer will point to an array of size
///     @p out_number_of_vertices * 3 or @c NULL if the size is 0.
void ArAugmentedFace_getMeshVertices(const ArSession *session,
                                     const ArAugmentedFace *face,
                                     const float **out_vertices,
                                     int32_t *out_number_of_vertices);

/// @ingroup ArAugmentedFace
/// Returns a pointer to an array of 3D normals in (x, y, z) packing, where each
/// (x, y, z) is a unit vector of the normal to the surface at each vertex.
/// There is exactly one normal vector for each vertex. These normals are
/// relative to the center pose of the face.
///
/// The pointer returned by this function is valid until
/// @c ::ArTrackable_release or the next @c ::ArSession_update is called. The
/// application must copy the data if they wish to retain it for longer.
///
/// If the face's tracking state is @c #AR_TRACKING_STATE_PAUSED, then the
/// value of the size of the returned array is 0.
///
/// @param[in]  session               The ARCore session.
/// @param[in]  face                  The face for which to retrieve normals.
/// @param[out] out_normals           A pointer to an array of 3D normals in
///                                   (x, y, z) packing.
/// @param[out] out_number_of_normals The number of normals in the mesh. The
///     returned pointer will point to an array of size
///     @p out_number_of_normals * 3, or @c NULL if the size is 0.
void ArAugmentedFace_getMeshNormals(const ArSession *session,
                                    const ArAugmentedFace *face,
                                    const float **out_normals,
                                    int32_t *out_number_of_normals);

/// @ingroup ArAugmentedFace
/// Returns a pointer to an array of UV texture coordinates in (u, v) packing.
/// There is a pair of texture coordinates for each vertex. These values
/// never change.
///
/// The pointer returned by this function is valid until
/// @c ::ArTrackable_release or the next @c ::ArSession_update is called. The
/// application must copy the data if they wish to retain it for longer.
///
/// If the face's tracking state is @c #AR_TRACKING_STATE_PAUSED, then the
/// value of the size of the returned array is 0.
///
/// @param[in]  session                 The ARCore session.
/// @param[in]  face                    The face for which to retrieve texture
///                                     coordinates.
/// @param[out] out_texture_coordinates A pointer to an array of UV texture
///                                     coordinates in (u, v) packing.
/// @param[out] out_number_of_texture_coordinates The number of texture
///     coordinates in the mesh. The returned pointer will point to an array of
///     size @p out_number_of_texture_coordinates * 2,
///     or @c NULL if the size is 0.
void ArAugmentedFace_getMeshTextureCoordinates(
    const ArSession *session,
    const ArAugmentedFace *face,
    const float **out_texture_coordinates,
    int32_t *out_number_of_texture_coordinates);

/// @ingroup ArAugmentedFace
/// Returns a pointer to an array of triangles indices in consecutive triplets.
///
/// Every three consecutive values are indices that represent a triangle. The
/// vertex position and texture coordinates are mapped by the indices. The front
/// face of each triangle is defined by the face where the vertices are in
/// counter clockwise winding order. These values never change.
///
/// The pointer returned by this function is valid until
/// @c ::ArTrackable_release or the next @c ::ArSession_update is called. The
/// application must copy the data if they wish to retain it for longer.
///
/// If the face's tracking state is @c #AR_TRACKING_STATE_PAUSED, then the
/// value of the size of the returned array is 0.
///
/// @param[in]  session                 The ARCore session.
/// @param[in]  face                    The face for which to retrieve triangle
///                                     indices.
/// @param[out] out_triangle_indices    A pointer to an array of triangle
///                                     indices packed in consecutive triplets.
/// @param[out] out_number_of_triangles The number of triangles in the mesh. The
///     returned pointer will point to an array of size
///     @p out_number_of_triangles * 3, or @c NULL if the size is 0.
void ArAugmentedFace_getMeshTriangleIndices(
    const ArSession *session,
    const ArAugmentedFace *face,
    const uint16_t **out_triangle_indices,
    int32_t *out_number_of_triangles);

/// @ingroup ArAugmentedFace
/// Returns the pose of a face region in world coordinates when the face
/// trackable state is @c #AR_TRACKING_STATE_TRACKING. When face trackable state
/// is @c #AR_TRACKING_STATE_PAUSED, the identity pose will be returned.
///
/// @param[in]  session     The ARCore session.
/// @param[in]  face        The face for which to retrieve face region pose.
/// @param[in]  region_type The face region for which to get the pose.
/// @param[out] out_pose    The Pose of the selected region when
///     @c #AR_TRACKING_STATE_TRACKING, or an identity pose when
///     @c #AR_TRACKING_STATE_PAUSED.
void ArAugmentedFace_getRegionPose(const ArSession *session,
                                   const ArAugmentedFace *face,
                                   const ArAugmentedFaceRegionType region_type,
                                   ArPose *out_pose);

/// @ingroup ArAugmentedFace
/// Returns the pose of the center of the face.
///
/// @param[in]    session  The ARCore session.
/// @param[in]    face     The face for which to retrieve center pose.
/// @param[inout] out_pose An already-allocated @c ::ArPose object into which
///     the pose will be stored.
void ArAugmentedFace_getCenterPose(const ArSession *session,
                                   const ArAugmentedFace *face,
                                   ArPose *out_pose);

// === ArAugmentedImageDatabase functions ===

/// @ingroup ArAugmentedImageDatabase
/// Creates a new empty image database.
void ArAugmentedImageDatabase_create(
    const ArSession *session,
    ArAugmentedImageDatabase **out_augmented_image_database);

/// @ingroup ArAugmentedImageDatabase
/// Creates a new image database from a byte array. The contents of the byte
/// array must have been generated by the command-line database generation tool
/// provided in the SDK, or at runtime from
/// @c ::ArAugmentedImageDatabase_serialize.
///
/// Note: this function takes about 10-20ms for a 5MB byte array. Run this in a
/// background thread if this affects your application.
///
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_DATA_INVALID_FORMAT - the bytes are in an invalid format.
/// - @c #AR_ERROR_DATA_UNSUPPORTED_VERSION - the database is not supported by
///   this version of the SDK.
ArStatus ArAugmentedImageDatabase_deserialize(
    const ArSession *session,
    const uint8_t *database_raw_bytes,
    int64_t database_raw_bytes_size,
    ArAugmentedImageDatabase **out_augmented_image_database);

/// @ingroup ArAugmentedImageDatabase
/// Serializes an image database to a byte array.
///
/// This function will allocate memory for the serialized raw byte array, and
/// set @p *out_image_database_raw_bytes to point to that byte array. The caller
/// is expected to release the byte array using @c ::ArByteArray_release when
/// the byte array is no longer needed.
void ArAugmentedImageDatabase_serialize(
    const ArSession *session,
    const ArAugmentedImageDatabase *augmented_image_database,
    uint8_t **out_image_database_raw_bytes,
    int64_t *out_image_database_raw_bytes_size);

/// @ingroup ArAugmentedImageDatabase
/// Adds a single named image of unknown physical size to an image database,
/// from an array of grayscale pixel values. Returns the zero-based positional
/// index of the image within the image database.
///
/// If the physical size of the image is known, use
/// @c ::ArAugmentedImageDatabase_addImageWithPhysicalSize instead, to improve
/// image detection time.
///
/// For images added via @c ::ArAugmentedImageDatabase_addImage, ARCore
/// estimates the physical image's size and pose at runtime when the physical
/// image is visible and is being tracked. This extra estimation step will
/// require the user to move their device to view the physical image from
/// different viewpoints before the size and pose of the physical image can be
/// estimated.
///
/// This function takes time to perform non-trivial image processing (20-30ms),
/// and should be run on a background thread.
///
/// The image name is expected to be a null-terminated string in UTF-8 format.
///
/// Currently, only images for which the stride is equal to the width are
/// supported.
///
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_IMAGE_INSUFFICIENT_QUALITY - image quality is insufficient,
///   e.g. because of lack of features in the image.
/// - @c #AR_ERROR_INVALID_ARGUMENT - if @p image_stride_in_pixels is not equal
///   to @p image_width_in_pixels.
ArStatus ArAugmentedImageDatabase_addImage(
    const ArSession *session,
    ArAugmentedImageDatabase *augmented_image_database,
    const char *image_name,
    const uint8_t *image_grayscale_pixels,
    int32_t image_width_in_pixels,
    int32_t image_height_in_pixels,
    int32_t image_stride_in_pixels,
    int32_t *out_index);

/// @ingroup ArAugmentedImageDatabase
/// Adds a single named image to an image database, from an array of grayscale
/// pixel values, along with a positive physical width in meters for this image.
/// Returns the zero-based positional index of the image within the image
/// database.
///
/// If the physical size of the image is not known, use
/// @c ::ArAugmentedImageDatabase_addImage instead, at the expense of an
/// increased image detection time.
///
/// For images added via @c ::ArAugmentedImageDatabase_addImageWithPhysicalSize,
/// ARCore can estimate the pose of the physical image at runtime as soon as
/// ARCore detects the physical image, without requiring the user to move the
/// device to view the physical image from different viewpoints. Note that
/// ARCore will refine the estimated size and pose of the physical image as it
/// is viewed from different viewpoints.
///
/// This function takes time to perform non-trivial image processing (20-30ms),
/// and should be run on a background thread.
///
/// The image name is expected to be a null-terminated string in UTF-8 format.
///
/// Currently, only images for which the stride is equal to the width are
/// supported.
///
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_IMAGE_INSUFFICIENT_QUALITY - image quality is insufficient,
///   e.g. because of lack of features in the image.
/// - @c #AR_ERROR_INVALID_ARGUMENT - @p image_width_in_meters is <= 0 or if
///   image_stride_in_pixels is not equal to @p image_width_in_pixels.
ArStatus ArAugmentedImageDatabase_addImageWithPhysicalSize(
    const ArSession *session,
    ArAugmentedImageDatabase *augmented_image_database,
    const char *image_name,
    const uint8_t *image_grayscale_pixels,
    int32_t image_width_in_pixels,
    int32_t image_height_in_pixels,
    int32_t image_stride_in_pixels,
    float image_width_in_meters,
    int32_t *out_index);

/// @ingroup ArAugmentedImageDatabase
/// Returns the number of images in the image database.
void ArAugmentedImageDatabase_getNumImages(
    const ArSession *session,
    const ArAugmentedImageDatabase *augmented_image_database,
    int32_t *out_number_of_images);

/// @ingroup ArAugmentedImageDatabase
/// Releases memory used by an image database.
void ArAugmentedImageDatabase_destroy(
    ArAugmentedImageDatabase *augmented_image_database);

// === ArHitResultList functions ===

/// @ingroup ArHitResult
/// Creates a hit result list object.
void ArHitResultList_create(const ArSession *session,
                            ArHitResultList **out_hit_result_list);

/// @ingroup ArHitResult
/// Releases the memory used by a hit result list object, along with all the
/// trackable references it holds.
void ArHitResultList_destroy(ArHitResultList *hit_result_list);

/// @ingroup ArHitResult
/// Retrieves the number of hit results in this list.
void ArHitResultList_getSize(const ArSession *session,
                             const ArHitResultList *hit_result_list,
                             int32_t *out_size);

/// @ingroup ArHitResult
/// Copies an indexed entry in the list.  This acquires a reference to any
/// trackable referenced by the item, and releases any reference currently held
/// by the provided result object.
///
/// @param[in]    session           The ARCore session.
/// @param[in]    hit_result_list   The list from which to copy an item.
/// @param[in]    index             Index of the entry to copy.
/// @param[inout] out_hit_result    An already-allocated @c ::ArHitResult object
///     into which the result will be copied.
void ArHitResultList_getItem(const ArSession *session,
                             const ArHitResultList *hit_result_list,
                             int32_t index,
                             ArHitResult *out_hit_result);

// === ArHitResult functions ===

/// @ingroup ArHitResult
/// Allocates an empty hit result object.
void ArHitResult_create(const ArSession *session, ArHitResult **out_hit_result);

/// @ingroup ArHitResult
/// Releases the memory used by a hit result object, along with any
/// trackable reference it holds.
void ArHitResult_destroy(ArHitResult *hit_result);

/// @ingroup ArHitResult
/// Returns the distance from the camera to the hit location, in meters.
void ArHitResult_getDistance(const ArSession *session,
                             const ArHitResult *hit_result,
                             float *out_distance);

/// @ingroup ArHitResult
/// Returns the pose of the intersection between a ray and detected real-world
/// geometry. The position is the location in space where the ray intersected
/// the geometry. The orientation is a best effort to face the ray origin,
/// and its exact definition differs depending on the Trackable that was hit.
///
/// @c ::ArPlane : X+ is perpendicular to the cast ray and parallel to the
/// plane, Y+ points along the plane normal (up, for @c
/// #AR_PLANE_HORIZONTAL_UPWARD_FACING planes), and Z+ is parallel to the plane,
/// pointing roughly toward the ray origin.
///
/// @c ::ArPoint :
/// Attempt to estimate the normal of the surface centered around the hit test.
/// Surface normal estimation is most likely to succeed on textured surfaces
/// and with camera motion.
/// If @c ::ArPoint_getOrientationMode returns
/// @c #AR_POINT_ORIENTATION_ESTIMATED_SURFACE_NORMAL, then X+ is perpendicular
/// to the cast ray and parallel to the physical surface centered around the hit
/// test, Y+ points along the estimated surface normal, and Z+ points roughly
/// toward the ray origin. If
/// @c ::ArPoint_getOrientationMode returns
/// @c #AR_POINT_ORIENTATION_INITIALIZED_TO_IDENTITY, then X+ is perpendicular
/// to the cast ray and points right from the perspective of the ray origin,
/// Y+ points up, and Z+ points roughly toward the ray origin.
///
/// If you wish to retain the location of this pose beyond the duration of a
/// single frame, create an anchor using @c ::ArHitResult_acquireNewAnchor to
/// save the pose in a physically consistent way.
///
/// @param[in]    session    The ARCore session.
/// @param[in]    hit_result The hit result to retrieve the pose of.
/// @param[inout] out_pose   An already-allocated @c ::ArPose object into which
///     the pose will be stored.
void ArHitResult_getHitPose(const ArSession *session,
                            const ArHitResult *hit_result,
                            ArPose *out_pose);

/// @ingroup ArHitResult
/// Acquires reference to the hit trackable. This call must be paired with a
/// call to @c ::ArTrackable_release.
void ArHitResult_acquireTrackable(const ArSession *session,
                                  const ArHitResult *hit_result,
                                  ArTrackable **out_trackable);

/// @ingroup ArHitResult
/// Creates a new anchor at the hit location. See @c ::ArHitResult_getHitPose
/// for details.  This is equivalent to creating an anchor on the hit trackable
/// at the hit pose.
///
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_NOT_TRACKING
/// - @c #AR_ERROR_SESSION_PAUSED
/// - @c #AR_ERROR_RESOURCE_EXHAUSTED
/// - @c #AR_ERROR_DEADLINE_EXCEEDED - hit result must be used before the next
///   call to @c ::ArSession_update.
ArStatus ArHitResult_acquireNewAnchor(ArSession *session,
                                      ArHitResult *hit_result,
                                      ArAnchor **out_anchor);

// === Utility functions for releasing data ===

/// @ingroup utility_functions
/// Releases a string acquired using an ARCore API function.
///
/// @param[in] str The string to be released.
void ArString_release(char *str);

/// @ingroup utility_functions
/// Releases a byte array created using an ARCore API function.
void ArByteArray_release(uint8_t *byte_array);

/// @ingroup ArEarth
/// Describes the current state of @c ::ArEarth. When @c
/// ::ArTrackable_getTrackingState does not become @c
/// #AR_TRACKING_STATE_TRACKING, @c ::ArEarthState may contain the cause of this
/// failure.
///
/// Obtain using @c ::ArEarth_getEarthState.
AR_DEFINE_ENUM(ArEarthState){
    /// @c ::ArEarth is enabled, and has not encountered any problems. Check @c
    /// ::ArTrackable_getTrackingState to determine if it can be used.
    AR_EARTH_STATE_ENABLED = 0,

    /// Earth localization has encountered an internal error. The app should not
    /// attempt to recover from this error. Please see the Android logs for
    /// additional information.
    AR_EARTH_STATE_ERROR_INTERNAL = -1,

    /// The given @c ::ArEarth is no longer valid and has @c ::ArTrackingState
    /// @c #AR_TRACKING_STATE_STOPPED due to @c #AR_GEOSPATIAL_MODE_DISABLED
    /// being set on the @c ::ArSession. The given @c ::ArEarth object should be
    /// released by calling @c ::ArTrackable_release.
    AR_EARTH_STATE_ERROR_GEOSPATIAL_MODE_DISABLED = -2,

    /// The authorization provided by the application is not valid.
    /// - The Google Cloud project may not have enabled the ARCore API.
    /// - When using API key authentication, this will happen if the API key in
    ///   the manifest is invalid or unauthorized. It may also fail if the API
    ///   key is restricted to a set of apps not including the current one.
    /// - When using keyless authentication, this may happen when no OAuth
    ///   client has been created, or when the signing key and package name
    ///   combination does not match the values used in the Google Cloud
    ///   project.  It may also fail if Google Play Services isn't installed,
    ///   is too old, or is malfunctioning for some reason (e.g. killed
    ///   due to memory pressure).
    AR_EARTH_STATE_ERROR_NOT_AUTHORIZED = -3,

    /// The application has exhausted the quota allotted to the given
    /// Google Cloud project. The developer should <a
    /// href="https://cloud.google.com/docs/quota#requesting_higher_quota">request
    /// additional quota</a> for the ARCore API for their project from the
    /// Google Cloud Console.
    AR_EARTH_STATE_ERROR_RESOURCE_EXHAUSTED = -4,

    /// The APK is older than the supported version.
    AR_EARTH_STATE_ERROR_APK_VERSION_TOO_OLD = -5,
};

/// @ingroup ArEarth
/// Gets the current @c ::ArEarthState of the @c ::ArEarth. This state is
/// guaranteed not to change until @c ::ArSession_update is called.
///
/// @param[in]    session   The ARCore session.
/// @param[in]    earth     The earth object.
/// @param[inout] out_state The current state of @c ::ArEarth.
void ArEarth_getEarthState(const ArSession *session,
                           const ArEarth *earth,
                           ArEarthState *out_state);

/// @ingroup ArEarth
/// Returns the Earth-relative camera pose for the latest frame.
///
/// The orientation of the obtained @c ::ArGeospatialPose approximates the
/// direction the user is facing in the EUS coordinate system. The EUS
/// coordinate system has X+ pointing east, Y+ pointing up, and Z+ pointing
/// south.
///
/// Note: This pose is only valid when @c ::ArTrackable_getTrackingState is @c
/// #AR_TRACKING_STATE_TRACKING. Otherwise, the resulting @c ::ArGeospatialPose
/// will contain default values.
///
/// @param[in]    session                    The ARCore session.
/// @param[in]    earth                      The @c ::ArEarth object.
/// @param[out]   out_camera_geospatial_pose Pointer to a @c ::ArGeospatialPose
///     to receive the Geospatial pose.
void ArEarth_getCameraGeospatialPose(
    const ArSession *session,
    const ArEarth *earth,
    ArGeospatialPose *out_camera_geospatial_pose);

/// @ingroup ArEarth
/// Converts the provided @p pose to a @c ::ArGeospatialPose with respect to the
/// Earth. The @p out_geospatial_pose's rotation quaternion is a rotation with
/// respect to an East-Up-South coordinate frame. An identity quaternion will
/// have the anchor oriented such that X+ points to the east, Y+ points up away
/// from the center of the earth, and Z+ points to the south.
///
/// The heading value for a @c ::ArGeospatialPose obtained by this function
/// will be 0. See @c ::ArGeospatialPose_getEastUpSouthQuaternion for an
/// orientation in 3D space.
///
/// @param[in] session   The ARCore session.
/// @param[in] earth     The @c ::ArEarth handle.
/// @param[in] pose      The local pose to translate to an Earth pose.
/// @param[out] out_geospatial_pose Pointer to a @c ::ArGeospatialPose
///     that receives the Geospatial pose.
/// @return @c #AR_SUCCESS, or any of:
/// - @c #AR_ERROR_NOT_TRACKING if the Earth's tracking state is not @c
/// #AR_TRACKING_STATE_TRACKING.
/// - @c #AR_ERROR_INVALID_ARGUMENT if any of the parameters are null.
ArStatus ArEarth_getGeospatialPose(const ArSession *session,
                                   const ArEarth *earth,
                                   const ArPose *pose,
                                   ArGeospatialPose *out_geospatial_pose);

/// @ingroup ArEarth
/// Converts the provided Earth specified horizontal position, altitude and
/// rotation with respect to an east-up-south coordinate frame to a pose with
/// respect to GL world coordinates.
///
/// Position near the north pole or south pole is not supported. If the
/// latitude is within 0.1 degrees of the north pole or south pole (90 degrees
/// or -90 degrees), this function will return @c #AR_ERROR_INVALID_ARGUMENT.
///
/// @param[in] session   The ARCore session.
/// @param[in] earth     The @c ::ArEarth handle.
/// @param[in] latitude         The latitude of the anchor relative to the WGS84
///     ellipsoid.
/// @param[in] longitude        The longitude of the anchor relative to the
///     WGS84 ellipsoid.
/// @param[in] altitude         The altitude of the anchor relative to the WGS84
///     ellipsoid, in meters.
/// @param[in] eus_quaternion_4 The rotation quaternion as {qx, qy, qx, qw}.
/// @param[out] out_pose A pointer to @c ::ArPose of the local pose with the
/// latest frame.
/// @return @c #AR_SUCCESS, or any of:
/// - @c #AR_ERROR_NOT_TRACKING if the Earth's tracking state is not @c
/// #AR_TRACKING_STATE_TRACKING.
/// - @c #AR_ERROR_INVALID_ARGUMENT, if any of the parameters are null or @p
/// latitude is outside the allowable range.
ArStatus ArEarth_getPose(const ArSession *session,
                         const ArEarth *earth,
                         double latitude,
                         double longitude,
                         double altitude,
                         const float *eus_quaternion_4,
                         ArPose *out_pose);

/// @ingroup ArEarth
/// Creates a new @c ::ArAnchor at the specified geodetic location and
/// orientation relative to the Earth.
///
/// Latitude and longitude are defined by the
/// <a href="https://en.wikipedia.org/wiki/World_Geodetic_System">WGS84
/// specification</a>, and altitude values are defined as the elevation above
/// the WGS84 ellipsoid in meters.
///
/// The rotation provided by @p eus_quaternion_4 is a rotation with respect to
/// an east-up-south coordinate frame. An identity rotation will have the anchor
/// oriented such that X+ points to the east, Y+ points up away from the center
/// of the earth, and Z+ points to the south.
///
/// An anchor's @c ::ArTrackingState will be @c #AR_TRACKING_STATE_PAUSED while
/// @c ::ArEarth is @c #AR_TRACKING_STATE_PAUSED. The tracking state will
/// permanently become @c #AR_TRACKING_STATE_STOPPED if the @c ::ArSession
/// configuration is set to @c #AR_GEOSPATIAL_MODE_DISABLED.
///
/// Creating anchors near the north pole or south pole is not supported. If the
/// latitude is within 0.1 degrees of the north pole or south pole (90 degrees
/// or -90 degrees), this function will return @c #AR_ERROR_INVALID_ARGUMENT and
/// the anchor will fail to be created.
///
/// @param[in] session          The ARCore session.
/// @param[in] earth            The @c ::ArEarth handle.
/// @param[in] latitude         The latitude of the anchor relative to the WGS84
///     ellipsoid.
/// @param[in] longitude        The longitude of the anchor relative to the
///     WGS84 ellipsoid.
/// @param[in] altitude         The altitude of the anchor relative to the WGS84
///     ellipsoid, in meters.
/// @param[in] eus_quaternion_4 The rotation quaternion as {qx, qy, qx, qw}.
/// @param[out] out_anchor      The newly-created @c ::ArAnchor. This will be
///     set to  @c NULL if no anchor was created.
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_ILLEGAL_STATE if @p earth is @c #AR_TRACKING_STATE_STOPPED
///   due to @c #AR_GEOSPATIAL_MODE_DISABLED configured on the @c ::ArSession.
///   Reacquire @c ::ArEarth if earth mode was reenabled.
/// - @c #AR_ERROR_INVALID_ARGUMENT if @p latitude is outside the allowable
///   range, or if either @p session, @p earth, or @p eus_quaternion_4 is @c
///   NULL.
ArStatus ArEarth_acquireNewAnchor(ArSession *session,
                                  ArEarth *earth,
                                  double latitude,
                                  double longitude,
                                  double altitude,
                                  const float *eus_quaternion_4,
                                  ArAnchor **out_anchor);

/// @ingroup ArEarth
/// Creates an anchor at a specified horizontal position and altitude relative
/// to the horizontal position’s terrain. Terrain means the ground, or ground
/// floor inside a building with VPS coverage. For areas not covered by VPS,
/// consider using @c ::ArEarth_acquireNewAnchor to attach anchors to Earth.
///
/// The specified altitude is interpreted to be relative to the Earth's terrain
/// (or floor) at the specified latitude/longitude geodetic coordinates, rather
/// than relative to the WGS-84 ellipsoid. Specifying an altitude of 0 will
/// position the anchor directly on the terrain (or floor) whereas specifying a
/// positive altitude will position the anchor above the terrain (or floor),
/// against the direction of gravity.
///
/// This creates a new @c ::ArAnchor and schedules a task to resolve the
/// anchor's pose using the given parameters. You may resolve multiple anchors
/// at a time, but a session cannot be tracking more than 100 Terrain anchors at
/// time. Attempting to resolve more than 100 Terrain anchors will result in
/// resolve calls returning status @c #AR_ERROR_RESOURCE_EXHAUSTED.
///
/// If this function returns @c #AR_SUCCESS, the terrain anchor state of @p
/// out_terrain_anchor will be @c #AR_TERRAIN_ANCHOR_STATE_TASK_IN_PROGRESS, and
/// its tracking state will be @c #AR_TRACKING_STATE_PAUSED. This anchor
/// remains in this state until its pose has been successfully resolved. If
/// the resolving task results in an error, the tracking state will be set to @c
/// #AR_TRACKING_STATE_STOPPED. If the return value is not @c #AR_SUCCESS, then
/// @p out_cloud_anchor will be set to @c NULL.
///
/// Creating a terrain anchor requires an active @c ::ArEarth for which the @c
/// ::ArEarthState is @c #AR_EARTH_STATE_ENABLED and @c ::ArTrackingState is @c
/// #AR_TRACKING_STATE_TRACKING. If it is not, then this function returns @c
/// #AR_ERROR_ILLEGAL_STATE. This call also requires a working internet
/// connection to communicate with the ARCore API on Google Cloud. ARCore will
/// continue to retry if it is unable to establish a connection to the ARCore
/// service.
///
/// Latitude and longitude are defined by the
/// <a href="https://en.wikipedia.org/wiki/World_Geodetic_System">WGS84
/// specification</a>.
///
/// The rotation provided by @p eus_quaternion_4 is a rotation with respect to
/// an east-up-south coordinate frame. An identity rotation will have the anchor
/// oriented such that X+ points to the east, Y+ points up away from the center
/// of the earth, and Z+ points to the south.
///
/// @param[in] session          The ARCore session.
/// @param[in] earth            The @c ::ArEarth handle.
/// @param[in] latitude         The latitude of the anchor relative to the
///                             WGS-84 ellipsoid.
/// @param[in] longitude        The longitude of the anchor relative to the
///                             WGS-84 ellipsoid.
/// @param[in] altitude_above_terrain The altitude of the anchor above the
///                             Earth's terrain (or floor).
/// @param[in] eus_quaternion_4 The rotation quaternion as {qx, qy, qx, qw}.
/// @param[out] out_anchor      The newly-created anchor.
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_ILLEGAL_STATE if @c ::ArEarthState is not
///   @c #AR_EARTH_STATE_ENABLED.
/// - @c #AR_ERROR_INVALID_ARGUMENT if @p latitude is outside the allowable
///   range, or if either @p session, @p earth, or @p eus_quaternion_4 is @c
///   NULL.
/// - @c #AR_ERROR_RESOURCE_EXHAUSTED if too many terrain anchors are currently
///   held.
/// @deprecated Use @c ::ArEarth_resolveAnchorOnTerrainAsync instead.
ArStatus ArEarth_resolveAndAcquireNewAnchorOnTerrain(
    ArSession *session,
    ArEarth *earth,
    double latitude,
    double longitude,
    double altitude_above_terrain,
    const float *eus_quaternion_4,
    ArAnchor **out_anchor)
    AR_DEPRECATED("Use ArEarth_resolveAnchorOnTerrainAsync instead.");

/// @ingroup ArAnchor
/// Describes the result of a Terrain anchor resolving operation.
AR_DEFINE_ENUM(ArTerrainAnchorState){
    /// Not a valid value for a terrain anchor operation.
    AR_TERRAIN_ANCHOR_STATE_NONE = 0,

    /// Resolving the Terrain anchor is in progress. Once the task completes in
    /// the background, the anchor will get a new state after the next @c
    /// ::ArSession_update call.
    AR_TERRAIN_ANCHOR_STATE_TASK_IN_PROGRESS AR_DEPRECATED(
        "Not returned by async APIs - replaced by "
        "AR_FUTURE_STATE_PENDING.") = 1,

    /// A resolving task for this anchor has been successfully resolved.
    AR_TERRAIN_ANCHOR_STATE_SUCCESS = 2,

    /// Resolving task for this anchor finished with an internal
    /// error. The app should not attempt to recover from this error.
    AR_TERRAIN_ANCHOR_STATE_ERROR_INTERNAL = -1,

    /// The authorization provided by the application is not valid.
    /// - The Google Cloud project may not have enabled the ARCore API.
    /// - When using API key authentication, this will happen if the API key in
    ///   the manifest is invalid or unauthorized. It may also fail if the API
    ///   key is restricted to a set of apps not including the current one.
    /// - When using keyless authentication, this may happen when no OAuth
    ///   client has been created, or when the signing key and package name
    ///   combination does not match the values used in the Google Cloud
    ///   project.  It may also fail if Google Play Services isn't installed,
    ///   is too old, or is malfunctioning for some reason (e.g. killed
    ///   due to memory pressure).
    AR_TERRAIN_ANCHOR_STATE_ERROR_NOT_AUTHORIZED = -2,

    /// There is no terrain info at this location, such as the center of the
    /// ocean.
    AR_TERRAIN_ANCHOR_STATE_ERROR_UNSUPPORTED_LOCATION = -3,
};

/// @ingroup ArAnchor
/// Gets the current Terrain anchor state of the anchor. This state is
/// guaranteed not to change until @c ::ArSession_update is called.
///
/// @param[in]    session   The ARCore session.
/// @param[in]    anchor    The anchor to retrieve the terrain anchor state of.
/// @param[inout] out_state The current terrain anchor state of the anchor.
///                         Non-terrain anchors will always be in
///                         @c #AR_TERRAIN_ANCHOR_STATE_NONE state.
/// @deprecated Use @c
/// ::ArResolveAnchorOnTerrainFuture_getResultTerrainAnchorState.
void ArAnchor_getTerrainAnchorState(const ArSession *session,
                                    const ArAnchor *anchor,
                                    ArTerrainAnchorState *out_state)
    AR_DEPRECATED(
        "For anchors resolved using async terrain anchor APIs, the state can "
        "be obtained from the callback or future object.");

/// @ingroup ArResolveAnchorOnTerrainFuture
/// Gets the resolved Terrain anchor. If the operation isn't done yet or the
/// operation failed, this will be @c NULL. The caller must release the anchor
/// using @c ::ArAnchor_release.
///
/// @param[in] session     The ARCore session.
/// @param[in] future      The handle for the asynchronous operation.
/// @param[out] out_anchor The anchor.
void ArResolveAnchorOnTerrainFuture_acquireResultAnchor(
    const ArSession *session,
    const ArResolveAnchorOnTerrainFuture *future,
    ArAnchor **out_anchor);

/// @ingroup ArResolveAnchorOnTerrainFuture
/// Gets the result status of the resolving operation, if the operation is done.
///
/// @param[in] session    The ARCore session.
/// @param[in] future     The handle for the asynchronous operation.
/// @param[out] out_terrain_anchor_state The result status.
void ArResolveAnchorOnTerrainFuture_getResultTerrainAnchorState(
    const ArSession *session,
    const ArResolveAnchorOnTerrainFuture *future,
    ArTerrainAnchorState *out_terrain_anchor_state);

/// @ingroup ArResolveAnchorOnTerrainFuture
/// Callback definition for @c ::ArEarth_resolveAnchorOnTerrainAsync. The
/// @p context argument will be the same as that passed to
/// @c ::ArEarth_resolveAnchorOnTerrainAsync. The @p anchor argument will be the
/// same as that returned by
/// @c ::ArResolveAnchorOnTerrainFuture_acquireResultAnchor and must be released
/// using @c ::ArAnchor_release. The @p terrain_anchor_state argument will be
/// the same as that returned by
/// @c ::ArResolveAnchorOnTerrainFuture_getResultTerrainAnchorState.
///
/// It is a best practice to free @c context memory provided to
/// @c ::ArEarth_resolveAnchorOnTerrainAsync at the end of the callback
/// implementation.
typedef void (*ArResolveAnchorOnTerrainCallback)(
    void *context, ArAnchor *anchor, ArTerrainAnchorState terrain_anchor_state);

/// @ingroup ArEarth
/// Asynchronously creates an anchor at a specified horizontal position and
/// altitude relative to the horizontal position’s terrain. See the <a
/// href="https://developers.google.com/ar/develop/geospatial/c/anchors#terrain-anchors">Terrain
/// anchors developer guide</a> for more information.
///
/// The specified altitude is interpreted to be relative to the Earth's terrain
/// (or floor) at the specified latitude/longitude geodetic coordinates, rather
/// than relative to the WGS-84 ellipsoid. Specifying an altitude of 0 will
/// position the anchor directly on the terrain (or floor) whereas specifying a
/// positive altitude will position the anchor above the terrain (or floor),
/// against the direction of gravity.
///
/// This launches an asynchronous operation used to query the Google Cloud
/// ARCore API. See @c ::ArFuture for information on obtaining results and
/// cancelling the operation.
///
/// You may resolve multiple anchors at a time, but a session cannot
/// be tracking more than 100 Terrain and Rooftop anchors at time. Attempting to
/// resolve more than 100 Terrain or Rooftop anchors will result in resolve
/// calls returning status @c #AR_ERROR_RESOURCE_EXHAUSTED.
///
/// Creating a terrain anchor requires an active @c ::ArEarth for which the @c
/// ::ArEarthState is @c #AR_EARTH_STATE_ENABLED and @c ::ArTrackingState is @c
/// #AR_TRACKING_STATE_TRACKING. If it is not, then this function returns @c
/// #AR_ERROR_ILLEGAL_STATE. This call also requires a working internet
/// connection to communicate with the ARCore API on Google Cloud. ARCore will
/// continue to retry if it is unable to establish a connection to the ARCore
/// service.
///
/// Latitude and longitude are defined by the
/// <a href="https://en.wikipedia.org/wiki/World_Geodetic_System">WGS84
/// specification</a>.
///
/// The rotation provided by @p eus_quaternion_4 is a rotation with respect to
/// an east-up-south coordinate frame. An identity rotation will have the anchor
/// oriented such that X+ points to the east, Y+ points up away from the center
/// of the earth, and Z+ points to the south.
///
/// @param[in] session          The ARCore session.
/// @param[in] earth            The @c ::ArEarth handle.
/// @param[in] latitude         The latitude of the anchor relative to the
///                             WGS-84 ellipsoid.
/// @param[in] longitude        The longitude of the anchor relative to the
///                             WGS-84 ellipsoid.
/// @param[in] altitude_above_terrain The altitude of the anchor above the
///                             Earth's terrain (or floor).
/// @param[in] eus_quaternion_4 The rotation quaternion as {qx, qy, qx, qw}.
/// @param[in] context An optional void pointer passed when using a @ref
///   future_callback "callback".
/// @param[in] callback A @ref future_callback "callback function".
/// @param[out] out_future An optional @c ::ArFuture which can be @ref
///   future_polling "polled".
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_ILLEGAL_STATE if @c ::ArEarthState is not
///   @c #AR_EARTH_STATE_ENABLED.
/// - @c #AR_ERROR_INVALID_ARGUMENT if @p latitude is outside the allowable
///   range, or if either @p session, @p earth, or @p eus_quaternion_4 is @c
///   NULL, or both @p callback and @p out_future are @c NULL.
/// - @c #AR_ERROR_RESOURCE_EXHAUSTED if too many anchors (counting both Terrain
///   and Rooftop anchors) are currently held or are being resolved.
ArStatus ArEarth_resolveAnchorOnTerrainAsync(
    ArSession *session,
    ArEarth *earth,
    double latitude,
    double longitude,
    double altitude_above_terrain,
    const float *eus_quaternion_4,
    void *context,
    ArResolveAnchorOnTerrainCallback callback,
    ArResolveAnchorOnTerrainFuture **out_future);

/// @ingroup ArAnchor
/// Describes the current Rooftop anchor state of an @c ::ArAnchor. Obtained by
/// @c ::ArResolveAnchorOnRooftopFuture_getResultRooftopAnchorState or as a
/// parameter to the callback.
AR_DEFINE_ENUM(ArRooftopAnchorState){
    /// Not a valid value for a Rooftop anchor operation.
    AR_ROOFTOP_ANCHOR_STATE_NONE = 0,
    /// A resolving task for this anchor has been successfully resolved.
    AR_ROOFTOP_ANCHOR_STATE_SUCCESS = 1,

    /// Resolving task for this anchor finished with an internal
    /// error. The app should not attempt to recover from this error.
    AR_ROOFTOP_ANCHOR_STATE_ERROR_INTERNAL = -1,

    /// The authorization provided by the application is not valid.
    /// - The Google Cloud project may not have enabled the ARCore API.
    /// - When using API key authentication, this will happen if the API key in
    ///   the manifest is invalid or unauthorized. It may also fail if the API
    ///   key is restricted to a set of apps not including the current one.
    /// - When using keyless authentication, this may happen when no OAuth
    ///   client has been created, or when the signing key and package name
    ///   combination does not match the values used in the Google Cloud
    ///   project.  It may also fail if Google Play Services isn't installed,
    ///   is too old, or is malfunctioning for some reason (e.g. killed
    ///   due to memory pressure).
    AR_ROOFTOP_ANCHOR_STATE_ERROR_NOT_AUTHORIZED = -2,

    /// There is no rooftop or terrain info at this location, such as the center
    /// of the ocean.
    AR_ROOFTOP_ANCHOR_STATE_ERROR_UNSUPPORTED_LOCATION = -3,
};

/// @ingroup ArResolveAnchorOnRooftopFuture
/// Gets the resolved Rooftop anchor. If the operation isn't done yet or the
/// operation failed, this will be @c NULL. The caller must release the anchor
/// using @c ::ArAnchor_release.
///
/// @param[in] session    The ARCore session.
/// @param[in] future     The handle for the asynchronous operation.
/// @param[out] out_anchor The anchor.
void ArResolveAnchorOnRooftopFuture_acquireResultAnchor(
    const ArSession *session,
    const ArResolveAnchorOnRooftopFuture *future,
    ArAnchor **out_anchor);

/// @ingroup ArResolveAnchorOnRooftopFuture
/// Gets the result status of the resolving operation, if the operation is done.
///
/// @param[in] session    The ARCore session.
/// @param[in] future     The handle for the asynchronous operation.
/// @param[out] out_rooftop_anchor_state The result status.
void ArResolveAnchorOnRooftopFuture_getResultRooftopAnchorState(
    const ArSession *session,
    const ArResolveAnchorOnRooftopFuture *future,
    ArRooftopAnchorState *out_rooftop_anchor_state);

/// @ingroup ArResolveAnchorOnRooftopFuture
/// Callback definition for @c ::ArEarth_resolveAnchorOnRooftopAsync.
/// The @p context argument will be the same as that passed to
/// @c ::ArEarth_resolveAnchorOnRooftopAsync. The @p anchor argument
/// will be the same as that returned by
/// @c ::ArResolveAnchorOnRooftopFuture_acquireResultAnchor and must be
/// released using @c ::ArAnchor_release. The @p rooftop_anchor_state argument
/// will be the same as that returned by @c
/// ::ArResolveAnchorOnRooftopFuture_getResultRooftopAnchorState.
///
/// It is a best practice to free @c context memory provided to
/// @c ::ArEarth_resolveAnchorOnRooftopAsync at the end of the callback
/// implementation.
typedef void (*ArResolveAnchorOnRooftopCallback)(
    void *context, ArAnchor *anchor, ArRooftopAnchorState rooftop_anchor_state);

/// @ingroup ArEarth
/// Asynchronously creates an anchor at a specified horizontal position and
/// altitude relative to the horizontal position’s rooftop. See the <a
/// href="https://developers.google.com/ar/develop/geospatial/c/anchors#rooftop-anchors">Rooftop
/// anchors developer guide</a> for more information.
///
/// The specified @p altitude_above_rooftop is interpreted to be relative to the
/// top of a building at the given horizontal location, rather than relative to
/// the WGS84 ellipsoid. If there is no building at the given location, then the
/// altitude is interpreted to be relative to the terrain instead. Specifying an
/// altitude of 0 will position the anchor directly on the rooftop whereas
/// specifying a positive altitude will position the anchor above the rooftop,
/// against the direction of gravity.
///
/// This launches an asynchronous operation used to query the Google Cloud
/// ARCore API. See @c ::ArFuture for information on obtaining results and
/// cancelling the operation.
///
/// You may resolve multiple anchors at a time, but a session cannot
/// be tracking more than 100 Terrain and Rooftop anchors at time. Attempting to
/// resolve more than 100 Terrain or Rooftop anchors will result in resolve
/// calls returning status @c #AR_ERROR_RESOURCE_EXHAUSTED.
///
/// Creating a Rooftop anchor requires an active @c ::ArEarth for which the @c
/// ::ArEarthState is @c #AR_EARTH_STATE_ENABLED and @c ::ArTrackingState is @c
/// #AR_TRACKING_STATE_TRACKING. If it is not, then this function returns @c
/// #AR_ERROR_ILLEGAL_STATE. This call also requires a working internet
/// connection to communicate with the ARCore API on Google Cloud. ARCore will
/// continue to retry if it is unable to establish a connection to the ARCore
/// service.
///
/// Latitude and longitude are defined by the
/// <a href="https://en.wikipedia.org/wiki/World_Geodetic_System">WGS84
/// specification</a>.
///
/// The rotation provided by @p eus_quaternion_4 is a rotation with respect to
/// an east-up-south coordinate frame. An identity rotation will have the anchor
/// oriented such that X+ points to the east, Y+ points up away from the center
/// of the earth, and Z+ points to the south.
///
/// To create an anchor that has the +Z axis pointing in the same direction as
/// heading obtained from @c ::ArGeospatialPose, use the following formula:
///
/// \code
/// {qx, qy, qz, qw} = {0, sin((pi - heading * M_PI / 180.0) / 2), 0, cos((pi -
/// heading * M_PI / 180.0) / 2)}}.
/// \endcode
///
/// @param[in] session          The ARCore session.
/// @param[in] earth            The @c ::ArEarth handle.
/// @param[in] latitude         The latitude of the anchor relative to the
///                             WGS-84 ellipsoid.
/// @param[in] longitude        The longitude of the anchor relative to the
///                             WGS-84 ellipsoid.
/// @param[in] altitude_above_rooftop The altitude of the anchor above the
///                             Earth's rooftop.
/// @param[in] eus_quaternion_4 The rotation quaternion as {qx, qy, qx, qw}.
/// @param[in] context An optional void pointer passed when using a @ref
///   future_callback "callback".
/// @param[in] callback A @ref future_callback "callback function".
/// @param[out] out_future An optional @c ::ArFuture which can be @ref
///   future_polling "polled".
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_ILLEGAL_STATE if @p earth is @c #AR_TRACKING_STATE_STOPPED
///   and
///   @c ::ArEarthState is not AR_EARTH_STATE_ENABLED due to @c
///   #AR_GEOSPATIAL_MODE_DISABLED configured on the @c ::ArSession.
///   Reacquire @c ::ArEarth if geospatial mode was reenabled.
/// - @c #AR_ERROR_INVALID_ARGUMENT if @p latitude is outside the allowable
///   range, or if either @p session, @p earth, or @p eus_quaternion_4 is @c
///   NULL, or both @p callback and @p out_future are null.
/// - @c #AR_ERROR_RESOURCE_EXHAUSTED if too many Rooftop and Terrain anchors
///   are currently held.
ArStatus ArEarth_resolveAnchorOnRooftopAsync(
    ArSession *session,
    ArEarth *earth,
    double latitude,
    double longitude,
    double altitude_above_rooftop,
    const float *eus_quaternion_4,
    void *context,
    ArResolveAnchorOnRooftopCallback callback,
    ArResolveAnchorOnRooftopFuture **out_future);

/// @ingroup ArVpsAvailabilityFuture
/// The result of @c ::ArSession_checkVpsAvailabilityAsync, obtained by @c
/// ::ArVpsAvailabilityFuture_getResult or from an invocation of an @c
/// #ArVpsAvailabilityCallback.
AR_DEFINE_ENUM(ArVpsAvailability){
    /// The request to the remote service is not yet completed, so the
    /// availability is not yet known.
    AR_VPS_AVAILABILITY_UNKNOWN = 0,
    /// VPS is available at the requested location.
    AR_VPS_AVAILABILITY_AVAILABLE = 1,
    /// VPS is not available at the requested location.
    AR_VPS_AVAILABILITY_UNAVAILABLE = 2,
    /// An internal error occurred while determining availability.
    AR_VPS_AVAILABILITY_ERROR_INTERNAL = -1,
    /// The external service could not be reached due to a network connection
    /// error.
    AR_VPS_AVAILABILITY_ERROR_NETWORK_CONNECTION = -2,
    /// An authorization error occurred when communicating with the Google Cloud
    /// ARCore API. See <a
    /// href="https://developers.google.com/ar/develop/c/geospatial/enable">Enable
    /// the Geospatial API</a> for troubleshooting steps.
    AR_VPS_AVAILABILITY_ERROR_NOT_AUTHORIZED = -3,
    /// Too many requests were sent.
    AR_VPS_AVAILABILITY_ERROR_RESOURCE_EXHAUSTED = -4,
};

/// @ingroup ArVpsAvailabilityFuture
/// Callback definition for @c ::ArSession_checkVpsAvailabilityAsync. The
/// @p context argument will be the same as that passed to
/// @c ::ArSession_checkVpsAvailabilityAsync. The @p availability argument
/// will be the same as the result obtained from the future returned by
/// @c ::ArSession_checkVpsAvailabilityAsync.
///
/// It is a best practice to free @c context memory provided to @c
/// ::ArSession_checkVpsAvailabilityAsync at the end of the callback
/// implementation.
typedef void (*ArVpsAvailabilityCallback)(void *context,
                                          ArVpsAvailability availability);

/// @ingroup ArVpsAvailabilityFuture
/// Deprecated alias of @c ::ArVpsAvailabilityCallback.
///
/// @deprecated Deprecated in release 1.37.0. Use @c ::ArVpsAvailabilityCallback
/// instead.
typedef ArVpsAvailabilityCallback ArCheckVpsAvailabilityCallback AR_DEPRECATED(
    "Deprecated in release 1.37. "
    "Use ArVpsAvailabilityCallback instead.");

/// @ingroup ArVpsAvailabilityFuture
/// Handle to an asynchronous operation launched by
/// @c ::ArSession_checkVpsAvailabilityAsync.
/// Release with @c ::ArFuture_release.
/// (@ref ownership "reference type, long-lived").
typedef struct ArVpsAvailabilityFuture_ ArVpsAvailabilityFuture;

#ifdef __cplusplus
/// @ingroup type_conversions
/// Upcasts to @c ::ArFuture
inline ArFuture *ArAsFuture(ArVpsAvailabilityFuture *future) {
  return reinterpret_cast<ArFuture *>(future);
}
#endif  // __cplusplus

/// @ingroup ArSession
/// Gets the availability of the Visual Positioning System (VPS) at a specified
/// horizontal position. The availability of VPS in a given location helps to
/// improve the quality of Geospatial localization and tracking accuracy. See <a
/// href="https://developers.google.com/ar/develop/c/geospatial/check-vps-availability">Check
/// VPS Availability</a> for more details.
///
/// This launches an asynchronous operation used to query the Google Cloud
/// ARCore API. See @c ::ArFuture for information on obtaining results and
/// cancelling the operation.
///
/// This may be called without first calling @c ::ArSession_resume
/// or @c ::ArSession_configure, for example to present an "Enter AR" button
/// only when VPS is available.
///
/// Your app must be properly set up to communicate with the Google Cloud ARCore
/// API in order to obtain a result from this call.
///
/// @param[in] session             The ARCore session.
/// @param[in] latitude_degrees    The latitude to query, in degrees.
/// @param[in] longitude_degrees   The longitude to query, in degrees.
/// @param[in] context An optional void pointer passed when using a @ref
///   future_callback "callback".
/// @param[in] callback A @ref future_callback "callback function".
/// @param[out] out_future An optional @c ::ArFuture which can be @ref
///   future_polling "polled".
/// @return @c #AR_SUCCESS, or any of:
/// - @c #AR_ERROR_INTERNET_PERMISSION_NOT_GRANTED if the @c INTERNET permission
///   has not been granted.
/// - @c #AR_ERROR_INVALID_ARGUMENT if @p session is null, or both @p callback
///   and @p out_future are null.
ArStatus ArSession_checkVpsAvailabilityAsync(
    ArSession *session,
    double latitude_degrees,
    double longitude_degrees,
    void *context,
    ArVpsAvailabilityCallback callback,
    ArVpsAvailabilityFuture **out_future);

/// @ingroup ArVpsAvailabilityFuture
/// Gets the state of an asynchronous operation.
///
/// @param[in] session    The ARCore session.
/// @param[in] future     The handle for the asynchronous operation.
/// @param[out] out_state The state of the operation.
void ArVpsAvailabilityFuture_getState(const ArSession *session,
                                      const ArVpsAvailabilityFuture *future,
                                      ArFutureState *out_state)
    AR_DEPRECATED("Deprecated in release 1.37. Use ArFuture_getState instead.");

/// @ingroup ArVpsAvailabilityFuture
/// Returns the result of an asynchronous operation. The returned result is only
/// valid when @c ::ArFuture_getState returns @c #AR_FUTURE_STATE_DONE.
///
/// @param[in] session The ARCore session.
/// @param[in] future The handle for the asynchronous operation.
/// @param[out] out_result_availability The result of the operation, if the
/// Future's state is @c #AR_FUTURE_STATE_DONE.
void ArVpsAvailabilityFuture_getResult(
    const ArSession *session,
    const ArVpsAvailabilityFuture *future,
    ArVpsAvailability *out_result_availability);

/// @ingroup ArVpsAvailabilityFuture
/// Tries to cancel execution of this operation. @p out_was_cancelled will be
/// set to 1 if the operation was cancelled by this invocation, and in that case
/// it is a best practice to free @c context memory provided to @c
/// ::ArSession_checkVpsAvailabilityAsync.
///
/// @param[in] session The ARCore session.
/// @param[in] future The handle for the asynchronous operation.
/// @param[out] out_was_cancelled Set to 1 if this invocation successfully
///     cancelled the operation, 0 otherwise. This may be null.
void ArVpsAvailabilityFuture_cancel(const ArSession *session,
                                    ArVpsAvailabilityFuture *future,
                                    int32_t *out_was_cancelled)
    AR_DEPRECATED("Deprecated in release 1.37. Use ArFuture_cancel instead.");

/// @ingroup ArVpsAvailabilityFuture
/// Releases a reference to a future. This does not mean that the operation will
/// be terminated - see @c ::ArFuture_cancel.
///
/// This function may safely be called with @c NULL - it will do nothing.
void ArVpsAvailabilityFuture_release(ArVpsAvailabilityFuture *future)
    AR_DEPRECATED("Deprecated in release 1.37. Use ArFuture_release instead.");

// === ArGeospatialPose functions ===

/// @ingroup ArGeospatialPose
/// Allocates and initializes a new instance. This function may be used in cases
/// where an instance of this type needs to be created to receive a pose.
/// It must be destroyed with @c ::ArGeospatialPose_destroy after use.
//
/// @param[in] session an @c ::ArSession instance.
/// @param[out] out_pose the new pose.
void ArGeospatialPose_create(const ArSession *session,
                             ArGeospatialPose **out_pose);

/// @ingroup ArGeospatialPose
/// Releases memory used by the given @p pose object.
///
/// @param[in] pose the pose to destroy.
void ArGeospatialPose_destroy(ArGeospatialPose *pose);

/// @ingroup ArGeospatialPose
/// Gets the @c ::ArGeospatialPose's latitude and longitude in degrees, with
/// positive values being north of the equator and east of the prime meridian,
/// as defined by the <a
/// href="https://en.wikipedia.org/wiki/World_Geodetic_System">WGS84
/// specification</a>.
///
/// @param[in]    session                The ARCore session.
/// @param[in]    geospatial_pose        The geospatial pose.
/// @param[out]   out_latitude_degrees   The latitude of the pose in degrees.
/// @param[out]   out_longitude_degrees  The longitude of the pose in degrees.
void ArGeospatialPose_getLatitudeLongitude(
    const ArSession *session,
    const ArGeospatialPose *geospatial_pose,
    double *out_latitude_degrees,
    double *out_longitude_degrees);

/// @ingroup ArGeospatialPose
/// Gets the @c ::ArGeospatialPose's estimated horizontal accuracy in meters
/// with respect to latitude and longitude.
///
/// We define horizontal accuracy as the radius of the 68th percentile
/// confidence level around the estimated horizontal location. In other words,
/// if you draw a circle centered at this @c ::ArGeospatialPose's latitude and
/// longitude, and with a radius equal to the horizontal accuracy, then there is
/// a 68% probability that the true location is inside the circle. Larger
/// numbers indicate lower accuracy.
///
/// For example, if the latitude is 10, longitude is 10, and @p
/// out_horizontal_accuracy_meters is 15, then there is a 68% probability that
/// the true location is within 15 meters of the (10°, 10°) latitude/longitude
/// coordinate.
///
/// @param[in]    session                        The ARCore session.
/// @param[in]    geospatial_pose                The geospatial pose.
/// @param[out]   out_horizontal_accuracy_meters The estimated horizontal
///     accuracy of this @c ::ArGeospatialPose, radial, in meters.
void ArGeospatialPose_getHorizontalAccuracy(
    const ArSession *session,
    const ArGeospatialPose *geospatial_pose,
    double *out_horizontal_accuracy_meters);

/// @ingroup ArGeospatialPose
/// Gets the @c ::ArGeospatialPose's altitude in meters as elevation above the
/// <a href="https://en.wikipedia.org/wiki/World_Geodetic_System">WGS84
/// ellipsoid</a>.
///
/// @param[in]    session                The ARCore session.
/// @param[in]    geospatial_pose        The Geospatial pose.
/// @param[out]   out_altitude_meters    The altitude of the pose in meters
///     above the WGS84 ellipsoid.
void ArGeospatialPose_getAltitude(const ArSession *session,
                                  const ArGeospatialPose *geospatial_pose,
                                  double *out_altitude_meters);

/// @ingroup ArGeospatialPose
/// Gets the @c ::ArGeospatialPose's estimated altitude accuracy.
///
/// We define vertical accuracy as the radius of the 68th percentile confidence
/// level around the estimated altitude. In other words, there is a 68%
/// probability that the true altitude is within @p out_vertical_accuracy_meters
/// of this @c ::ArGeospatialPose's altitude (above or below). Larger numbers
/// indicate lower accuracy.
///
/// For example, if this @c ::ArGeospatialPose's
/// altitude is 100 meters, and @p out_vertical_accuracy_meters is 20 meters,
/// there is a 68% chance that the true altitude is within 20 meters of 100
/// meters.
///
/// @param[in]    session                    The ARCore session.
/// @param[in]    geospatial_pose            The Geospatial pose.
/// @param[out] out_vertical_accuracy_meters The estimated vertical accuracy in
///     meters.
void ArGeospatialPose_getVerticalAccuracy(
    const ArSession *session,
    const ArGeospatialPose *geospatial_pose,
    double *out_vertical_accuracy_meters);

/// @ingroup ArGeospatialPose
/// Gets the @c ::ArGeospatialPose's heading.
///
/// This function will return valid values for @c ::ArGeospatialPose's
/// from @c ::ArEarth_getCameraGeospatialPose and returns 0 for all other @c
/// ::ArGeospatialPose objects.
///
/// Heading is specified in degrees clockwise from true north and approximates
/// the direction the device is facing. The value returned when facing north is
/// 0°, when facing east is 90°, when facing south is +/-180°, and when facing
/// west is -90°.
///
/// The heading approximation is based on the rotation of the device in its
/// current orientation mode (i.e., portrait or landscape) and pitch. For
/// example, when the device is held vertically or upright, the heading is based
/// on the camera optical axis. If the device is held horizontally, looking
/// downwards, the heading is based on the top of the device, with respect to
/// the orientation mode.
///
/// Note: Heading is currently only supported in the device's default
/// orientation mode, which is portrait mode for most supported devices.
///
/// @param[in]    session                The ARCore session.
/// @param[in]    geospatial_pose        The Geospatial pose.
/// @param[out]   out_heading_degrees    The heading component of this pose's
///     orientation in [-180.0, 180.0] degree range.
///
/// @deprecated This function has been deprecated in favor of
/// @c ::ArGeospatialPose_getEastUpSouthQuaternion, which provides orientation
/// values in 3D space. To determine a value analogous to the heading value,
/// calculate the yaw, pitch, and roll values from @c
/// ::ArGeospatialPose_getEastUpSouthQuaternion. When the device is pointing
/// downwards, i.e. perpendicular to the ground, heading is analoguous to roll,
/// and when the device is upright in the device's default orientation mode,
/// heading is analogous to yaw.
void ArGeospatialPose_getHeading(const ArSession *session,
                                 const ArGeospatialPose *geospatial_pose,
                                 double *out_heading_degrees)
    AR_DEPRECATED("Deprecated in release 1.35.0. Please use "
                  "ArGeospatialPose_getEastUpSouthQuaternion instead.");

/// @ingroup ArGeospatialPose
/// Gets the @c ::ArGeospatialPose's estimated heading accuracy.
///
/// This function will return valid values for @c ::ArGeospatialPose's
/// from @c ::ArEarth_getCameraGeospatialPose and returns 0 for all other @c
/// ::ArGeospatialPose's objects.
///
/// We define heading accuracy as the estimated radius of the 68th percentile
/// confidence level around @c ::ArGeospatialPose_getHeading. In other words,
/// there is a 68% probability that the true heading is within @p
/// out_heading_accuracy_degrees of this @c ::ArGeospatialPose's heading. Larger
/// numbers indicate lower accuracy.
///
/// For example, if the estimated heading is 60°, and @p
/// out_heading_accuracy_degrees is 10°, then there is a 68% probability of the
/// true heading being between 50° and 70°.
///
/// @param[in]    session                      The ARCore session.
/// @param[in]    geospatial_pose              The geospatial pose.
/// @param[out]   out_heading_accuracy_degrees The accuracy of the heading
///     confidence in degrees.
///
/// @deprecated This function has deprecated in favor of @c
/// ::ArGeospatialPose_getOrientationYawAccuracy, which provides the accuracy
/// analogous to the heading accuracy when the device is held upright in the
/// default orientation mode.
void ArGeospatialPose_getHeadingAccuracy(
    const ArSession *session,
    const ArGeospatialPose *geospatial_pose,
    double *out_heading_accuracy_degrees)
    AR_DEPRECATED("Deprecated in release 1.35.0. Please use "
                  "ArGeospatialPose_getOrientationYawAccuracy instead.");

/// @ingroup ArGeospatialPose
/// Extracts the orientation from a Geospatial pose. It represents rotation of
/// the target with respect to the east-up-south coordinates. That is, X+ points
/// east, Y+ points up, and Z+ points south. Right handed coordinate system.
///
/// @param[in]  session          The ARCore session.
/// @param[in]  geospatial_pose  The geospatial pose.
/// @param[out] out_quaternion_4 A pointer of 4 float values, which will store
///     the quaternion values as [x, y, z, w].
void ArGeospatialPose_getEastUpSouthQuaternion(
    const ArSession *session,
    const ArGeospatialPose *geospatial_pose,
    float *out_quaternion_4);

/// @ingroup ArGeospatialPose
/// Gets the @c ::ArGeospatialPose's estimated orientation yaw angle accuracy.
/// Yaw rotation is the angle between the pose's compass direction and
/// north, and can be determined from @c
/// ::ArGeospatialPose_getEastUpSouthQuaternion.
///
/// We define yaw accuracy as the estimated radius of the 68th percentile
/// confidence level around yaw angles from
/// @c ::ArGeospatialPose_getEastUpSouthQuaternion. In other words, there is a
/// 68% probability that the true yaw angle is within
/// @p out_orientation_yaw_accuracy_degrees of this @c ::ArGeospatialPose's
/// rotation. Larger numbers indicate lower accuracy.
///
/// For example, if the estimated yaw angle is 60°, and @p
/// out_orientation_yaw_accuracy_degrees is 10°, then there is a 68% probability
/// of the true yaw angle being between 50° and 70°.
///
/// @param[in]    session                      The ARCore session.
/// @param[in]    geospatial_pose              The Geospatial pose.
/// @param[out]   out_orientation_yaw_accuracy_degrees The accuracy of the
///     orientation confidence in degrees.
void ArGeospatialPose_getOrientationYawAccuracy(
    const ArSession *session,
    const ArGeospatialPose *geospatial_pose,
    double *out_orientation_yaw_accuracy_degrees);

// === ArStreetscapeGeometry functions ===

/// @ingroup ArStreetscapeGeometry
/// Gets the origin pose of the @p streetscape_geometry. This pose
/// should be applied to all the points in the geometry from @c
/// ::ArStreetscapeGeometry_acquireMesh.
void ArStreetscapeGeometry_getMeshPose(
    const ArSession *session,
    const ArStreetscapeGeometry *streetscape_geometry,
    ArPose *out_pose);

/// @ingroup ArStreetscapeGeometry
/// Gets the @c ::ArStreetscapeGeometryType corresponding to this geometry.
void ArStreetscapeGeometry_getType(
    const ArSession *session,
    const ArStreetscapeGeometry *streetscape_geometry,
    ArStreetscapeGeometryType *out_type);

/// @ingroup ArStreetscapeGeometry
/// Gets the @c ::ArStreetscapeGeometryQuality corresponding to this geometry.
void ArStreetscapeGeometry_getQuality(
    const ArSession *session,
    const ArStreetscapeGeometry *streetscape_geometry,
    ArStreetscapeGeometryQuality *out_quality);

/// @ingroup ArStreetscapeGeometry
/// Acquires a polygon @c ::ArMesh that corresponds to this geometry. A @c
/// ::ArMesh describes the geometry as a collection of polygons which should be
/// transformed by @c ::ArStreetscapeGeometry_getMeshPose.
///
/// @c ::ArMesh must be released by calling @c ::ArMesh_release.
void ArStreetscapeGeometry_acquireMesh(
    const ArSession *session,
    const ArStreetscapeGeometry *streetscape_geometry,
    ArMesh **out_mesh);

/// @ingroup ArImageMetadata
/// Defines a rational data type in @c ::ArImageMetadata_const_entry.
///
/// Struct matches @c ACameraMetadata_rational in Android NDK r21.
typedef struct ArImageMetadata_rational {
  /// Numerator in the rational data type of this metadata entry.
  int32_t numerator;
  /// Denominator in the rational data type of this metadata entry.
  int32_t denominator;
} ArImageMetadata_rational;

/// @ingroup ArImageMetadata
/// Defines a single read-only image metadata entry.
///
/// Struct matches @c ACameraMetadata_const_entry in Android NDK r21.
typedef struct ArImageMetadata_const_entry {
  /// The tag identifying the entry.
  uint32_t tag;
  /// The data type of this metadata entry. Determines which data pointer in the
  /// @c union below is valid. See @c ACAMERA_TYPE_* enum values in the NDK.
  uint8_t type;
  /// Count of elements (NOT count of bytes) in this metadata entry.
  uint32_t count;

  /// Pointers to the data held in this metadata entry.
  ///
  /// The field @c type defines which union member pointer is valid.
  union {
    /// Pointer to data of single byte or the first element of the byte array.
    const uint8_t *u8;
    /// Pointer to data of single 32-bit int or the first element of the 32-bit
    /// int array.
    const int32_t *i32;
    /// Pointer to data of single 32-bit float or the first element of the
    /// 32-bit float array.
    const float *f;
    /// Pointer to data of single 64-bit int or the first element of the 64-bit
    /// int array.
    const int64_t *i64;
    /// Pointer to data of single double or the first element of the double
    /// array.
    const double *d;
    /// Pointer to data of single @c ::ArImageMetadata_rational or the first
    /// element of the @c ::ArImageMetadata_rational array.
    const ArImageMetadata_rational *r;
  } data;
} ArImageMetadata_const_entry;

/// @ingroup ArImageMetadata
/// Retrieves the list of the supported image metadata tags that can be queried
/// for their value.
///
/// The @p out_tags list remains valid until @p image_metadata is released via
/// @c ::ArImageMetadata_release.
///
/// @param[in]  session             The ARCore session.
/// @param[in]  image_metadata      @c ::ArImageMetadata struct obtained from
///     @c ::ArFrame_acquireImageMetadata.
/// @param[out] out_number_of_tags  Number of metadata tags returned in the
///     list.
/// @param[out] out_tags            The data pointer to the beginning of the
///     list of @c uint32_t tags.
// TODO(b/161001774) Finalize documentation
void ArImageMetadata_getAllKeys(const ArSession *session,
                                const ArImageMetadata *image_metadata,
                                int32_t *out_number_of_tags,
                                const uint32_t **out_tags);

/// @ingroup ArImageMetadata
/// Get a metadata entry for the provided @c ::ArImageMetadata and tag.
///
/// The returned @p out_metadata_entry remains valid until the provided @p
/// image_metadata is released via @c ::ArImageMetadata_release.
///
/// @param[in]  session             The ARCore session.
/// @param[in]  image_metadata      @c ::ArImageMetadata struct obtained from
///     @c ::ArFrame_acquireImageMetadata.
/// @param[in]  tag                 The desired @c uint32_t metadata tag to be
///     retrieved from the provided @c ::ArImageMetadata struct.
/// @param[out] out_metadata_entry  The @c ::ArImageMetadata_const_entry struct
///     to which the metadata tag data should be written to, updated only when
///     function returns @c #AR_SUCCESS.
///
/// @return @c #AR_SUCCESS or any of:
/// - @c #AR_ERROR_INVALID_ARGUMENT - if either @p session, @p image_metadata or
///   @p out_metadata_entry is null.
/// - @c #AR_ERROR_METADATA_NOT_FOUND - if @p image_metadata does not contain an
///   entry of the @p tag value.
// TODO(b/161001774) Finalize documentation
ArStatus ArImageMetadata_getConstEntry(
    const ArSession *session,
    const ArImageMetadata *image_metadata,
    uint32_t tag,
    ArImageMetadata_const_entry *out_metadata_entry);

#undef AR_DEFINE_ENUM

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // ARCORE_C_API_H_
