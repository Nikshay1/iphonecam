#include <QtTest>
#include <QDBusMetaType>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QFile>
#include <QTcpServer>
#include <QTcpSocket>
#include "protocol.h"
#include "backend.h"
#include "tray.h"
#include "theme.h"
#include "translator.h"

class IphonecamTests : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() { QCoreApplication::setOrganizationName("iphonecam-tests"); QQuickStyle::setStyle("Material"); }
    void usbIdsAreValidated() {
        QCOMPARE(Protocol::devices("00000000-0000000000000000\n-not-an-id\n00000000-0000000000000000\n"), QStringList{"00000000-0000000000000000"});
        QVERIFY(!Protocol::validUdid("--network"));
        QVERIFY(!Protocol::validUdid("00000000-0000000000000000; touch /tmp/no"));
        QVERIFY(Protocol::validUdid(QString(40, 'a')));
    }
    void cameraIdsPreserveReportedOrder() {
        auto lenses = Protocol::cameras("Back\n\nFront\n");
        QCOMPARE(lenses.size(), 2);
        QCOMPARE(lenses[1].toMap().value("id").toInt(), 2);
        QCOMPARE(Protocol::cameraPath(2, lenses), QString("/v1/camera/active/2"));
        QVERIFY(Protocol::cameraPath(1, lenses).isEmpty());
        QVERIFY(Protocol::cameraPath(-1, lenses).isEmpty());
    }
    void framesCanOnlyTargetDedicatedDevice() {
        QVERIFY(Protocol::ffmpegArguments(12345, "1280x720", "/dev/video0").isEmpty());
        QVERIFY(Protocol::ffmpegArguments(12345, "1x1", "/dev/video42").isEmpty());
        auto args = Protocol::ffmpegArguments(12345, "1280x720", "/dev/video42");
        QVERIFY(args.contains("http://127.0.0.1:12345/video/1280x720"));
        QCOMPARE(args.last(), QString("/dev/video42"));
        QVERIFY(!Protocol::isOurCamera("/dev/video0"));
    }
    void demoStateSupportsSwitchAndStop() {
        Backend backend(true);
        QVERIFY(!backend.enabled()); QVERIFY(!backend.streaming());
        backend.toggle(); QVERIFY(backend.streaming());
        backend.switchCamera(1); QCOMPARE(backend.activeCamera(), 1);
        backend.switchCamera(99); QCOMPARE(backend.activeCamera(), 1);
        backend.stop(); QVERIFY(!backend.enabled()); QVERIFY(!backend.streaming());
    }
    void menuUsesValidDbusLayout() {
        Backend backend(true); Tray tray(&backend, "/tmp/iphonecam.svg");
        QCOMPARE(QString::fromLatin1(QDBusMetaType::typeToSignature(QMetaType::fromType<MenuLayout>())), QString("(ia{sv}av)"));
        auto layout = tray.layout(); QVERIFY(layout.children.size() >= 8);
        QCOMPARE(qvariant_cast<MenuLayout>(layout.children.first()).id, 1);
        tray.handleMenuEvent(2, "hovered"); QVERIFY(!backend.enabled());
        tray.handleMenuEvent(2, "clicked"); QVERIFY(backend.streaming());
        tray.handleMenuEvent(101, "clicked"); QCOMPARE(backend.activeCamera(), 1);
        tray.handleMenuEvent(2, "clicked"); QVERIFY(!backend.streaming());
    }
    void qmlLoadsAndActionsWork() {
        Backend backend(true); Theme theme("/nonexistent"); Translator translator("/nonexistent");
        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        engine.rootContext()->setContextProperty("theme", &theme);
        engine.rootContext()->setContextProperty("translator", &translator);
        engine.rootContext()->setContextProperty("startHidden", false);
        engine.load(QUrl("qrc:/Main.qml"));
        QVERIFY(!engine.rootObjects().isEmpty());
        auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().first()); QVERIFY(window);
        auto toggle = window->findChild<QObject *>("cameraToggle"); QVERIFY(toggle);
        QVERIFY(QMetaObject::invokeMethod(toggle, "clicked")); QVERIFY(backend.streaming());
        QTest::keyClick(window, Qt::Key_Space); QTRY_VERIFY(!backend.streaming());
        QTest::keyClick(window, Qt::Key_Escape); QTRY_VERIFY(!window->isVisible());
    }
    void translationReloadsLive() {
        QTemporaryDir temp; QFile file(temp.filePath("language")); QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("da\n"); file.close();
        Translator translator(file.fileName());
        QCOMPARE(translator.translate("Main", "Turn camera on"), QString::fromUtf8("Tænd kamera"));
        QVERIFY(file.open(QIODevice::WriteOnly)); file.write("en\n"); file.close();
        QTRY_COMPARE(translator.language(), QString("en"));
    }
    void themePaletteFollowsFileReplacement() {
        QTemporaryDir temp; QDir(temp.path()).mkdir("theme");
        auto path = temp.filePath("theme/colors.toml");
        QFile file(path); QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("accent = \"#aabbcc\"\nbackground = \"#090809\"\nforeground = \"#ffffff\"\n"); file.close();
        Theme theme(temp.path()); QCOMPARE(theme.background(), QString("#090809")); QVERIFY(theme.dark());
        QVERIFY(QFile::rename(path, path + ".old"));
        QVERIFY(file.open(QIODevice::WriteOnly)); file.write("accent = \"#112233\"\nbackground = \"#eeeeee\"\nforeground = \"#111111\"\n"); file.close();
        QTRY_COMPARE(theme.background(), QString("#eeeeee")); QVERIFY(!theme.dark());
        QCOMPARE(theme.accent(), QString("#112233"));
    }
    void realHttpControlsHonorProAndReportConfirmedLens() {
        QTcpServer phone; QVERIFY(phone.listen(QHostAddress::LocalHost));
        int active = 0; bool pro = false; QStringList requests;
        connect(&phone, &QTcpServer::newConnection, this, [&] {
            auto *socket = phone.nextPendingConnection();
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            connect(socket, &QTcpSocket::readyRead, socket, [&, socket] {
                QByteArray buffer = socket->property("buffer").toByteArray() + socket->readAll();
                socket->setProperty("buffer", buffer);
                if (!buffer.contains("\r\n\r\n") || socket->property("handled").toBool()) return;
                socket->setProperty("handled", true);
                const auto line = QString::fromUtf8(buffer.split('\n').first()).trimmed(); requests << line;
                QByteArray body; int code = 200;
                if (line.startsWith("PUT /v1/camera/active/")) active = line.split(' ').value(1).split('/').last().toInt();
                else if (!pro) code = 550;
                else body = QByteArray("{\"active\":") + QByteArray::number(active) + "}";
                socket->write("HTTP/1.1 " + QByteArray::number(code) + " Response\r\nConnection: close\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
                socket->disconnectFromHost();
            });
        });
        Backend backend(true); backend.m_demo = false; backend.m_port = phone.serverPort(); backend.m_streaming = true;
        backend.refreshControls(); QTRY_VERIFY(!backend.lensControls());
        QVERIFY(backend.message().contains("Pro"));
        backend.switchCamera(1); QCOMPARE(requests.size(), 1);
        pro = true; backend.refreshControls(); QTRY_VERIFY(backend.lensControls());
        backend.switchCamera(1); QTRY_COMPARE(backend.activeCamera(), 1);
        QVERIFY(requests.contains("PUT /v1/camera/active/1 HTTP/1.1"));
        backend.stop(); QVERIFY(!backend.enabled()); QVERIFY(!backend.streaming());
    }
    void staleUsbResponsesCannotChangeNewSession() {
        QTcpServer phone; QVERIFY(phone.listen(QHostAddress::LocalHost));
        Backend backend(true); backend.m_port = phone.serverPort();
        bool called = false;
        backend.request("/v1/phone/name", false, [&](int, const QByteArray &) { called = true; });
        QTRY_VERIFY(phone.hasPendingConnections());
        auto *socket = phone.nextPendingConnection();
        backend.stop();
        socket->write("HTTP/1.1 200 OK\r\nContent-Length: 3\r\nConnection: close\r\n\r\nOld"); socket->disconnectFromHost();
        QTest::qWait(100); QVERIFY(!called); socket->deleteLater();
    }
};
QTEST_MAIN(IphonecamTests)
#include "iphonecam_tests.moc"
