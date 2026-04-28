package com.google.ar.core.examples.kotlin.geospatial

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


import android.app.AlertDialog
import android.app.Dialog
import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import androidx.fragment.app.DialogFragment

/** A DialogFragment for the Privacy Notice Dialog Box. */
class PrivacyNoticeDialogFragment : DialogFragment() {

    /** Listener for a privacy notice response. */
    interface NoticeDialogListener {
        /** Invoked when the user accepts sharing experience. */
        fun onDialogPositiveClick(dialog: DialogFragment)
    }

    private var noticeDialogListener: NoticeDialogListener? = null

    override fun onAttach(context: Context) {
        super.onAttach(context)
        // Verify that the host activity implements the callback interface
        noticeDialogListener = context as? NoticeDialogListener
            ?: throw AssertionError("Must implement NoticeDialogListener")
    }

    override fun onDetach() {
        super.onDetach()
        noticeDialogListener = null
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        return AlertDialog.Builder(requireActivity(), R.style.AlertDialogCustom)
            .setTitle(R.string.share_experience_title)
            .setMessage(R.string.share_experience_message)
            .setPositiveButton(R.string.agree_to_share) { _, _ ->
                noticeDialogListener?.onDialogPositiveClick(this)
            }
            .setNegativeButton(R.string.learn_more) { _, _ ->
                val browserIntent = Intent(Intent.ACTION_VIEW, Uri.parse(getString(R.string.learn_more_url)))
                requireActivity().startActivity(browserIntent)
            }
            .create()
    }

    companion object {
        fun createDialog(): PrivacyNoticeDialogFragment = PrivacyNoticeDialogFragment()
    }
}