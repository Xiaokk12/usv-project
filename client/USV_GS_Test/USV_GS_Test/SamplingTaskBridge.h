#ifndef SAMPLINGTASKBRIDGE_H
#define SAMPLINGTASKBRIDGE_H

#include <QObject>
#include <QTcpSocket>
#include <QString>
#include <QTimer>

#include "BackendEngine.h"
#include "BackendProtocol.h"

class SamplingTaskBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString systemState READ systemState NOTIFY snapshotChanged)
    Q_PROPERTY(QString missionState READ missionState NOTIFY snapshotChanged)
    Q_PROPERTY(QString statusSummary READ statusSummary NOTIFY snapshotChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY snapshotChanged)
    Q_PROPERTY(int currentWaypoint READ currentWaypoint NOTIFY snapshotChanged)
    Q_PROPERTY(int routePointCount READ routePointCount NOTIFY snapshotChanged)
    Q_PROPERTY(double batterySoc READ batterySoc NOTIFY snapshotChanged)
    Q_PROPERTY(bool gnssOk READ gnssOk NOTIFY snapshotChanged)
    Q_PROPERTY(bool motorOk READ motorOk NOTIFY snapshotChanged)
    Q_PROPERTY(bool rudderOk READ rudderOk NOTIFY snapshotChanged)
    Q_PROPERTY(bool samplerOk READ samplerOk NOTIFY snapshotChanged)
    Q_PROPERTY(bool winchReady READ winchReady NOTIFY snapshotChanged)
    Q_PROPERTY(bool pumpReady READ pumpReady NOTIFY snapshotChanged)
    Q_PROPERTY(bool rinsing READ rinsing NOTIFY snapshotChanged)
    Q_PROPERTY(bool sampling READ sampling NOTIFY snapshotChanged)
    Q_PROPERTY(QString healthSummary READ healthSummary NOTIFY snapshotChanged)
    Q_PROPERTY(QString samplerSummary READ samplerSummary NOTIFY snapshotChanged)
    Q_PROPERTY(int missionBottle READ missionBottle NOTIFY missionParametersChanged)
    Q_PROPERTY(double missionVolume READ missionVolume NOTIFY missionParametersChanged)
    Q_PROPERTY(double missionDepth READ missionDepth NOTIFY missionParametersChanged)
    Q_PROPERTY(int missionSite READ missionSite NOTIFY missionParametersChanged)
public:
    explicit SamplingTaskBridge(QObject *parent = nullptr);

    QString systemState() const { return systemState_; }
    QString missionState() const { return missionState_; }
    QString statusSummary() const { return statusSummary_; }
    QString lastError() const { return lastError_; }
    int currentWaypoint() const { return currentWaypoint_; }
    int routePointCount() const { return routePointCount_; }
    double batterySoc() const { return batterySoc_; }
    bool gnssOk() const { return gnssOk_; }
    bool motorOk() const { return motorOk_; }
    bool rudderOk() const { return rudderOk_; }
    bool samplerOk() const { return samplerOk_; }
    bool winchReady() const { return winchReady_; }
    bool pumpReady() const { return pumpReady_; }
    bool rinsing() const { return rinsing_; }
    bool sampling() const { return sampling_; }
    QString healthSummary() const { return healthSummary_; }
    QString samplerSummary() const { return samplerSummary_; }
    int missionBottle() const { return missionBottle_; }
    double missionVolume() const { return missionVolume_; }
    double missionDepth() const { return missionDepth_; }
    int missionSite() const { return missionSite_; }

    Q_INVOKABLE void requestSystemStart();
    Q_INVOKABLE void requestSystemStop();
    Q_INVOKABLE void requestSetManual();
    Q_INVOKABLE void requestSetAuto();
    Q_INVOKABLE void requestStartMission();
    Q_INVOKABLE void requestPause();
    Q_INVOKABLE void requestResume();
    Q_INVOKABLE void requestAbort();
    Q_INVOKABLE void requestReturnHome();

    // QML -> 后端：手动采样开始/停止
    Q_INVOKABLE void requestManualStart(int bottle, double volume, double depth, int site);
    Q_INVOKABLE void requestManualStop();

    // QML -> 后端：自动任务控制
    Q_INVOKABLE void requestAutoRefresh();
    Q_INVOKABLE void requestAutoStop();

    // 后端 -> QML：更新自动采样任务状态
    Q_INVOKABLE void updateAutoStatus(int bottle, double volume, double depth, int site, const QString &status);
    Q_INVOKABLE void setRoutePointCount(int count);
    Q_INVOKABLE void updateHealthStatus(bool gnssOk, bool motorOk, bool rudderOk, bool samplerOk);
    Q_INVOKABLE void updateSamplerStatus(bool winchReady, bool pumpReady, bool rinsing, bool sampling);
    Q_INVOKABLE void notifyMissionReachedWaypoint();
    Q_INVOKABLE void notifyMissionPrepDone();
    Q_INVOKABLE void notifyMissionRinseDone();
    Q_INVOKABLE void notifyMissionSampleDone();
    Q_INVOKABLE void notifyMissionPostDone();
    Q_INVOKABLE void notifyReturnHomeDone();

signals:
    void manualStartRequested(int bottle, double volume, double depth, int site);
    void manualStopRequested();
    void autoRefreshRequested();
    void autoStopRequested();
    void autoStatusUpdated(int bottle, double volume, double depth, int site, const QString &status);
    void snapshotChanged();
    void missionParametersChanged();

private:
    void publishSnapshot(const usv::backend::Snapshot& snapshot);
    void publishSnapshot();
    void sendCommand(usv::backend::CommandType type);
    void sendTelemetry(const usv::backend::TelemetryEvent& tel);
    void sendRemoteLine(const QString& line);
    void onTick();
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketReadyRead();
    void updateMissionParameters(int bottle, double volume, double depth, int site);

    static QString toQString(usv::backend::SystemState state);
    static QString toQString(usv::backend::MissionState state);
    QString composeStatusSummary(const usv::backend::Snapshot& snapshot) const;

    usv::backend::BackendEngine engine_{};
    QTimer tickTimer_{};
    QTcpSocket socket_{};
    QByteArray socketBuffer_{};
    bool useRemoteBackend_{false};
    QString remoteHost_{};
    quint16 remotePort_{45454};
    QString systemState_{};
    QString missionState_{};
    QString statusSummary_{};
    QString lastError_{};
    int currentWaypoint_{0};
    int routePointCount_{0};
    double batterySoc_{1.0};
    bool gnssOk_{true};
    bool motorOk_{true};
    bool rudderOk_{true};
    bool samplerOk_{true};
    bool winchReady_{false};
    bool pumpReady_{false};
    bool rinsing_{false};
    bool sampling_{false};
    QString healthSummary_{};
    QString samplerSummary_{};
    int missionBottle_{1};
    double missionVolume_{0.5};
    double missionDepth_{0.5};
    int missionSite_{1};
};

#endif // SAMPLINGTASKBRIDGE_H
