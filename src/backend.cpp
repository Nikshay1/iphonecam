#include "backend.h"
#include "protocol.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QTcpServer>

Backend::Backend(bool demo, QObject *parent) : QObject(parent), m_demo(demo) {
    m_network.setProxy(QNetworkProxy::NoProxy);
    m_autoConnect = m_settings.value("autoConnect", false).toBool();
    QString saved = m_settings.value("resolution", "1280x720").toString();
    if (QStringList{"640x480", "1280x720", "1920x1080"}.contains(saved)) m_resolution = saved;
    m_poll.setInterval(3000);
    m_retry.setSingleShot(true); m_retry.setInterval(4000);
    m_probeTimer.setSingleShot(true); m_probeTimer.setInterval(500);
    m_discoveryTimeout.setSingleShot(true); m_discoveryTimeout.setInterval(2000);
    m_watchdog.setSingleShot(true); m_watchdog.setInterval(12000);
    connect(&m_watchdog, &QTimer::timeout, this, [this] {
        stopStream();
        setState("error", tr("No video frames received. Open DroidCam on the unlocked iPhone, then retry."));
        if (m_wanted) m_retry.start();
    });
    connect(&m_poll, &QTimer::timeout, this, &Backend::discover);
    connect(&m_retry, &QTimer::timeout, this, [this] { if (m_ready) startStream(); else openTunnel(); });
    connect(&m_probeTimer, &QTimer::timeout, this, &Backend::probe);
    connect(&m_discoveryTimeout, &QTimer::timeout, &m_discovery, &QProcess::kill);
    connect(&m_discovery, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
        m_discoveryTimeout.stop();
        const auto output = m_discovery.readAllStandardOutput();
        m_discovery.readAllStandardError();
        acceptDevices(code == 0 && status == QProcess::NormalExit ? Protocol::devices(output) : QStringList{});
    });
    connect(&m_discovery, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            m_discoveryTimeout.stop();
            setState("setup", tr("idevice_id is missing. Run iphonecam setup."));
        }
    });
    connect(&m_tunnel, &QProcess::started, this, [this] { m_probeTimer.start(); });
    connect(&m_tunnel, &QProcess::readyReadStandardOutput, this, [this] { m_tunnel.readAllStandardOutput(); });
    connect(&m_tunnel, &QProcess::readyReadStandardError, this, [this] { m_tunnel.readAllStandardError(); });
    connect(&m_tunnel, &QProcess::finished, this, [this] {
        m_ready = false; stopStream();
        if (connected()) {
            setState("waiting", tr("USB tunnel closed. Reconnecting…"));
            m_retry.start();
        }
    });
    connect(&m_tunnel, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) setState("setup", tr("iproxy is missing. Run iphonecam setup."));
    });
    connect(&m_ffmpeg, &QProcess::readyReadStandardError, this, [this] {
        // Keep diagnostics bounded in memory. Camera frames are never written to disk.
        m_errors = (m_errors + m_ffmpeg.readAllStandardError()).right(4096);
    });
    connect(&m_ffmpeg, &QProcess::readyReadStandardOutput, this, [this] {
        m_progress += m_ffmpeg.readAllStandardOutput();
        while (m_progress.contains('\n')) {
            int end = m_progress.indexOf('\n');
            auto line = m_progress.left(end); m_progress.remove(0, end + 1);
            if (line.startsWith("frame=") && line.mid(6).trimmed().toInt() > 0) {
                m_watchdog.start(7000);
                if (!m_streaming) {
                    m_streaming = true;
                    setState("streaming", tr("Choose iPhone Camera in your calling app. Keep DroidCam open."));
                    refreshControls();
                }
            }
        }
        if (m_progress.size() > 8192) m_progress.clear();
    });
    connect(&m_ffmpeg, &QProcess::finished, this, [this] {
        m_watchdog.stop(); m_streaming = false; m_lensControls = false; m_lensBusy = false;
        if (m_wanted && connected()) {
            setState("error", tr("Video stopped. Check DroidCam, camera permission, and the USB cable. Retrying…"));
            m_retry.start();
        } else emit changed();
    });
    connect(&m_ffmpeg, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            m_watchdog.stop();
            setState("setup", tr("FFmpeg could not start. Run iphonecam setup."));
        }
    });
    if (m_demo) {
        m_devices = {"00000000-0000000000000000"}; m_selected = m_devices.first();
        m_phoneName = tr("iPhone"); m_ready = true;
        m_cameras = Protocol::cameras("Back\nFront");
        m_activeCamera = 0; m_lensControls = true;
        setState("ready", tr("Demo mode · no camera or USB connection is opened."));
    } else {
        setState("disconnected", tr("Connect your iPhone by USB, unlock it, and open DroidCam."));
        m_poll.start(); QTimer::singleShot(0, this, &Backend::discover);
    }
}

