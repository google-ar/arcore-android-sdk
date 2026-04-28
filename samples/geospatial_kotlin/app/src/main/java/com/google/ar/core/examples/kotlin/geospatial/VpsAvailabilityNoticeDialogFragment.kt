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
import android.os.Bundle
import androidx.fragment.app.DialogFragment

/** A DialogFragment for the VPS availability Notice Dialog Box. */
class VpsAvailabilityNoticeDialogFragment : DialogFragment() {

    /** Listener for a VPS availability notice response. */
    interface NoticeDialogListener {
        /** Invoked when the user accepts sharing experience. */
        fun onDialogContinueClick(dialog: DialogFragment)
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
        val dialog = AlertDialog.Builder(requireActivity(), R.style.AlertDialogCustom)
            .setTitle(R.string.vps_unavailable_title)
            .setMessage(R.string.vps_unavailable_message)
            .setCancelable(false)
            .setPositiveButton(R.string.continue_button) { _, _ ->
                noticeDialogListener?.onDialogContinueClick(this)
            }
            .create()
        dialog.setCanceledOnTouchOutside(false)
        return dialog
    }

    companion object {
        fun createDialog(): VpsAvailabilityNoticeDialogFragment = VpsAvailabilityNoticeDialogFragment()
    }
}