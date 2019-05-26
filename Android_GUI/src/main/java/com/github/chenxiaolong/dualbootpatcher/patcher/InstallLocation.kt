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

package com.github.chenxiaolong.dualbootpatcher.patcher

import android.content.Context
import androidx.annotation.StringRes

open class InstallLocation(
        val id: String,
        @StringRes private val displayNameId: Int,
        private val displayNameArgs: Array<Any>,
        @StringRes private val descriptionId: Int,
        private val descriptionArgs: Array<Any>
) {
    constructor(
            id: String,
            @StringRes displayNameId: Int,
            @StringRes descriptionId: Int
    ) : this(id, displayNameId, emptyArray(), descriptionId, emptyArray())

    open fun getDisplayName(context: Context): String {
        return context.getString(displayNameId, *displayNameArgs)
    }

    open fun getDescription(context: Context): String {
        return context.getString(descriptionId, *descriptionArgs)
    }
}

private class GeneratedInstallLocation(
        prefix: String,
        private val suffix: String,
        @StringRes displayNameId: Int,
        displayNameArgs: Array<Any>,
        @StringRes descriptionId: Int,
        descriptionArgs: Array<Any>
) : InstallLocation(prefix + suffix, displayNameId, displayNameArgs, descriptionId,
        descriptionArgs) {
    override fun getDisplayName(context: Context): String {
        return super.getDisplayName(context)
                .replace(TemplateLocation.PLACEHOLDER_ID, id)
                .replace(TemplateLocation.PLACEHOLDER_SUFFIX, suffix)
    }

    override fun getDescription(context: Context): String {
        return super.getDescription(context)
                .replace(TemplateLocation.PLACEHOLDER_ID, id)
                .replace(TemplateLocation.PLACEHOLDER_SUFFIX, suffix)
    }
}

class TemplateLocation(
        val prefix: String,
        @StringRes private val templateDisplayNameId: Int,
        private val templateDisplayNameArgs: Array<Any>,
        @StringRes private val displayNameId: Int,
        private val displayNameArgs: Array<Any>,
        @StringRes private val descriptionId: Int,
        private val descriptionArgs: Array<Any>
) {
    companion object {
        val PLACEHOLDER_ID = "${InstallLocation::class.java.name}.placeholder_id"
        val PLACEHOLDER_SUFFIX = "${InstallLocation::class.java.name}.placeholder_suffix"
    }

    constructor(
            prefix: String,
            @StringRes templateDiaplayNameId: Int,
            @StringRes displayNameId: Int,
            @StringRes descriptionId: Int
    ) : this(prefix, templateDiaplayNameId, emptyArray(), displayNameId, emptyArray(),
            descriptionId, emptyArray())

    fun getTemplateDisplayName(context: Context): String {
        return context.getString(templateDisplayNameId, *templateDisplayNameArgs)
    }

    fun toInstallLocation(suffix: String): InstallLocation {
        return GeneratedInstallLocation(prefix, suffix, displayNameId, displayNameArgs,
                descriptionId, descriptionArgs)
    }
}