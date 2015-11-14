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

package com.github.chenxiaolong.dualbootpatcher.switcher;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.media.ThumbnailUtils;
import android.net.Uri;
import android.os.AsyncTask;

import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.squareup.picasso.Picasso;

import org.apache.commons.io.IOUtils;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

public class CacheRomThumbnailTask extends AsyncTask<Void, Void, Void> {
    private final Context mContext;
    private final RomInformation mRomInfo;
    private final Uri mUri;
    private final CacheRomThumbnailTaskListener mListener;

    public interface CacheRomThumbnailTaskListener {
        void onRomThumbnailCached(RomInformation info);
    }

    public CacheRomThumbnailTask(Context context, RomInformation info, Uri uri,
                                 CacheRomThumbnailTaskListener listener) {
        mContext = context;
        mRomInfo = info;
        mUri = uri;
        mListener = listener;
    }

    private Bitmap createThumbnailFromUri(Uri uri) {
        try {
            InputStream input = mContext.getContentResolver().openInputStream(uri);
            Bitmap bitmap = BitmapFactory.decodeStream(input);
            input.close();

            if (bitmap == null) {
                return null;
            }

            return ThumbnailUtils.extractThumbnail(bitmap, 500, 500);
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }

        return null;
    }

    @Override
    protected Void doInBackground(Void... params) {
        Bitmap thumbnail = createThumbnailFromUri(mUri);

        if (thumbnail == null) {
            return null;
        }

        File f = new File(mRomInfo.getThumbnailPath());
        f.getParentFile().mkdirs();

        FileOutputStream out = null;

        try {
            out = new FileOutputStream(f);
            thumbnail.compress(Bitmap.CompressFormat.WEBP, 100, out);
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } finally {
            IOUtils.closeQuietly(out);
        }

        Picasso.with(mContext).invalidate(f);

        return null;
    }

    @Override
    protected void onPostExecute(Void result) {
        if (mListener != null) {
            mListener.onRomThumbnailCached(mRomInfo);
        }
    }
}