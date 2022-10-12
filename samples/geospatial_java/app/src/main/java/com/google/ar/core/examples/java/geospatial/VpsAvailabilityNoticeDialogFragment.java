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

package com.google.ar.core.examples.java.geospatial;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import androidx.fragment.app.DialogFragment;

/** A DialogFragment for the VPS availability Notice Dialog Box. */
public class VpsAvailabilityNoticeDialogFragment extends DialogFragment {

  /** Listener for a VPS availability notice response. */
  public interface NoticeDialogListener {

    /** Invoked when the user accepts sharing experience. */
    void onDialogContinueClick(DialogFragment dialog);
  }

  NoticeDialogListener noticeDialogListener;

  static VpsAvailabilityNoticeDialogFragment createDialog() {
    VpsAvailabilityNoticeDialogFragment dialogFragment = new VpsAvailabilityNoticeDialogFragment();
    return dialogFragment;
  }

  @Override
  public void onAttach(Context context) {
    super.onAttach(context);
    // Verify that the host activity implements the callback interface
    try {
      noticeDialogListener = (NoticeDialogListener) context;
    } catch (ClassCastException e) {
      throw new AssertionError("Must implement NoticeDialogListener", e);
    }
  }

  @Override
  public void onDetach() {
    super.onDetach();
    noticeDialogListener = null;
  }

  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState) {
    AlertDialog.Builder builder = new AlertDialog.Builder(getActivity(), R.style.AlertDialogCustom);
    builder
        .setTitle(R.string.vps_unavailable_title)
        .setMessage(R.string.vps_unavailable_message)
        .setCancelable(false)
        .setPositiveButton(
            R.string.continue_button,
            new DialogInterface.OnClickListener() {
              @Override
              public void onClick(DialogInterface dialog, int id) {
                // Send the positive button event back to the host activity
                noticeDialogListener.onDialogContinueClick(
                    VpsAvailabilityNoticeDialogFragment.this);
              }
            });
    Dialog dialog = builder.create();
    dialog.setCanceledOnTouchOutside(false);
    return dialog;
  }
}
