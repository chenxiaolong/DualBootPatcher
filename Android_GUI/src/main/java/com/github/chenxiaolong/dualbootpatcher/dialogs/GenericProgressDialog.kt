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
import android.widget.TextView
import androidx.annotation.StringRes
import androidx.fragment.app.DialogFragment

import com.afollestad.materialdialogs.MaterialDialog
import com.afollestad.materialdialogs.customview.customView
import com.afollestad.materialdialogs.customview.getCustomView
import com.github.chenxiaolong.dualbootpatcher.R

import java.io.Serializable

class GenericProgressDialog : DialogFragment() {
    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val args = arguments
        val builder = args!!.getSerializable(ARG_BUILDER) as Builder

        val dialog = MaterialDialog(requireActivity())
                .customView(R.layout.dialog_progress)

        if (builder.titleResId != null || builder.title != null) {
            dialog.title(builder.titleResId, text = builder.title)
        }

        val tv = dialog.getCustomView().findViewById<TextView>(R.id.message)

        if (builder.messageResId != null) {
            tv.setText(builder.messageResId!!)
        } else if (builder.message != null) {
            tv.text = builder.message
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
