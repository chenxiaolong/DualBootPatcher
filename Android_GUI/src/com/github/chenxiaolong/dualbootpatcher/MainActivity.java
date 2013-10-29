package com.github.chenxiaolong.dualbootpatcher;

import java.lang.ref.WeakReference;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Intent;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;

public class MainActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        SharedState.mActivity = new WeakReference<MainActivity>(this);

        Button chooseFileButton = (Button)findViewById(R.id.choose_file);
        chooseFileButton.setOnClickListener(new OnClickListener() {
        	@Override
            public void onClick(View v) {
        		Intent fileIntent = new Intent(Intent.ACTION_GET_CONTENT);
        		fileIntent.setType("file/*");
        		startActivityForResult(fileIntent, SharedState.REQUEST_FILE);
        	}
        });

        if (SharedState.mPatcherFileVer.equals("")) {
        	try {
				SharedState.mPatcherFileVer =
						getPackageManager().getPackageInfo(getPackageName(), 0).versionName;
				SharedState.mPatcherFileName =
						SharedState.mPatcherFileBase +
						SharedState.mPatcherFileVer +
						SharedState.mPatcherFileExt;
			} catch (NameNotFoundException e) {
				e.printStackTrace();
			}
        }

        /* Include version in action bar */
        setTitle(getTitle() + " (v" + SharedState.mPatcherFileVer + ")");

        SharedState.mProgressDialog = new ProgressDialog(this);
        SharedState.mProgressDialog.setCancelable(false);
        SharedState.mProgressDialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
        SharedState.mProgressDialog.setIcon(0);

		SharedState.mConfirmDialogBuilder = new AlertDialog.Builder(this);
		SharedState.mConfirmDialogBuilder.setNegativeButton(
				SharedState.mConfirmDialogNegativeText,
				SharedState.mConfirmDialogNegative);
		SharedState.mConfirmDialogBuilder.setPositiveButton(
				SharedState.mConfirmDialogPositiveText,
				SharedState.mConfirmDialogPositive);

		tryShowDialogs();
    }

    @Override
    protected void onDestroy() {
    	super.onDestroy();
        if (SharedState.mProgressDialog != null) {
            SharedState.mProgressDialog.dismiss();
            SharedState.mProgressDialog = null;
        }
    	if (SharedState.mConfirmDialog != null) {
    		SharedState.mConfirmDialog.dismiss();
    		SharedState.mConfirmDialog = null;
    	}
    	if (SharedState.mConfirmDialogBuilder != null) {
    		SharedState.mConfirmDialogBuilder = null;
    	}
    	SharedState.mActivity = null;
    }

    @Override
    protected void onActivityResult(int request, int result, Intent data) {
    	switch (request) {
    	case SharedState.REQUEST_FILE:
            if (data != null && result == RESULT_OK) {
                SharedState.zipFile = data.getData();

                SharedState.mConfirmDialogNegative = new SharedState.ConfirmDialogNegative();
    			SharedState.mConfirmDialogPositive = new SharedState.ConfirmDialogPositive();
    			SharedState.mConfirmDialogNegativeText = getString(R.string.dialog_cancel);
    			SharedState.mConfirmDialogPositiveText = getString(R.string.dialog_continue);
        		SharedState.mConfirmDialogBuilder.setNegativeButton(
        				SharedState.mConfirmDialogNegativeText,
        				SharedState.mConfirmDialogNegative);
        		SharedState.mConfirmDialogBuilder.setPositiveButton(
        				SharedState.mConfirmDialogPositiveText,
        				SharedState.mConfirmDialogPositive);

            	SharedState.mConfirmDialogTitle =
            			getString(R.string.dialog_patch_zip_title);
            	SharedState.mConfirmDialogText =
            			getString(R.string.dialog_patch_zip_msg)
            			+ "\n\n" + SharedState.zipFile.getPath();
        		SharedState.mConfirmDialogVisible = true;
        		tryShowDialogs();
            }
            break;

    	case SharedState.REQUEST_WINDOW_HANDLE:
    		if (data != null && result == RESULT_OK) {
    			SharedState.mHandle =
    					data.getStringExtra("jackpal.androidterm.window_handle");
    		}
    		break;
    	}

    	super.onActivityResult(request, result, data);
    }

    private void tryShowDialogs() {
    	if (SharedState.mProgressDialogVisible) {
            SharedState.mProgressDialog.setMessage(SharedState.mProgressDialogText);
            SharedState.mProgressDialog.setTitle(SharedState.mProgressDialogTitle);
            SharedState.mProgressDialog.show();
        }
		if (SharedState.mConfirmDialogVisible) {
			SharedState.mConfirmDialogBuilder.setTitle(SharedState.mConfirmDialogTitle);
			SharedState.mConfirmDialogBuilder.setMessage(SharedState.mConfirmDialogText);
			SharedState.mConfirmDialog = SharedState.mConfirmDialogBuilder.create();
			SharedState.mConfirmDialog.show();
		}
    }
}
