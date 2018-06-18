/*
 * Copyright (C) 2015-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
import android.arch.lifecycle.Observer
import android.arch.lifecycle.ViewModelProviders
import android.content.Context
import android.os.Bundle
import android.support.v4.app.DialogFragment
import android.support.v4.app.Fragment
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
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbDevice.Device

class PatcherOptionsDialog : DialogFragment() {
    private var preselectedDeviceId: String? = null
    private var preselectedRomId: String? = null

    private lateinit var deviceSpinner: AppCompatSpinner
    private lateinit var locationSpinner: AppCompatSpinner
    private lateinit var templateSuffixEditor: AppCompatEditText
    private lateinit var dummy: View

    private lateinit var deviceAdapter: ArrayAdapter<String>
    private lateinit var locationAdapter: ArrayAdapter<String>

    private lateinit var model: PatcherOptionsViewModel

    private var lastDevicePosition = AppCompatSpinner.INVALID_POSITION
    private var lastLocationPosition = AppCompatSpinner.INVALID_POSITION

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

    interface PatcherOptionsDialogListener {
        fun onConfirmedOptions(id: Int, device: Device, romId: String)
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val id = arguments!!.getInt(ARG_ID)

        preselectedDeviceId = arguments!!.getString(ARG_PRESELECTED_DEVICE_ID)
        preselectedRomId = arguments!!.getString(ARG_PRESELECTED_ROM_ID)

        val dialog = MaterialDialog.Builder(activity!!)
                .title(R.string.patcher_options_dialog_title)
                .customView(R.layout.dialog_patcher_opts, true)
                .positiveText(R.string.proceed)
                .negativeText(R.string.cancel)
                .onPositive { _, _ ->
                    owner?.onConfirmedOptions(id, model.device, model.selectedLocation.value!!.id)
                }
                .build()

        val view = dialog.customView!!
        deviceSpinner = view.findViewById(R.id.spinner_device)
        locationSpinner = view.findViewById(R.id.spinner_location)
        templateSuffixEditor = view.findViewById(R.id.template_suffix)
        val romIdDesc: TextView = view.findViewById(R.id.location_desc)
        dummy = view.findViewById(R.id.customopts_dummylayout)

        initDevices()
        initLocations()
        initActions()

        isCancelable = false
        dialog.setCanceledOnTouchOutside(false)

        model = ViewModelProviders.of(this)[PatcherOptionsViewModel::class.java]

        model.optionsData.observe(this, Observer {
            refreshDevices(it!!.devices, it.currentDevice)
            refreshRomIds(it.installLocations, it.templateLocations)

            if (lastDevicePosition != AppCompatSpinner.INVALID_POSITION) {
                deviceSpinner.setSelection(lastDevicePosition)
            }
            if (lastLocationPosition != AppCompatSpinner.INVALID_POSITION) {
                locationSpinner.setSelection(lastLocationPosition)
            }
        })

        model.selectedLocation.observe(this, Observer {
            if (it != null) {
                romIdDesc.text = it.getDescription(context!!)
            } else {
                romIdDesc.setText(R.string.install_location_named_slot_id_empty)
            }
        })

        model.validationState.observe(this, Observer {
            val dialogButton = dialog.getActionButton(DialogAction.POSITIVE)

            when (it!!) {
                PatcherOptionsValidationState.VALID -> {
                    dialogButton.isEnabled = true
                }
                PatcherOptionsValidationState.INVALID -> {
                    dialogButton.isEnabled = false
                    templateSuffixEditor.error = getString(
                            R.string.install_location_named_slot_id_error_invalid)
                }
                PatcherOptionsValidationState.EMPTY -> {
                    dialogButton.isEnabled = false
                    templateSuffixEditor.error = getString(
                            R.string.install_location_named_slot_id_error_is_empty)
                }
            }
        })

        model.suffixEditorVisible.observe(this, Observer {
            templateSuffixEditor.visibility = if (it!!) View.VISIBLE else View.GONE
        })

        model.deviceSelectionEvent.observe(this, Observer {
            deviceSpinner.setSelection(it!!)
        })

        model.locationSelectionEvent.observe(this, Observer {
            locationSpinner.setSelection(it!!)
        })

        model.suffixSetEvent.observe(this, Observer {
            templateSuffixEditor.setText(it)
        })

        if (savedInstanceState != null) {
            lastDevicePosition = savedInstanceState.getInt(STATE_DEVICE_POSITION,
                    AppCompatSpinner.INVALID_POSITION)
            lastLocationPosition = savedInstanceState.getInt(STATE_LOCATION_POSITION,
                    AppCompatSpinner.INVALID_POSITION)
        }

        return dialog
    }

    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
        outState.putInt(STATE_DEVICE_POSITION, deviceSpinner.selectedItemPosition)
        outState.putInt(STATE_LOCATION_POSITION, locationSpinner.selectedItemPosition)
    }

    private fun initDevices() {
        deviceAdapter = ArrayAdapter(activity!!,
                android.R.layout.simple_spinner_item, android.R.id.text1)
        deviceAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)

        deviceSpinner.adapter = deviceAdapter
        deviceSpinner.onItemSelectedListener = object : OnItemSelectedListener {
            override fun onItemSelected(parent: AdapterView<*>, view: View, position: Int,
                                        id: Long) {
                model.onSelectedDevice(position)
            }

            override fun onNothingSelected(parent: AdapterView<*>?) {}
        }
    }

    private fun initLocations() {
        locationAdapter = ArrayAdapter(activity!!,
                android.R.layout.simple_spinner_item, android.R.id.text1)
        locationAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)

        locationSpinner.adapter = locationAdapter
        locationSpinner.onItemSelectedListener = object : OnItemSelectedListener {
            override fun onItemSelected(parent: AdapterView<*>, view: View, position: Int,
                                        id: Long) {
                model.onSelectedLocationItem(position)
            }

            override fun onNothingSelected(parent: AdapterView<*>) {}
        }

        templateSuffixEditor.addTextChangedListener(object : TextWatcher {
            override fun beforeTextChanged(s: CharSequence, start: Int, count: Int, after: Int) {}

            override fun onTextChanged(s: CharSequence, start: Int, before: Int, count: Int) {}

            override fun afterTextChanged(s: Editable) {
                model.onTemplateSuffixChanged(s.toString())
            }
        })
    }

    private fun initActions() {
        preventTextViewKeepFocus(templateSuffixEditor)
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
    private fun refreshDevices(devices: List<Device>, currentDevice: Device?) {
        deviceAdapter.setNotifyOnChange(false)
        deviceAdapter.clear()
        deviceAdapter.addAll(devices.map { "${it.id} - ${it.name}" })
        deviceAdapter.notifyDataSetChanged()

        // Select initial device
        if (lastDevicePosition == AppCompatSpinner.INVALID_POSITION) {
            (preselectedDeviceId ?: currentDevice?.id)?.let { model.selectDeviceId(it) }
        }
    }

    /**
     * Refresh the list of available ROM IDs
     */
    private fun refreshRomIds(installLocations: List<InstallLocation>,
                              templateLocations: List<TemplateLocation>) {
        locationAdapter.setNotifyOnChange(false)
        locationAdapter.clear()
        locationAdapter.addAll(installLocations.map { it.getDisplayName(context!!) })
        locationAdapter.addAll(templateLocations.map { it.getTemplateDisplayName(context!!) })
        locationAdapter.notifyDataSetChanged()

        // Select initial ROM ID
        if (lastLocationPosition == AppCompatSpinner.INVALID_POSITION) {
            preselectedRomId?.let { model.selectRomId(it) }
        }
    }

    companion object {
        private const val ARG_ID = "id"
        private const val ARG_PRESELECTED_DEVICE_ID = "preselected_device_id"
        private const val ARG_PRESELECTED_ROM_ID = "rom_id"

        private val STATE_DEVICE_POSITION =
                "${PatcherOptionsDialog::class.java.canonicalName}.state.device_position"
        private val STATE_LOCATION_POSITION =
                "${PatcherOptionsDialog::class.java.canonicalName}.state.location_position"

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
    }
}
