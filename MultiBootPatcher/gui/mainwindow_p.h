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

#ifndef MAINWINDOW_P_H
#define MAINWINDOW_P_H

#include "mainwindow.h"
#include "qcombocheckbox.h"

#include <libdbp/patcherconfig.h>
#include <libdbp/patcherinterface.h>

#include <memory>

#include <QtCore/QThread>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>

class MainWindowPrivate
{
public:
    enum State {
        FirstRun,
        ChoseFile,
        Patching,
        FinishedPatching,
    };

    enum SupportedFlag {
        NotSupported = 0x0,
        SupportedFile = 0x1,
        SupportedPartConfig = 0x2
    };
    Q_DECLARE_FLAGS(SupportedFlags, SupportedFlag)

    // Current state of the patcher
    State state = FirstRun;

    // Selected file
    QString fileName;
    bool autoMode;

    PatcherConfig *pc = nullptr;

    // Selected patcher
    QHash<QString, QString> reversePatcherMap;
    Patcher *patcher = nullptr;
    PatchInfo *patchInfo = nullptr;

    // Level of support of the file
    SupportedFlags supported;

    // Patcher finish status and error message
    QString patcherNewFile;
    bool patcherFailed;
    QString patcherError;

    // Threads
    QThread *thread;
    PatcherTask *task;

    // Selected device
    Device *device = nullptr;

    // List of partition configs for current patcher
    QList<PartitionConfig *> partConfigs;

    // List of available patchinfos
    std::vector<PatchInfo *> patchInfos;

    QWidget *mainContainer;
    QWidget *progressContainer;

    QList<QWidget *> messageWidgets;
    QList<QWidget *> unsupportedWidgets;
    QList<QWidget *> customPresetWidgets;
    QList<QWidget *> bootImageWidgets;

    // Main widgets
    QLabel *deviceLbl;
    QLabel *patcherLbl;
    QLabel *partConfigLbl;
    QComboBox *deviceSel;
    QComboBox *patcherSel;
    QComboBox *partConfigSel;
    QLabel *partConfigDesc;

    QLabel *messageLbl;

    // Unsupported file widgets
    QLabel *presetLbl;
    QComboBox *presetSel;
    QLabel *autoPatcherLbl;
    QComboCheckBox *autoPatcherSel;
    QLabel *deviceCheckLbl;
    QCheckBox *deviceCheckCb;
    QLabel *hasBootImageLbl;
    QCheckBox *hasBootImageCb;
    QLabel *bootImageLbl;
    QLineEdit *bootImageLe;
    QLabel *patchedInitLbl;
    QCheckBox *patchedInitCb;
    QLabel *initFileLbl;
    QComboBox *initFileSel;
    QLabel *ramdiskLbl;
    QComboBox *ramdiskSel;

    // Progress
    QLabel *detailsLbl;
    QProgressBar *progressBar;

    // Buttons
    QDialogButtonBox *buttons;
    QPushButton *chooseFileBtn;
    QPushButton *chooseAnotherFileBtn;
    QPushButton *startPatchingBtn;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MainWindowPrivate::SupportedFlags)

#endif // MAINWINDOW_P_H
