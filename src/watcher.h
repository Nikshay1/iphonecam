#pragma once
#include <QObject>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusServiceWatcher>
#include <QProcess>
#include <QCoreApplication>
#include <QTextStream>
#include <QTimer>

// A persistent JSON event stream lets the native shell panel follow D-Bus
// without spawning repeated commands or depending on private shell APIs.
class StreamWatcher : public QObject {
    Q_OBJECT
public:
    explicit StreamWatcher(QObject *parent = nullptr) : QObject(parent),
        m_watcher("io.github.Nikshay1.iphonecam", QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForOwnerChange) {
        auto bus = QDBusConnection::sessionBus();
        bus.connect("io.github.Nikshay1.iphonecam", "/App", "io.github.Nikshay1.iphonecam", "StateChanged", this, SLOT(consume(QString)));
        connect(&m_watcher, &QDBusServiceWatcher::serviceRegistered, this, [this] { QTimer::singleShot(100, this, &StreamWatcher::refresh); });
        connect(&m_watcher, &QDBusServiceWatcher::serviceUnregistered, this, [this] { offline(); });
        QTimer::singleShot(0, this, [this] {
            QDBusInterface remote("io.github.Nikshay1.iphonecam", "/App", "io.github.Nikshay1.iphonecam");
            if (!remote.isValid()) QProcess::startDetached(QCoreApplication::applicationFilePath(), {"--tray"});
            refresh();
        });
    }
public slots:
    void consume(const QString &json) {
        if (json == m_previous) return;
        m_previous = json;
        QTextStream(stdout) << json << Qt::endl;
    }
private:
    void offline() { consume("{\"connected\":false,\"enabled\":false,\"streaming\":false,\"status\":\"offline\",\"message\":\"Camera service is not running.\"}"); }
    void refresh() {
        QDBusInterface remote("io.github.Nikshay1.iphonecam", "/App", "io.github.Nikshay1.iphonecam");
        QDBusReply<QString> reply = remote.call("Status");
        if (reply.isValid()) consume(reply.value()); else offline();
    }
    QDBusServiceWatcher m_watcher;
    QString m_previous;
};
