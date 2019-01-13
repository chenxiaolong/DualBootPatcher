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

package com.github.chenxiaolong.dualbootpatcher

import android.annotation.TargetApi
import android.content.ContentResolver
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.provider.DocumentsContract
import android.provider.OpenableColumns
import android.util.Log
import androidx.documentfile.provider.DocumentFile
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMiniZip.MiniZipInputFile
import com.github.chenxiaolong.dualbootpatcher.pathchooser.PathChooserActivity
import org.apache.commons.text.translate.LookupTranslator
import java.io.BufferedReader
import java.io.File
import java.io.FileOutputStream
import java.io.FileReader
import java.io.IOException
import java.util.*

object FileUtils {
    private val TAG = FileUtils::class.java.simpleName

    const val FILE_SCHEME = "file"
    const val SAF_SCHEME = "content"
    const val SAF_AUTHORITY = "com.android.externalstorage.documents"

    private const val FORCE_PATH_CHOOSER = false
    private const val FORCE_GET_CONTENT = false

    private const val PROC_MOUNTS = "/proc/mounts"

    // According to the getmntent(3) manpage and the glibc source code, these are the only escaped
    // values
    @Suppress("UNCHECKED_CAST")
    private val UNESCAPE_MOUNT_ENTRY = LookupTranslator(hashMapOf(
            "\\040" to " ",
            "\\011" to "\t",
            "\\012" to "\n",
            "\\134" to "\\",
            "\\\\" to "\\"
    ) as Map<CharSequence, CharSequence>)

    private val BASE2_ABBREVS = arrayOf(
            Base2Abbrev(1L shl 60, R.string.format_exbibytes),
            Base2Abbrev(1L shl 50, R.string.format_pebibytes),
            Base2Abbrev(1L shl 40, R.string.format_tebibytes),
            Base2Abbrev(1L shl 30, R.string.format_gibibytes),
            Base2Abbrev(1L shl 20, R.string.format_mebibytes),
            Base2Abbrev(1L shl 10, R.string.format_kibibytes),
            Base2Abbrev(1L, R.string.format_bytes)
    )

    val mounts: Array<MountEntry>
        @Throws(IOException::class)
        get() {
            val entries = ArrayList<MountEntry>()

            BufferedReader(FileReader(PROC_MOUNTS)).use { br ->
                br.forEachLine { line ->
                    val pieces = line.split("[ \t]".toRegex())
                    if (pieces.size != 6) {
                        throw IOException("Illegal mount entry: $line")
                    }

                    val mnt_freq: Int
                    val mnt_passno: Int

                    try {
                        mnt_freq = Integer.parseInt(pieces[4])
                    } catch (e: NumberFormatException) {
                        throw IOException("Illegal mnt_freq value: ${pieces[4]}", e)
                    }

                    try {
                        mnt_passno = Integer.parseInt(pieces[5])
                    } catch (e: NumberFormatException) {
                        throw IOException("Illegal mnt_passno value: ${pieces[5]}", e)
                    }

                    entries.add(MountEntry(
                            UNESCAPE_MOUNT_ENTRY.translate(pieces[0]),
                            UNESCAPE_MOUNT_ENTRY.translate(pieces[1]),
                            UNESCAPE_MOUNT_ENTRY.translate(pieces[2]),
                            UNESCAPE_MOUNT_ENTRY.translate(pieces[3]),
                            mnt_freq,
                            mnt_passno
                    ))
                }
            }

            return entries.toTypedArray()
        }

    private fun canHandleIntent(pm: PackageManager, intent: Intent?): Boolean {
        val list = pm.queryIntentActivities(intent, 0)
        return list.size > 0
    }

    private fun shouldHaveNativeSaf(): Boolean {
        return !FORCE_PATH_CHOOSER && Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT
    }

    private fun isOxygenOS(context: Context): Boolean {
        var value: String? = SystemPropertiesProxy[context, "ro.build.version.ota"]
        if (value == null) {
            value = SystemPropertiesProxy[context, "ro.build.ota.versionname"]
        }
        return value.contains("OnePlus") && value.contains("Oxygen")
    }

    private fun isOpenDocumentBroken(context: Context): Boolean {
        return FORCE_GET_CONTENT || isOxygenOS(context)
    }

    private fun setCommonNativeSafOptions(intent: Intent) {
        // Avoid cloud providers since we need the C file descriptor for the file
        intent.putExtra(Intent.EXTRA_LOCAL_ONLY, true)
        // Hack to show internal/external storage by default
        intent.putExtra("android.content.extra.SHOW_ADVANCED", true)
    }

