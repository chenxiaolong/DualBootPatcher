/*
 * Copyright (C) 2014-2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
import androidx.annotation.StringRes
import androidx.fragment.app.DialogFragment
import androidx.fragment.app.Fragment
import com.afollestad.materialdialogs.MaterialDialog
import java.io.Serializable

class GenericConfirmDialog : DialogFragment() {
    private lateinit var target: DialogListenerTarget
    private lateinit var dialogTag: String

    private val owner: GenericConfirmDialogListener?
        get() {
            return when (target) {
                DialogListenerTarget.ACTIVITY -> activity as GenericConfirmDialogListener?
                DialogListenerTarget.FRAGMENT -> targetFragment as GenericConfirmDialogListener?
                DialogListenerTarget.NONE -> null
            }
        }

    interface GenericConfirmDialogListener {
        fun onConfirmOk(tag: String)
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val args = arguments
        val builder = args!!.getSerializable(ARG_BUILDER) as Builder
        target = args.getSerializable(ARG_TARGET) as DialogListenerTarget
        dialogTag = args.getString(ARG_TAG)!!

        val dialog = MaterialDialog(requireActivity())

        if (builder.titleResId != null || builder.title != null) {
            dialog.title(builder.titleResId, text = builder.title)
        }

        if (builder.messageResId != null || builder.message != null) {
            dialog.message(builder.messageResId, text = builder.message)
        }

        if (builder.buttonTextResId != null || builder.buttonText != null) {
            dialog.positiveButton(builder.buttonTextResId, text = builder.buttonText) {
                owner?.onConfirmOk(dialogTag)
            }
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
        internal var buttonText: String? = null
        @StringRes
        internal var buttonTextResId: Int? = null

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

        fun buttonText(text: String?): Builder {
            buttonText = text
            buttonTextResId = null
            return this
        }

        fun buttonText(@StringRes textResId: Int): Builder {
            buttonText = null
            buttonTextResId = textResId
            return this
        }

        fun build(): GenericConfirmDialog {
            val args = Bundle()
            args.putSerializable(ARG_BUILDER, this)
            args.putSerializable(ARG_TARGET, DialogListenerTarget.NONE)
            args.putString(ARG_TAG, "")

            val dialog = GenericConfirmDialog()
            dialog.arguments = args
            return dialog
        }

        fun buildFromFragment(tag: String, parent: Fragment): GenericConfirmDialog {
            val args = Bundle()
            args.putSerializable(ARG_BUILDER, this)
            args.putSerializable(ARG_TARGET, DialogListenerTarget.FRAGMENT)
            args.putString(ARG_TAG, tag)

            val dialog = GenericConfirmDialog()
            dialog.setTargetFragment(parent, 0)
            dialog.arguments = args
            return dialog
        }

        fun buildFromActivity(tag: String): GenericConfirmDialog {
            val args = Bundle()
            args.putSerializable(ARG_BUILDER, this)
            args.putSerializable(ARG_TARGET, DialogListenerTarget.ACTIVITY)
            args.putString(ARG_TAG, tag)

            val dialog = GenericConfirmDialog()
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
