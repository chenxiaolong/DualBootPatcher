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
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation

class SetKernelConfirmDialog : DialogFragment() {
    internal val owner: SetKernelConfirmDialogListener?
        get() {
            val f = targetFragment
            return if (f == null) {
                if (activity !is SetKernelConfirmDialogListener) {
                    throw IllegalStateException(
                            "Parent activity must implement SetKernelConfirmDialogListener")
                }
                activity as SetKernelConfirmDialogListener?
            } else {
                targetFragment as SetKernelConfirmDialogListener?
            }
        }

    interface SetKernelConfirmDialogListener {
        fun onConfirmSetKernel()
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val info = arguments!!.getParcelable<RomInformation>(ARG_ROM)!!

        val message = String.format(getString(R.string.switcher_ask_set_kernel_desc), info.name)

        val dialog = MaterialDialog(requireActivity())
                .title(R.string.switcher_ask_set_kernel_title)
                .message(text = message)
                .positiveButton(R.string.proceed) {
                    owner?.onConfirmSetKernel()
                }
                .negativeButton(R.string.cancel)

        isCancelable = false
        dialog.setCanceledOnTouchOutside(false)

        return dialog
    }

    companion object {
        private const val ARG_ROM = "rom"

        fun newInstanceFromFragment(parent: Fragment?, info: RomInformation):
                SetKernelConfirmDialog {
            if (parent != null) {
                if (parent !is SetKernelConfirmDialogListener) {
                    throw IllegalStateException(
                            "Parent fragment must implement SetKernelConfirmDialogListener")
                }
            }

            val frag = SetKernelConfirmDialog()
            frag.setTargetFragment(parent, 0)
            val args = Bundle()
            args.putParcelable(ARG_ROM, info)
            frag.arguments = args
            return frag
        }

        fun newInstanceFromActivity(info: RomInformation): SetKernelConfirmDialog {
            val frag = SetKernelConfirmDialog()
            val args = Bundle()
            args.putParcelable(ARG_ROM, info)
            frag.arguments = args
            return frag
        }
    }
}