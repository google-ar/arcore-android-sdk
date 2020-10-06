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
 */

package com.google.ar.core.examples.java.persistentcloudanchor;

import android.opengl.Matrix;
import com.google.ar.core.Pose;
import com.google.ar.core.Session.FeatureMapQuality;
import com.google.ar.core.examples.java.common.rendering.ObjectRenderer;

/** Helper class to display the Feature Map Quality UI for the Persistent Cloud Anchor Sample. */
class FeatureMapQualityUi {
  private static final String TAG = FeatureMapQualityUi.class.getSimpleName();
  private static final float[] ROTATION_QUATERNION_180_Y =
      new float[] {0, (float) Math.sin(Math.PI / 2.0), 0, (float) Math.cos(Math.PI / 2.0)};
  private static final float[] ROTATION_QUATERNION_90_Y =
      new float[] {0, (float) Math.sin(-Math.PI / 4.0), 0, (float) Math.cos(-Math.PI / 4.0)};
  private static final float[] ROTATION_QUATERNION_90_X =
      new float[] {(float) Math.sin(Math.PI / 4.0), 0, 0, (float) Math.cos(Math.PI / 4.0)};
  // For anchors on horizontal planes. The UI coordinate frame is rotated 180 degrees with respect
  // to the anchor frame to simplify calculations. The positive x-axis points to the left of the
  // screen and the positive z-axis points away from the camera.
  private static final Pose HORIZONTAL_UI_TRANSFORM = Pose.makeRotation(ROTATION_QUATERNION_180_Y);
  // For anchors on vertical planes. The UI coordinate frame rotated with respect to the anchor
  // to face outwards toward the user such that the positive x-axis points to the left of the screen
  // and the positive z-axis points away from the camera into the plane.
  private static final Pose VERTICAL_UI_TRANSFORM =
      Pose.makeRotation(ROTATION_QUATERNION_90_Y)
          .compose(Pose.makeRotation(ROTATION_QUATERNION_90_X));
  // Spacing between indicator bars.
  private static final double MAPPING_UI_SPACING_RADIANS = Math.toRadians(7.5);
  private static final float MAPPING_UI_RADIUS = 0.2f;
  private static final float BAR_SCALE = 0.3f;
  private static final float[] BAR_COLOR_UNKNOWN_QUALITY =
      new float[] {218.0f, 220.0f, 240.0f, 255.0f};
  private static final float[] BAR_COLOR_LOW_QUALITY = new float[] {234.0f, 67.0f, 53.0f, 255.0f};
  private static final float[] BAR_COLOR_MEDIUM_QUALITY =
      new float[] {250.0f, 187.0f, 5.0f, 255.0f};
  private static final float[] BAR_COLOR_HIGH_QUALITY = new float[] {52.0f, 168.0f, 82.0f, 255.0f};

  final boolean isHorizontal;
  final double arcStartRad;
  final double arcEndRad;
  final int numBars;
  final float radius;
  ObjectRenderer objectRenderer;
  final QualityBar[] bars;
  Pose featureMapQualityUIPose;

  enum Quality {
    UNKNOWN,
    INSUFFICIENT,
    SUFFICIENT,
    GOOD
  }

  class QualityBar {
    private final Pose localPose;
    private final float[] modelMatrix;
    private Quality quality;

    public QualityBar(double rad) {
      modelMatrix = new float[16];
      localPose = computeLocalPose(rad);
      quality = Quality.UNKNOWN;
    }

    private Pose computeLocalPose(double rad) {
      // Rotate around y axis
      float[] rotation = {0, (float) Math.sin(rad / 2.0), 0, (float) Math.cos(rad / 2.0), 0};
      float[] translation = {radius, 0, 0};
      Pose pose = Pose.makeRotation(rotation).compose(Pose.makeTranslation(translation));
      return pose;
    }

    public void updateQuality(FeatureMapQuality quality) {
      if (quality == FeatureMapQuality.INSUFFICIENT) {
        this.quality = Quality.INSUFFICIENT;
      } else if (quality == FeatureMapQuality.SUFFICIENT) {
        this.quality = Quality.SUFFICIENT;
      } else {
        this.quality = Quality.GOOD;
      }
    }

