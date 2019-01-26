/*
 * Copyright (C) 2015-2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.github.chenxiaolong.dualbootpatcher.switcher

import android.app.Dialog
import android.os.Bundle
import androidx.fragment.app.DialogFragment
import androidx.fragment.app.Fragment
import com.afollestad.materialdialogs.MaterialDialog
import com.github.chenxiaolong.dualbootpatcher.R

class SetKernelNeededDialog : DialogFragment() {
    internal val owner: SetKernelNeededDialogListener?
        get() = targetFragment as SetKernelNeededDialogListener?

    interface SetKernelNeededDialogListener {
        fun onConfirmSetKernelNeeded()
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val messageResId = arguments!!.getInt(ARG_MESSAGE_RES_ID)

        val dialog = MaterialDialog(requireActivity())
                .message(messageResId)
                .positiveButton(R.string.set_kernel_now) {
                    owner?.onConfirmSetKernelNeeded()
                }
                .negativeButton(R.string.set_kernel_later)

        isCancelable = false
        dialog.setCanceledOnTouchOutside(false)

        return dialog
    }

    companion object {
        private const val ARG_MESSAGE_RES_ID = "message_res_id"

        fun newInstance(parent: Fragment?, messageResId: Int): SetKernelNeededDialog {
            if (parent != null) {
                if (parent !is SetKernelNeededDialogListener) {
                    throw IllegalStateException(
                            "Parent fragment must implement SetKernelNeededDialogListener")
                }
            }

            val frag = SetKernelNeededDialog()
            frag.setTargetFragment(parent, 0)
            val args = Bundle()
            args.putInt(ARG_MESSAGE_RES_ID, messageResId)
            frag.arguments = args
            return frag
        }
    }
}