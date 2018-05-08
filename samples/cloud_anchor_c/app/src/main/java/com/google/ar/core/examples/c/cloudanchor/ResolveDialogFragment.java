/*
 * Copyright 2018 Google Inc. All Rights Reserved.
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

package com.google.ar.core.examples.c.cloudanchor;

import android.app.AlertDialog;
import android.app.Dialog;
import android.os.Bundle;
import android.support.v4.app.DialogFragment;
import android.text.InputType;
import android.view.ViewGroup.LayoutParams;
import android.widget.EditText;
import android.widget.LinearLayout;

/** A DialogFragment for the Resolve Dialog Box. */
public class ResolveDialogFragment extends DialogFragment {
  interface ResultListener {
    /**
     * This method is called by the dialog box when its OK button is pressed.
     *
     * @param dialogValue the long value from the dialog box
     */
    void onOkPressed(Long dialogValue);

    /** This method is called by the dialog box when its cancel button is pressed. */
    void onCancelPressed();
  }

  private EditText roomCodeField;
  private ResultListener resultListener;
  private boolean okOrCancelWasPressed;

  public void setResultListener(ResultListener resultListener) {
    this.resultListener = resultListener;
  }

  private LinearLayout createLayout() {
    LinearLayout layout = new LinearLayout(getContext());
    roomCodeField = new EditText(getContext());
    roomCodeField.setHint(R.string.resolve_edit_hint);
    roomCodeField.setInputType(InputType.TYPE_CLASS_NUMBER);
    roomCodeField.setLayoutParams(
        new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));
    layout.addView(roomCodeField);
    layout.setLayoutParams(new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));
    return layout;
  }

  @Override
  public void onDetach() {
    super.onDetach();
    if (!okOrCancelWasPressed) {
      if (resultListener != null) {
        resultListener.onCancelPressed();
      }
    }
  }

  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState) {
    AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
    builder
        .setView(createLayout())
        .setTitle(R.string.resolve_dialog_title)
        .setPositiveButton(
            R.string.resolve_ok,
            (dialog, id) -> {
              if (resultListener != null) {
                try {
                  Long longVal = Long.valueOf(roomCodeField.getText().toString());
                  if (longVal != null) {
                    resultListener.onOkPressed(longVal);
                  }
                } catch (NumberFormatException exception) {
                  // Treat invalid numbers equivalent to pushing cancel.
                  resultListener.onCancelPressed();
                } finally {
                  okOrCancelWasPressed = true;
                }
              }
            })
        .setNegativeButton(
            R.string.cancel,
            (dialog, id) -> {
              if (resultListener != null) {
                okOrCancelWasPressed = true;
                resultListener.onCancelPressed();
              }
            })
        .setCancelable(false);
    return builder.create();
  }
}
