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

package com.github.chenxiaolong.dualbootpatcher.dialogs

import android.app.Dialog
import android.os.Bundle
import android.support.v4.app.DialogFragment
import android.support.v4.app.Fragment
import android.widget.CheckBox
import android.widget.TextView
import com.afollestad.materialdialogs.MaterialDialog
import com.github.chenxiaolong.dualbootpatcher.R

class FirstUseDialog : DialogFragment() {
    internal val owner: FirstUseDialogListener?
        get() = targetFragment as FirstUseDialogListener?

    interface FirstUseDialogListener {
        fun onConfirmFirstUse(dontShowAgain: Boolean)
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val titleResId = arguments!!.getInt(ARG_TITLE_RES_ID)
        val messageResId = arguments!!.getInt(ARG_MESSAGE_RES_ID)

        val builder = MaterialDialog.Builder(activity!!)
                .customView(R.layout.dialog_first_time, true)
                .positiveText(R.string.ok)
                .onPositive { dialog, _ ->
                    val cb = dialog.findViewById(R.id.checkbox) as CheckBox

                    val owner = owner
                    owner?.onConfirmFirstUse(cb.isChecked)
                }

        if (titleResId != 0) {
            builder.title(titleResId)
        }

        val dialog = builder.build()

        if (messageResId != 0) {
            val tv = (dialog as MaterialDialog).customView!!.findViewById(R.id.message) as TextView
            tv.setText(messageResId)
        }

        isCancelable = false
        dialog.setCanceledOnTouchOutside(false)

        return dialog
    }

    companion object {
        private const val ARG_TITLE_RES_ID = "titleResId"
        private const val ARG_MESSAGE_RES_ID = "messageResId"

        fun newInstance(parent: Fragment?, titleResId: Int, messageResId: Int): FirstUseDialog {
            if (parent != null) {
                if (parent !is FirstUseDialogListener) {
                    throw IllegalStateException(
                            "Parent fragment must implement FirstUseDialogListener")
                }
            }

            val frag = FirstUseDialog()
            frag.setTargetFragment(parent, 0)
            val args = Bundle()
            args.putInt(ARG_TITLE_RES_ID, titleResId)
            args.putInt(ARG_MESSAGE_RES_ID, messageResId)
            frag.arguments = args
            return frag
        }
    }
}
