/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher.patcher

import android.app.Dialog
import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.support.v4.app.DialogFragment
import android.support.v4.app.Fragment
import android.support.v4.app.LoaderManager.LoaderCallbacks
import android.support.v4.content.AsyncTaskLoader
import android.support.v4.content.Loader
import android.support.v7.widget.AppCompatEditText
import android.support.v7.widget.AppCompatSpinner
import android.text.Editable
import android.text.TextWatcher
import android.view.KeyEvent
import android.view.View
import android.view.inputmethod.EditorInfo
import android.view.inputmethod.InputMethodManager
import android.widget.AdapterView
import android.widget.AdapterView.OnItemSelectedListener
import android.widget.ArrayAdapter
import android.widget.TextView

import com.afollestad.materialdialogs.DialogAction
import com.afollestad.materialdialogs.MaterialDialog
import com.github.chenxiaolong.dualbootpatcher.R
import com.github.chenxiaolong.dualbootpatcher.RomUtils
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbDevice.Device
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherOptionsDialog.LoaderResult
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherUtils.InstallLocation

import java.util.ArrayList
import java.util.Collections

class PatcherOptionsDialog : DialogFragment(), LoaderCallbacks<LoaderResult> {
    private var isInitial: Boolean = false

    private var preselectedDeviceId: String? = null
    private var preselectedRomId: String? = null

    private lateinit var dialog: MaterialDialog
    private lateinit var deviceSpinner: AppCompatSpinner
    private lateinit var romIdSpinner: AppCompatSpinner
    private lateinit var romIdNamedSlotId: AppCompatEditText
    private lateinit var romIdDesc: TextView
    private lateinit var dummy: View

    private lateinit var deviceAdapter: ArrayAdapter<String>
    private val devices = ArrayList<Device>()
    private val devicesNames = ArrayList<String>()
    private lateinit var romIdAdapter: ArrayAdapter<String>
    private val romIds = ArrayList<String>()
    private val installLocations = ArrayList<InstallLocation>()

    private var isNamedSlot: Boolean = false

    internal val owner: PatcherOptionsDialogListener?
        get() {
            return if (targetFragment == null) {
                if (activity !is PatcherOptionsDialogListener) {
                    throw IllegalStateException(
                            "Parent activity must implement PatcherOptionsDialogListener")
                }
                activity as PatcherOptionsDialogListener?
            } else {
                targetFragment as PatcherOptionsDialogListener?
            }
        }

    private val romId: String?
        get() {
            if (isNamedSlot) {
                if (romIdSpinner.selectedItemPosition == romIds.size - 2) {
                    return PatcherUtils.getDataSlotRomId(romIdNamedSlotId.text.toString())
                } else if (romIdSpinner.selectedItemPosition == romIds.size - 1) {
                    return PatcherUtils.getExtsdSlotRomId(romIdNamedSlotId.text.toString())
                }
            } else {
                val pos = romIdSpinner.selectedItemPosition
                if (pos >= 0) {
                    return installLocations[pos].id
                }
            }
            return null
        }

    interface PatcherOptionsDialogListener {
        fun onConfirmedOptions(id: Int, device: Device, romId: String?)
    }

