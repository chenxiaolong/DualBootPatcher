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

package com.github.chenxiaolong.dualbootpatcher.patcher

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import com.github.chenxiaolong.dualbootpatcher.OneTimeLiveEvent
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbDevice.Device

internal class PatcherOptionsViewModel(application: Application) : AndroidViewModel(application) {
    /**
     * Asynchronously loaded data containing the available patcher options.
     */
    val optionsData: PatcherOptionsLiveData = PatcherOptionsLiveData(application)

    /**
     * Backing field for [selectedLocation].
     */
    private val _selectedLocation = MutableLiveData<InstallLocation?>()

    /**
     * Currently selected fixed or generated install location.
     *
     * NOTE: This is not necessarily valid. Make sure [validationState] is
     * [PatcherOptionsValidationState.VALID] before using this value.
     */
    val selectedLocation: LiveData<InstallLocation?>
        get() = _selectedLocation

    /**
     * Backing field for [validationState].
     */
    private val _validationState = MutableLiveData<PatcherOptionsValidationState>()

    /**
     * Whether the current set of options is valid.
     */
    val validationState: LiveData<PatcherOptionsValidationState>
        get() = _validationState

    /**
     * Backing field for [suffixEditorVisible].
     */
    private val _suffixEditorVisible = MutableLiveData<Boolean>()

    /**
     * Whether the view for editing the template suffix should be visible.
     */
    val suffixEditorVisible: LiveData<Boolean>
        get() = _suffixEditorVisible

    /**
     * Backing field for [deviceSelectionEvent].
     */
    private val _deviceSelectionEvent = OneTimeLiveEvent<Int>()

    /**
     * One-off event for selecting a device.
     *
     * Set by [selectDeviceId].
     */
    val deviceSelectionEvent: LiveData<Int>
        get() = _deviceSelectionEvent

    /**
     * Backing field for [locationSelectionEvent].
     */
    private val _locationSelectionEvent = OneTimeLiveEvent<Int>()

    /**
     * One-off event for selecting an installation location.
     *
     * Set by [selectRomId].
     */
    val locationSelectionEvent: LiveData<Int>
        get() = _locationSelectionEvent

    /**
     * Backing field for [suffixSetEvent].
     */
    private val _suffixSetEvent = OneTimeLiveEvent<String>()

    /**
     * One-off event for setting a template suffix.
     */
    val suffixSetEvent: LiveData<String>
        get() = _suffixSetEvent

    /**
     * Currently selected device.
     */
    lateinit var device: Device

    /**
     * Currently selected template location.
     */
    private var templateLocation: TemplateLocation? = null

    /**
     * Currently entered template location suffix.
     */
    private var templateSuffix: String = ""

    /**
     * Called when a device is selected.
     */
    fun onSelectedDevice(position: Int) {
        val data = optionsData.value!!
        device = data.devices[position]
    }

    /**
     * Called when a installation location or template location is selected.
     *
     * NOTE: It is possible for this function to be called for a template location selection after
     * [onTemplateSuffixChanged] had already been called.
     */
    fun onSelectedLocationItem(position: Int) {
        val data = optionsData.value!!

        if (position >= data.installLocations.size) {
            // Template location
            templateLocation = data.templateLocations[position - data.installLocations.size]
            _suffixEditorVisible.value = true
            setSelectedLocationFromSuffix()
        } else {
            // Fixed install location
            templateLocation = null
            _suffixEditorVisible.value = false
            _selectedLocation.value = data.installLocations[position]
        }

        validateState()
    }

    /**
     * Called when the entered template suffix is changed.
     *
     * NOTE: It is possible for this function to be called before a template location is selected
     * via [onSelectedLocationItem].
     */
    fun onTemplateSuffixChanged(suffix: String) {
        templateSuffix = suffix
        setSelectedLocationFromSuffix()

        validateState()
    }

    /**
     * Update the UI to select the specified device.
     *
     * If the device does not match anything, the UI is left unchanged.
     */
    fun selectDeviceId(deviceId: String) {
        val data = optionsData.value!!

        for ((i, device) in data.devices.withIndex()) {
            if (device.id == deviceId) {
                _deviceSelectionEvent.value = i
                return
            }
        }
    }

    /**
     * Update the UI to select the installation location matching the ROM ID.
     *
     * If the ROM ID does not match anything, the UI is left unchanged.
     */
    fun selectRomId(romId: String) {
        val data = optionsData.value!!

        for ((i, loc) in data.installLocations.withIndex()) {
            if (loc.id == romId) {
                _locationSelectionEvent.value = i
                return
            }
        }
        for ((i, template) in data.templateLocations.withIndex()) {
            if (romId.startsWith(template.prefix)) {
                val suffix = romId.substring(template.prefix.length)
                _locationSelectionEvent.value = data.installLocations.size + i
                _suffixSetEvent.value = suffix
                return
            }
        }
    }

    /**
     * Set currently selected location based on the chosen template suffix.
     *
     * @return True if [_selectedLocation] was updated. False if [templateLocation] has not been set
     *         yet.
     */
    private fun setSelectedLocationFromSuffix(): Boolean {
        val loc = templateLocation ?: return false

        _selectedLocation.value = if (templateSuffix.isEmpty()) {
            null
        } else {
            loc.toInstallLocation(templateSuffix)
        }

        return true
    }

    /**
     * Validate that the options are in a valid state.
     *
     * If the currently selected location is a template location, the suffix will also be validated.
     */
    private fun validateState() {
        var state = PatcherOptionsValidationState.VALID

        if (templateLocation != null) {
            if (templateSuffix.isEmpty()) {
                state = PatcherOptionsValidationState.EMPTY
            } else if (!isValidTemplateSuffix(templateSuffix)) {
                state = PatcherOptionsValidationState.INVALID
            }
        }

        _validationState.value = state
    }

    companion object {
        private val SUFFIX_REGEX = "[a-z0-9]+".toRegex()

        private fun isValidTemplateSuffix(suffix: String): Boolean {
            return suffix.matches(SUFFIX_REGEX)
        }
    }
}