Backend::~Backend() {
    m_poll.stop(); m_retry.stop(); stopTunnel();
    if (m_discovery.state() != QProcess::NotRunning) { m_discovery.kill(); m_discovery.waitForFinished(1000); }
}

void Backend::setState(const QString &status, const QString &message) {
    m_status = status; m_message = message; emit changed();
}

void Backend::discover() {
    if (m_demo || m_discovery.state() != QProcess::NotRunning) return;
    m_discovery.start("idevice_id", {"-l"});
    m_discoveryTimeout.start();
}

void Backend::acceptDevices(const QStringList &devices) {
    m_devices = devices;
    if (!m_devices.contains(m_selected)) {
        stopTunnel(); m_selected.clear(); m_phoneName.clear(); m_cameras.clear(); m_activeCamera = -1;
        if (!m_devices.isEmpty()) {
            m_selected = m_devices.first();
            m_wanted = m_autoConnect;
            openTunnel();
        } else {
            m_wanted = false;
            setState("disconnected", tr("Connect your iPhone by USB, unlock it, and open DroidCam."));
        }
    }
    emit changed();
}

void Backend::selectDevice(const QString &device) {
    if (!m_devices.contains(device) || m_selected == device) return;
    stopTunnel(); m_selected = device; m_wanted = false; m_cameras.clear(); m_phoneName.clear();
    m_activeCamera = -1; openTunnel();
}

void Backend::openTunnel() {
    if (m_demo || !connected() || m_tunnel.state() != QProcess::NotRunning) return;
    ++m_generation;
    QTcpServer reservation;
    if (!reservation.listen(QHostAddress::LocalHost, 0)) {
        setState("error", tr("Could not reserve a local USB port.")); return;
    }
    m_port = reservation.serverPort(); reservation.close();
    setState("waiting", tr("Connecting to DroidCam over USB…"));
    m_tunnel.start("iproxy", {"--local", "--source", "127.0.0.1", "--udid", m_selected,
                             QString("%1:4747").arg(m_port)});
}

void Backend::request(const QString &path, bool put, std::function<void(int, QByteArray)> callback) {
    QNetworkRequest req(QUrl(QString("http://127.0.0.1:%1%2").arg(m_port).arg(path)));
    req.setTransferTimeout(2500);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    req.setRawHeader("Connection", "close");
    auto *reply = put ? m_network.put(req, QByteArray()) : m_network.get(req);
    reply->setReadBufferSize(16385);
    const auto generation = m_generation;
    connect(reply, &QNetworkReply::readyRead, reply, [reply] {
        if (reply->bytesAvailable() > 16384) reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback, generation] {
        int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError && code != 550 && code != 403) code = 0;
        QByteArray data = reply->read(16384); reply->deleteLater();
        if (generation == m_generation) callback(code, data);
    });
}

void Backend::probe() {
    if (!connected() || m_tunnel.state() != QProcess::Running) return;
    request("/v1/phone/name", false, [this](int code, const QByteArray &data) {
        if (code != 200) {
            setState("waiting", tr("Open DroidCam on your unlocked iPhone and accept Trust This Computer."));
            m_probeTimer.start(3000); return;
        }
        m_ready = true;
        m_phoneName = QString::fromUtf8(data).trimmed().left(80);
        m_phoneName.remove(QRegularExpression("[\\x00-\\x1f\\x7f]"));
        setState("ready", tr("USB connected. Turn on the camera when you are ready."));
        request("/v1/camera/camera_list", false, [this](int code, const QByteArray &data) {
            m_cameras = code == 200 ? Protocol::cameras(data) : QVariantList{}; emit changed();
        });
        if (m_wanted) startStream();
    });
}

void Backend::toggle() {
    if (m_demo) {
        m_wanted = !m_wanted; m_streaming = m_wanted;
        setState(m_wanted ? "streaming" : "ready", tr("Demo mode · no camera or USB connection is opened.")); return;
    }
    if (m_wanted) { stop(); return; }
    m_wanted = true;
    if (!connected()) {
        m_wanted = false; setState("disconnected", tr("Connect an iPhone by USB first.")); return;
    }
    if (m_ready) startStream(); else { openTunnel(); m_probeTimer.start(); }
    emit changed();
}

