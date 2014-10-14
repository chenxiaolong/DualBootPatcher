/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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

#include <libdbp/partitionconfig.h>
#include <libdbp/patchinfo.h>

#include <QtCore/QStringBuilder>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>


MainWindow::MainWindow(PatcherPaths *pp, QWidget *parent)
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

    d->pp = pp;

    addWidgets();
    setWidgetActions();
    populateWidgets();
    setWidgetDefaults();
    updateWidgetsVisibility();
}

MainWindow::~MainWindow()
{
}

void MainWindow::onDeviceSelected(int index)
{
    Q_D(MainWindow);
    d->device = d->pp->devices()[index];

    refreshPresets();
    refreshRamdisks();

    if (d->state == MainWindowPrivate::FinishedPatching) {
        d->state = MainWindowPrivate::ChoseFile;
    }

    checkSupported();
    updateWidgetsVisibility();
}

void MainWindow::onPatcherSelected(const QString &patcher)
{
    Q_D(MainWindow);

    QString patcherId = d->reversePatcherMap[patcher];
    d->patcher = d->pp->createPatcher(patcherId);

    refreshPartConfigs();

    if (d->state == MainWindowPrivate::FinishedPatching) {
        d->state = MainWindowPrivate::ChoseFile;
    }

    checkSupported();
    updateWidgetsVisibility();
}

void MainWindow::onPartConfigSelected(int index)
{
    Q_D(MainWindow);

    if (index >= 0) {
        d->partConfigDesc->setText(d->partConfigs[index]->description());

        if (d->state == MainWindowPrivate::FinishedPatching) {
            d->state = MainWindowPrivate::ChoseFile;
        }

        checkSupported();
        updateWidgetsVisibility();
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

        onPatchedInitToggled();
    } else {
        for (QWidget *widget : d->bootImageWidgets) {
            widget->setEnabled(false);
        }
    }

    updateBootImageTextBox();
}

