/*
 * Copyright (C) 2015-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
import android.support.annotation.ArrayRes
import android.support.annotation.StringRes
import android.support.v4.app.DialogFragment
import android.support.v4.app.Fragment
import android.view.View

import com.afollestad.materialdialogs.DialogAction
import com.afollestad.materialdialogs.MaterialDialog
import com.afollestad.materialdialogs.MaterialDialog.ListCallbackSingleChoice
import com.afollestad.materialdialogs.MaterialDialog.SingleButtonCallback

import java.io.Serializable

class GenericSingleChoiceDialog : DialogFragment(), ListCallbackSingleChoice, SingleButtonCallback {
    private lateinit var target: DialogListenerTarget
    private lateinit var dialogTag: String

    private var selectedText: String? = null

    private val owner: GenericSingleChoiceDialogListener?
        get() {
            return when (target) {
                DialogListenerTarget.ACTIVITY -> activity as GenericSingleChoiceDialogListener?
                DialogListenerTarget.FRAGMENT -> targetFragment as GenericSingleChoiceDialogListener?
                DialogListenerTarget.NONE -> null
            }
        }

    interface GenericSingleChoiceDialogListener {
        fun onConfirmSingleChoice(tag: String, index: Int, text: String?)
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val args = arguments
        val builder = args!!.getSerializable(ARG_BUILDER) as Builder
        target = args.getSerializable(ARG_TARGET) as DialogListenerTarget
        dialogTag = args.getString(ARG_TAG)

        var selectedIndex = -1

        if (savedInstanceState != null) {
            selectedIndex = savedInstanceState.getInt(EXTRA_SELECTED_INDEX, -1)
            selectedText = savedInstanceState.getString(EXTRA_SELECTED_TEXT)
        }

        val dialogBuilder = MaterialDialog.Builder(activity!!)

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

        if (builder.choices != null) {
            dialogBuilder.items(*builder.choices!!)
        } else if (builder.choicesResId != 0) {
            dialogBuilder.items(builder.choicesResId)
        }

        dialogBuilder.itemsCallbackSingleChoice(selectedIndex, this)
        dialogBuilder.alwaysCallSingleChoiceCallback()

        dialogBuilder.onPositive(this)

        val dialog = dialogBuilder.build()

        // Nothing selected by default, so don't enable OK button
        dialog.getActionButton(DialogAction.POSITIVE).isEnabled = selectedIndex >= 0

        isCancelable = false
        dialog.setCanceledOnTouchOutside(false)

        return dialog
    }

    override fun onSaveInstanceState(outState: Bundle) {
        val dialog = dialog as MaterialDialog?
        if (dialog != null) {
            outState.putInt(EXTRA_SELECTED_INDEX, dialog.selectedIndex)
        }
        outState.putString(EXTRA_SELECTED_TEXT, selectedText)
    }

    override fun onSelection(dialog: MaterialDialog, itemView: View, which: Int,
                             text: CharSequence): Boolean {
        dialog.getActionButton(DialogAction.POSITIVE).isEnabled = true
        selectedText = text.toString()
        return true
    }

    override fun onClick(dialog: MaterialDialog, which: DialogAction) {
        owner?.onConfirmSingleChoice(dialogTag, dialog.selectedIndex, selectedText)
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
        internal var choices: Array<String>? = null
        @ArrayRes
        internal var choicesResId: Int = 0

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

        fun choices(vararg choices: String): Builder {
            this.choices = arrayOf(*choices)
            choicesResId = 0
            return this
        }

        fun choices(@ArrayRes choicesResId: Int): Builder {
            choices = null
            this.choicesResId = choicesResId
            return this
        }

        fun buildFromFragment(tag: String, parent: Fragment): GenericSingleChoiceDialog {
            val args = Bundle()
            args.putSerializable(ARG_BUILDER, this)
            args.putSerializable(ARG_TARGET, DialogListenerTarget.FRAGMENT)
            args.putString(ARG_TAG, tag)

            val dialog = GenericSingleChoiceDialog()
            dialog.setTargetFragment(parent, 0)
            dialog.arguments = args
            return dialog
        }

        fun buildFromActivity(tag: String): GenericSingleChoiceDialog {
            val args = Bundle()
            args.putSerializable(ARG_BUILDER, this)
            args.putSerializable(ARG_TARGET, DialogListenerTarget.ACTIVITY)
            args.putString(ARG_TAG, tag)

            val dialog = GenericSingleChoiceDialog()
            dialog.arguments = args
            return dialog
        }
    }

    companion object {
        private const val ARG_BUILDER = "builder"
        private const val ARG_TARGET = "target"
        private const val ARG_TAG = "tag"

        private const val EXTRA_SELECTED_INDEX = "selected_index"
        private const val EXTRA_SELECTED_TEXT = "selected_text"
    }
}
