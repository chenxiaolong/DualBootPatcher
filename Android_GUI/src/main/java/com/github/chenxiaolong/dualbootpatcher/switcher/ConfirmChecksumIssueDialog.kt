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

class ConfirmChecksumIssueDialog : DialogFragment() {
    internal val owner: ConfirmChecksumIssueDialogListener?
        get() {
            val f = targetFragment
            return if (f == null) {
                if (activity !is ConfirmChecksumIssueDialogListener) {
                    throw IllegalStateException(
                            "Parent activity must implement ConfirmChecksumIssueDialogListener")
                }
                activity as ConfirmChecksumIssueDialogListener?
            } else {
                targetFragment as ConfirmChecksumIssueDialogListener?
            }
        }

    interface ConfirmChecksumIssueDialogListener {
        fun onConfirmChecksumIssue(romId: String)
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val issue = arguments!!.getInt(ARG_ISSUE)
        val romId = arguments!!.getString(ARG_ROM_ID)!!

        val message = when (issue) {
            CHECKSUM_INVALID -> getString(R.string.checksums_invalid, romId)
            CHECKSUM_MISSING -> getString(R.string.checksums_missing, romId)
            else -> throw IllegalArgumentException("Invalid value for issue param: $issue")
        }

        val dialog = MaterialDialog(requireActivity())
                .message(text = message)
                .positiveButton(R.string.proceed) {
                    owner?.onConfirmChecksumIssue(romId)
                }
                .negativeButton(R.string.cancel)

        isCancelable = false
        dialog.setCanceledOnTouchOutside(false)

        return dialog
    }

    companion object {
        const val CHECKSUM_INVALID = 1
        const val CHECKSUM_MISSING = 2

        private const val ARG_ISSUE = "issue"
        private const val ARG_ROM_ID = "rom_id"

        fun newInstanceFromFragment(parent: Fragment?, issue: Int, romId: String):
                ConfirmChecksumIssueDialog {
            if (parent != null) {
                if (parent !is ConfirmChecksumIssueDialogListener) {
                    throw IllegalStateException(
                            "Parent fragment must implement ConfirmChecksumIssueDialogListener")
                }
            }

            when (issue) {
                CHECKSUM_INVALID, CHECKSUM_MISSING -> {}
                else -> throw IllegalArgumentException("Invalid value for issue param: $issue")
            }

            val frag = ConfirmChecksumIssueDialog()
            frag.setTargetFragment(parent, 0)
            val args = Bundle()
            args.putInt(ARG_ISSUE, issue)
            args.putString(ARG_ROM_ID, romId)
            frag.arguments = args
            return frag
        }

        fun newInstanceFromActivity(issue: Int, romId: String): ConfirmChecksumIssueDialog {
            when (issue) {
                CHECKSUM_INVALID, CHECKSUM_MISSING -> {}
                else -> throw IllegalArgumentException("Invalid value for issue param: $issue")
            }

            val frag = ConfirmChecksumIssueDialog()
            val args = Bundle()
            args.putInt(ARG_ISSUE, issue)
            args.putString(ARG_ROM_ID, romId)
            frag.arguments = args
            return frag
        }
    }
}
