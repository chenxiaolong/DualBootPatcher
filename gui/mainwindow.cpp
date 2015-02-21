/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mainwindow.h"
#include "mainwindow_p.h"

#include <libmbp/patchinfo.h>
#include <libmbp/patchererror.h>

#include <boost/algorithm/string.hpp>

#include <QtCore/QStringBuilder>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>


const int patcherPtrTypeId = qRegisterMetaType<PatcherPtr>("PatcherPtr");
const int fileInfoPtrTypeId = qRegisterMetaType<FileInfoPtr>("FileInfoPtr");

MainWindow::MainWindow(PatcherConfig *pc, QWidget *parent)
    : QWidget(parent), d_ptr(new MainWindowPrivate())
{
    Q_D(MainWindow);

    setWindowIcon(QIcon(QStringLiteral(":/icons/icon.png")));
    setWindowTitle(qApp->applicationName());

    // If we're passed an argument, switch to automatic mode
    if (qApp->arguments().size() > 2) {
        d->autoMode = true;
        d->fileName = qApp->arguments().at(1);
    } else {
        d->autoMode = false;
        d->fileName.clear();
    }

    d->pc = pc;

    d->patcher = pc->createPatcher("MultiBootPatcher");

    addWidgets();
    setWidgetActions();
    populateWidgets();
    setWidgetDefaults();
    updateWidgetsVisibility();

    // Create thread
    d->thread = new QThread(this);
    d->task = new PatcherTask();
    d->task->moveToThread(d->thread);

    connect(d->thread, &QThread::finished,
            d->task, &QObject::deleteLater);
    connect(this, &MainWindow::runThread,
            d->task, &PatcherTask::patch);
    connect(d->task, &PatcherTask::finished,
            this, &MainWindow::onPatchingFinished);
    connect(d->task, &PatcherTask::maxProgressUpdated,
            this, &MainWindow::onMaxProgressUpdated);
    connect(d->task, &PatcherTask::progressUpdated,
            this, &MainWindow::onProgressUpdated);
    connect(d->task, &PatcherTask::detailsUpdated,
            this, &MainWindow::onDetailsUpdated);

    d->thread->start();
}

MainWindow::~MainWindow()
{
    Q_D(MainWindow);

    if (d->patcher) {
        d->patcher->cancelPatching();
        d->pc->destroyPatcher(d->patcher);
        d->patcher = nullptr;
    }

    if (d->thread != nullptr) {
        d->thread->quit();
        d->thread->wait();
    }
}

void MainWindow::onDeviceSelected(int index)
{
    Q_D(MainWindow);
    d->device = d->pc->devices()[index];

    refreshPresets();

    if (d->state == MainWindowPrivate::FinishedPatching) {
        d->state = MainWindowPrivate::ChoseFile;
    }

    checkSupported();
    updateWidgetsVisibility();
}

void MainWindow::onButtonClicked(QAbstractButton *button)
{
    Q_D(MainWindow);

    if (button == d->chooseFileBtn) {
        chooseFile();
    } else if (button == d->chooseAnotherFileBtn) {
        chooseFile();
    } else if (button == d->startPatchingBtn) {
        startPatching();
    }
}

void MainWindow::onPresetSelected(const QString &preset)
{
    Q_D(MainWindow);

    if (preset == tr("Custom")) {
        for (QWidget *widget : d->customPresetWidgets) {
            widget->setEnabled(true);
        }

        setWidgetDefaults();
    } else {
        for (QWidget *widget : d->customPresetWidgets) {
            widget->setEnabled(false);
        }
    }
}

void MainWindow::onHasBootImageToggled()
{
    Q_D(MainWindow);

    if (d->hasBootImageCb->isChecked()) {
        for (QWidget *widget : d->bootImageWidgets) {
            widget->setEnabled(true);
        }
    } else {
        for (QWidget *widget : d->bootImageWidgets) {
            widget->setEnabled(false);
        }
    }
}

void MainWindow::onMaxProgressUpdated(int max)
{
    Q_D(MainWindow);

    d->progressBar->setMaximum(max);
}

