package com.google.ar.core.examples.c.cloudanchor;

import android.app.Activity;
import android.content.Context;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.opengl.GLUtils;
import android.util.Log;
import java.io.IOException;

/** JNI interface to native layer. */
public class JniInterface {
  static {
    System.loadLibrary("cloud_anchor_native");
  }

  /** Interface used by reverse Jni calls to effect UX. **/
  public interface CallbackInterface {
    public void setHostAndResolveVisibility(boolean hostRoomEnabled, boolean resolveRoomEnabled);

    public void showResolveDialog();

    public void displayMessageOnLowerSnackbar(String newMessage);

    public void updateFirebaseRoomCode(boolean update, long optionalNewRoomCode);

    public void maybeUpdateFirebase(String anchorId);
  }

  private static CallbackInterface callbackInterface;

  private static final String TAG = "JniInterface";
  static AssetManager assetManager;

  public static void setCallbackInterface(CallbackInterface callbackInterface) {
    JniInterface.callbackInterface = callbackInterface;
  }

  public static native long createNativeApplication(AssetManager assetManager);

  public static native void destroyNativeApplication(long nativeApplication);

  public static native void onPause(long nativeApplication);

  public static native void onResume(long nativeApplication, Context context, Activity activity);

  /** Allocate OpenGL resources for rendering. */
  public static native void onGlSurfaceCreated(long nativeApplication);

  /**
   * Called on the OpenGL thread before onGlSurfaceDrawFrame when the view port width, height, or
   * display rotation may have changed.
   */
  public static native void onDisplayGeometryChanged(
      long nativeApplication, int displayRotation, int width, int height);

  /** Main render loop, called on the OpenGL thread. */
  public static native void onGlSurfaceDrawFrame(long nativeApplication);

  /** OnTouch event, called on the OpenGL thread. */
  public static native void onTouched(long nativeApplication, float x, float y);

  /** OnHostButtonPress event, called on the OpenGL thread. */
  public static native void onHostButtonPress(long nativeApplication);

  /** OnResolvePress event, called on the OpenGL thread. */
  public static native void onResolveButtonPress(long nativeApplication);

  /** OnResolve Dialog OK button pressed, called on the UI thread. */
  public static native void onResolveOkPress(long nativeApplication, long roomCode);

  /** OnResolve Dialog cancel button pressed, called on the UI thread. */
  public static native void onResolveCancelPress(long nativeApplication);

  /** Called when an anchor ID has been made available from Firebase. */
  public static native void onCloudAnchorIdMadeAvailable(long nativeApplication, String anchorId);

  /** Called when an error has occurred with Firebase. */
  public static native void onFirebaseError(long nativeApplication);

  /** Get plane count in current session. Used to disable the "searching for surfaces" snackbar. */
  public static native boolean hasDetectedPlanes(long nativeApplication);

  /** Gets the active CloudAnchorId, or null if there isn't one. */
  public static native String getCloudAnchorId(long nativeApplication);

  public static Bitmap loadImage(String imageName) {
    try {
      return BitmapFactory.decodeStream(assetManager.open(imageName));
    } catch (IOException e) {
      Log.e(TAG, "Cannot open image " + imageName);
      return null;
    }
  }

  public static void loadTexture(int target, Bitmap bitmap) {
    GLUtils.texImage2D(target, 0, bitmap, 0);
  }

  public static void setHostAndResolveVisibility(int hostAndResolveVisibility) {
    // This is hacky, but enums are hard to communicate properly from rJni calls.
    // 0:  All visible
    // 1:  Host is visible.
    // 2:  Resolve is visible.
    if (hostAndResolveVisibility == 0) {
      JniInterface.callbackInterface.setHostAndResolveVisibility(true, true);
    } else if (hostAndResolveVisibility == 1) {
      JniInterface.callbackInterface.setHostAndResolveVisibility(true, false);
    } else if (hostAndResolveVisibility == 2) {
      JniInterface.callbackInterface.setHostAndResolveVisibility(false, true);
    } else {
      Log.e(TAG, "An invalid value was passed to setHostAndResolveVisibility");
    }
  }

  public static void showResolveDialog() {
    JniInterface.callbackInterface.showResolveDialog();
  }

  public static void displayMessageOnLowerSnackbar(String message) {
    JniInterface.callbackInterface.displayMessageOnLowerSnackbar(message);
  }

  public static void updateFirebaseRoomCode(boolean update, long optionalNewRoomCode) {
    JniInterface.callbackInterface.updateFirebaseRoomCode(update, optionalNewRoomCode);
  }

  public static void maybeUpdateFirebase(String message) {
    JniInterface.callbackInterface.maybeUpdateFirebase(message);
  }
}