    private fun setCommonOpenOptions(intent: Intent, mimeType: String?) {
        intent.addCategory(Intent.CATEGORY_OPENABLE)
        intent.type = mimeType
    }

    private fun setCommonSaveOptions(intent: Intent, mimeType: String?, defaultName: String?) {
        intent.addCategory(Intent.CATEGORY_OPENABLE)
        intent.type = mimeType
        intent.putExtra(Intent.EXTRA_TITLE, defaultName)
    }

    @TargetApi(Build.VERSION_CODES.KITKAT)
    private fun buildSafOpenDocumentIntent(mimeType: String?): Intent {
        val intent = Intent(Intent.ACTION_OPEN_DOCUMENT)
        setCommonNativeSafOptions(intent)
        setCommonOpenOptions(intent, mimeType)
        return intent
    }

    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    private fun buildSafOpenDocumentTreeIntent(): Intent {
        val intent = Intent(Intent.ACTION_OPEN_DOCUMENT_TREE)
        setCommonNativeSafOptions(intent)
        return intent
    }

    private fun buildSafGetContentIntent(mimeType: String?): Intent {
        val intent = Intent(Intent.ACTION_GET_CONTENT)
        setCommonNativeSafOptions(intent)
        setCommonOpenOptions(intent, mimeType)
        return intent
    }