void MainWindow::onProgressUpdated(int value)
{
    Q_D(MainWindow);

    d->progressBar->setValue(value);
}

void MainWindow::onDetailsUpdated(const QString &text)
{
    Q_D(MainWindow);

    d->detailsLbl->setText(text);
}

void MainWindow::onPatchingFinished(const QString &newFile, bool failed,
                                    const QString &errorMessage)
{
    Q_D(MainWindow);

    d->patcherNewFile = newFile;
    d->patcherFailed = failed;
    d->patcherError = errorMessage;

    d->state = MainWindowPrivate::FinishedPatching;
    updateWidgetsVisibility();
}

void MainWindow::addWidgets()
{
    Q_D(MainWindow);

    int i = 0;

    d->mainContainer = new QWidget(this);

    // Selectors and file chooser
    d->deviceSel = new QComboBox(d->mainContainer);

    // Labels
    d->deviceLbl = new QLabel(tr("Device:"), d->mainContainer);

    QGridLayout *layout = new QGridLayout(d->mainContainer);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(d->deviceLbl, i, 0);
    layout->addWidget(d->deviceSel, i, 1, 1, -1);

    // Add items for unsupported files
    d->messageLbl = new QLabel(d->mainContainer);
    // Don't allow the window to grow too big
    d->messageLbl->setWordWrap(true);
    d->messageLbl->setMaximumWidth(550);
    d->presetLbl = new QLabel(tr("Preset:"), d->mainContainer);
    d->presetSel = new QComboBox(d->mainContainer);
    d->deviceCheckLbl = new QLabel(tr("Remove device check"), d->mainContainer);
    d->deviceCheckCb = new QCheckBox(d->mainContainer);
    d->hasBootImageLbl = new QLabel(tr("Has boot image"), d->mainContainer);
    d->hasBootImageCb = new QCheckBox(d->mainContainer);
    d->bootImageLbl = new QLabel(tr("Boot image"), d->mainContainer);
    d->bootImageLe = new QLineEdit(d->mainContainer);
    d->bootImageLe->setPlaceholderText(tr("Leave blank to autodetect"));

    d->chooseFileBtn = new QPushButton(tr("Choose file"), d->mainContainer);
    d->chooseAnotherFileBtn = new QPushButton(tr("Choose another file"), d->mainContainer);
    d->startPatchingBtn = new QPushButton(tr("Start patching"), d->mainContainer);

    d->buttons = new QDialogButtonBox(d->mainContainer);
    d->buttons->addButton(d->chooseFileBtn, QDialogButtonBox::ActionRole);
    d->buttons->addButton(d->chooseAnotherFileBtn, QDialogButtonBox::ActionRole);
    d->buttons->addButton(d->startPatchingBtn, QDialogButtonBox::ActionRole);

    QWidget *horiz1 = newHorizLine(d->mainContainer);
    QWidget *horiz2 = newHorizLine(d->mainContainer);
    QWidget *horiz3 = newHorizLine(d->mainContainer);

    layout->setColumnStretch(0, 0);
    layout->setColumnStretch(1, 0);
    layout->setColumnStretch(2, 0);
    layout->setColumnStretch(3, 1);

    layout->addWidget(horiz1,             ++i, 0, 1, -1);
    layout->addWidget(d->messageLbl,      ++i, 0, 1, -1);
    layout->addWidget(horiz2,             ++i, 0, 1, -1);
    layout->addWidget(d->presetLbl,       ++i, 0, 1,  1);
    layout->addWidget(d->presetSel,         i, 2, 1, -1);
    layout->addWidget(horiz3,             ++i, 0, 1, -1);
    layout->addWidget(d->deviceCheckLbl,  ++i, 0, 1,  1);
    layout->addWidget(d->deviceCheckCb,     i, 1, 1,  1);
    layout->addWidget(d->hasBootImageLbl, ++i, 0, 1,  1);
    layout->addWidget(d->hasBootImageCb,    i, 1, 1,  1);
    layout->addWidget(d->bootImageLbl,      i, 2, 1,  1);
    layout->addWidget(d->bootImageLe,       i, 3, 1, -1);


    layout->addWidget(newHorizLine(d->mainContainer), ++i, 0, 1, -1);

    layout->addWidget(d->buttons,         ++i, 0, 1, -1);


    d->mainContainer->setLayout(layout);

    // List of widgets related to the message label
    d->messageWidgets << horiz1;
    d->messageWidgets << d->messageLbl;

    // List of unsupported widgets
    d->unsupportedWidgets << horiz2;
    d->unsupportedWidgets << horiz3;
    d->unsupportedWidgets << d->presetLbl;
    d->unsupportedWidgets << d->presetSel;
    d->unsupportedWidgets << d->deviceCheckLbl;
    d->unsupportedWidgets << d->deviceCheckCb;
    d->unsupportedWidgets << d->hasBootImageLbl;
    d->unsupportedWidgets << d->hasBootImageCb;
    d->unsupportedWidgets << d->bootImageLbl;
    d->unsupportedWidgets << d->bootImageLe;

    // List of custom preset widgets
    d->customPresetWidgets << d->deviceCheckLbl;
    d->customPresetWidgets << d->deviceCheckCb;
    d->customPresetWidgets << d->hasBootImageLbl;
    d->customPresetWidgets << d->hasBootImageCb;
    d->customPresetWidgets << d->bootImageLbl;
    d->customPresetWidgets << d->bootImageLe;

    // List of boot image-related widgets
    d->bootImageWidgets << d->bootImageLbl;
    d->bootImageWidgets << d->bootImageLe;

    // Buttons
    d->progressContainer = new QWidget(this);
    QBoxLayout *progressLayout = new QVBoxLayout(d->progressContainer);
    progressLayout->setContentsMargins(0, 0, 0, 0);

    QGroupBox *detailsBox = new QGroupBox(d->progressContainer);
    detailsBox->setTitle(tr("Details"));

    d->detailsLbl = new QLabel(detailsBox);
    d->detailsLbl->setWordWrap(true);
    // Make sure the window doesn't change size while patching
    d->detailsLbl->setFixedWidth(500);

    QVBoxLayout *detailsLayout = new QVBoxLayout(detailsBox);
    detailsLayout->addWidget(d->detailsLbl);
    detailsBox->setLayout(detailsLayout);

    d->progressBar = new QProgressBar(d->progressContainer);
    d->progressBar->setFormat(tr("%p% - %v / %m files"));
    d->progressBar->setMaximum(0);
    d->progressBar->setMinimum(0);
    d->progressBar->setValue(0);

    progressLayout->addWidget(detailsBox);
    //progressLayout->addStretch(1);
    progressLayout->addWidget(newHorizLine(d->progressContainer));
    progressLayout->addWidget(d->progressBar);
    d->progressContainer->setLayout(progressLayout);


    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);
    mainLayout->addWidget(d->mainContainer);
    mainLayout->addWidget(d->progressContainer);
    setLayout(mainLayout);
}

