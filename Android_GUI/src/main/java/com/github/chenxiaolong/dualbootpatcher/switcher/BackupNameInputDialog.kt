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

package com.github.chenxiaolong.dualbootpatcher.switcher

import android.app.Dialog
import android.os.Bundle
import android.text.InputType
import androidx.fragment.app.DialogFragment
import androidx.fragment.app.Fragment
import com.afollestad.materialdialogs.MaterialDialog
import com.afollestad.materialdialogs.WhichButton
import com.afollestad.materialdialogs.actions.setActionButtonEnabled
import com.afollestad.materialdialogs.input.getInputField
import com.afollestad.materialdialogs.input.input
import com.github.chenxiaolong.dualbootpatcher.R

class BackupNameInputDialog : DialogFragment() {
    internal val owner: BackupNameInputDialogListener?
        get() {
            val f = targetFragment
            return if (f == null) {
                if (activity !is BackupNameInputDialogListener) {
                    throw IllegalStateException(
                            "Parent activity must implement BackupNameInputDialogListener")
                }
                activity as BackupNameInputDialogListener?
            } else {
                targetFragment as BackupNameInputDialogListener?
            }
        }

    interface BackupNameInputDialogListener {
        fun onSelectedBackupName(name: String)
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val suggestedName = arguments!!.getString(ARG_SUGGESTED_NAME)
        val inputType = InputType.TYPE_TEXT_FLAG_AUTO_COMPLETE or
                InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS

        val dialog = MaterialDialog(requireActivity())
                .message(R.string.br_name_dialog_desc)
                .negativeButton(R.string.cancel)
                .positiveButton(R.string.ok) { dialog ->
                    val owner = owner
                    if (owner != null) {
                        val editText = dialog.getInputField()
                        if (editText != null) {
                            owner.onSelectedBackupName(editText.text.toString())
                        }
                    }
                }
                .input(hintRes = R.string.br_name_dialog_hint, prefill = suggestedName,
                        inputType = inputType, allowEmpty = false, waitForPositiveButton = false)
                { dialog, input ->
                    val text = input.toString()
                    if (text.contains("/") || text == "." || text == "..") {
                        dialog.setActionButtonEnabled(WhichButton.POSITIVE, false)
                    }
                }

        isCancelable = false
        dialog.setCanceledOnTouchOutside(false)

        return dialog
    }

    companion object {
        private const val ARG_SUGGESTED_NAME = "suggested_name"

        fun newInstanceFromFragment(parent: Fragment?, suggestedName: String):
                BackupNameInputDialog {
            if (parent != null) {
                if (parent !is BackupNameInputDialogListener) {
                    throw IllegalStateException(
                            "Parent fragment must implement BackupNameInputDialogListener")
                }
            }

            val args = Bundle()
            args.putString(ARG_SUGGESTED_NAME, suggestedName)

            val frag = BackupNameInputDialog()
            frag.setTargetFragment(parent, 0)
            frag.arguments = args
            return frag
        }

        fun newInstanceFromActivity(suggestedName: String): BackupNameInputDialog {
            val args = Bundle()
            args.putString(ARG_SUGGESTED_NAME, suggestedName)

            val frag = BackupNameInputDialog()
            frag.arguments = args
            return frag
        }
    }
}
