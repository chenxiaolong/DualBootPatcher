#!/usr/bin/env python3

# Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import os
import re
import shutil
import sys
from PyQt5 import Qt
from PyQt5 import QtCore
from PyQt5 import QtGui
from PyQt5 import QtWidgets

import multiboot.autopatcher as autopatcher
import multiboot.config as config
import multiboot.fileinfo as fileinfo
import multiboot.operatingsystem as OS
import multiboot.partitionconfigs as partitionconfigs
import multiboot.patcher as patcher
import multiboot.ramdisk as ramdisk
import multiboot.ui.qtui as qtui


class PatcherTask(QtCore.QObject):
    finished = QtCore.pyqtSignal(bool, str)

    def __init__(self, file_info):
        super().__init__()
        self.file_info = file_info
        patcher.set_ui(qtui)

    def patch(self):
        output = patcher.patch_file(self.file_info)
        failed = True

        if output:
            failed = False

            self.file_info.move_to_target(output, move=True)

        self.finished.emit(failed, self.file_info.get_new_filename())


class Task(QtWidgets.QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)

        # Checkmark or dot
        self.status = QtWidgets.QLabel(self)
        # Task
        self.label = QtWidgets.QLabel(self)

        layout = QtWidgets.QHBoxLayout(self)
        layout.setContentsMargins(QtCore.QMargins(0, 0, 0, 0))

        self.status.setFixedWidth(self.status.sizeHint().width())
        self.label.setText('A task')

        layout.addWidget(self.status)
        layout.addWidget(self.label)
        layout.addStretch(1)

        self.setLayout(layout)
        self.setSizePolicy(QtWidgets.QSizePolicy.Fixed,
                           QtWidgets.QSizePolicy.Fixed)
        #self.setFixedHeight(self.sizeHint().height())

    def settext(self, msg):
        self.label.setText(msg)

    def gettext(self):
        return self.label.text()

    def setinprogress(self):
        self.status.setText('\u2022')

    def setcomplete(self):
        self.status.setText('\u2713')


