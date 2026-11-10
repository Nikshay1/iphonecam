#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QProcess>
#include <QSettings>
#include <QTimer>
#include <QVariantList>
#include <functional>

class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(QString phoneName READ phoneName NOTIFY changed)
    Q_PROPERTY(QStringList devices READ devices NOTIFY changed)
    Q_PROPERTY(QString selectedDevice READ selectedDevice NOTIFY changed)
    Q_PROPERTY(QVariantList cameras READ cameras NOTIFY changed)
    Q_PROPERTY(int activeCamera READ activeCamera NOTIFY changed)
    Q_PROPERTY(bool connected READ connected NOTIFY changed)
    Q_PROPERTY(bool enabled READ enabled NOTIFY changed)
    Q_PROPERTY(bool streaming READ streaming NOTIFY changed)
    Q_PROPERTY(bool lensControls READ lensControls NOTIFY changed)
    Q_PROPERTY(bool lensBusy READ lensBusy NOTIFY changed)
    Q_PROPERTY(bool autoConnect READ autoConnect WRITE setAutoConnect NOTIFY changed)
    Q_PROPERTY(QString resolution READ resolution WRITE setResolution NOTIFY changed)
public:
    explicit Backend(bool demo = false, QObject *parent = nullptr);
    ~Backend() override;
    QString status() const { return m_status; }
    QString message() const { return m_message; }
    QString phoneName() const { return m_phoneName; }
    QStringList devices() const { return m_devices; }
    QString selectedDevice() const { return m_selected; }
    QVariantList cameras() const { return m_cameras; }
    int activeCamera() const { return m_activeCamera; }
    bool connected() const { return !m_selected.isEmpty(); }
    bool enabled() const { return m_wanted; }
    bool streaming() const { return m_streaming; }
    bool lensControls() const { return m_lensControls; }
    bool lensBusy() const { return m_lensBusy; }
    bool autoConnect() const { return m_autoConnect; }
    QString resolution() const { return m_resolution; }
    Q_INVOKABLE void toggle();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void selectDevice(const QString &device);
    Q_INVOKABLE void switchCamera(int id);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void setup();
    Q_INVOKABLE void quit();
    void setAutoConnect(bool value);
    void setResolution(const QString &value);
signals:
    void changed();
    void openRequested();
private:
    friend class IphonecamTests;
    void setState(const QString &status, const QString &message);
    void discover();
    void acceptDevices(const QStringList &devices);
    void openTunnel();
    void probe();
    void startStream();
    void stopStream();
    void stopTunnel();
    void request(const QString &path, bool put, std::function<void(int, QByteArray)> callback);
    void refreshControls();
    QSettings m_settings;
    QProcess m_discovery, m_tunnel, m_ffmpeg;
    QTimer m_poll, m_retry, m_probeTimer, m_discoveryTimeout, m_watchdog;
    QNetworkAccessManager m_network;
    QStringList m_devices;
    QString m_selected, m_status = "disconnected", m_message, m_phoneName;
    QString m_resolution = "1280x720";
    QVariantList m_cameras;
    int m_activeCamera = -1;
    quint16 m_port = 0;
    quint64 m_generation = 0;
    bool m_wanted = false, m_streaming = false, m_ready = false;
    bool m_lensControls = false, m_lensBusy = false, m_autoConnect = false, m_demo = false;
    QByteArray m_progress, m_errors;
};
