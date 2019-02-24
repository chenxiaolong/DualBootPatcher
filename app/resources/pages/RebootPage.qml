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

import QtQuick 2.6
import QtQuick.Layouts 1.0
import QtQuick.Controls 2.1

ScrollablePage {
    Column {
        spacing: 40
        width: parent.width

        ColumnLayout {
            spacing: 20
            anchors.horizontalCenter: parent.horizontalCenter

            Button {
                text: qsTr("Reboot to system")
                Layout.fillWidth: true
            }
            Button {
                text: qsTr("Reboot to recovery")
                Layout.fillWidth: true
            }
            Button {
                text: qsTr("Reboot to bootloader")
                Layout.fillWidth: true
            }
            Button {
                text: qsTr("Shut down")
                Layout.fillWidth: true
            }
        }
    }
}
