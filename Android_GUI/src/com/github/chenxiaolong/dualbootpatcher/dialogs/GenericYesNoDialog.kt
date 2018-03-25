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
import android.support.annotation.StringRes
import android.support.v4.app.DialogFragment
import android.support.v4.app.Fragment

import com.afollestad.materialdialogs.DialogAction
import com.afollestad.materialdialogs.MaterialDialog
import com.afollestad.materialdialogs.MaterialDialog.SingleButtonCallback

import java.io.Serializable

class GenericYesNoDialog : DialogFragment(), SingleButtonCallback {
    private lateinit var target: DialogListenerTarget
    private lateinit var dialogTag: String

    private val owner: GenericYesNoDialogListener?
        get() {
            return when (target) {
                DialogListenerTarget.ACTIVITY -> activity as GenericYesNoDialogListener?
                DialogListenerTarget.FRAGMENT -> targetFragment as GenericYesNoDialogListener?
                DialogListenerTarget.NONE -> null
            }
        }

    interface GenericYesNoDialogListener {
        fun onConfirmYesNo(tag: String, choice: Boolean)
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val args = arguments
        val builder = args!!.getSerializable(ARG_BUILDER) as Builder
        target = args.getSerializable(ARG_TARGET) as DialogListenerTarget
        dialogTag = args.getString(ARG_TAG)

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

        dialogBuilder.onPositive(this)
        dialogBuilder.onNegative(this)

        val dialog = dialogBuilder.build()

        isCancelable = false
        dialog.setCanceledOnTouchOutside(false)

        return dialog
    }

    override fun onClick(dialog: MaterialDialog, which: DialogAction) {
        val owner = owner ?: return

        when (which) {
            DialogAction.POSITIVE -> owner.onConfirmYesNo(dialogTag, true)
            DialogAction.NEGATIVE -> owner.onConfirmYesNo(dialogTag, false)
            DialogAction.NEUTRAL -> {}
        }
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

        fun buildFromFragment(tag: String, parent: Fragment): GenericYesNoDialog {
            val args = Bundle()
            args.putSerializable(ARG_BUILDER, this)
            args.putSerializable(ARG_TARGET, DialogListenerTarget.FRAGMENT)
            args.putString(ARG_TAG, tag)

            val dialog = GenericYesNoDialog()
            dialog.setTargetFragment(parent, 0)
            dialog.arguments = args
            return dialog
        }

        fun buildFromActivity(tag: String): GenericYesNoDialog {
            val args = Bundle()
            args.putSerializable(ARG_BUILDER, this)
            args.putSerializable(ARG_TARGET, DialogListenerTarget.ACTIVITY)
            args.putString(ARG_TAG, tag)

            val dialog = GenericYesNoDialog()
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