class ProgressDialog(QtWidgets.QDialog):
    trigger_add_task = QtCore.pyqtSignal(str)
    trigger_set_task = QtCore.pyqtSignal(str)
    trigger_details = QtCore.pyqtSignal(str)
    trigger_clear = QtCore.pyqtSignal()
    trigger_info = QtCore.pyqtSignal(str)
    trigger_command_error = QtCore.pyqtSignal(str, str)
    trigger_failed = QtCore.pyqtSignal(str)
    trigger_succeeded = QtCore.pyqtSignal(str)
    trigger_max_progress = QtCore.pyqtSignal(int)
    trigger_set_progress = QtCore.pyqtSignal(int)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.blockexit = True
        self.failmsg = None

        # The patcher runs in another thread
        self.connect_slots()

        self.setWindowTitle('Patching file...')
        self.setFixedSize(550, 250)
        self.setWindowFlags(self.windowFlags() \
                            | QtCore.Qt.CustomizeWindowHint \
                            & ~QtCore.Qt.WindowCloseButtonHint)

        # Tasks
        self.tasks = list()

        # Container for tasks list
        self.tasksbox = QtWidgets.QGroupBox(self)
        self.tasksbox.setTitle('Tasks')

        self.taskslayout = QtWidgets.QVBoxLayout(self.tasksbox)
        self.taskslayout.addStretch(1)

        # Keep at top and don't stretch
        self.tasksbox.setLayout(self.taskslayout)
        self.tasksbox.setAlignment(QtCore.Qt.AlignTop)
        self.tasksbox.setSizePolicy(QtWidgets.QSizePolicy.Fixed,
                                    QtWidgets.QSizePolicy.Minimum)

        # Vertical separator
        vline = QtWidgets.QFrame(self)
        vline.setFrameShape(QtWidgets.QFrame.VLine)
        vline.setFrameShadow(QtWidgets.QFrame.Sunken)
        vline.setSizePolicy(QtWidgets.QSizePolicy.Minimum,
                            QtWidgets.QSizePolicy.Expanding)

        # Details & progress container
        detailsprogress = QtWidgets.QWidget(self)

        detailsprogresslayout = QtWidgets.QVBoxLayout(detailsprogress)
        detailsprogresslayout.setContentsMargins(0, 0, 0, 0)

        # Details container
        detailsbox = QtWidgets.QGroupBox(detailsprogress)
        detailsbox.setTitle('Details')

        # Details
        self.detailslabel = QtWidgets.QLabel(detailsbox)
        self.detailslabel.setWordWrap(True)

        detailslayout = QtWidgets.QVBoxLayout(detailsbox)
        detailslayout.addWidget(self.detailslabel)
        detailsbox.setLayout(detailslayout)

        # Horizontal separator
        hline = QtWidgets.QFrame(detailsprogress)
        hline.setFrameShape(QtWidgets.QFrame.HLine)
        hline.setFrameShadow(QtWidgets.QFrame.Sunken)
        hline.setSizePolicy(QtWidgets.QSizePolicy.Expanding,
                            QtWidgets.QSizePolicy.Minimum)

        # Progress bar (for zip compression step)
        self.progressbar = QtWidgets.QProgressBar(detailsprogress)
        self.progressbar.setFormat(r'%p% - %v / %m files')
        self.progressbar.setMaximum(0)
        self.progressbar.setValue(0)

        # Right half layout
        detailsprogresslayout.addWidget(detailsbox)
        detailsprogresslayout.addStretch(1)
        detailsprogresslayout.addWidget(hline)
        detailsprogresslayout.addWidget(self.progressbar)

        detailsprogress.setLayout(detailsprogresslayout)

        # Dialog layout
        layout = QtWidgets.QHBoxLayout(self)
        layout.addWidget(self.tasksbox)
        layout.addWidget(vline)
        layout.addWidget(detailsprogress)

        self.setLayout(layout)

    def closeEvent(self, event):
        if self.blockexit:
            event.ignore()

    def keyPressEvent(self, event):
        if event.key() == QtCore.Qt.Key_Escape:
            # QDialogs exit with escape key by default
            event.ignore()
        else:
            super().keyPressEvent(event)

    def connect_slots(self):
        self.trigger_add_task.connect(self.add_task)
        self.trigger_set_task.connect(self.set_task)
        self.trigger_details.connect(self.details)
        self.trigger_clear.connect(self.clear)
        self.trigger_info.connect(self.info)
        self.trigger_command_error.connect(self.command_error)
        self.trigger_failed.connect(self.failed)
        self.trigger_succeeded.connect(self.succeeded)
        self.trigger_max_progress.connect(self.max_progress)
        self.trigger_set_progress.connect(self.set_progress)

    def disconnect_slots(self):
        self.trigger_add_task.disconnect()
        self.trigger_set_task.disconnect()
        self.trigger_details.disconnect()
        self.trigger_clear.disconnect()
        self.trigger_info.disconnect()
        self.trigger_command_error.disconnect()
        self.trigger_failed.disconnect()
        self.trigger_succeeded.disconnect()
        self.trigger_max_progress.disconnect()
        self.trigger_set_progress.disconnect()

    def add_task(self, msg):
        task = Task(self.tasksbox)
        task.settext(msg)
        self.taskslayout.insertWidget(self.taskslayout.count() - 1, task)
        self.tasks.append(task)

    def set_task(self, task):
        for i in self.tasks:
            if i.gettext() == task:
                i.setinprogress()
                break
            else:
                i.setcomplete()

    def details(self, msg):
        self.detailslabel.setText(msg)

    def clear(self):
        self.detailslabel.setText('')

    def info(self, msg):
        self.detailslabel.setText(msg)

    def command_error(self, output, error):
        pass

    def failed(self, msg):
        self.failmsg = msg

    def succeeded(self, msg):
        pass

    def max_progress(self, num):
        self.progressbar.setMaximum(num)

    def set_progress(self, num):
        self.progressbar.setValue(num)

    def finished(self, failed, newfile):
        # Clean up
        for i in range(len(self.tasks) - 1, -1, -1):
            #self.taskslayout.removeWidget(self.tasks[i])
            self.tasks[i].setParent(None)
            del self.tasks[i]
        self.set_progress(0)
        self.max_progress(0)
        self.details('')
        self.blockexit = False
        self.close()

    def show(self):
        self.blockexit = True
        super().show()


