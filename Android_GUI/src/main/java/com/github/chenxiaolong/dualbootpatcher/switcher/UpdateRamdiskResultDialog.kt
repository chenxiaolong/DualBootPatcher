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
import androidx.fragment.app.DialogFragment
import androidx.fragment.app.Fragment
import com.afollestad.materialdialogs.MaterialDialog
import com.github.chenxiaolong.dualbootpatcher.R

class UpdateRamdiskResultDialog : DialogFragment() {
    internal val owner: UpdateRamdiskResultDialogListener?
        get() {
            val f = targetFragment
            return if (f == null) {
                if (activity !is UpdateRamdiskResultDialogListener) {
                    throw IllegalStateException(
                            "Parent activity must implement UpdateRamdiskResultDialogListener")
                }
                activity as UpdateRamdiskResultDialogListener?
            } else {
                targetFragment as UpdateRamdiskResultDialogListener?
            }
        }

    interface UpdateRamdiskResultDialogListener {
        fun onConfirmUpdatedRamdisk(reboot: Boolean)
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val succeeded = arguments!!.getBoolean(ARG_SUCCEEDED)
        val allowReboot = arguments!!.getBoolean(ARG_ALLOW_REBOOT)

        val dialog = MaterialDialog(requireActivity())

        if (!succeeded) {
            dialog.title(R.string.update_ramdisk_failure_title)
            dialog.message(R.string.update_ramdisk_failure_desc)

            dialog.negativeButton(R.string.ok)
        } else {
            dialog.title(R.string.update_ramdisk_success_title)

            if (allowReboot) {
                dialog.message(R.string.update_ramdisk_reboot_desc)
                dialog.negativeButton(R.string.reboot_later) {
                    confirm(false)
                }
                dialog.positiveButton(R.string.reboot_now) {
                    confirm(true)
                }
            } else {
                dialog.message(R.string.update_ramdisk_no_reboot_desc)
                dialog.positiveButton(R.string.ok) {
                    confirm(false)
                }
            }
        }

        isCancelable = false
        dialog.setCanceledOnTouchOutside(false)

        return dialog
    }

    private fun confirm(reboot: Boolean) {
        owner?.onConfirmUpdatedRamdisk(reboot)
    }

    companion object {
        private const val ARG_SUCCEEDED = "succeeded"
        private const val ARG_ALLOW_REBOOT = "allow_reboot"

        fun newInstanceFromFragment(parent: Fragment?, succeeded: Boolean, allowReboot: Boolean):
                UpdateRamdiskResultDialog {
            if (parent != null) {
                if (parent !is UpdateRamdiskResultDialogListener) {
                    throw IllegalStateException(
                            "Parent fragment must implement UpdateRamdiskResultDialogListener")
                }
            }

            val frag = UpdateRamdiskResultDialog()
            frag.setTargetFragment(parent, 0)
            val args = Bundle()
            args.putBoolean(ARG_SUCCEEDED, succeeded)
            args.putBoolean(ARG_ALLOW_REBOOT, allowReboot)
            frag.arguments = args
            return frag
        }

        fun newInstanceFromActivity(succeeded: Boolean, allowReboot: Boolean):
                UpdateRamdiskResultDialog {
            val frag = UpdateRamdiskResultDialog()
            val args = Bundle()
            args.putBoolean(ARG_SUCCEEDED, succeeded)
            args.putBoolean(ARG_ALLOW_REBOOT, allowReboot)
            frag.arguments = args
            return frag
        }
    }
}
