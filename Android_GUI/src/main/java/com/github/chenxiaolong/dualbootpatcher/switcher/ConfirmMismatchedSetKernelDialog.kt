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

class ConfirmMismatchedSetKernelDialog : DialogFragment() {
    internal val owner: ConfirmMismatchedSetKernelDialogListener?
        get() {
            val f = targetFragment
            return if (f == null) {
                if (activity !is ConfirmMismatchedSetKernelDialogListener) {
                    throw IllegalStateException(
                            "Parent activity must implement ConfirmMismatchedSetKernelDialogListener")
                }
                activity as ConfirmMismatchedSetKernelDialogListener?
            } else {
                targetFragment as ConfirmMismatchedSetKernelDialogListener?
            }
        }

    interface ConfirmMismatchedSetKernelDialogListener {
        fun onConfirmMismatchedSetKernel()
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val currentRom = arguments!!.getString(ARG_CURRENT_ROM)
        val targetRom = arguments!!.getString(ARG_TARGET_ROM)
        val message = String.format(
                getString(R.string.set_kernel_mismatched_rom), currentRom, targetRom)

        val dialog = MaterialDialog(requireActivity())
                .message(text = message)
                .positiveButton(R.string.proceed) {
                    owner?.onConfirmMismatchedSetKernel()
                }
                .negativeButton(R.string.cancel)

        isCancelable = false
        dialog.setCanceledOnTouchOutside(false)

        return dialog
    }

    companion object {
        private const val ARG_CURRENT_ROM = "current_rom"
        private const val ARG_TARGET_ROM = "target_rom"

        fun newInstanceFromFragment(parent: Fragment?, currentRom: String, targetRom: String):
                ConfirmMismatchedSetKernelDialog {
            if (parent != null) {
                if (parent !is ConfirmMismatchedSetKernelDialogListener) {
                    throw IllegalStateException(
                            "Parent fragment must implement ConfirmMismatchedSetKernelDialogListener")
                }
            }

            val frag = ConfirmMismatchedSetKernelDialog()
            frag.setTargetFragment(parent, 0)
            val args = Bundle()
            args.putString(ARG_CURRENT_ROM, currentRom)
            args.putString(ARG_TARGET_ROM, targetRom)
            frag.arguments = args
            return frag
        }

        fun newInstanceFromActivity(currentRom: String, targetRom: String):
                ConfirmMismatchedSetKernelDialog {
            val frag = ConfirmMismatchedSetKernelDialog()
            val args = Bundle()
            args.putString(ARG_CURRENT_ROM, currentRom)
            args.putString(ARG_TARGET_ROM, targetRom)
            frag.arguments = args
            return frag
        }
    }
}
