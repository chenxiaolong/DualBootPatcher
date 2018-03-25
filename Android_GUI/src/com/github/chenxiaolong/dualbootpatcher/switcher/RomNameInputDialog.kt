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
import android.view.WindowManager
import android.widget.EditText
import android.widget.TextView
import com.afollestad.materialdialogs.MaterialDialog
import com.afollestad.materialdialogs.MaterialDialog.SingleButtonCallback
import com.github.chenxiaolong.dualbootpatcher.R
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation

class RomNameInputDialog : DialogFragment() {
    internal val owner: RomNameInputDialogListener?
        get() {
            val f = targetFragment
            return if (f == null) {
                if (activity !is RomNameInputDialogListener) {
                    throw IllegalStateException(
                            "Activity must implement RomNameInputDialogListener")
                }
                activity as RomNameInputDialogListener?
            } else {
                targetFragment as RomNameInputDialogListener?
            }
        }

    interface RomNameInputDialogListener {
        fun onRomNameChanged(newName: String?)
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val info = arguments!!.getParcelable<RomInformation>(ARG_ROM)!!

        val title = String.format(getString(R.string.rename_rom_title), info.defaultName)
        val message = String.format(getString(R.string.rename_rom_desc), info.defaultName)

        val dialog = MaterialDialog.Builder(activity!!)
                .title(title)
                .customView(R.layout.dialog_textbox, true)
                .positiveText(R.string.ok)
                .negativeText(R.string.cancel)
                .onPositive(SingleButtonCallback { dialog, _ ->
                    val owner = owner ?: return@SingleButtonCallback

                    val et = dialog.findViewById(R.id.edittext) as EditText
                    val newName = et.text.toString().trim { it <= ' ' }

                    if (newName.isEmpty()) {
                        owner.onRomNameChanged(null)
                    } else {
                        owner.onRomNameChanged(newName)
                    }
                })
                .build()

        val tv = (dialog as MaterialDialog).customView!!.findViewById<TextView>(R.id.message)
        tv.text = message

        isCancelable = false
        dialog.setCanceledOnTouchOutside(false)

        // Show keyboard
        val window = dialog.window
        window!!.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE)

        return dialog
    }

    companion object {
        private const val ARG_ROM = "rom"

        fun newInstanceFromFragment(parent: Fragment?, info: RomInformation): RomNameInputDialog {
            if (parent != null) {
                if (parent !is RomNameInputDialogListener) {
                    throw IllegalStateException(
                            "Parent fragment must implement RomNameInputDialogListener")
                }
            }

            val frag = RomNameInputDialog()
            frag.setTargetFragment(parent, 0)
            val args = Bundle()
            args.putParcelable(ARG_ROM, info)
            frag.arguments = args
            return frag
        }

        fun newInstanceFromActivity(info: RomInformation): RomNameInputDialog {
            val frag = RomNameInputDialog()
            val args = Bundle()
            args.putParcelable(ARG_ROM, info)
            frag.arguments = args
            return frag
        }
    }
}
