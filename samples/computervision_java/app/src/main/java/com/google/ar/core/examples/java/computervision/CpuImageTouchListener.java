package com.google.ar.core.examples.java.computervision;

import android.app.Activity;
import android.content.Context;
import android.view.MotionEvent;
import android.view.View;
import android.widget.RadioGroup;

/**
 * Tracks the touches to the rendering view and updates the splitter position in {@link
 * CpuImageRenderer}.
 */
class CpuImageTouchListener implements View.OnTouchListener {

  private final CpuImageRenderer cpuImageRenderer;
  private final Context context;

  public CpuImageTouchListener(CpuImageRenderer cpuImageRenderer, Context context) {
    this.cpuImageRenderer = cpuImageRenderer;
    this.context = context;
  }

  @Override
  public boolean onTouch(View view, MotionEvent motionEvent) {
    if (motionEvent.getAction() == MotionEvent.ACTION_UP) {
      float newPosition = (cpuImageRenderer.getSplitterPosition() < 0.5f) ? 1.0f : 0.0f;
      cpuImageRenderer.setSplitterPosition(newPosition);

      // Display the CPU resolution related UI only when CPU image is being displayed.
      boolean show = (newPosition < 0.5f);
      RadioGroup radioGroup =
          (RadioGroup) ((Activity) context).findViewById(R.id.radio_camera_configs);
      radioGroup.setVisibility(show ? View.VISIBLE : View.INVISIBLE);
    }

    return true;
  }
}
