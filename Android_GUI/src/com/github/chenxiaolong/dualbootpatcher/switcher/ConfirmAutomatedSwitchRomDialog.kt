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
import android.widget.CheckBox
import android.widget.TextView

import com.afollestad.materialdialogs.MaterialDialog
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

        val builder = MaterialDialog.Builder(activity!!)
                .customView(R.layout.dialog_first_time, true)
                .title(R.string.switching_rom)
                .positiveText(R.string.proceed)
                .negativeText(R.string.cancel)
                .onPositive { dialog, _ ->
                    val cb = dialog.findViewById(R.id.checkbox) as CheckBox
                    listener?.onConfirmSwitchRom(cb.isChecked)
                }
                .onNegative { _, _ ->
                    listener?.onCancelSwitchRom()
                }

        val dialog = builder.build()

        if (dialog.customView != null) {
            val tv: TextView = dialog.customView!!.findViewById(R.id.message)
            tv.text = getString(R.string.confirm_automated_switch_rom, romId)
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