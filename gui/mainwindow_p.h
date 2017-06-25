/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of DualBootPatcher
 *
 * DualBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DualBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DualBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MAINWINDOW_P_H
#define MAINWINDOW_P_H

#include "mainwindow.h"

#include <mbdevice/device.h>
#include <mbp/patcherconfig.h>
#include <mbp/patcherinterface.h>

#include <memory>
#include <vector>

#include <QtCore/QSettings>
#include <QtCore/QThread>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenu>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>

typedef std::unique_ptr<Device, void (*)(Device *)> ScopedDevice;

class InstallLocation
{
public:
    QString id;
    QString name;
    QString description;
};

class MainWindowPrivate
{
public:
    enum State {
        FirstRun,
        ChoseFile,
        Patching,
        FinishedPatching,
    };

    MainWindowPrivate();

    uint64_t bytes;
    uint64_t maxBytes;
    uint64_t files;
    uint64_t maxFiles;

    QSettings settings;

    // Current state of the patcher
    State state = FirstRun;

    // Selected file
    QString patcherId;
    QString fileName;
    bool autoMode;

    mbp::PatcherConfig *pc = nullptr;
    std::vector<ScopedDevice> devices;

    // Selected patcher
    mbp::Patcher *patcher = nullptr;

    // Patcher finish status and error message
    QString patcherNewFile;
    bool patcherFailed;
    QString patcherError;

    // Threads
    QThread *thread;
    PatcherTask *task;

    // Selected device
    Device *device = nullptr;

    // List of installation locations
    QList<InstallLocation> instLocs;

    QWidget *mainContainer;
    QWidget *progressContainer;

    QList<QWidget *> messageWidgets;

    // Main widgets
    QLabel *deviceLbl;
    QComboBox *deviceSel;
    QLabel *instLocLbl;
    QComboBox *instLocSel;
    QLabel *instLocDesc;
    QLineEdit *instLocLe;

    QLabel *messageLbl;

    // Progress
    QLabel *detailsLbl;
    QProgressBar *progressBar;

    // Menus
    QMenu *chooseFileMenu;
    QAction *chooseFlashableZip;
    QAction *chooseOdinImage;

    // Buttons
    QDialogButtonBox *buttons;
    QPushButton *chooseFileBtn;
    QPushButton *chooseAnotherFileBtn;
    QPushButton *startPatchingBtn;
};

#endif // MAINWINDOW_P_H
