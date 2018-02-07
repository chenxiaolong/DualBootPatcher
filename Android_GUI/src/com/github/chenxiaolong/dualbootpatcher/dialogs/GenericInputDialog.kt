/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
import android.support.annotation.StringRes
import android.support.v4.app.DialogFragment
import android.support.v4.app.Fragment

import com.afollestad.materialdialogs.MaterialDialog

import java.io.Serializable

class GenericInputDialog : DialogFragment(), MaterialDialog.InputCallback {
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
        dialogTag = args.getString(ARG_TAG)

        val dialogBuilder = MaterialDialog.Builder(activity!!)

        var hint: CharSequence? = null
        var prefill: CharSequence? = null

        if (builder.title != null) {
            dialogBuilder.title(builder.title!!)
        } else if (builder.titleResId != 0) {
            dialogBuilder.title(builder.titleResId)
        }

        if (builder.message != null) {
            dialogBuilder.content(builder.message!!)
        } else if (builder.messageResId != 0) {
            dialogBuilder.content(builder.messageResId)
        }

        if (builder.positive != null) {
            dialogBuilder.positiveText(builder.positive!!)
        } else if (builder.positiveResId != 0) {
            dialogBuilder.positiveText(builder.positiveResId)
        }

        if (builder.negative != null) {
            dialogBuilder.negativeText(builder.negative!!)
        } else if (builder.negativeResId != 0) {
            dialogBuilder.negativeText(builder.negativeResId)
        }

        if (builder.hint != null) {
            hint = builder.hint
        } else if (builder.hintResId != 0) {
            hint = getText(builder.hintResId)
        }

        if (builder.preFill != null) {
            prefill = builder.preFill
        } else if (builder.preFillResId != 0) {
            prefill = getText(builder.preFillResId)
        }

        if (builder.inputType != 0) {
            dialogBuilder.inputType(builder.inputType)
        }

        dialogBuilder.input(hint, prefill, builder.allowEmpty, this)

        val dialog = dialogBuilder.build()

        isCancelable = false
        dialog.setCanceledOnTouchOutside(false)

        return dialog
    }

    override fun onInput(dialog: MaterialDialog, input: CharSequence) {
        owner?.onConfirmInput(dialogTag, input.toString())
    }

    class Builder : Serializable {
        internal var title: String? = null
        @StringRes
        internal var titleResId: Int = 0
        internal var message: String? = null
        @StringRes
        internal var messageResId: Int = 0
        internal var positive: String? = null
        @StringRes
        internal var positiveResId: Int = 0
        internal var negative: String? = null
        @StringRes
        internal var negativeResId: Int = 0
        internal var hint: String? = null
        @StringRes
        internal var hintResId: Int = 0
        internal var preFill: String? = null
        @StringRes
        internal var preFillResId: Int = 0
        internal var allowEmpty = true
        internal var inputType: Int = 0

        fun title(title: String?): Builder {
            this.title = title
            titleResId = 0
            return this
        }

        fun title(@StringRes titleResId: Int): Builder {
            title = null
            this.titleResId = titleResId
            return this
        }

        fun message(message: String?): Builder {
            this.message = message
            messageResId = 0
            return this
        }

        fun message(@StringRes messageResId: Int): Builder {
            message = null
            this.messageResId = messageResId
            return this
        }

        fun positive(text: String?): Builder {
            positive = text
            positiveResId = 0
            return this
        }

        fun positive(@StringRes textResId: Int): Builder {
            positive = null
            positiveResId = textResId
            return this
        }

        fun negative(text: String?): Builder {
            negative = text
            negativeResId = 0
            return this
        }

        fun negative(@StringRes textResId: Int): Builder {
            negative = null
            negativeResId = textResId
            return this
        }

        fun hint(text: String?): Builder {
            hint = text
            hintResId = 0
            return this
        }

        fun hint(@StringRes textResId: Int): Builder {
            hint = null
            hintResId = textResId
            return this
        }

        fun prefill(text: String?): Builder {
            preFill = text
            preFillResId = 0
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