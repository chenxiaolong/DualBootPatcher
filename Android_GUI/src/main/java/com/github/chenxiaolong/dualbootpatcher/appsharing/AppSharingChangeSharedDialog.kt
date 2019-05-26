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

package com.github.chenxiaolong.dualbootpatcher.appsharing

import android.app.Dialog
import android.os.Bundle
import androidx.fragment.app.DialogFragment
import androidx.fragment.app.Fragment
import com.afollestad.materialdialogs.MaterialDialog
import com.afollestad.materialdialogs.list.listItemsMultiChoice
import com.github.chenxiaolong.dualbootpatcher.R
import java.util.*

class AppSharingChangeSharedDialog : DialogFragment() {
    internal val owner: AppSharingChangeSharedDialogListener?
        get() = targetFragment as AppSharingChangeSharedDialogListener?

    interface AppSharingChangeSharedDialogListener {
        fun onChangedShared(pkg: String, shareData: Boolean)
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val args = arguments!!
        val pkg = args.getString(ARG_PKG)!!
        val name = args.getString(ARG_NAME)!!
        val shareData = args.getBoolean(ARG_SHARE_DATA)
        val romsUsingSharedData = args.getStringArrayList(ARG_ROMS_USING_SHARED_DATA)

        val shareDataItem = getString(R.string.indiv_app_sharing_dialog_share_data)

        val items = listOf(shareDataItem)
        val message = StringBuilder()

        if (!romsUsingSharedData!!.isEmpty()) {
            val fmt = getString(R.string.indiv_app_sharing_other_roms_using_shared_data)
            val roms = romsUsingSharedData.joinToString(", ")
            message.append(String.format(fmt, roms))
            message.append("\n\n")
        }

        message.append(getString(R.string.indiv_app_sharing_dialog_default_message))

        val selected = if (shareData) intArrayOf(0) else intArrayOf()

        val dialog = MaterialDialog(requireActivity())
                .title(text = name)
                .message(text = message)
                .negativeButton(R.string.cancel)
                .positiveButton(R.string.ok)
                .listItemsMultiChoice(items = items, initialSelection = selected,
                        allowEmptySelection = true) { _, indices, _ ->
                    val shouldShareData = indices.contains(0)

                    val owner = owner
                    owner?.onChangedShared(pkg, shouldShareData)
                }

        isCancelable = false
        dialog.setCanceledOnTouchOutside(false)

        return dialog
    }

    companion object {
        private const val ARG_PKG = "pkg"
        private const val ARG_NAME = "name"
        private const val ARG_SHARE_DATA = "share_data"
        private const val ARG_IS_SYSTEM_APP = "is_system_app"
        private const val ARG_ROMS_USING_SHARED_DATA = "roms_using_shared_data"

        fun newInstance(parent: Fragment?, pkg: String, name: String,
                        shareData: Boolean, isSystemApp: Boolean,
                        romsUsingSharedData: ArrayList<String>): AppSharingChangeSharedDialog {
            if (parent != null) {
                if (parent !is AppSharingChangeSharedDialogListener) {
                    throw IllegalStateException(
                            "Parent fragment must implement AppSharingChangeSharedDialogListener")
                }
            }

            val frag = AppSharingChangeSharedDialog()
            frag.setTargetFragment(parent, 0)
            val args = Bundle()
            args.putString(ARG_PKG, pkg)
            args.putString(ARG_NAME, name)
            args.putBoolean(ARG_SHARE_DATA, shareData)
            args.putBoolean(ARG_IS_SYSTEM_APP, isSystemApp)
            args.putStringArrayList(ARG_ROMS_USING_SHARED_DATA, romsUsingSharedData)
            frag.arguments = args
            return frag
        }
    }
}
