// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root
    objectName: "kirigamiWindow"
    title: "Uke Control"
    visible: true
    width: Kirigami.Units.gridUnit * 60
    height: Kirigami.Units.gridUnit * 42
    minimumWidth: Kirigami.Units.gridUnit * 20
    minimumHeight: Kirigami.Units.gridUnit * 24
    property bool initialized: false
    property bool changingPage: false
    property bool browsingCatalog: false
    property var navigationActions: []
    pageStack.columnView.columnResizeMode: Kirigami.ColumnView.SingleColumn
    pageStack.onCurrentItemChanged: {
        if (!initialized || changingPage) return
        if (!pageStack.currentItem || pageStack.currentItem.objectName !== "guidePage") backend.leaveGuide()
        if (pageStack.currentItem && pageStack.currentItem.objectName === "areaPage") browsingCatalog = false
    }
    pageStack.onPageRemoved: page => page.destroy()
    function showArea(index) {
        changingPage = true
        try {
            browsingCatalog = false
            backend.selectArea(index)
            pageStack.clear()
            pageStack.push(areaPage)
            if (globalDrawer.modal) globalDrawer.close()
        } finally { changingPage = false }
    }
    function openCatalog(area = -1) {
        changingPage = true
        try {
            backend.leaveGuide()
            if (pageStack.currentItem && pageStack.currentItem.objectName === "guidePage") pageStack.pop()
            browsingCatalog = true
            if (pageStack.currentItem && pageStack.currentItem.objectName === "catalogPage") pageStack.currentItem.focusSearch()
            else pageStack.push(catalogPage, {initialArea: area})
            if (globalDrawer.modal) globalDrawer.close()
        } finally { changingPage = false }
    }
    function openGuide(id) {
        changingPage = true
        try {
            if (pageStack.currentItem && pageStack.currentItem.objectName === "guidePage") pageStack.pop()
            if (backend.openAction(id)) pageStack.push(guidePage)
        } finally { changingPage = false }
    }
    Component {
        id: navigationAction
        Kirigami.Action {
            required property int areaIndex
            objectName: "navigate_" + backend.areas[areaIndex].id
            text: backend.areas[areaIndex].title
            icon.name: themeIcons.names[backend.areas[areaIndex].icon]
            checkable: true
            checked: !root.browsingCatalog && backend.section === areaIndex
            onTriggered: root.showArea(areaIndex)
        }
    }
    Kirigami.Action { id: catalogSeparator; separator: true }
    Kirigami.Action {
        id: catalogAction
        text: "All actions"
        icon.name: themeIcons.names["edit-find"]
        checkable: true
        checked: root.browsingCatalog
        onTriggered: root.openCatalog()
    }
    globalDrawer: Kirigami.GlobalDrawer {
        objectName: "navigationDrawer"
        title: "Uke Control"
        titleIcon: themeIcons.names["preferences-system"]
        handleClosedIcon.name: themeIcons.names["application-menu"]
        handleOpenIcon.name: themeIcons.names["window-close"]
        handleClosedToolTip: "Open navigation"
        handleOpenToolTip: "Close navigation"
        isMenu: false
        modal: !root.wideScreen
        drawerOpen: !modal
        actions: root.navigationActions
    }
    pageStack.initialPage: areaPage
    Component { id: areaPage; AreaPage { parent: root.pageStack; appWindow: root } }
    Component { id: catalogPage; CatalogPage { parent: root.pageStack; appWindow: root } }
    Component { id: guidePage; GuidePage { parent: root.pageStack; appWindow: root } }
    ReviewSheet { id: reviewSheet }
    Connections {
        target: backend
        function onReviewChanged() {
            if (backend.review.title !== undefined) reviewSheet.open()
            else reviewSheet.close()
        }
    }
    Shortcut { sequence: StandardKey.Quit; onActivated: root.close() }
    Shortcut { sequence: "Ctrl+K"; onActivated: root.openCatalog() }
    Shortcut { sequence: "Alt+Left"; enabled: pageStack.depth > 1; onActivated: pageStack.pop() }
    onClosing: backend.cancelRead()
    Component.onCompleted: {
        let items = []
        for (let area of backend.areas) items.push(navigationAction.createObject(root, {areaIndex: area.index}))
        items.push(catalogSeparator, catalogAction)
        navigationActions = items
        if (backend.guide.id !== undefined) pageStack.push(guidePage)
        initialized = true
    }
}
