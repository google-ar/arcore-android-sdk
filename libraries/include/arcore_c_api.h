/*
 * Copyright 2017 Google LLC
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
/// For example, ::ArAnchorList, which is a value type, will hold references to
/// anchors, which are long-lived objects.
///
/// @section spaces Poses and coordinate spaces
///
/// An ::ArPose describes an rigid transformation from one coordinate space
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
/// Note: There is no runtime checking that casts are correct. Call
/// ::ArTrackable_getType beforehand to figure out the correct cast.

/// @defgroup ArAnchor ArAnchor
/// Describes a fixed location and orientation in the real world, representing
/// local and Cloud Anchors.

/// @defgroup ArAugmentedFace ArAugmentedFace
/// Describes a face detected by ARCore and provides functions to access
/// additional center and face region poses as well as face mesh related data.
///
/// Augmented Faces supports front-facing (selfie) camera only, and does not
/// support attaching anchors nor raycast hit testing. Calling
/// ::ArTrackable_acquireNewAnchor will return #AR_ERROR_ILLEGAL_STATE.
#endif  // __cplusplus

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

/// @defgroup ArConfig ArConfig
/// Session configuration.
///
/// To configure an ::ArSession:
///
/// 1. Use ::ArConfig_create to create an ::ArConfig object.
/// 2. Call any number of configuration functions on the newly created object.
/// 3. To apply the configuration to the session, use ::ArSession_configure.
/// 4. To release the memory used by the ::ArConfig object, use
///    ::ArConfig_destroy.
///
/// Note: None of the `ArConfig_set*()` functions will actually affect the state
/// of the given ::ArSession until ::ArSession_configure is called.

/// @defgroup ArCoreApk ArCoreApk
/// Functions for installing and updating "Google Play Services for AR" (ARCore)
/// and determining whether the current device is an
/// <a href="https://developers.google.com/ar/discover/supported-devices">ARCore
/// supported device</a>.

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
/// ::ArFrame_hitTestInstantPlacement.
///
/// If ARCore has an accurate 3D pose for the ::ArInstantPlacementPoint returned
/// by ::ArFrame_hitTestInstantPlacement it will start with tracking method
/// #AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_FULL_TRACKING. Otherwise, it
/// will start with tracking method
/// #AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_SCREENSPACE_WITH_APPROXIMATE_DISTANCE,<!--NOLINT-->
/// and will transition to
/// #AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_FULL_TRACKING
/// once ARCore has an accurate 3D pose. Once the tracking method is
/// #AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_FULL_TRACKING it will not revert
/// to
/// #AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_SCREENSPACE_WITH_APPROXIMATE_DISTANCE.<!--NOLINT-->
///
/// When the tracking method changes from
/// #AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_SCREENSPACE_WITH_APPROXIMATE_DISTANCE<!--NOLINT-->
/// in one frame to #AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_FULL_TRACKING in
/// the next frame, the pose will jump from its initial location based on the
/// provided approximate distance to a new location at an accurate distance.
///
/// This instantaneous change in pose will change the apparent scale of any
/// objects that are anchored to the ::ArInstantPlacementPoint. That is, an
/// object will suddenly appear larger or smaller than it was in the previous
/// frame.
///
/// To avoid the visual jump due to the sudden change in apparent object scale,
/// use the following procedure:
/// 1. Keep track of the pose and tracking method of the
///    ::ArInstantPlacementPoint in each frame.
/// 2. Wait for the tracking method to change to
///    #AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_FULL_TRACKING.
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

/// @defgroup ArSegmentation ArSegmentation
/// Segmentation of people from the background camera image.

/// @defgroup ArSession ArSession
/// Session management.

/// @defgroup ArTrackable ArTrackable
/// Something that can be tracked and that anchors can be attached to.

/// @ingroup ArConfig
/// An opaque session configuration object (@ref ownership "value type").
///
/// - Create with: ::ArConfig_create
/// - Release with: ::ArConfig_destroy
typedef struct ArConfig_ ArConfig;

// CameraConfig objects and list.

/// @ingroup ArCameraConfig
/// A camera config struct that contains the config supported by
/// the physical camera obtained from the low level device profiles.
/// (@ref ownership "value type").
///
/// - Create with: ::ArCameraConfig_create
/// - Release with: ::ArCameraConfig_destroy
typedef struct ArCameraConfig_ ArCameraConfig;

/// @ingroup ArCameraConfig
/// A list of camera config (@ref ownership "value type").
///
/// - Create with: ::ArCameraConfigList_create
/// - Release with: ::ArCameraConfigList_destroy
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
/// - Create with: ::ArCameraConfigFilter_create
/// - Release with: ::ArCameraConfigFilter_destroy
typedef struct ArCameraConfigFilter_ ArCameraConfigFilter;

/// @ingroup ArRecordingConfig
/// A recording config struct that contains the config to set the recorder.
///
/// (@ref ownership "value type").
///
/// - Create with: ::ArRecordingConfig_create
/// - Release with: ::ArRecordingConfig_destroy
typedef struct ArRecordingConfig_ ArRecordingConfig;

/// @ingroup ArSession
/// The ARCore session (@ref ownership "value type").
///
/// - Create with: ::ArSession_create
/// - Release with: ::ArSession_destroy
typedef struct ArSession_ ArSession;

/// @ingroup ArPose
/// A structured rigid transformation (@ref ownership "value type").
///
/// - Allocate with: ::ArPose_create
/// - Release with: ::ArPose_destroy
typedef struct ArPose_ ArPose;

// Camera.

/// @ingroup ArCamera
/// The virtual and physical camera
/// (@ref ownership "reference type, long-lived").
///
/// - Acquire with: ::ArFrame_acquireCamera
/// - Release with: ::ArCamera_release
typedef struct ArCamera_ ArCamera;

// === Camera intrinstics types and functions ===

/// @ingroup ArCameraIntrinsics
/// The physical characteristics of a given camera.
///
/// - Allocate with: ::ArCameraIntrinsics_create
/// - Release with: ::ArCameraIntrinsics_destroy
typedef struct ArCameraIntrinsics_ ArCameraIntrinsics;

// Frame and frame objects.

/// @ingroup ArFrame
/// The world state resulting from an update (@ref ownership "value type").
///
/// - Create with: ::ArFrame_create
/// - Allocate with: ::ArSession_update
/// - Release with: ::ArFrame_destroy
typedef struct ArFrame_ ArFrame;

// LightEstimate.

/// @ingroup ArLightEstimate
/// An estimate of the real-world lighting (@ref ownership "value type").
///
/// - Create with: ::ArLightEstimate_create
/// - Allocate with: ::ArFrame_getLightEstimate
/// - Release with: ::ArLightEstimate_destroy
typedef struct ArLightEstimate_ ArLightEstimate;

// PointCloud.

/// @ingroup ArPointCloud
/// A cloud of tracked 3D visual feature points
/// (@ref ownership "reference type, large data").
///
/// - Acquire with: ::ArFrame_acquirePointCloud
/// - Release with: ::ArPointCloud_release
typedef struct ArPointCloud_ ArPointCloud;

// ImageMetadata.

/// @ingroup ArImageMetadata
/// Camera capture metadata (@ref ownership "reference type, large data").
///
/// - Acquire with: ::ArFrame_acquireImageMetadata
/// - Release with: ::ArImageMetadata_release
typedef struct ArImageMetadata_ ArImageMetadata;

/// @ingroup ArImage
/// Accessing CPU image from the camera
/// (@ref ownership "reference type, large data").
///
/// - Acquire with: ::ArFrame_acquireCameraImage
/// - Release with: ::ArImage_release.
/// Convert to NDK @c AImage with ::ArImage_getNdkImage
typedef struct ArImage_ ArImage;

/// @ingroup ArImage
/// Convenient definition for cubemap image storage where it is a fixed size
/// array of 6 ::ArImage.
typedef ArImage *ArImageCubemap[6];

/// @ingroup ArImage
/// Forward declaring the Android NDK @c AImage struct, which is used
/// in ::ArImage_getNdkImage.
typedef struct AImage AImage;

// Trackables.

/// @ingroup ArTrackable
/// Trackable base type (@ref ownership "reference type, long-lived").
typedef struct ArTrackable_ ArTrackable;

/// @ingroup ArTrackable
/// A list of ::ArTrackable's (@ref ownership "value type").
///
/// - Create with: ::ArTrackableList_create
/// - Release with: ::ArTrackableList_destroy
typedef struct ArTrackableList_ ArTrackableList;

// Planes.

/// @ingroup ArPlane
/// A detected planar surface (@ref ownership "reference type, long-lived").
///
/// - Trackable type: #AR_TRACKABLE_PLANE
/// - Release with: ::ArTrackable_release
typedef struct ArPlane_ ArPlane;

// Points.

/// @ingroup ArPoint
/// An arbitrary point in space (@ref ownership "reference type, long-lived").
///
/// - Trackable type: #AR_TRACKABLE_POINT
/// - Release with: ::ArTrackable_release
typedef struct ArPoint_ ArPoint;

// Instant Placement points.

/// @ingroup ArInstantPlacementPoint
/// (@ref ownership "reference type, long-lived").
///
/// - Trackable type: #AR_TRACKABLE_INSTANT_PLACEMENT_POINT
/// - Release with: ::ArTrackable_release
typedef struct ArInstantPlacementPoint_ ArInstantPlacementPoint;

// Augmented Images.

/// @ingroup ArAugmentedImage
/// An image that has been detected and tracked
/// (@ref ownership "reference type, long-lived").
///
/// - Trackable type: #AR_TRACKABLE_AUGMENTED_IMAGE
/// - Release with: ::ArTrackable_release
typedef struct ArAugmentedImage_ ArAugmentedImage;

// Augmented Faces.

/// @ingroup ArAugmentedFace
/// A detected face trackable (@ref ownership "reference type, long-lived").
///
/// - Trackable type: #AR_TRACKABLE_FACE
/// - Release with: ::ArTrackable_release
typedef struct ArAugmentedFace_ ArAugmentedFace;

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
/// #AR_TRACKING_STATE_TRACKING/#AR_TRACKING_STATE_PAUSED state will immediately
/// be set to the #AR_TRACKING_STATE_STOPPED state if a different or @c NULL
/// image database is made active in the current session Config.
///
/// - Create with: ::ArAugmentedImageDatabase_create or
///   ::ArAugmentedImageDatabase_deserialize
/// - Release with: ::ArAugmentedImageDatabase_destroy
typedef struct ArAugmentedImageDatabase_ ArAugmentedImageDatabase;

// Anchors.

/// @ingroup ArAnchor
/// A position in space attached to a trackable
/// (@ref ownership "reference type, long-lived").
///
/// - To create a new anchor call ::ArSession_acquireNewAnchor or
///   ::ArHitResult_acquireNewAnchor.
/// - To have ARCore stop tracking the anchor, call ::ArAnchor_detach.
/// - To release the memory associated with this anchor reference, call
///   ::ArAnchor_release. Note that this will not cause ARCore to stop tracking
///   the anchor. Other references to the same anchor acquired through
///   ::ArAnchorList_acquireItem are unaffected.
typedef struct ArAnchor_ ArAnchor;

/// @ingroup ArAnchor
/// A list of anchors (@ref ownership "value type").
///
/// - Create with: ::ArAnchorList_create
/// - Release with: ::ArAnchorList_destroy
typedef struct ArAnchorList_ ArAnchorList;

// Hit result functionality.

/// @ingroup ArHitResult
/// A single trackable hit (@ref ownership "value type").
///
/// - Create with: ::ArHitResult_create
/// - Allocate with: ::ArHitResultList_getItem
/// - Release with: ::ArHitResult_destroy
typedef struct ArHitResult_ ArHitResult;

/// @ingroup ArHitResult
/// A list of hit test results (@ref ownership "value type").
///
/// - Create with: ::ArHitResultList_create
/// - Release with: ::ArHitResultList_destroy
typedef struct ArHitResultList_ ArHitResultList;

/// @ingroup ArImageMetadata
/// Forward declaring the @c ACameraMetadata struct from Android NDK, which is
/// used in ::ArImageMetadata_getNdkCameraMetadata.
typedef struct ACameraMetadata ACameraMetadata;

#ifdef __cplusplus
/// @ingroup type_conversions
/// Upcasts to ::ArTrackable
inline ArTrackable *ArAsTrackable(ArPlane *plane) {
  return reinterpret_cast<ArTrackable *>(plane);
}

/// @ingroup type_conversions
/// Upcasts to ::ArTrackable
inline ArTrackable *ArAsTrackable(ArPoint *point) {
  return reinterpret_cast<ArTrackable *>(point);
}

/// @ingroup type_conversions
/// Upcasts to ::ArTrackable
inline ArTrackable *ArAsTrackable(ArAugmentedImage *augmented_image) {
  return reinterpret_cast<ArTrackable *>(augmented_image);
}

/// @ingroup type_conversions
/// Downcasts to ::ArPlane.
inline ArPlane *ArAsPlane(ArTrackable *trackable) {
  return reinterpret_cast<ArPlane *>(trackable);
}

/// @ingroup type_conversions
/// Downcasts to ::ArPoint.
inline ArPoint *ArAsPoint(ArTrackable *trackable) {
  return reinterpret_cast<ArPoint *>(trackable);
}

/// @ingroup type_conversions
/// Upcasts to ::ArTrackable.
inline ArTrackable *ArAsTrackable(
    ArInstantPlacementPoint *instant_placement_point) {
  return reinterpret_cast<ArTrackable *>(instant_placement_point);
}

/// @ingroup type_conversions
/// Downcasts to ::ArInstantPlacementPoint.
inline ArInstantPlacementPoint *ArAsInstantPlacementPoint(
    ArTrackable *trackable) {
  return reinterpret_cast<ArInstantPlacementPoint *>(trackable);
}

/// @ingroup type_conversions
/// Downcasts to ::ArAugmentedImage.
inline ArAugmentedImage *ArAsAugmentedImage(ArTrackable *trackable) {
  return reinterpret_cast<ArAugmentedImage *>(trackable);
}

/// @ingroup type_conversions
/// Upcasts to ::ArTrackable
inline ArTrackable *ArAsTrackable(ArAugmentedFace *face) {
  return reinterpret_cast<ArTrackable *>(face);
}

/// @ingroup type_conversions
/// Downcasts to ::ArAugmentedFace
inline ArAugmentedFace *ArAsFace(ArTrackable *trackable) {
  return reinterpret_cast<ArAugmentedFace *>(trackable);
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
    /// The base Trackable type. Can be passed to ::ArSession_getAllTrackables
    /// and ::ArFrame_getUpdatedTrackables as the @p filter_type to get
    /// all/updated Trackables of all types.
    AR_TRACKABLE_BASE_TRACKABLE = 0x41520100,

    /// The ::ArPlane subtype of Trackable.
    AR_TRACKABLE_PLANE = 0x41520101,

    /// The ::ArPoint subtype of Trackable.
    AR_TRACKABLE_POINT = 0x41520102,

    /// The ::ArAugmentedImage subtype of Trackable.
    AR_TRACKABLE_AUGMENTED_IMAGE = 0x41520104,

    /// Trackable type for faces.
    AR_TRACKABLE_FACE = 0x41520105,

    /// Trackable type for results retrieved from
    /// ::ArFrame_hitTestInstantPlacement. This trackable type is only available
    /// when when ::ArConfig_setInstantPlacementMode is
    /// #AR_INSTANT_PLACEMENT_MODE_LOCAL_Y_UP.
    AR_TRACKABLE_INSTANT_PLACEMENT_POINT = 0x41520112,

    /// An invalid Trackable type.
    AR_TRACKABLE_NOT_VALID = 0};

/// @ingroup ArSession
/// Feature names for use with ::ArSession_createWithFeatures
///
/// All currently defined features are mutually compatible.
AR_DEFINE_ENUM(ArSessionFeature){
    /// Indicates the end of a features list. This must be the last entry in the
    /// array passed to ::ArSession_createWithFeatures.
    AR_SESSION_FEATURE_END_OF_LIST = 0,

    /// Use the front-facing (selfie) camera. When the front camera is selected,
    /// ARCore's behavior changes in the following ways:
    ///
    /// - The display will be mirrored. Specifically,
    ///   ::ArCamera_getProjectionMatrix will include a horizontal flip in the
    ///   generated projection matrix and APIs that reason about things in
    ///   screen
    ///   space, such as ::ArFrame_transformCoordinates2d, will mirror screen
    ///   coordinates. Open GL apps should consider using \c glFrontFace to
    ///   render mirrored assets without changing their winding direction.
    /// - ::ArCamera_getTrackingState will always output
    ///   #AR_TRACKING_STATE_PAUSED.
    /// - ::ArFrame_hitTest will always output an empty list.
    /// - ::ArCamera_getDisplayOrientedPose will always output an identity pose.
    /// - ::ArSession_acquireNewAnchor will always return
    /// #AR_ERROR_NOT_TRACKING.
    /// - Planes will never be detected.
    /// - ::ArSession_configure will fail if the supplied configuration requests
    ///   Cloud Anchors, Augmented Images, or Environmental HDR Lighting
    ///   Estimation mode.
    AR_SESSION_FEATURE_FRONT_CAMERA = 1,
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
    /// #AR_TRACKING_STATE_TRACKING state, but the session was not.
    AR_ERROR_NOT_TRACKING = -5,

    /// A texture name was not set by calling ::ArSession_setCameraTextureName
    /// before the first call to ::ArSession_update.
    AR_ERROR_TEXTURE_NOT_SET = -6,

    /// An operation required GL context but one was not available.
    AR_ERROR_MISSING_GL_CONTEXT = -7,

    /// The configuration supplied to ::ArSession_configure is unsupported.
    /// To avoid this error, ensure that Session_checkSupported() returns true.
    AR_ERROR_UNSUPPORTED_CONFIGURATION = -8,

    /// The application does not have Android camera permission.
    AR_ERROR_CAMERA_PERMISSION_NOT_GRANTED = -9,

    /// Acquire failed because the object being acquired was already released.
    /// For example, this happens if the application holds an ::ArFrame beyond
    /// the next call to ::ArSession_update, and then tries to acquire its Point
    /// Cloud.
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

    /// ::ArSession_configure failed because the specified configuration
    /// required the Android INTERNET permission, which the application did not
    /// have.
    AR_ERROR_INTERNET_PERMISSION_NOT_GRANTED = -15,

    /// ::ArSession_hostAndAcquireNewCloudAnchor failed because the anchor is
    /// not
    /// a type of anchor that is currently supported for hosting.
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
    /// developer. For example, ::ArSession_resume will return this status if
    /// the camera configuration was changed and there is at least one
    /// unreleased image.
    AR_ERROR_ILLEGAL_STATE = -20,

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
/// Describes the tracking state of an ::ArTrackable, an ::ArAnchor or the
/// ::ArCamera.
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
/// Describes possible tracking failure reasons of an ::ArCamera.
AR_DEFINE_ENUM(ArTrackingFailureReason){
    /// Indicates expected motion tracking behavior. Always returned when
    /// ::ArCamera_getTrackingState is #AR_TRACKING_STATE_TRACKING. When
    /// ::ArCamera_getTrackingState is #AR_TRACKING_STATE_PAUSED, indicates that
    /// the session is initializing normally.
    AR_TRACKING_FAILURE_REASON_NONE = 0,
    /// Motion tracking lost due to bad internal state. No specific user action
    /// is likely to resolve this issue.
    AR_TRACKING_FAILURE_REASON_BAD_STATE = 1,
    /// Motion tracking lost due to poor lighting conditions. Ask the user to
    /// move to a more brightly lit area.
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
    /// to ARCore SDK 1.13, #AR_TRACKING_FAILURE_REASON_NONE is returned in this
    /// case instead.
    AR_TRACKING_FAILURE_REASON_CAMERA_UNAVAILABLE = 5};

/// @ingroup ArAnchor
/// Describes the current cloud state of an ::ArAnchor.
AR_DEFINE_ENUM(ArCloudAnchorState){
    /// The anchor is purely local. It has never been hosted using
    /// ::ArSession_hostAndAcquireNewCloudAnchor, and has not been resolved
    /// using
    /// ::ArSession_resolveAndAcquireNewCloudAnchor.
    AR_CLOUD_ANCHOR_STATE_NONE = 0,

    /// A hosting/resolving task for the anchor is in progress. Once the task
    /// completes in the background, the anchor will get a new cloud state after
    /// the next ::ArSession_update call.
    AR_CLOUD_ANCHOR_STATE_TASK_IN_PROGRESS = 1,

    /// A hosting/resolving task for this anchor completed successfully.
    AR_CLOUD_ANCHOR_STATE_SUCCESS = 2,

    /// A hosting/resolving task for this anchor finished with an internal
    /// error. The app should not attempt to recover from this error.
    AR_CLOUD_ANCHOR_STATE_ERROR_INTERNAL = -1,

    /// The authorization provided by the application is not valid.
    /// - The Google Cloud project may not have enabled the ARCore Cloud Anchor
    ///   API.
    /// - It may fail if the operation you are trying to perform is not allowed.
    /// - When using API key authentication, this will happen if the API key in
    ///   the manifest is invalid, unauthorized or missing. It may also fail if
    ///   the API key is restricted to a set of apps not including the current
    ///   one.
    /// - When using keyless authentication, this will happen if the developer
    ///   fails to create OAuth client. It may also fail if Google Play Services
    ///   isn't installed, is too old, or is malfunctioning for some reason
    ///   (e.g.
    ///   services killed due to memory pressure).
    AR_CLOUD_ANCHOR_STATE_ERROR_NOT_AUTHORIZED = -2,

    AR_CLOUD_ANCHOR_STATE_ERROR_SERVICE_UNAVAILABLE AR_DEPRECATED(
        "AR_CLOUD_ANCHOR_STATE_ERROR_SERVICE_UNAVAILABLE is deprecated in "
        "ARCore SDK 1.12. See release notes to learn more.") = -3,

    /// The application has exhausted the request quota allotted to the given
    /// API key. The developer should request additional quota for the ARCore
    /// Cloud Anchor service for their API key from the Google Developers
    /// Console.
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
    /// to
    /// host it.
    AR_CLOUD_ANCHOR_STATE_ERROR_RESOLVING_SDK_VERSION_TOO_OLD = -8,

    /// The Cloud Anchor could not be resolved because the SDK version used to
    /// resolve the anchor is newer than and incompatible with the version used
    /// to
    /// host it.
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
/// Indicates the outcome of a call to ::ArCoreApk_requestInstall.
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
    /// ::ArCoreApk_requestInstall, skip user education dialog.
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
    /// #AR_LIGHT_ESTIMATION_MODE_ENVIRONMENTAL_HDR is not supported when using
    /// #AR_SESSION_FEATURE_FRONT_CAMERA.
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
    // The dataset recorder is not recording.
    AR_RECORDING_NONE = 0,
    // The dataset recorder is recording normally.
    AR_RECORDING_OK = 1,
    // The dataset recorder encountered an error while recording.
    AR_RECORDING_IO_ERROR = 2,
};

/// @ingroup ArConfig
/// Selects the behavior of ::ArSession_update.
AR_DEFINE_ENUM(ArUpdateMode){
    /// ::ArSession_update will wait until a new camera image is available, or
    /// until the built-in timeout (currently 66ms) is reached. On most devices
    /// the camera is configured to capture 30 frames per second. If the camera
    /// image does not arrive by the built-in timeout, then ::ArSession_update
    /// will return the most recent ::ArFrame object.
    AR_UPDATE_MODE_BLOCKING = 0,
    /// ::ArSession_update will return immediately without blocking. If no new
    /// camera image is available, then ::ArSession_update will return the most
    /// recent
    /// ::ArFrame object.
    AR_UPDATE_MODE_LATEST_CAMERA_IMAGE = 1,
};

/// @ingroup ArConfig
/// Selects the behavior of Augmented Faces subsystem.
/// Default value is #AR_AUGMENTED_FACE_MODE_DISABLED.
AR_DEFINE_ENUM(ArAugmentedFaceMode){
    /// Disable augmented face mode.
    AR_AUGMENTED_FACE_MODE_DISABLED = 0,

    /// Face 3D mesh is enabled. Augmented Faces is currently only
    /// supported when using the front-facing (selfie) camera. See
    /// #AR_SESSION_FEATURE_FRONT_CAMERA for details and additional
    /// restrictions.
    AR_AUGMENTED_FACE_MODE_MESH3D = 2,
};

/// @ingroup ArAugmentedImage
/// Defines the current tracking mode for an Augmented Image. To retrieve the
/// tracking mode for an image use ::ArAugmentedImage_getTrackingMethod.
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
/// use ::ArAugmentedFace_getCenterPose.
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
/// Selects the desired depth mode. Not all devices support all modes, use
/// ::ArSession_isDepthModeSupported to find whether the current device and the
/// selected camera support a particular depth mode.
AR_DEFINE_ENUM(ArDepthMode){
    /// No depth information will be provided. Calling
    /// ::ArFrame_acquireDepthImage will return #AR_ERROR_ILLEGAL_STATE.
    AR_DEPTH_MODE_DISABLED = 0,
    /// On supported devices the best possible depth is estimated based on
    /// hardware and software sources. Available sources of automatic depth are:
    ///  - Depth from motion
    ///  - Active depth cameras
    ///
    /// Provides depth estimation for every pixel in the image, and works best
    /// for
    /// static scenes. For a list of supported devices, see:
    /// https://developers.google.com/ar/discover/supported-devices
    /// Adds significant computational load.
    AR_DEPTH_MODE_AUTOMATIC = 1,
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
/// Tracks the validity of an ::ArLightEstimate object.
AR_DEFINE_ENUM(ArLightEstimateState){
    /// The ::ArLightEstimate is not valid this frame and should not be used
    /// for rendering.
    AR_LIGHT_ESTIMATE_STATE_NOT_VALID = 0,
    /// The ::ArLightEstimate is valid this frame.
    AR_LIGHT_ESTIMATE_STATE_VALID = 1};

/// @ingroup ArPoint
/// Indicates the orientation mode of the ::ArPoint.
AR_DEFINE_ENUM(ArPointOrientationMode){
    /// The orientation of the ::ArPoint is initialized to identity but may
    /// adjust slightly over time.
    AR_POINT_ORIENTATION_INITIALIZED_TO_IDENTITY = 0,
    /// The orientation of the ::ArPoint will follow the behavior described in
    /// ::ArHitResult_getHitPose.
    AR_POINT_ORIENTATION_ESTIMATED_SURFACE_NORMAL = 1};

/// @ingroup ArInstantPlacementPoint
/// Tracking methods for ::ArInstantPlacementPoint.
AR_DEFINE_ENUM(ArInstantPlacementPointTrackingMethod){
    /// The ::ArInstantPlacementPoint is not currently being tracked. The
    /// ::ArTrackingState is #AR_TRACKING_STATE_PAUSED or
    /// #AR_TRACKING_STATE_STOPPED.
    AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_NOT_TRACKING = 0,
    /// The ::ArInstantPlacementPoint is currently being tracked in screen space
    /// and the pose returned by ::ArInstantPlacementPoint_getPose is being
    /// estimated using the approximate distance provided to
    /// ::ArFrame_hitTestInstantPlacement.
    ///
    /// ARCore concurrently tracks at most 20 Instant Placement points that are
    /// #AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_SCREENSPACE_WITH_APPROXIMATE_DISTANCE.<!--NOLINT-->
    /// As additional Instant Placement points with
    /// #AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_SCREENSPACE_WITH_APPROXIMATE_DISTANCE<!--NOLINT-->
    /// are created, the oldest points will become permanently
    /// #AR_TRACKING_STATE_STOPPED in order to maintain the maximum number of
    /// concurrently tracked points.
    AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_SCREENSPACE_WITH_APPROXIMATE_DISTANCE =  // NOLINT
    1,
    /// The ::ArInstantPlacementPoint is being tracked normally and
    /// ::ArInstantPlacementPoint_getPose is using a pose fully determined by
    /// ARCore.
    ///
    /// ARCore doesn't limit the number of Instant Placement points with
    /// #AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_FULL_TRACKING that are being
    /// tracked concurently.
    AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_FULL_TRACKING = 2};

/// @ingroup ArAnchor
/// Indicates the cloud configuration of the ::ArSession.
AR_DEFINE_ENUM(ArCloudAnchorMode){
    /// Cloud Anchors are disabled. This is the value set in the default
    /// ::ArConfig.
    AR_CLOUD_ANCHOR_MODE_DISABLED = 0,
    /// This mode will enable Cloud Anchors. Setting this value and calling
    /// ::ArSession_configure will require the application to have the Android
    /// INTERNET permission.
    AR_CLOUD_ANCHOR_MODE_ENABLED = 1,
};

/// @ingroup ArInstantPlacementPoint
/// Used in ::ArConfig to indicate whether Instant Placement should be enabled
/// or disabled. Default value is #AR_INSTANT_PLACEMENT_MODE_DISABLED.
AR_DEFINE_ENUM(ArInstantPlacementMode){
    /// Instant Placement is disabled.

    /// When Instant Placement is disabled, any ::ArInstantPlacementPoint that
    /// has
    /// #AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_SCREENSPACE_WITH_APPROXIMATE_DISTANCE<!--NOLINT-->
    /// tracking method will result in tracking state becoming permanently
    /// #AR_TRACKING_STATE_STOPPED.
    AR_INSTANT_PLACEMENT_MODE_DISABLED = 0,

    /// Enable Instant Placement. If the hit test is successful,
    /// ::ArFrame_hitTestInstantPlacement will return a single
    /// ::ArInstantPlacementPoint with the +Y pointing upward, against gravity.
    /// Otherwise, returns an empty result set.
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
    ///    the
    ///    wall, other than +Y being up, against gravity.
    ///  - The ::ArInstantPlacementPoint's tracking method may never become
    ///    #AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_FULL_TRACKING or may take
    ///    a
    ///    long time to reach this state. The tracking method remains
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
    /// CPU
    /// image resolution.
    AR_COORDINATES_2D_IMAGE_PIXELS = 2,
    /// CPU image, (x,y) normalized to [0.0f, 1.0f] range.
    AR_COORDINATES_2D_IMAGE_NORMALIZED = 3,
    /// OpenGL Normalized Device Coordinates, display-rotated,
    /// (x,y) normalized to [-1.0f, 1.0f] range.
    AR_COORDINATES_2D_OPENGL_NORMALIZED_DEVICE_COORDINATES = 6,
    /// Android view, display-rotated, (x,y) in pixels.
    AR_COORDINATES_2D_VIEW = 7,
    /// Android view, display-rotated, (x,y) normalized to [0.0f, 1.0f] range.
    AR_COORDINATES_2D_VIEW_NORMALIZED = 8,

};

/// @ingroup ArCameraConfig
/// Describes the direction a camera is facing relative to the device.  Used by
/// ::ArCameraConfig_getFacingDirection.
AR_DEFINE_ENUM(ArCameraConfigFacingDirection){
    /// Camera looks out the back of the device (away from the user).
    AR_CAMERA_CONFIG_FACING_DIRECTION_BACK = 0,
    /// Camera looks out the front of the device (towards the user).  To create
    /// a session using the front-facing (selfie) camera, include
    /// #AR_SESSION_FEATURE_FRONT_CAMERA in the feature list passed to
    /// ::ArSession_createWithFeatures.
    AR_CAMERA_CONFIG_FACING_DIRECTION_FRONT = 1};

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

// Note: Destroy functions do not take ArSession* to allow late destruction in
// finalizers of garbage-collected languages such as Java.

/// @ingroup ArCoreApk
/// Determines if ARCore is supported on this device. This may initiate a query
/// with a remote service to determine if the device is compatible, in which
/// case it will return immediately with @p out_availability set to
/// #AR_AVAILABILITY_UNKNOWN_CHECKING.
///
/// For ARCore-required apps (as indicated by the <a
/// href="https://developers.google.com/ar/develop/c/enable-arcore#ar_required">manifest
/// @c meta-data </a>) this function will assume device compatibility and will
/// always immediately return one of #AR_AVAILABILITY_SUPPORTED_INSTALLED,
/// #AR_AVAILABILITY_SUPPORTED_APK_TOO_OLD, or
/// #AR_AVAILABILITY_SUPPORTED_NOT_INSTALLED.
///
/// Note: A result #AR_AVAILABILITY_SUPPORTED_INSTALLED only indicates presence
/// of a suitably versioned ARCore APK. Session creation may still fail if the
/// ARCore APK has been side-loaded onto an unsupported device.
///
/// May be called prior to ::ArSession_create.
///
/// @param[in] env The application's @c JNIEnv object
/// @param[in] context A @c JOBject for an Android @c Context.
/// @param[out] out_availability A pointer to an ::ArAvailability to receive
///     the result.
void ArCoreApk_checkAvailability(void *env,
                                 void *context,
                                 ArAvailability *out_availability);

/// @ingroup ArCoreApk
/// On supported devices initiates download and installation of
/// Google Play Services for AR (ARCore) and ARCore device profile data, see
/// https://developers.google.com/ar/develop/c/enable-arcore.
///
/// Do not call this function unless ::ArCoreApk_checkAvailability has returned
/// either #AR_AVAILABILITY_SUPPORTED_APK_TOO_OLD or
/// #AR_AVAILABILITY_SUPPORTED_NOT_INSTALLED.
///
/// When your application launches or wishes to enter AR mode, call this
/// function with @p user_requested_install = 1.
///
/// If Google Play Services for AR and device profile data are fully installed
/// and up to date, this function will set @p out_install_status to
/// #AR_INSTALL_STATUS_INSTALLED.
///
/// If Google Play Services for AR or device profile data is not installed or
/// not up to date, the function will set @p out_install_status to
/// #AR_INSTALL_STATUS_INSTALL_REQUESTED and return immediately. The current
/// activity will then pause while the user is prompted to install
/// Google Play Services for AR (market://details?id=com.google.ar.core) and/or
/// ARCore downloads required device profile data.
///
/// When your activity resumes, call this function again, this time with
/// @p user_requested_install = 0. This will either set @p out_install_status
/// to #AR_INSTALL_STATUS_INSTALLED or return an error code indicating the
/// reason that installation could not be completed.
///
/// Once this function returns with @p out_install_status set to
/// #AR_INSTALL_STATUS_INSTALLED, it is safe to call ::ArSession_create.
///
/// Side-loading Google Play Services for AR (ARCore) on unsupported devices
/// will not work. Although ::ArCoreApk_checkAvailability may return
/// #AR_AVAILABILITY_SUPPORTED_APK_TOO_OLD or
/// #AR_AVAILABILITY_SUPPORTED_INSTALLED after side-loading the ARCore APK, the
/// device will still fail to create an AR session, because it is unable to
/// locate the required ARCore device profile data.
///
/// For more control over the message displayed and ease of exiting the process,
/// see ::ArCoreApk_requestInstallCustom.
///
/// <b>Caution:</b> The value of @p *out_install_status is only valid when
/// #AR_SUCCESS is returned.  Otherwise this value must be ignored.
///
/// @param[in] env The application's @c JNIEnv object
/// @param[in] application_activity A @c JObject referencing the application's
///     current Android @c Activity.
/// @param[in] user_requested_install if set, override the previous installation
///     failure message and always show the installation interface.
/// @param[out] out_install_status A pointer to an ::ArInstallStatus to receive
///     the resulting install status, if successful. Value is only valid when
///     the return value is #AR_SUCCESS.
/// @return #AR_SUCCESS, or any of:
/// - #AR_ERROR_FATAL if an error occurs while checking for or requesting
///     installation
/// - #AR_UNAVAILABLE_DEVICE_NOT_COMPATIBLE if ARCore is not supported
///     on this device.
/// - #AR_UNAVAILABLE_USER_DECLINED_INSTALLATION if the user previously declined
///     installation.
ArStatus ArCoreApk_requestInstall(void *env,
                                  void *application_activity,
                                  int32_t user_requested_install,
                                  ArInstallStatus *out_install_status);

/// @ingroup ArCoreApk
/// Initiates installation of Google Play Services for AR (ARCore) and required
/// device profile data, with configurable behavior.
///
/// This is a more flexible version of ::ArCoreApk_requestInstall, allowing the
/// application control over the initial informational dialog and ease of
/// exiting or cancelling the installation.
///
/// Refer to ::ArCoreApk_requestInstall for correct use and expected runtime
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
/// @param[out] out_install_status A pointer to an ::ArInstallStatus to receive
///     the resulting install status, if successful. Value is only valid when
///     the return value is #AR_SUCCESS.
/// @return #AR_SUCCESS, or any of:
/// - #AR_ERROR_FATAL if an error occurs while checking for or requesting
///     installation
/// - #AR_UNAVAILABLE_DEVICE_NOT_COMPATIBLE if ARCore is not supported
///     on this device.
/// - #AR_UNAVAILABLE_USER_DECLINED_INSTALLATION if the user previously declined
///     installation.
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
/// - ::ArCoreApk_requestInstall or ::ArCoreApk_requestInstallCustom returns
///   #AR_INSTALL_STATUS_INSTALLED, or
/// - ::ArCoreApk_checkAvailability returns
/// #AR_AVAILABILITY_SUPPORTED_INSTALLED.
///
/// This check must be performed prior to creating an ::ArSession, otherwise
/// ::ArSession creation will fail, and subsequent installation or upgrade of
/// ARCore will require an app restart and might cause Android to kill your app.
///
/// @param[in]  env                 The application's @c JNIEnv object
/// @param[in]  context A @c JObject for an Android @c Context
/// @param[out] out_session_pointer A pointer to an ::ArSession* to receive
///     the address of the newly allocated session.
/// @return #AR_SUCCESS or any of:
/// - #AR_ERROR_FATAL if an internal error occurred while creating the session.
///   `adb logcat` may contain useful information.
/// - #AR_ERROR_CAMERA_PERMISSION_NOT_GRANTED if your app does not have the
///   [CAMERA](https://developer.android.com/reference/android/Manifest.permission.html#CAMERA)
///   permission.
/// - #AR_UNAVAILABLE_ARCORE_NOT_INSTALLED if the ARCore APK is not present.
///   This can be prevented by the installation check described above.
/// - #AR_UNAVAILABLE_DEVICE_NOT_COMPATIBLE if the device is not compatible with
///   ARCore.  If encountered after completing the installation check, this
///   usually indicates a user has side-loaded ARCore onto an incompatible
///   device.
/// - #AR_UNAVAILABLE_APK_TOO_OLD if the installed ARCore APK is too old for the
///   ARCore SDK with which this application was built. This can be prevented by
///   the installation check described above.
/// - #AR_UNAVAILABLE_SDK_TOO_OLD if the ARCore SDK that this app was built with
///   is too old and no longer supported by the installed ARCore APK.
ArStatus ArSession_create(void *env,
                          void *context,
                          ArSession **out_session_pointer);

/// @ingroup ArSession
/// Creates a new ARCore session requesting additional features.  Prior to
/// calling this function, your app must check that ARCore is installed by
/// verifying that either:
///
/// - ::ArCoreApk_requestInstall or ::ArCoreApk_requestInstallCustom returns
///   #AR_INSTALL_STATUS_INSTALLED, or
/// - ::ArCoreApk_checkAvailability returns
///   #AR_AVAILABILITY_SUPPORTED_INSTALLED.
///
/// This check must be performed prior to creating an ::ArSession, otherwise
/// ::ArSession creation will fail, and subsequent installation or upgrade of
/// ARCore will require an app restart and might cause Android to kill your app.
///
/// @param[in]  env                 The application's @c JNIEnv object
/// @param[in]  context A @c JObject for an Android @c Context
/// @param[in]  features            The list of requested features, terminated
///     by with #AR_SESSION_FEATURE_END_OF_LIST.
/// @param[out] out_session_pointer A pointer to an ::ArSession* to receive
///     the address of the newly allocated session.
/// @return #AR_SUCCESS or any of:
/// - #AR_ERROR_FATAL if an internal error occurred while creating the session.
///   `adb logcat` may contain useful information.
/// - #AR_ERROR_CAMERA_PERMISSION_NOT_GRANTED if your app does not have the
///   [CAMERA](https://developer.android.com/reference/android/Manifest.permission.html#CAMERA)
///   permission.
/// - #AR_ERROR_INVALID_ARGUMENT if the requested features are mutually
///   incompatible.  See ::ArSessionFeature for details.
/// - #AR_UNAVAILABLE_ARCORE_NOT_INSTALLED if the ARCore APK is not present.
///   This can be prevented by the installation check described above.
/// - #AR_UNAVAILABLE_DEVICE_NOT_COMPATIBLE if the device is not compatible with
///   ARCore.  If encountered after completing the installation check, this
///   usually indicates a user has side-loaded ARCore onto an incompatible
///   device.
/// - #AR_UNAVAILABLE_APK_TOO_OLD if the installed ARCore APK is too old for the
///   ARCore SDK with which this application was built. This can be prevented by
///   the installation check described above.
/// - #AR_UNAVAILABLE_SDK_TOO_OLD if the ARCore SDK that this app was built with
///   is too old and no longer supported by the installed ARCore APK.
ArStatus ArSession_createWithFeatures(void *env,
                                      void *context,
                                      const ArSessionFeature *features,
                                      ArSession **out_session_pointer);

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
/// Stores the currently configured ::ArLightEstimationMode mode into
/// @p *light_estimation_mode.
void ArConfig_getLightEstimationMode(
    const ArSession *session,
    const ArConfig *config,
    ArLightEstimationMode *light_estimation_mode);

/// @ingroup ArConfig
/// Sets the desired ::ArLightEstimationMode. See ::ArLightEstimationMode for
/// available options.
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
/// Sets the desired plane finding mode. See ::ArPlaneFindingMode for available
/// options.
void ArConfig_setPlaneFindingMode(const ArSession *session,
                                  ArConfig *config,
                                  ArPlaneFindingMode plane_finding_mode);

/// @ingroup ArConfig
/// Stores the currently configured behavior of ::ArSession_update into
/// @p *update_mode.
void ArConfig_getUpdateMode(const ArSession *session,
                            const ArConfig *config,
                            ArUpdateMode *update_mode);

/// @ingroup ArConfig
/// Sets the behavior of ::ArSession_update. See
/// ::ArUpdateMode for available options.
void ArConfig_setUpdateMode(const ArSession *session,
                            ArConfig *config,
                            ArUpdateMode update_mode);

/// @ingroup ArConfig
/// Gets the current Cloud Anchor mode from the ::ArConfig.
void ArConfig_getCloudAnchorMode(const ArSession *session,
                                 const ArConfig *config,
                                 ArCloudAnchorMode *out_cloud_anchor_mode);

/// @ingroup ArConfig
/// Sets the desired cloud configuration. See ::ArCloudAnchorMode for available
/// options.
void ArConfig_setCloudAnchorMode(const ArSession *session,
                                 ArConfig *config,
                                 ArCloudAnchorMode cloud_anchor_mode);

/// @ingroup ArConfig
/// Sets the image database in the session configuration.
///
/// Any images in the currently active image database that have a
/// #AR_TRACKING_STATE_TRACKING/#AR_TRACKING_STATE_PAUSED state will immediately
/// be set to the #AR_TRACKING_STATE_STOPPED state if a different or @c NULL
/// image database is set.
///
/// This function makes a copy of the image database.
void ArConfig_setAugmentedImageDatabase(
    const ArSession *session,
    ArConfig *config,
    const ArAugmentedImageDatabase *augmented_image_database);

/// @ingroup ArConfig
/// Returns the image database from the session configuration.
///
/// This function returns a copy of the internally stored image database.
void ArConfig_getAugmentedImageDatabase(
    const ArSession *session,
    const ArConfig *config,
    ArAugmentedImageDatabase *out_augmented_image_database);

/// @ingroup ArConfig
/// Stores the currently configured augmented face mode into
/// @p *augmented_face_mode.
void ArConfig_getAugmentedFaceMode(const ArSession *session,
                                   const ArConfig *config,
                                   ArAugmentedFaceMode *augmented_face_mode);

/// @ingroup ArConfig
/// Sets the desired face mode. See ::ArAugmentedFaceMode for
/// available options. Augmented Faces is currently only supported when using
/// the front-facing (selfie) camera.  See #AR_SESSION_FEATURE_FRONT_CAMERA for
/// details.
void ArConfig_setAugmentedFaceMode(const ArSession *session,
                                   ArConfig *config,
                                   ArAugmentedFaceMode augmented_face_mode);

/// @ingroup ArConfig
/// On supported devices, selects the desired camera focus mode. On these
/// devices, the default desired focus mode is currently #AR_FOCUS_MODE_FIXED,
/// although this default might change in the future. See the
/// <a href="https://developers.google.com/ar/discover/supported-devices">ARCore
/// supported devices</a> page for a list of devices on which ARCore does not
/// support changing the desired focus mode.
///
/// For optimal AR tracking performance, use the focus mode provided by the
/// default session config. While capturing pictures or video, use
/// #AR_FOCUS_MODE_AUTO. For optimal AR  tracking, revert to the default focus
/// mode once auto focus behavior is no longer needed. If your app requires
/// fixed focus camera, set #AR_FOCUS_MODE_FIXED before enabling the AR session.
/// This ensures that your app always uses fixed focus, even if the default
/// camera config focus mode changes in a future release.
///
/// To determine whether the configured ARCore camera supports auto focus, check
/// ACAMERA_LENS_INFO_MINIMUM_FOCUS_DISTANCE, which is 0 for fixed-focus
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
/// <a href="https://developers.google.com/ar/discover/supported-devices">ARCore
/// supported devices page</a> for a list of affected devices.
void ArConfig_getFocusMode(const ArSession *session,
                           ArConfig *config,
                           ArFocusMode *focus_mode);

/// @ingroup ArConfig
/// Gets the currently configured desired ::ArDepthMode.
void ArConfig_getDepthMode(const ArSession *session,
                           const ArConfig *config,
                           ArDepthMode *out_depth_mode);

/// @ingroup ArConfig
/// Sets the desired ::ArDepthMode.
///
/// Notes:
/// - Not all devices support all modes. Use ::ArSession_isDepthModeSupported
///   to determine whether the current device and the selected camera support a
///   particular depth mode.
/// - With depth enabled through this call, calls to
///   ::ArFrame_acquireDepthImage can be made to acquire the latest computed
///   depth image.
void ArConfig_setDepthMode(const ArSession *session,
                           ArConfig *config,
                           ArDepthMode mode);

/// @ingroup ArConfig
/// Sets the current Instant Placement mode from the ::ArConfig.
void ArConfig_setInstantPlacementMode(
    const ArSession *session,
    ArConfig *config,
    ArInstantPlacementMode instant_placement_mode);

/// @ingroup ArConfig
/// Gets the current Instant Placement Region mode from the ::ArConfig.
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
/// @param[out]  out_list     A pointer to an ::ArCameraConfigList* to receive
///     the address of the newly allocated ::ArCameraConfigList.
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
/// @param[out]  out_camera_config  Pointer to an ::ArCameraConfig* to receive
///     the address of the newly allocated ::ArCameraConfig.
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
/// one of the values from ::ArCameraConfigDepthSensorUsage enum.
void ArCameraConfig_getDepthSensorUsage(const ArSession *session,
                                        const ArCameraConfig *camera_config,
                                        uint32_t *out_depth_sensor_usage);

/// @ingroup ArCameraConfig
/// Obtains the camera id for the given camera config which is obtained from the
/// list of ARCore compatible camera configs. The acquired ID must be released
/// after use by the ::ArString_release function.
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
    /// Used as a camera filter, via ::ArCameraConfigFilter_setTargetFps.
    AR_CAMERA_CONFIG_TARGET_FPS_30 = 0x0001,

    /// Target 60fps camera capture frame rate.
    ///
    /// Increases power consumption and may increase app memory usage.
    ///
    /// See the ARCore supported devices
    /// (https://developers.google.com/ar/discover/supported-devices)
    /// page for a list of devices that currently support 60fps.
    ///
    /// Used as a camera filter, via ::ArCameraConfigFilter_setTargetFps.
    AR_CAMERA_CONFIG_TARGET_FPS_60 = 0x0002,
};

/// @ingroup ArCameraConfigFilter
/// Depth sensor usage.
AR_DEFINE_ENUM(ArCameraConfigDepthSensorUsage){
    /// When used as a camera filter, via
    /// ::ArCameraConfigFilter_setDepthSensorUsage, filters for camera
    /// configs that require a depth sensor to be present on the device, and
    /// that will be used by ARCore.
    ///
    /// See the ARCore supported devices
    /// (https://developers.google.com/ar/discover/supported-devices)
    /// page for a list of devices that currently have supported depth sensors.
    ///
    /// When returned by ::ArCameraConfig_getDepthSensorUsage, indicates
    /// that a depth sensor is present, and that the camera config will use the
    /// available depth sensor.
    AR_CAMERA_CONFIG_DEPTH_SENSOR_USAGE_REQUIRE_AND_USE = 0x0001,

    /// When used as a camera filter, via
    /// ::ArCameraConfigFilter_setDepthSensorUsage, filters for camera configs
    /// where a depth sensor is not present, or is present but will not be used
    /// by ARCore.
    ///
    /// Most commonly used to filter camera configurations when the app requires
    /// exclusive access to the depth sensor outside of ARCore, for example to
    /// support 3D mesh reconstruction. Available on all ARCore supported
    /// devices.
    ///
    /// When returned by ::ArCameraConfig_getDepthSensorUsage, indicates that
    /// the camera config will not use a depth sensor, even if it is present.
    AR_CAMERA_CONFIG_DEPTH_SENSOR_USAGE_DO_NOT_USE = 0x0002,
};

/// @ingroup ArCameraConfigFilter
/// Stereo camera usage.
// TODO(b/166280987) Finalize documentation
AR_DEFINE_ENUM(ArCameraConfigStereoCameraUsage){
    /// When used as a camera filter, via
    /// ::ArCameraConfigFilter_setStereoCameraUsage, indicates that a stereo
    /// camera must be present on the device, and the stereo multi-camera
    /// (https://source.android.com/devices/camera/multi-camera) must be used by
    /// ARCore. Increases CPU and device power consumption. Not supported on all
    /// devices.
    ///
    /// See the ARCore supported devices
    /// (https://developers.google.com/ar/discover/supported-devices)
    /// page for a list of devices that currently have supported stereo camera
    /// capability.
    ///
    /// When returned by ::ArCameraConfig_getStereoCameraUsage, indicates that a
    /// stereo camera is present on the device and that the camera config will
    /// use the available stereo camera.
    AR_CAMERA_CONFIG_STEREO_CAMERA_USAGE_REQUIRE_AND_USE = 0x0001,

    /// When used as a camera filter, via
    /// ::ArCameraConfigFilter_setStereoCameraUsage, indicates that ARCore will
    /// not attempt to use a stereo multi-camera
    /// (https://source.android.com/devices/camera/multi-camera), even if one is
    /// present. Can be used to limit power consumption. Available on all ARCore
    /// supported devices.
    ///
    /// When returned by ::ArCameraConfig_getStereoCameraUsage, indicates that
    /// the camera config will not use a stereo camera, even if one is present
    /// on the device.
    AR_CAMERA_CONFIG_STEREO_CAMERA_USAGE_DO_NOT_USE = 0x0002,
};

/// @ingroup ArCameraConfig
/// Gets the stereo multi-camera
/// (https://source.android.com/devices/camera/multi-camera) usage settings. @p
/// out_stereo_camera_usage will contain one of the values from
/// ::ArCameraConfigStereoCameraUsage enum.
// TODO(b/166280987) Finalize documentation
void ArCameraConfig_getStereoCameraUsage(
    const ArSession *session,
    const ArCameraConfig *camera_config,
    ArCameraConfigStereoCameraUsage *out_stereo_camera_usage);

/// @ingroup ArCameraConfigFilter
/// Creates a camera config filter object.
///
/// @param[in]   session     The ARCore session
/// @param[out]  out_filter  A pointer to an ::ArCameraConfigFilter* to receive
///     the address of the newly allocated ::ArCameraConfigFilter
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
///     ::ArCameraConfigTargetFps values, bitwise-or'd together
void ArCameraConfigFilter_setTargetFps(const ArSession *session,
                                       ArCameraConfigFilter *filter,
                                       const uint32_t fps_filters);

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
///     multiple ::ArCameraConfigDepthSensorUsage values, bitwise-or'd
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
///     multiple ::ArCameraConfigStereoCameraUsage values, bitwise-or'd together
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

/// @ingroup ArRecordingConfig
/// Creates a dataset recording config object.
///
/// @param[in]   session     The ARCore session
/// @param[out]  out_config  Pointer to an ::ArRecordingConfig* to receive
///     the address of the newly allocated ::ArRecordingConfig
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
/// @param[in]  session                   The ARCore session
/// @param[in]  config                    The config object to query
/// @param[out] out_mp4_dataset_file_path Pointer to an @c char* to receive
///     the address of the newly allocated file path.
void ArRecordingConfig_getMp4DatasetFilePath(const ArSession *session,
                                             const ArRecordingConfig *config,
                                             char **out_mp4_dataset_file_path);

/// @ingroup ArRecordingConfig
/// Sets the file path to save an MP4 dataset file for the recording.
///
/// @param[in] session               The ARCore session
/// @param[in, out] config           The config object to change
/// @param[in] mp4_dataset_file_path A string representing the file path
void ArRecordingConfig_setMp4DatasetFilePath(const ArSession *session,
                                             ArRecordingConfig *config,
                                             const char *mp4_dataset_file_path);

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

// === ArSession functions ===

/// @ingroup ArSession
/// Releases resources used by an ARCore session.
/// This function will take several seconds to complete. To prevent blocking
/// the main thread, call ::ArSession_pause on the main thread, and then call
/// ::ArSession_destroy on a background thread.
///
void ArSession_destroy(ArSession *session);

/// @ingroup ArSession
/// Before release 1.2.0: Checks if the provided configuration is usable on the
/// this device. If this function returns #AR_ERROR_UNSUPPORTED_CONFIGURATION,
/// calls to ::ArSession_configure with this configuration will fail.
///
/// This function now always returns true. See documentation for each
/// configuration entry to know which configuration options & combinations are
/// supported.
///
/// @param[in] session The ARCore session
/// @param[in] config  The configuration to test
/// @return #AR_SUCCESS or:
///  - #AR_ERROR_INVALID_ARGUMENT if any of the arguments are @c NULL.
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
/// Configures the session.
///
/// A session initially has a default configuration. This should be called if a
/// configuration different than default is needed.
///
/// The following configurations are unsupported:
///
/// - When using the (default) back-facing camera:
///   - #AR_AUGMENTED_FACE_MODE_MESH3D.
/// - When using the front-facing (selfie) camera
///   (#AR_SESSION_FEATURE_FRONT_CAMERA):
///   - Any config using ::ArConfig_setAugmentedImageDatabase.
///   - #AR_CLOUD_ANCHOR_MODE_ENABLED.
///   - #AR_LIGHT_ESTIMATION_MODE_ENVIRONMENTAL_HDR.
///
/// @param[in] session The ARCore session.
/// @param[in] config The new configuration setting for the session.
///
/// @return #AR_SUCCESS or any of:
/// - #AR_ERROR_FATAL
/// - #AR_ERROR_UNSUPPORTED_CONFIGURATION if the configuration is not supported.
///   See above restrictions.
/// - #AR_ERROR_INTERNET_PERMISSION_NOT_GRANTED
ArStatus ArSession_configure(ArSession *session, const ArConfig *config);

/// @ingroup ArSession
/// Gets the current config. More specifically, fills the given ::ArConfig
/// object with the copy of the configuration most recently set by
/// ::ArSession_configure. Note: if the session was not explicitly configured, a
/// default configuration is returned (same as ::ArConfig_create).
void ArSession_getConfig(ArSession *session, ArConfig *out_config);

/// @ingroup ArSession
/// Starts or resumes the ARCore Session.
///
/// Typically this should be called from <a
/// href="https://developer.android.com/reference/android/app/Activity.html#onResume()"
/// >@c Activity.onResume() </a>.
///
/// Note that if the camera configuration has been changed by
/// ::ArSession_setCameraConfig since the last call to ::ArSession_resume, all
/// images previously acquired using ::ArFrame_acquireCameraImage must be
/// released by calling ::ArImage_release before calling ::ArSession_resume.  If
/// there are open images, ::ArSession_resume will return
/// #AR_ERROR_ILLEGAL_STATE and the session will not resume.
///
/// @return #AR_SUCCESS or any of:
/// - #AR_ERROR_FATAL
/// - #AR_ERROR_CAMERA_PERMISSION_NOT_GRANTED
/// - #AR_ERROR_CAMERA_NOT_AVAILABLE
/// - #AR_ERROR_ILLEGAL_STATE
ArStatus ArSession_resume(ArSession *session);

/// @ingroup ArSession
/// Pause the current session. This function will stop the camera feed and
/// release resources. The session can be restarted again by calling
/// ::ArSession_resume.
///
/// Typically this should be called from <a
/// href="https://developer.android.com/reference/android/app/Activity.html#onPause()"
/// >@c Activity.onPause() </a>.
///
/// Note that ARCore might continue consuming substantial computing resources
/// for up to 10 seconds after calling this function.
///
/// @return #AR_SUCCESS or any of:
/// - #AR_ERROR_FATAL
ArStatus ArSession_pause(ArSession *session);

/// @ingroup ArSession
/// Sets the OpenGL texture names (IDs) that will be assigned to incoming camera
/// frames in sequence in a ring buffer. The textures must be bound to the
/// @c GL_TEXTURE_EXTERNAL_OES target for use. Shaders accessing these textures
/// must use a @c samplerExternalOES sampler.
///
/// The texture contents are not guaranteed to remain valid after another call
/// to ::ArSession_setCameraTextureName or ::ArSession_setCameraTextureNames,
/// and additionally are not guaranteed to remain valid after a call to
/// ::ArSession_pause or ::ArSession_destroy.
///
/// Passing multiple textures allows for a multithreaded rendering pipeline,
/// unlike ::ArSession_setCameraTextureName.
///
/// Note: this function doesn't fail. If given invalid input, it logs an error
/// without setting the texture names.
///
/// @param[in] session The ARCore session
/// @param[in] number_of_textures The number of textures being passed. This
/// must always be at least 1.
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
/// to ::ArSession_setCameraTextureName or ::ArSession_setCameraTextureNames,
/// and additionally are not guaranteed to remain valid after a call to
/// ::ArSession_pause or ::ArSession_destroy.
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
/// ::ArFrame_getUpdatedTrackables.
///
/// ::ArSession_update in blocking mode (see ::ArUpdateMode) will wait until a
/// new camera image is available, or until the built-in timeout
/// (currently 66ms) is reached.
/// If the camera image does not arrive by the built-in timeout, then
/// ::ArSession_update will return the most recent ::ArFrame object. For some
/// applications it may be important to know if a new frame was actually
/// obtained (for example, to avoid redrawing if the camera did not produce a
/// new frame). To do that, compare the current frame's timestamp, obtained via
/// ::ArFrame_getTimestamp, with the previously recorded frame timestamp. If
/// they are different, this is a new frame.
///
/// During startup the camera system may not produce actual images
/// immediately. In this common case, a frame with timestamp = 0 will be
/// returned.
///
/// @param[in]    session   The ARCore session
/// @param[inout] out_frame The Frame object to populate with the updated world
///     state.  This frame must have been previously created using
///     ::ArFrame_create.  The same ::ArFrame instance may be used when calling
///     this repeatedly.
///
/// @return #AR_SUCCESS or any of:
/// - #AR_ERROR_FATAL
/// - #AR_ERROR_SESSION_PAUSED
/// - #AR_ERROR_TEXTURE_NOT_SET
/// - #AR_ERROR_MISSING_GL_CONTEXT
/// - #AR_ERROR_CAMERA_NOT_AVAILABLE - camera was removed during runtime.
ArStatus ArSession_update(ArSession *session, ArFrame *out_frame);

/// @ingroup ArSession
/// Defines a tracked location in the physical world.
///
/// @return #AR_SUCCESS or any of:
/// - #AR_ERROR_NOT_TRACKING
/// - #AR_ERROR_SESSION_PAUSED
/// - #AR_CLOUD_ANCHOR_STATE_ERROR_RESOURCE_EXHAUSTED
ArStatus ArSession_acquireNewAnchor(ArSession *session,
                                    const ArPose *pose,
                                    ArAnchor **out_anchor);

/// @ingroup ArSession
/// Returns all known anchors, including those not currently tracked. Anchors
/// forgotten by ARCore due to a call to ::ArAnchor_detach or entering the
/// #AR_TRACKING_STATE_STOPPED state will not be included.
///
/// @param[in]    session         The ARCore session
/// @param[inout] out_anchor_list The list to fill.  This list must have already
///     been allocated with ::ArAnchorList_create.  If previously used, the list
///     will first be cleared.
void ArSession_getAllAnchors(const ArSession *session,
                             ArAnchorList *out_anchor_list);

/// @ingroup ArSession
/// Returns the list of all known ::ArTrackable objects.  This includes
/// ::ArPlane objects if plane detection is enabled, as well as ::ArPoint
/// objects created as a side effect of calls to ::ArSession_acquireNewAnchor or
/// ::ArFrame_hitTest.
///
/// @param[in]    session            The ARCore session
/// @param[in]    filter_type        The type(s) of trackables to return.  See
///     ::ArTrackableType for legal values.
/// @param[inout] out_trackable_list The list to fill.  This list must have
///     already been allocated with ::ArTrackableList_create.  If previously
///     used, the list will first be cleared.
void ArSession_getAllTrackables(const ArSession *session,
                                ArTrackableType filter_type,
                                ArTrackableList *out_trackable_list);

/// @ingroup ArAnchor
/// Describes the quality of the visual features seen by ARCore in the preceding
/// few seconds and visible from a desired camera ::ArPose. A higher quality
/// indicates a Cloud Anchor hosted at the current time with the current set of
/// recently seen features will generally be easier to resolve more accurately.
/// For more details, see
/// https://developers.google.com/ar/develop/c/cloud-anchors/overview-c
AR_DEFINE_ENUM(ArFeatureMapQuality){
    /// The quality of features seen from the pose in the preceding
    /// seconds is low. This state indicates that ARCore will likely have more
    /// difficulty resolving (::ArSession_resolveAndAcquireNewCloudAnchor) the
    /// Cloud Anchor. Encourage the user to move the device, so that the desired
    /// position of the Cloud Anchor to be hosted is seen from different angles.
    AR_FEATURE_MAP_QUALITY_INSUFFICIENT = 0,
    /// The quality of features seen from the pose in the preceding few
    /// seconds is likely sufficient for ARCore to successfully resolve
    /// (::ArSession_resolveAndAcquireNewCloudAnchor) a Cloud Anchor, although
    /// the accuracy of the resolved pose will likely be reduced. Encourage the
    /// user to move the device, so that the desired position of the Cloud
    /// Anchor to be hosted is seen from different angles.
    AR_FEATURE_MAP_QUALITY_SUFFICIENT = 1,
    /// The quality of features seen from the pose in the preceding few
    /// seconds is likely sufficient for ARCore to successfully resolve
    /// (::ArSession_resolveAndAcquireNewCloudAnchor) a Cloud Anchor with a high
    /// degree of accuracy.
    AR_FEATURE_MAP_QUALITY_GOOD = 2,
};

/// @ingroup ArSession
/// Estimates the quality of the visual features seen by ARCore in the
/// preceding few seconds and visible from the provided camera pose.
/// Cloud Anchors hosted using higher quality features will generally result
/// in easier and more accurately resolved Cloud Anchor poses.
///
/// @param[in] session The ARCore session.
/// @param[in] pose The camera pose to use in estimating the quality.
/// @param[out] out_feature_map_quality The estimated quality of the visual
///     features seen by ARCore in the preceding few seconds and visible from
///     the provided camera pose.
/// @return #AR_SUCCESS or any of:
/// - #AR_ERROR_INVALID_ARGUMENT
/// - #AR_ERROR_NOT_TRACKING
/// - #AR_ERROR_SESSION_PAUSED
/// - #AR_ERROR_CLOUD_ANCHORS_NOT_CONFIGURED
ArStatus ArSession_estimateFeatureMapQualityForHosting(
    const ArSession *session,
    const ArPose *pose,
    ArFeatureMapQuality *out_feature_map_quality);

/// @ingroup ArSession
/// This creates a new Cloud Anchor using the pose and other metadata from
/// @p anchor.
///
/// If the function returns #AR_SUCCESS, the cloud state of @p out_cloud_anchor
/// will be set to #AR_CLOUD_ANCHOR_STATE_TASK_IN_PROGRESS and the initial pose
/// will be set to the pose of @p anchor. However, the new @p out_cloud_anchor
/// is completely independent of @p anchor, and the poses may diverge over time.
/// If the return value of this function is not #AR_SUCCESS, then
/// @p out_cloud_anchor will be set to @c NULL.
///
/// @param[in]    session          The ARCore session
/// @param[in]    anchor           The anchor to be hosted
/// @param[inout] out_cloud_anchor The new Cloud Anchor
/// @return #AR_SUCCESS or any of:
/// - #AR_ERROR_NOT_TRACKING
/// - #AR_ERROR_SESSION_PAUSED
/// - #AR_ERROR_CLOUD_ANCHORS_NOT_CONFIGURED
/// - #AR_ERROR_RESOURCE_EXHAUSTED
/// - #AR_ERROR_ANCHOR_NOT_SUPPORTED_FOR_HOSTING
ArStatus ArSession_hostAndAcquireNewCloudAnchor(ArSession *session,
                                                const ArAnchor *anchor,
                                                ArAnchor **out_cloud_anchor);

/// @ingroup ArSession
/// This creates a new Cloud Anchor and schedules a task to resolve the anchor's
/// pose using the given Cloud Anchor ID. You don't need to wait for a call to
/// resolve a Cloud Anchor to complete before initiating another call.
/// A session can be resolving up to 40 Cloud Anchors at a given time.
///
/// If this function returns #AR_SUCCESS, the cloud state of @p out_cloud_anchor
/// will be #AR_CLOUD_ANCHOR_STATE_TASK_IN_PROGRESS, and its tracking state will
/// be #AR_TRACKING_STATE_PAUSED. This anchor will never start tracking until
/// its pose has been successfully resolved. If the resolving task ends in an
/// error, the tracking state will be set to #AR_TRACKING_STATE_STOPPED. If the
/// return value is not #AR_SUCCESS, then @p out_cloud_anchor will be set to
/// @c NULL.
///
/// @param[in]    session          The ARCore session
/// @param[in]    cloud_anchor_id  The cloud ID of the anchor to be resolved
/// @param[inout] out_cloud_anchor The new Cloud Anchor
/// @return #AR_SUCCESS or any of:
/// - #AR_ERROR_NOT_TRACKING
/// - #AR_ERROR_SESSION_PAUSED
/// - #AR_ERROR_CLOUD_ANCHORS_NOT_CONFIGURED
/// - #AR_ERROR_RESOURCE_EXHAUSTED
ArStatus ArSession_resolveAndAcquireNewCloudAnchor(ArSession *session,
                                                   const char *cloud_anchor_id,
                                                   ArAnchor **out_cloud_anchor);

/// @ingroup ArSession
/// This creates a new Cloud Anchor with a given lifetime in days, using the
/// pose of the provided @p anchor.
///
/// The cloud state of the returned anchor will be set to
/// #AR_CLOUD_ANCHOR_STATE_TASK_IN_PROGRESS and the initial pose
/// will be set to the pose of the provided @p anchor. However, the returned
/// anchor is completely independent of the original @p anchor, and the two
/// poses might diverge over time.
///
/// Hosting requires an active session for which the ::ArTrackingState
/// is #AR_TRACKING_STATE_TRACKING, as well as a working internet connection.
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
/// @return #AR_SUCCESS or any of:
/// - #AR_ERROR_INVALID_ARGUMENT
/// - #AR_ERROR_NOT_TRACKING
/// - #AR_ERROR_SESSION_PAUSED
/// - #AR_ERROR_CLOUD_ANCHORS_NOT_CONFIGURED
/// - #AR_ERROR_RESOURCE_EXHAUSTED
/// - #AR_ERROR_ANCHOR_NOT_SUPPORTED_FOR_HOSTING
ArStatus ArSession_hostAndAcquireNewCloudAnchorWithTtl(
    ArSession *session,
    const ArAnchor *anchor,
    int32_t ttl_days,
    ArAnchor **out_cloud_anchor);

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
///      been allocated with ::ArCameraConfigList_create.  The list is cleared
///      to remove any existing elements.  Once it is no longer needed, the list
///      must be destroyed using ::ArCameraConfigList_destroy to release
///      allocated memory.
/// @deprecated Deprecated in release 1.11.0. Use
/// ::ArSession_getSupportedCameraConfigsWithFilter instead.
// TODO(b/146903940): Change ArSession_getSupportedCameraConfigs to return
// ArStatus.
void ArSession_getSupportedCameraConfigs(const ArSession *session,
                                         ArCameraConfigList *list)
    AR_DEPRECATED(
        "Deprecated in release 1.11.0. Please see function documentation.");

/// @ingroup ArSession
/// Sets the ::ArCameraConfig that the ::ArSession should use.  Can only be
/// called while the session is paused.  The provided ::ArCameraConfig must be
/// one of the
///  configs returned by ::ArSession_getSupportedCameraConfigsWithFilter.
///
/// The camera config will be applied once the session is resumed.
/// All previously acquired frame images must be released with ::ArImage_release
/// before calling resume(). Failure to do so will cause resume() to return
/// #AR_ERROR_ILLEGAL_STATE error.
///
/// Note: Starting in ARCore 1.12, changing the active camera config may cause
/// the tracking state on certain devices to become permanently
/// #AR_TRACKING_STATE_PAUSED. For consistent behavior across all supported
/// devices, release any previously created anchors and trackables when setting
/// a new camera config.
///
/// @param[in]    session          The ARCore session
/// @param[in]    camera_config    The provided ::ArCameraConfig must be from a
///     list returned by ::ArSession_getSupportedCameraConfigsWithFilter.
/// @return #AR_SUCCESS or any of:
/// - #AR_ERROR_INVALID_ARGUMENT
/// - #AR_ERROR_SESSION_NOT_PAUSED
ArStatus ArSession_setCameraConfig(const ArSession *session,
                                   const ArCameraConfig *camera_config);

/// @ingroup ArSession
/// Gets the ::ArCameraConfig that the ::ArSession is currently using.  If the
/// camera config was not explicitly set then it returns the default
/// camera config.  Use ::ArCameraConfig_destroy to release memory associated
/// with the returned camera config once it is no longer needed.
///
/// @param[in]    session           The ARCore session
/// @param[inout] out_camera_config The camera config object to fill. This
///      object must have already been allocated with ::ArCameraConfig_create.
///      Use ::ArCameraConfig_destroy to release memory associated with
///      @p out_camera_config once it is no longer needed.
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
/// (https://developers.google.com/ar/discover/supported-devices) page for
/// an up to date list of supported devices.
///
/// Element 0 will contain the camera config that best matches the filter
/// settings, according to the following priority:
///
/// 1. Target FPS: prefer #AR_CAMERA_CONFIG_TARGET_FPS_60 over
///    #AR_CAMERA_CONFIG_TARGET_FPS_30
/// 2. Depth sensor usage: prefer
/// #AR_CAMERA_CONFIG_DEPTH_SENSOR_USAGE_REQUIRE_AND_USE over
///    #AR_CAMERA_CONFIG_DEPTH_SENSOR_USAGE_DO_NOT_USE
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
/// Sets a MP4 dataset file to playback instead of live camera feed.
///
/// Restrictions:
/// - Can only be called while the session is paused. Playback of the MP4
/// dataset file will start once the session is resumed.
/// - The MP4 dataset file must use the same camera facing direction as is
/// configured in the session.
///
/// When an MP4 dataset file is set:
/// - All existing trackables (::ArAnchor and ::ArTrackable) immediately enter
/// tracking state #AR_TRACKING_STATE_STOPPED.
/// - The desired focus mode (::ArConfig_setFocusMode) is ignored, and will not
/// affect the previously recorded camera images.
/// - The current camera configuration (::ArCameraConfig) is immediately set to
/// the default for the device the MP4 dataset file was recorded on.
/// - Calls to ::ArSession_getSupportedCameraConfigs will return camera configs
/// supported by the device the MP4 dataset file was recorded on.
/// - Setting a previously obtained camera config to
/// ::ArSession_setCameraConfig will have no effect.
///
/// @param[in] session               The ARCore session
/// @param[in] mp4_dataset_file_path A string file path to a MP4 dataset file
/// or @c NULL to use the live camera feed.
///
/// @return #AR_SUCCESS or any of:
/// - #AR_ERROR_SESSION_NOT_PAUSED if called when session is not paused.
/// - #AR_ERROR_SESSION_UNSUPPORTED if playback is incompatible with selected
/// features.
/// - #AR_ERROR_PLAYBACK_FAILED if an error occurred with the MP4 dataset file
/// such as not being able to open the file or the file is unable to be decoded.
ArStatus ArSession_setPlaybackDataset(ArSession *session,
                                      const char *mp4_dataset_file_path);

/// @ingroup ArRecording
/// Describe the current playback status.
AR_DEFINE_ENUM(ArPlaybackStatus){
    // The session is not playing back an MP4 dataset file.
    AR_PLAYBACK_NONE = 0,
    // Playback is in process without issues.
    AR_PLAYBACK_OK = 1,
    // Playback has stopped due to an error.
    AR_PLAYBACK_IO_ERROR = 2,
    // Playback has finished successfully.
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
/// @param[in] session           The ARCore session
/// @param[in] recording_config  The configuration defined for recording.
///
/// @return #AR_SUCCESS or any of:
/// - #AR_ERROR_ILLEGAL_STATE
/// - #AR_ERROR_INVALID_ARGUMENT
/// - #AR_ERROR_RECORDING_FAILED
ArStatus ArSession_startRecording(ArSession *session,
                                  const ArRecordingConfig *recording_config);

/// @ingroup ArSession
/// Stops recording and flushes unwritten data to disk. The MP4 dataset file
/// will be ready to read after this call.
///
/// Recording can be stopped automatically when ::ArSession_pause is called, if
/// auto stop is enabled via ::ArRecordingConfig_setAutoStopOnPause.
/// Recording errors that would be thrown in stopRecording() are silently
/// ignored in ::ArSession_pause.
///
/// @param[in] session  The ARCore session
///
/// @return #AR_SUCCESS or any of:
/// - #AR_ERROR_RECORDING_FAILED
ArStatus ArSession_stopRecording(ArSession *session);

/// @ingroup ArSession
/// Returns the current recording status.
///
/// @param[in] session The ARCore session.
/// @param[out] out_recording_status The current recording status.
void ArSession_getRecordingStatus(ArSession *session,
                                  ArRecordingStatus *out_recording_status);

/// @ingroup ArSession
/// Checks whether the provided ::ArDepthMode is supported on this device
/// with the selected camera configuration. The current list of supported
/// devices is documented on the <a
/// href="https://developers.google.com/ar/discover/supported-devices">ARCore
/// supported devices</a> page.
///
/// @param[in] session The ARCore session.
/// @param[in] depth_mode The desired depth mode to check.
/// @param[out] out_is_supported Non zero if the depth mode is supported on this
/// device.
void ArSession_isDepthModeSupported(const ArSession *session,
                                    ArDepthMode depth_mode,
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
///     ::ArPose_create.
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
/// * ::ArCamera_getDisplayOrientedPose for the pose of the virtual camera. It
///   will differ by a local rotation about the Z axis by a multiple of 90
///   degrees.
/// * ::ArFrame_getAndroidSensorPose for the pose of the Android sensor frame.
///   It will differ in both orientation and location.
/// * ::ArFrame_transformCoordinates2d to convert viewport coordinates to
///   texture coordinates.
///
/// Note: This pose is only useful when ::ArCamera_getTrackingState returns
/// #AR_TRACKING_STATE_TRACKING and otherwise should not be used.
///
/// @param[in]    session  The ARCore session
/// @param[in]    camera   The session's camera (retrieved from any frame).
/// @param[inout] out_pose An already-allocated ::ArPose object into which the
///     pose will be stored.
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
/// * ::ArCamera_getViewMatrix to conveniently compute the OpenGL view matrix.
/// * ::ArCamera_getPose for the physical pose of the camera. It will differ by
///   a local rotation about the Z axis by a multiple of 90 degrees.
/// * ::ArFrame_getAndroidSensorPose for the pose of the android sensor frame.
///   It will differ in both orientation and location.
/// * ::ArSession_setDisplayGeometry to update the display rotation.
///
/// Note: This pose is only useful when ::ArCamera_getTrackingState returns
/// #AR_TRACKING_STATE_TRACKING and otherwise should not be used.
///
/// @param[in]    session  The ARCore session
/// @param[in]    camera   The session's camera (retrieved from any frame).
/// @param[inout] out_pose An already-allocated ::ArPose object into which the
///     pose will be stored.
void ArCamera_getDisplayOrientedPose(const ArSession *session,
                                     const ArCamera *camera,
                                     ArPose *out_pose);

/// @ingroup ArCamera
/// Returns the view matrix for the camera for this frame. This matrix performs
/// the inverse transform as the pose provided by
/// ::ArCamera_getDisplayOrientedPose.
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
/// anything other than #AR_TRACKING_STATE_TRACKING the pose should not be
/// considered useful. Use ::ArCamera_getTrackingFailureReason to determine the
/// best recommendation to provide to the user to restore motion tracking.
///
/// Note: Starting in ARCore 1.12, changing the active camera config using
/// ::ArSession_setCameraConfig may cause the tracking state on certain
/// devices to become permanently #AR_TRACKING_STATE_PAUSED. For consistent
/// behavior across all supported devices, release any previously created
/// anchors and trackables when setting a new camera config.
void ArCamera_getTrackingState(const ArSession *session,
                               const ArCamera *camera,
                               ArTrackingState *out_tracking_state);

/// @ingroup ArCamera
/// Gets the reason that ::ArCamera_getTrackingState is
/// #AR_TRACKING_STATE_PAUSED.
///
/// Note: This function returns
/// #AR_TRACKING_FAILURE_REASON_NONE briefly after
/// ::ArSession_resume while the motion tracking is initializing.
/// This function always returns
/// #AR_TRACKING_FAILURE_REASON_NONE when
/// ::ArCamera_getTrackingState is #AR_TRACKING_STATE_TRACKING.
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
/// Note: When using #AR_SESSION_FEATURE_FRONT_CAMERA, the returned projection
/// matrix will incorporate a horizontal flip.
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
/// @param[inout] out_camera_intrinsics  The ::ArCameraIntrinsics data.
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
/// @param[inout] out_camera_intrinsics  The ::ArCameraIntrinsics data.
void ArCamera_getTextureIntrinsics(const ArSession *session,
                                   const ArCamera *camera,
                                   ArCameraIntrinsics *out_camera_intrinsics);

/// @ingroup ArCamera
/// Releases a reference to the camera.  This must match a call to
/// ::ArFrame_acquireCamera.
///
/// This function may safely be called with @c NULL - it will do nothing.
void ArCamera_release(ArCamera *camera);

// === ArCameraIntrinsics functions ===
/// @ingroup ArCameraIntrinsics
/// Allocates a camera intrinstics object.
///
/// @param[in]    session                The ARCore session
/// @param[inout] out_camera_intrinsics  The ::ArCameraIntrinsics data.
void ArCameraIntrinsics_create(const ArSession *session,
                               ArCameraIntrinsics **out_camera_intrinsics);

/// @ingroup ArCameraIntrinsics
/// Returns the focal length in pixels.
/// The focal length is conventionally represented in pixels. For a detailed
/// explanation, please see http://ksimek.github.io/2013/08/13/intrinsic.
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
/// Allocates a new ::ArFrame object, storing the pointer into @p *out_frame.
///
/// Note: the same ::ArFrame can be used repeatedly when calling
/// ::ArSession_update.
void ArFrame_create(const ArSession *session, ArFrame **out_frame);

/// @ingroup ArFrame
/// Releases an ::ArFrame and any references it holds.
void ArFrame_destroy(ArFrame *frame);

/// @ingroup ArFrame
/// Checks if the display rotation or viewport geometry changed since the
/// previous call to ::ArSession_update. The application should re-query
/// ::ArCamera_getProjectionMatrix and ::ArFrame_transformCoordinates2d
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
/// * ::ArCamera_getDisplayOrientedPose for the pose of the virtual camera.
/// * ::ArCamera_getPose for the pose of the physical camera.
/// * ::ArFrame_getTimestamp for the system time that this pose was estimated
///   for.
///
/// Note: This pose is only useful when ::ArCamera_getTrackingState returns
/// #AR_TRACKING_STATE_TRACKING and otherwise should not be used.
///
/// @param[in]    session  The ARCore session
/// @param[in]    frame    The current frame.
/// @param[inout] out_pose An already-allocated ::ArPose object into which the
///     pose will be stored.
void ArFrame_getAndroidSensorPose(const ArSession *session,
                                  const ArFrame *frame,
                                  ArPose *out_pose);

/// @ingroup ArFrame
/// Transform the given texture coordinates to correctly show the background
/// image. This accounts for the display rotation, and any additional required
/// adjustment. For performance, this function should be called only if
/// ::ArFrame_getDisplayGeometryChanged indicates a change.
///
/// @param[in]    session      The ARCore session
/// @param[in]    frame        The current frame.
/// @param[in]    num_elements The number of floats to transform.  Must be
///     a multiple of 2.  @p uvs_in and @p uvs_out must point to arrays of at
///     least this many floats.
/// @param[in]    uvs_in       Input UV coordinates in normalized screen space.
/// @param[inout] uvs_out      Output UV coordinates in texture coordinates.
/// @deprecated Deprecated in release 1.7.0. Use
/// ::ArFrame_transformCoordinates2d instead.
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
/// For Android view coordinates (#AR_COORDINATES_2D_VIEW,
/// #AR_COORDINATES_2D_VIEW_NORMALIZED), the view information is taken from the
/// most recent call to ::ArSession_setDisplayGeometry.
///
/// Must be called on the most recently obtained ::ArFrame object. If this
/// function is called on an older frame, a log message will be printed and
/// out_vertices_2d will remain unchanged.
///
/// Some examples of useful conversions:
///  - To transform from [0,1] range to screen-quad coordinates for rendering:
///    #AR_COORDINATES_2D_VIEW_NORMALIZED ->
///    #AR_COORDINATES_2D_TEXTURE_NORMALIZED
///  - To transform from [-1,1] range to screen-quad coordinates for rendering:
///    #AR_COORDINATES_2D_OPENGL_NORMALIZED_DEVICE_COORDINATES ->
///    #AR_COORDINATES_2D_TEXTURE_NORMALIZED
///  - To transform a point found by a computer vision algorithm in a cpu image
///    into a point on the screen that can be used to place an Android View
///    (e.g. Button) at that location:
///    #AR_COORDINATES_2D_IMAGE_PIXELS -> #AR_COORDINATES_2D_VIEW
///  - To transform a point found by a computer vision algorithm in a CPU image
///    into a point to be rendered using GL in clip-space ([-1,1] range):
///    #AR_COORDINATES_2D_IMAGE_PIXELS ->
///    #AR_COORDINATES_2D_OPENGL_NORMALIZED_DEVICE_COORDINATES
///
/// If inputCoordinates is same as outputCoordinates, the input vertices will be
/// copied to the output vertices unmodified.
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
/// Performs a ray cast from the user's device in the direction of the given
/// location in the camera view. Intersections with detected scene geometry are
/// returned, sorted by distance from the device; the nearest intersection is
/// returned first.
///
/// Note: Significant geometric leeway is given when returning hit results. For
/// example, a plane hit may be generated if the ray came close, but did not
/// actually hit within the plane extents or plane bounds
/// (::ArPlane_isPoseInExtents and ::ArPlane_isPoseInPolygon can be used to
/// determine these cases). A point (Point Cloud) hit is generated when a point
/// is roughly within one finger-width of the provided screen coordinates.
///
/// The resulting list is ordered by distance, with the nearest hit first
///
/// Note: If not tracking, the @p hit_result_list will be empty.
///
/// Note: If called on an old frame (not the latest produced by
///     ::ArSession_update the @p hit_result_list will be empty).
///
/// Note: When using #AR_SESSION_FEATURE_FRONT_CAMERA, the returned hit result
///     list will always be empty, as the camera is not
///     #AR_TRACKING_STATE_TRACKING. Hit testing against tracked faces is not
///     currently supported.
///
/// @param[in]    session         The ARCore session.
/// @param[in]    frame           The current frame.
/// @param[in]    pixel_x         Logical X position within the view, as from an
///     Android UI event.
/// @param[in]    pixel_y         Logical Y position within the view, as from an
///     Android UI event.
/// @param[inout] hit_result_list The list to fill.  This list must have been
///     previously allocated using ::ArHitResultList_create.  If the list has
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
/// ::ArInstantPlacementPoint tracking method and the provided
/// @p approximate_distance_meters. A discussion of the different tracking
/// methods and the effects of apparent object scale are described in
/// ::ArInstantPlacementPoint.
///
/// This function will succeed only if ::ArInstantPlacementMode is
/// #AR_INSTANT_PLACEMENT_MODE_LOCAL_Y_UP in the ARCore session configuration,
/// the ARCore session tracking state is #AR_TRACKING_STATE_TRACKING, and there
/// are sufficient feature points to track the point in screen space.
///
/// @param[in]    session         The ARCore session.
/// @param[in]    frame           The current frame.
/// @param[in]    pixel_x         Logical X position within the view, as from an
///     Android UI event.
/// @param[in]    pixel_y         Logical Y position within the view, as from an
///     Android UI event.
/// @param[in]    approximate_distance_meters The distance at which to create an
///     ::ArInstantPlacementPoint. This is only used while the tracking method
///     for the returned point is
///     #AR_INSTANT_PLACEMENT_POINT_TRACKING_METHOD_SCREENSPACE_WITH_APPROXIMATE_DISTANCE.<!--NOLINT-->
/// @param[inout] hit_result_list The list to fill. If successful the list will
///     contain a single ::ArHitResult, otherwise it will be cleared. The
///     ::ArHitResult will have a trackable of type ::ArInstantPlacementPoint.
///     The list must have been previously allocated using
///     ::ArHitResultList_create.
void ArFrame_hitTestInstantPlacement(const ArSession *session,
                                     const ArFrame *frame,
                                     float pixel_x,
                                     float pixel_y,
                                     float approximate_distance_meters,
                                     ArHitResultList *hit_result_list);

/// @ingroup ArFrame
/// Similar to ::ArFrame_hitTest, but takes an arbitrary ray in world space
/// coordinates instead of a screen space point.
///
/// @param[in]    session         The ARCore session.
/// @param[in]    frame           The current frame.
/// @param[in]    ray_origin_3    A pointer to float[3] array containing ray
///     origin in world space coordinates.
/// @param[in]    ray_direction_3 A pointer to float[3] array containing ray
///     direction in world space coordinates. Does not have to be normalized.
/// @param[inout] hit_result_list The list to fill.  This list must have been
///     previously allocated using ::ArHitResultList_create.  If the list has
///     been previously used, it will first be cleared.
void ArFrame_hitTestRay(const ArSession *session,
                        const ArFrame *frame,
                        const float *ray_origin_3,
                        const float *ray_direction_3,
                        ArHitResultList *hit_result_list);

/// @ingroup ArFrame
/// Gets the current ::ArLightEstimate, if Lighting Estimation is enabled.
///
/// @param[in]    session            The ARCore session.
/// @param[in]    frame              The current frame.
/// @param[inout] out_light_estimate The ::ArLightEstimate to fill. This object
///    must have been previously created with ::ArLightEstimate_create.
void ArFrame_getLightEstimate(const ArSession *session,
                              const ArFrame *frame,
                              ArLightEstimate *out_light_estimate);

/// @ingroup ArFrame
/// Acquires the current set of estimated 3d points attached to real-world
/// geometry. A matching call to ::ArPointCloud_release must be made when the
/// application is done accessing the Point Cloud.
///
/// Note: This information is for visualization and debugging purposes only. Its
/// characteristics and format are subject to change in subsequent versions of
/// the API.
///
/// @param[in]  session         The ARCore session.
/// @param[in]  frame           The current frame.
/// @param[out] out_point_cloud Pointer to an ::ArPointCloud* receive the
///     address of the Point Cloud.
/// @return #AR_SUCCESS or any of:
/// - #AR_ERROR_DEADLINE_EXCEEDED if @p frame is not the latest frame from
///   by ::ArSession_update.
/// - #AR_ERROR_RESOURCE_EXHAUSTED if too many Point Clouds are currently held.
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
/// @return #AR_SUCCESS or any of:
/// - #AR_ERROR_DEADLINE_EXCEEDED if @p frame is not the latest frame from
///   by ::ArSession_update.
/// - #AR_ERROR_RESOURCE_EXHAUSTED if too many metadata objects are currently
///   held.
/// - #AR_ERROR_NOT_YET_AVAILABLE if the camera failed to produce metadata for
///   the given frame. Note: this commonly happens for few frames right
///   after ::ArSession_resume due to the camera stack bringup.
ArStatus ArFrame_acquireImageMetadata(const ArSession *session,
                                      const ArFrame *frame,
                                      ArImageMetadata **out_metadata);

/// @ingroup ArFrame
/// Returns the CPU image for the current frame.
/// Caller is responsible for later releasing the image with ::ArImage_release.
/// Not supported on all devices
/// (see https://developers.google.com/ar/discover/supported-devices).
/// Return values:
/// @return #AR_SUCCESS or any of:
/// - #AR_ERROR_INVALID_ARGUMENT - one more input arguments are invalid.
/// - #AR_ERROR_DEADLINE_EXCEEDED - the input frame is not the current frame.
/// - #AR_ERROR_RESOURCE_EXHAUSTED - the caller app has exceeded maximum number
///   of images that it can hold without releasing.
/// - #AR_ERROR_NOT_YET_AVAILABLE - image with the timestamp of the input frame
///   was not found within a bounded amount of time, or the camera failed to
///   produce the image
ArStatus ArFrame_acquireCameraImage(ArSession *session,
                                    ArFrame *frame,
                                    ArImage **out_image);

/// @ingroup ArFrame
/// Gets the set of anchors that were changed by the ::ArSession_update that
/// produced this Frame.
///
/// @param[in]    session            The ARCore session
/// @param[in]    frame              The current frame.
/// @param[inout] out_anchor_list The list to fill.  This list must have already
///     been allocated with ::ArAnchorList_create.  If previously used, the list
///     is cleared first.
void ArFrame_getUpdatedAnchors(const ArSession *session,
                               const ArFrame *frame,
                               ArAnchorList *out_anchor_list);

/// @ingroup ArFrame
/// Gets the set of trackables of a particular type that were changed by the
/// ::ArSession_update call that produced this Frame.
///
/// @param[in]    session            The ARCore session
/// @param[in]    frame              The current frame.
/// @param[in]    filter_type        The type(s) of trackables to return.  See
///     ::ArTrackableType for legal values.
/// @param[inout] out_trackable_list The list to fill.  This list must have
///     already been allocated with ::ArTrackableList_create.  If previously
///     used, the list is cleared first.
void ArFrame_getUpdatedTrackables(const ArSession *session,
                                  const ArFrame *frame,
                                  ArTrackableType filter_type,
                                  ArTrackableList *out_trackable_list);

/// @ingroup ArFrame
/// Attempts to acquire a depth image that corresponds to the current frame.
///
/// The depth image has a single 16-bit plane at index 0. Each pixel contains
/// the distance in millimeters to the camera plane. Currently, only the low
/// order 13 bits are used. The 3 highest order bits are always set to 000.
/// The image plane is stored in big-endian format. The actual resolution of the
/// depth image depends on the device and its display aspect ratio, with sizes
/// typically around 160x120 pixels. This size may change in the future.
///
/// The output depth image can express depth values from 0 millimeters to 8191
/// millimeters. Optimal depth accuracy is achieved between 50 millimeters and
/// 5000 millimeters from the camera. Error increases quadratically as distance
/// from the camera increases.
///
/// Depth is estimated using data from previous frames and the current frame. As
/// the user moves their device through the environment 3D depth data is
/// collected and cached, improving the quality of subsequent  depth images and
/// reducing the error introduced by camera distance.
///
/// If an up to date depth image isn't ready for the current frame, the most
/// recent depth image available from an earlier frame is returned instead.
/// This is only expected to occur on compute-constrained devices. An up to
/// date depth image should typically become available again within a few
/// frames. Compare ::ArImage_getTimestamp depth image timestamp with the
/// ::ArFrame_getTimestamp frame timestamp to determine which camera frame the
/// depth image corresponds to.
///
/// The image must be released with ::ArImage_release once it is no
/// longer needed.
///
/// @param[in]  session                The ARCore session.
/// @param[in]  frame                  The current frame.
/// @param[out] out_depth_image        On successful return, this is filled out
/// with a pointer to an ::ArImage. On error return, this is filled out with
/// @c nullptr.
/// @return #AR_SUCCESS or any of:
/// - #AR_ERROR_INVALID_ARGUMENT if the session, frame, or depth image arguments
/// are invalid.
/// - #AR_ERROR_NOT_YET_AVAILABLE if the number of observed camera frames is not
/// yet sufficient for depth estimation; or depth estimation was not possible
/// due to poor lighting, camera occlusion, or insufficient motion observed.
/// - #AR_ERROR_NOT_TRACKING The session is not in the
/// #AR_TRACKING_STATE_TRACKING state, which is required to acquire depth
/// images.
/// - #AR_ERROR_ILLEGAL_STATE if a supported depth mode was not enabled in
/// Session configuration.
/// - #AR_ERROR_RESOURCE_EXHAUSTED if the caller app has exceeded maximum number
/// of depth images that it can hold without releasing.
/// - #AR_ERROR_DEADLINE_EXCEEDED if the provided Frame is not the current
/// one.
ArStatus ArFrame_acquireDepthImage(const ArSession *session,
                                   const ArFrame *frame,
                                   ArImage **out_depth_image);

/// @ingroup ArFrame
/// Returns the OpenGL ES camera texture name (ID) associated with this frame.
/// This is guaranteed to be one of the texture names previously set via
/// ::ArSession_setCameraTextureNames or ::ArSession_setCameraTextureName.
/// Texture names (IDs) are returned in a round robin fashion in sequential
/// frames.
///
/// @param[in]   session         The ARCore session.
/// @param[in]   frame           The current frame.
/// @param[out]  out_texture_id  Where to store the texture name (ID).
void ArFrame_getCameraTextureName(const ArSession *session,
                                  const ArFrame *frame,
                                  uint32_t *out_texture_id);

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
/// The pointer returned by this function is valid until ::ArPointCloud_release
/// is called. If the number of points is zero, then the value of
/// @p *out_point_cloud_data is undefined.
///
/// If your app needs to keep some Point Cloud data, for example to compare
/// Point Cloud data frame to frame, consider copying just the data points your
/// app needs, and then calling ::ArPointCloud_release to reduce the amount of
/// memory required.
void ArPointCloud_getData(const ArSession *session,
                          const ArPointCloud *point_cloud,
                          const float **out_point_cloud_data);

/// @ingroup ArPointCloud
/// Retrieves a pointer to the Point Cloud point IDs. The number of IDs is the
/// same as number of points, and is given by
/// ::ArPointCloud_getNumberOfPoints.
///
/// Each point has a unique identifier (within a session) that is persistent
/// across frames. That is, if a point from Point Cloud 1 has the same id as the
/// point from Point Cloud 2, then it represents the same point in space.
///
/// The pointer returned by this function is valid until ::ArPointCloud_release
/// is called. If the number of points is zero, then the value of
/// @p *out_point_ids is undefined.
///
/// If your app needs to keep some Point Cloud data, for example to compare
/// Point Cloud data frame to frame, consider copying just the data points your
/// app needs, and then calling ::ArPointCloud_release to reduce the amount of
/// memory required.
void ArPointCloud_getPointIds(const ArSession *session,
                              const ArPointCloud *point_cloud,
                              const int32_t **out_point_ids);

/// @ingroup ArPointCloud
/// Returns the timestamp in nanoseconds when this Point Cloud was observed.
/// This timestamp uses the same time base as ::ArFrame_getTimestamp.
void ArPointCloud_getTimestamp(const ArSession *session,
                               const ArPointCloud *point_cloud,
                               int64_t *out_timestamp_ns);

/// @ingroup ArPointCloud
/// Releases a reference to the Point Cloud.  This must match a call to
/// ::ArFrame_acquirePointCloud.
///
/// This function may safely be called with @c NULL - it will do nothing.
void ArPointCloud_release(ArPointCloud *point_cloud);

// === Image Metadata functions ===

/// @ingroup ArImageMetadata
/// Retrieves the capture metadata for the current camera image.
///
/// @c ACameraMetadata is a struct in Android NDK. Include NdkCameraMetadata.h
/// to use this type.
///
/// Note: The @c ACameraMetadata returned from this function will be invalid
/// after its ::ArImageMetadata object is released.
/// @deprecated Deprecated in release 1.20.0. Use
/// ::ArImageMetadata_getConstEntry instead of ACameraMetadata_getConstEntry.
void ArImageMetadata_getNdkCameraMetadata(
    const ArSession *session,
    const ArImageMetadata *image_metadata,
    const ACameraMetadata **out_ndk_metadata)
    AR_DEPRECATED(
        "Deprecated in ARCore 1.20.0. Use ::ArImageMetadata_getConstEntry "
        "instead of ACameraMetadata_getConstEntry.");

/// @ingroup ArImageMetadata
/// Releases a reference to the metadata.  This must match a call to
/// ::ArFrame_acquireImageMetadata.
///
/// This function may safely be called with @c NULL - it will do nothing.
void ArImageMetadata_release(ArImageMetadata *metadata);

/// @ingroup ArImage
/// Image formats produced by ARCore.
AR_DEFINE_ENUM(ArImageFormat){
    /// Invalid image format. Produced by ARCore when an invalid session/image
    /// is given to ::ArImage_getFormat.
    AR_IMAGE_FORMAT_INVALID = 0,

    /// Produced by ::ArFrame_acquireCameraImage.
    /// Int value equal to Android NDK @c AIMAGE_FORMAT_YUV_420_888
    /// (https://developer.android.com/ndk/reference/group/media#group___media_1gga9c3dace30485a0f28163a882a5d65a19aea9797f9b5db5d26a2055a43d8491890).
    /// and
    /// https://developer.android.com/reference/android/graphics/ImageFormat.html#YUV_420_888
    AR_IMAGE_FORMAT_YUV_420_888 = 0x23,

    /// Produced by ::ArFrame_acquireDepthImage.
    /// Int value equal to
    /// https://developer.android.com/reference/android/graphics/ImageFormat.html#DEPTH16
    AR_IMAGE_FORMAT_DEPTH16 = 0x44363159,

    /// Produced by ::ArLightEstimate_acquireEnvironmentalHdrCubemap.
    /// Int value equal to Android NDK @c AIMAGE_FORMAT_RGBA_FP16
    /// (https://developer.android.com/ndk/reference/group/media#group___media_1gga9c3dace30485a0f28163a882a5d65a19aa0f5b9a07c9f3dc8a111c0098b18363a).
    AR_IMAGE_FORMAT_RGBA_FP16 = 0x16,
};

/// @ingroup ArImage
/// Gets the width of the input ::ArImage.
///
/// @param[in]    session                The ARCore session.
/// @param[in]    image                  The ::ArImage of interest.
/// @param[inout] out_width              The width of the image in pixels.
void ArImage_getWidth(const ArSession *session,
                      const ArImage *image,
                      int32_t *out_width);

/// @ingroup ArImage
/// Gets the height of the input ::ArImage.
///
/// @param[in]    session                The ARCore session.
/// @param[in]    image                  The ::ArImage of interest.
/// @param[inout] out_height             The height of the image in pixels.
void ArImage_getHeight(const ArSession *session,
                       const ArImage *image,
                       int32_t *out_height);

/// @ingroup ArImage
/// Gets the source-specific timestamp of the provided ::ArImage in nanoseconds.
/// The timestamp is normally monotonically increasing. The timestamps for the
/// images from different sources may have different timebases and should not be
/// compared with each other. The specific meaning and timebase of the returned
/// timestamp depends on the source providing images.
///
/// @param[in]    session                The ARCore session.
/// @param[in]    image                  The ::ArImage of interest.
/// @param[inout] out_timestamp_ns       The timestamp of the image in
///                                      nanoseconds.
void ArImage_getTimestamp(const ArSession *session,
                          const ArImage *image,
                          int64_t *out_timestamp_ns);

/// @ingroup ArImage
/// Gets the image format of the provided ::ArImage.
///
/// @param[in]    session                The ARCore session.
/// @param[in]    image                  The ::ArImage of interest.
/// @param[inout] out_format             The image format, one of
/// ::ArImageFormat values.
void ArImage_getFormat(const ArSession *session,
                       const ArImage *image,
                       ArImageFormat *out_format);

/// @ingroup ArImage
/// Gets the number of planes in the provided ::ArImage. The number of planes
/// and format of data in each plane is format dependent. Use
/// ::ArImage_getFormat to determine the format.
///
/// @param[in]    session                The ARCore session.
/// @param[in]    image                  The ::ArImage of interest.
/// @param[inout] out_num_planes         The number of planes in the image.
void ArImage_getNumberOfPlanes(const ArSession *session,
                               const ArImage *image,
                               int32_t *out_num_planes);

/// @ingroup ArImage
/// Gets the byte distance between the start of two consecutive pixels in
/// the image. The pixel stride is always greater than 0.
///
/// @param[in]    session                The ARCore session.
/// @param[in]    image                  The ::ArImage of interest.
/// @param[in]    plane_index            The index of the plane, between 0 and
/// n-1, where n is number of planes for this image.
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
/// @param[in]    image                  The ::ArImage of interest.
/// @param[in]    plane_index            The index of the plane, between 0 and
/// n-1, where n is number of planes for this image.
/// @param[inout] out_row_stride         The row stride of the image in bytes.
void ArImage_getPlaneRowStride(const ArSession *session,
                               const ArImage *image,
                               int32_t plane_index,
                               int32_t *out_row_stride);

/// @ingroup ArImage
/// Gets the data pointer of the provided image for direct application access.
/// Note that once the ::ArImage data is released with ::ArImage_release, the
/// data pointer from the corresponding ::ArImage_getPlaneData call becomes
/// invalid. Do NOT use it after the ::ArImage is released.
///
/// @param[in]    session                The ARCore session.
/// @param[in]    image                  The ::ArImage of interest.
/// @param[in]    plane_index            The index of the plane, between 0 and
/// n-1, where n is number of planes for this image.
/// @param[inout] out_data               The data pointer to the image.
/// @param[inout] out_data_length        The length of data in bytes.
void ArImage_getPlaneData(const ArSession *session,
                          const ArImage *image,
                          int32_t plane_index,
                          const uint8_t **out_data,
                          int32_t *out_data_length);

/// @ingroup ArImage
/// Converts an ::ArImage object to an Android NDK @c AImage object. The
/// converted image object format is @c AIMAGE_FORMAT_YUV_420_888.
///
/// @deprecated Deprecated in release 1.10.0. Use the other ::ArImage functions
/// to obtain image data. ARCore can produce a wide variety of images, not all
/// of which can be represented using Android NDK @c AImage provided by this
/// function. In those cases, this function will return @c NULL in
/// out_ndk_image.
void ArImage_getNdkImage(const ArImage *image, const AImage **out_ndk_image)
    AR_DEPRECATED(
        "Deprecated in release 1.10.0. Please see function documentation");

/// @ingroup ArImage
/// Releases an instance of ::ArImage returned by ::ArFrame_acquireCameraImage.
void ArImage_release(ArImage *image);

// === ArLightEstimate functions ===

/// @ingroup ArLightEstimate
/// Allocates an ::ArLightEstimate object.
void ArLightEstimate_create(const ArSession *session,
                            ArLightEstimate **out_light_estimate);

/// @ingroup ArLightEstimate
/// Releases the provided ::ArLightEstimate object.
void ArLightEstimate_destroy(ArLightEstimate *light_estimate);

/// @ingroup ArLightEstimate
/// Retrieves the validity state of an ::ArLightEstimate.  If the resulting
/// value of
/// @p *out_light_estimate_state is not #AR_LIGHT_ESTIMATE_STATE_VALID, the
/// estimate should not be used for rendering.
void ArLightEstimate_getState(const ArSession *session,
                              const ArLightEstimate *light_estimate,
                              ArLightEstimateState *out_light_estimate_state);

/// @ingroup ArLightEstimate
/// Retrieves the pixel intensity, in gamma color space, of the current camera
/// view. Values are in the range [0.0, 1.0], with zero being black and one
/// being white. If #AR_LIGHT_ESTIMATION_MODE_AMBIENT_INTENSITY mode is not set,
/// returns zero.
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
/// of the light in the scene. If #AR_LIGHT_ESTIMATION_MODE_AMBIENT_INTENSITY
/// mode is not set, returns all zeros.
///
/// `out_color_correction_4` components are:
///   - `[0]` Red channel scale factor. This value is larger or equal to zero.
///   - `[1]` Green channel scale factor. This value is always 1.0 as the green
///           channel is the reference baseline.
///   - `[2]` Blue channel scale factor. This value is larger or equal to zero.
///   - `[3]` Pixel intensity. This is the same value as the one return from
///       ::ArLightEstimate_getPixelIntensity.
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
/// Returns the timestamp of the given ::ArLightEstimate in nanoseconds. This
/// timestamp uses the same time base as ::ArFrame_getTimestamp.
void ArLightEstimate_getTimestamp(const ArSession *session,
                                  const ArLightEstimate *light_estimate,
                                  int64_t *out_timestamp_ns);

/// @ingroup ArLightEstimate
/// Returns the direction of the main directional light based on the inferred
/// Environmental HDR Lighting Estimation. If
/// #AR_LIGHT_ESTIMATION_MODE_ENVIRONMENTAL_HDR mode is not set, returns
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
/// to zero. If #AR_LIGHT_ESTIMATION_MODE_ENVIRONMENTAL_HDR mode is not set,
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
///    #AR_LIGHT_ESTIMATION_MODE_ENVIRONMENTAL_HDR mode is not set, returns zero
///    for all 27 coefficients.
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
///                                ::ArImageCubemap type has been created to
///                                facilitate representing the array of
///                                ::ArImage pointers.
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
/// If #AR_LIGHT_ESTIMATION_MODE_ENVIRONMENTAL_HDR mode is not set, all textures
/// will be assigned with zero pixel values. All 6 acquired images must be
/// released with ::ArImage_release once they are no longer needed.
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
/// eventually be matched with a call to ::ArAnchor_release.
void ArAnchorList_acquireItem(const ArSession *session,
                              const ArAnchorList *anchor_list,
                              int32_t index,
                              ArAnchor **out_anchor);

// === ArAnchor functions ===

/// @ingroup ArAnchor
/// Retrieves the pose of the anchor in the world coordinate space. This pose
/// produced by this call may change each time ::ArSession_update is called.
/// This pose should only be used for rendering if ::ArAnchor_getTrackingState
/// returns #AR_TRACKING_STATE_TRACKING.
///
/// @param[in]    session  The ARCore session.
/// @param[in]    anchor   The anchor to retrieve the pose of.
/// @param[inout] out_pose An already-allocated ::ArPose object into which the
///     pose will be stored.
void ArAnchor_getPose(const ArSession *session,
                      const ArAnchor *anchor,
                      ArPose *out_pose);

/// @ingroup ArAnchor
/// Retrieves the current state of the pose of this anchor.
///
/// Note: Starting in ARCore 1.12, changing the active camera config using
/// ::ArSession_setCameraConfig may cause the tracking state on certain
/// devices to become permanently #AR_TRACKING_STATE_PAUSED. For consistent
/// behavior across all supported devices, release any previously created
/// anchors and trackables when setting a new camera config.
void ArAnchor_getTrackingState(const ArSession *session,
                               const ArAnchor *anchor,
                               ArTrackingState *out_tracking_state);

/// @ingroup ArAnchor
/// Tells ARCore to stop tracking and forget this anchor.  This call does not
/// release any references to the anchor - that must be done separately using
/// ::ArAnchor_release.
void ArAnchor_detach(ArSession *session, ArAnchor *anchor);

/// @ingroup ArAnchor
/// Releases a reference to an anchor. To stop tracking for this anchor, call
/// ::ArAnchor_detach first.
///
/// This function may safely be called with @c NULL - it will do nothing.
void ArAnchor_release(ArAnchor *anchor);

/// @ingroup ArAnchor
/// Acquires the Cloud Anchor ID of the anchor. The ID acquired is an ASCII
/// null-terminated string. The acquired ID must be released after use by the
/// ::ArString_release function. For anchors with cloud state
/// #AR_CLOUD_ANCHOR_STATE_NONE or #AR_CLOUD_ANCHOR_STATE_TASK_IN_PROGRESS, this
/// will always be an empty string.
///
/// @param[in]    session             The ARCore session.
/// @param[in]    anchor              The anchor to retrieve the cloud ID of.
/// @param[inout] out_cloud_anchor_id A pointer to the acquired ID string.
void ArAnchor_acquireCloudAnchorId(ArSession *session,
                                   ArAnchor *anchor,
                                   char **out_cloud_anchor_id);

/// @ingroup ArAnchor
/// Gets the current Cloud Anchor state of the anchor. This state is guaranteed
/// not to change until ::ArSession_update is called.
///
/// @param[in]    session   The ARCore session.
/// @param[in]    anchor    The anchor to retrieve the cloud state of.
/// @param[inout] out_state The current cloud state of the anchor.
void ArAnchor_getCloudAnchorState(const ArSession *session,
                                  const ArAnchor *anchor,
                                  ArCloudAnchorState *out_state);

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
/// eventually be matched with a call to ::ArTrackable_release.
void ArTrackableList_acquireItem(const ArSession *session,
                                 const ArTrackableList *trackable_list,
                                 int32_t index,
                                 ArTrackable **out_trackable);

// === ArTrackable functions ===

/// @ingroup ArTrackable
/// Releases a reference to a trackable. This does not mean that the trackable
/// will necessarily stop tracking. The same trackable may still be included in
/// from other calls, for example ::ArSession_getAllTrackables.
///
/// This function may safely be called with @c NULL - it will do nothing.
void ArTrackable_release(ArTrackable *trackable);

/// @ingroup ArTrackable
/// Retrieves the type of the trackable.  See ::ArTrackableType for valid types.
void ArTrackable_getType(const ArSession *session,
                         const ArTrackable *trackable,
                         ArTrackableType *out_trackable_type);

/// @ingroup ArTrackable
/// Retrieves the current state of ARCore's knowledge of the pose of this
/// trackable.
///
/// Note: Starting in ARCore 1.12, changing the active camera config using
/// ::ArSession_setCameraConfig may cause the tracking state on certain
/// devices to become permanently #AR_TRACKING_STATE_PAUSED. For consistent
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
/// @return #AR_SUCCESS or any of:
/// - #AR_ERROR_NOT_TRACKING if the trackable's tracking state was not
///   #AR_TRACKING_STATE_TRACKING
/// - #AR_ERROR_SESSION_PAUSED if the session was paused
/// - #AR_ERROR_RESOURCE_EXHAUSTED if too many anchors exist
/// - #AR_ERROR_ILLEGAL_STATE if this trackable doesn't support anchors
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
///     already been allocated with ::ArAnchorList_create.  If previously
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
/// being included in the output of ::ArFrame_getUpdatedTrackables.
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
/// Retrieves the type (orientation) of the plane.  See ::ArPlaneType.
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
/// @param[inout] out_pose An already-allocated ::ArPose object into which the
///     pose will be stored.
void ArPlane_getCenterPose(const ArSession *session,
                           const ArPlane *plane,
                           ArPose *out_pose);

/// @ingroup ArPlane
/// Retrieves the length of this plane's bounding rectangle measured along the
/// local X-axis of the coordinate space defined by the output of
/// ::ArPlane_getCenterPose.
void ArPlane_getExtentX(const ArSession *session,
                        const ArPlane *plane,
                        float *out_extent_x);

/// @ingroup ArPlane
/// Retrieves the length of this plane's bounding rectangle measured along the
/// local Z-axis of the coordinate space defined by the output of
/// ::ArPlane_getCenterPose.
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
/// by the pose (::ArPlane_getCenterPose) to get the boundary in world
/// coordinates.
///
/// @param[in]    session        The ARCore session.
/// @param[in]    plane          The plane to retrieve the polygon from.
/// @param[inout] out_polygon_xz A pointer to an array of floats.  The length of
///     this array must be at least that reported by ::ArPlane_getPolygonSize.
void ArPlane_getPolygon(const ArSession *session,
                        const ArPlane *plane,
                        float *out_polygon_xz);

/// @ingroup ArPlane
/// Sets @p *out_pose_in_extents to non-zero if the given pose (usually obtained
/// from an ::ArHitResult) is in the plane's rectangular extents.
void ArPlane_isPoseInExtents(const ArSession *session,
                             const ArPlane *plane,
                             const ArPose *pose,
                             int32_t *out_pose_in_extents);

/// @ingroup ArPlane
/// Sets @p *out_pose_in_extents to non-zero if the given pose (usually obtained
/// from an ::ArHitResult) is in the plane's polygon.
void ArPlane_isPoseInPolygon(const ArSession *session,
                             const ArPlane *plane,
                             const ArPose *pose,
                             int32_t *out_pose_in_polygon);

// === ArPoint functions ===

/// @ingroup ArPoint
/// Returns the pose of the point.
/// If ::ArPoint_getOrientationMode returns
/// #AR_POINT_ORIENTATION_ESTIMATED_SURFACE_NORMAL, the orientation will follow
/// the behavior described in ::ArHitResult_getHitPose. If
/// ::ArPoint_getOrientationMode returns
/// #AR_POINT_ORIENTATION_INITIALIZED_TO_IDENTITY, then returns an orientation
/// that is identity or close to identity.
/// @param[in]    session  The ARCore session.
/// @param[in]    point    The point to retrieve the pose of.
/// @param[inout] out_pose An already-allocated ::ArPose object into which the
/// pose will be stored.
void ArPoint_getPose(const ArSession *session,
                     const ArPoint *point,
                     ArPose *out_pose);

/// @ingroup ArPoint
/// Returns the ::ArPointOrientationMode of the point. For ::ArPoint objects
/// created by
/// ::ArFrame_hitTest.
/// If ::ArPointOrientationMode is
/// #AR_POINT_ORIENTATION_ESTIMATED_SURFACE_NORMAL, then normal of the surface
/// centered around the ::ArPoint was estimated successfully.
///
/// @param[in]    session              The ARCore session.
/// @param[in]    point                The point to retrieve the pose of.
/// @param[inout] out_orientation_mode OrientationMode output result for the
///     point.
void ArPoint_getOrientationMode(const ArSession *session,
                                const ArPoint *point,
                                ArPointOrientationMode *out_orientation_mode);

/// @ingroup ArInstantPlacementPoint
/// Returns the pose of the ::ArInstantPlacementPoint.
/// @param[in]    session  The ARCore session.
/// @param[in]    instant_placement_point  The Instant Placement point to
/// retrieve the pose of.
/// @param[inout] out_pose An ::ArPose object already-allocated via
/// ::ArPose_create into which the pose will be stored.
void ArInstantPlacementPoint_getPose(
    const ArSession *session,
    const ArInstantPlacementPoint *instant_placement_point,
    ArPose *out_pose);

/// @ingroup ArInstantPlacementPoint
/// Returns the tracking method of the ::ArInstantPlacementPoint.
/// @param[in]    session  The ARCore session.
/// @param[in]    instant_placement_point  The Instant Placement point to
/// retrieve the tracking method of.
/// @param[inout] out_tracking_method An already-allocated
/// ::ArInstantPlacementPointTrackingMethod object into which the tracking
/// method will be stored.
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
/// If the tracking state is #AR_TRACKING_STATE_PAUSED or
/// #AR_TRACKING_STATE_STOPPED, this returns the pose when the image state was
/// last #AR_TRACKING_STATE_TRACKING, or the identity pose if the image state
/// has never been #AR_TRACKING_STATE_TRACKING.
void ArAugmentedImage_getCenterPose(const ArSession *session,
                                    const ArAugmentedImage *augmented_image,
                                    ArPose *out_pose);

/// @ingroup ArAugmentedImage
/// Retrieves the estimated width, in metres, of the corresponding physical
/// image, as measured along the local X-axis of the coordinate space with
/// origin and axes as defined by ::ArAugmentedImage_getCenterPose.
///
/// ARCore will attempt to estimate the physical image's width and continuously
/// update this estimate based on its understanding of the world. If the
/// optional physical size is specified in the image database, this estimation
/// process will happen more quickly. However, the estimated size may be
/// different from the originally specified size.
///
/// If the tracking state is
/// #AR_TRACKING_STATE_PAUSED/#AR_TRACKING_STATE_STOPPED, this returns the
/// estimated width when the image state was last #AR_TRACKING_STATE_TRACKING.
/// If the image state has never been #AR_TRACKING_STATE_TRACKING, this returns
/// 0, even the image has a specified physical size in the image database.
void ArAugmentedImage_getExtentX(const ArSession *session,
                                 const ArAugmentedImage *augmented_image,
                                 float *out_extent_x);

/// @ingroup ArAugmentedImage
/// Retrieves the estimated height, in metres, of the corresponding physical
/// image, as measured along the local Z-axis of the coordinate space with
/// origin and axes as defined by ::ArAugmentedImage_getCenterPose.
///
/// ARCore will attempt to estimate the physical image's height and continuously
/// update this estimate based on its understanding of the world. If an optional
/// physical size is specified in the image database, this estimation process
/// will happen more quickly. However, the estimated size may be different from
/// the originally specified size.
///
/// If the tracking state is
/// #AR_TRACKING_STATE_PAUSED/#AR_TRACKING_STATE_STOPPED, this returns the
/// estimated height when the image state was last #AR_TRACKING_STATE_TRACKING.
/// If the image state has never been #AR_TRACKING_STATE_TRACKING, this returns
/// 0, even the image has a specified physical size in the image database.
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
/// the string using ::ArString_release when the string is no longer needed.
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
/// The pointer returned by this function is valid until ::ArTrackable_release
/// or the next ::ArSession_update is called. The application must copy the
/// data if they wish to retain it for longer.
///
/// If the face's tracking state is #AR_TRACKING_STATE_PAUSED, then the
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
/// The pointer returned by this function is valid until ::ArTrackable_release
/// or the next ::ArSession_update is called. The application must copy the
/// data if they wish to retain it for longer.
///
/// If the face's tracking state is #AR_TRACKING_STATE_PAUSED, then the
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
/// The pointer returned by this function is valid until ::ArTrackable_release
/// or the next ::ArSession_update is called. The application must copy the
/// data if they wish to retain it for longer.
///
/// If the face's tracking state is #AR_TRACKING_STATE_PAUSED, then the
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
/// The pointer returned by this function is valid until ::ArTrackable_release
/// or the next ::ArSession_update is called. The application must copy the
/// data if they wish to retain it for longer.
///
/// If the face's tracking state is #AR_TRACKING_STATE_PAUSED, then the
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
/// trackable state is #AR_TRACKING_STATE_TRACKING. When face trackable state is
/// #AR_TRACKING_STATE_PAUSED, the identity pose will be returned.
///
/// @param[in]  session     The ARCore session.
/// @param[in]  face        The face for which to retrieve face region pose.
/// @param[in]  region_type The face region for which to get the pose.
/// @param[out] out_pose    The Pose of the selected region when
///     #AR_TRACKING_STATE_TRACKING, or an identity pose when
///     #AR_TRACKING_STATE_PAUSED.
void ArAugmentedFace_getRegionPose(const ArSession *session,
                                   const ArAugmentedFace *face,
                                   const ArAugmentedFaceRegionType region_type,
                                   ArPose *out_pose);

/// @}

/// @ingroup ArAugmentedFace
/// Returns the pose of the center of the face.
///
/// @param[in]    session  The ARCore session.
/// @param[in]    face     The face for which to retrieve center pose.
/// @param[inout] out_pose An already-allocated ::ArPose object into which the
///     pose will be stored.
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
/// ::ArAugmentedImageDatabase_serialize.
///
/// Note: this function takes about 10-20ms for a 5MB byte array. Run this in a
/// background thread if this affects your application.
///
/// @return #AR_SUCCESS or any of:
/// - #AR_ERROR_DATA_INVALID_FORMAT - the bytes are in an invalid format.
/// - #AR_ERROR_DATA_UNSUPPORTED_VERSION - the database is not supported by
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
/// is expected to release the byte array using ::ArByteArray_release when the
/// byte array is no longer needed.
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
/// ::ArAugmentedImageDatabase_addImageWithPhysicalSize instead, to improve
/// image detection time.
///
/// For images added via ::ArAugmentedImageDatabase_addImage, ARCore estimates
/// the physical image's size and pose at runtime when the physical image is
/// visible and is being tracked. This extra estimation step will require the
/// user to move their device to view the physical image from different
/// viewpoints before the size and pose of the physical image can be estimated.
///
/// This function takes time to perform non-trivial image processing (20-30ms),
/// and should be run on a background thread.
///
/// The image name is expected to be a null-terminated string in UTF-8 format.
///
/// Currently, only images for which the stride is equal to the width are
/// supported.
///
/// @return #AR_SUCCESS or any of:
/// - #AR_ERROR_IMAGE_INSUFFICIENT_QUALITY - image quality is insufficient, e.g.
///   because of lack of features in the image.
/// - #AR_ERROR_INVALID_ARGUMENT - if @p image_stride_in_pixels is not equal to
///   @p image_width_in_pixels.
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
/// ::ArAugmentedImageDatabase_addImage instead, at the expense of an increased
/// image detection time.
///
/// For images added via ::ArAugmentedImageDatabase_addImageWithPhysicalSize,
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
/// @return #AR_SUCCESS or any of:
/// - #AR_ERROR_IMAGE_INSUFFICIENT_QUALITY - image quality is insufficient, e.g.
///   because of lack of features in the image.
/// - #AR_ERROR_INVALID_ARGUMENT - @p image_width_in_meters is <= 0 or if
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
/// @param[inout] out_hit_result    An already-allocated ::ArHitResult object
/// into
///     which the result will be copied.
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
/// the geometry. The orientation is a best effort to face the user's device,
/// and its exact definition differs depending on the Trackable that was hit.
///
/// ::ArPlane : X+ is perpendicular to the cast ray and parallel to the plane,
/// Y+ points along the plane normal (up, for #AR_PLANE_HORIZONTAL_UPWARD_FACING
/// planes), and Z+ is parallel to the plane, pointing roughly toward the
/// user's device.
///
/// ::ArPoint :
/// Attempt to estimate the normal of the surface centered around the hit test.
/// Surface normal estimation is most likely to succeed on textured surfaces
/// and with camera motion.
/// If ::ArPoint_getOrientationMode returns
/// #AR_POINT_ORIENTATION_ESTIMATED_SURFACE_NORMAL, then X+ is perpendicular to
/// the cast ray and parallel to the physical surface centered around the hit
/// test, Y+ points along the estimated surface normal, and Z+ points roughly
/// toward the user's device. If
/// ::ArPoint_getOrientationMode returns
/// #AR_POINT_ORIENTATION_INITIALIZED_TO_IDENTITY, then X+ is perpendicular to
/// the cast ray and points right from the perspective of the user's device, Y+
/// points up, and Z+ points roughly toward the user's device.
///
/// If you wish to retain the location of this pose beyond the duration of a
/// single frame, create an anchor using ::ArHitResult_acquireNewAnchor to save
/// the pose in a physically consistent way.
///
/// @param[in]    session    The ARCore session.
/// @param[in]    hit_result The hit result to retrieve the pose of.
/// @param[inout] out_pose   An already-allocated ::ArPose object into which the
///     pose will be stored.
void ArHitResult_getHitPose(const ArSession *session,
                            const ArHitResult *hit_result,
                            ArPose *out_pose);

/// @ingroup ArHitResult
/// Acquires reference to the hit trackable. This call must be paired with a
/// call to ::ArTrackable_release.
void ArHitResult_acquireTrackable(const ArSession *session,
                                  const ArHitResult *hit_result,
                                  ArTrackable **out_trackable);

/// @ingroup ArHitResult
/// Creates a new anchor at the hit location. See ::ArHitResult_getHitPose for
/// details.  This is equivalent to creating an anchor on the hit trackable at
/// the hit pose.
///
/// @return #AR_SUCCESS or any of:
/// - #AR_ERROR_NOT_TRACKING
/// - #AR_ERROR_SESSION_PAUSED
/// - #AR_ERROR_RESOURCE_EXHAUSTED
/// - #AR_ERROR_DEADLINE_EXCEEDED - hit result must be used before the next call
///     to ::ArSession_update.
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

/// @ingroup ArImageMetadata
/// Defines a rational data type in ::ArImageMetadata_const_entry.
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
  /// @c union below is valid.
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
    /// Pointer to data of single ::ArImageMetadata_rational or the first
    /// element of the ::ArImageMetadata_rational array.
    const ArImageMetadata_rational *r;
  } data;
} ArImageMetadata_const_entry;

/// @ingroup ArImageMetadata
/// Retrieves the list of the supported image metadata tags that can be queried
/// for their value.
///
/// The @p out_tags list remains valid until @p image_metadata is released via
/// ::ArImageMetadata_release.
///
/// @param[in]  session             The ARCore session.
/// @param[in]  image_metadata      ::ArImageMetadata struct obtained from
///     ::ArFrame_acquireImageMetadata.
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
/// Get a metadata entry for the provided ::ArImageMetadata and tag.
///
/// The returned @p out_metadata_entry remains valid until the provided @p
/// image_metadata is released via ::ArFrame_acquireImageMetadata.
///
/// @param[in]  session             The ARCore session.
/// @param[in]  image_metadata      ::ArImageMetadata struct obtained from
///     ::ArFrame_acquireImageMetadata.
/// @param[in]  tag                 The desired @c uint32_t metadata tag to be
///     retrieved from the provided ::ArImageMetadata struct.
/// @param[out] out_metadata_entry  The ::ArImageMetadata_const_entry struct to
///     which the metadata tag data should be written to, updated only when
///     function returns #AR_SUCCESS.
///
/// @return #AR_SUCCESS or any of:
/// - #AR_ERROR_INVALID_ARGUMENT - if either @p session, @p image_metadata or
///     @p out_metadata_entry is null.
/// - #AR_ERROR_METADATA_NOT_FOUND - if @p image_metadata does not contain an
///     entry of the @p tag value.
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
