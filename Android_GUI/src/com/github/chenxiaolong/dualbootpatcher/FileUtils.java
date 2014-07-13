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

import android.annotation.SuppressLint;
import android.content.ContentUris;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.storage.StorageManager;
import android.provider.DocumentsContract;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.Method;

public class FileUtils {
    @SuppressLint("NewApi")
    private static String getPathFromDocumentsUri(Context context, Uri uri) {
        // Based on
        // frameworks/base/packages/ExternalStorageProvider/src/com/android/externalstorage/ExternalStorageProvider.java
        StorageManager sm = (StorageManager) context.getSystemService(Context.STORAGE_SERVICE);

        try {
            Class<? extends StorageManager> StorageManager = sm.getClass();
            Method getVolumeList = StorageManager.getDeclaredMethod("getVolumeList");
            Object[] vols = (Object[]) getVolumeList.invoke(sm);

            Class<?> StorageVolume = Class.forName("android.os.storage.StorageVolume");
            Method isPrimary = StorageVolume.getDeclaredMethod("isPrimary");
            Method isEmulated = StorageVolume.getDeclaredMethod("isEmulated");
            Method getUuid = StorageVolume.getDeclaredMethod("getUuid");
            Method getPath = StorageVolume.getDeclaredMethod("getPath");

            // As of AOSP 4.4.2 in ExternalStorageProvider.java
            final String ROOT_ID_PRIMARY_EMULATED = "primary";

            String[] split = DocumentsContract.getDocumentId(uri).split(":");

            String volId;
            for (Object vol : vols) {
                if ((Boolean) isPrimary.invoke(vol) && (Boolean) isEmulated.invoke(vol)) {
                    volId = ROOT_ID_PRIMARY_EMULATED;
                } else if (getUuid.invoke(vol) != null) {
                    volId = (String) getUuid.invoke(vol);
                } else {
                    Log.e("DualBootPatcher", "Missing UUID for " + getPath.invoke(vol));
                    continue;
                }

                if (volId.equals(split[0])) {
                    return getPath.invoke(vol) + File.separator + split[1];
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
        Uri contentUri = ContentUris.withAppendedId(Uri.parse
                ("content://downloads/public_downloads"), Long.valueOf(id));

        Cursor cursor = null;
        String[] projection = {"_data"};

        try {
            cursor = context.getContentResolver().query(contentUri, projection, null, null, null);
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
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT &&
                ("com.android.externalstorage.documents").equals(uri.getAuthority())) {
            return getPathFromDocumentsUri(context, uri);
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT
                && ("com.android.providers.downloads.documents").equals(uri.getAuthority())) {
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

    public static String extractVersionedAssetToCache(Context context, String asset) {
        String prefix = asset + "::";
        String target = prefix + BuildConfig.VERSION_NAME;

        File cachedFile = new File(context.getCacheDir() + File.separator + target);
        if (!cachedFile.exists()) {
            FileUtils.extractAsset(context, asset, cachedFile);
        }

        return cachedFile.getAbsolutePath();
    }

    public static void deleteOldCachedAsset(Context context, String asset) {
        String prefix = asset + "::";
        String current = prefix + BuildConfig.VERSION_NAME;

        for (File f : context.getCacheDir().listFiles()) {
            if (f.isFile()) {
                String name = f.getName();

                if (name.startsWith(prefix) && !name.equals(current)) {
                    f.delete();
                }
            }
        }
    }

    public static boolean isSameInode(Context context, String file1, String file2) {
        if (!new RootFile(file1).isFile() || !new RootFile(file2).isFile()) {
            return false;
        }

        String ls = CommandUtils.getBusyboxCommandString(context, "ls", "-id", "\"${1}\"");
        String awk = CommandUtils.getBusyboxCommandString(context, "awk", "'{print $1}'");

        StringBuilder sb = new StringBuilder();
        sb.append("get_inode() {");
        sb.append("  ").append(ls).append(" | ").append(awk).append(";");
        sb.append("};");
        sb.append("same_inode() {");
        sb.append("  if test $(get_inode \"${1}\") == $(get_inode \"${2}\"); then");
        sb.append("    return 0;");
        sb.append("  else");
        sb.append("    return 1;");
        sb.append("  fi;");
        sb.append("};");
        sb.append("same_inode ").append(file1).append(" ").append(file2);

        return CommandUtils.runRootCommand(sb.toString()) == 0;
    }
}
