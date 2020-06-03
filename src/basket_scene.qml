/*
 * SPDX-FileCopyrightText: (C) 2020 by Ismael Asensio <isma.af@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as QQC2

import org.kde.kirigami 2.12 as Kirigami


ColumnLayout {
    property var basketModel;

    RowLayout {
        Layout.fillWidth: true
        Layout.preferredHeight: implicitHeight

        Kirigami.Heading {
            Layout.fillWidth: true
            level: 3
            text: "BasketQMLViewDraft"
        }

        Kirigami.Heading {
            level: 5
            text: "Notes : " + rootRenderer.count
            Layout.alignment: Qt.AlignRight
        }
    }

    GroupRenderer {
        id: rootRenderer
        model: basketModel
    }
}
