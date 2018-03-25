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
import com.afollestad.materialdialogs.MaterialDialog
import com.github.chenxiaolong.dualbootpatcher.R
import mbtool.daemon.v3.MbWipeTarget

class WipeTargetsSelectionDialog : DialogFragment() {
    internal val owner: WipeTargetsSelectionDialogListener?
        get() {
            val f = targetFragment
            return if (f == null) {
                if (activity !is WipeTargetsSelectionDialogListener) {
                    throw IllegalStateException(
                            "Parent activity must implement WipeTargetsSelectionDialogListener")
                }
                activity as WipeTargetsSelectionDialogListener?
            } else {
                targetFragment as WipeTargetsSelectionDialogListener?
            }
        }

    interface WipeTargetsSelectionDialogListener {
        fun onSelectedWipeTargets(targets: ShortArray)
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val items = arrayOf(
                getString(R.string.wipe_target_system),
                getString(R.string.wipe_target_cache),
                getString(R.string.wipe_target_data),
                getString(R.string.wipe_target_dalvik_cache),
                getString(R.string.wipe_target_multiboot_files)
        )

        val dialog = MaterialDialog.Builder(activity!!)
                .title(R.string.wipe_rom_dialog_title)
                .items(*items)
                .negativeText(R.string.cancel)
                .positiveText(R.string.ok)
                .itemsCallbackMultiChoice(null) { _, which, _ ->
                    val targets = ShortArray(which.size)

                    for (i in which.indices) {
                        val arrIndex = which[i]

                        when (arrIndex) {
                            0 -> targets[i] = MbWipeTarget.SYSTEM
                            1 -> targets[i] = MbWipeTarget.CACHE
                            2 -> targets[i] = MbWipeTarget.DATA
                            3 -> targets[i] = MbWipeTarget.DALVIK_CACHE
                            4 -> targets[i] = MbWipeTarget.MULTIBOOT
                        }
                    }

                    val owner = owner
                    owner?.onSelectedWipeTargets(targets)

                    true
                }
                .build()

        isCancelable = false
        dialog.setCanceledOnTouchOutside(false)

        return dialog
    }

    companion object {
        fun newInstanceFromFragment(parent: Fragment?): WipeTargetsSelectionDialog {
            if (parent != null) {
                if (parent !is WipeTargetsSelectionDialogListener) {
                    throw IllegalStateException(
                            "Parent fragment must implement WipeTargetsSelectionDialogListener")
                }
            }

            val frag = WipeTargetsSelectionDialog()
            frag.setTargetFragment(parent, 0)
            return frag
        }

        fun newInstanceFromActivity(): WipeTargetsSelectionDialog {
            return WipeTargetsSelectionDialog()
        }
    }
}
