/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
import com.afollestad.materialdialogs.DialogAction
import com.afollestad.materialdialogs.MaterialDialog
import com.github.chenxiaolong.dualbootpatcher.R
import com.github.chenxiaolong.dualbootpatcher.switcher.actions.BackupRestoreParams.Action
import java.util.*

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
        val items = arrayOf(
                getString(R.string.br_target_system),
                getString(R.string.br_target_cache),
                getString(R.string.br_target_data),
                getString(R.string.br_target_boot),
                getString(R.string.br_target_config)
        )

        val action = arguments!!.getSerializable(ARG_ACTION) as Action
        var descResid = 0

        if (action == Action.BACKUP) {
            descResid = R.string.br_backup_targets_dialog_desc
        } else if (action == Action.RESTORE) {
            descResid = R.string.br_restore_targets_dialog_desc
        }

        val selected = ArrayList<String>(items.size)

        @Suppress("UNCHECKED_CAST")
        val dialog = MaterialDialog.Builder(activity!!)
                .content(descResid)
                .items(*items as Array<CharSequence>)
                .negativeText(R.string.cancel)
                .positiveText(R.string.ok)
                .itemsCallbackMultiChoice(null) { dialog, which, _ ->
                    dialog.getActionButton(DialogAction.POSITIVE).isEnabled = which.isNotEmpty()

                    selected.clear()
                    for (arrIndex in which) {
                        when (arrIndex) {
                            0 -> selected.add("system")
                            1 -> selected.add("cache")
                            2 -> selected.add("data")
                            3 -> selected.add("boot")
                            4 -> selected.add("config")
                        }
                    }

                    true
                }
                .alwaysCallMultiChoiceCallback()
                .onPositive { _, _ ->
                    val owner = owner
                    if (owner != null) {
                        val targets = selected.toTypedArray()
                        owner.onSelectedBackupRestoreTargets(targets)
                    }
                }
                .build()

        // No targets selected by default, so don't enable OK button
        dialog.getActionButton(DialogAction.POSITIVE).isEnabled = false

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
