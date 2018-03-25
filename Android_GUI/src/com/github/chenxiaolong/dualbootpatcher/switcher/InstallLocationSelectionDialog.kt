/*
 * Copyright (C) 2014-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
import com.afollestad.materialdialogs.MaterialDialog
import com.github.chenxiaolong.dualbootpatcher.R
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherUtils
import java.util.ArrayList

class InstallLocationSelectionDialog : DialogFragment() {
    internal val owner: RomIdSelectionDialogListener?
        get() = targetFragment as RomIdSelectionDialogListener?

    interface RomIdSelectionDialogListener {
        fun onSelectedInstallLocation(romId: String)

        fun onSelectedTemplateLocation(prefix: String)
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val names = ArrayList<String>()

        PatcherUtils.installLocations.mapTo(names) { it.getDisplayName(context!!) }
        PatcherUtils.templateLocations.mapTo(names) { it.getTemplateDisplayName(context!!) }

        val dialog = MaterialDialog.Builder(activity!!)
                .title(R.string.in_app_flashing_dialog_installation_location)
                .items(*names.toTypedArray())
                .negativeText(R.string.cancel)
                .itemsCallbackSingleChoice(-1) { _, _, which, _ ->
                    when (which) {
                        in 0 until PatcherUtils.installLocations.size -> {
                            val loc = PatcherUtils.installLocations[which]
                            owner?.onSelectedInstallLocation(loc.id)
                        }
                        else -> {
                            val index = which - PatcherUtils.installLocations.size
                            val template = PatcherUtils.templateLocations[index]
                            owner?.onSelectedTemplateLocation(template.prefix)
                        }
                    }

                    true
                }
                .build()

        isCancelable = false
        dialog.setCanceledOnTouchOutside(false)

        return dialog
    }

    companion object {
        fun newInstance(parent: Fragment?): InstallLocationSelectionDialog {
            if (parent != null) {
                if (parent !is RomIdSelectionDialogListener) {
                    throw IllegalStateException(
                            "Parent fragment must implement RomIdSelectionDialogListener")
                }
            }

            val frag = InstallLocationSelectionDialog()
            frag.setTargetFragment(parent, 0)
            return frag
        }
    }
}