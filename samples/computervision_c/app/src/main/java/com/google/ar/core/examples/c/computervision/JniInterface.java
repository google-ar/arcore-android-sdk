package com.google.ar.core.examples.c.computervision;

import android.app.Activity;
import android.content.Context;

/** JNI interface to native layer. */
public class JniInterface {
  static {
    System.loadLibrary("computer_vision_native");
  }

  /** Creates the native application to demo CPU Image access from ARCore. */
  static native long createNativeApplication();

  /**
   * Destroys the native application, clean up data and states.
   *
   * @param nativeApplication the native application handle.
   */
  static native void destroyNativeApplication(long nativeApplication);

  static native void onPause(long nativeApplication);

  static native void onResume(long nativeApplication, Context context, Activity activity);

  /**
   * Allocates OpenGL resources for rendering.
   *
   * @param nativeApplication the native application handle.
   */
  static native void onGlSurfaceCreated(long nativeApplication);

  /**
   * Pass display geometry change event into native application to handle GL view port change.
   *
   * <p>Called on the OpenGL thread before onGlSurfaceDrawFrame when the view port width, height, or
   * display rotation may have changed.
   *
   * @param nativeApplication the native application handle.
   * @param displayRotation the display's rotation.
   * @param cameraToDisplayRotation the camera's rotation relative to display.
   * @param width the window's width.
   * @param height the window's height.
   */
  static native void onDisplayGeometryChanged(
      long nativeApplication,
      int displayRotation,
      int cameraToDisplayRotation,
      int width,
      int height);

  /**
   * Main render loop, called on the OpenGL thread.
   *
   * @param nativeApplication the native application handle.
   * @param splitPosition the current screen split position in proportion to the whole screen.
   */
  static native void onGlSurfaceDrawFrame(long nativeApplication, float splitPosition);
}