void MainWindow::setWidgetActions()
{
    Q_D(MainWindow);

    void (QComboBox::*indexChangedInt)(int) =
            &QComboBox::currentIndexChanged;
    void (QComboBox::*indexChangedQString)(const QString &) =
            &QComboBox::currentIndexChanged;

    // Device
    connect(d->deviceSel, indexChangedInt,
            this, &MainWindow::onDeviceSelected);

    // Buttons
    connect(d->buttons, &QDialogButtonBox::clicked,
            this, &MainWindow::onButtonClicked);

    // Preset
    connect(d->presetSel, indexChangedQString,
            this, &MainWindow::onPresetSelected);

    // Has boot image checkbox
    connect(d->hasBootImageCb, &QCheckBox::toggled,
            this, &MainWindow::onHasBootImageToggled);
}

bool sortByPatchInfoId(PatchInfo *pi1, PatchInfo *pi2)
{
    return boost::ilexicographical_compare(pi1->id(), pi2->id());
}

void MainWindow::populateWidgets()
{
    Q_D(MainWindow);

    // Populate devices
    for (Device *device : d->pc->devices()) {
        d->deviceSel->addItem(QStringLiteral("%1 (%2)")
                .arg(QString::fromStdString(device->id()))
                .arg(QString::fromStdString(device->name())));
    }
}

