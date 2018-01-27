/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation

class AddToHomeScreenOptionsDialog : DialogFragment() {
    internal val owner: AddToHomeScreenOptionsDialogListener?
        get() {
            val f = targetFragment
            return if (f == null) {
                if (activity !is AddToHomeScreenOptionsDialogListener) {
                    throw IllegalStateException(
                            "Parent activity must implement AddToHomeScreenOptionsDialogListener")
                }
                activity as AddToHomeScreenOptionsDialogListener?
            } else {
                targetFragment as AddToHomeScreenOptionsDialogListener?
            }
        }

    interface AddToHomeScreenOptionsDialogListener {
        fun onConfirmAddToHomeScreenOptions(info: RomInformation?, reboot: Boolean)
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val info = arguments!!.getParcelable<RomInformation>(ARG_ROM_INFO)

        val dialog = MaterialDialog.Builder(activity!!)
                .content(R.string.auto_reboot_after_switching)
                .negativeText(R.string.no)
                .positiveText(R.string.yes)
                .onPositive { _, _ ->
                    owner?.onConfirmAddToHomeScreenOptions(info, true)
                }
                .onNegative { _, _ ->
                    owner?.onConfirmAddToHomeScreenOptions(info, false)
                }
                .build()

        isCancelable = false
        dialog.setCanceledOnTouchOutside(false)

        return dialog
    }

    companion object {
        private const val ARG_ROM_INFO = "rom_info"

        fun newInstanceFromFragment(parent: Fragment?, info: RomInformation):
                AddToHomeScreenOptionsDialog {
            if (parent != null) {
                if (parent !is AddToHomeScreenOptionsDialogListener) {
                    throw IllegalStateException(
                            "Parent fragment must implement AddToHomeScreenOptionsDialogListener")
                }
            }

            val frag = AddToHomeScreenOptionsDialog()
            frag.setTargetFragment(parent, 0)
            val args = Bundle()
            args.putParcelable(ARG_ROM_INFO, info)
            frag.arguments = args
            return frag
        }

        fun newInstanceFromActivity(info: RomInformation): AddToHomeScreenOptionsDialog {
            val frag = AddToHomeScreenOptionsDialog()
            val args = Bundle()
            args.putParcelable(ARG_ROM_INFO, info)
            frag.arguments = args
            return frag
        }
    }
}
