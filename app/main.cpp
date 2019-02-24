/*
 * Copyright (C) 2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <QtCore/QDir>
#include <QtCore/QLoggingCategory>
#include <QtCore/QtPlugin>
#include <QtCore/QSettings>
#include <QtCore/QTranslator>
#include <QtGui/QFontDatabase>
#include <QtGui/QGuiApplication>
#include <QtGui/QIcon>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlFileSelector>
#include <QtQuick/QQuickView>
#include <QtQuickControls2/QQuickStyle>

#include "mbutil/properties.h"


Q_IMPORT_PLUGIN(QLinuxFbIntegrationPlugin)
Q_IMPORT_PLUGIN(QmlSettingsPlugin)
Q_IMPORT_PLUGIN(QtQmlModelsPlugin)
Q_IMPORT_PLUGIN(QtQuick2Plugin)
Q_IMPORT_PLUGIN(QtQuick2WindowPlugin)
Q_IMPORT_PLUGIN(QtQuickControls2UniversalStylePlugin)
Q_IMPORT_PLUGIN(QtQuickControls2Plugin)
Q_IMPORT_PLUGIN(QtQuickLayoutsPlugin)
Q_IMPORT_PLUGIN(QtQuickTemplates2Plugin)


static void set_up_qpa()
{
    // Allow user overrides
    auto platform = qgetenv("QT_QPA_PLATFORM");
    if (platform.isEmpty()) {
        platform = QByteArrayLiteral("linuxfb");
        qputenv("QT_QPA_PLATFORM", platform);
    }

    // Android uses a reference DPI of 160 when calculating the scale factor
    qputenv("QT_QPA_LINUXFB_REFERENCE_DPI", "160");

    // Use Android DPI if possible
    auto dpi = mb::util::property_get_string("ro.sf.lcd_density", "");
    if (!dpi.empty()) {
        qputenv("QT_QPA_LINUXFB_PHYSICAL_DPI", dpi.c_str());
    }

    QLoggingCategory::setFilterRules(QStringLiteral("qt.qpa.linuxfb=true"));
}

int main(int argc, char *argv[])
{
    set_up_qpa();

    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("chenxiaolong"));
    app.setOrganizationDomain(QStringLiteral("dbp.noobdev.io"));
    app.setApplicationName(QFileInfo(app.applicationFilePath()).baseName());

    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/NotoSans-Regular.ttf"));

    QIcon::setThemeName(QStringLiteral("dbp"));

    QQuickStyle::setStyle(QStringLiteral("universal"));

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