void MainWindow::onPatchedInitToggled()
{
    Q_D(MainWindow);

    d->initFileLbl->setEnabled(d->patchedInitCb->isChecked());
    d->initFileSel->setEnabled(d->patchedInitCb->isChecked());
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

void MainWindow::onThreadCompleted()
{
    Q_D(MainWindow);

    d->thread->deleteLater();
    d->thread = nullptr;

    d->task->deleteLater();
    d->task = nullptr;

    disconnect(d->patcher.data(), &Patcher::maxProgressUpdated,
               this, &MainWindow::onMaxProgressUpdated);
    disconnect(d->patcher.data(), &Patcher::progressUpdated,
               this, &MainWindow::onProgressUpdated);
    disconnect(d->patcher.data(), &Patcher::detailsUpdated,
               this, &MainWindow::onDetailsUpdated);
}

void MainWindow::addWidgets()
{
    Q_D(MainWindow);

    int i = 0;

    d->mainContainer = new QWidget(this);

    // Selectors and file chooser
    d->patcherSel = new QComboBox(d->mainContainer);
    d->deviceSel = new QComboBox(d->mainContainer);
    d->partConfigSel = new QComboBox(d->mainContainer);
    d->partConfigDesc = new QLabel(d->mainContainer);

    // Labels
    d->deviceLbl = new QLabel(tr("Device:"), d->mainContainer);
    d->patcherLbl = new QLabel(tr("Patcher:"), d->mainContainer);
    d->partConfigLbl = new QLabel(tr("Configuration:"), d->mainContainer);

    QGridLayout *layout = new QGridLayout(d->mainContainer);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(d->deviceLbl, i, 0);
    layout->addWidget(d->deviceSel, i, 1, 1, -1);
    layout->addWidget(d->patcherLbl, ++i, 0);
    layout->addWidget(d->patcherSel, i, 1, 1, -1);
    layout->addWidget(d->partConfigLbl, ++i, 0);
    layout->addWidget(d->partConfigSel, i, 1, 1, -1);
    layout->addWidget(d->partConfigDesc, ++i, 1, 1, -1);

    // Add items for unsupported files
    d->messageLbl = new QLabel(d->mainContainer);
    // Don't allow the window to grow too big
    d->messageLbl->setWordWrap(true);
    d->messageLbl->setMaximumWidth(550);
    d->presetLbl = new QLabel(tr("Preset:"), d->mainContainer);
    d->presetSel = new QComboBox(d->mainContainer);
    d->autoPatcherLbl = new QLabel(tr("Auto-patcher"), d->mainContainer);
    d->autoPatcherSel = new QComboCheckBox(d->mainContainer);
    d->autoPatcherSel->setAutoUpdateDisplayText(true);
    d->autoPatcherSel->setNothingSelectedText(tr("No autopatchers selected"));
    d->deviceCheckLbl = new QLabel(tr("Remove device check"), d->mainContainer);
    d->deviceCheckCb = new QCheckBox(d->mainContainer);
    d->hasBootImageLbl = new QLabel(tr("Has boot image"), d->mainContainer);
    d->hasBootImageCb = new QCheckBox(d->mainContainer);
    d->bootImageLbl = new QLabel(tr("Boot image"), d->mainContainer);
    d->bootImageLe = new QLineEdit(d->mainContainer);
    d->bootImageLe->setPlaceholderText(tr("Leave blank to autodetect"));
    d->patchedInitLbl = new QLabel(tr("Needs patched init"), d->mainContainer);
    d->patchedInitCb = new QCheckBox(d->mainContainer);
    d->initFileLbl = new QLabel(tr("Init file"), d->mainContainer);
    d->initFileSel = new QComboBox(d->mainContainer);
    d->ramdiskLbl = new QLabel(tr("Ramdisk"), d->mainContainer);
    d->ramdiskSel = new QComboBox(d->mainContainer);

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
    QWidget *horiz4 = newHorizLine(d->mainContainer);

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
    layout->addWidget(d->autoPatcherLbl,  ++i, 0, 1,  1);
    layout->addWidget(d->autoPatcherSel,    i, 2, 1, -1);
    layout->addWidget(d->deviceCheckLbl,  ++i, 0, 1,  1);
    layout->addWidget(d->deviceCheckCb,     i, 1, 1,  1);
    layout->addWidget(d->hasBootImageLbl, ++i, 0, 1,  1);
    layout->addWidget(d->hasBootImageCb,    i, 1, 1,  1);
    layout->addWidget(d->bootImageLbl,      i, 2, 1,  1);
    layout->addWidget(d->bootImageLe,       i, 3, 1, -1);
    layout->addWidget(horiz4,             ++i, 0, 1, -1);
    layout->addWidget(d->patchedInitLbl,  ++i, 0, 1,  1);
    layout->addWidget(d->patchedInitCb,     i, 1, 1,  1);
    layout->addWidget(d->initFileLbl,       i, 2, 1,  1);
    layout->addWidget(d->initFileSel,       i, 3, 1, -1);
    layout->addWidget(d->ramdiskLbl,      ++i, 2, 1,  1);
    layout->addWidget(d->ramdiskSel,        i, 3, 1, -1);


    layout->addWidget(newHorizLine(d->mainContainer), ++i, 0, 1, -1);

    layout->addWidget(d->buttons,         ++i, 0, 1, -1);


    d->mainContainer->setLayout(layout);

    // List of widgets related to the message label
    d->messageWidgets << horiz1;
    d->messageWidgets << d->messageLbl;

    // List of unsupported widgets
    d->unsupportedWidgets << horiz2;
    d->unsupportedWidgets << horiz3;
    d->unsupportedWidgets << horiz4;
    d->unsupportedWidgets << d->presetLbl;
    d->unsupportedWidgets << d->presetSel;
    d->unsupportedWidgets << d->autoPatcherLbl;
    d->unsupportedWidgets << d->autoPatcherSel;
    d->unsupportedWidgets << d->deviceCheckLbl;
    d->unsupportedWidgets << d->deviceCheckCb;
    d->unsupportedWidgets << d->hasBootImageLbl;
    d->unsupportedWidgets << d->hasBootImageCb;
    d->unsupportedWidgets << d->bootImageLbl;
    d->unsupportedWidgets << d->bootImageLe;
    d->unsupportedWidgets << d->patchedInitLbl;
    d->unsupportedWidgets << d->patchedInitCb;
    d->unsupportedWidgets << d->initFileLbl;
    d->unsupportedWidgets << d->initFileSel;
    d->unsupportedWidgets << d->ramdiskLbl;
    d->unsupportedWidgets << d->ramdiskSel;

    // List of custom preset widgets
    d->customPresetWidgets << d->autoPatcherLbl;
    d->customPresetWidgets << d->autoPatcherSel;
    d->customPresetWidgets << d->deviceCheckLbl;
    d->customPresetWidgets << d->deviceCheckCb;
    d->customPresetWidgets << d->hasBootImageLbl;
    d->customPresetWidgets << d->hasBootImageCb;
    d->customPresetWidgets << d->bootImageLbl;
    d->customPresetWidgets << d->bootImageLe;
    d->customPresetWidgets << d->patchedInitLbl;
    d->customPresetWidgets << d->patchedInitCb;
    d->customPresetWidgets << d->initFileLbl;
    d->customPresetWidgets << d->initFileSel;
    d->customPresetWidgets << d->ramdiskLbl;
    d->customPresetWidgets << d->ramdiskSel;

    // List of boot image-related widgets
    d->bootImageWidgets << d->bootImageLbl;
    d->bootImageWidgets << d->bootImageLe;
    d->bootImageWidgets << d->patchedInitLbl;
    d->bootImageWidgets << d->patchedInitCb;
    d->bootImageWidgets << d->initFileLbl;
    d->bootImageWidgets << d->initFileSel;
    d->bootImageWidgets << d->ramdiskLbl;
    d->bootImageWidgets << d->ramdiskSel;

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

    // Patcher
    connect(d->patcherSel, indexChangedQString,
            this, &MainWindow::onPatcherSelected);

    // Partition config
    connect(d->partConfigSel, indexChangedInt,
            this, &MainWindow::onPartConfigSelected);

    // Buttons
    connect(d->buttons, &QDialogButtonBox::clicked,
            this, &MainWindow::onButtonClicked);

    // Preset
    connect(d->presetSel, indexChangedQString,
            this, &MainWindow::onPresetSelected);

    // Has boot image checkbox
    connect(d->hasBootImageCb, &QCheckBox::toggled,
            this, &MainWindow::onHasBootImageToggled);

    // Needs patched init checkbox
    connect(d->patchedInitCb, &QCheckBox::toggled,
            this, &MainWindow::onPatchedInitToggled);
}

bool sortByPatchInfoPath(PatchInfo *pi1, PatchInfo *pi2)
{
    return QString::compare(pi1->path(), pi2->path(), Qt::CaseInsensitive) < 0;
}

void MainWindow::populateWidgets()
{
    Q_D(MainWindow);

    // Populate devices
    for (Device *device : d->pp->devices()) {
        d->deviceSel->addItem(QStringLiteral("%1 (%2)")
                .arg(device->codename()).arg(device->name()));
    }

    // Populate patchers
    QStringList patcherNames;
    for (const QString &id : d->pp->patchers()) {
        QString name = d->pp->patcherName(id);
        patcherNames << name;
        d->reversePatcherMap[name] = id;
    }
    patcherNames.sort();
    d->patcherSel->addItems(patcherNames);

    // Populate init binaries
    d->initFileSel->addItems(d->pp->initBinaries());

    // Populate autopatchers
    QStringList autoPatchers;
    for (const QString &id : d->pp->autoPatchers()) {
        if (id == QStringLiteral("PatchFile")) {
            continue;
        }
        autoPatchers << id;
    }
    autoPatchers.sort();
    d->autoPatcherSel->addItems(autoPatchers);
}

void MainWindow::setWidgetDefaults()
{
    Q_D(MainWindow);

    // Don't remove device checks
    d->deviceCheckCb->setChecked(false);

    // Assume boot image exists
    d->hasBootImageCb->setChecked(true);
    onHasBootImageToggled();

    // No patched init binary
    d->patchedInitCb->setChecked(false);
    onPatchedInitToggled();

    resetAutoPatchers();
}

void MainWindow::refreshPartConfigs()
{
    Q_D(MainWindow);

    d->partConfigSel->clear();
    d->partConfigs.clear();

    for (const QString &id : d->patcher->supportedPartConfigIds()) {
        PartitionConfig *config = d->pp->partitionConfig(id);
        if (config != nullptr) {
            d->partConfigs << config;
            d->partConfigSel->addItem(config->name(), config->id());
        }
    }
}

void MainWindow::refreshPresets()
{
    Q_D(MainWindow);

    d->presetSel->clear();
    d->patchInfos.clear();

    d->presetSel->addItem(tr("Custom"));
    d->patchInfos = d->pp->patchInfos(d->device);
    std::sort(d->patchInfos.begin(), d->patchInfos.end(), sortByPatchInfoPath);
    for (PatchInfo *info : d->patchInfos) {
        d->presetSel->addItem(info->path());
    }
}

void MainWindow::refreshRamdisks()
{
    Q_D(MainWindow);

    d->ramdiskSel->clear();

    QStringList ramdisks;
    for (const QString &id : d->pp->ramdiskPatchers()) {
        if (id.startsWith(d->device->codename())) {
            ramdisks << id;
        }
    }
    ramdisks.sort();
    d->ramdiskSel->addItems(ramdisks);
}

void MainWindow::updateBootImageTextBox()
{
    Q_D(MainWindow);

    // Disable the boot image textbox if the selected file is a boot image
    if (d->state == MainWindowPrivate::ChoseFile) {
        bool isImg = d->fileName.endsWith(
            QStringLiteral(".img"), Qt::CaseInsensitive);
        bool isLok = d->fileName.endsWith(
            QStringLiteral(".lok"), Qt::CaseInsensitive);
        d->bootImageLe->setEnabled(!isImg && !isLok);
    }
}

void MainWindow::resetAutoPatchers()
{
    Q_D(MainWindow);

    for (int i = 0; i < d->autoPatcherSel->count(); i++) {
        // Select StandardPatcher by default
        bool isStandardPatcher = d->autoPatcherSel->itemText(i)
                == QStringLiteral("StandardPatcher");

        d->autoPatcherSel->setChecked(i, isStandardPatcher);
    }
}

void MainWindow::chooseFile()
{
    Q_D(MainWindow);

    QString fileName = QFileDialog::getOpenFileName(
            this, QString(), QString(), tr("(*.zip *.img *.lok)"));
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
            d->supported |= MainWindowPrivate::SupportedPartConfig;
        }

        // Otherwise, check if it really is supported
        else if ((d->patchInfo = PatchInfo::findMatchingPatchInfo(
                d->pp, d->device, d->fileName)) != nullptr) {
            d->supported |= MainWindowPrivate::SupportedFile;

            QString key = d->patchInfo->keyFromFilename(d->fileName);
            QStringList configs = d->patchInfo->supportedConfigs(key);
            PartitionConfig *curConfig =
                    d->partConfigs[d->partConfigSel->currentIndex()];

            if ((configs.contains(QStringLiteral("all"))
                    || configs.contains(curConfig->id()))
                    && !configs.contains(QLatin1Char('!') % curConfig->id())) {
                d->supported |= MainWindowPrivate::SupportedPartConfig;
            }
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

    if (d->partConfigs.size() <= 1) {
        d->partConfigLbl->setVisible(false);
        d->partConfigSel->setVisible(false);
        d->partConfigDesc->setVisible(false);
    } else {
        d->partConfigLbl->setVisible(true);
        d->partConfigSel->setVisible(true);
        d->partConfigDesc->setVisible(true);
    }

    for (QWidget *widget : d->unsupportedWidgets) {
        widget->setVisible(d->state == MainWindowPrivate::ChoseFile
                && d->supported == MainWindowPrivate::NotSupported);
    }

    updateBootImageTextBox();

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
                message.append(tr("Detected %1").arg(d->patchInfo->name()));
            }

            // Otherwise, if the partition configuration is not supported, then
            // warn the user
            else if ((d->supported & MainWindowPrivate::SupportedPartConfig) == 0) {
                message.append(newLines);
                message.append(tr("The current partition configuration is not supported for this file"));
                d->startPatchingBtn->setEnabled(false);
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

            PatchInfo::AutoPatcherItems items;
            for (int i = 0; i < d->autoPatcherSel->count(); i++) {
                if (d->autoPatcherSel->isChecked(i)) {
                    items << PatchInfo::AutoPatcherItem(
                            d->autoPatcherSel->itemText(i),
                                    PatchInfo::AutoPatcherArgs());
                }
            }

            d->patchInfo->setAutoPatchers(PatchInfo::Default, items);

            d->patchInfo->setHasBootImage(PatchInfo::Default,
                                          d->hasBootImageCb->isChecked());
            if (d->patchInfo->hasBootImage(PatchInfo::Default)) {
                d->patchInfo->setRamdisk(PatchInfo::Default,
                                         d->ramdiskSel->currentText());
                QString text = d->bootImageLe->text().trimmed();
                if (!text.isEmpty()) {
                    d->patchInfo->setBootImages(PatchInfo::Default,
                                                text.split(QLatin1Char(',')));
                }

                if (d->patchedInitCb->isChecked()) {
                    d->patchInfo->setPatchedInit(PatchInfo::Default,
                                                 d->initFileSel->currentText());
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

    QSharedPointer<FileInfo> fileInfo(new FileInfo());
    fileInfo->setFilename(d->fileName);
    fileInfo->setDevice(d->device);
    if (!d->partConfigs.isEmpty()) {
        fileInfo->setPartConfig(d->partConfigs[d->partConfigSel->currentIndex()]);
    }
    fileInfo->setPatchInfo(d->patchInfo);

    d->thread = new QThread(this);
    d->task = new PatcherTask(d->patcher, fileInfo);
    d->task->moveToThread(d->thread);

    connect(d->thread, &QThread::finished,
            this, &MainWindow::onThreadCompleted);
    connect(d->thread, &QThread::started,
            d->task, &PatcherTask::patch);
    connect(d->task, &PatcherTask::finished,
            this, &MainWindow::onPatchingFinished);
    connect(d->patcher.data(), &Patcher::maxProgressUpdated,
            this, &MainWindow::onMaxProgressUpdated);
    connect(d->patcher.data(), &Patcher::progressUpdated,
            this, &MainWindow::onProgressUpdated);
    connect(d->patcher.data(), &Patcher::detailsUpdated,
            this, &MainWindow::onDetailsUpdated);

    d->thread->start();
}

QWidget * MainWindow::newHorizLine(QWidget *parent)
{
    QFrame *frame = new QFrame(parent);
    frame->setFrameShape(QFrame::HLine);
    frame->setFrameShadow(QFrame::Sunken);
    frame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    return frame;
}

PatcherTask::PatcherTask(QSharedPointer<Patcher> patcher,
                         QSharedPointer<FileInfo> info,
                         QWidget *parent)
    : QObject(parent), d_ptr(new PatcherTaskPrivate())
{
    Q_D(PatcherTask);

    d->patcher = patcher;
    d->info = info;
}

PatcherTask::~PatcherTask()
{
}

void PatcherTask::patch()
{
    Q_D(PatcherTask);

    d->patcher->setFileInfo(d->info.data());
    if (!d->patcher->patchFile()) {
        emit finished(QString(), true, d->patcher->errorString());
    } else {
        emit finished(d->patcher->newFilePath(), false, QString());
    }
}
