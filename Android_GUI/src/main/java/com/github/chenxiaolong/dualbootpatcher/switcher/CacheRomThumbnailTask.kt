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

package com.github.chenxiaolong.dualbootpatcher.switcher

import android.content.Context
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.media.ThumbnailUtils
import android.net.Uri
import android.os.AsyncTask
import android.util.Log
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation
import com.squareup.picasso.Picasso
import java.io.File
import java.io.FileNotFoundException
import java.io.FileOutputStream
import java.io.IOException

class CacheRomThumbnailTask(
        private val context: Context,
        private val romInfo: RomInformation,
        private val uri: Uri,
        private val listener: CacheRomThumbnailTaskListener?
) : AsyncTask<Void, Void, Void>() {
    interface CacheRomThumbnailTaskListener {
        fun onRomThumbnailCached(info: RomInformation)
    }

    private fun createThumbnailFromUri(uri: Uri): Bitmap? {
        return try {
            val input = context.contentResolver.openInputStream(uri)
            input?.use {
                val bitmap = BitmapFactory.decodeStream(input)
                if (bitmap == null) null
                        else ThumbnailUtils.extractThumbnail(bitmap, 500, 500)
            }
        } catch (e: FileNotFoundException) {
            Log.w(TAG, "Failed to cache thumbnail", e)
            null
        } catch (e: IOException) {
            Log.w(TAG, "Failed to cache thumbnail", e)
            null
        }
    }

    override fun doInBackground(vararg params: Void): Void? {
        val thumbnail = createThumbnailFromUri(uri) ?: return null

        val f = File(romInfo.thumbnailPath!!)
        f.parentFile.mkdirs()

        try {
            FileOutputStream(f).use { out ->
                thumbnail.compress(Bitmap.CompressFormat.WEBP, 100, out)
            }
        } catch (e: FileNotFoundException) {
            Log.w(TAG, "Failed to write compressed thumbnail", e)
        }

        Picasso.get().invalidate(f)

        return null
    }

    override fun onPostExecute(result: Void?) {
        listener?.onRomThumbnailCached(romInfo)
    }

    companion object {
        val TAG: String = CacheRomThumbnailTask::class.java.simpleName
    }
}