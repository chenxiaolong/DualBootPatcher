/*
 * Copyright (C) 2014-2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher.dialogs

import android.app.Dialog
import android.os.Bundle
import androidx.fragment.app.DialogFragment
import androidx.fragment.app.Fragment
import com.afollestad.materialdialogs.MaterialDialog
import com.afollestad.materialdialogs.checkbox.checkBoxPrompt
import com.afollestad.materialdialogs.checkbox.isCheckPromptChecked
import com.github.chenxiaolong.dualbootpatcher.R

class FirstUseDialog : DialogFragment() {
    internal val owner: FirstUseDialogListener?
        get() = targetFragment as FirstUseDialogListener?

    interface FirstUseDialogListener {
        fun onConfirmFirstUse(dontShowAgain: Boolean)
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val titleResId = arguments!!.getInt(ARG_TITLE_RES_ID)
        val messageResId = arguments!!.getInt(ARG_MESSAGE_RES_ID)

        val dialog = MaterialDialog(requireActivity())
                .checkBoxPrompt(R.string.do_not_show_again, onToggle = null)
                .positiveButton(R.string.ok) { dialog ->
                    val owner = owner
                    owner?.onConfirmFirstUse(dialog.isCheckPromptChecked())
                }

        if (titleResId != 0) {
            dialog.title(titleResId)
        }
        if (messageResId != 0) {
            dialog.message(messageResId)
        }

        isCancelable = false
        dialog.setCanceledOnTouchOutside(false)

        return dialog
    }

    companion object {
        private const val ARG_TITLE_RES_ID = "titleResId"
        private const val ARG_MESSAGE_RES_ID = "messageResId"

        fun newInstance(parent: Fragment?, titleResId: Int, messageResId: Int): FirstUseDialog {
            if (parent != null) {
                if (parent !is FirstUseDialogListener) {
                    throw IllegalStateException(
                            "Parent fragment must implement FirstUseDialogListener")
                }
            }

            val frag = FirstUseDialog()
            frag.setTargetFragment(parent, 0)
            val args = Bundle()
            args.putInt(ARG_TITLE_RES_ID, titleResId)
            args.putInt(ARG_MESSAGE_RES_ID, messageResId)
            frag.arguments = args
            return frag
        }
    }
}
