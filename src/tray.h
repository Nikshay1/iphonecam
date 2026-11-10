#pragma once
#include <QDBusAbstractAdaptor>
#include <QDBusArgument>
#include <QDBusServiceWatcher>
#include <QDBusVariant>
#include <QObject>
#include "backend.h"

struct MenuLayout { int id; QVariantMap properties; QVariantList children; };
Q_DECLARE_METATYPE(MenuLayout)
QDBusArgument &operator<<(QDBusArgument &, const MenuLayout &);
const QDBusArgument &operator>>(const QDBusArgument &, MenuLayout &);
struct MenuProperties { int id; QVariantMap properties; };
Q_DECLARE_METATYPE(MenuProperties)
using MenuPropertyList = QList<MenuProperties>;
Q_DECLARE_METATYPE(MenuPropertyList)
QDBusArgument &operator<<(QDBusArgument &, const MenuProperties &);
const QDBusArgument &operator>>(const QDBusArgument &, MenuProperties &);

class Tray : public QObject {
    Q_OBJECT
public:
    explicit Tray(Backend *backend, QString icon, QObject *parent = nullptr);
    Backend *backend;
    QString icon;
    uint revision = 1;
    MenuLayout layout(int parentId = 0, int depth = -1) const;
    void handleMenuEvent(int id, const QString &event);
    void registerWithHost();
signals:
    void updated();
    void showRequested();
private:
    QObject m_menu;
    QDBusServiceWatcher m_watcher;
};

class ItemAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.StatusNotifierItem")
    Q_PROPERTY(QString Category READ category CONSTANT)
    Q_PROPERTY(QString Id READ id CONSTANT)
    Q_PROPERTY(QString Title READ title CONSTANT)
    Q_PROPERTY(QString Status READ status)
    Q_PROPERTY(QString IconName READ iconName)
    Q_PROPERTY(bool ItemIsMenu READ itemIsMenu CONSTANT)
    Q_PROPERTY(QDBusObjectPath Menu READ menu CONSTANT)
public:
    explicit ItemAdaptor(Tray *tray);
    QString category() const { return "Hardware"; }
    QString id() const { return "iphonecam"; }
    QString title() const { return tr("iPhone Camera"); }
    QString status() const { return "Active"; }
    QString iconName() const;
    bool itemIsMenu() const { return true; }
    QDBusObjectPath menu() const { return QDBusObjectPath("/Menu"); }
public slots:
    void Activate(int, int);
    void SecondaryActivate(int, int);
    void ContextMenu(int, int);
    void Scroll(int, const QString &) {}
signals:
    void NewIcon();
    void NewTitle();
    void NewStatus(const QString &status);
private:
    Tray *m_tray;
};

class MenuAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.canonical.dbusmenu")
    Q_PROPERTY(uint Version READ version CONSTANT)
    Q_PROPERTY(QString TextDirection READ textDirection)
    Q_PROPERTY(QString Status READ status CONSTANT)
    Q_PROPERTY(QStringList IconThemePath READ iconThemePath CONSTANT)
public:
    MenuAdaptor(QObject *object, Tray *tray);
    uint version() const { return 3; }
    QString textDirection() const;
    QString status() const { return "normal"; }
    QStringList iconThemePath() const { return {}; }
public slots:
    void GetLayout(int parentId, int recursionDepth, const QStringList &propertyNames, uint &revision, MenuLayout &layout);
    MenuPropertyList GetGroupProperties(const QList<int> &ids, const QStringList &propertyNames);
    QDBusVariant GetProperty(int id, const QString &name);
    bool AboutToShow(int) { return false; }
    void Event(int id, const QString &eventId, const QDBusVariant &, uint);
signals:
    void LayoutUpdated(uint revision, int parent);
private:
    Tray *m_tray;
};

class AppAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "io.github.Nikshay1.iphonecam")
public:
    explicit AppAdaptor(Tray *tray);
public slots:
    void Show() { emit m_tray->showRequested(); }
    void Toggle() { m_tray->backend->toggle(); }
    void Stop() { m_tray->backend->stop(); }
    void SwitchCamera(int id) { m_tray->backend->switchCamera(id); }
    void SetResolution(const QString &value) { m_tray->backend->setResolution(value); }
    void SetAutoConnect(bool enabled) { m_tray->backend->setAutoConnect(enabled); }
    void Reconnect() { m_tray->backend->refresh(); }
    QString Status();
    void Quit();
signals:
    void StateChanged(const QString &json);
private:
    Tray *m_tray;
};
