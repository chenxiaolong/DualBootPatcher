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
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.provider.DocumentsContract;
import android.provider.OpenableColumns;
import android.support.annotation.NonNull;
import android.util.Log;

import org.apache.commons.io.IOUtils;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

import io.noobdev.neuteredsaf.DocumentsActivity;
import io.noobdev.neuteredsaf.DocumentsApplication;
import io.noobdev.neuteredsaf.compat.DocumentsContractCompat;
import io.noobdev.neuteredsaf.providers.ExternalStorageProvider;
import io.noobdev.neuteredsaf.providers.ProviderConstants;

public class FileUtils {
    private static final String TAG = FileUtils.class.getSimpleName();

    private static final boolean FORCE_NEUTERED_SAF = false;
    private static final boolean FORCE_GET_CONTENT = false;

    @SuppressLint("NewApi")
    public static Uri getDocumentUriFromTreeUri(Uri uri) {
        String neuteredSafAuthority = DocumentsApplication.getApplicationId() +
                ExternalStorageProvider.AUTHORITY_SUFFIX;

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT &&
                ("com.android.externalstorage.documents").equals(uri.getAuthority())) {
            return DocumentsContract.buildDocumentUriUsingTree(
                    uri, DocumentsContract.getTreeDocumentId(uri));
        } else if (neuteredSafAuthority.equals(uri.getAuthority())) {
            return DocumentsContractCompat.buildDocumentUriUsingTree(
                    uri, DocumentsContractCompat.getTreeDocumentId(uri));
        } else {
            return null;
        }
    }

    private static boolean canHandleIntent(PackageManager pm, Intent intent) {
        List<ResolveInfo> list = pm.queryIntentActivities(intent, 0);
        return list.size() > 0;
    }

    private static boolean shouldHaveNativeSaf() {
        return !FORCE_NEUTERED_SAF && Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT;
    }

    private static boolean isOxygenOS(Context context) {
        String value = SystemPropertiesProxy.get(context, "ro.build.version.ota");
        if (value == null) {
            value = SystemPropertiesProxy.get(context, "ro.build.ota.versionname");
        }
        return value != null && value.contains("OnePlus") && value.contains("Oxygen");
    }

    private static boolean isOpenDocumentBroken(Context context) {
        return FORCE_GET_CONTENT || isOxygenOS(context);
    }

    private static void setCommonNativeSafOptions(Intent intent) {
        // Avoid cloud providers since we need the C file descriptor for the file
        intent.putExtra(Intent.EXTRA_LOCAL_ONLY, true);
        // Hack to show internal/external storage by default
        intent.putExtra("android.content.extra.SHOW_ADVANCED", true);
    }

    private static void setCommonOpenOptions(Intent intent, String mimeType) {
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType(mimeType);
    }

    private static void setCommonSaveOptions(Intent intent, String mimeType, String defaultName) {
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType(mimeType);
        intent.putExtra(Intent.EXTRA_TITLE, defaultName);
    }

    private static Intent buildNativeSafOpenDocumentIntent(String mimeType) {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        setCommonNativeSafOptions(intent);
        setCommonOpenOptions(intent, mimeType);
        return intent;
    }

    private static Intent buildNativeSafOpenDocumentTreeIntent() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        setCommonNativeSafOptions(intent);
        return intent;
    }

    private static Intent buildNativeSafGetContentIntent(String mimeType) {
        Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
        setCommonNativeSafOptions(intent);
        setCommonOpenOptions(intent, mimeType);
        return intent;
    }

    private static Intent buildNeuteredSafOpenDocumentIntent(Context context, String mimeType) {
        Intent intent = new Intent(context, DocumentsActivity.class);
        intent.setAction(ProviderConstants.ACTION_OPEN_DOCUMENT);
        setCommonOpenOptions(intent, mimeType);
        return intent;
    }

    private static Intent buildNeuteredSafOpenDocumentTreeIntent(Context context) {
        Intent intent = new Intent(context, DocumentsActivity.class);
        intent.setAction(ProviderConstants.ACTION_OPEN_DOCUMENT_TREE);
        return intent;
    }

    private static Intent buildNativeSafCreateDocumentIntent(String mimeType, String defaultName) {
        Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        setCommonNativeSafOptions(intent);
        setCommonSaveOptions(intent, mimeType, defaultName);
        return intent;
    }

    private static Intent buildNeuteredSafCreateDocumentIntent(Context context, String mimeType,
                                                               String defaultName) {
        Intent intent = new Intent(context, DocumentsActivity.class);
        intent.setAction(ProviderConstants.ACTION_CREATE_DOCUMENT);
        setCommonNativeSafOptions(intent);
        setCommonSaveOptions(intent, mimeType, defaultName);
        return intent;
    }

    @NonNull
    public static Intent getFileOpenIntent(Context context, String mimeType) {
        PackageManager pm = context.getPackageManager();
        Intent intent = null;

        if (shouldHaveNativeSaf()) {
            // Avoid ACTION_OPEN_DOCUMENT on OxygenOS since it browses zips instead of
            // selecting them
            if (isOpenDocumentBroken(context)) {
                Log.d(TAG, "Can't use ACTION_OPEN_DOCUMENT due to OxygenOS bug");
            } else {
                intent = buildNativeSafOpenDocumentIntent(mimeType);
            }

            // Use ACTION_GET_CONTENT if this is 4.4+, but DocumentsUI is missing (wtf) or
            // ACTION_OPEN_DOCUMENT won't work for some reason
            if (intent == null || !canHandleIntent(pm, intent)) {
                Log.w(TAG, "ACTION_OPEN_DOCUMENT cannot be handled on 4.4+ device");
                intent = buildNativeSafGetContentIntent(mimeType);
            }

            // If neither ACTION_OPEN_DOCUMENT nor ACTION_GET_CONTENT can be handled, fall back to
            // NeuteredSaf
            if (!canHandleIntent(pm, intent)) {
                Log.w(TAG, "Neither ACTION_OPEN_DOCUMENT nor ACTION_GET_CONTENT can be handled");
                intent = null;
            }
        }

        // Fall back to NeuteredSaf for all other scenarios
        if (intent == null) {
            intent = buildNeuteredSafOpenDocumentIntent(context, mimeType);
        }

        return intent;
    }

    @NonNull
    public static Intent getFileTreeOpenIntent(Context context) {
        PackageManager pm = context.getPackageManager();
        Intent intent = null;

        if (shouldHaveNativeSaf()) {
            intent = buildNativeSafOpenDocumentTreeIntent();

            if (!canHandleIntent(pm, intent)) {
                Log.w(TAG, "ACTION_OPEN_DOCUMENT_TREE cannot be handled");
                intent = null;
            }
        }

        // Fall back to NeuteredSaf
        if (intent == null) {
            intent = buildNeuteredSafOpenDocumentTreeIntent(context);
        }

        return intent;
    }

    @NonNull
    public static Intent getFileSaveIntent(Context context, String mimeType, String defaultName) {
        Intent intent = null;

        if (shouldHaveNativeSaf()) {
            // Try ACTION_CREATE_DOCUMENT
            intent = buildNativeSafCreateDocumentIntent(mimeType, defaultName);

            // If DocumentsUI is missing, fall back to NeuteredSaf
            if (!canHandleIntent(context.getPackageManager(), intent)) {
                Log.w(TAG, "ACTION_CREATE_DOCUMENT cannot be handled on 4.4+ device");
                intent = null;
            }
        }

        // Fall back to NeuteredSaf for all other scenarios
        if (intent == null) {
            intent = buildNeuteredSafCreateDocumentIntent(context, mimeType, defaultName);
        }

        return intent;
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

    public static boolean zipExtractFile(Context context, Uri uri, String filename,
                                         String destFile) {
        ZipInputStream zis = null;
        FileOutputStream fos = null;

        try {
            zis = new ZipInputStream(context.getContentResolver().openInputStream(uri));

            ZipEntry entry;
            while ((entry = zis.getNextEntry()) != null) {
                if (entry.getName().equals(filename)) {
                    fos = new FileOutputStream(destFile);
                    IOUtils.copy(zis, fos);
                    return true;
                }
            }
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            IOUtils.closeQuietly(zis);
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

    public static class UriMetadata {
        public Uri uri;
        public String displayName;
        public long size;
        public String mimeType;
    }

    public static UriMetadata[] queryUriMetadata(ContentResolver cr, Uri... uris) {
        ThreadUtils.enforceExecutionOnNonMainThread();

        UriMetadata[] metadatas = new UriMetadata[uris.length];
        for (int i = 0; i < metadatas.length; i++) {
            UriMetadata metadata = new UriMetadata();
            metadatas[i] = metadata;
            metadata.uri = uris[i];
            metadata.mimeType = cr.getType(metadata.uri);

            if ("content".equals(metadata.uri.getScheme())) {
                Cursor cursor = cr.query(metadata.uri, null, null, null, null, null);
                try {
                    if (cursor != null && cursor.moveToFirst()) {
                        int nameIndex = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                        int sizeIndex = cursor.getColumnIndex(OpenableColumns.SIZE);

                        metadata.displayName = cursor.getString(nameIndex);
                        if (cursor.isNull(sizeIndex)) {
                            metadata.size = -1;
                        } else {
                            metadata.size = cursor.getLong(sizeIndex);
                        }
                    }
                } finally {
                    IOUtils.closeQuietly(cursor);
                }
            } else if ("file".equals(metadata.uri.getScheme())) {
                metadata.displayName = metadata.uri.getLastPathSegment();
                metadata.size = new File(metadata.uri.getPath()).length();
            } else {
                throw new IllegalArgumentException("Cannot handle URI: " + metadata.uri);
            }
        }

        return metadatas;
    }
}
