/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation
import java.util.*

class RomIdSelectionDialog : DialogFragment() {
    internal val owner: RomIdSelectionDialogListener?
        get() = targetFragment as RomIdSelectionDialogListener?

    enum class RomIdType {
        BUILT_IN_ROM_ID,
        NAMED_DATA_SLOT,
        NAMED_EXTSD_SLOT
    }

    interface RomIdSelectionDialogListener {
        fun onSelectedRomId(type: RomIdType, romId: String?)
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val infos = arguments!!.getParcelableArrayList<RomInformation>(ARG_INFOS)
        val names = arrayOfNulls<String>(infos!!.size + 2)

        for (i in infos.indices) {
            names[i] = infos[i].defaultName
        }

        names[infos.size] = getString(R.string.install_location_data_slot)
        names[infos.size + 1] = getString(R.string.install_location_extsd_slot)

        val dialog = MaterialDialog.Builder(activity!!)
                .title(R.string.in_app_flashing_dialog_installation_location)
                .items(*names)
                .negativeText(R.string.cancel)
                .itemsCallbackSingleChoice(-1) { _, _, which, _ ->
                    val owner = owner
                    if (owner != null) {
                        when (which) {
                            infos.size -> owner.onSelectedRomId(RomIdType.NAMED_DATA_SLOT, null)
                            infos.size + 1 -> owner.onSelectedRomId(RomIdType.NAMED_EXTSD_SLOT, null)
                            else -> {
                                val info = infos[which]
                                owner.onSelectedRomId(RomIdType.BUILT_IN_ROM_ID, info.id)
                            }
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
        private val ARG_INFOS = "infos"

        fun newInstance(parent: Fragment?, infos: ArrayList<RomInformation>):
                RomIdSelectionDialog {
            if (parent != null) {
                if (parent !is RomIdSelectionDialogListener) {
                    throw IllegalStateException(
                            "Parent fragment must implement RomIdSelectionDialogListener")
                }
            }

            val frag = RomIdSelectionDialog()
            frag.setTargetFragment(parent, 0)
            val args = Bundle()
            args.putParcelableArrayList(ARG_INFOS, infos)
            frag.arguments = args
            return frag
        }
    }
}