    override fun onActivityCreated(savedInstanceState: Bundle?) {
        super.onActivityCreated(savedInstanceState)

        loaderManager.initLoader(0, null, this)
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        isInitial = savedInstanceState == null

        val id = arguments!!.getInt(ARG_ID)

        preselectedDeviceId = arguments!!.getString(ARG_PRESELECTED_DEVICE_ID)
        preselectedRomId = arguments!!.getString(ARG_PRESELECTED_ROM_ID)

        dialog = MaterialDialog.Builder(activity!!)
                .title(R.string.patcher_options_dialog_title)
                .customView(R.layout.dialog_patcher_opts, true)
                .positiveText(R.string.proceed)
                .negativeText(R.string.cancel)
                .onPositive { _, _ ->
                    val owner = owner
                    if (owner != null) {
                        val position = deviceSpinner.selectedItemPosition
                        val device = devices[position]
                        if (device.id == "hero2qlte") {
                            val intent = rickRollIntent
                            activity!!.startActivity(intent)
                        } else {
                            owner.onConfirmedOptions(id, device, romId)
                        }
                    }
                }
                .build()

        val view = dialog.customView!!
        deviceSpinner = view.findViewById(R.id.spinner_device)
        romIdSpinner = view.findViewById(R.id.spinner_rom_id)
        romIdNamedSlotId = view.findViewById(R.id.rom_id_named_slot_id)
        romIdDesc = view.findViewById(R.id.rom_id_desc)
        dummy = view.findViewById(R.id.customopts_dummylayout)

        // Initialize devices
        initDevices()
        // Initialize ROM IDs
        initRomIds()
        // Initialize actions
        initActions()

        isCancelable = false
        dialog.setCanceledOnTouchOutside(false)

        return dialog
    }

    private fun initDevices() {
        deviceAdapter = ArrayAdapter(activity!!,
                android.R.layout.simple_spinner_item, android.R.id.text1, devicesNames)
        deviceAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        deviceSpinner.adapter = deviceAdapter
    }

    private fun initRomIds() {
        romIdAdapter = ArrayAdapter(activity!!,
                android.R.layout.simple_spinner_item, android.R.id.text1, romIds)
        romIdAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        romIdSpinner.adapter = romIdAdapter

        romIdSpinner.onItemSelectedListener = object : OnItemSelectedListener {
            override fun onItemSelected(parent: AdapterView<*>, view: View, position: Int, id: Long) {
                dialog.getActionButton(DialogAction.POSITIVE).isEnabled = true

                isNamedSlot = romIds.size >= 2
                        && (position == romIds.size - 1 || position == romIds.size - 2)

                onRomIdSelected(position)
                if (isNamedSlot) {
                    onNamedSlotIdTextChanged(romIdNamedSlotId.text.toString())
                }
            }

            override fun onNothingSelected(parent: AdapterView<*>) {}
        }

        romIdNamedSlotId.addTextChangedListener(object : TextWatcher {
            override fun beforeTextChanged(s: CharSequence, start: Int, count: Int, after: Int) {}

            override fun onTextChanged(s: CharSequence, start: Int, before: Int, count: Int) {}

            override fun afterTextChanged(s: Editable) {
                onNamedSlotIdTextChanged(s.toString())
            }
        })
    }

    private fun initActions() {
        preventTextViewKeepFocus(romIdNamedSlotId)
    }

    /**
     * Ugly hack to prevent the text box from keeping its focus.
     */
    private fun preventTextViewKeepFocus(tv: TextView?) {
        tv!!.setOnEditorActionListener { _, actionId, event ->
            if (actionId == EditorInfo.IME_ACTION_SEARCH
                    || actionId == EditorInfo.IME_ACTION_DONE
                    || (event != null && event.action == KeyEvent.ACTION_DOWN
                            && event.keyCode == KeyEvent.KEYCODE_ENTER)) {
                tv.clearFocus()
                val imm = activity!!.getSystemService(Context.INPUT_METHOD_SERVICE) as InputMethodManager
                imm.hideSoftInputFromWindow(tv.applicationWindowToken, 0)
                dummy.requestFocus()
            }
            false
        }
    }

    /**
     * Refresh the list of supported devices from libmbpatcher.
     */
    private fun refreshDevices(devices: Array<Device>?, currentDevice: Device?) {
        this.devices.clear()
        devicesNames.clear()

        if (devices != null) {
            for (device in devices) {
                this.devices.add(device)
                devicesNames.add("${device.id} - ${device.name}")

                if (device.id == "hero2lte" || device.id == "herolte") {
                    val rrd = rickRollDevice
                    this.devices.add(rrd)
                    devicesNames.add("${rrd.id} - ${rrd.name}")
                }
            }
        }

        deviceAdapter.notifyDataSetChanged()

        if (isInitial) {
            var deviceId = preselectedDeviceId

            if (deviceId == null) {
                if (currentDevice != null) {
                    deviceId = currentDevice.id
                } else {
                    val rrd = rickRollDevice
                    val codename = RomUtils.getDeviceCodename(activity!!)
                    for (c in rrd.codenames!!) {
                        if (c == codename) {
                            deviceId = rrd.id
                            break
                        }
                    }
                }
            }
            if (deviceId != null) {
                selectDeviceId(deviceId)
            }
        }
    }

