/*
 * Copyright 2017 Google LLC
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
package com.google.ar.core.examples.java.common.helpers;

import android.app.Activity;
import android.view.View;
import android.widget.TextView;
import com.google.android.material.snackbar.BaseTransientBottomBar;
import com.google.android.material.snackbar.Snackbar;

/**
 * Helper to manage the sample snackbar. Hides the Android boilerplate code, and exposes simpler
 * methods.
 */
public final class SnackbarHelper {
  private static final int BACKGROUND_COLOR = 0xbf323232;
  private Snackbar messageSnackbar;
  private enum DismissBehavior { HIDE, SHOW, FINISH };
  private int maxLines = 2;
  private String lastMessage = "";
  private View snackbarView;

  public boolean isShowing() {
    return messageSnackbar != null;
  }

  /** Shows a snackbar with a given message. */
  public void showMessage(Activity activity, String message) {
    if (!message.isEmpty() && (!isShowing() || !lastMessage.equals(message))) {
      lastMessage = message;
      show(activity, message, DismissBehavior.HIDE);
    }
  }

  /** Shows a snackbar with a given message, and a dismiss button. */
  public void showMessageWithDismiss(Activity activity, String message) {
    show(activity, message, DismissBehavior.SHOW);
  }

  /** Shows a snackbar with a given message for Snackbar.LENGTH_SHORT milliseconds */
  public void showMessageForShortDuration(Activity activity, String message) {
    show(activity, message, DismissBehavior.SHOW, Snackbar.LENGTH_SHORT);
  }

  /** Shows a snackbar with a given message for Snackbar.LENGTH_LONG milliseconds */
  public void showMessageForLongDuration(Activity activity, String message) {
    show(activity, message, DismissBehavior.SHOW, Snackbar.LENGTH_LONG);
  }

  /**
   * Shows a snackbar with a given error message. When dismissed, will finish the activity. Useful
   * for notifying errors, where no further interaction with the activity is possible.
   */
  public void showError(Activity activity, String errorMessage) {
    show(activity, errorMessage, DismissBehavior.FINISH);
  }

  /**
   * Hides the currently showing snackbar, if there is one. Safe to call from any thread. Safe to
   * call even if snackbar is not shown.
   */
  public void hide(Activity activity) {
    if (!isShowing()) {
      return;
    }
    lastMessage = "";
    Snackbar messageSnackbarToHide = messageSnackbar;
    messageSnackbar = null;
    activity.runOnUiThread(
        new Runnable() {
          @Override
          public void run() {
            messageSnackbarToHide.dismiss();
          }
        });
  }

  public void setMaxLines(int lines) {
    maxLines = lines;
  }

  /**
   * Sets the view that will be used to find a suitable parent view to hold the Snackbar view.
   *
   * <p>To use the root layout ({@link android.R.id.content}), pass in {@code null}.
   *
   * @param snackbarView the view to pass to {@link
   *     com.google.android.material.snackbar.Snackbar#make(…)} which will be used to find a
   *     suitable parent, which is a {@link androidx.coordinatorlayout.widget.CoordinatorLayout}, or
   *     the window decor's content view, whichever comes first.
   */
  public void setParentView(View snackbarView) {
    this.snackbarView = snackbarView;
  }

  private void show(Activity activity, String message, DismissBehavior dismissBehavior) {
    show(activity, message, dismissBehavior, Snackbar.LENGTH_INDEFINITE);
  }

  private void show(
      final Activity activity,
      final String message,
      final DismissBehavior dismissBehavior,
      int duration) {
    activity.runOnUiThread(
        new Runnable() {
          @Override
          public void run() {
            messageSnackbar =
                Snackbar.make(
                    snackbarView == null
                        ? activity.findViewById(android.R.id.content)
                        : snackbarView,
                    message,
                    duration);
            messageSnackbar.getView().setBackgroundColor(BACKGROUND_COLOR);
            if (dismissBehavior != DismissBehavior.HIDE && duration == Snackbar.LENGTH_INDEFINITE) {
              messageSnackbar.setAction(
                  "Dismiss",
                  new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                      messageSnackbar.dismiss();
                    }
                  });
              if (dismissBehavior == DismissBehavior.FINISH) {
                messageSnackbar.addCallback(
                    new BaseTransientBottomBar.BaseCallback<Snackbar>() {
                      @Override
                      public void onDismissed(Snackbar transientBottomBar, int event) {
                        super.onDismissed(transientBottomBar, event);
                        activity.finish();
                      }
                    });
              }
            }
            ((TextView)
                    messageSnackbar
                        .getView()
                        .findViewById(com.google.android.material.R.id.snackbar_text))
                .setMaxLines(maxLines);
            messageSnackbar.show();
          }
        });
  }
}