class UnsupportedFileDialog(QtWidgets.QDialog):
    def __init__(self, parent=None, file_info=None):
        super().__init__(parent)

        self.addonce = False
        self.file_info = file_info

        self.setWindowTitle('Unsupported File')

        self.bootimgwidgets = list()

        optslayout = QtWidgets.QGridLayout(self)

        # Message
        maintext = QtWidgets.QLabel()
        maintext.setText(
            'File: %s<br /><br />The file you have selected is not supported.'
            ' You can attempt to patch the file anyway using the options below.'
            % self.file_info._filename)

        optslayout.addWidget(maintext, 0, 0, 1, -1)

        # Horizontal separator
        hline = QtWidgets.QFrame()
        hline.setFrameShape(QtWidgets.QFrame.HLine)
        hline.setFrameShadow(QtWidgets.QFrame.Sunken)
        hline.setSizePolicy(QtWidgets.QSizePolicy.Expanding,
                            QtWidgets.QSizePolicy.Minimum)

        optslayout.addWidget(hline, 1, 0, 1, -1)

        # Preset
        self.cmbpreset = QtWidgets.QComboBox()
        self.lbpreset = QtWidgets.QLabel()
        self.lbpreset.setText('Preset')

        optslayout.addWidget(self.lbpreset, 2, 0, 1, 1)
        optslayout.addWidget(self.cmbpreset, 2, 4, 1, -1)

        # Horizontal separator
        hline = QtWidgets.QFrame()
        hline.setFrameShape(QtWidgets.QFrame.HLine)
        hline.setFrameShadow(QtWidgets.QFrame.Sunken)
        hline.setSizePolicy(QtWidgets.QSizePolicy.Expanding,
                            QtWidgets.QSizePolicy.Minimum)

        optslayout.addWidget(hline, 3, 0, 1, -1)

        # Auto patch
        self.rbautopatch = QtWidgets.QRadioButton()
        self.lbautopatch = QtWidgets.QLabel()
        self.lbautopatch.setText('Auto-patch')

        # Auto patch selection
        self.cmbautopatchsel = QtWidgets.QComboBox()
        self.lbautopatchsel = QtWidgets.QLabel()
        self.lbautopatchsel.setText('Auto-patcher')

        optslayout.addWidget(self.lbautopatch, 4, 0, 1, 1)
        optslayout.addWidget(self.rbautopatch, 4, 1, 1, 1)
        optslayout.addWidget(self.lbautopatchsel, 4, 3, 1, 1)
        optslayout.addWidget(self.cmbautopatchsel, 4, 4, 1, -1)

        # Patch
        self.rbpatch = QtWidgets.QRadioButton()
        self.lbpatch = QtWidgets.QLabel()
        self.lbpatch.setText('Patch')

        # Patch path input
        self.lepatchinput = QtWidgets.QLineEdit()
        self.lbpatchinput = QtWidgets.QLabel()
        self.lbpatchinput.setText('Patch path')
        self.btnpatchinput = QtWidgets.QPushButton()
        self.btnpatchinput.setText('Browse')

        optslayout.addWidget(self.lbpatch, 5, 0, 1, 1)
        optslayout.addWidget(self.rbpatch, 5, 1, 1, 1)
        optslayout.addWidget(self.lbpatchinput, 5, 3, 1, 1)
        optslayout.addWidget(self.lepatchinput, 5, 4, 1, 1)
        optslayout.addWidget(self.btnpatchinput, 5, 5, 1, 1)

        # Horizontal separator
        hline = QtWidgets.QFrame()
        hline.setFrameShape(QtWidgets.QFrame.HLine)
        hline.setFrameShadow(QtWidgets.QFrame.Sunken)
        hline.setSizePolicy(QtWidgets.QSizePolicy.Expanding,
                            QtWidgets.QSizePolicy.Minimum)

        optslayout.addWidget(hline, 6, 0, 1, -1)

        # Remove device check
        self.cbdevicecheck = QtWidgets.QCheckBox()
        self.lbdevicecheck = QtWidgets.QLabel()
        self.lbdevicecheck.setText('Remove device check')

        optslayout.addWidget(self.lbdevicecheck, 7, 0, 1, 1)
        optslayout.addWidget(self.cbdevicecheck, 7, 1, 1, 1)

        # Has boot image
        self.cbhasbootimg = QtWidgets.QCheckBox()
        self.lbhasbootimg = QtWidgets.QLabel()
        self.lbhasbootimg.setText('Has boot image')

        optslayout.addWidget(self.lbhasbootimg, 8, 0, 1, 1)
        optslayout.addWidget(self.cbhasbootimg, 8, 1, 1, 1)

        # Boot image
        self.lebootimg = QtWidgets.QLineEdit()
        self.lbbootimg = QtWidgets.QLabel()
        self.lbbootimg.setText('Boot image')

        optslayout.addWidget(self.lbbootimg, 8, 3, 1, 1)
        optslayout.addWidget(self.lebootimg, 8, 4, 1, -1)
        self.bootimgwidgets.append(self.lbbootimg)
        self.bootimgwidgets.append(self.lebootimg)

        # Horizontal separator
        hline = QtWidgets.QFrame()
        hline.setFrameShape(QtWidgets.QFrame.HLine)
        hline.setFrameShadow(QtWidgets.QFrame.Sunken)
        hline.setSizePolicy(QtWidgets.QSizePolicy.Expanding,
                             QtWidgets.QSizePolicy.Minimum)

        optslayout.addWidget(hline, 9, 0, 1, -1)

        # Needs patched init
        self.cbpatchedinit = QtWidgets.QCheckBox()
        self.lbpatchedinit = QtWidgets.QLabel()
        self.lbpatchedinit.setText('Needs patched init')

        optslayout.addWidget(self.lbpatchedinit, 10, 0, 1, 1)
        optslayout.addWidget(self.cbpatchedinit, 10, 1, 1, 1)
        self.bootimgwidgets.append(self.cbpatchedinit)
        self.bootimgwidgets.append(self.lbpatchedinit)

        self.cmbinitfile = QtWidgets.QComboBox()
        self.lbinitfile = QtWidgets.QLabel()
        self.lbinitfile.setText('Init file')

        optslayout.addWidget(self.lbinitfile, 10, 3, 1, 1)
        optslayout.addWidget(self.cmbinitfile, 10, 4, 1, -1)
        self.bootimgwidgets.append(self.cmbinitfile)
        self.bootimgwidgets.append(self.lbinitfile)

        # Ramdisk
        self.cmbramdisk = QtWidgets.QComboBox()
        self.lbramdisk = QtWidgets.QLabel()
        self.lbramdisk.setText('Ramdisk')

        optslayout.addWidget(self.lbramdisk, 11, 3, 1, 1)
        optslayout.addWidget(self.cmbramdisk, 11, 4, 1, -1)
        self.bootimgwidgets.append(self.lbramdisk)
        self.bootimgwidgets.append(self.cmbramdisk)

        # Horizontal separator
        hline = QtWidgets.QFrame()
        hline.setFrameShape(QtWidgets.QFrame.HLine)
        hline.setFrameShadow(QtWidgets.QFrame.Sunken)
        hline.setSizePolicy(QtWidgets.QSizePolicy.Expanding,
                             QtWidgets.QSizePolicy.Minimum)

        optslayout.addWidget(hline, 12, 0, 1, -1)

        # Standard buttons
        self.stdbuttons = QtWidgets.QDialogButtonBox()
        self.stdbuttons.setStandardButtons(
            QtWidgets.QDialogButtonBox.Ok | QtWidgets.QDialogButtonBox.Cancel)

        optslayout.addWidget(self.stdbuttons, 13, 0, 1, -1)

        self.setLayout(optslayout)

        self.setMinimumWidth(550)
        #self.setSizePolicy(QtWidgets.QSizePolicy.Fixed,
        #                   QtWidgets.QSizePolicy.Fixed)
        self.setFixedSize(self.sizeHint())

        self.setwidgetactions()
        self.setdefaultvalues()

    def keyPressEvent(self, event):
        if event.key() == QtCore.Qt.Key_Escape:
            # QDialogs exit with escape key by default
            event.ignore()
        else:
            super().keyPressEvent(event)

    def setdefaultvalues(self):
        # Auto patch by default
        self.rbautopatch.setChecked(True)
        self.patchtypetoggled()

        # Don't remove device checks
        self.cbdevicecheck.setChecked(False)

        # Boot image exists
        self.cbhasbootimg.setChecked(True)
        self.hasbootimagetoggled()

        # No patched init
        self.cbpatchedinit.setChecked(False)
        self.patchedinittoggled()

        # 'boot.img'
        self.lebootimg.setText('boot.img')
        self.enablebootimgcombobox(True)

        if not self.addonce:
            self.addonce = True

            self.presets = fileinfo.get_infos(self.file_info._device)
            self.cmbpreset.addItem('Custom')
            for i in self.presets:
                self.cmbpreset.addItem(i[0][:-3])

            for i in fileinfo.get_inits():
                self.cmbinitfile.addItem(i)

            for i in ramdisk.list_ramdisks(self.file_info._device):
                self.cmbramdisk.addItem(i[:-4])

            self.autopatchers = autopatcher.get_auto_patchers()
            for i in self.autopatchers:
                self.cmbautopatchsel.addItem(i.name)

    def setwidgetactions(self):
        # Preset
        self.cmbpreset.currentIndexChanged.connect(self.presetchanged)

        # Radio buttons
        self.rbautopatch.toggled.connect(self.patchtypetoggled)
        self.rbpatch.toggled.connect(self.patchtypetoggled)

        # Patch file selection
        self.btnpatchinput.clicked.connect(self.choosefile)

        # Has boot image checkbox
        self.cbhasbootimg.toggled.connect(self.hasbootimagetoggled)

        # Needs patched init checkbox
        self.cbpatchedinit.toggled.connect(self.patchedinittoggled)

        # Standard buttons
        self.stdbuttons.button(QtWidgets.QDialogButtonBox.Ok) \
            .clicked.connect(self.startpatching)
        self.stdbuttons.button(QtWidgets.QDialogButtonBox.Cancel) \
            .clicked.connect(self.canceldialog)

    @QtCore.pyqtSlot()
    def presetchanged(self):
        if self.cmbpreset.currentText() == 'Custom':
            self.enableallwidgets(True)
            self.setdefaultvalues()
        else:
            self.enableallwidgets(False)

    def enableallwidgets(self, onoff):
        self.rbautopatch.setEnabled(onoff)
        self.lbautopatch.setEnabled(onoff)
        self.cmbautopatchsel.setEnabled(onoff)
        self.lbautopatchsel.setEnabled(onoff)
        self.rbpatch.setEnabled(onoff)
        self.lbpatch.setEnabled(onoff)
        self.lepatchinput.setEnabled(onoff)
        self.lbpatchinput.setEnabled(onoff)
        self.btnpatchinput.setEnabled(onoff)
        self.cbdevicecheck.setEnabled(onoff)
        self.lbdevicecheck.setEnabled(onoff)
        self.cbhasbootimg.setEnabled(onoff)
        self.lbhasbootimg.setEnabled(onoff)
        for widget in self.bootimgwidgets:
            widget.setEnabled(onoff)

    def enablebootimgcombobox(self, onoff):
        if self.file_info._filetype == fileinfo.ZIP_FILE:
            self.lebootimg.setEnabled(onoff)
            self.lbbootimg.setEnabled(onoff)
        else:
            self.lebootimg.setEnabled(False)
            self.lbbootimg.setEnabled(False)

    @QtCore.pyqtSlot()
    def patchtypetoggled(self):
        if self.rbautopatch.isChecked():
            self.lbautopatchsel.setEnabled(True)
            self.cmbautopatchsel.setEnabled(True)

            self.lbpatchinput.setEnabled(False)
            self.lepatchinput.setEnabled(False)
            self.btnpatchinput.setEnabled(False)

        elif self.rbpatch.isChecked():
            self.lbautopatchsel.setEnabled(False)
            self.cmbautopatchsel.setEnabled(False)

            self.lbpatchinput.setEnabled(True)
            self.lepatchinput.setEnabled(True)
            self.btnpatchinput.setEnabled(True)

    @QtCore.pyqtSlot()
    def hasbootimagetoggled(self):
        if self.cbhasbootimg.isChecked():
            for widget in self.bootimgwidgets:
                widget.setEnabled(True)
            self.patchedinittoggled()
            self.enablebootimgcombobox(True)

        else:
            for widget in self.bootimgwidgets:
                widget.setEnabled(False)

    @QtCore.pyqtSlot()
    def patchedinittoggled(self):
        if self.cbpatchedinit.isChecked():
            self.cmbinitfile.setEnabled(True)
            self.lbinitfile.setEnabled(True)
        else:
            self.cmbinitfile.setEnabled(False)
            self.lbinitfile.setEnabled(False)

    @QtCore.pyqtSlot()
    def choosefile(self):
        supported = [ 'Patch files (*.patch *.diff)' ]

        filename = QtWidgets.QFileDialog.getOpenFileName(
            self, filter=';;'.join(supported))[0]

        self.lepatchinput.setText(filename)

    @QtCore.pyqtSlot()
    def startpatching(self):
        if self.cmbpreset.currentText() == 'Custom':
            self.file_info.name = 'Unsupported file (with custom patcher options)'

            if self.rbautopatch.isChecked():
                ap = self.autopatchers[self.cmbautopatchsel.currentIndex()]
                self.file_info.patch = ap.patcher
                self.file_info.extract = ap.extractor
            elif self.rbpatch.isChecked():
                self.file_info.patch = self.lepatchinput.text()

            self.file_info.has_boot_image = self.cbhasbootimg.isChecked()
            if self.file_info.has_boot_image:
                self.file_info.ramdisk = self.cmbramdisk.currentText() + '.def'
                self.file_info.bootimg = self.lebootimg.text()

            if self.cbpatchedinit.isChecked():
                self.file_info.patched_init = self.cmbinitfile.currentText()
            else:
                self.file_info.patched_init = None

            self.file_info.device_check = not self.cbdevicecheck.isChecked()

        else:
            orig_file_info = self.presets[self.cmbpreset.currentIndex() - 1][1]

            self.file_info.name = 'Unsupported file (manually set to: %s)' % orig_file_info.name
            self.file_info.patch = orig_file_info.patch
            self.file_info.extract = orig_file_info.extract
            self.file_info.ramdisk = orig_file_info.ramdisk
            self.file_info.bootimg = orig_file_info.bootimg
            self.file_info.has_boot_image = orig_file_info.has_boot_image
            self.file_info.patched_init = orig_file_info.patched_init
            self.file_info.device_check = orig_file_info.device_check
            self.file_info.configs = orig_file_info.configs
            # Possibly override?
            #self.file_info.configs = ['all']

        self.accept()

    @QtCore.pyqtSlot()
    def canceldialog(self):
        self.setdefaultvalues()
        self.reject()