    /**
     * Refresh the list of available ROM IDs
     */
    private fun refreshRomIds(locations: Array<InstallLocation>?) {
        installLocations.clear()
        Collections.addAll(installLocations, *PatcherUtils.getInstallLocations(activity!!))
        Collections.addAll(installLocations, *locations!!)

        romIds.clear()
        installLocations.mapTo(romIds) { it.name }
        romIds.add(getString(R.string.install_location_data_slot))
        romIds.add(getString(R.string.install_location_extsd_slot))
        romIdAdapter.notifyDataSetChanged()

        // Select initial ROM ID
        if (preselectedRomId != null) {
            selectRomId(preselectedRomId!!)
        }
    }

    private fun selectDeviceId(deviceId: String) {
        for (i in devices.indices) {
            if (devices[i].id == deviceId) {
                deviceSpinner.setSelection(i)
                return
            }
        }
    }

    private fun selectRomId(romId: String) {
        for (i in installLocations.indices) {
            if (installLocations[i].id == romId) {
                romIdSpinner.setSelection(i)
                return
            }
        }
        if (PatcherUtils.isDataSlotRomId(romId)) {
            romIdSpinner.setSelection(romIds.size - 2)
            val namedId = PatcherUtils.getDataSlotIdFromRomId(romId)
            romIdNamedSlotId.setText(namedId)
            onNamedSlotIdChanged(namedId)
        } else if (PatcherUtils.isExtsdSlotRomId(romId)) {
            romIdSpinner.setSelection(romIds.size - 1)
            val namedId = PatcherUtils.getExtsdSlotIdFromRomId(romId)
            romIdNamedSlotId.setText(namedId)
            onNamedSlotIdChanged(namedId)
        }
    }

    private fun onRomIdSelected(position: Int) {
        if (isNamedSlot) {
            onNamedSlotIdChanged(romIdNamedSlotId.text.toString())
            romIdNamedSlotId.visibility = View.VISIBLE
        } else {
            romIdDesc.text = installLocations[position].description
            romIdNamedSlotId.visibility = View.GONE
        }
    }

    private fun onNamedSlotIdTextChanged(text: String) {
        if (!isNamedSlot) {
            return
        }

        if (text.isEmpty()) {
            romIdNamedSlotId.error = getString(
                    R.string.install_location_named_slot_id_error_is_empty)
            dialog.getActionButton(DialogAction.POSITIVE).isEnabled = false
        } else if (!text.matches("[a-z0-9]+".toRegex())) {
            romIdNamedSlotId.error = getString(
                    R.string.install_location_named_slot_id_error_invalid)
            dialog.getActionButton(DialogAction.POSITIVE).isEnabled = false
        } else {
            dialog.getActionButton(DialogAction.POSITIVE).isEnabled = true
        }

        onNamedSlotIdChanged(text)
    }

    private fun onNamedSlotIdChanged(text: String?) {
        when {
            text!!.isEmpty() -> romIdDesc.setText(R.string.install_location_named_slot_id_empty)
            romIdSpinner.selectedItemPosition == romIds.size - 2 -> {
                val location = PatcherUtils.getDataSlotInstallLocation(activity!!, text)
                romIdDesc.text = location.description
            }
            romIdSpinner.selectedItemPosition == romIds.size - 1 -> {
                val location = PatcherUtils.getExtsdSlotInstallLocation(activity!!, text)
                romIdDesc.text = location.description
            }
        }
    }

