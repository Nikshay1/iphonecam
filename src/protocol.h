#pragma once
#include <QVariantList>
#include <QStringList>

namespace Protocol {
bool validUdid(const QString &value);
QStringList devices(const QByteArray &data);
QVariantList cameras(const QByteArray &data);
QString cameraPath(int id, const QVariantList &available);
QStringList ffmpegArguments(quint16 port, const QString &resolution, const QString &device);
bool isOurCamera(const QString &device, QString *reason = nullptr);
}
