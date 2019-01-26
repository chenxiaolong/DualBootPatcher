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
import com.afollestad.materialdialogs.MaterialDialog
import com.afollestad.materialdialogs.checkbox.checkBoxPrompt
import com.afollestad.materialdialogs.checkbox.isCheckPromptChecked
import com.github.chenxiaolong.dualbootpatcher.R

class ConfirmAutomatedSwitchRomDialog : DialogFragment() {
    private var listener: ConfirmAutomatedSwitchRomDialogListener? = null

    interface ConfirmAutomatedSwitchRomDialogListener {
        fun onConfirmSwitchRom(dontShowAgain: Boolean)

        fun onCancelSwitchRom()
    }

    override fun onActivityCreated(savedInstanceState: Bundle?) {
        super.onActivityCreated(savedInstanceState)

        listener = activity as ConfirmAutomatedSwitchRomDialogListener?
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val romId = arguments!!.getString(ARG_ROM_ID)

        val dialog = MaterialDialog(requireActivity())
                .title(R.string.switching_rom)
                .message(text = getString(R.string.confirm_automated_switch_rom, romId))
                .checkBoxPrompt(R.string.do_not_show_again, onToggle = null)
                .positiveButton(R.string.proceed) { dialog ->
                    listener?.onConfirmSwitchRom(dialog.isCheckPromptChecked())
                }
                .negativeButton(R.string.cancel) {
                    listener?.onCancelSwitchRom()
                }

        isCancelable = false
        dialog.setCanceledOnTouchOutside(false)

        return dialog
    }

    companion object {
        private const val ARG_ROM_ID = "rom_id"

        fun newInstance(romId: String): ConfirmAutomatedSwitchRomDialog {
            val frag = ConfirmAutomatedSwitchRomDialog()
            val args = Bundle()
            args.putString(ARG_ROM_ID, romId)
            frag.arguments = args
            return frag
        }
    }
}