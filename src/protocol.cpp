#include "protocol.h"
#include <QFile>
#include <QRegularExpression>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <unistd.h>

bool Protocol::validUdid(const QString &value) {
    static const QRegularExpression pattern("^(?:[a-fA-F0-9]{40}|[a-fA-F0-9]{8}-[a-fA-F0-9]{16})$");
    return pattern.match(value).hasMatch();
}

QStringList Protocol::devices(const QByteArray &data) {
    QStringList result;
    for (const auto &line : QString::fromUtf8(data.left(16384)).split('\n')) {
        const auto value = line.trimmed();
        if (validUdid(value) && !result.contains(value)) result << value;
        if (result.size() == 16) break;
    }
    return result;
}

QVariantList Protocol::cameras(const QByteArray &data) {
    QVariantList result;
    const auto lines = QString::fromUtf8(data.left(8192)).split('\n');
    // DroidCam's camera list is ordered; its zero-based row is the API ID.
    // Preserve IDs even if a row is blank. Never assume front/back ordering.
    for (int id = 0; id < qMin(lines.size(), 16); ++id) {
        QString label = lines[id].trimmed().left(80);
        label.remove(QRegularExpression("[\\x00-\\x1f\\x7f]"));
        if (!label.isEmpty()) result << QVariantMap{{"id", id}, {"label", label}};
    }
    return result;
}

QString Protocol::cameraPath(int id, const QVariantList &available) {
    for (const auto &camera : available)
        if (camera.toMap().value("id").toInt() == id)
            return QString("/v1/camera/active/%1").arg(id);
    return {};
}

QStringList Protocol::ffmpegArguments(quint16 port, const QString &resolution, const QString &device) {
    if (!port || !QStringList{"640x480", "1280x720", "1920x1080"}.contains(resolution)
        || device != "/dev/video42") return {};
    auto dimensions = resolution.split('x');
    return {"-hide_banner", "-nostdin", "-loglevel", "warning", "-nostats", "-progress", "pipe:1",
            "-rw_timeout", "5000000", "-analyzeduration", "1000000", "-probesize", "1000000",
            "-i", QString("http://127.0.0.1:%1/video/%2").arg(port).arg(resolution),
            "-an", "-vf", QString("scale=%1:%2:force_original_aspect_ratio=decrease,pad=%1:%2:(ow-iw)/2:(oh-ih)/2,format=yuv420p").arg(dimensions[0], dimensions[1]),
            "-r", "30", "-c:v", "rawvideo", "-pix_fmt", "yuv420p", "-f", "v4l2", device};
}

bool Protocol::isOurCamera(const QString &device, QString *reason) {
    auto fail = [reason](const QString &message) { if (reason) *reason = message; return false; };
    if (device != "/dev/video42") return fail(QStringLiteral("The dedicated camera must be /dev/video42."));
    int fd = ::open(QFile::encodeName(device).constData(), O_RDWR | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) return fail(QStringLiteral("Run iphonecam setup to install and load the virtual camera driver."));
    v4l2_capability cap{};
    bool ok = ::ioctl(fd, VIDIOC_QUERYCAP, &cap) == 0;
    ::close(fd);
    if (!ok || QByteArray(reinterpret_cast<char *>(cap.driver)) != "v4l2 loopback"
        || QByteArray(reinterpret_cast<char *>(cap.card)) != "iPhone Camera")
        return fail(QStringLiteral("/dev/video42 is not the dedicated iPhone Camera. It will not be used."));
    return true;
}
