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

package com.github.chenxiaolong.dualbootpatcher

import java.util.regex.Pattern

class Version @Throws(Version.VersionParseException::class)
constructor(versionString: String) : Comparable<Version> {
    private var majorVer: Int = 0
    private var minorVer: Int = 0
    private var patchVer: Int = 0
    private var suffix: String? = null
    private var revision: Int = 0
    private var gitCommit: String? = null

    init {
        parseVersion(versionString)
    }

    @Throws(Version.VersionParseException::class)
    private fun parseVersion(versionString: String) {
        val p = Pattern.compile("(\\d+)\\.(\\d+)\\.(\\d+)(.*?)?(?:-.+)?")
        val m = p.matcher(versionString)

        if (!m.matches()) {
            throw VersionParseException("Invalid version number: $versionString")
        }

        majorVer = Integer.parseInt(m.group(1))
        minorVer = Integer.parseInt(m.group(2))
        patchVer = Integer.parseInt(m.group(3))
        val suffix = m.group(4)
        if (suffix != null && !suffix.isEmpty()) {
            this.suffix = suffix
        }

        if (this.suffix != null) {
            parseRevision()
        }
    }

    @Throws(Version.VersionParseException::class)
    private fun parseRevision() {
        val p = Pattern.compile("\\.r(\\d+)(?:\\.g(.+))?")
        val m = p.matcher(suffix!!)

        if (!m.matches()) {
            throw VersionParseException("Invalid version suffix")
        }

        revision = Integer.parseInt(m.group(1))
        gitCommit = m.group(2)
    }

    override fun compareTo(other: Version): Int {
        if (majorVer < other.majorVer) {
            return -1
        } else if (majorVer > other.majorVer) {
            return 1
        }

        if (minorVer < other.minorVer) {
            return -1
        } else if (minorVer > other.minorVer) {
            return 1
        }

        if (patchVer < other.patchVer) {
            return -1
        } else if (patchVer > other.patchVer) {
            return 1
        }

        // No suffix is older (suffix is n revisions ahead of the last git tag)
        if (suffix != null && other.suffix == null) {
            return 1
        } else if (suffix == null && other.suffix != null) {
            return -1
        }

        if (revision < other.revision) {
            return -1
        } else if (revision > other.revision) {
            return 1
        }

        return 0
    }

    override fun equals(other: Any?): Boolean {
        if (this === other) {
            return true
        }

        if (other !is Version) {
            return false
        }

        return (majorVer == other.majorVer
                && minorVer == other.minorVer
                && patchVer == other.patchVer
                && (if (suffix == null) other.suffix == null
                        else suffix == other.suffix)
                && revision == other.revision
                && (if (gitCommit == null) other.gitCommit == null
                        else gitCommit == other.gitCommit))
    }

    override fun hashCode(): Int {
        var hashCode = 1
        hashCode = 31 * hashCode + majorVer
        hashCode = 31 * hashCode + minorVer
        hashCode = 31 * hashCode + patchVer
        hashCode = 31 * hashCode + if (suffix == null) 0 else suffix!!.hashCode()
        hashCode = 31 * hashCode + revision
        hashCode = 31 * hashCode + if (gitCommit == null) 0 else gitCommit!!.hashCode()
        return hashCode
    }

    override fun toString(): String {
        val sb = StringBuilder()
        sb.append(majorVer).append(".")
        sb.append(minorVer).append(".")
        sb.append(patchVer)
        if (revision > 0) {
            sb.append(".r").append(revision)
        }
        if (gitCommit != null) {
            sb.append(".g").append(gitCommit)
        }
        return sb.toString()
    }

    fun dump(): String {
        return ("majorVer: $majorVer" +
                ", minorVer: $minorVer" +
                ", patchVer: $patchVer" +
                ", suffix: $suffix" +
                ", revision: $revision" +
                ", gitCommit: $gitCommit")
    }

    class VersionParseException : Exception {
        constructor(detailMessage: String) : super(detailMessage)

        constructor(detailMessage: String, throwable: Throwable) : super(detailMessage, throwable)

        constructor(throwable: Throwable) : super(throwable)
    }

    companion object {
        /**
         * This factory method returns null instead of throwing an exception.
         *
         * @param versionString Version string
         * @return Version object or null
         */
        fun from(versionString: String): Version? {
            return try {
                Version(versionString)
            } catch (e: VersionParseException) {
                null
            }
        }
    }
}