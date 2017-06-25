/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <mbp/fileinfo.h>
#include <mbp/patcherconfig.h>
#include <mbp/patcherinterface.h>

#include <QtCore/QMetaType>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QWidget>


typedef mbp::Patcher * PatcherPtr;
Q_DECLARE_METATYPE(PatcherPtr)
typedef mbp::FileInfo * FileInfoPtr;
Q_DECLARE_METATYPE(FileInfoPtr)

class MainWindowPrivate;

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow(mbp::PatcherConfig *pc, QWidget* parent = 0);
    ~MainWindow();

signals:
    void runThread(PatcherPtr patcher, FileInfoPtr info);

private slots:
    void onDeviceSelected(int index);
    void onInstallationLocationSelected(int index);
    void onInstallationLocationIdChanged(const QString &text);
    void onButtonClicked(QAbstractButton *button);
    void onChooseFileItemClicked(QAction *action);

    // Progress
    void onProgressUpdated(uint64_t bytes, uint64_t maxBytes);
    void onFilesUpdated(uint64_t files, uint64_t maxFiles);
    void onDetailsUpdated(const QString &text);

    void onPatchingFinished(const QString &newFile, bool failed,
                            const QString &errorMessage);

private:
    virtual void closeEvent(QCloseEvent *event) override;

    void updateRomIdDescText(const QString &text);
    void updateProgressText();

    void addWidgets();
    void setWidgetActions();
    void populateDevices();
    void populateInstallationLocations();

    void chooseFile(const QString &patterns);
    void startPatching();

    void updateWidgetsVisibility();

    QWidget * newHorizLine(QWidget *parent = 0);

    const QScopedPointer<MainWindowPrivate> d_ptr;
    Q_DECLARE_PRIVATE(MainWindow)
};

class PatcherTaskPrivate;

class PatcherTask : public QObject
{
    Q_OBJECT

public:
    PatcherTask(QWidget *parent = 0);

    void patch(PatcherPtr patcher, FileInfoPtr info);

    void progressUpdatedCb(uint64_t bytes, uint64_t maxBytes);
    void filesUpdatedCb(uint64_t files, uint64_t maxFiles);
    void detailsUpdatedCb(const std::string &text);

signals:
    void finished(const QString &newFile, bool failed,
                  const QString &errorMessage);
    void progressUpdated(uint64_t bytes, uint64_t maxBytes);
    void filesUpdated(uint64_t files, uint64_t maxFiles);
    void detailsUpdated(const QString &text);
};

#endif // MAINWINDOW_H
