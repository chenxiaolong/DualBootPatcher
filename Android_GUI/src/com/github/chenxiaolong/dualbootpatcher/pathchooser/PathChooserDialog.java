/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher.pathchooser;

import android.app.Dialog;
import android.os.Bundle;
import android.os.Environment;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.Fragment;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.text.Editable;
import android.text.InputType;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.View;
import android.webkit.MimeTypeMap;
import android.widget.EditText;

import com.afollestad.materialdialogs.DialogAction;
import com.afollestad.materialdialogs.MaterialDialog;
import com.afollestad.materialdialogs.MaterialDialog.SingleButtonCallback;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.dialogs.DialogListenerTarget;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericInputDialog;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericInputDialog
        .GenericInputDialogListener;
import com.github.chenxiaolong.dualbootpatcher.pathchooser.PathChooserItemAdapter
        .PathChooserItemClickListener;

import org.apache.commons.io.FilenameUtils;

import java.io.File;
import java.io.Serializable;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

public class PathChooserDialog extends DialogFragment implements SingleButtonCallback,
        PathChooserItemClickListener, GenericInputDialogListener {
    private static final String DIALOG_NEW_FOLDER =
            PathChooserDialog.class.getCanonicalName() + ".dialog.new_folder";

    private static final String EMULATED_STORAGE_DIR = "/storage/emulated";

    private static final String ARG_BUILDER = "builder";
    private static final String ARG_TARGET = "target";
    private static final String ARG_TAG = "tag";

    private static final String EXTRA_CWD = "cwd";

    private static final Comparator<File> PATH_COMPARATOR = new DirectoryFirstNameComparator();
    private static final Comparator<File> DIRECTORY_COMPARATOR = new DirectoryComparator();

    private Builder mBuilder;
    private DialogListenerTarget mTarget;
    private String mTag;

    private EditText mEditText;

    private Comparator<File> mComparator;
    private PathChooserItemAdapter mAdapter;
    private File mCwd;
    private List<File> mContents = new ArrayList<>();
    private List<String> mNames = new ArrayList<>();
    private boolean mHasParent = true;

    interface FileChooserDialogListener {
        void onPathSelected(@Nullable String tag, @NonNull File file);

        void onPathSelectionCancelled(@Nullable String tag);
    }

    @SuppressWarnings("ConstantConditions")
    @NonNull
    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        Bundle args = getArguments();
        mBuilder = (Builder) args.getSerializable(ARG_BUILDER);
        mTarget = (DialogListenerTarget) args.getSerializable(ARG_TARGET);
        mTag = args.getString(ARG_TAG);

        switch (mBuilder.mType) {
        case OPEN_DIRECTORY:
            mComparator = DIRECTORY_COMPARATOR;
            break;
        case OPEN_FILE:
        case SAVE_FILE:
            mComparator = PATH_COMPARATOR;
            break;
        }

        if (savedInstanceState != null) {
            mCwd = new File(savedInstanceState.getString(EXTRA_CWD));
        } else {
            mCwd = new File(mBuilder.mInitialPath);
        }

        mAdapter = new PathChooserItemAdapter(mNames, this);
        refreshContents();

        MaterialDialog.Builder builder = new MaterialDialog.Builder(getActivity());
        builder.title(mCwd.getAbsolutePath());
        builder.customView(R.layout.dialog_path_chooser, false);
        builder.onNegative(this);
        builder.autoDismiss(false);
        builder.negativeText(R.string.cancel);

        if (mBuilder.mType == Type.OPEN_DIRECTORY) {
            builder.onPositive(this);
            builder.positiveText(R.string.path_chooser_choose_label);
        } else if (mBuilder.mType == Type.SAVE_FILE) {
            builder.onPositive(this);
            builder.positiveText(R.string.path_chooser_save_label);
            builder.onNeutral(this);
            builder.neutralText(R.string.path_chooser_new_folder_label);
        }

        MaterialDialog dialog = builder.build();
        View view = dialog.getView();

        RecyclerView rv = (RecyclerView) view.findViewById(R.id.files);
        mEditText = (EditText) view.findViewById(R.id.filename);

        if (mBuilder.mType == Type.SAVE_FILE) {
            mEditText.addTextChangedListener(new NameValidationTextWatcher(dialog));
            mEditText.setText(mBuilder.mDefaultName);
        } else {
            mEditText.setVisibility(View.GONE);
        }

        rv.setHasFixedSize(true);
        rv.setAdapter(mAdapter);

        LinearLayoutManager llm = new LinearLayoutManager(getActivity());
        llm.setOrientation(LinearLayoutManager.VERTICAL);
        rv.setLayoutManager(llm);

        return dialog;
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putString(EXTRA_CWD, mCwd.getAbsolutePath());
    }

    @Override
    public void onPathChooserItemClicked(int position, String item) {
        File selectedPath = mCwd;

        if (mHasParent && position == 0) {
            selectedPath = selectedPath.getParentFile();
            if (EMULATED_STORAGE_DIR.equals(selectedPath.getAbsolutePath())) {
                selectedPath = selectedPath.getParentFile();
            }
        } else {
            selectedPath = mContents.get(mHasParent ? position - 1 : position);
            if (EMULATED_STORAGE_DIR.equals(selectedPath.getAbsolutePath())) {
                selectedPath = Environment.getExternalStorageDirectory();
            }
        }

        if (selectedPath.isFile()) {
            switch (mBuilder.mType) {
            case OPEN_FILE:
                getOwner().onPathSelected(mTag, selectedPath);
                dismiss();
                break;
            case SAVE_FILE:
                mEditText.setText(selectedPath.getName());
                break;
            }
        } else {
            mCwd = selectedPath;
            mHasParent = mCwd.getParent() != null;

            refreshContents();

            MaterialDialog dialog = (MaterialDialog) getDialog();
            dialog.setTitle(mCwd.getAbsolutePath());
        }
    }

    @Override
    public void onClick(@NonNull MaterialDialog dialog, @NonNull DialogAction which) {
        switch (which) {
        case POSITIVE:
            if (mBuilder.mType == Type.OPEN_DIRECTORY) {
                getOwner().onPathSelected(mTag, mCwd);
                dialog.dismiss();
            } else if (mBuilder.mType == Type.SAVE_FILE) {
                getOwner().onPathSelected(mTag, new File(mCwd, mEditText.getText().toString()));
                dialog.dismiss();
            }
            break;
        case NEGATIVE:
            getOwner().onPathSelectionCancelled(mTag);
            dialog.dismiss();
            break;
        case NEUTRAL:
            if (mBuilder.mType == Type.SAVE_FILE) {
                showNewFolderDialog();
            }
            break;
        }
    }

    @Override
    public void onConfirmInput(@Nullable String tag, String text) {
        if (DIALOG_NEW_FOLDER.equals(tag)) {
            File newDirectory = new File(mCwd, text);
            //noinspection ResultOfMethodCallIgnored
            newDirectory.mkdir();
            refreshContents();
        }
    }

    @NonNull
    private FileChooserDialogListener getOwner() {
        switch (mTarget) {
        case ACTIVITY:
            return (FileChooserDialogListener) getActivity();
        case FRAGMENT:
            return (FileChooserDialogListener) getTargetFragment();
        case NONE:
        default:
            throw new IllegalStateException("File chooser dialog without a listener is useless");
        }
    }

    private void showNewFolderDialog() {
        GenericInputDialog.Builder builder = new GenericInputDialog.Builder();
        builder.title(R.string.path_chooser_new_folder_label);
        builder.negative(R.string.cancel);
        builder.positive(R.string.ok);
        builder.allowEmpty(false);
        builder.inputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);

        GenericInputDialog dialog = builder.buildFromFragment(DIALOG_NEW_FOLDER, this);
        dialog.show(getFragmentManager(), DIALOG_NEW_FOLDER);
    }

    private void refreshContents() {
        File[] contents = listFilesWithFilter(mCwd, mBuilder.mMimeType, mComparator);

        mContents.clear();
        if (contents != null) {
            for (File file : contents) {
                // Only add directories if choosing a directory. Otherwise, add everything
                if (file.isDirectory() || mBuilder.mType != Type.OPEN_DIRECTORY) {
                    mContents.add(file);
                }
            }
        }

        mNames.clear();
        if (mHasParent) {
            mNames.add("..");
        }
        for (File file : mContents) {
            if (file.isDirectory()) {
                mNames.add(file.getName() + '/');
            } else {
                mNames.add(file.getName());
            }
        }

        mAdapter.notifyDataSetChanged();
    }

    @Nullable
    private static File[] listFilesWithFilter(@NonNull File directory, @Nullable String mimeType,
                                              @NonNull Comparator<File> comparator) {
        File[] contents = directory.listFiles();
        ArrayList<File> results = new ArrayList<>();

        if (contents != null) {
            MimeTypeMap mimeTypeMap = MimeTypeMap.getSingleton();
            for (File file : contents) {
                if (file.isDirectory() || isMimeType(file, mimeType, mimeTypeMap)) {
                    results.add(file);
                }
            }

            Collections.sort(results, comparator);
            return results.toArray(new File[results.size()]);
        }

        return null;
    }

    private static boolean mimeMatches(@Nullable String filter, @Nullable String test) {
        return test != null
                && (filter == null || "*/*".equals(filter) || filter.equals(test)
                        || (filter.endsWith("/*")
                                && filter.regionMatches(0, test, 0, filter.indexOf('/'))));
    }

    private static boolean isMimeType(@NonNull File file, @Nullable String filter,
                                      @NonNull MimeTypeMap mimeTypeMap) {
        if (filter != null) {
            String extension = FilenameUtils.getExtension(file.getName());
            String mimeType = mimeTypeMap.getMimeTypeFromExtension(extension);
            return mimeMatches(filter, mimeType);
        }
        return true;
    }

    enum Type {
        OPEN_FILE,
        OPEN_DIRECTORY,
        SAVE_FILE,
    }

    private static class NameValidationTextWatcher implements TextWatcher {
        private final MaterialDialog mDialog;

        NameValidationTextWatcher(MaterialDialog dialog) {
            mDialog = dialog;
        }

        @Override
        public void beforeTextChanged(CharSequence s, int start, int count, int after) {
        }

        @Override
        public void onTextChanged(CharSequence s, int start, int before, int count) {
            mDialog.getActionButton(DialogAction.POSITIVE).setEnabled(!TextUtils.isEmpty(s));
        }

        @Override
        public void afterTextChanged(Editable s) {
        }
    }

    private static class DirectoryFirstNameComparator implements Comparator<File> {
        @Override
        public int compare(File lhs, File rhs) {
            if (lhs.isDirectory() && !rhs.isDirectory()) {
                return -1;
            } else if (!lhs.isDirectory() && rhs.isDirectory()) {
                return 1;
            } else {
                return lhs.getName().compareTo(rhs.getName());
            }
        }
    }

    private static class DirectoryComparator implements Comparator<File> {
        @Override
        public int compare(File lhs, File rhs) {
            return lhs.getName().compareTo(rhs.getName());
        }
    }

    @SuppressWarnings("unused")
    public static class Builder implements Serializable {
        private static final String DEFAULT_INITIAL_PATH =
                Environment.getExternalStorageDirectory().getAbsolutePath();

        @NonNull
        final Type mType;
        @NonNull
        String mInitialPath = DEFAULT_INITIAL_PATH;
        @Nullable
        String mMimeType;
        @Nullable
        String mDefaultName;

        Builder(@NonNull Type type) {
            mType = type;
        }

        @NonNull
        Builder initialPath(@Nullable String path) {
            if (path == null) {
                mInitialPath = DEFAULT_INITIAL_PATH;
            } else {
                mInitialPath = path;
            }
            return this;
        }

        @NonNull
        Builder mimeType(@Nullable String mimeType) {
            if (mType == Type.OPEN_DIRECTORY) {
                throw new IllegalStateException(
                        "Cannot filter by MIME type when selecting directory");
            }

            mMimeType = mimeType;
            return this;
        }

        @NonNull
        Builder defaultName(@Nullable String defaultName) {
            if (mType != Type.SAVE_FILE) {
                throw new IllegalStateException("Cannot set default name when opening a path");
            }

            mDefaultName = defaultName;
            return this;
        }

        @NonNull
        PathChooserDialog buildFromFragment(@Nullable String tag, @NonNull Fragment parent) {
            Bundle args = new Bundle();
            args.putSerializable(ARG_BUILDER, this);
            args.putSerializable(ARG_TARGET, DialogListenerTarget.FRAGMENT);
            args.putString(ARG_TAG, tag);

            PathChooserDialog dialog = new PathChooserDialog();
            dialog.setTargetFragment(parent, 0);
            dialog.setArguments(args);
            return dialog;
        }

        @NonNull
        PathChooserDialog buildFromActivity(@Nullable String tag) {
            Bundle args = new Bundle();
            args.putSerializable(ARG_BUILDER, this);
            args.putSerializable(ARG_TARGET, DialogListenerTarget.ACTIVITY);
            args.putString(ARG_TAG, tag);

            PathChooserDialog dialog = new PathChooserDialog();
            dialog.setArguments(args);
            return dialog;
        }
    }
}