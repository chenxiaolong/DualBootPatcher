/*
 * Copyright (C) 2016-2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher.pathchooser

import android.app.Dialog
import android.os.Bundle
import android.os.Environment
import android.text.Editable
import android.text.InputType
import android.text.TextUtils
import android.text.TextWatcher
import android.view.View
import android.webkit.MimeTypeMap
import android.widget.EditText
import androidx.fragment.app.DialogFragment
import androidx.fragment.app.Fragment
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.afollestad.materialdialogs.MaterialDialog
import com.afollestad.materialdialogs.WhichButton
import com.afollestad.materialdialogs.actions.setActionButtonEnabled
import com.afollestad.materialdialogs.customview.customView
import com.afollestad.materialdialogs.customview.getCustomView
import com.github.chenxiaolong.dualbootpatcher.R
import com.github.chenxiaolong.dualbootpatcher.dialogs.DialogListenerTarget
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericInputDialog
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericInputDialog.GenericInputDialogListener
import com.github.chenxiaolong.dualbootpatcher.pathchooser.PathChooserItemAdapter.PathChooserItemClickListener
import java.io.File
import java.io.Serializable
import java.util.*

class PathChooserDialog : DialogFragment(), PathChooserItemClickListener,
        GenericInputDialogListener {
    private lateinit var builder: Builder
    private lateinit var target: DialogListenerTarget
    private lateinit var dialogTag: String

    private lateinit var editText: EditText

    private lateinit var comparator: Comparator<File>
    private lateinit var adapter: PathChooserItemAdapter
    private lateinit var cwd: File
    private val contents = ArrayList<File>()
    private val names = ArrayList<String>()
    private var hasParent = true

    private val owner: FileChooserDialogListener
        get() {
            return when (target) {
                DialogListenerTarget.ACTIVITY -> activity as FileChooserDialogListener
                DialogListenerTarget.FRAGMENT -> targetFragment as FileChooserDialogListener
                DialogListenerTarget.NONE ->
                    throw IllegalStateException("File chooser dialog without a listener is useless")
            }
        }

    internal interface FileChooserDialogListener {
        fun onPathSelected(tag: String, file: File)

        fun onPathSelectionCancelled(tag: String)
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val args = arguments
        builder = args!!.getSerializable(ARG_BUILDER) as Builder
        target = args.getSerializable(ARG_TARGET) as DialogListenerTarget
        dialogTag = args.getString(ARG_TAG)!!

        comparator = when (builder.type) {
            PathChooserDialog.Type.OPEN_DIRECTORY -> DIRECTORY_COMPARATOR
            PathChooserDialog.Type.OPEN_FILE, PathChooserDialog.Type.SAVE_FILE -> PATH_COMPARATOR
        }

        cwd = if (savedInstanceState != null) {
            File(savedInstanceState.getString(EXTRA_CWD)!!)
        } else {
            File(builder.initialPath)
        }

        adapter = PathChooserItemAdapter(names, this)
        refreshContents()

        val dialog = MaterialDialog(requireActivity())
                .title(text = cwd.absolutePath)
                .customView(R.layout.dialog_path_chooser, scrollable = false)
                .negativeButton(R.string.cancel) { dialog ->
                    owner.onPathSelectionCancelled(dialogTag)
                    dialog.dismiss()
                }
                .noAutoDismiss()

        if (builder.type == Type.OPEN_DIRECTORY) {
            dialog.positiveButton(R.string.path_chooser_choose_label) {
                owner.onPathSelected(dialogTag, cwd)
                dialog.dismiss()
            }
        } else if (builder.type == Type.SAVE_FILE) {
            dialog.positiveButton(R.string.path_chooser_save_label) {
                owner.onPathSelected(dialogTag, File(cwd, editText.text.toString()))
                dialog.dismiss()
            }
            dialog.neutralButton(R.string.path_chooser_new_folder_label) {
                showNewFolderDialog()
            }
        }

        val view = dialog.getCustomView()

        val rv: RecyclerView = view.findViewById(R.id.files)
        editText = view.findViewById(R.id.filename)

        if (builder.type == Type.SAVE_FILE) {
            editText.addTextChangedListener(NameValidationTextWatcher(dialog))
            editText.setText(builder.defaultName)
        } else {
            editText.visibility = View.GONE
        }

        rv.setHasFixedSize(true)
        rv.adapter = adapter

        val llm = LinearLayoutManager(activity)
        llm.orientation = RecyclerView.VERTICAL
        rv.layoutManager = llm

        return dialog
    }

    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
        outState.putString(EXTRA_CWD, cwd.absolutePath)
    }

    override fun onPathChooserItemClicked(position: Int, item: String) {
        var selectedPath = cwd

        if (hasParent && position == 0) {
            selectedPath = selectedPath.parentFile
            if (EMULATED_STORAGE_DIR == selectedPath.absolutePath) {
                selectedPath = selectedPath.parentFile
            }
        } else {
            selectedPath = contents[if (hasParent) position - 1 else position]
            if (EMULATED_STORAGE_DIR == selectedPath.absolutePath) {
                selectedPath = Environment.getExternalStorageDirectory()
            }
        }

        if (selectedPath.isFile) {
            when (builder.type) {
                PathChooserDialog.Type.OPEN_FILE -> {
                    owner.onPathSelected(dialogTag, selectedPath)
                    dismiss()
                }
                PathChooserDialog.Type.SAVE_FILE -> editText.setText(selectedPath.name)
                PathChooserDialog.Type.OPEN_DIRECTORY -> throw IllegalStateException()
            }
        } else {
            cwd = selectedPath
            hasParent = cwd.parent != null

            refreshContents()

            val dialog = dialog as MaterialDialog
            dialog.setTitle(cwd.absolutePath)
        }
    }

    override fun onConfirmInput(tag: String, text: String) {
        if (DIALOG_NEW_FOLDER == tag) {
            val newDirectory = File(cwd, text)

            newDirectory.mkdir()
            refreshContents()
        }
    }

    private fun showNewFolderDialog() {
        val builder = GenericInputDialog.Builder()
        builder.title(R.string.path_chooser_new_folder_label)
        builder.negative(R.string.cancel)
        builder.positive(R.string.ok)
        builder.allowEmpty(false)
        builder.inputType(InputType.TYPE_CLASS_TEXT or InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS)

        val dialog = builder.buildFromFragment(DIALOG_NEW_FOLDER, this)
        dialog.show(fragmentManager!!, DIALOG_NEW_FOLDER)
    }

    private fun refreshContents() {
        contents.clear()

        listFilesWithFilter(cwd, builder.mimeType, comparator)?.filterTo(contents) {
            // Only add directories if choosing a directory. Otherwise, add everything
            it.isDirectory || builder.type != Type.OPEN_DIRECTORY
        }

        names.clear()
        if (hasParent) {
            names.add("..")
        }
        for (file in contents) {
            if (file.isDirectory) {
                names.add(file.name + '/')
            } else {
                names.add(file.name)
            }
        }

        adapter.notifyDataSetChanged()
    }

    internal enum class Type {
        OPEN_FILE,
        OPEN_DIRECTORY,
        SAVE_FILE
    }

    private class NameValidationTextWatcher
    internal constructor(private val dialog: MaterialDialog) : TextWatcher {
        override fun beforeTextChanged(s: CharSequence, start: Int, count: Int, after: Int) {}

        override fun onTextChanged(s: CharSequence, start: Int, before: Int, count: Int) {
            dialog.setActionButtonEnabled(WhichButton.POSITIVE, !TextUtils.isEmpty(s))
        }

        override fun afterTextChanged(s: Editable) {}
    }

    private class DirectoryFirstNameComparator : Comparator<File> {
        override fun compare(lhs: File, rhs: File): Int {
            return if (lhs.isDirectory && !rhs.isDirectory) {
                -1
            } else if (!lhs.isDirectory && rhs.isDirectory) {
                1
            } else {
                lhs.name.compareTo(rhs.name)
            }
        }
    }

    private class DirectoryComparator : Comparator<File> {
        override fun compare(lhs: File, rhs: File): Int {
            return lhs.name.compareTo(rhs.name)
        }
    }

    class Builder internal constructor(internal val type: Type) : Serializable {
        internal var initialPath = DEFAULT_INITIAL_PATH
        internal var mimeType: String? = null
        internal var defaultName: String? = null

        internal fun initialPath(path: String?): Builder {
            initialPath = path ?: DEFAULT_INITIAL_PATH
            return this
        }

        internal fun mimeType(mimeType: String?): Builder {
            if (type == Type.OPEN_DIRECTORY) {
                throw IllegalStateException(
                        "Cannot filter by MIME type when selecting directory")
            }

            this.mimeType = mimeType
            return this
        }

        internal fun defaultName(defaultName: String?): Builder {
            if (type != Type.SAVE_FILE) {
                throw IllegalStateException("Cannot set default name when opening a path")
            }

            this.defaultName = defaultName
            return this
        }

        internal fun buildFromFragment(tag: String, parent: Fragment): PathChooserDialog {
            val args = Bundle()
            args.putSerializable(ARG_BUILDER, this)
            args.putSerializable(ARG_TARGET, DialogListenerTarget.FRAGMENT)
            args.putString(ARG_TAG, tag)

            val dialog = PathChooserDialog()
            dialog.setTargetFragment(parent, 0)
            dialog.arguments = args
            return dialog
        }

        internal fun buildFromActivity(tag: String): PathChooserDialog {
            val args = Bundle()
            args.putSerializable(ARG_BUILDER, this)
            args.putSerializable(ARG_TARGET, DialogListenerTarget.ACTIVITY)
            args.putString(ARG_TAG, tag)

            val dialog = PathChooserDialog()
            dialog.arguments = args
            return dialog
        }

        companion object {
            private val DEFAULT_INITIAL_PATH =
                    Environment.getExternalStorageDirectory().absolutePath
        }
    }

    companion object {
        private val DIALOG_NEW_FOLDER =
                "${PathChooserDialog::class.java.name}.dialog.new_folder"

        private const val EMULATED_STORAGE_DIR = "/storage/emulated"

        private const val ARG_BUILDER = "builder"
        private const val ARG_TARGET = "target"
        private const val ARG_TAG = "tag"

        private const val EXTRA_CWD = "cwd"

        private val PATH_COMPARATOR = DirectoryFirstNameComparator()
        private val DIRECTORY_COMPARATOR = DirectoryComparator()

        private fun listFilesWithFilter(directory: File, mimeType: String?,
                                        comparator: Comparator<File>): Array<File>? {
            val contents = directory.listFiles()
            if (contents != null) {
                val mimeTypeMap = MimeTypeMap.getSingleton()
                val results = ArrayList<File>()

                contents.filterTo(results) {
                    it.isDirectory || isMimeType(it, mimeType, mimeTypeMap)
                }

                Collections.sort(results, comparator)
                return results.toTypedArray()
            }

            return null
        }

        private fun mimeMatches(filter: String?, test: String?): Boolean {
            return test != null
                    && (filter == null || "*/*" == filter || filter == test || filter.endsWith("/*")
                            && filter.regionMatches(0, test, 0, filter.indexOf('/')))
        }

        private fun isMimeType(file: File, filter: String?, mimeTypeMap: MimeTypeMap): Boolean {
            if (filter != null) {
                val mimeType = mimeTypeMap.getMimeTypeFromExtension(file.extension)
                return mimeMatches(filter, mimeType)
            }
            return true
        }
    }
}