#include "SamplingTaskBridge.h"

#include <algorithm>
#include <chrono>

using namespace usv::backend;

SamplingTaskBridge::SamplingTaskBridge(QObject *parent)
    : QObject(parent)
{
    tickTimer_.setInterval(100);
    connect(&tickTimer_, &QTimer::timeout, this, &SamplingTaskBridge::onTick);
    tickTimer_.start();
    publishSnapshot();
}

QString SamplingTaskBridge::toQString(SystemState state)
{
    switch (state) {
    case SystemState::BOOT: return QStringLiteral("BOOT");
    case SystemState::IDLE: return QStringLiteral("IDLE");
    case SystemState::MANUAL: return QStringLiteral("MANUAL");
    case SystemState::AUTO_NAV: return QStringLiteral("AUTO_NAV");
    case SystemState::MISSION: return QStringLiteral("MISSION");
    case SystemState::RETURN_HOME: return QStringLiteral("RETURN_HOME");
    case SystemState::PAUSED: return QStringLiteral("PAUSED");
    case SystemState::FAULT: return QStringLiteral("FAULT");
    case SystemState::SHUTDOWN: return QStringLiteral("SHUTDOWN");
    }
    return QStringLiteral("UNKNOWN");
}

QString SamplingTaskBridge::toQString(MissionState state)
{
    switch (state) {
    case MissionState::NONE: return QStringLiteral("NONE");
    case MissionState::INIT: return QStringLiteral("INIT");
    case MissionState::GOTO_WP: return QStringLiteral("GOTO_WP");
    case MissionState::PREP: return QStringLiteral("PREP");
    case MissionState::RINSE: return QStringLiteral("RINSE");
    case MissionState::SAMPLE: return QStringLiteral("SAMPLE");
    case MissionState::POST: return QStringLiteral("POST");
    case MissionState::NEXT: return QStringLiteral("NEXT");
    case MissionState::DONE: return QStringLiteral("DONE");
    case MissionState::ABORTED: return QStringLiteral("ABORTED");
    case MissionState::FAILED: return QStringLiteral("FAILED");
    }
    return QStringLiteral("UNKNOWN");
}

QString SamplingTaskBridge::composeStatusSummary(const Snapshot& snapshot) const
{
    if (snapshot.system == SystemState::FAULT) {
        if (!snapshot.last_error.empty()) {
            return QStringLiteral("FAULT: %1").arg(QString::fromStdString(snapshot.last_error));
        }
        return QStringLiteral("FAULT");
    }

    if (snapshot.system == SystemState::MISSION) {
        return QStringLiteral("%1 / %2").arg(toQString(snapshot.system), toQString(snapshot.mission));
    }

    if (snapshot.system == SystemState::RETURN_HOME && snapshot.mission == MissionState::ABORTED) {
        return QStringLiteral("RETURN_HOME / ABORTED");
    }

    return toQString(snapshot.system);
}

static QString makeBoolLabel(bool value, const QString& okText, const QString& badText)
{
    return value ? okText : badText;
}

