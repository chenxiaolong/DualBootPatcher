package com.github.chenxiaolong.dualbootpatcher;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.lang.ref.WeakReference;
import java.util.Arrays;

import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.widget.Button;

public class SharedState {
	public static WeakReference<MainActivity> mActivity = null;

	public static final int REQUEST_FILE = 1234;
	public static String mHandle;

	public static boolean mPatcherFailed = false;
	public static String mPatcherExitMsg = "";

	// Buttons
	public static boolean mPatchFileVisible = false;

	// File
	public static String zipFile = "";

	public static String mPatcherFileBase = "DualBootPatcherAndroid-";
	public static String mPatcherFileName = "";
	public static String mPatcherFileVer = "";
	//public static String mPatcherFileExt = ".zip";
	public static String mPatcherFileExt = ".tar.xz";

	public static BufferedReader stdout = null;
	public static BufferedReader stderr = null;

	// Dialogs
    public static ProgressDialog mProgressDialog = null;
    public static boolean mProgressDialogVisible = false;
    public static String mProgressDialogText = "";
    public static String mProgressDialogTitle = "";

	public static AlertDialog.Builder mConfirmDialogBuilder = null;
	public static AlertDialog mConfirmDialog = null;
	public static boolean mConfirmDialogVisible = false;
	public static String mConfirmDialogTitle = "";
	public static String mConfirmDialogText = "";
	public static DialogInterface.OnClickListener mConfirmDialogNegative = null;
	public static DialogInterface.OnClickListener mConfirmDialogPositive = null;
	public static String mConfirmDialogNegativeText = "";
	public static String mConfirmDialogPositiveText = "";

	public static class ConfirmDialogNegative implements DialogInterface.OnClickListener {
		@Override
		public void onClick(DialogInterface dialog, int which) {
			mConfirmDialogVisible = false;
			SharedState.mConfirmDialog.cancel();
		}
	}

	public static class ConfirmDialogPositive implements DialogInterface.OnClickListener {
		@Override
		public void onClick(DialogInterface dialog, int which) {
			mConfirmDialogVisible = false;
			((Button)mActivity.get().findViewById(R.id.choose_file)).setEnabled(false);

			/* Show progress dialog */
	        mProgressDialogTitle = mActivity.get().getString(R.string.progress_title_patching_files);
	        mProgressDialogText = mActivity.get().getString(R.string.progress_text);
	        mProgressDialog.setTitle(mProgressDialogTitle);
	        mProgressDialog.setMessage(mProgressDialogText);
	        mProgressDialog.show();
	        mProgressDialogVisible = true;

			new Thread() {
				@Override
                public void run() {
					File tar = new File(mActivity.get().getCacheDir() + "/tar");
					File target = new File(mActivity.get().getCacheDir() + "/" + mPatcherFileName);
					File targetDir = new File(mActivity.get().getFilesDir() + "/" +
							mPatcherFileBase + mPatcherFileVer.replace("-DEBUG", ""));

					/* Remove temporary files in case the script crashes and
					 * doesn't clean itself up properly */
					mHandler.obtainMessage(
							EVENT_UPDATE_PROGRESS_TITLE,
							mActivity.get().getString(R.string.progress_title_removing_temp))
							.sendToTarget();

					run_command(new String[] {
							"sh", "-c",
							"rm -rf " + mActivity.get().getFilesDir() + "/*/tmp*" + " " +
							mActivity.get().getCacheDir() + "/*"
					}, null, null);

					if (!targetDir.exists()) {
						mHandler.obtainMessage(
								EVENT_UPDATE_PROGRESS_TITLE,
								mActivity.get().getString(R.string.progress_title_updating_files))
								.sendToTarget();

						extract_asset("tar", tar);
						extract_asset(mPatcherFileName, target);

						/* Remove all previous files */
						run_command(new String[] {
								"sh", "-c",
								"rm -rf " + mActivity.get().getFilesDir() + "/*"
						}, null, null);

						run_command(new String[] {
				                "chmod", "755",
				                tar.getPath()
				        }, null, null);

						run_command(new String[] {
								tar.getPath(),
								"-J", "-x", "-v", "-f",
								target.getPath()
						}, null, mActivity.get().getFilesDir());

						run_command(new String[] {
								"chmod", "755",
								"pythonportable/bin/python"
						}, null, targetDir);

						/* Delete tar.xz */
						target.delete();
						tar.delete();
					}

					mHandler.obtainMessage(
							EVENT_UPDATE_PROGRESS_TITLE,
							mActivity.get().getString(R.string.progress_title_patching_files))
							.sendToTarget();

					int exit_code = run_command(new String[] {
							"pythonportable/bin/python", "-B",
							"scripts/patchfile.py", SharedState.zipFile
					}, new String[] {
					        "LD_LIBRARY_PATH=pythonportable/lib",
                            "PYTHONUNBUFFERED=true",
                            "TMPDIR=" + mActivity.get().getCacheDir()
					}, targetDir);

					mHandler.obtainMessage(EVENT_CLOSE_PROGRESS_DIALOG).sendToTarget();
					mHandler.obtainMessage(EVENT_ENABLE_CHOOSE_FILE_BUTTON).sendToTarget();

					if (exit_code == 0 && !mPatcherFailed) {
						// TODO: Don't hardcode this
						String newFile = zipFile.replace(".zip", "_dualboot.zip");

						String msg = mActivity.get().getString(R.string.dialog_text_new_file) +
						        newFile;

						if (!mPatcherExitMsg.equals("")) {
						    msg = msg + "\n\n" + mPatcherExitMsg;
						}

						mHandler.obtainMessage(EVENT_SHOW_COMPLETION_DIALOG, msg).sendToTarget();
					}
					else {
					    String msg = mActivity.get().getString(R.string.dialog_text_file) +
					            zipFile;

					    if (!mPatcherExitMsg.equals("")) {
					        msg = msg + "\n\nError: " + mPatcherExitMsg;
                        }

						mHandler.obtainMessage(EVENT_SHOW_FAILED_DIALOG, msg).sendToTarget();
					}
				}
			}.start();
		}
	}