void MainWindow::setWidgetDefaults()
{
    Q_D(MainWindow);

    // Don't remove device checks
    d->deviceCheckCb->setChecked(false);

    // Assume boot image exists
    d->hasBootImageCb->setChecked(true);
    onHasBootImageToggled();
}

void MainWindow::refreshPresets()
{
    Q_D(MainWindow);

    d->presetSel->clear();
    d->patchInfos.clear();

    d->presetSel->addItem(tr("Custom"));
    d->patchInfos = d->pc->patchInfos(d->device);
    std::sort(d->patchInfos.begin(), d->patchInfos.end(), sortByPatchInfoId);
    for (PatchInfo *info : d->patchInfos) {
        d->presetSel->addItem(QString::fromStdString(info->id()));
    }
}

void MainWindow::chooseFile()
{
    Q_D(MainWindow);

    QString fileName = QFileDialog::getOpenFileName(
            this, QString(), QString(), tr("Zip files (*.zip)"));
    if (fileName.isNull()) {
        return;
    }

    d->state = MainWindowPrivate::ChoseFile;

    d->fileName = fileName;

    checkSupported();
    updateWidgetsVisibility();
}

void MainWindow::checkSupported()
{
    Q_D(MainWindow);

    d->supported = MainWindowPrivate::NotSupported;

    if (d->state == MainWindowPrivate::ChoseFile) {
        // If the patcher doesn't use the patchinfo files, then just
        // assume everything is supported.
        if (!d->patcher->usesPatchInfo()) {
            d->supported |= MainWindowPrivate::SupportedFile;
        }

        // Otherwise, check if it really is supported
        else if ((d->patchInfo = d->pc->findMatchingPatchInfo(
                d->device, d->fileName.toStdString())) != nullptr) {
            d->supported |= MainWindowPrivate::SupportedFile;
        }
    }
}

void MainWindow::updateWidgetsVisibility()
{
    Q_D(MainWindow);

    d->mainContainer->setVisible(
            d->state != MainWindowPrivate::Patching);
    d->progressContainer->setVisible(
            d->state == MainWindowPrivate::Patching);

    for (QWidget *widget : d->unsupportedWidgets) {
        widget->setVisible(d->state == MainWindowPrivate::ChoseFile
                && d->supported == MainWindowPrivate::NotSupported);
    }

    d->chooseFileBtn->setVisible(
            d->state == MainWindowPrivate::FirstRun);
    d->chooseAnotherFileBtn->setVisible(
            d->state != MainWindowPrivate::FirstRun);
    d->startPatchingBtn->setVisible(
            d->state == MainWindowPrivate::ChoseFile);
    d->startPatchingBtn->setEnabled(true);

    for (QWidget *widget : d->messageWidgets) {
        widget->setVisible(d->state != MainWindowPrivate::FirstRun);
    }

    if (d->state == MainWindowPrivate::ChoseFile) {
        QString message = tr("File: %1").arg(d->fileName);
        QString newLines(QStringLiteral("\n\n"));

        if ((d->supported & MainWindowPrivate::SupportedFile) == 0) {
            message.append(newLines);
            message.append(tr("The file you have selected is not supported. You can attempt to patch the file anyway using the options below."));
        } else {
            // If the patcher uses patchinfo files, show the detected message
            if (d->patcher->usesPatchInfo()) {
                message.append(newLines);
                message.append(tr("Detected %1")
                        .arg(QString::fromStdString(d->patchInfo->name())));
            }
        }

        d->messageLbl->setText(message);
    } else if (d->state == MainWindowPrivate::FinishedPatching) {
        QString message;

        if (d->patcherFailed) {
            message.append(tr("Failed to patch file: %1\n\n").arg(d->fileName));
            message.append(d->patcherError);
        } else {
            message.append(tr("New file: %1\n\n").arg(d->patcherNewFile));
            message.append(tr("Successfully patched file"));
        }

        d->messageLbl->setText(message);
    }
}

