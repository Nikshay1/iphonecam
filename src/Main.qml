import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: win
    width: 560; height: 730
    minimumWidth: 460; minimumHeight: 640
    visible: !startHidden
    title: qsTr("iPhone Camera")
    color: theme.background
    font.family: "monospace"
    font.pixelSize: 12
    Material.foreground: theme.foreground
    Material.background: theme.background
    Material.theme: theme.dark ? Material.Dark : Material.Light
    Material.accent: theme.accent
    Material.roundedScale: theme.radius
    property bool helpVisible: false
    onClosing: function(close) { close.accepted = false; win.hide() }
    Shortcut { sequence: "Space"; context: Qt.ApplicationShortcut; onActivated: backend.toggle() }
    Shortcut { sequence: "Q"; context: Qt.ApplicationShortcut; onActivated: backend.quit() }
    Shortcut { sequence: "Escape"; context: Qt.ApplicationShortcut; onActivated: win.hide() }
    Shortcut { sequence: "?"; context: Qt.ApplicationShortcut; onActivated: win.helpVisible = !win.helpVisible }


    component AppButton: Button {
        id: control
        implicitHeight: 38
        leftPadding: 14; rightPadding: 14
        opacity: enabled ? 1 : .45
        background: Rectangle {
            implicitHeight: 38
            radius: theme.radius
            color: control.highlighted ? theme.accent : control.down ? theme.border : theme.surface
            border.width: 1
            border.color: control.activeFocus || control.checked || control.hovered ? theme.accent : theme.border
        }
        contentItem: Text {
            text: control.text; textFormat: Text.PlainText
            font: control.font
            color: control.highlighted ? theme.accentForeground : control.checked ? theme.accent : theme.foreground
            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }
    component AppComboBox: ComboBox {
        id: control
        implicitHeight: 38
        leftPadding: 12; rightPadding: 32
        background: Rectangle {
            radius: theme.radius; color: theme.surface
            border.color: control.activeFocus || control.hovered ? theme.accent : theme.border
        }
        contentItem: Text {
            text: control.displayText; textFormat: Text.PlainText; font: control.font
            color: theme.foreground; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight
        }
        indicator: Text {
            x: control.width - width - 12; anchors.verticalCenter: parent.verticalCenter
            text: "⌄"; color: theme.foreground; font: control.font
        }
        delegate: ItemDelegate {
            width: control.width
            contentItem: Text { text: modelData; textFormat: Text.PlainText; font: control.font; color: theme.foreground }
            background: Rectangle { color: parent.highlighted ? theme.border : theme.surface; radius: theme.radius }
            highlighted: control.highlightedIndex === index
        }
        popup: Popup {
            y: control.height + 3
            width: control.width
            padding: 1
            implicitHeight: contentItem.implicitHeight + 2
            contentItem: ListView {
                clip: true; implicitHeight: contentHeight
                model: control.popup.visible ? control.delegateModel : null
                currentIndex: control.highlightedIndex
            }
            background: Rectangle { color: theme.surface; border.color: theme.border; radius: theme.radius }
        }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        ColumnLayout {
            width: parent.width
            spacing: 18
            LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
            LayoutMirroring.childrenInherit: true
            Item { Layout.preferredHeight: 10 }
            ColumnLayout {
                Layout.fillWidth: true; Layout.leftMargin: 28; Layout.rightMargin: 28
                spacing: 5
                Label { text: qsTr("YOUR PHONE. YOUR WEBCAM."); color: theme.accent; font.pixelSize: 11; font.letterSpacing: 1.6 }
                Label { text: qsTr("iPhone Camera"); font.pixelSize: 26; font.weight: Font.DemiBold }
                Label { text: qsTr("A direct USB connection, one tray switch."); color: theme.muted; font.pixelSize: 12 }
            }
            Rectangle {
                Layout.fillWidth: true; Layout.leftMargin: 28; Layout.rightMargin: 28
                implicitHeight: statusColumn.implicitHeight + 36
                color: theme.surface; border.color: backend.streaming ? theme.accent : theme.border; radius: theme.radius
                ColumnLayout {
                    id: statusColumn
                    anchors.fill: parent; anchors.margins: 18
                    spacing: 12
                    RowLayout {
                        Rectangle { width: 9; height: 9; radius: 4.5; color: backend.streaming ? theme.success : backend.connected ? theme.accent : theme.muted }
                        Label {
                            Layout.fillWidth: true
                            text: backend.streaming ? qsTr("CAMERA ON") : backend.connected ? qsTr("USB CONNECTED") : qsTr("WAITING FOR IPHONE")
                            font.pixelSize: 11; font.weight: Font.Bold; font.letterSpacing: 1
                        }
                        Label { text: "USB"; color: theme.muted; font.pixelSize: 11 }
                    }
                    Label { text: backend.phoneName || qsTr("Connect your iPhone"); textFormat: Text.PlainText; font.pixelSize: 20; Layout.fillWidth: true; elide: Text.ElideRight }
                    Label { text: backend.message; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: theme.foreground; font.pixelSize: 12; Layout.fillWidth: true }
                    AppButton {
                        objectName: "cameraToggle"
                        text: backend.enabled ? qsTr("Turn camera off") : qsTr("Turn camera on")
                        enabled: backend.connected
                        highlighted: !backend.enabled
                        Layout.fillWidth: true
                        onClicked: backend.toggle()
                    }
                }
            }
            ColumnLayout {
                Layout.fillWidth: true; Layout.leftMargin: 28; Layout.rightMargin: 28
                spacing: 9
                Label { text: qsTr("CAMERA LENS"); color: theme.muted; font.pixelSize: 11; font.letterSpacing: 1.2 }
                RowLayout {
                    Layout.fillWidth: true
                    Repeater {
                        model: backend.cameras
                        AppButton {
                            required property var modelData
                            Layout.fillWidth: true
                            text: modelData.label
                            checkable: true; checked: backend.activeCamera === modelData.id
                            enabled: backend.lensControls && !backend.lensBusy
                            onClicked: backend.switchCamera(modelData.id)
                        }
                    }
                    Label { visible: backend.cameras.length === 0; text: qsTr("Lenses appear when DroidCam connects."); color: theme.muted; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                }
                Label { text: qsTr("Turn video on to check remote lens support. DroidCam Pro may be required."); wrapMode: Text.WordWrap; font.pixelSize: 12; color: theme.muted; Layout.fillWidth: true }
                AppComboBox {
                    visible: backend.devices.length > 1
                    Layout.fillWidth: true; model: backend.devices
                    currentIndex: backend.devices.indexOf(backend.selectedDevice)
                    onActivated: backend.selectDevice(currentText)
                }
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: qsTr("Video size"); Layout.fillWidth: true; color: theme.foreground }
                    AppComboBox {
                        objectName: "resolutionSelector"
                        Layout.preferredWidth: 170
                        model: ["640x480", "1280x720", "1920x1080"]
                        currentIndex: model.indexOf(backend.resolution)
                        onActivated: backend.resolution = currentText
                    }
                }
                CheckBox {
                    objectName: "autoConnectToggle"
                    text: qsTr("Turn camera on automatically after USB connection")
                    checked: backend.autoConnect
                    onToggled: backend.autoConnect = checked
                    Layout.fillWidth: true
                    indicator: Rectangle {
                        width: 16; height: 16
                        x: parent.leftPadding; anchors.verticalCenter: parent.verticalCenter
                        radius: Math.min(2, theme.radius)
                        color: parent.checked ? theme.accent : theme.background
                        border.color: parent.activeFocus || parent.checked ? theme.accent : theme.muted
                        Text { anchors.centerIn: parent; text: "✓"; visible: parent.parent.checked; color: theme.accentForeground; font.pixelSize: 12 }
                    }
                    contentItem: Label { text: parent.text; color: theme.foreground; leftPadding: parent.indicator.width + parent.spacing; verticalAlignment: Text.AlignVCenter; wrapMode: Text.WordWrap; font.pixelSize: 12 }
                }
                Label { text: qsTr("In your calling app, select “iPhone Camera” once. Keep DroidCam open on the unlocked phone."); wrapMode: Text.WordWrap; font.pixelSize: 12; color: theme.muted; Layout.fillWidth: true }
                RowLayout {
                    Layout.fillWidth: true
                    AppButton { text: qsTr("Reconnect"); enabled: backend.connected; onClicked: backend.refresh() }
                    Item { Layout.fillWidth: true }
                    AppButton { text: qsTr("Set up driver"); onClicked: backend.setup() }
                }
                Label { visible: win.helpVisible; text: qsTr("Space · Camera on/off\nEscape · Hide to tray\nQ · Quit and stop video\n? · Show shortcuts"); color: theme.accent; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                Label { text: qsTr("Closing this window keeps the tray controls running."); color: theme.muted; font.pixelSize: 11; Layout.fillWidth: true; wrapMode: Text.WordWrap }
            }
            Item { Layout.preferredHeight: 10 }
        }
    }
}
