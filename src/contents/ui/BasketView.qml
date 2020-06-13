/*
 * SPDX-FileCopyrightText: (C) 2020 by Ismael Asensio <isma.af@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as QQC2
import QtQml.Models 2.14

import org.kde.kirigami 2.12 as Kirigami


ColumnLayout {
    property var basketModel;

    spacing: 0

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: viewHeader.implicitHeight

        Kirigami.Theme.colorSet: Kirigami.Theme.Window
        color: Kirigami.Theme.backgroundColor

        RowLayout {
            id: viewHeader
            anchors.fill: parent

            Kirigami.Heading {
                Layout.fillWidth: true
                level: 3
                text: "BasketQMLViewDraft"
            }

            Kirigami.Heading {
                level: 5
                text: "Groups : " + groupRepeater.count
                Layout.alignment: Qt.AlignRight
            }
        }
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.fillHeight: true

        Kirigami.Theme.colorSet: Kirigami.Theme.View
        color: Kirigami.Theme.backgroundColor

        Loader {
            id: groupLoader
            anchors.fill: parent
            sourceComponent: (basketModel.layout === 0) ? freeBasketLayout : columnBasketLayout

            Component {
                id: freeBasketLayout
                Item {
                    implicitWidth: childrenRect.width
                    implicitHeight: childrenRect.height
                }
            }
            Component {
                id: columnBasketLayout
                QQC2.SplitView {
                    handle: Rectangle {
                        implicitWidth: Kirigami.Units.largeSpacing
                        implicitHeight: Kirigami.Units.largeSpacing
                        Kirigami.Theme.colorSet: Kirigami.Theme.Window
                        color: (QQC2.SplitHandle.pressed) ? Kirigami.Theme.highlightColor
                             : (QQC2.SplitHandle.hovered) ? Kirigami.Theme.hoverColor
                                                          : Kirigami.Theme.backgroundColor
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.SplitHCursor
                            acceptedButtons: Qt.NoButton
                        }
                    }
                }
            }
        }
    }

    Repeater {
        id: groupRepeater
        model: basketModel
        delegate: groupDelegate

        property rect contentRect: {
            let x = itemMap(Math.min, item => item.x)
            let y = itemMap(Math.min, item => item.y)
            let w = itemMap(Math.max, item => item.x + item.width) - x
            let h = itemMap(Math.max, item => item.y + item.height) - y
            return Qt.rect(x, y, w, h)
        }

        function itemMap(operation, itemFunc, initValue) {
            var acc = null
            for (var idx = 0; idx < count; idx++) {
                var item = itemAt(idx)
                acc = (acc) ? operation(acc, itemFunc(item)) : itemFunc(item)
            }
            return acc;
        }
    }

    Component {
        id: groupDelegate
        ListView {
            parent: groupLoader.item

            model: DelegateModel {
                id: groupModel
                model: basketModel
                rootIndex: groupModel.modelIndex(index)
                delegate: NoteDelegate { Kirigami.Theme.colorSet: Kirigami.Theme.View }
            }
            x: geometry.x
            y: geometry.y
            implicitWidth: geometry.width
            height: contentHeight

            Component.onCompleted: {
                currentIndex = -1
            }
        }
    }
}
