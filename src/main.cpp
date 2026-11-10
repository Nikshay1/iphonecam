#include <QGuiApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDir>
#include <QFile>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QTextStream>
#include <QTimer>
#include <QTemporaryDir>
#include "backend.h"
#include "protocol.h"
#include "theme.h"
#include "translator.h"
#include "tray.h"
#include "watcher.h"

int main(int argc, char **argv) {
    const QStringList args = [&] { QStringList a; for (int i = 1; i < argc; ++i) a << QString::fromLocal8Bit(argv[i]); return a; }();
    const QString command = args.value(0);
    if (QStringList{"watch", "doctor", "status", "stop", "toggle", "quit", "setup", "--help", "--version"}.contains(command)
        || args.contains("--screenshot")) qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app(argc, argv);
    app.setApplicationName("iphonecam"); app.setOrganizationName("iphonecam"); app.setApplicationVersion("0.2.0");
    app.setDesktopFileName("iphonecam"); app.setQuitOnLastWindowClosed(false);
    auto out = QTextStream(stdout); auto err = QTextStream(stderr);
    if (command == "--help") {
        out << "iphonecam [open|--tray|watch|doctor [--json]|status|toggle|stop|quit|setup|--demo]\n"; return 0;
    }
    if (command == "--version") { out << app.applicationVersion() << '\n'; return 0; }
    const auto directory = QCoreApplication::applicationDirPath();
    if (command == "setup") {
        auto script = directory + "/../bin/setup-system";
        if (!QFile::exists(script)) script = "/usr/share/iphonecam/setup-system";
        return QProcess::execute("bash", {script});
    }
    if (command == "doctor") {
        QJsonObject tools; bool ok = true;
        for (const auto &tool : {"ffmpeg", "iproxy", "idevice_id"}) {
            const auto path = QStandardPaths::findExecutable(tool); tools[tool] = path; ok &= !path.isEmpty();
        }
        QString reason; bool camera = Protocol::isOurCamera("/dev/video42", &reason); ok &= camera;
        QJsonObject result{{"tools", tools}, {"virtualCamera", camera}, {"device", "/dev/video42"},
                           {"hint", reason}, {"ok", ok}};
        out << QJsonDocument(result).toJson(args.contains("--json") ? QJsonDocument::Compact : QJsonDocument::Indented) << '\n';
        return ok ? 0 : 1;
    }
    const QString service = "io.github.Nikshay1.iphonecam";
    if (command == "watch") { StreamWatcher watcher; return app.exec(); }
    if (QStringList{"status", "toggle", "stop", "quit"}.contains(command)) {
        QDBusInterface remote(service, "/App", service);
        if (!remote.isValid()) { err << "iphonecam is not running. Launch iphonecam first.\n"; return 1; }
        if (command == "status") {
            QDBusReply<QString> reply = remote.call("Status");
            if (!reply.isValid()) { err << reply.error().message() << '\n'; return 1; }
            out << reply.value() << '\n';
        } else {
            QString method = command; method[0] = method[0].toUpper();
            auto reply = remote.call(method);
            if (reply.type() == QDBusMessage::ErrorMessage) { err << reply.errorMessage() << '\n'; return 1; }
        }
        return 0;
    }
    const bool demo = args.contains("--demo"), screenshot = args.contains("--screenshot");
    if (screenshot && !demo) { err << "Screenshots require --demo; real camera images are not captured.\n"; return 1; }
    if (!demo && !QDBusConnection::sessionBus().registerService(service)) {
        QDBusInterface remote(service, "/App", service);
        if (!remote.isValid()) { err << "Unable to register the application on the session bus.\n"; return 1; }
        if (!args.contains("--tray")) remote.call("Show");
        return 0;
    }
    QQuickStyle::setStyle("Material");
    QTemporaryDir demoFiles;
    const auto languageFile = demoFiles.filePath("language");
    if (demo) {
        QFile language(languageFile);
        if (!language.open(QIODevice::WriteOnly)) { err << "Cannot create demo language settings.\n"; return 1; }
        language.write(args.contains("--danish") ? "da\n" : "en\n");
    }
    Translator translator(demo ? languageFile : QString()); app.installTranslator(&translator);
    Theme theme(demo ? demoFiles.path() : QString()); Backend backend(demo);
    QQmlApplicationEngine engine;
    QObject::connect(&translator, &Translator::changed, &engine, &QQmlEngine::retranslate);
    engine.rootContext()->setContextProperty("backend", &backend);
    engine.rootContext()->setContextProperty("theme", &theme);
    engine.rootContext()->setContextProperty("translator", &translator);
    engine.rootContext()->setContextProperty("startHidden", args.contains("--tray"));
    engine.load(QUrl("qrc:/Main.qml"));
    if (engine.rootObjects().isEmpty()) return 1;
    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    auto show = [window] { window->show(); window->raise(); window->requestActivate(); };
    QObject::connect(&backend, &Backend::openRequested, &app, show);
    QString icon = "/usr/share/icons/hicolor/scalable/apps/iphonecam.svg";
    if (!QFile::exists(icon)) icon = directory + "/../pkgbuild/iphonecam.svg";
    app.setWindowIcon(QIcon(icon));
    std::unique_ptr<Tray> tray;
    if (!demo) { tray = std::make_unique<Tray>(&backend, icon); QObject::connect(tray.get(), &Tray::showRequested, &app, show); }
    if (screenshot) QTimer::singleShot(1200, &app, [&] {
        const auto path = args.value(args.indexOf("--screenshot") + 1);
        const bool saved = !path.isEmpty() && window->grabWindow().save(path);
        app.exit(saved ? 0 : 1);
    });
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &backend, &Backend::stop);
    return app.exec();
}
