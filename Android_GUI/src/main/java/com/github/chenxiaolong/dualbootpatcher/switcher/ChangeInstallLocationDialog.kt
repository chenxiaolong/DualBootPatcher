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

class ChangeInstallLocationDialog : DialogFragment() {
    internal val owner: ChangeInstallLocationDialogListener?
        get() = targetFragment as ChangeInstallLocationDialogListener?

    interface ChangeInstallLocationDialogListener {
        fun onChangeInstallLocationClicked(changeInstallLocation: Boolean)
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val zipRomId = arguments!!.getString(ARG_ZIP_ROM_ID)
        val message = String.format(
                getString(R.string.in_app_flashing_change_install_location_desc), zipRomId)

        val dialog = MaterialDialog(requireActivity())
                .message(text = message)
                .positiveButton(R.string.in_app_flashing_change_install_location) {
                    owner?.onChangeInstallLocationClicked(true)
                }
                .negativeButton(R.string.in_app_flashing_keep_current_location) {
                    owner?.onChangeInstallLocationClicked(false)
                }

        isCancelable = false
        dialog.setCanceledOnTouchOutside(false)

        return dialog
    }

    companion object {
        private const val ARG_ZIP_ROM_ID = "zip_rom_id"

        fun newInstance(parent: Fragment?, zipRomId: String): ChangeInstallLocationDialog {
            if (parent != null) {
                if (parent !is ChangeInstallLocationDialogListener) {
                    throw IllegalStateException(
                            "Parent fragment must implement ChangeInstallLocationDialogListener")
                }
            }

            val frag = ChangeInstallLocationDialog()
            frag.setTargetFragment(parent, 0)
            val args = Bundle()
            args.putString(ARG_ZIP_ROM_ID, zipRomId)
            frag.arguments = args
            return frag
        }
    }
}
