/*
 * Copyright (C) 2016-2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
import android.text.InputType
import androidx.annotation.StringRes
import androidx.fragment.app.DialogFragment
import androidx.fragment.app.Fragment
import com.afollestad.materialdialogs.MaterialDialog
import com.afollestad.materialdialogs.input.input
import java.io.Serializable

class GenericInputDialog : DialogFragment() {
    private lateinit var target: DialogListenerTarget
    private lateinit var dialogTag: String

    private val owner: GenericInputDialogListener?
        get() {
            return when (target) {
                DialogListenerTarget.ACTIVITY -> activity as GenericInputDialogListener?
                DialogListenerTarget.FRAGMENT -> targetFragment as GenericInputDialogListener?
                DialogListenerTarget.NONE -> null
            }
        }

    interface GenericInputDialogListener {
        fun onConfirmInput(tag: String, text: String)
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val args = arguments
        val builder = args!!.getSerializable(ARG_BUILDER) as Builder
        target = args.getSerializable(ARG_TARGET) as DialogListenerTarget
        dialogTag = args.getString(ARG_TAG)!!

        val dialog = MaterialDialog(requireActivity())
                .input(hint = builder.hint, hintRes = builder.hintResId,
                        prefill = builder.preFill, prefillRes = builder.preFillResId,
                        inputType = builder.inputType, allowEmpty = builder.allowEmpty) { _, text ->
                    owner?.onConfirmInput(dialogTag, text.toString())
                }

        if (builder.titleResId != null || builder.title != null) {
            dialog.title(builder.titleResId, text = builder.title)
        }

        if (builder.messageResId != null || builder.message != null) {
            dialog.message(builder.messageResId, text = builder.message)
        }

        if (builder.positiveResId != null || builder.positive != null) {
            dialog.positiveButton(builder.positiveResId, text = builder.positive)
        }

        if (builder.negativeResId != null || builder.negative != null) {
            dialog.negativeButton(builder.negativeResId, text = builder.negative)
        }

        isCancelable = false
        dialog.setCanceledOnTouchOutside(false)

        return dialog
    }

    class Builder : Serializable {
        internal var title: String? = null
        @StringRes
        internal var titleResId: Int? = null
        internal var message: String? = null
        @StringRes
        internal var messageResId: Int? = null
        internal var positive: String? = null
        @StringRes
        internal var positiveResId: Int? = null
        internal var negative: String? = null
        @StringRes
        internal var negativeResId: Int? = null
        internal var hint: String? = null
        @StringRes
        internal var hintResId: Int? = null
        internal var preFill: String? = null
        @StringRes
        internal var preFillResId: Int? = null
        internal var allowEmpty = true
        internal var inputType: Int = InputType.TYPE_CLASS_TEXT

        fun title(title: String?): Builder {
            this.title = title
            titleResId = null
            return this
        }

        fun title(@StringRes titleResId: Int): Builder {
            title = null
            this.titleResId = titleResId
            return this
        }

        fun message(message: String?): Builder {
            this.message = message
            messageResId = null
            return this
        }

        fun message(@StringRes messageResId: Int): Builder {
            message = null
            this.messageResId = messageResId
            return this
        }

        fun positive(text: String?): Builder {
            positive = text
            positiveResId = null
            return this
        }

        fun positive(@StringRes textResId: Int): Builder {
            positive = null
            positiveResId = textResId
            return this
        }

        fun negative(text: String?): Builder {
            negative = text
            negativeResId = null
            return this
        }

        fun negative(@StringRes textResId: Int): Builder {
            negative = null
            negativeResId = textResId
            return this
        }

        fun hint(text: String?): Builder {
            hint = text
            hintResId = null
            return this
        }

        fun hint(@StringRes textResId: Int): Builder {
            hint = null
            hintResId = textResId
            return this
        }

        fun prefill(text: String?): Builder {
            preFill = text
            preFillResId = null
            return this
        }

        fun prefill(@StringRes textResId: Int): Builder {
            preFill = null
            preFillResId = textResId
            return this
        }

        fun allowEmpty(allowEmpty: Boolean): Builder {
            this.allowEmpty = allowEmpty
            return this
        }

        fun inputType(type: Int): Builder {
            inputType = type
            return this
        }

        fun buildFromFragment(tag: String, parent: Fragment): GenericInputDialog {
            val args = Bundle()
            args.putSerializable(ARG_BUILDER, this)
            args.putSerializable(ARG_TARGET, DialogListenerTarget.FRAGMENT)
            args.putString(ARG_TAG, tag)

            val dialog = GenericInputDialog()
            dialog.setTargetFragment(parent, 0)
            dialog.arguments = args
            return dialog
        }

        fun buildFromActivity(tag: String): GenericInputDialog {
            val args = Bundle()
            args.putSerializable(ARG_BUILDER, this)
            args.putSerializable(ARG_TARGET, DialogListenerTarget.ACTIVITY)
            args.putString(ARG_TAG, tag)

            val dialog = GenericInputDialog()
            dialog.arguments = args
            return dialog
        }
    }

    companion object {
        private const val ARG_BUILDER = "builder"
        private const val ARG_TARGET = "target"
        private const val ARG_TAG = "tag"
    }
}