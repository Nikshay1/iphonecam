import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: win
    width: 520; height: 700
    minimumWidth: 460; minimumHeight: 640
    visible: !startHidden
    title: qsTr("iPhone Camera")
    color: "#111315"
    Material.theme: Material.Dark
    Material.accent: theme.accent
    Material.roundedScale: theme.radius
    property bool helpVisible: false
    onClosing: function(close) { close.accepted = false; win.hide() }
    Shortcut { sequence: "Space"; context: Qt.ApplicationShortcut; onActivated: backend.toggle() }
    Shortcut { sequence: "Q"; context: Qt.ApplicationShortcut; onActivated: backend.quit() }
    Shortcut { sequence: "Escape"; context: Qt.ApplicationShortcut; onActivated: win.hide() }
    Shortcut { sequence: "?"; context: Qt.ApplicationShortcut; onActivated: win.helpVisible = !win.helpVisible }

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
                Label { text: qsTr("iPhone Camera"); font.pixelSize: 32; font.weight: Font.DemiBold }
                Label { text: qsTr("A direct USB connection, one tray switch."); color: "#92999f"; font.pixelSize: 14 }
            }
            Rectangle {
                Layout.fillWidth: true; Layout.leftMargin: 28; Layout.rightMargin: 28
                implicitHeight: statusColumn.implicitHeight + 36
                color: "#1b1f22"; border.color: backend.streaming ? theme.accent : "#30363a"; radius: theme.radius
                ColumnLayout {
                    id: statusColumn
                    anchors.fill: parent; anchors.margins: 18
                    spacing: 12
                    RowLayout {
                        Rectangle { width: 9; height: 9; radius: 4.5; color: backend.streaming ? "#85df98" : backend.connected ? theme.accent : "#747b82" }
                        Label {
                            Layout.fillWidth: true
                            text: backend.streaming ? qsTr("CAMERA ON") : backend.connected ? qsTr("USB CONNECTED") : qsTr("WAITING FOR IPHONE")
                            font.pixelSize: 11; font.weight: Font.Bold; font.letterSpacing: 1
                        }
                        Label { text: "USB"; color: "#92999f"; font.pixelSize: 11 }
                    }
                    Label { text: backend.phoneName || qsTr("Connect your iPhone"); textFormat: Text.PlainText; font.pixelSize: 22; Layout.fillWidth: true; elide: Text.ElideRight }
                    Label { text: backend.message; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: "#b6bec4"; font.pixelSize: 13; Layout.fillWidth: true }
                    Button {
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
                Label { text: qsTr("CAMERA LENS"); color: "#92999f"; font.pixelSize: 11; font.letterSpacing: 1.2 }
                RowLayout {
                    Layout.fillWidth: true
                    Repeater {
                        model: backend.cameras
                        Button {
                            required property var modelData
                            Layout.fillWidth: true
                            text: modelData.label
                            contentItem: Label { text: parent.text; textFormat: Text.PlainText; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight }
                            checkable: true; checked: backend.activeCamera === modelData.id
                            enabled: backend.lensControls && !backend.lensBusy
                            onClicked: backend.switchCamera(modelData.id)
                        }
                    }
                    Label { visible: backend.cameras.length === 0; text: qsTr("Lenses appear when DroidCam connects."); color: "#92999f"; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                }
                Label { text: qsTr("Turn video on to check remote lens support. DroidCam Pro may be required."); wrapMode: Text.WordWrap; font.pixelSize: 12; color: "#92999f"; Layout.fillWidth: true }
                ComboBox {
                    visible: backend.devices.length > 1
                    Layout.fillWidth: true; model: backend.devices
                    currentIndex: backend.devices.indexOf(backend.selectedDevice)
                    onActivated: backend.selectDevice(currentText)
                }
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: qsTr("Video size"); Layout.fillWidth: true; color: "#b6bec4" }
                    ComboBox {
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
                    contentItem: Label { text: parent.text; leftPadding: parent.indicator.width + parent.spacing; verticalAlignment: Text.AlignVCenter; wrapMode: Text.WordWrap; font.pixelSize: 12 }
                }
                Label { text: qsTr("In your calling app, select “iPhone Camera” once. Keep DroidCam open on the unlocked phone."); wrapMode: Text.WordWrap; font.pixelSize: 12; color: "#92999f"; Layout.fillWidth: true }
                RowLayout {
                    Layout.fillWidth: true
                    Button { text: qsTr("Reconnect"); enabled: backend.connected; onClicked: backend.refresh() }
                    Item { Layout.fillWidth: true }
                    Button { text: qsTr("Set up driver"); onClicked: backend.setup() }
                }
                Label { visible: win.helpVisible; text: qsTr("Space · Camera on/off\nEscape · Hide to tray\nQ · Quit and stop video\n? · Show shortcuts"); color: theme.accent; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                Label { text: qsTr("Closing this window keeps the tray controls running."); color: "#737d84"; font.pixelSize: 11; Layout.fillWidth: true; wrapMode: Text.WordWrap }
            }
            Item { Layout.preferredHeight: 10 }
        }
    }
}
