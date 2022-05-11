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
import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import androidx.fragment.app.DialogFragment;

/** A DialogFragment for the Privacy Notice Dialog Box. */
public class PrivacyNoticeDialogFragment extends DialogFragment {

  /** Listener for weather to start a host or resolve operation. */
  public interface HostResolveListener {

    /** Invoked when the user accepts sharing experience. */
    void onPrivacyNoticeReceived();
  }

  HostResolveListener hostResolveListener;

  static PrivacyNoticeDialogFragment createDialog(HostResolveListener hostResolveListener) {
    PrivacyNoticeDialogFragment dialogFragment = new PrivacyNoticeDialogFragment();
    dialogFragment.hostResolveListener = hostResolveListener;
    return dialogFragment;
  }

  @Override
  public void onDetach() {
    super.onDetach();
    hostResolveListener = null;
  }

  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState) {
    AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
    builder
        .setTitle(R.string.share_experience_title)
        .setMessage(R.string.share_experience_message)
        .setPositiveButton(
            R.string.agree_to_share,
            new DialogInterface.OnClickListener() {
              @Override
              public void onClick(DialogInterface dialog, int id) {
                // Send the positive button event back to the host activity
                hostResolveListener.onPrivacyNoticeReceived();
              }
            })
        .setNegativeButton(
            R.string.learn_more,
            new DialogInterface.OnClickListener() {
              @Override
              public void onClick(DialogInterface dialog, int id) {
                Intent browserIntent =
                    new Intent(
                        Intent.ACTION_VIEW, Uri.parse(getString(R.string.learn_more_url)));
                getActivity().startActivity(browserIntent);
              }
            });
    return builder.create();
  }
}