void MainWindow::startPatching()
{
    Q_D(MainWindow);

    d->progressBar->setMaximum(0);
    d->progressBar->setValue(0);
    d->detailsLbl->clear();

    if (!d->supported) {
        if (d->presetSel->currentIndex() == 0) {
            d->patchInfo = new PatchInfo();

            d->patchInfo->addAutoPatcher(PatchInfo::Default,
                                         "StandardPatcher",
                                         PatchInfo::AutoPatcherArgs());
            d->patchInfo->setHasBootImage(PatchInfo::Default,
                                          d->hasBootImageCb->isChecked());
            if (d->patchInfo->hasBootImage(PatchInfo::Default)) {
                d->patchInfo->setRamdisk(PatchInfo::Default,
                                         d->device->id() + "/default");
                QString text = d->bootImageLe->text().trimmed();
                if (!text.isEmpty()) {
                    const std::string textStdString = text.toStdString();
                    std::vector<std::string> bootImages;
                    boost::split(bootImages, textStdString, boost::is_any_of(","));
                    d->patchInfo->setBootImages(PatchInfo::Default, bootImages);
                }
            }

            d->patchInfo->setDeviceCheck(PatchInfo::Default,
                                         !d->deviceCheckCb->isChecked());
        } else {
            d->patchInfo = d->patchInfos[d->presetSel->currentIndex() - 1];
        }
    }

    d->state = MainWindowPrivate::Patching;
    updateWidgetsVisibility();

    FileInfoPtr fileInfo = new FileInfo();
    fileInfo->setFilename(d->fileName.toStdString());
    fileInfo->setDevice(d->device);
    fileInfo->setPatchInfo(d->patchInfo);

    emit runThread(d->patcher, fileInfo);
}

QWidget * MainWindow::newHorizLine(QWidget *parent)
{
    QFrame *frame = new QFrame(parent);
    frame->setFrameShape(QFrame::HLine);
    frame->setFrameShadow(QFrame::Sunken);
    frame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    return frame;
}