    override fun onCreateLoader(i: Int, bundle: Bundle?): Loader<LoaderResult> {
        return OptionsLoader(activity!!)
    }

    override fun onLoadFinished(loader: Loader<LoaderResult>, result: LoaderResult) {
        refreshDevices(result.devices, result.currentDevice)
        refreshRomIds(result.namedLocations)
    }

    override fun onLoaderReset(loader: Loader<LoaderResult>) {}

    class LoaderResult(
        internal var currentDevice: Device?,
        internal var devices: Array<Device>?,
        internal var namedLocations: Array<InstallLocation>?
    )

    private class OptionsLoader(context: Context) : AsyncTaskLoader<LoaderResult>(context) {
        private var result: LoaderResult? = null

        init {
            onContentChanged()
        }

        override fun onStartLoading() {
            if (result != null) {
                deliverResult(result)
            } else if (takeContentChanged()) {
                forceLoad()
            }
        }

        override fun loadInBackground(): LoaderResult? {
            this.result = LoaderResult(PatcherUtils.getCurrentDevice(context),
                    PatcherUtils.getDevices(context),
                    PatcherUtils.getNamedInstallLocations(context))
            return this.result
        }
    }

    companion object {
        private val ARG_ID = "id"
        private val ARG_PRESELECTED_DEVICE_ID = "preselected_device_id"
        private val ARG_PRESELECTED_ROM_ID = "rom_id"

        private var hero2qlteDevice: Device? = null

        fun newInstanceFromFragment(parent: Fragment?, id: Int,
                                    preselectedDeviceId: String?,
                                    preselectedRomId: String?): PatcherOptionsDialog {
            if (parent != null) {
                if (parent !is PatcherOptionsDialogListener) {
                    throw IllegalStateException(
                            "Parent fragment must implement PatcherOptionsDialogListener")
                }
            }

            val frag = PatcherOptionsDialog()
            frag.setTargetFragment(parent, 0)
            val args = Bundle()
            args.putInt(ARG_ID, id)
            args.putString(ARG_PRESELECTED_DEVICE_ID, preselectedDeviceId)
            args.putString(ARG_PRESELECTED_ROM_ID, preselectedRomId)
            frag.arguments = args
            return frag
        }

        fun newInstanceFromActivity(id: Int,
                                    preselectedDeviceId: String?,
                                    preselectedRomId: String?): PatcherOptionsDialog {
            val frag = PatcherOptionsDialog()
            val args = Bundle()
            args.putInt(ARG_ID, id)
            args.putString(ARG_PRESELECTED_DEVICE_ID, preselectedDeviceId)
            args.putString(ARG_PRESELECTED_ROM_ID, preselectedRomId)
            frag.arguments = args
            return frag
        }

        private val rickRollDevice: Device
            get() {
                if (hero2qlteDevice == null) {
                    hero2qlteDevice = Device()
                    hero2qlteDevice!!.id = "hero2qlte"
                    hero2qlteDevice!!.codenames = arrayOf("hero2qlte", "hero2qlteatt",
                            "hero2qltespr", "hero2qltetmo", "hero2qltevzw")
                    hero2qlteDevice!!.name = "Samsung Galaxy S 7 Edge (Qcom)"
                    hero2qlteDevice!!.architecture = "arm64-v8a"
                    hero2qlteDevice!!.systemBlockDevs = arrayOf("/dev/null")
                    hero2qlteDevice!!.cacheBlockDevs = arrayOf("/dev/null")
                    hero2qlteDevice!!.dataBlockDevs = arrayOf("/dev/null")
                    hero2qlteDevice!!.bootBlockDevs = arrayOf("/dev/null")
                    hero2qlteDevice!!.extraBlockDevs = arrayOf("/dev/null")
                }
                return hero2qlteDevice!!
            }

        private val rickRollIntent: Intent
            get() = Intent(Intent.ACTION_VIEW,
                    Uri.parse("https://www.youtube.com/watch?v=dQw4w9WgXcQ"))
    }
}