    public void draw(
        Pose uiPose, float[] viewMatrix, float[] projectionMatrix, float[] colorCorrectionRgba) {
      uiPose.compose(localPose).toMatrix(modelMatrix, 0);
      objectRenderer.updateModelMatrix(modelMatrix, BAR_SCALE);
      if (this.quality == Quality.UNKNOWN) {
        objectRenderer.draw(
            viewMatrix, projectionMatrix, colorCorrectionRgba, BAR_COLOR_UNKNOWN_QUALITY);
      } else if (this.quality == Quality.INSUFFICIENT) {
        objectRenderer.draw(
            viewMatrix, projectionMatrix, colorCorrectionRgba, BAR_COLOR_LOW_QUALITY);
      } else if (this.quality == Quality.SUFFICIENT) {
        objectRenderer.draw(
            viewMatrix, projectionMatrix, colorCorrectionRgba, BAR_COLOR_MEDIUM_QUALITY);
      } else {
        objectRenderer.draw(
            viewMatrix, projectionMatrix, colorCorrectionRgba, BAR_COLOR_HIGH_QUALITY);
      }
    }
  }

  /**
   * Returns true if the anchor (specified by anchorTranslationWorld is visible in the camera view (
   * specified by viewMatrix and projectionMatrix); otherwise false.
   */
  public static boolean isAnchorInView(
      float[] anchorTranslationWorld, float[] viewMatrix, float[] projectionMatrix) {
    float[] viewProjectionMatrix = new float[16];
    final float[] anchorTranslationNDC = new float[4];
    // Project point to Normalized Device Coordinates and check if in bounds.
    Matrix.multiplyMM(viewProjectionMatrix, 0, projectionMatrix, 0, viewMatrix, 0);
    Matrix.multiplyMV(anchorTranslationNDC, 0, viewProjectionMatrix, 0, anchorTranslationWorld, 0);
    float ndcX = anchorTranslationNDC[0] / anchorTranslationNDC[3];
    float ndcY = anchorTranslationNDC[1] / anchorTranslationNDC[3];
    return !(ndcX < -1 || ndcX > 1 || ndcY < -1 || ndcY > 1);
  }

  public static FeatureMapQualityUi createHorizontalFeatureMapQualityUi(
      ObjectRenderer objectRenderer) {
    return new FeatureMapQualityUi(true, objectRenderer);
  }

  public static FeatureMapQualityUi createVerticalFeatureMapQualityUi(
      ObjectRenderer objectRenderer) {
    return new FeatureMapQualityUi(false, objectRenderer);
  }

  private FeatureMapQualityUi(boolean isHorizontal, ObjectRenderer objectRenderer) {
    this.isHorizontal = isHorizontal;
    this.radius = MAPPING_UI_RADIUS;
    this.objectRenderer = objectRenderer;
    arcStartRad = 0;
    arcEndRad = Math.PI;
    numBars = (int) Math.round(Math.PI / MAPPING_UI_SPACING_RADIANS);

    bars = new QualityBar[numBars];
    for (int i = 0; i < numBars; ++i) {
      bars[i] = new QualityBar(Math.PI / (double) numBars * i);
    }
  }

  public Pose getUiTransform() {
    if (this.isHorizontal) {
      return HORIZONTAL_UI_TRANSFORM;
    } else {
      return VERTICAL_UI_TRANSFORM;
    }
  }

  // Average quality value computed over all bars (0.0 for INSUFFICIENT, 0.6 for
  // SUFFICIENT, and 1.0 for GOOD)
  public float computeOverallQuality() {
    float sumQuality = 0;
    for (QualityBar bar : bars) {
      if (bar.quality == Quality.SUFFICIENT) {
        sumQuality += 0.6f;
      } else if (bar.quality == Quality.GOOD) {
        sumQuality += 1.0f;
      }
    }
    return sumQuality / numBars;
  }

  public void updateQualityForViewpoint(float[] cameraPosition, FeatureMapQuality quality) {
    int idx = computeBarIndex(cameraPosition);
    if (idx >= 0 && idx < numBars) {
      QualityBar barInView = bars[idx];
      barInView.updateQuality(quality);
    }
  }

  public void drawUi(
      Pose anchorPose, float[] viewMatrix, float[] projectionMatrix, float[] colorCorrectionRgba) {
    if (isHorizontal) {
      featureMapQualityUIPose = anchorPose.compose(HORIZONTAL_UI_TRANSFORM);
    } else {
      featureMapQualityUIPose = anchorPose.compose(VERTICAL_UI_TRANSFORM);
    }
    for (QualityBar bar : bars) {
      bar.draw(featureMapQualityUIPose, viewMatrix, projectionMatrix, colorCorrectionRgba);
    }
  }

  private static int computeBarIndex(float[] viewRay) {
    // positive indices.
    double rad = -Math.atan2(viewRay[2], viewRay[0]);
    return (int) Math.floor(rad / MAPPING_UI_SPACING_RADIANS);
  }
}
