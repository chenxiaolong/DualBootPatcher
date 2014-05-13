package com.github.chenxiaolong.dualbootpatcher;

import java.io.File;
import java.lang.reflect.Method;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.Fragment;
import android.app.FragmentTransaction;
import android.content.BroadcastReceiver;
import android.content.ContentUris;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager.NameNotFoundException;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.storage.StorageManager;
import android.provider.DocumentsContract;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.TextView;

public class MainActivity extends Activity {
    private static final int REQUEST_FILE = 1234;
    private static final int REQUEST_PATCH_FILE = 2345;
    private static final String PATCH_FILE = "com.github.chenxiaolong.dualbootpatcher.PATCH_FILE";

    private Button mChooseFileButton;

    private Spinner mPartConfigSpinner;
    private int mPartConfigSpinnerPos = 0;
    private String mPartConfigText = "";

    private PatcherService.PatcherInformation mInfo;
    private boolean mGettingPatcherInformation = false;
    private boolean mShowingProgress = false;

    private String mZipFile;

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Bundle bundle = intent.getExtras();

            if (bundle != null) {
                String state = bundle.getString(PatcherService.STATE);

                if (state.equals(PatcherService.STATE_FETCHED_PATCHER_INFO)) {
                    mInfo = bundle.getParcelable("info");

                    populateSpinner();

                    mChooseFileButton.setEnabled(true);
                    mPartConfigSpinner.setEnabled(true);
                    mShowingProgress = false;
                    updateProgress();

                    mGettingPatcherInformation = false;
                }
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        CardBackground backgroundButton = new CardBackground(getResources()
                .getDisplayMetrics());
        CardBackground backgroundPartConfig = new CardBackground(getResources()
                .getDisplayMetrics());
        CardBackground backgroundNoCard = new CardBackground(getResources()
                .getDisplayMetrics());
        backgroundNoCard.setNoCard(true);

        backgroundButton.widgetOnTop();
        backgroundPartConfig.widgetOnBottom();

        ((LinearLayout) findViewById(R.id.button_container))
                .setBackground(backgroundButton);
        ((LinearLayout) findViewById(R.id.part_config_container))
                .setBackground(backgroundPartConfig);
        ((LinearLayout) findViewById(R.id.main_layout))
                .setBackground(backgroundNoCard);

        mPartConfigSpinner = (Spinner) findViewById(R.id.choose_partition_config);

        // Include version in action bar
        try {
            String version = getPackageManager().getPackageInfo(
                    getPackageName(), 0).versionName;
            setTitle(getTitle() + " (v" + version + ")");
        } catch (NameNotFoundException e) {
            e.printStackTrace();
        }

        TextView partConfigDesc = (TextView) findViewById(R.id.partition_config_desc);
        partConfigDesc.setText(mPartConfigText);

        mChooseFileButton = (Button) findViewById(R.id.choose_file);
        mChooseFileButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent fileIntent = new Intent(Intent.ACTION_GET_CONTENT);
                if (Build.VERSION.SDK_INT < 19) {
                    // Intent fileIntent = new
                    // Intent(Intent.ACTION_OPEN_DOCUMENT);
                    fileIntent.addCategory(Intent.CATEGORY_OPENABLE);
                }
                fileIntent.setType("application/zip");
                startActivityForResult(fileIntent, REQUEST_FILE);
            }
        });
    }

    @Override
    public void onPostCreate(Bundle savedInstanceState) {
        super.onPostCreate(savedInstanceState);

        // Update patcher
        if (!mGettingPatcherInformation && mInfo == null) {
            mGettingPatcherInformation = true;

            mShowingProgress = true;
            updateProgress();

            mChooseFileButton.setEnabled(false);
            mPartConfigSpinner.setEnabled(false);

            Intent intent = new Intent(this, PatcherService.class);
            intent.putExtra(PatcherService.ACTION,
                    PatcherService.ACTION_GET_PATCHER_INFORMATION);
            startService(intent);
        } else {
            updateProgress();
            populateSpinner();
        }

        mPartConfigSpinner.setSelection(mPartConfigSpinnerPos);
    }

    @Override
    protected void onDestroy() {
        mChooseFileButton = null;
        mPartConfigSpinner = null;

        super.onDestroy();
    }

    @Override
    public void onResume() {
        super.onResume();
        registerReceiver(mReceiver, new IntentFilter(
                PatcherService.BROADCAST_INTENT));
    }

    @Override
    public void onPause() {
        super.onPause();
        unregisterReceiver(mReceiver);
    }

    @Override
    public void onSaveInstanceState(Bundle savedInstanceState) {
        super.onSaveInstanceState(savedInstanceState);
        savedInstanceState.putParcelable("info", mInfo);
        savedInstanceState.putInt("partConfigPos", mPartConfigSpinnerPos);
    }

    @Override
    public void onRestoreInstanceState(Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);
        mInfo = savedInstanceState.getParcelable("info");
        mPartConfigSpinnerPos = savedInstanceState.getInt("partConfigPos");
    }

    @Override
    protected void onActivityResult(int request, int result, Intent data) {
        switch (request) {
        case REQUEST_FILE:
            if (data != null && result == RESULT_OK) {
                mZipFile = getPathFromUri(data.getData());

                /* Show alert dialog */
                FragmentTransaction ft = getFragmentManager()
                        .beginTransaction();
                Fragment prev = getFragmentManager().findFragmentByTag(
                        AlertDialogFragment.TAG);

                if (prev != null) {
                    ft.remove(prev);
                }

                AlertDialogFragment f = AlertDialogFragment.newInstance(
                        getString(R.string.dialog_patch_zip_title),
                        getString(R.string.dialog_patch_zip_msg) + "\n\n"
                                + mZipFile, getString(R.string.dialog_cancel),
                        new CancelDialog(),
                        getString(R.string.dialog_continue),
                        new ConfirmDialogPositive());
                f.show(ft, AlertDialogFragment.TAG);
            }
            break;

        case REQUEST_PATCH_FILE:
            if (result == RESULT_OK) {
                boolean failed = data.getExtras().getBoolean("failed");
                String message = data.getExtras().getString("message");
                String newZipFile = data.getExtras().getString("newZipFile");

                if (!failed) {
                    message = getString(R.string.dialog_text_new_file)
                            + newZipFile + "\n\n" + message;
                }

                /* Show alert dialog */
                FragmentTransaction ft = getFragmentManager()
                        .beginTransaction();
                Fragment prev = getFragmentManager().findFragmentByTag(
                        AlertDialogFragment.TAG);

                if (prev != null) {
                    ft.remove(prev);
                }

                String title = "";

                if (failed) {
                    title = getString(R.string.dialog_patch_zip_title_failed);
                } else {
                    title = getString(R.string.dialog_patch_zip_title_success);
                }

                AlertDialogFragment f = AlertDialogFragment.newInstance(title,
                        message, "", null, getString(R.string.dialog_finish),
                        new DismissDialog());
                f.show(ft, AlertDialogFragment.TAG);

                mChooseFileButton.setEnabled(true);
                mPartConfigSpinner.setEnabled(true);
            }
            break;
        }

        super.onActivityResult(request, result, data);
    }

    private void updateProgress() {
        if (mShowingProgress) {
            findViewById(R.id.extracting_progress).setVisibility(View.VISIBLE);
            findViewById(R.id.choose_partition_config).setVisibility(View.GONE);
            findViewById(R.id.partition_config_desc).setVisibility(View.GONE);
        } else {
            findViewById(R.id.extracting_progress).setVisibility(View.GONE);
            findViewById(R.id.choose_partition_config).setVisibility(
                    View.VISIBLE);
            findViewById(R.id.partition_config_desc)
                    .setVisibility(View.VISIBLE);
        }
    }

    private void populateSpinner() {
        ArrayAdapter<String> sa = new ArrayAdapter<String>(MainActivity.this,
                android.R.layout.simple_spinner_item, android.R.id.text1);
        sa.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mPartConfigSpinner.setAdapter(sa);

        for (int i = 0; i < mInfo.mPartitionConfigs.length; i++) {
            sa.add(mInfo.mPartitionConfigs[i].mName);
        }
        sa.notifyDataSetChanged();
        mPartConfigSpinner.setSelection(mPartConfigSpinnerPos);

        mPartConfigSpinner
                .setOnItemSelectedListener(new OnItemSelectedListener() {
                    @Override
                    public void onItemSelected(AdapterView<?> parent,
                            View view, int position, long id) {
                        mPartConfigSpinnerPos = position;
                        mPartConfigText = mInfo.mPartitionConfigs[position].mDesc;
                        TextView t = (TextView) findViewById(R.id.partition_config_desc);
                        t.setText(mPartConfigText);
                    }

                    @Override
                    public void onNothingSelected(AdapterView<?> parent) {
                    }
                });
    }

    private String getPathFromUri(Uri uri) {
        if (Build.VERSION.SDK_INT >= 19
                && "com.android.externalstorage.documents".equals(uri
                        .getAuthority())) {
            return getPathFromDocumentsUri(uri);
        } else if (Build.VERSION.SDK_INT >= 19
                && "com.android.providers.downloads.documents".equals(uri
                        .getAuthority())) {
            return getPathFromDownloadsUri(uri);
        } else {
            return uri.getPath();
        }
    }

    @SuppressLint("NewApi")
    private String getPathFromDocumentsUri(Uri uri) {
        // Based on
        // frameworks/base/packages/ExternalStorageProvider/src/com/android/externalstorage/ExternalStorageProvider.java
        StorageManager sm = (StorageManager) getSystemService(Context.STORAGE_SERVICE);

        try {
            Class StorageManager = sm.getClass();
            Method getVolumeList = StorageManager
                    .getDeclaredMethod("getVolumeList");
            Object[] vols = (Object[]) getVolumeList
                    .invoke(sm, new Object[] {});

            Class StorageVolume = Class
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
    private String getPathFromDownloadsUri(Uri uri) {
        // Based on
        // https://github.com/iPaulPro/aFileChooser/blob/master/aFileChooser/src/com/ipaulpro/afilechooser/utils/FileUtils.java
        String id = DocumentsContract.getDocumentId(uri);
        Uri contentUri = ContentUris.withAppendedId(
                Uri.parse("content://downloads/public_downloads"),
                Long.valueOf(id));

        Cursor cursor = null;
        String[] projection = { "_data" };

        try {
            cursor = getContentResolver().query(contentUri, projection, null,
                    null, null);
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

    public class CancelDialog implements DialogInterface.OnClickListener {
        @Override
        public void onClick(DialogInterface dialog, int which) {
            Fragment prev = getFragmentManager().findFragmentByTag(
                    AlertDialogFragment.TAG);
            ((AlertDialogFragment) prev).cancel();
        }
    }

    public class DismissDialog implements DialogInterface.OnClickListener {
        @Override
        public void onClick(DialogInterface dialog, int which) {
            Fragment prev = getFragmentManager().findFragmentByTag(
                    AlertDialogFragment.TAG);
            ((AlertDialogFragment) prev).dismiss();
        }
    }

    public class ConfirmDialogPositive implements
            DialogInterface.OnClickListener {
        @Override
        public void onClick(DialogInterface dialog, int which) {
            ((Button) findViewById(R.id.choose_file)).setEnabled(false);
            ((Spinner) findViewById(R.id.choose_partition_config))
                    .setEnabled(false);
            // mChooseFileButton.setEnabled(false);
            // mPartConfigSpinner.setEnabled(false);

            Intent i = new Intent(PATCH_FILE);
            i.addCategory(Intent.CATEGORY_DEFAULT);
            i.putExtra("zipFile", mZipFile);
            i.putExtra("partConfig",
                    mInfo.mPartitionConfigs[mPartConfigSpinnerPos].mId);
            startActivityForResult(i, REQUEST_PATCH_FILE);
        }
    }
}
