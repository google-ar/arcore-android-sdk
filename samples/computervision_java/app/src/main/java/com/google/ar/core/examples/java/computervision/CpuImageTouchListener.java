package com.google.ar.core.examples.java.computervision;

import android.view.MotionEvent;
import android.view.View;

/**
 * Tracks the touches to the rendering view and updates the splitter position in {@link
 * CpuImageRenderer}.
 */
class CpuImageTouchListener implements View.OnTouchListener {

  private final CpuImageRenderer cpuImageRenderer;

  public CpuImageTouchListener(CpuImageRenderer cpuImageRenderer) {
    this.cpuImageRenderer = cpuImageRenderer;
  }

  @Override
  public boolean onTouch(View view, MotionEvent motionEvent) {
    if (motionEvent.getAction() == MotionEvent.ACTION_UP) {
      float newPosition = (cpuImageRenderer.getSplitterPosition() < 0.5f) ? 1.0f : 0.0f;
      cpuImageRenderer.setSplitterPosition(newPosition);
    }

    return true;
  }
}
