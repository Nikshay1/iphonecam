#include "tray.h"
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>

QDBusArgument &operator<<(QDBusArgument &a, const MenuLayout &v) {
    a.beginStructure(); a << v.id << v.properties << v.children; a.endStructure(); return a;
}
const QDBusArgument &operator>>(const QDBusArgument &a, MenuLayout &v) {
    a.beginStructure(); a >> v.id >> v.properties >> v.children; a.endStructure(); return a;
}
QDBusArgument &operator<<(QDBusArgument &a, const MenuProperties &v) {
    a.beginStructure(); a << v.id << v.properties; a.endStructure(); return a;
}
const QDBusArgument &operator>>(const QDBusArgument &a, MenuProperties &v) {
    a.beginStructure(); a >> v.id >> v.properties; a.endStructure(); return a;
}

Tray::Tray(Backend *value, QString image, QObject *parent)
    : QObject(parent), backend(value), icon(std::move(image)),
      m_watcher("org.kde.StatusNotifierWatcher", QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForRegistration) {
    qDBusRegisterMetaType<MenuLayout>(); qDBusRegisterMetaType<MenuProperties>(); qDBusRegisterMetaType<MenuPropertyList>();
    new ItemAdaptor(this); new AppAdaptor(this); new MenuAdaptor(&m_menu, this);
    auto bus = QDBusConnection::sessionBus();
    bus.registerObject("/StatusNotifierItem", this, QDBusConnection::ExportAdaptors);
    bus.registerObject("/App", this, QDBusConnection::ExportAdaptors);
    bus.registerObject("/Menu", &m_menu, QDBusConnection::ExportAdaptors);
    connect(backend, &Backend::changed, this, [this] { ++revision; emit updated(); });
    connect(&m_watcher, &QDBusServiceWatcher::serviceRegistered, this, [this] { registerWithHost(); });
    registerWithHost();
}

void Tray::registerWithHost() {
    QDBusInterface watcher("org.kde.StatusNotifierWatcher", "/StatusNotifierWatcher", "org.kde.StatusNotifierWatcher");
    watcher.asyncCall("RegisterStatusNotifierItem", QDBusConnection::sessionBus().baseService());
}

MenuLayout Tray::layout(int parentId, int depth) const {
    MenuLayout root{0, {{"children-display", "submenu"}}, {}};
    auto add = [&](int id, const QString &label, bool enabled, int checked = -1) {
        QVariantMap properties{{"label", label}, {"enabled", enabled}, {"visible", true}};
        if (checked >= 0) { properties["toggle-type"] = id == 5 ? "checkmark" : "radio"; properties["toggle-state"] = checked; }
        MenuLayout item{id, properties, {}};
        root.children << QVariant::fromValue(item);
    };
    add(1, backend->streaming() ? tr("iPhone Camera · On") : tr("iPhone Camera · Off"), false);
    add(2, backend->enabled() ? tr("Turn camera off") : tr("Turn camera on"), backend->connected());
    for (const auto &entry : backend->cameras()) {
        const auto camera = entry.toMap(); const int id = camera.value("id").toInt();
        add(100 + id, camera.value("label").toString(), backend->lensControls() && !backend->lensBusy(), backend->activeCamera() == id);
    }
    add(3, tr("Controls and setup…"), true);
    add(4, tr("Reconnect USB"), backend->connected());
    add(5, tr("Connect automatically"), true, backend->autoConnect());
    add(6, tr("Quit"), true);
    if (parentId != 0) {
        for (const auto &child : root.children) {
            auto node = qvariant_cast<MenuLayout>(child);
            if (node.id == parentId) return node;
        }
        return {parentId, {}, {}};
    }
    if (depth == 0) root.children.clear();
    return root;
}

void Tray::handleMenuEvent(int id, const QString &event) {
    if (event != "clicked") return;
    if (id == 2 && backend->connected()) backend->toggle();
    else if (id == 3) emit showRequested();
    else if (id == 4 && backend->connected()) backend->refresh();
    else if (id == 5) backend->setAutoConnect(!backend->autoConnect());
    else if (id == 6) backend->quit();
    else if (id >= 100 && id < 116) backend->switchCamera(id - 100);
}

ItemAdaptor::ItemAdaptor(Tray *tray) : QDBusAbstractAdaptor(tray), m_tray(tray) {
    connect(tray, &Tray::updated, this, [this] { emit NewIcon(); emit NewTitle(); emit NewStatus(status()); });
}
QString ItemAdaptor::iconName() const {
    return m_tray->backend->streaming() ? QString(m_tray->icon).replace("iphonecam.svg", "iphonecam-on.svg") : m_tray->icon;
}
void ItemAdaptor::Activate(int, int) { emit m_tray->showRequested(); }
void ItemAdaptor::ContextMenu(int, int) { emit m_tray->showRequested(); }
void ItemAdaptor::SecondaryActivate(int, int) { m_tray->backend->toggle(); }

MenuAdaptor::MenuAdaptor(QObject *object, Tray *tray) : QDBusAbstractAdaptor(object), m_tray(tray) {
    connect(tray, &Tray::updated, this, [this] { emit LayoutUpdated(m_tray->revision, 0); });
}
QString MenuAdaptor::textDirection() const { return QGuiApplication::layoutDirection() == Qt::RightToLeft ? "rtl" : "ltr"; }
void MenuAdaptor::GetLayout(int id, int depth, const QStringList &, uint &revision, MenuLayout &layout) {
    revision = m_tray->revision; layout = m_tray->layout(id, depth);
}
MenuPropertyList MenuAdaptor::GetGroupProperties(const QList<int> &ids, const QStringList &names) {
    MenuPropertyList result;
    for (int id : ids) {
        auto item = m_tray->layout(id, 0); QVariantMap properties;
        if (names.isEmpty()) properties = item.properties;
        else for (const auto &name : names) if (item.properties.contains(name)) properties[name] = item.properties[name];
        result << MenuProperties{id, properties};
    }
    return result;
}
QDBusVariant MenuAdaptor::GetProperty(int id, const QString &name) {
    auto value = m_tray->layout(id, 0).properties.value(name);
    return QDBusVariant(value.isValid() ? value : QVariant(QString()));
}
void MenuAdaptor::Event(int id, const QString &event, const QDBusVariant &, uint) { m_tray->handleMenuEvent(id, event); }

QString AppAdaptor::Status() {
    auto *b = m_tray->backend;
    return QString::fromUtf8(QJsonDocument(QJsonObject{{"status", b->status()}, {"connected", b->connected()},
        {"enabled", b->enabled()}, {"streaming", b->streaming()}, {"lensControls", b->lensControls()},
        {"message", b->message()}}).toJson(QJsonDocument::Compact));
}
void AppAdaptor::Quit() { m_tray->backend->quit(); }
