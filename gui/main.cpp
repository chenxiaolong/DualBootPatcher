/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mainwindow.h"

#include <QtCore/QStringBuilder>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>

#include <iostream>

#if !defined(DATA_DIR)
#  error DATA_DIR must be defined
#endif


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setApplicationName(QObject::tr("Dual Boot Patcher"));

    mbp::PatcherConfig pc;
    pc.setDataDirectory(a.applicationDirPath().toStdString() + "/" + DATA_DIR);

    MainWindow w(&pc);
    w.show();

    return a.exec();
}
