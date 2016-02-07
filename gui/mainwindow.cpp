/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <cassert>

#include <mbp/errors.h>

#include <QtCore/QStringBuilder>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>


const int patcherPtrTypeId = qRegisterMetaType<PatcherPtr>("PatcherPtr");
const int fileInfoPtrTypeId = qRegisterMetaType<FileInfoPtr>("FileInfoPtr");
const int uint64TypeId = qRegisterMetaType<uint64_t>("uint64_t");

MainWindowPrivate::MainWindowPrivate()
    : settings(qApp->applicationDirPath() % QStringLiteral("/settings.ini"),
               QSettings::IniFormat)
{
}

MainWindow::MainWindow(mbp::PatcherConfig *pc, QWidget *parent)
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
    refreshInstallationLocations();
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
    connect(d->task, &PatcherTask::progressUpdated,
            this, &MainWindow::onProgressUpdated);
    connect(d->task, &PatcherTask::filesUpdated,
            this, &MainWindow::onFilesUpdated);
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

    if (d->state == MainWindowPrivate::FinishedPatching) {
        d->state = MainWindowPrivate::ChoseFile;
    }

    updateWidgetsVisibility();
}

void MainWindow::onInstallationLocationSelected(int index)
{
    Q_D(MainWindow);

    if (index < 0) {
        return;
    } else if (index < d->instLocs.size()) {
        d->instLocDesc->setText(d->instLocs[index].description);
        d->instLocLe->setVisible(false);
        d->buttons->setEnabled(true);
    } else if (index == d->instLocs.size()) {
        updateRomIdDescText(d->instLocLe->text());
        d->instLocLe->setVisible(true);
    } else if (index == d->instLocs.size() + 1) {
        updateRomIdDescText(d->instLocLe->text());
        d->instLocLe->setVisible(true);
    }
}

void MainWindow::onInstallationLocationIdChanged(const QString &text)
{
    updateRomIdDescText(text);
}

