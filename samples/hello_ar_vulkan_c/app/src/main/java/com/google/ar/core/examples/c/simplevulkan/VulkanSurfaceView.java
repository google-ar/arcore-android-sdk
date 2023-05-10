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
package com.google.ar.core.examples.c.simplevulkan;

import android.content.Context;
import android.graphics.Canvas;
import android.util.AttributeSet;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import java.util.Timer;
import java.util.TimerTask;

/**
 * An implementation of SurfaceView that uses the dedicated surface for displaying Vulkan rendering.
 *
 * <p>Manages a surface, which is a special piece of memory that can be composited into the Android
 * view system.
 *
 * <p>Accepts a user-provided Renderer object that does the actual rendering.
 */
public class VulkanSurfaceView extends SurfaceView implements SurfaceHolder.Callback2 {
  private Renderer renderer;
  private static final String TAG = VulkanSurfaceView.class.getSimpleName();
  private Timer drawTimer;
  private TimerTask drawTimerTask;
  /** 30 frames per second. */
  private int frameTimerPeriod = 34;

  private int timerTaskDelayInstant = 1;

  /**
   * A generic renderer interface.
   *
   * <p>The renderer is responsible for making Vulkan calls to render a frame.
   */
  public interface Renderer {
    /*
     * Called when the surface is created or recreated.
     *
     * @param surface The surface to draw on.
     */
    public void onSurfaceCreated(Surface surface);

    /**
     * Called when the surface size has changed.
     *
     * @param width The new width of the surface.
     * @param height The new height of the surface.
     */
    public void onSurfaceChanged(int width, int height);

    /** Called to draw the current frame. */
    public void onDrawFrame();
  }

  /**
   * Standard View constructor. In order to render something, you must call {@link #setRenderer} to
   * register a renderer.
   *
   * @param context Interface to global information about an application environment.
   */
  public VulkanSurfaceView(Context context) {
    super(context);
    // Install a SurfaceHolder.Callback so we get notified when the
    // underlying surface is created and destroyed
    getHolder().addCallback(this);
  }

  /**
   * Standard View constructor. In order to render something, you must call {@link #setRenderer} to
   * register a renderer.
   *
   * @param context Interface to global information about an application environment.
   * @param arrtrs A collection of attributes defined in the .xml layout file.
   */
  public VulkanSurfaceView(Context context, AttributeSet attrs) {
    super(context, attrs);
    // Install a SurfaceHolder.Callback so we get notified when the
    // underlying surface is created and destroyed
    getHolder().addCallback(this);
  }

  /**
   * Set the renderer associated with this view.
   *
   * <p>This method should be called once and only once in the life-cycle of a VulkanSurfaceView.
   *
   * @param renderer The renderer to use to perform Vulkan drawing.
   */
  public void setRenderer(Renderer renderer) {
    this.renderer = renderer;
  }

  /** This method is responsible for drawing the current frame. */
  @Override
  protected void onDraw(Canvas canvas) {
    super.onDraw(canvas);

    renderer.onDrawFrame();
  }

  /** This method is part of the SurfaceHolder.Callback2 interface. */
  public void surfaceCreated(SurfaceHolder holder) {
    // Setting this flag allows the onDraw() callback to be called.
    setWillNotDraw(false);
    renderer.onSurfaceCreated(holder.getSurface());
    startDrawTimer();
  }

  /** This method is part of the SurfaceHolder.Callback2 interface. */
  public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
    renderer.onSurfaceChanged(width, height);
  }

  /** This method is part of the SurfaceHolder.Callback2 interface. */
  public void surfaceDestroyed(SurfaceHolder holder) {
    drawTimer.cancel();
  }

  /** This method is part of the SurfaceHolder.Callback2 interface. */
  public void surfaceRedrawNeeded(SurfaceHolder holder) {}

  /**
   * Starts the timer and sets the timerTask to call invalidate. This allows the surface to be
   * redrawn.
   */
  private void startDrawTimer() {
    drawTimer = new Timer();
    drawTimerTask =
        new TimerTask() {
          public void run() {
            invalidate();
          }
        };

    drawTimer.schedule(drawTimerTask, timerTaskDelayInstant, frameTimerPeriod);
  }
}
