/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
import android.support.v4.app.DialogFragment
import android.support.v4.app.Fragment
import android.text.InputType
import com.afollestad.materialdialogs.DialogAction
import com.afollestad.materialdialogs.MaterialDialog
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

        val dialog = MaterialDialog.Builder(activity!!)
                .content(R.string.br_name_dialog_desc)
                .negativeText(R.string.cancel)
                .positiveText(R.string.ok)
                .inputType(InputType.TYPE_TEXT_FLAG_AUTO_COMPLETE or InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS)
                .input(getString(R.string.br_name_dialog_hint), suggestedName, false) { dialog, input ->
                    val text = input.toString()
                    if (text.contains("/") || text == "." || text == "..") {
                        dialog.getActionButton(DialogAction.POSITIVE).isEnabled = false
                    }
                }
                .alwaysCallInputCallback()
                .onPositive { dialog, _ ->
                    val owner = owner
                    if (owner != null) {
                        val editText = dialog.inputEditText
                        if (editText != null) {
                            owner.onSelectedBackupName(editText.text.toString())
                        }
                    }
                }
                .build()

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
