/*
 * Copyright 2019 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.google.ar.core.examples.c.helloarvulkan;

import android.app.Activity;
import android.content.Context;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.util.Log;
import android.view.Surface;
import androidx.annotation.Nullable;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;

/** JNI interface to native layer. */
public class JniInterface {
  static {
    System.loadLibrary("hello_ar_vulkan_native");
  }

  private static final String TAG = "JniInterface";
  static AssetManager assetManager;

  public static native long createNativeApplication(AssetManager assetManager);

  public static native void destroyNativeApplication(long nativeApplication);

  public static native void onPause(long nativeApplication);

  public static native void onResume(long nativeApplication, Context context, Activity activity);

  /** Allocate resources for rendering. */
  public static native void onSurfaceCreated(long nativeApplication, Surface surface);

  public static native void onSurfaceDestroyed(long nativeApplication);

  /** Called when the view port width, height, or display rotation may have changed. */
  public static native void onDisplayGeometryChanged(
      long nativeApplication, int displayRotation, int width, int height);

  /** Main render loop. */
  public static native void onSurfaceDrawFrame(
      long nativeApplication, boolean depthColorVisualizationEnabled, boolean useDepthForOcclusion);

  /** Called when a touch event is detected. */
  public static native void onTouched(long nativeApplication, float x, float y);

  /** Get plane count in current session. Used to disable the "searching for surfaces" snackbar. */
  public static native boolean hasDetectedPlanes(long nativeApplication);

  /** Checks if the device supports the Depth API. */
  public static native boolean isDepthSupported(long nativeApplication);

  /** Called when the Instant Placement setting is changed. */
  public static native void onSettingsChange(
      long nativeApplication, boolean isInstantPlacementEnabled);

  /** This class is a simple container to pass image data from Java to C++. */
  public static class ImageData {
    public final int width;
    public final int height;
    public final ByteBuffer buffer;

    ImageData(int w, int h, ByteBuffer b) {
      width = w;
      height = h;
      buffer = b;
    }
  }

  /** Loads a PNG from assets and returns its raw pixel data in a ByteBuffer. */
  @Nullable
  public static ImageData loadImageData(String path) {
    try (InputStream is = assetManager.open(path)) {
      Bitmap bitmap = BitmapFactory.decodeStream(is);
      int size = bitmap.getByteCount();
      ByteBuffer buffer = ByteBuffer.allocateDirect(size);
      bitmap.copyPixelsToBuffer(buffer);
      buffer.rewind();
      return new ImageData(bitmap.getWidth(), bitmap.getHeight(), buffer);

    } catch (IOException e) {
      Log.e(TAG, "Cannot load image from assets: " + path);
      return null;
    }
  }

}
