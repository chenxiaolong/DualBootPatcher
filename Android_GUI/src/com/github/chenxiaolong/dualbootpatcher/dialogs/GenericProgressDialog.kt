/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

import com.afollestad.materialdialogs.MaterialDialog

import java.io.Serializable

class GenericProgressDialog : DialogFragment() {
    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val args = arguments
        val builder = args!!.getSerializable(ARG_BUILDER) as Builder

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

        dialogBuilder.progress(true, 0)

        val dialog = dialogBuilder.build()

        isCancelable = false
        dialog.setCanceledOnTouchOutside(false)

        return dialog
    }

    class Builder : Serializable {
        internal var title: String? = null
        @StringRes
        internal var titleResId: Int = 0
        internal var message: String? = null
        @StringRes
        internal var messageResId: Int = 0

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

        fun build(): GenericProgressDialog {
            val args = Bundle()
            args.putSerializable(ARG_BUILDER, this)

            val dialog = GenericProgressDialog()
            dialog.arguments = args
            return dialog
        }
    }

    companion object {
        private const val ARG_BUILDER = "builder"
    }
}
