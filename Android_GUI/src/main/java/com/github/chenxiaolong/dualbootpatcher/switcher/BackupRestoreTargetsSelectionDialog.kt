/*
 * Copyright (C) 2016-2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
import com.afollestad.materialdialogs.WhichButton
import com.afollestad.materialdialogs.actions.setActionButtonEnabled
import com.afollestad.materialdialogs.list.listItemsMultiChoice
import com.github.chenxiaolong.dualbootpatcher.R
import com.github.chenxiaolong.dualbootpatcher.switcher.actions.BackupRestoreParams.Action

class BackupRestoreTargetsSelectionDialog : DialogFragment() {
    internal val owner: BackupRestoreTargetsSelectionDialogListener?
        get() {
            val f = targetFragment
            return if (f == null) {
                if (activity !is BackupRestoreTargetsSelectionDialogListener) {
                    throw IllegalStateException(
                            "Parent activity must implement BackupRestoreTargetsSelectionDialogListener")
                }
                activity as BackupRestoreTargetsSelectionDialogListener?
            } else {
                targetFragment as BackupRestoreTargetsSelectionDialogListener?
            }
        }

    interface BackupRestoreTargetsSelectionDialogListener {
        fun onSelectedBackupRestoreTargets(targets: Array<String>)
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val items = listOf(
                getString(R.string.br_target_system),
                getString(R.string.br_target_cache),
                getString(R.string.br_target_data),
                getString(R.string.br_target_boot),
                getString(R.string.br_target_config)
        )

        val action = arguments!!.getSerializable(ARG_ACTION) as Action
        val descResid = when (action) {
            Action.BACKUP -> R.string.br_backup_targets_dialog_desc
            Action.RESTORE -> R.string.br_restore_targets_dialog_desc
        }

        val dialog = MaterialDialog(requireActivity())
                .message(descResid)
                .negativeButton(R.string.cancel)
                .positiveButton(R.string.ok)
                .listItemsMultiChoice(items = items) { dialog, indices, _ ->
                    dialog.setActionButtonEnabled(WhichButton.POSITIVE, indices.isNotEmpty())

                    val selected = Array(indices.size) {
                        val arrIndex = indices[it]

                        when (arrIndex) {
                            0 -> "system"
                            1 -> "cache"
                            2 -> "data"
                            3 -> "boot"
                            4 -> "config"
                            else -> throw IllegalStateException()
                        }
                    }

                    owner?.onSelectedBackupRestoreTargets(selected)
                }

        isCancelable = false
        dialog.setCanceledOnTouchOutside(false)

        return dialog
    }

    companion object {
        private const val ARG_ACTION = "action"

        fun newInstanceFromFragment(parent: Fragment?, action: Action):
                BackupRestoreTargetsSelectionDialog {
            if (parent != null) {
                if (parent !is BackupRestoreTargetsSelectionDialogListener) {
                    throw IllegalStateException(
                            "Parent fragment must implement BackupRestoreTargetsSelectionDialogListener")
                }
            }

            val args = Bundle()
            args.putSerializable(ARG_ACTION, action)

            val frag = BackupRestoreTargetsSelectionDialog()
            frag.setTargetFragment(parent, 0)
            frag.arguments = args
            return frag
        }

        fun newInstanceFromActivity(action: Action): BackupRestoreTargetsSelectionDialog {
            val args = Bundle()
            args.putSerializable(ARG_ACTION, action)

            val frag = BackupRestoreTargetsSelectionDialog()
            frag.arguments = args
            return frag
        }
    }
}
