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
import android.text.Editable
import android.text.InputType
import android.text.TextWatcher
import com.afollestad.materialdialogs.DialogAction
import com.afollestad.materialdialogs.MaterialDialog
import com.github.chenxiaolong.dualbootpatcher.R
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherUtils

class NamedSlotIdInputDialog : DialogFragment() {
    internal val owner: NamedSlotIdInputDialogListener?
        get() = targetFragment as NamedSlotIdInputDialogListener?

    interface NamedSlotIdInputDialogListener {
        fun onSelectedNamedSlotRomId(romId: String)
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val prefix = arguments!!.getString(ARG_PREFIX)
        val template = PatcherUtils.templateLocations.find {
            it.prefix == prefix
        } ?: throw IllegalArgumentException("Invalid prefix: $prefix")

        val builder = MaterialDialog.Builder(activity!!)
                .content(R.string.install_location_named_slot_id_hint)
                .inputType(InputType.TYPE_CLASS_TEXT or InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS)
                .input(R.string.install_location_named_slot_id_hint, 0, false,
                        { _, input ->
                            val loc = template.toInstallLocation(input.toString())
                            owner?.onSelectedNamedSlotRomId(loc.id)
                        })

        builder.title(template.getTemplateDisplayName(context!!))

        val dialog = builder.build()

        val et = dialog.inputEditText
        et?.addTextChangedListener(object : TextWatcher {
            override fun beforeTextChanged(s: CharSequence, start: Int, count: Int, after: Int) {}

            override fun onTextChanged(s: CharSequence, start: Int, before: Int, count: Int) {}

            override fun afterTextChanged(s: Editable) {
                val text = s.toString()

                if (text.isEmpty()) {
                    et.error = getString(
                            R.string.install_location_named_slot_id_error_is_empty)
                    dialog.getActionButton(DialogAction.POSITIVE).isEnabled = false
                } else if (!text.matches("[a-z0-9]+".toRegex())) {
                    et.error = getString(
                            R.string.install_location_named_slot_id_error_invalid)
                    dialog.getActionButton(DialogAction.POSITIVE).isEnabled = false
                } else {
                    dialog.getActionButton(DialogAction.POSITIVE).isEnabled = true
                }

                val loc = template.toInstallLocation(text)
                dialog.setContent(loc.getDescription(context!!))
            }
        })

        isCancelable = false
        dialog.setCanceledOnTouchOutside(false)

        return dialog
    }

    companion object {
        private const val ARG_PREFIX = "prefix"

        fun newInstance(parent: Fragment?, prefix: String): NamedSlotIdInputDialog {
            if (parent != null) {
                if (parent !is NamedSlotIdInputDialogListener) {
                    throw IllegalStateException(
                            "Parent fragment must implement NamedSlotIdInputDialogListener")
                }
            }

            val args = Bundle()
            args.putString(ARG_PREFIX, prefix)

            val frag = NamedSlotIdInputDialog()
            frag.arguments = args
            frag.setTargetFragment(parent, 0)
            return frag
        }
    }
}