	public static final int EVENT_ENABLE_CHOOSE_FILE_BUTTON = 1;
	public static final int EVENT_CLOSE_PROGRESS_DIALOG = 2;
	public static final int EVENT_UPDATE_PROGRESS_MSG = 3;
	public static final int EVENT_UPDATE_PROGRESS_TITLE = 4;
	public static final int EVENT_SHOW_COMPLETION_DIALOG = 5;
	public static final int EVENT_SHOW_FAILED_DIALOG = 6;

	public static final Handler mHandler = new Handler(Looper.getMainLooper()) {
		@Override
        public void handleMessage(Message msg) {
			switch (msg.what) {
			case EVENT_UPDATE_PROGRESS_MSG:
				mProgressDialogText = (String)msg.obj;
				mProgressDialog.setMessage(mProgressDialogText);
				break;

			case EVENT_UPDATE_PROGRESS_TITLE:
				mProgressDialogTitle = (String)msg.obj;
				mProgressDialog.setTitle(mProgressDialogTitle);
				break;

			case EVENT_SHOW_COMPLETION_DIALOG:
				mConfirmDialogTitle = mActivity.get().getString(
						R.string.dialog_patch_zip_title_success);
				mConfirmDialogText = (String)msg.obj;

				mConfirmDialogBuilder.setTitle(mConfirmDialogTitle);
				mConfirmDialogBuilder.setMessage(mConfirmDialogText);

				mConfirmDialogNegative = null;
    			mConfirmDialogPositive = new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						mConfirmDialogVisible = false;
						mConfirmDialog.dismiss();
					}
				};
				mConfirmDialogNegativeText = null;
    			mConfirmDialogPositiveText =
    					mActivity.get().getString(R.string.dialog_finish);

        		mConfirmDialogBuilder.setNegativeButton(
        				mConfirmDialogNegativeText,
        				mConfirmDialogNegative);
        		mConfirmDialogBuilder.setPositiveButton(
        				mConfirmDialogPositiveText,
        				mConfirmDialogPositive);

        		mConfirmDialog = mConfirmDialogBuilder.create();

        		mConfirmDialog.show();
        		mConfirmDialogVisible = true;
        		break;

			case EVENT_SHOW_FAILED_DIALOG:
				mConfirmDialogTitle = mActivity.get().getString(
						R.string.dialog_patch_zip_title_failed);
				mConfirmDialogText = (String)msg.obj;

				mConfirmDialogBuilder.setTitle(mConfirmDialogTitle);
				mConfirmDialogBuilder.setMessage(mConfirmDialogText);

				mConfirmDialogNegative = null;
    			mConfirmDialogPositive = new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						mConfirmDialogVisible = false;
						mConfirmDialog.dismiss();
					}
				};
				mConfirmDialogNegativeText = null;
    			mConfirmDialogPositiveText =
    					mActivity.get().getString(R.string.dialog_finish);

        		mConfirmDialogBuilder.setNegativeButton(
        				mConfirmDialogNegativeText,
        				mConfirmDialogNegative);
        		mConfirmDialogBuilder.setPositiveButton(
        				mConfirmDialogPositiveText,
        				mConfirmDialogPositive);

        		mConfirmDialog = mConfirmDialogBuilder.create();

        		mConfirmDialog.show();
        		mConfirmDialogVisible = true;
        		break;

			case EVENT_ENABLE_CHOOSE_FILE_BUTTON:
				((Button)mActivity.get().findViewById(R.id.choose_file)).setEnabled(true);
				break;

			case EVENT_CLOSE_PROGRESS_DIALOG:
		        mProgressDialogVisible = false;
		        mProgressDialog.dismiss();
		        break;

			default:
				super.handleMessage(msg);
				break;
			}
		}
	};

	public static int run_command(String[] command, String[] environment, File cwd) {
	    final String TAG = "DualBootPatcher";
		try {
		    Log.e(TAG, "COMMAND: " + Arrays.toString(command));
		    if (environment != null) {
		        Log.e(TAG, "ENVIRONMENT: " + Arrays.toString(environment));
		    }
		    else {
		        Log.e(TAG, "ENVIRONMENT: Inherited");
		    }
		    if (cwd != null) {
		        Log.e(TAG, "CWD: " + cwd);
		    }
		    else {
		        Log.e(TAG, "CWD: Inherited");
		    }

			ProcessBuilder pb = new ProcessBuilder(Arrays.asList(command));
			if (environment != null) {
				for (String s : environment) {
					String[] split = s.split("=");
					pb.environment().put(split[0], split[1]);
				}
			}
			if (cwd != null) {
				pb.directory(cwd);
			}
			Process p = pb.start();
			//Process p = Runtime.getRuntime().exec(command, environment, cwd);
	        stdout = new BufferedReader(new InputStreamReader(p.getInputStream()));
            stderr = new BufferedReader(new InputStreamReader(p.getErrorStream()));

            // Read stdout and stderr at the same time
            Thread stdout_reader = new Thread() {
				@Override
                public void run() {
					String s;
					try {
						while ((s = stdout.readLine()) != null) {
							Log.e(TAG, "STDOUT: " + s);
							Message updateOutput = mHandler.obtainMessage(EVENT_UPDATE_PROGRESS_MSG, s);
							updateOutput.sendToTarget();
						}
					} catch (IOException e) {
						e.printStackTrace();
					}
				}
			};

			Thread stderr_reader = new Thread() {
				@Override
                public void run() {
				    mPatcherExitMsg = "";
				    mPatcherFailed = false;

					String s;
					try {
						while ((s = stderr.readLine()) != null) {
						    Log.e(TAG, "STDERR: " + s);
						    if (s.contains("EXITFAIL:")) {
						        mPatcherExitMsg = s.replace("EXITFAIL:", "");
						        mPatcherFailed = true;
						    }
						    else if (s.contains("EXITSUCCESS:")) {
						        mPatcherExitMsg = s.replace("EXITSUCCESS:", "");
						        mPatcherFailed = false;
						    }
						    // Don't send stderr stuff to the GUI
							//Message updateOutput = mHandler.obtainMessage(EVENT_UPDATE_PROGRESS_MSG, s);
							//updateOutput.sendToTarget();
						}
					} catch (IOException e) {
						e.printStackTrace();
					}
				}
			};

			stdout_reader.start();
			stderr_reader.start();
			stdout_reader.join();
			stderr_reader.join();
			p.waitFor();
			return p.exitValue();
		} catch (IOException e) {
			e.printStackTrace();
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		return -1;
	}

	public static void extract_asset(String src, File dest) {
		try {
			InputStream i = mActivity.get().getAssets().open(src);
			FileOutputStream o = new FileOutputStream(dest);

			//byte[] buffer = new byte[4096];
			//int length;
			//while ((length = i.read(buffer)) > 0) {
	        //    o.write(buffer, 0, length);
	        //}

			/* The files are small enough to just read all of it into memory */
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
}
