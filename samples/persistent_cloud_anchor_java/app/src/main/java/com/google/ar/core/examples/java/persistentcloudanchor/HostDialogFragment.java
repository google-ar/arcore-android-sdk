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
 * limitations under the License.
 */

package com.google.ar.core.examples.java.persistentcloudanchor;

import android.app.AlertDialog;
import android.app.Dialog;
import android.os.Bundle;
import android.text.Editable;
import android.view.View;
import android.widget.EditText;
import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.FragmentActivity;
import com.google.common.base.Preconditions;

/** A DialogFragment for the Save Anchor Dialog Box. */
public class HostDialogFragment extends DialogFragment {

  interface OkListener {
    /**
     * This method is called by the dialog box when its OK button is pressed.
     *
     * @param dialogValue the long value from the dialog box
     */
    void onOkPressed(String dialogValue);
  }

  private EditText nicknameField;
  private OkListener okListener;

  public void setOkListener(OkListener okListener) {
    this.okListener = okListener;
  }

  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    String defaultNickname = getArguments().getString("nickname");
    FragmentActivity activity =
        Preconditions.checkNotNull(getActivity(), "The activity cannot be null.");
    AlertDialog.Builder builder = new AlertDialog.Builder(activity);

    // Passing null as the root is fine, because the view is for a dialog.
    View dialogView = activity.getLayoutInflater().inflate(R.layout.save_anchor_dialog, null);
    nicknameField = dialogView.findViewById(R.id.nickname_edit_text);
    nicknameField.setText(defaultNickname);
    builder
        .setView(dialogView)
        .setTitle(R.string.nickname_title_text)
        .setPositiveButton(
            R.string.nickname_dialog_ok,
            (dialog, which) -> {
              Editable nicknameText = nicknameField.getText();
              if (okListener != null) {
                okListener.onOkPressed(nicknameText.toString());
              }
            });
    return builder.create();
  }
}
