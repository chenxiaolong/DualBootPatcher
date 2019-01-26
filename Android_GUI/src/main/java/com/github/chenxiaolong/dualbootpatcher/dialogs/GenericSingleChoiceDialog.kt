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

package com.github.chenxiaolong.dualbootpatcher.dialogs

import android.app.Dialog
import android.os.Bundle
import androidx.annotation.ArrayRes
import androidx.annotation.StringRes
import androidx.fragment.app.DialogFragment
import androidx.fragment.app.Fragment
import com.afollestad.materialdialogs.MaterialDialog
import com.afollestad.materialdialogs.WhichButton
import com.afollestad.materialdialogs.actions.setActionButtonEnabled
import com.afollestad.materialdialogs.list.listItemsSingleChoice
import java.io.Serializable

class GenericSingleChoiceDialog : DialogFragment() {
    private lateinit var target: DialogListenerTarget
    private lateinit var dialogTag: String

    private var selectedIndex: Int = -1
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
        dialogTag = args.getString(ARG_TAG)!!

        var selectedIndex = -1

        if (savedInstanceState != null) {
            selectedIndex = savedInstanceState.getInt(EXTRA_SELECTED_INDEX, -1)
            selectedText = savedInstanceState.getString(EXTRA_SELECTED_TEXT)
        }

        val dialog = MaterialDialog(requireActivity())

        if (builder.titleResId != null || builder.title != null) {
            dialog.title(builder.titleResId, text = builder.title)
        }

        if (builder.messageResId != null || builder.message != null) {
            dialog.message(builder.messageResId, text = builder.message)
        }

        if (builder.positiveResId != null || builder.positive != null) {
            dialog.positiveButton(builder.positiveResId, text = builder.positive) {
                owner?.onConfirmSingleChoice(dialogTag, selectedIndex, selectedText)
            }
        }

        if (builder.negativeResId != null || builder.negative != null) {
            dialog.negativeButton(builder.negativeResId, text = builder.negative)
        }

        if (builder.choicesResId != null || builder.choices != null) {
            @Suppress("CheckResult")
            dialog.listItemsSingleChoice(builder.choicesResId, items = builder.choices,
                    waitForPositiveButton = false) { _, index, text ->
                dialog.setActionButtonEnabled(WhichButton.POSITIVE, true)
                selectedIndex = index
                selectedText = text
            }
        }

        // Nothing selected by default, so don't enable OK button
        dialog.setActionButtonEnabled(WhichButton.POSITIVE, selectedIndex >= 0)

        isCancelable = false
        dialog.setCanceledOnTouchOutside(false)

        return dialog
    }

    override fun onSaveInstanceState(outState: Bundle) {
        outState.putInt(EXTRA_SELECTED_INDEX, selectedIndex)
        outState.putString(EXTRA_SELECTED_TEXT, selectedText)
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
        internal var choices: List<String>? = null
        @ArrayRes
        internal var choicesResId: Int? = null

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

        fun choices(vararg choices: String): Builder {
            this.choices = listOf(*choices)
            choicesResId = null
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