void MainWindow::updateRomIdDescText(const QString &text)
{
    Q_D(MainWindow);

    d->buttons->setEnabled(!text.isEmpty());

    if (text.isEmpty()) {
        d->instLocDesc->setText(tr("Enter an ID above"));
    } else {
        QString location;

        if (d->instLocSel->currentIndex() == d->instLocs.size()) {
            location = QStringLiteral("/data/multiboot/data-slot-%1")
                .arg(d->instLocLe->text());
        } else if (d->instLocSel->currentIndex() == d->instLocs.size() + 1) {
            location = QStringLiteral("[External SD]/multiboot/extsd-slot-%1")
                .arg(d->instLocLe->text());
        }

        d->instLocDesc->setText(tr("Installs ROM to %1").arg(location));
    }
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

void MainWindow::onProgressUpdated(uint64_t bytes, uint64_t maxBytes)
{
    Q_D(MainWindow);

    // Normalize values to 1000000
    static const int normalize = 1000000;

    int value;
    int max;
    if (maxBytes == 0) {
        value = 0;
        max = 0;
    } else {
        value = (double) bytes / maxBytes * normalize;
        max = normalize;
    }

    d->progressBar->setMaximum(max);
    d->progressBar->setValue(value);
    d->bytes = bytes;
    d->maxBytes = maxBytes;

    updateProgressText();
}

void MainWindow::onFilesUpdated(uint64_t files, uint64_t maxFiles)
{
    Q_D(MainWindow);

    d->files = files;
    d->maxFiles = maxFiles;

    updateProgressText();
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

void MainWindow::updateProgressText()
{
    Q_D(MainWindow);

    double percentage = 0.0;
    if (d->maxBytes != 0) {
        percentage = 100.0 * d->bytes / d->maxBytes;
    }

    d->progressBar->setFormat(tr("%1% - %2 / %3 files")
            .arg(percentage, 0, 'f', 2).arg(d->files).arg(d->maxFiles));
}

void MainWindow::addWidgets()
{
    Q_D(MainWindow);

    int i = 0;

    d->mainContainer = new QWidget(this);

    // Selectors and file chooser
    d->deviceSel = new QComboBox(d->mainContainer);
    d->instLocSel = new QComboBox(d->mainContainer);
    d->instLocDesc = new QLabel(d->mainContainer);

    // Labels
    d->deviceLbl = new QLabel(tr("Device:"), d->mainContainer);
    d->instLocLbl = new QLabel(tr("Install to:"), d->mainContainer);

    // Text boxes
    d->instLocLe = new QLineEdit(d->mainContainer);
    d->instLocLe->setPlaceholderText(tr("Enter an ID"));
    QRegExp re(QStringLiteral("[a-z0-9]+"));
    QValidator *validator = new QRegExpValidator(re, this);
    d->instLocLe->setValidator(validator);

    QGridLayout *layout = new QGridLayout(d->mainContainer);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(d->deviceLbl, i, 0);
    layout->addWidget(d->deviceSel, i, 1, 1, -1);
    layout->addWidget(d->instLocLbl, ++i, 0);
    layout->addWidget(d->instLocSel, i, 1, 1, -1);
    layout->addWidget(d->instLocLe, ++i, 1, 1, -1);
    layout->addWidget(d->instLocDesc, ++i, 1, 1, -1);

    d->messageLbl = new QLabel(d->mainContainer);
    // Don't allow the window to grow too big
    d->messageLbl->setWordWrap(true);
    d->messageLbl->setMaximumWidth(550);

    d->chooseFileBtn = new QPushButton(tr("Choose file"), d->mainContainer);
    d->chooseAnotherFileBtn = new QPushButton(tr("Choose another file"), d->mainContainer);
    d->startPatchingBtn = new QPushButton(tr("Start patching"), d->mainContainer);

    d->buttons = new QDialogButtonBox(d->mainContainer);
    d->buttons->addButton(d->chooseFileBtn, QDialogButtonBox::ActionRole);
    d->buttons->addButton(d->chooseAnotherFileBtn, QDialogButtonBox::ActionRole);
    d->buttons->addButton(d->startPatchingBtn, QDialogButtonBox::ActionRole);

    QWidget *horiz1 = newHorizLine(d->mainContainer);

    layout->setColumnStretch(0, 0);
    layout->setColumnStretch(1, 0);
    layout->setColumnStretch(2, 0);
    layout->setColumnStretch(3, 1);

    layout->addWidget(horiz1,             ++i, 0, 1, -1);
    layout->addWidget(d->messageLbl,      ++i, 0, 1, -1);

    layout->addWidget(newHorizLine(d->mainContainer), ++i, 0, 1, -1);

    layout->addWidget(d->buttons,         ++i, 0, 1, -1);


    d->mainContainer->setLayout(layout);

    // List of widgets related to the message label
    d->messageWidgets << horiz1;
    d->messageWidgets << d->messageLbl;

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
    //d->progressBar->setFormat(tr("%p% - %v / %m files"));
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

    // Device
    connect(d->deviceSel, indexChangedInt,
            this, &MainWindow::onDeviceSelected);

    // Installation location
    connect(d->instLocSel, indexChangedInt,
            this, &MainWindow::onInstallationLocationSelected);
    connect(d->instLocLe, &QLineEdit::textChanged,
            this, &MainWindow::onInstallationLocationIdChanged);

    // Buttons
    connect(d->buttons, &QDialogButtonBox::clicked,
            this, &MainWindow::onButtonClicked);
}

void MainWindow::populateWidgets()
{
    Q_D(MainWindow);

    // Populate devices
    for (mbp::Device *device : d->pc->devices()) {
        d->deviceSel->addItem(QStringLiteral("%1 - %2")
                .arg(QString::fromStdString(device->id()))
                .arg(QString::fromStdString(device->name())));
    }
}

void MainWindow::refreshInstallationLocations()
{
    Q_D(MainWindow);

    d->instLocs << InstallLocation{
        QStringLiteral("primary"),
        tr("Primary ROM Upgrade"),
        tr("Update primary ROM without affecting other ROMS")
    };
    d->instLocs << InstallLocation{
        QStringLiteral("dual"),
        tr("Secondary"),
        tr("Installs ROM to /system/multiboot/dual")
    };
    d->instLocs << InstallLocation{
        QStringLiteral("multi-slot-1"),
        tr("Multi-slot 1"),
        tr("Installs ROM to /cache/multiboot/multi-slot-1")
    };
    d->instLocs << InstallLocation{
        QStringLiteral("multi-slot-2"),
        tr("Multi-slot 2"),
        tr("Installs ROM to /cache/multiboot/multi-slot-2")
    };
    d->instLocs << InstallLocation{
        QStringLiteral("multi-slot-3"),
        tr("Multi-slot 3"),
        tr("Installs ROM to /cache/multiboot/multi-slot-3")
    };

    for (const InstallLocation &instLoc : d->instLocs) {
        d->instLocSel->addItem(instLoc.name);
    }

    d->instLocSel->addItem(tr("Data-slot"));
    d->instLocSel->addItem(tr("Extsd-slot"));
}

void MainWindow::chooseFile()
{
    Q_D(MainWindow);

    QString fileName = QFileDialog::getOpenFileName(this, QString(),
            d->settings.value(QStringLiteral("last_dir")).toString(),
            tr("Zip files (*.zip)"));
    if (fileName.isNull()) {
        return;
    }

    d->settings.setValue(QStringLiteral("last_dir"),
                         QFileInfo(fileName).dir().absolutePath());

    d->state = MainWindowPrivate::ChoseFile;

    d->fileName = fileName;

    updateWidgetsVisibility();
}

void MainWindow::updateWidgetsVisibility()
{
    Q_D(MainWindow);

    d->mainContainer->setVisible(
            d->state != MainWindowPrivate::Patching);
    d->progressContainer->setVisible(
            d->state == MainWindowPrivate::Patching);

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
        d->messageLbl->setText(tr("File: %1").arg(d->fileName));
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

    d->bytes = 0;
    d->maxBytes = 0;
    d->files = 0;
    d->maxFiles = 0;

    d->progressBar->setMaximum(0);
    d->progressBar->setValue(0);
    d->detailsLbl->clear();

    d->state = MainWindowPrivate::Patching;
    updateWidgetsVisibility();

    QString romId;
    if (d->instLocSel->currentIndex() == d->instLocs.size()) {
        romId = QStringLiteral("data-slot-%1").arg(d->instLocLe->text());
    } else if (d->instLocSel->currentIndex() == d->instLocs.size() + 1) {
        romId = QStringLiteral("extsd-slot-%1").arg(d->instLocLe->text());
    } else {
        romId = d->instLocs[d->instLocSel->currentIndex()].id;
    }


    QFileInfo qFileInfo(d->fileName);

    // Output file name is <parent path>/<base name>_<rom id>.<suffix>
    QString outputName(qFileInfo.completeBaseName()
            % QStringLiteral("_")
            % romId
            % QStringLiteral(".")
            % qFileInfo.suffix());

    QString inputPath(QDir::toNativeSeparators(qFileInfo.filePath()));
    QString outputPath(QDir::toNativeSeparators(
            qFileInfo.dir().filePath(outputName)));

    FileInfoPtr fileInfo = new mbp::FileInfo();
    fileInfo->setInputPath(inputPath.toUtf8().constData());
    fileInfo->setOutputPath(outputPath.toUtf8().constData());
    fileInfo->setDevice(d->device);
    fileInfo->setRomId(romId.toUtf8().constData());

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


static QString errorToString(const mbp::ErrorCode &error) {
    switch (error) {
    case mbp::ErrorCode::NoError:
        return QObject::tr("No error has occurred");
    case mbp::ErrorCode::PatcherCreateError:
        return QObject::tr("Failed to create patcher");
    case mbp::ErrorCode::AutoPatcherCreateError:
        return QObject::tr("Failed to create autopatcher");
    case mbp::ErrorCode::RamdiskPatcherCreateError:
        return QObject::tr("Failed to create ramdisk patcher");
    case mbp::ErrorCode::FileOpenError:
        return QObject::tr("Failed to open file");
    case mbp::ErrorCode::FileReadError:
        return QObject::tr("Failed to read from file");
    case mbp::ErrorCode::FileWriteError:
        return QObject::tr("Failed to write to file");
    case mbp::ErrorCode::BootImageParseError:
        return QObject::tr("Failed to parse boot image");
    case mbp::ErrorCode::BootImageApplyBumpError:
        return QObject::tr("Failed to apply Bump to the boot image");
    case mbp::ErrorCode::BootImageApplyLokiError:
        return QObject::tr("Failed to apply Loki to the boot image");
    case mbp::ErrorCode::CpioFileAlreadyExistsError:
        return QObject::tr("File already exists in cpio archive");
    case mbp::ErrorCode::CpioFileNotExistError:
        return QObject::tr("File does not exist in cpio archive");
    case mbp::ErrorCode::ArchiveReadOpenError:
        return QObject::tr("Failed to open archive for reading");
    case mbp::ErrorCode::ArchiveReadDataError:
        return QObject::tr("Failed to read archive data for file");
    case mbp::ErrorCode::ArchiveReadHeaderError:
        return QObject::tr("Failed to read archive entry header");
    case mbp::ErrorCode::ArchiveWriteOpenError:
        return QObject::tr("Failed to open archive for writing");
    case mbp::ErrorCode::ArchiveWriteDataError:
        return QObject::tr("Failed to write archive data for file");
    case mbp::ErrorCode::ArchiveWriteHeaderError:
        return QObject::tr("Failed to write archive header for file");
    case mbp::ErrorCode::ArchiveCloseError:
        return QObject::tr("Failed to close archive");
    case mbp::ErrorCode::ArchiveFreeError:
        return QObject::tr("Failed to free archive header memory");
    case mbp::ErrorCode::PatchingCancelled:
        return QObject::tr("Patching was cancelled");
    default:
        assert(false);
    }

    return QString();
}


PatcherTask::PatcherTask(QWidget *parent)
    : QObject(parent)
{
}

static void progressUpdatedCbWrapper(uint64_t bytes, uint64_t maxBytes,
                                     void *userData)
{
    PatcherTask *task = static_cast<PatcherTask *>(userData);
    task->progressUpdatedCb(bytes, maxBytes);
}

static void filesUpdatedCbWrapper(uint64_t files, uint64_t maxFiles,
                                  void *userData)
{
    PatcherTask *task = static_cast<PatcherTask *>(userData);
    task->filesUpdatedCb(files, maxFiles);
}

static void detailsUpdatedCbWrapper(const std::string &text, void *userData)
{
    PatcherTask *task = static_cast<PatcherTask *>(userData);
    task->detailsUpdatedCb(text);
}

void PatcherTask::patch(PatcherPtr patcher, FileInfoPtr info)
{
    patcher->setFileInfo(info);

    bool ret = patcher->patchFile(&progressUpdatedCbWrapper,
                                  &filesUpdatedCbWrapper,
                                  &detailsUpdatedCbWrapper,
                                  this);

    QString newFile(QString::fromStdString(info->outputPath()));

    patcher->setFileInfo(nullptr);
    delete info;

    if (!ret) {
        emit finished(QString(), true, errorToString(patcher->error()));
    } else {
        emit finished(newFile, false, QString());
    }
}

void PatcherTask::progressUpdatedCb(uint64_t bytes, uint64_t maxBytes)
{
    emit progressUpdated(bytes, maxBytes);
}

void PatcherTask::filesUpdatedCb(uint64_t files, uint64_t maxFiles)
{
    emit filesUpdated(files, maxFiles);
}

void PatcherTask::detailsUpdatedCb(const std::string &text)
{
    emit detailsUpdated(QString::fromStdString(text));
}
