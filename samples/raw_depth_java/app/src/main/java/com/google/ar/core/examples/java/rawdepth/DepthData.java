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

package com.google.ar.core.examples.java.rawdepth;

import android.media.Image;
import com.google.ar.core.Anchor;
import com.google.ar.core.CameraIntrinsics;
import com.google.ar.core.Frame;
import com.google.ar.core.Session;
import com.google.ar.core.exceptions.NotYetAvailableException;
import java.nio.FloatBuffer;

/**
 * Stores depth data from ARCore as a 3D pointcloud. Points are added by calling the Raw Depth API,
 * and reprojected into 3D space. The points are stored relative to an anchor created with each
 * instance. The color of the points are matched with the latest color image from the same frame.
 */
final class DepthData {
  /** Buffer of point coordinates and confidence values. */
  private FloatBuffer points;

  /** Buffer of point RGB color values. */
  private FloatBuffer colors;

  /** The anchor to the 3D position of the camera at the point of depth acquisition. */
  private final Anchor anchor;

  /** The timestamp in nanoseconds when the raw depth image was observed. */
  private long timestamp;

  private DepthData(
      FloatBuffer points, FloatBuffer colors, long timestamp, Anchor cameraPoseAnchor) {
    this.points = points;
    this.colors = colors;
    this.timestamp = timestamp;
    this.anchor = cameraPoseAnchor;
  }

  public static DepthData create(Session session, Frame frame) {
    try (Image cameraImage = frame.acquireCameraImage();
        Image depthImage = frame.acquireRawDepthImage16Bits();
        Image confidenceImage = frame.acquireRawDepthConfidenceImage()) {
      // Depth images vary in size depending on device, and can be large on devices with a depth
      // camera. To ensure smooth framerate, we cap the number of points each frame.
      final int maxNumberOfPointsToRender = 15000;

      // To transform 2D depth pixels into 3D points we retrieve the intrinsic camera parameters
      // corresponding to the depth image. See more information about the depth values at
      // https://developers.google.com/ar/develop/java/depth/overview#understand-depth-values.
      CameraIntrinsics intrinsics = frame.getCamera().getTextureIntrinsics();
      FloatBuffer points =
          PointCloudHelper.convertRawDepthImagesTo3dPointBuffer(
              depthImage, confidenceImage, intrinsics, maxNumberOfPointsToRender);

      // To give each point a color from the RGB camera we need to look up the RGB pixel
      // corresponding to each depth pixel. RGB and depth images usually have different aspect
      // ratios. Here we calculate the CPU image region that corresponds to the area covered by the
      // depth image.
      FloatBuffer imageRegionCoordinates =
          PointCloudHelper.getImageCoordinatesForFullTexture(frame);

      FloatBuffer colors =
          PointCloudHelper.convertImageToColorBuffer(
              cameraImage, depthImage, imageRegionCoordinates, maxNumberOfPointsToRender);

      Anchor cameraPoseAnchor = session.createAnchor(frame.getCamera().getPose());
      return new DepthData(points, colors, depthImage.getTimestamp(), cameraPoseAnchor);
    } catch (NotYetAvailableException e) {
      // This normally means that depth data is not available yet. This is normal so we will not
      // spam the logcat with this.
    }

    return null;
  }

  /**
   * Buffer of point coordinates and confidence values.
   *
   * <p>Each point is represented by four consecutive values in the buffer; first the X, Y, Z
   * position coordinates, followed by a confidence value. This is the same format as described in
   * {@link android.graphics.ImageFormat#DEPTH_POINT_CLOUD}.
   *
   * <p>Point locations are in the world coordinate space, consistent with the camera position for
   * the frame that provided the point cloud.
   */
  public FloatBuffer getPoints() {
    return points;
  }

  /**
   * Buffer of point RGB values from the color camera.
   *
   * <p>Each point is represented by three consecutive values in the buffer for the red, green and
   * blue image channels. The values for each color are in 0-1 range (inclusive).
   */
  public FloatBuffer getColors() {
    return colors;
  }

  /** Returns the anchor corresponding to the camera pose where the depth data was acquired. */
  public Anchor getAnchor() {
    return anchor;
  }

  /**
   * Retrieves the linearized column-major 4x4 matrix representing the transform from pointcloud to
   * the session coordinates.
   */
  public void getModelMatrix(float[] modelMatrix) {
    anchor.getPose().toMatrix(modelMatrix, 0);
  }

}
