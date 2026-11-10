import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Quickshell
import Quickshell.Io
import qs.Ui
import qs.Commons
import qs.Commons as Commons
import "Model.js" as Model

Panel {
    id: root
    moduleName: "nikshay.iphonecam"
    ipcTarget: "iphonecam"
    manageIpc: false
    readonly property color foreground: bar ? bar.foreground : Commons.Color.foreground
    readonly property color dim: Qt.darker(foreground, 1.55)
    readonly property string fontFamily: bar ? bar.fontFamily : Style.font.family
    property var state: Model.snapshot("{}")
    property string page: ""
    property int cursor: -1
    property string panelError: ""
    property bool demoMode: false
    readonly property var options: page === "lens" ? state.cameras : page === "resolution"
        ? [{ id: "640x480", label: "640 × 480" }, { id: "1280x720", label: "1280 × 720" }, { id: "1920x1080", label: "1920 × 1080" }] : []
    readonly property int rowCount: page === "" ? 6 : options.length + 1
    implicitWidth: button.implicitWidth
    implicitHeight: button.implicitHeight

    Commons.Translations { id: strings; directory: Qt.resolvedUrl("i18n") }
    Process {
        id: observer
        command: ["iphonecam", "watch"]
        running: !root.demoMode
        stdout: SplitParser {
            onRead: function(data) {
                var next = Model.snapshot(data)
                if (next) { root.state = next; root.panelError = "" }
            }
        }
        onExited: if (!root.demoMode) restartObserver.restart()
    }
    Timer { id: restartObserver; interval: 3000; onTriggered: if (!root.demoMode) observer.running = true }
    Process {
        id: action
        stdout: StdioCollector {}
        stderr: StdioCollector {}
        onExited: function(code) { if (code !== 0) root.panelError = strings.t("actionFailed") }
    }
    function run(method, value) {
        if (demoMode) return
        if (action.running) return
        var args = ["gdbus", "call", "--session", "--dest", "io.github.Nikshay1.iphonecam", "--object-path", "/App", "--method", "io.github.Nikshay1.iphonecam." + method]
        if (value !== undefined) args.push(String(value))
        action.command = args
        action.running = true
    }
    function choose(index) {
        if (index === 0) { page = ""; cursor = -1; return }
        var option = options[index - 1]
        if (!option) return
        if (page === "lens") {
            if (!state.lensControls || state.lensBusy) return
            run("SwitchCamera", option.id)
        } else if (page === "resolution") run("SetResolution", option.id)
        page = ""; cursor = -1
    }
    function activate(index) {
        if (page !== "") { choose(index); return }
        if (index === 0 && state.connected) run("Toggle")
        else if (index === 1) { page = "lens"; cursor = -1 }
        else if (index === 2) { page = "resolution"; cursor = -1 }
        else if (index === 3) run("SetAutoConnect", !state.autoConnect)
        else if (index === 4 && state.connected) run("Reconnect")
        else if (index === 5) { run("Show"); close() }
        else if (index === 6) { run("Stop"); close() }
    }
    onOpenedChanged: if (opened) { page = ""; cursor = -1 }
    IpcHandler {
        target: root.ipcTarget
        function open(): void { root.open() }
        function close(): void { root.close() }
        function toggle(): void { root.toggle() }
        function geometry(): string {
            return JSON.stringify({ x: popup.cardOrigin.x + (popup.screen ? popup.screen.x : 0),
                y: popup.cardOrigin.y + (popup.screen ? popup.screen.y : 0), width: popup.contentWidth, height: popup.drawnHeight })
        }
        function preview(enabled: bool): void {
            root.demoMode = enabled
            if (enabled) root.state = Model.snapshot(JSON.stringify({ connected: true, enabled: false, streaming: false,
                lensControls: true, activeCamera: 0, cameras: [{ id: 0, label: "Back" }, { id: 1, label: "Front" }],
                phoneName: "iPhone", resolution: "1280x720", autoConnect: false, status: "ready",
                message: "Choose iPhone Camera in your calling app. Keep DroidCam open on your phone." }))
            root.open()
        }
    }

    BarIconButton {
        id: button
        anchors.fill: parent
        bar: root.bar
        text: "󰄀"
        foreground: root.state.streaming ? Commons.Color.accent : root.state.connected ? root.barForeground : Qt.darker(root.barForeground, 1.55)
        onPressed: function(mouseButton) {
            if (mouseButton === Qt.MiddleButton && root.state.connected) root.run("Toggle")
            else root.toggle()
        }
    }
    KeyboardPanel {
        id: popup
        anchorItem: button
        owner: root
        bar: root.bar
        open: root.opened
        focusTarget: keys
        contentWidth: popup.fittedContentWidth(Style.space(360))
        contentHeight: popup.fittedContentHeight(column.implicitHeight, Style.space(570))
        PanelKeyCatcher {
            id: keys
            anchors.fill: parent
            onMoveRequested: function(dx, dy) {
                if (dx < 0 && root.page !== "") { root.page = ""; root.cursor = -1; return }
                if (dx > 0 && root.cursor >= 0) { root.activate(root.cursor); return }
                if (dy !== 0) root.cursor = Math.max(0, Math.min(root.rowCount - 1, root.cursor + dy))
            }
            onActivateRequested: root.activate(root.cursor < 0 ? 0 : root.cursor)
            onCloseRequested: root.close()
            onTabRequested: function(direction) { root.switchPanel(direction) }
            Flickable {
                id: scroll
                anchors.fill: parent
                contentWidth: width; contentHeight: column.implicitHeight
                clip: true; boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                Column {
                    id: column
                    x: Style.space(2)
                    width: scroll.width - Style.space(4)
                    spacing: Style.space(12)
                    PanelHero {
                        width: parent.width
                        title: strings.t("title")
                        meta: root.state.streaming ? strings.t("on") : root.state.status === "starting" ? strings.t("starting") : root.state.connected ? strings.t("connected") : strings.t("disconnected")
                        foreground: root.foreground; fontFamily: root.fontFamily
                        iconComponent: Component { PlainText { text: "󰄀"; font.pixelSize: Style.font.display } }
                        trailingControl: Component {
                            ToggleSwitch {
                                checked: root.state.enabled
                                interactive: root.state.connected
                                foreground: root.foreground
                                hasCursor: root.page === "" && root.cursor === 0
                                onToggled: root.run("Toggle")
                                onHovered: function(hovered) { if (hovered) root.cursor = 0 }
                            }
                        }
                    }
                    PanelSeparator { foreground: root.foreground }
                    Column {
                        width: parent.width
                        visible: root.page === ""
                        spacing: Style.space(6)
                        SettingRow { row: 1; label: strings.t("lens"); value: Model.lensLabel(root.state) || strings.t("choosePhone"); chevron: true }
                        SettingRow { row: 2; label: strings.t("resolution"); value: root.state.resolution.replace("x", " × "); chevron: true }
                        SettingRow { row: 3; label: strings.t("autoConnect"); value: root.state.autoConnect ? strings.t("enabled") : strings.t("disabled"); chevron: true }
                    }
                    Column {
                        width: parent.width
                        visible: root.page !== ""
                        spacing: Style.space(6)
                        SettingRow { row: 0; label: "‹ " + strings.t("back"); value: "" }
                        Repeater {
                            model: root.options
                            SettingRow {
                                required property var modelData
                                required property int index
                                row: index + 1
                                label: modelData.label
                                value: (root.page === "lens" ? root.state.activeCamera === modelData.id : root.state.resolution === modelData.id) ? "✓" : ""
                                available: root.page !== "lens" || (root.state.lensControls && !root.state.lensBusy)
                            }
                        }
                    }
                    PanelSeparator { foreground: root.foreground }
                    Column {
                        width: parent.width; spacing: Style.space(6)
                        PlainText { text: root.state.phoneName || strings.t("noPhone"); width: parent.width; elide: Text.ElideRight }
                        PlainText {
                            width: parent.width
                            text: root.panelError || root.state.message || strings.t("connectHint")
                            color: root.dim; font.pixelSize: Style.font.bodySmall
                            wrapMode: Text.WordWrap
                        }
                    }
                    PanelSeparator { foreground: root.foreground }
                    Column {
                        width: parent.width; spacing: Style.space(6)
                        visible: root.page === ""
                        SettingRow { row: 4; label: "󰑓  " + strings.t("reconnect"); value: ""; available: root.state.connected }
                        SettingRow { row: 5; label: "󰒓  " + strings.t("settings"); value: ""; chevron: true }
                    }
                }
            }
        }
    }
    component PlainText: Text {
        textFormat: Text.PlainText
        color: root.foreground; font.family: root.fontFamily; font.pixelSize: Style.font.body
    }
    component SettingRow: CursorSurface {
        id: rowItem
        required property int row
        property string label: ""
        property string value: ""
        property bool chevron: false
        property bool available: true
        width: parent.width
        implicitHeight: Style.space(34)
        hasCursor: root.cursor === row
        foreground: root.foreground
        opacity: available ? 1 : .45
        RowLayout {
            anchors.fill: parent; anchors.leftMargin: Style.space(10); anchors.rightMargin: Style.space(10)
            spacing: Style.space(12)
            PlainText { text: rowItem.label; Layout.fillWidth: true; elide: Text.ElideRight }
            PlainText { text: rowItem.value; color: root.dim; Layout.maximumWidth: rowItem.width * .58; elide: Text.ElideRight }
            PlainText { visible: rowItem.chevron; text: Commons.I18n.rightToLeft ? "‹" : "›" }
        }
        MouseArea {
            anchors.fill: parent; hoverEnabled: true
            cursorShape: rowItem.available ? Qt.PointingHandCursor : Qt.ArrowCursor
            onEntered: root.cursor = rowItem.row
            onClicked: if (rowItem.available) root.activate(rowItem.row)
        }
    }
}