class MessageBox(QtWidgets.QMessageBox):
    MAX_WIDTH = 550

    def __init__(self, parent=None):
        super().__init__(parent)

    # QMessageBox bases the dialog size on the main text instead of the
    # informative text unfortunately
    def setInformativeText(self, text):
        super().setInformativeText(text)

        templabel = QtWidgets.QLabel()
        templabel.setText(text)
        minimumwidth = templabel.sizeHint().width()
        del templabel

        labels = self.findChildren(QtWidgets.QLabel)
        for label in labels:
            if label.text() == text:
                if minimumwidth > MessageBox.MAX_WIDTH:
                    label.setFixedWidth(MessageBox.MAX_WIDTH)
                else:
                    label.setMinimumWidth(minimumwidth)
                    label.setMaximumWidth(MessageBox.MAX_WIDTH)
                break


class Patcher(QtWidgets.QWidget):
    def __init__(self, parent=None, filename=None):
        super().__init__(parent)
        iconpath = os.path.join(OS.rootdir, 'scripts', 'icon.png')
        self.setWindowIcon(QtGui.QIcon(iconpath))

        if filename:
            self.filename = os.path.abspath(filename)
            self.automode = True
        else:
            self.filename = None
            self.automode = False

        self.partconfigs = partitionconfigs.get()
        self.devices = config.list_devices()

        self.setWindowTitle('Dual Boot Patcher')

        # Partition configuration selector and file chooser
        self.partconfigdesc = QtWidgets.QLabel(self)
        self.partconfigcombobox = QtWidgets.QComboBox(self)
        self.devicecombobox = QtWidgets.QComboBox(self)
        self.filechooserbutton = QtWidgets.QPushButton(self)

        # A simple vertical layout will do
        layout = QtWidgets.QVBoxLayout(self)
        layout.addWidget(self.devicecombobox)
        layout.addWidget(self.partconfigcombobox)
        layout.addWidget(self.partconfigdesc)
        layout.addStretch(1)
        layout.addWidget(self.filechooserbutton)

        self.setLayout(layout)

        # Populate devices
        for d in self.devices:
            self.devicecombobox.addItem('%s (%s)' % (d, config.get(d, 'name')))

        self.devicecombobox.currentIndexChanged.connect(self.deviceselected)
        self.deviceselected(self.devicecombobox.currentIndex())

        # Populate partition configs
        for i in self.partconfigs:
            self.partconfigcombobox.addItem(i.name)

        self.partconfigcombobox.currentIndexChanged.connect(self.partconfigselected)
        self.partconfigselected(self.partconfigcombobox.currentIndex())

        # Choose file button
        if self.automode:
            self.filechooserbutton.setText("Continue")
        else:
            self.filechooserbutton.setText("Choose file to patch")
        self.filechooserbutton.clicked.connect(self.selectfile)

        # Progress dialog
        self.progressdialog = ProgressDialog(self)

        qtui.listener = self.progressdialog

        # Fixed size
        self.setMinimumWidth(300)
        self.setFixedHeight(self.sizeHint().height())
        self.move(300, 300)

    @QtCore.pyqtSlot(int)
    def deviceselected(self, index):
        self.device = self.devices[self.devicecombobox.currentIndex()]

    @QtCore.pyqtSlot(int)
    def partconfigselected(self, index):
        self.partconfigdesc.setText(self.partconfigs[index].description)

    @QtCore.pyqtSlot()
    def selectfile(self):
        if not self.automode:
            supported = [
                'Zip files and boot images (*.zip *.img *.lok)',
                'Zip files (*.zip)',
                'Boot images (*.img)',
                'Loki boot images (*.lok)'
            ]

            self.filename = QtWidgets.QFileDialog.getOpenFileName(
                self, filter=';;'.join(supported))[0]

            if not self.filename:
                return

        self.hidewidgets()

        file_info = fileinfo.FileInfo()
        file_info.set_filename(self.filename)
        file_info.set_device(self.device)

        if self.automode and not file_info.is_filetype_supported():
            ext = os.path.splitext(self.filename)[1]
            msgbox.setText('The \'%s\' file extension is not supported.' % ext)
            msgbox.setIcon(QtWidgets.QMessageBox.Warning)
            msgbox.setStandardButtons(QtWidgets.QMessageBox.Ok)
            msgbox.exec()
            self.close()
            return

        if not file_info.find_and_merge_patchinfo():
            msgbox = UnsupportedFileDialog(self, file_info=file_info)
            ret = msgbox.exec()
            if ret == QtWidgets.QDialog.Accepted:
                self.startpatching(file_info)
            else:
                self.showwidgets()

        else:
            self.startpatching(file_info)

    def hidewidgets(self):
        self.devicecombobox.setEnabled(False)
        self.partconfigcombobox.setEnabled(False)
        self.partconfigdesc.setEnabled(False)
        self.filechooserbutton.setEnabled(False)

    def showwidgets(self):
        self.devicecombobox.setEnabled(True)
        self.partconfigcombobox.setEnabled(True)
        self.partconfigdesc.setEnabled(True)
        self.filechooserbutton.setEnabled(True)

    def startpatching(self, file_info):
        msgbox = MessageBox(self)
        msgbox.setWindowTitle('Dual Boot Patcher')

        partconfig = self.partconfigs[self.partconfigcombobox.currentIndex()]

        if not file_info.is_partconfig_supported(partconfig):
            msgbox.setText(
                'The \'%s\' partition configuration is not supported for the file:<br />%s'
                % (partconfig.name, file_info._filename))
            msgbox.setIcon(QtWidgets.QMessageBox.Warning)
            msgbox.setStandardButtons(QtWidgets.QMessageBox.Ok)
            msgbox.exec()
            self.showwidgets()
            return

        file_info.set_partconfig(partconfig)

        msgbox.setText('Detected: <b>%s</b><br /><br />Click OK to patch:<br />%s'
            % (file_info.name, file_info._filename))
        msgbox.setIcon(QtWidgets.QMessageBox.Information)
        msgbox.setStandardButtons(QtWidgets.QMessageBox.Ok | QtWidgets.QMessageBox.Cancel)

        ret = msgbox.exec()

        if ret == QtWidgets.QMessageBox.Cancel:
            self.showwidgets()
            return

        self.progressdialog.show()

        self.task = PatcherTask(file_info)
        self.thread = QtCore.QThread(self)
        self.task.moveToThread(self.thread)
        self.thread.started.connect(self.task.patch)
        self.task.finished.connect(self.patchingfinished)

        self.thread.start()

    def patchingfinished(self, failed, newfile):
        self.progressdialog.finished(failed, newfile)

        self.thread.exit()
        self.task = None
        self.thread = None

        msgbox = MessageBox(self)
        msgbox.setWindowTitle('Dual Boot Patcher')

        if not failed:
            msgbox.setText('Successfully patched file.')
            msgbox.setInformativeText('New file: ' + newfile)
            msgbox.setIcon(QtWidgets.QMessageBox.Information)
        else:
            msgbox.setText('Failed to patch file.')
            msgbox.setInformativeText(self.progressdialog.failmsg)
            msgbox.setIcon(QtWidgets.QMessageBox.Warning)

        msgbox.setStandardButtons(QtWidgets.QMessageBox.Ok)

        msgbox.exec()

        self.showwidgets()

        if self.automode:
            self.close()


if __name__ == '__main__':
    app = QtWidgets.QApplication(sys.argv)
    if len(sys.argv) > 1:
        gui = Patcher(filename=sys.argv[1])
    else:
        gui = Patcher()
    gui.show()
    sys.exit(app.exec_())
