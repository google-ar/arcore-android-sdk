package com.google.ar.core.examples.java.computervision;

import android.view.MotionEvent;
import android.view.Surface;
import android.view.View;

/**
 * Tracks the touches to the rendering view and updates the splitter position in
 * {@link CpuImageRenderer}.
 */
class CpuImageTouchListener implements View.OnTouchListener {
  private static final float SWIPE_SCALING_FACTOR = 1.15f;
  private static final float MIN_DELTA = .01f;
  private float startPosition = 0;
  private float startCoordX = 0;
  private float startCoordY = 0;
  private int displayRotation = 0;

  private final CpuImageDisplayRotationHelper cpuImageDisplayRotationHelper;
  private final CpuImageRenderer cpuImageRenderer;

  public CpuImageTouchListener(
      CpuImageDisplayRotationHelper cpuImageDisplayRotationHelper,
      CpuImageRenderer cpuImageRenderer) {
    this.cpuImageDisplayRotationHelper = cpuImageDisplayRotationHelper;
    this.cpuImageRenderer = cpuImageRenderer;
  }

  @Override
  public boolean onTouch(View view, MotionEvent motionEvent) {
    if (motionEvent.getAction() == MotionEvent.ACTION_DOWN) {
      startCoordX = motionEvent.getX();
      startCoordY = motionEvent.getY();
      displayRotation = cpuImageDisplayRotationHelper.getRotation();
      startPosition = cpuImageRenderer.getSplitterPosition();

    } else if (motionEvent.getAction() == MotionEvent.ACTION_MOVE) {
      float delta = 0;
      switch (displayRotation) {
        case Surface.ROTATION_90:
          delta = (motionEvent.getX() - startCoordX) / view.getWidth();
          break;
        case Surface.ROTATION_180:
          delta = -(motionEvent.getY() - startCoordY) / view.getHeight();
          break;
        case Surface.ROTATION_270:
          delta = -(motionEvent.getX() - startCoordX) / view.getWidth();
          break;
        case Surface.ROTATION_0:
        default:
          delta = (motionEvent.getY() - startCoordY) / view.getHeight();
          break;
      }
      if (Math.abs(delta) > MIN_DELTA) {
        float newPosition = startPosition + delta * SWIPE_SCALING_FACTOR;
        newPosition = Math.min(1.f, Math.max(0.f, newPosition));
        cpuImageRenderer.setSplitterPosition(newPosition);
      }
    }

    return true;
  }
}