void Backend::startStream() {
    if (!m_wanted || !m_ready || m_ffmpeg.state() != QProcess::NotRunning) return;
    QString reason;
    if (!Protocol::isOurCamera("/dev/video42", &reason)) {
        m_wanted = false; setState("setup", reason); return;
    }
    m_errors.clear(); m_progress.clear(); m_lensControls = false;
    setState("starting", tr("Starting the iPhone camera…"));
    m_ffmpeg.start("ffmpeg", Protocol::ffmpegArguments(m_port, m_resolution, "/dev/video42"));
    m_watchdog.start(12000);
}

void Backend::stopStream() {
    m_watchdog.stop();
    // Disconnect exit handling while stopping deliberately, so it cannot restart itself.
    if (m_ffmpeg.state() != QProcess::NotRunning) {
        const QSignalBlocker blocker(&m_ffmpeg);
        m_ffmpeg.terminate();
        if (!m_ffmpeg.waitForFinished(1200)) { m_ffmpeg.kill(); m_ffmpeg.waitForFinished(500); }
    }
    m_streaming = false; m_lensControls = false; m_lensBusy = false;
}

void Backend::stopTunnel() {
    ++m_generation; m_retry.stop(); m_probeTimer.stop(); stopStream();
    if (m_tunnel.state() != QProcess::NotRunning) {
        const QSignalBlocker blocker(&m_tunnel);
        m_tunnel.terminate();
        if (!m_tunnel.waitForFinished(500)) { m_tunnel.kill(); m_tunnel.waitForFinished(500); }
    }
    m_port = 0; m_ready = false;
}

void Backend::stop() {
    m_wanted = false; m_retry.stop(); stopStream();
    // Invalidate in-flight controls before allowing a new camera session.
    ++m_generation;
    setState(m_ready ? "ready" : "disconnected", tr("Camera off. Video transmission to the laptop has stopped."));
}

void Backend::refreshControls() {
    request("/v1/camera/info", false, [this](int code, const QByteArray &data) {
        if (!m_streaming) return;
        const auto object = QJsonDocument::fromJson(data).object();
        const auto active = object.value("active");
        m_lensControls = code == 200 && active.isDouble() && active.toDouble() == active.toInt(-1)
            && !Protocol::cameraPath(active.toInt(-1), m_cameras).isEmpty();
        m_activeCamera = m_lensControls ? object.value("active").toInt(-1) : -1;
        if (code == 550 || code == 403)
            m_message = tr("Video is on. Tray lens switching requires DroidCam Pro; you can switch on the phone.");
        else if (!m_lensControls)
            m_message = tr("Video is on. This DroidCam version did not expose remote lens controls.");
        emit changed();
    });
}

void Backend::switchCamera(int id) {
    if (!m_lensControls || m_lensBusy || (!m_streaming && !m_demo)) return;
    const auto path = Protocol::cameraPath(id, m_cameras);
    if (path.isEmpty()) return;
    if (m_demo) { m_activeCamera = id; emit changed(); return; }
    m_lensBusy = true; emit changed();
    request(path, true, [this](int code, const QByteArray &) {
        m_lensBusy = false;
        if (!m_streaming) return;
        if (code == 200 || code == 204) refreshControls();
        else {
            if (code == 550 || code == 403) m_lensControls = false;
            m_message = tr("DroidCam refused the lens change. Check the Pro upgrade and camera availability.");
            emit changed();
        }
    });
}

void Backend::setAutoConnect(bool value) {
    m_autoConnect = value; if (!m_demo) m_settings.setValue("autoConnect", value); emit changed();
}

void Backend::setResolution(const QString &value) {
    if (!QStringList{"640x480", "1280x720", "1920x1080"}.contains(value) || value == m_resolution) return;
    m_resolution = value; if (!m_demo) m_settings.setValue("resolution", value);
    if (!m_demo && m_wanted) { stopStream(); startStream(); }
    emit changed();
}

void Backend::refresh() {
    if (m_demo) return;
    stopTunnel(); openTunnel(); discover();
}

void Backend::setup() {
    const auto path = QCoreApplication::applicationDirPath();
    QString script = path + "/../bin/setup-system";
    if (!QFile::exists(script)) script = "/usr/share/iphonecam/setup-system";
    if (m_demo) return;
    if (!QFile::exists(script) || !QProcess::startDetached("foot", {"-e", "bash", script}))
        setState("setup", tr("Open a terminal and run iphonecam setup."));
}

void Backend::quit() { QCoreApplication::exit(0); }