    @TargetApi(Build.VERSION_CODES.KITKAT)
    private fun buildSafCreateDocumentIntent(mimeType: String?, defaultName: String?): Intent {
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT)
        setCommonNativeSafOptions(intent)
        setCommonSaveOptions(intent, mimeType, defaultName)
        return intent
    }

    private fun buildPathChooserOpenFileIntent(context: Context, mimeType: String?): Intent {
        val intent = Intent(context, PathChooserActivity::class.java)
        intent.action = PathChooserActivity.ACTION_OPEN_FILE
        setCommonOpenOptions(intent, mimeType)
        return intent
    }

    private fun buildPathChooserOpenDirectoryIntent(context: Context): Intent {
        val intent = Intent(context, PathChooserActivity::class.java)
        intent.action = PathChooserActivity.ACTION_OPEN_DIRECTORY
        return intent
    }

    private fun buildPathChooserSaveFileIntent(context: Context, mimeType: String?,
                                               defaultName: String?): Intent {
        val intent = Intent(context, PathChooserActivity::class.java)
        intent.action = PathChooserActivity.ACTION_SAVE_FILE
        setCommonSaveOptions(intent, mimeType, defaultName)
        return intent
    }

    fun getFileOpenIntent(context: Context, mimeType: String?): Intent {
        val pm = context.packageManager
        var intent: Intent? = null

        if (shouldHaveNativeSaf()) {
            // Avoid ACTION_OPEN_DOCUMENT on OxygenOS since it browses zips instead of
            // selecting them
            if (isOpenDocumentBroken(context)) {
                Log.d(TAG, "Can't use ACTION_OPEN_DOCUMENT due to OxygenOS bug")
            } else {
                intent = buildSafOpenDocumentIntent(mimeType)
            }

            // Use ACTION_GET_CONTENT if this is 4.4+, but DocumentsUI is missing (wtf) or
            // ACTION_OPEN_DOCUMENT won't work for some reason
            if (intent == null || !canHandleIntent(pm, intent)) {
                Log.w(TAG, "ACTION_OPEN_DOCUMENT cannot be handled on 4.4+ device")
                intent = buildSafGetContentIntent(mimeType)
            }

            // If neither ACTION_OPEN_DOCUMENT nor ACTION_GET_CONTENT can be handled, fall back to
            // PathChooserActivity
            if (!canHandleIntent(pm, intent)) {
                Log.w(TAG, "Neither ACTION_OPEN_DOCUMENT nor ACTION_GET_CONTENT can be handled")
                intent = null
            }
        }

        // Fall back to PathChooserActivity for all other scenarios
        if (intent == null) {
            intent = buildPathChooserOpenFileIntent(context, mimeType)
        }

        return intent
    }

    fun getFileTreeOpenIntent(context: Context): Intent {
        val pm = context.packageManager
        var intent: Intent? = null

        if (shouldHaveNativeSaf()) {
            intent = buildSafOpenDocumentTreeIntent()

            if (!canHandleIntent(pm, intent)) {
                Log.w(TAG, "ACTION_OPEN_DOCUMENT_TREE cannot be handled")
                intent = null
            }
        }

        // Fall back to PathChooserActivity
        if (intent == null) {
            intent = buildPathChooserOpenDirectoryIntent(context)
        }

        return intent
    }

    fun getFileSaveIntent(context: Context, mimeType: String?, defaultName: String?): Intent {
        var intent: Intent? = null

        if (shouldHaveNativeSaf()) {
            // Try ACTION_CREATE_DOCUMENT
            intent = buildSafCreateDocumentIntent(mimeType, defaultName)

            // If DocumentsUI is missing, fall back to NeuteredSaf
            if (!canHandleIntent(context.packageManager, intent)) {
                Log.w(TAG, "ACTION_CREATE_DOCUMENT cannot be handled on 4.4+ device")
                intent = null
            }
        }

        // Fall back to PathChooserActivity for all other scenarios
        if (intent == null) {
            intent = buildPathChooserSaveFileIntent(context, mimeType, defaultName)
        }

        return intent
    }

    @Throws(IOException::class)
    fun extractAsset(context: Context, src: String, dest: File) {
        context.assets.open(src).use { i ->
            FileOutputStream(dest).use { o ->
                i.copyTo(o)
            }
        }
    }

    @Throws(IOException::class)
    fun zipExtractFile(context: Context, uri: Uri, filename: String, destFile: String) {
        (context.contentResolver.openFileDescriptor(uri, "r") ?:
                throw IOException("Failed to open URI: $uri")).use { pfd ->
            MiniZipInputFile("/proc/self/fd/${pfd.fd}").use { mzif ->
                while (true) {
                    val entry = mzif.nextEntry()
                    when {
                        entry == null -> return
                        entry.name == filename -> {
                            FileOutputStream(destFile).use { fos ->
                                mzif.inputStream.copyTo(fos)
                            }
                            return
                        }
                    }
                }
            }
        }
    }

    private class Base2Abbrev(internal var factor: Long, internal var stringResId: Int)

    fun toHumanReadableSize(context: Context, size: Long, precision: Long): String {
        val abbrev: Base2Abbrev = BASE2_ABBREVS.firstOrNull { size >= it.factor } ?:
                return context.getString(R.string.format_bytes, size)

        val decimal = String.format("%.${precision}f", size.toDouble() / abbrev.factor)
        return context.getString(abbrev.stringResId, decimal)
    }

    class UriMetadata(val uri: Uri) {
        var displayName: String? = null
        var size: Long = 0
        var mimeType: String? = null
    }

    fun queryUriMetadata(cr: ContentResolver, vararg uris: Uri): Array<UriMetadata> {
        ThreadUtils.enforceExecutionOnNonMainThread()

        return uris.map { uri ->
            val metadata = UriMetadata(uri)
            metadata.mimeType = cr.getType(uri)

            when (uri.scheme) {
                SAF_SCHEME -> {
                    cr.query(uri, null, null, null, null, null)?.use { cursor ->
                        if (cursor.moveToFirst()) {
                            val nameIndex = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME)
                            val sizeIndex = cursor.getColumnIndex(OpenableColumns.SIZE)

                            metadata.displayName = cursor.getString(nameIndex)
                            if (cursor.isNull(sizeIndex)) {
                                metadata.size = -1
                            } else {
                                metadata.size = cursor.getLong(sizeIndex)
                            }
                        }
                    }
                }
                FILE_SCHEME -> {
                    metadata.displayName = uri.lastPathSegment
                    metadata.size = File(uri.path).length()
                }
                else -> throw IllegalArgumentException("Cannot handle URI: $uri")
            }

            metadata
        }.toTypedArray()
    }

    fun getDocumentFile(context: Context, uri: Uri): androidx.documentfile.provider.DocumentFile {
        var df: DocumentFile? = null

        if (FILE_SCHEME == uri.scheme) {
            df = DocumentFile.fromFile(File(uri.path))
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT &&
                SAF_SCHEME == uri.scheme && SAF_AUTHORITY == uri.authority) {
            if (DocumentsContract.isDocumentUri(context, uri)) {
                df = DocumentFile.fromSingleUri(context, uri)
            } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N
                    && DocumentsContract.isTreeUri(uri)) {
                df = DocumentFile.fromTreeUri(context, uri)
            } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                // Best guess is that it's a tree...
                df = DocumentFile.fromTreeUri(context, uri)
            }
        }

        if (df == null) {
            throw IllegalArgumentException("Invalid URI: $uri")
        }

        return df
    }

    class MountEntry(
        val mnt_fsname: String,
        val mnt_dir: String,
        val mnt_type: String,
        val mnt_opts: String,
        val mnt_freq: Int,
        val mnt_passno: Int
    )
}