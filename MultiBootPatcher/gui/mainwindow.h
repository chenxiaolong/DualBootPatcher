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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <libdbp/fileinfo.h>
#include <libdbp/patcherinterface.h>

#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QWidget>

class MainWindowPrivate;

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow(PatcherPaths* pp, QWidget* parent = 0);
    ~MainWindow();

private slots:
    void onDeviceSelected(int index);
    void onPatcherSelected(const QString &patcher);
    void onPartConfigSelected(int index);
    void onButtonClicked(QAbstractButton *button);

    // Unsupported files
    void onPresetSelected(const QString &preset);
    void onHasBootImageToggled();
    void onPatchedInitToggled();

    // Progress
    void onMaxProgressUpdated(int max);
    void onProgressUpdated(int value);
    void onDetailsUpdated(const QString &text);

    void onPatchingFinished(const QString &newFile, bool failed,
                            const QString &errorMessage);
    void onThreadCompleted();

private:
    void addWidgets();
    void setWidgetActions();
    void populateWidgets();
    void setWidgetDefaults();

    void refreshPartConfigs();
    void refreshPresets();
    void refreshRamdisks();
    void updateBootImageTextBox();

    void resetAutoPatchers();

    void chooseFile();
    void checkSupported();
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
    PatcherTask(QSharedPointer<Patcher> patcher,
                QSharedPointer<FileInfo> info,
                QWidget *parent = 0);
    ~PatcherTask();

    void patch();

signals:
    void finished(const QString &newFile, bool failed,
                  const QString &errorMessage);

private:
    const QScopedPointer <PatcherTaskPrivate> d_ptr;
    Q_DECLARE_PRIVATE(PatcherTask)
};

#endif // MAINWINDOW_H