void SamplingTaskBridge::publishSnapshot()
{
    const Snapshot& snapshot = engine_.snapshot();

    const QString nextSystemState = toQString(snapshot.system);
    const QString nextMissionState = toQString(snapshot.mission);
    const QString nextStatusSummary = composeStatusSummary(snapshot);
    const QString nextLastError = QString::fromStdString(snapshot.last_error);
    const int nextWaypoint = snapshot.current_waypoint;
    const double nextBatterySoc = snapshot.battery.soc;
    const bool nextGnssOk = snapshot.health.gnss_ok;
    const bool nextMotorOk = snapshot.health.motor_ok;
    const bool nextRudderOk = snapshot.health.rudder_ok;
    const bool nextSamplerOk = snapshot.health.sampler_ok;
    const bool nextWinchReady = snapshot.sampler.winch_ready;
    const bool nextPumpReady = snapshot.sampler.pump_ready;
    const bool nextRinsing = snapshot.sampler.rinsing;
    const bool nextSampling = snapshot.sampler.sampling;
    const QString nextHealthSummary = QStringLiteral("%1 | %2 | %3 | %4")
        .arg(makeBoolLabel(nextGnssOk, QStringLiteral("GNSS OK"), QStringLiteral("GNSS LOST")))
        .arg(makeBoolLabel(nextMotorOk, QStringLiteral("MOTOR OK"), QStringLiteral("MOTOR FAIL")))
        .arg(makeBoolLabel(nextRudderOk, QStringLiteral("RUDDER OK"), QStringLiteral("RUDDER FAIL")))
        .arg(makeBoolLabel(nextSamplerOk, QStringLiteral("SAMPLER OK"), QStringLiteral("SAMPLER FAIL")));
    const QString nextSamplerSummary = QStringLiteral("%1 | %2 | %3 | %4")
        .arg(makeBoolLabel(nextWinchReady, QStringLiteral("WINCH READY"), QStringLiteral("WINCH WAIT")))
        .arg(makeBoolLabel(nextPumpReady, QStringLiteral("PUMP READY"), QStringLiteral("PUMP OFF")))
        .arg(makeBoolLabel(nextRinsing, QStringLiteral("RINSING"), QStringLiteral("RINSE IDLE")))
        .arg(makeBoolLabel(nextSampling, QStringLiteral("SAMPLING"), QStringLiteral("SAMPLE IDLE")));

    bool snapshotDirty = false;
    if (systemState_ != nextSystemState) {
        systemState_ = nextSystemState;
        snapshotDirty = true;
    }
    if (missionState_ != nextMissionState) {
        missionState_ = nextMissionState;
        snapshotDirty = true;
    }
    if (statusSummary_ != nextStatusSummary) {
        statusSummary_ = nextStatusSummary;
        snapshotDirty = true;
    }
    if (lastError_ != nextLastError) {
        lastError_ = nextLastError;
        snapshotDirty = true;
    }
    if (currentWaypoint_ != nextWaypoint) {
        currentWaypoint_ = nextWaypoint;
        snapshotDirty = true;
    }
    if (batterySoc_ != nextBatterySoc) {
        batterySoc_ = nextBatterySoc;
        snapshotDirty = true;
    }
    if (gnssOk_ != nextGnssOk) {
        gnssOk_ = nextGnssOk;
        snapshotDirty = true;
    }
    if (motorOk_ != nextMotorOk) {
        motorOk_ = nextMotorOk;
        snapshotDirty = true;
    }
    if (rudderOk_ != nextRudderOk) {
        rudderOk_ = nextRudderOk;
        snapshotDirty = true;
    }
    if (samplerOk_ != nextSamplerOk) {
        samplerOk_ = nextSamplerOk;
        snapshotDirty = true;
    }
    if (winchReady_ != nextWinchReady) {
        winchReady_ = nextWinchReady;
        snapshotDirty = true;
    }
    if (pumpReady_ != nextPumpReady) {
        pumpReady_ = nextPumpReady;
        snapshotDirty = true;
    }
    if (rinsing_ != nextRinsing) {
        rinsing_ = nextRinsing;
        snapshotDirty = true;
    }
    if (sampling_ != nextSampling) {
        sampling_ = nextSampling;
        snapshotDirty = true;
    }
    if (healthSummary_ != nextHealthSummary) {
        healthSummary_ = nextHealthSummary;
        snapshotDirty = true;
    }
    if (samplerSummary_ != nextSamplerSummary) {
        samplerSummary_ = nextSamplerSummary;
        snapshotDirty = true;
    }

    if (snapshotDirty) {
        emit snapshotChanged();
    }

    emit autoStatusUpdated(missionBottle_, missionVolume_, missionDepth_, missionSite_, statusSummary_);
}

void SamplingTaskBridge::sendCommand(CommandType type)
{
    engine_.enqueue({std::chrono::steady_clock::now(), CommandEvent{type}});
    engine_.step();
    publishSnapshot();
}

void SamplingTaskBridge::sendTelemetry(const TelemetryEvent& tel)
{
    engine_.enqueue({std::chrono::steady_clock::now(), tel});
    engine_.step();
    publishSnapshot();
}

void SamplingTaskBridge::onTick()
{
    engine_.enqueue({std::chrono::steady_clock::now(), TimerEvent{TimerType::TICK}});
    engine_.step();
    publishSnapshot();
}