static QString errorToString(PatcherError error) {
    switch (error.errorCode()) {
    case MBP::ErrorCode::NoError:
        return QObject::tr("No error has occurred");
    case MBP::ErrorCode::UnknownError:
        return QObject::tr("An unknown error has occurred");
    case MBP::ErrorCode::PatcherCreateError:
        return QObject::tr("Failed to create patcher: %1")
                .arg(QString::fromStdString(error.patcherId()));
    case MBP::ErrorCode::AutoPatcherCreateError:
        return QObject::tr("Failed to create autopatcher: %1")
                .arg(QString::fromStdString(error.patcherId()));
    case MBP::ErrorCode::RamdiskPatcherCreateError:
        return QObject::tr("Failed to create ramdisk patcher: %1")
                .arg(QString::fromStdString(error.patcherId()));
    case MBP::ErrorCode::FileOpenError:
        return QObject::tr("Failed to open file: %1")
                .arg(QString::fromStdString(error.filename()));
    case MBP::ErrorCode::FileReadError:
        return QObject::tr("Failed to read from file: %1")
                .arg(QString::fromStdString(error.filename()));
    case MBP::ErrorCode::FileWriteError:
        return QObject::tr("Failed to write to file: %1")
                .arg(QString::fromStdString(error.filename()));
    case MBP::ErrorCode::DirectoryNotExistError:
        return QObject::tr("Directory does not exist: %1")
                .arg(QString::fromStdString(error.filename()));
    case MBP::ErrorCode::BootImageSmallerThanHeaderError:
        return QObject::tr("The boot image file is smaller than the boot image header size");
    case MBP::ErrorCode::BootImageNoAndroidHeaderError:
        return QObject::tr("Could not find the Android header in the boot image");
    case MBP::ErrorCode::BootImageNoRamdiskGzipHeaderError:
        return QObject::tr("Could not find the ramdisk's gzip header");
    case MBP::ErrorCode::BootImageNoRamdiskAddressError:
        return QObject::tr("Could not determine the ramdisk's memory address");
    case MBP::ErrorCode::CpioFileAlreadyExistsError:
        return QObject::tr("File already exists in cpio archive: %1")
                .arg(QString::fromStdString(error.filename()));
    case MBP::ErrorCode::CpioFileNotExistError:
        return QObject::tr("File does not exist in cpio archive: %1")
                .arg(QString::fromStdString(error.filename()));
    case MBP::ErrorCode::ArchiveReadOpenError:
        return QObject::tr("Failed to open archive for reading");
    case MBP::ErrorCode::ArchiveReadDataError:
        return QObject::tr("Failed to read archive data for file: %1")
                .arg(QString::fromStdString(error.filename()));
    case MBP::ErrorCode::ArchiveReadHeaderError:
        return QObject::tr("Failed to read archive entry header");
    case MBP::ErrorCode::ArchiveWriteOpenError:
        return QObject::tr("Failed to open archive for writing");
    case MBP::ErrorCode::ArchiveWriteDataError:
        return QObject::tr("Failed to write archive data for file: %1")
                .arg(QString::fromStdString(error.filename()));
    case MBP::ErrorCode::ArchiveWriteHeaderError:
        return QObject::tr("Failed to write archive header for file: %1")
                .arg(QString::fromStdString(error.filename()));
    case MBP::ErrorCode::ArchiveCloseError:
        return QObject::tr("Failed to close archive");
    case MBP::ErrorCode::ArchiveFreeError:
        return QObject::tr("Failed to free archive header memory");
    case MBP::ErrorCode::XmlParseFileError:
        return QObject::tr("Failed to parse XML file: %1")
                .arg(QString::fromStdString(error.filename()));
    case MBP::ErrorCode::OnlyZipSupported:
        return QObject::tr("Only ZIP files are supported by %1")
                .arg(QString::fromStdString(error.patcherId()));
    case MBP::ErrorCode::OnlyBootImageSupported:
        return QObject::tr("Only boot images are supported by %1")
                .arg(QString::fromStdString(error.patcherId()));
    case MBP::ErrorCode::PatchingCancelled:
        return QObject::tr("Patching was cancelled");
    case MBP::ErrorCode::SystemCacheFormatLinesNotFound:
        return QObject::tr("The patcher could not find any /system or /cache"
                           "formatting lines in the updater-script file.\n\n"
                           "If the file is a ROM, then something failed. If the"
                           "file is not a ROM (eg. kernel or mod), it doesn't"
                           "need to be patched.");
    default:
        assert(false);
    }

    return QString();
}


PatcherTask::PatcherTask(QWidget *parent)
    : QObject(parent)
{
}

static void maxProgressUpdatedCbWrapper(int max, void *userData)
{
    PatcherTask *task = static_cast<PatcherTask *>(userData);
    task->maxProgressUpdatedCb(max);
}

static void progressUpdatedCbWrapper(int value, void *userData)
{
    PatcherTask *task = static_cast<PatcherTask *>(userData);
    task->progressUpdatedCb(value);
}

static void detailsUpdatedCbWrapper(const std::string &text, void *userData)
{
    PatcherTask *task = static_cast<PatcherTask *>(userData);
    task->detailsUpdatedCb(text);
}

void PatcherTask::patch(PatcherPtr patcher, FileInfoPtr info)
{
    patcher->setFileInfo(info);

    bool ret = patcher->patchFile(&maxProgressUpdatedCbWrapper,
                                  &progressUpdatedCbWrapper,
                                  &detailsUpdatedCbWrapper,
                                  this);

    QString newFile(QString::fromStdString(patcher->newFilePath()));

    patcher->setFileInfo(nullptr);
    delete info;

    if (!ret) {
        emit finished(QString(), true, errorToString(patcher->error()));
    } else {
        emit finished(newFile, false, QString());
    }
}

void PatcherTask::maxProgressUpdatedCb(int max)
{
    emit maxProgressUpdated(max);
}

void PatcherTask::progressUpdatedCb(int value)
{
    emit progressUpdated(value);
}

void PatcherTask::detailsUpdatedCb(const std::string &text)
{
    emit detailsUpdated(QString::fromStdString(text));
}
