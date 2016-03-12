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

package com.github.chenxiaolong.dualbootpatcher;

import android.annotation.SuppressLint;
import android.app.Fragment;
import android.app.FragmentManager;
import android.content.ContentUris;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.storage.StorageManager;
import android.provider.DocumentsContract;
import android.support.annotation.NonNull;
import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericConfirmDialog;

import org.apache.commons.io.IOUtils;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.Method;
import java.util.Enumeration;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import io.noobdev.neuteredsaf.DocumentsActivity;
import io.noobdev.neuteredsaf.providers.ProviderConstants;

public class FileUtils {
    private static final boolean FORCE_NEUTERED_SAF = false;

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
                ("content://downloads/public_downloads"), Long.parseLong(id));

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

    private static boolean canHandleIntent(PackageManager pm, Intent intent) {
        List<ResolveInfo> list = pm.queryIntentActivities(intent, 0);
        return list.size() > 0;
    }

    public static boolean useNativeSaf() {
        return !FORCE_NEUTERED_SAF && Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT;
    }

    public static boolean isOxygenOS(Context context) {
        String value = SystemPropertiesProxy.get(context, "ro.build.version.ota");
        if (value == null) {
            value = SystemPropertiesProxy.get(context, "ro.build.ota.versionname");
        }
        return value != null && value.contains("OnePlus2Oxygen");
    }

    @NonNull
    public static Intent getFileOpenIntent(Context context) {
        Intent intent;

        if (useNativeSaf()) {
            if (isOxygenOS(context)) {
                intent = new Intent(Intent.ACTION_GET_CONTENT);
            } else {
                intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            }
            intent.putExtra(Intent.EXTRA_LOCAL_ONLY, true);
            intent.putExtra("android.content.extra.SHOW_ADVANCED", true);
        } else {
            intent = new Intent(context, DocumentsActivity.class);
            intent.setAction(ProviderConstants.ACTION_OPEN_DOCUMENT);
        }

        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");

        return intent;
    }

    @NonNull
    public static Intent getFileSaveIntent(Context context, String defaultName) {
        Intent intent;

        if (useNativeSaf()) {
            intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
            intent.putExtra(Intent.EXTRA_LOCAL_ONLY, true);
            intent.putExtra("android.content.extra.SHOW_ADVANCED", true);
        } else {
            intent = new Intent(context, DocumentsActivity.class);
            intent.setAction(ProviderConstants.ACTION_CREATE_DOCUMENT);
        }

        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        intent.putExtra(Intent.EXTRA_TITLE, defaultName);

        return intent;
    }

    public static void showMissingFileChooserDialog(Context context, FragmentManager fm) {
        Fragment prev = fm.findFragmentByTag("no_file_chooser");

        if (prev == null) {
            GenericConfirmDialog dialog = GenericConfirmDialog.newInstanceFromActivity(-1,
                    context.getString(R.string.filemanager_missing_title),
                    context.getString(R.string.filemanager_missing_desc), null);
            dialog.show(fm, "no_file_chooser");
        }
    }

    public static void extractAsset(Context context, String src, File dest) {
        InputStream i = null;
        FileOutputStream o = null;
        try {
            i = context.getAssets().open(src);
            o = new FileOutputStream(dest);

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
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            IOUtils.closeQuietly(i);
            IOUtils.closeQuietly(o);
        }
    }

    public static boolean zipExtractFile(String zipFile, String filename, String destFile) {
        ZipFile zf = null;
        FileOutputStream fos = null;

        try {
            zf = new ZipFile(zipFile);

            final Enumeration<? extends ZipEntry> entries = zf.entries();
            while (entries.hasMoreElements()) {
                final ZipEntry ze = entries.nextElement();

                if (ze.getName().equals(filename)) {
                    fos = new FileOutputStream(destFile);
                    InputStream is = zf.getInputStream(ze);
                    byte[] buffer = new byte[10240];

                    int len;
                    while ((len = is.read(buffer)) > 0) {
                        fos.write(buffer, 0, len);
                    }

                    return true;
                }
            }
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            //IOUtils.closeQuietly(zf);
            try {
                if (zf != null) {
                    zf.close();
                }
            } catch (IOException e) {
            }
            IOUtils.closeQuietly(fos);
        }

        return false;
    }

    private static class Base2Abbrev {
        long factor;
        int stringResId;

        public Base2Abbrev(long factor, int stringResId) {
            this.factor = factor;
            this.stringResId = stringResId;
        }
    }

    private static final Base2Abbrev[] BASE2_ABBREVS = new Base2Abbrev[] {
            new Base2Abbrev(1l << 60, R.string.format_exbibytes),
            new Base2Abbrev(1l << 50, R.string.format_pebibytes),
            new Base2Abbrev(1l << 40, R.string.format_tebibytes),
            new Base2Abbrev(1l << 30, R.string.format_gibibytes),
            new Base2Abbrev(1l << 20, R.string.format_mebibytes),
            new Base2Abbrev(1l << 10, R.string.format_kibibytes),
            new Base2Abbrev(1l,       R.string.format_bytes)
    };

    @NonNull
    public static String toHumanReadableSize(Context context, long size, long precision) {
        Base2Abbrev abbrev = null;

        for (Base2Abbrev b2a : BASE2_ABBREVS) {
            if (size >= b2a.factor) {
                abbrev = b2a;
                break;
            }
        }

        if (abbrev == null) {
            return context.getString(R.string.format_bytes, size);
        }

        String decimal = String.format("%." + precision + "f", (double) size / abbrev.factor);
        return context.getString(abbrev.stringResId, decimal);
    }
}