void SamplingTaskBridge::updateMissionParameters(int bottle, double volume, double depth, int site)
{
    bool dirty = false;
    if (missionBottle_ != bottle) {
        missionBottle_ = bottle;
        dirty = true;
    }
    if (missionVolume_ != volume) {
        missionVolume_ = volume;
        dirty = true;
    }
    if (missionDepth_ != depth) {
        missionDepth_ = depth;
        dirty = true;
    }
    if (missionSite_ != site) {
        missionSite_ = site;
        dirty = true;
    }

    if (dirty) {
        emit missionParametersChanged();
    }
}

void SamplingTaskBridge::requestSystemStart()
{
    sendCommand(CommandType::CMD_START);
}

void SamplingTaskBridge::requestSystemStop()
{
    sendCommand(CommandType::CMD_STOP);
}

void SamplingTaskBridge::requestSetManual()
{
    sendCommand(CommandType::CMD_SET_MANUAL);
}

void SamplingTaskBridge::requestSetAuto()
{
    sendCommand(CommandType::CMD_SET_AUTO);
}

void SamplingTaskBridge::requestStartMission()
{
    sendCommand(CommandType::CMD_START_MISSION);
}

void SamplingTaskBridge::requestPause()
{
    sendCommand(CommandType::CMD_PAUSE);
}

void SamplingTaskBridge::requestResume()
{
    sendCommand(CommandType::CMD_RESUME);
}

void SamplingTaskBridge::requestAbort()
{
    sendCommand(CommandType::CMD_ABORT);
}

void SamplingTaskBridge::requestReturnHome()
{
    sendCommand(CommandType::CMD_RETURN_HOME);
}

void SamplingTaskBridge::requestManualStart(int bottle, double volume, double depth, int site)
{
    updateMissionParameters(bottle, volume, depth, site);
    emit manualStartRequested(bottle, volume, depth, site);
    sendCommand(CommandType::CMD_START);
    sendCommand(CommandType::CMD_SET_MANUAL);
}

void SamplingTaskBridge::requestManualStop()
{
    emit manualStopRequested();
    publishSnapshot();
}

void SamplingTaskBridge::requestAutoRefresh()
{
    emit autoRefreshRequested();
    publishSnapshot();
}

void SamplingTaskBridge::requestAutoStop()
{
    emit autoStopRequested();
    sendCommand(CommandType::CMD_ABORT);
}

void SamplingTaskBridge::updateAutoStatus(int bottle, double volume, double depth, int site, const QString &status)
{
    updateMissionParameters(bottle, volume, depth, site);
    emit autoStatusUpdated(bottle, volume, depth, site, status);
}

void SamplingTaskBridge::setRoutePointCount(int count)
{
    const int nextCount = std::max(1, count);
    bool changed = routePointCount_ != nextCount;
    routePointCount_ = nextCount;
    engine_.setRouteSize(count);
    publishSnapshot();
    if (changed) {
        emit snapshotChanged();
    }
}

void SamplingTaskBridge::updateHealthStatus(bool gnssOk, bool motorOk, bool rudderOk, bool samplerOk)
{
    TelemetryEvent tel;
    tel.type = TelemetryType::TEL_HEALTH;
    tel.health = {gnssOk, motorOk, rudderOk, samplerOk};
    sendTelemetry(tel);
}

void SamplingTaskBridge::updateSamplerStatus(bool winchReady, bool pumpReady, bool rinsing, bool sampling)
{
    TelemetryEvent tel;
    tel.type = TelemetryType::TEL_SAMPLER_STATUS;
    tel.sampler = {winchReady, pumpReady, rinsing, sampling};
    sendTelemetry(tel);
}

void SamplingTaskBridge::notifyMissionReachedWaypoint()
{
    sendCommand(CommandType::CMD_MISSION_REACHED_WAYPOINT);
}

void SamplingTaskBridge::notifyMissionPrepDone()
{
    sendCommand(CommandType::CMD_MISSION_PREP_DONE);
}

void SamplingTaskBridge::notifyMissionRinseDone()
{
    sendCommand(CommandType::CMD_MISSION_RINSE_DONE);
}

void SamplingTaskBridge::notifyMissionSampleDone()
{
    sendCommand(CommandType::CMD_MISSION_SAMPLE_DONE);
}

void SamplingTaskBridge::notifyMissionPostDone()
{
    sendCommand(CommandType::CMD_MISSION_POST_DONE);
}

void SamplingTaskBridge::notifyReturnHomeDone()
{
    sendCommand(CommandType::CMD_RETURN_HOME_DONE);
}
