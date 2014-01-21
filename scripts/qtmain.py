#!/usr/bin/env python3

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

    def __init__(self, filename, partconfig, file_info):
        super().__init__()
        self.filename = filename
        self.partconfig = partconfig
        self.fileinfo = file_info
        self.ui = qtui
        patcher.set_ui(qtui)

        self.failed = True
        self.newfile = None

    def patch(self):
        loki_msg = ("The boot image was unloki'd in order to be patched. "
                    "*** Remember to flash loki-doki "
                    "if you have a locked bootloader ***")

        patcher.add_tasks(self.fileinfo)

        if self.filename.endswith('.zip'):
            output = patcher.patch_zip(self.filename, self.fileinfo, self.partconfig)

            if output:
                self.newfile = re.sub(r'\.zip$', '_%s.zip' % self.partconfig.id, self.filename)
                shutil.move(output, self.newfile)
                if self.fileinfo.loki:
                    self.ui.succeeded('Successfully patched zip. ' + loki_msg)
                else:
                    self.ui.succeeded(loki_msg)
                self.failed = False

        elif self.filename.endswith('.img'):
            output = patcher.patch_boot_image(self.filename, self.fileinfo, self.partconfig)

            if output:
                self.newfile = re.sub(r'\.img$', '_%s.img' % self.partconfig.id, self.filename)
                shutil.move(output, self.newfile)

                self.ui.succeeded('Successfully boot image')
                self.failed = False

        elif self.filename.endswith('.lok'):
            output = patcher.patch_boot_image(self.filename, self.fileinfo, self.partconfig)

            if output:
                self.newfile = re.sub(r'\.lok$', '_%s.lok' % self.partconfig.id, self.filename)
                shutil.move(output, self.newfile)

                self.ui.succeeded('Successfully patched Loki\'d boot image. ' + loki_msg)
                self.failed = False

        self.finished.emit(self.failed, self.newfile)


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
        self.label.setText("A task")

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
    def __init__(self, parent=None, filename='<NO FILENAME>'):
        super().__init__(parent)

        self.addonce = False
        self.filename = filename

        self.setWindowTitle('Unsupported File')

        optslayout = QtWidgets.QGridLayout(self)

        # Message
        maintext = QtWidgets.QLabel()
        maintext.setText(
            'File: %s<br /><br />The file you have selected is not supported.'
            ' You can attempt to patch the file anyway using the options below.'
            % filename)

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

        # Horizontal separator
        hline = QtWidgets.QFrame()
        hline.setFrameShape(QtWidgets.QFrame.HLine)
        hline.setFrameShadow(QtWidgets.QFrame.Sunken)
        hline.setSizePolicy(QtWidgets.QSizePolicy.Expanding,
                             QtWidgets.QSizePolicy.Minimum)

        optslayout.addWidget(hline, 9, 0, 1, -1)

        # Loki
        self.cbloki = QtWidgets.QCheckBox()
        self.lbloki = QtWidgets.QLabel()
        self.lbloki.setText('Loki')

        optslayout.addWidget(self.lbloki, 10, 0, 1, 1)
        optslayout.addWidget(self.cbloki, 10, 1, 1, 1)

        # Needs new init
        self.cbnewinit = QtWidgets.QCheckBox()
        self.lbnewinit = QtWidgets.QLabel()
        self.lbnewinit.setText('Needs new init (AOSP 4.2 only)')

        optslayout.addWidget(self.lbnewinit, 11, 0, 1, 1)
        optslayout.addWidget(self.cbnewinit, 11, 1, 1, 1)

        # Vertical separator
        vline = QtWidgets.QFrame()
        vline.setFrameShape(QtWidgets.QFrame.VLine)
        vline.setFrameShadow(QtWidgets.QFrame.Sunken)
        vline.setSizePolicy(QtWidgets.QSizePolicy.Minimum,
                            QtWidgets.QSizePolicy.Expanding)

        optslayout.addWidget(vline, 10, 2, 2, 1)

        # Boot image
        self.lebootimg = QtWidgets.QLineEdit()
        self.lbbootimg = QtWidgets.QLabel()
        self.lbbootimg.setText('Boot image')

        optslayout.addWidget(self.lbbootimg, 10, 3, 1, 1)
        optslayout.addWidget(self.lebootimg, 10, 4, 1, -1)

        # Ramdisk
        self.cmbramdisk = QtWidgets.QComboBox()
        self.lbramdisk = QtWidgets.QLabel()
        self.lbramdisk.setText('Ramdisk')

        optslayout.addWidget(self.lbramdisk, 11, 3, 1, 1)
        optslayout.addWidget(self.cmbramdisk, 11, 4, 1, -1)

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

        # No loki
        self.cbloki.setChecked(False)

        # No new init
        self.cbnewinit.setChecked(False)

        # 'boot.img'
        self.lebootimg.setText('boot.img')

        if not self.addonce:
            self.addonce = True

            self.presets = fileinfo.get_infos(self.filename, config.get_device())
            self.cmbpreset.addItem('Custom')
            for i in self.presets:
                self.cmbpreset.addItem(i[0][:-3])

            for i in ramdisk.list_ramdisks():
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
        self.cbloki.setEnabled(onoff)
        self.lbloki.setEnabled(onoff)
        self.cbnewinit.setEnabled(onoff)
        self.lbnewinit.setEnabled(onoff)
        self.lebootimg.setEnabled(onoff)
        self.lbbootimg.setEnabled(onoff)
        self.cmbramdisk.setEnabled(onoff)
        self.lbramdisk.setEnabled(onoff)

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
            self.cbloki.setEnabled(True)
            self.lbloki.setEnabled(True)
            self.cbnewinit.setEnabled(True)
            self.lbnewinit.setEnabled(True)
            self.lebootimg.setEnabled(True)
            self.lbbootimg.setEnabled(True)
            self.cmbramdisk.setEnabled(True)
            self.lbramdisk.setEnabled(True)

        else:
            self.cbloki.setEnabled(False)
            self.lbloki.setEnabled(False)
            self.cbnewinit.setEnabled(False)
            self.lbnewinit.setEnabled(False)
            self.lebootimg.setEnabled(False)
            self.lbbootimg.setEnabled(False)
            self.cmbramdisk.setEnabled(False)
            self.lbramdisk.setEnabled(False)

    @QtCore.pyqtSlot()
    def choosefile(self):
        supported = [ 'Patch files (*.patch *.diff)' ]

        filename = QtWidgets.QFileDialog.getOpenFileName(
            self, filter=';;'.join(supported))[0]

        self.lepatchinput.setText(filename)

    @QtCore.pyqtSlot()
    def startpatching(self):
        if self.cmbpreset.currentText() == 'Custom':
            # Create fileinfo
            file_info = fileinfo.FileInfo()
            file_info.name = 'Unsupported file (with custom patcher options)'

            if self.rbautopatch.isChecked():
                ap = self.autopatchers[self.cmbautopatchsel.currentIndex()]
                file_info.patch = ap.patcher
                file_info.extract = ap.extractor
            elif self.rbpatch.isChecked():
                file_info.patch = self.lepatchinput.text()

            file_info.has_boot_image = self.cbhasbootimg.isChecked()
            if file_info.has_boot_image:
                file_info.ramdisk = self.cmbramdisk.currentText() + '.def'
                file_info.bootimg = self.lebootimg.text()
                file_info.loki = self.cbloki.isChecked()

            file_info.need_new_init = self.cbnewinit.isChecked()

            file_info.device_check = not self.cbdevicecheck.isChecked()

            self.file_info = file_info

        else:
            orig_file_info = self.presets[self.cmbpreset.currentIndex() - 1][1]

            file_info = fileinfo.FileInfo()
            file_info.name = 'Unsupported file (manually set to: %s)' % orig_file_info.name
            file_info.patch = orig_file_info.patch
            file_info.extract = orig_file_info.extract
            file_info.ramdisk = orig_file_info.ramdisk
            file_info.bootimg = orig_file_info.bootimg
            file_info.has_boot_image = orig_file_info.has_boot_image
            file_info.need_new_init = orig_file_info.need_new_init
            file_info.loki = orig_file_info.loki
            file_info.device_check = orig_file_info.device_check
            file_info.configs = orig_file_info.configs
            # Possibly override?
            #file_info.configs = ['all']

            self.file_info = file_info

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

        self.setWindowTitle('Dual Boot Patcher')

        # Partition configuration selector and file chooser
        self.partconfigdesc = QtWidgets.QLabel(self)
        self.partconfigcombobox = QtWidgets.QComboBox(self)
        self.filechooserbutton = QtWidgets.QPushButton(self)

        # A simple vertical layout will do
        layout = QtWidgets.QVBoxLayout(self)
        layout.addWidget(self.partconfigcombobox)
        layout.addWidget(self.partconfigdesc)
        layout.addStretch(1)
        layout.addWidget(self.filechooserbutton)

        self.setLayout(layout)

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

        file_info = fileinfo.get_info(self.filename, config.get_device())

        if not file_info:
            msgbox = UnsupportedFileDialog(self, filename=self.filename)
            ret = msgbox.exec()
            if ret == QtWidgets.QDialog.Accepted:
                self.startpatching(msgbox.file_info)
            else:
                self.showwidgets()

        else:
            self.startpatching(file_info)


    def hidewidgets(self):
        self.partconfigcombobox.setEnabled(False)
        self.partconfigdesc.setEnabled(False)
        self.filechooserbutton.setEnabled(False)

    def showwidgets(self):
        self.partconfigcombobox.setEnabled(True)
        self.partconfigdesc.setEnabled(True)
        self.filechooserbutton.setEnabled(True)

    def startpatching(self, file_info):
        msgbox = MessageBox(self)
        msgbox.setWindowTitle('Dual Boot Patcher')

        partconfig = self.partconfigs[self.partconfigcombobox.currentIndex()]

        if (('all' not in file_info.configs
                and partconfig.id not in file_info.configs)
                or '!' + partconfig.id in file_info.configs):
            msgbox.setText(
                'The \'%s\' partition configuration is not supported for the file:<br />%s'
                % (partconfig.name, self.filename))
            msgbox.setIcon(QtWidgets.QMessageBox.Warning)
            msgbox.setStandardButtons(QtWidgets.QMessageBox.Ok)
            msgbox.exec()
            self.showwidgets()
            return

        msgbox.setText('Detected: <b>%s</b><br /><br />Click OK to patch:<br />%s'
            % (file_info.name, self.filename))
        msgbox.setIcon(QtWidgets.QMessageBox.Information)
        msgbox.setStandardButtons(QtWidgets.QMessageBox.Ok | QtWidgets.QMessageBox.Cancel)

        ret = msgbox.exec()

        if ret == QtWidgets.QMessageBox.Cancel:
            self.showwidgets()
            return

        self.progressdialog.show()

        self.task = PatcherTask(self.filename, partconfig, file_info)
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
