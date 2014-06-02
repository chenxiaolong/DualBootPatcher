/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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

package com.github.chenxiaolong.dualbootpatcher;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.Method;

import android.annotation.SuppressLint;
import android.content.ContentUris;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.storage.StorageManager;
import android.provider.DocumentsContract;
import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.CommandUtils.CommandResult;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.RootCommandParams;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.RootCommandRunner;

public class FileUtils {
    @SuppressLint("NewApi")
    private static String getPathFromDocumentsUri(Context context, Uri uri) {
        // Based on
        // frameworks/base/packages/ExternalStorageProvider/src/com/android/externalstorage/ExternalStorageProvider.java
        StorageManager sm = (StorageManager) context
                .getSystemService(Context.STORAGE_SERVICE);

        try {
            Class<? extends StorageManager> StorageManager = sm.getClass();
            Method getVolumeList = StorageManager
                    .getDeclaredMethod("getVolumeList");
            Object[] vols = (Object[]) getVolumeList
                    .invoke(sm, new Object[] {});

            Class<?> StorageVolume = Class
                    .forName("android.os.storage.StorageVolume");
            Method isPrimary = StorageVolume.getDeclaredMethod("isPrimary");
            Method isEmulated = StorageVolume.getDeclaredMethod("isEmulated");
            Method getUuid = StorageVolume.getDeclaredMethod("getUuid");
            Method getPath = StorageVolume.getDeclaredMethod("getPath");

            // As of AOSP 4.4.2 in ExternalStorageProvider.java
            final String ROOT_ID_PRIMARY_EMULATED = "primary";

            String[] split = DocumentsContract.getDocumentId(uri).split(":");

            String volId;
            for (Object vol : vols) {
                if ((Boolean) isPrimary.invoke(vol, new Object[] {})
                        && (Boolean) isEmulated.invoke(vol, new Object[] {})) {
                    volId = ROOT_ID_PRIMARY_EMULATED;
                } else if (getUuid.invoke(vol, new Object[] {}) != null) {
                    volId = (String) getUuid.invoke(vol, new Object[] {});
                } else {
                    Log.e("DualBootPatcher",
                            "Missing UUID for "
                                    + getPath.invoke(vol, new Object[] {}));
                    continue;
                }

                if (volId.equals(split[0])) {
                    return getPath.invoke(vol, new Object[] {})
                            + File.separator + split[1];
                }
            }

            return null;
        } catch (Exception e) {
            Log.e("DualBootPatcher", "Java reflection failure: " + e);
            return null;
        }
    }

    @SuppressLint("NewApi")
    private static String getPathFromDownloadsUri(Context context, Uri uri) {
        // Based on
        // https://github.com/iPaulPro/aFileChooser/blob/master/aFileChooser/src/com/ipaulpro/afilechooser/utils/FileUtils.java
        String id = DocumentsContract.getDocumentId(uri);
        Uri contentUri = ContentUris.withAppendedId(
                Uri.parse("content://downloads/public_downloads"),
                Long.valueOf(id));

        Cursor cursor = null;
        String[] projection = { "_data" };

        try {
            cursor = context.getContentResolver().query(contentUri, projection,
                    null, null, null);
            if (cursor != null && cursor.moveToFirst()) {
                return cursor.getString(cursor.getColumnIndexOrThrow("_data"));
            }
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }

        return null;
    }

    public static String getPathFromUri(Context context, Uri uri) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT
                && "com.android.externalstorage.documents".equals(uri
                        .getAuthority())) {
            return getPathFromDocumentsUri(context, uri);
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT
                && "com.android.providers.downloads.documents".equals(uri
                        .getAuthority())) {
            return getPathFromDownloadsUri(context, uri);
        } else {
            return uri.getPath();
        }
    }

    public static void extractAsset(Context context, String src, File dest) {
        try {
            InputStream i = context.getAssets().open(src);
            FileOutputStream o = new FileOutputStream(dest);

            // byte[] buffer = new byte[4096];
            // int length;
            // while ((length = i.read(buffer)) > 0) {
            // o.write(buffer, 0, length);
            // }

            int length = i.available();
            byte[] buffer = new byte[length];
            i.read(buffer);
            o.write(buffer);

            o.flush();
            o.close();
            i.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public static void recursiveDelete(File f) {
        if (f.isFile()) {
            f.delete();
        } else if (f.isDirectory()) {
            for (File f2 : f.listFiles()) {
                recursiveDelete(f2);
            }
            f.delete();
        }
    }

    public static boolean isExistsDirectory(String path) {
        File file = new File(path);
        if (file.exists() && file.isDirectory() && file.canRead()) {
            return true;
        } else {
            RootCommandParams params = new RootCommandParams();
            params.command = "ls " + path;

            RootCommandRunner cmd = new RootCommandRunner(params);
            cmd.start();

            try {
                cmd.join();
                CommandResult result = cmd.getResult();

                return result.exitCode == 0;
            } catch (Exception e) {
                e.printStackTrace();
            }

            return false;
        }
    }

    public static boolean makedirs(String directory) {
        File file = new File(directory);
        if (file.exists() && file.isDirectory()) {
            return true;
        } else if (file.mkdirs()) {
            return true;
        } else {
            RootCommandParams params = new RootCommandParams();
            params.command = "mkdir -p " + directory;

            RootCommandRunner cmd = new RootCommandRunner(params);
            cmd.start();
            try {
                cmd.join();
            } catch (Exception e) {
                e.printStackTrace();
            }

            return isExistsDirectory(directory);
        }
    }
}
