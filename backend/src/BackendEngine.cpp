#include "BackendEngine.h"

#include <algorithm>
#include <iostream>
#include <type_traits>

namespace usv::backend {

namespace {

const char* toString(SystemState s) {
    switch (s) {
    case SystemState::BOOT: return "BOOT";
    case SystemState::IDLE: return "IDLE";
    case SystemState::MANUAL: return "MANUAL";
    case SystemState::AUTO_NAV: return "AUTO_NAV";
    case SystemState::MISSION: return "MISSION";
    case SystemState::RETURN_HOME: return "RETURN_HOME";
    case SystemState::PAUSED: return "PAUSED";
    case SystemState::FAULT: return "FAULT";
    case SystemState::SHUTDOWN: return "SHUTDOWN";
    }
    return "UNKNOWN";
}

const char* toString(MissionState s) {
    switch (s) {
    case MissionState::NONE: return "NONE";
    case MissionState::INIT: return "INIT";
    case MissionState::GOTO_WP: return "GOTO_WP";
    case MissionState::PREP: return "PREP";
    case MissionState::RINSE: return "RINSE";
    case MissionState::SAMPLE: return "SAMPLE";
    case MissionState::POST: return "POST";
    case MissionState::NEXT: return "NEXT";
    case MissionState::DONE: return "DONE";
    case MissionState::ABORTED: return "ABORTED";
    case MissionState::FAILED: return "FAILED";
    }
    return "UNKNOWN";
}

const char* toString(CommandType cmd) {
    switch (cmd) {
    case CommandType::CMD_START: return "CMD_START";
    case CommandType::CMD_STOP: return "CMD_STOP";
    case CommandType::CMD_SET_MANUAL: return "CMD_SET_MANUAL";
    case CommandType::CMD_SET_AUTO: return "CMD_SET_AUTO";
    case CommandType::CMD_UPLOAD_ROUTE: return "CMD_UPLOAD_ROUTE";
    case CommandType::CMD_START_MISSION: return "CMD_START_MISSION";
    case CommandType::CMD_PAUSE: return "CMD_PAUSE";
    case CommandType::CMD_RESUME: return "CMD_RESUME";
    case CommandType::CMD_ABORT: return "CMD_ABORT";
    case CommandType::CMD_RETURN_HOME: return "CMD_RETURN_HOME";
    case CommandType::CMD_MISSION_REACHED_WAYPOINT: return "CMD_MISSION_REACHED_WAYPOINT";
    case CommandType::CMD_MISSION_PREP_DONE: return "CMD_MISSION_PREP_DONE";
    case CommandType::CMD_MISSION_RINSE_DONE: return "CMD_MISSION_RINSE_DONE";
    case CommandType::CMD_MISSION_SAMPLE_DONE: return "CMD_MISSION_SAMPLE_DONE";
    case CommandType::CMD_MISSION_POST_DONE: return "CMD_MISSION_POST_DONE";
    case CommandType::CMD_RETURN_HOME_DONE: return "CMD_RETURN_HOME_DONE";
    }
    return "UNKNOWN";
}

bool snapshotChanged(const Snapshot& before, const Snapshot& after) {
    return before.system != after.system
        || before.mission != after.mission
        || before.current_waypoint != after.current_waypoint
        || before.last_error != after.last_error;
}

void logSnapshotChange(const Snapshot& before, const Snapshot& after) {
    std::cout << "[STATE]";
    bool appended = false;
    if (before.system != after.system) {
        std::cout << " System: " << toString(before.system) << " -> " << toString(after.system);
        appended = true;
    }
    if (before.mission != after.mission) {
        std::cout << (appended ? " |" : "") << " Mission: " << toString(before.mission)
                  << " -> " << toString(after.mission);
        appended = true;
    }
    if (before.current_waypoint != after.current_waypoint) {
        std::cout << (appended ? " |" : "") << " Waypoint: " << before.current_waypoint
                  << " -> " << after.current_waypoint;
        appended = true;
    }
    if (before.last_error != after.last_error) {
        std::cout << (appended ? " |" : "") << " Error: "
                  << (after.last_error.empty() ? "none" : after.last_error);
    }
    std::cout << "\n";
}

} // namespace

BackendEngine::BackendEngine() {
    snapshot_.system = SystemState::BOOT;
    snapshot_.mission = MissionState::NONE;
    route_size_ = 3;
}

void BackendEngine::setRouteSize(int count) {
    route_size_ = std::max(1, count);
}

void BackendEngine::enqueue(Event evt) {
    queue_.push(std::move(evt));
}

std::vector<Action> BackendEngine::step() {
    if (queue_.empty()) {
        return {};
    }

    Event evt = queue_.front();
    queue_.pop();
    const Snapshot before = snapshot_;

    std::vector<Action> actions;
    std::visit([&](auto&& payload) {
        using T = std::decay_t<decltype(payload)>;
        if constexpr (std::is_same_v<T, CommandEvent>) {
            actions = handleCommand(payload, evt.timestamp);
        } else if constexpr (std::is_same_v<T, TelemetryEvent>) {
            actions = handleTelemetry(payload);
        } else if constexpr (std::is_same_v<T, TimerEvent>) {
            actions = handleTimer(payload, evt.timestamp);
        }
    }, evt.payload);

    if (snapshotChanged(before, snapshot_)) {
        logSnapshotChange(before, snapshot_);
    } else if (std::holds_alternative<CommandEvent>(evt.payload)) {
        const auto& cmd = std::get<CommandEvent>(evt.payload);
        std::cout << "[STATE] No transition for " << toString(cmd.type)
                  << " (system=" << toString(snapshot_.system)
                  << ", mission=" << toString(snapshot_.mission) << ")\n";
    }

    return actions;
}

std::vector<Action> BackendEngine::handleCommand(const CommandEvent& cmd,
                                                 const std::chrono::steady_clock::time_point& ts) {
    std::vector<Action> actions;
    switch (cmd.type) {
    case CommandType::CMD_START:
        if (snapshot_.system == SystemState::BOOT) {
            snapshot_.system = SystemState::IDLE;
        }
        break;
    case CommandType::CMD_SET_MANUAL:
        if (snapshot_.system == SystemState::IDLE) {
            snapshot_.system = SystemState::MANUAL;
        }
        break;
    case CommandType::CMD_SET_AUTO:
        if (snapshot_.system == SystemState::IDLE || snapshot_.system == SystemState::MANUAL) {
            snapshot_.system = SystemState::AUTO_NAV;
        }
        break;
    case CommandType::CMD_UPLOAD_ROUTE:
        actions.push_back({ActionType::ACT_NOTIFY_UI, "Route uploaded", 0.0, 0});
        break;
    case CommandType::CMD_START_MISSION:
        if (snapshot_.system == SystemState::AUTO_NAV) {
            snapshot_.system = SystemState::MISSION;
            snapshot_.current_waypoint = 0;
            snapshot_.last_error.clear();
            enterMissionState(MissionState::GOTO_WP, ts);
        }
        break;
    case CommandType::CMD_PAUSE:
        if (snapshot_.system == SystemState::MISSION || snapshot_.system == SystemState::AUTO_NAV) {
            snapshot_.system = SystemState::PAUSED;
        }
        break;
    case CommandType::CMD_RESUME:
        if (snapshot_.system == SystemState::PAUSED) {
            snapshot_.system = SystemState::AUTO_NAV;
        }
        break;
    case CommandType::CMD_ABORT:
        snapshot_.system = SystemState::RETURN_HOME;
        snapshot_.mission = MissionState::ABORTED;
        stage_start_ = ts;
        break;
    case CommandType::CMD_RETURN_HOME:
        snapshot_.system = SystemState::RETURN_HOME;
        stage_start_ = ts;
        break;
    case CommandType::CMD_STOP:
        snapshot_.system = SystemState::SHUTDOWN;
        break;
    case CommandType::CMD_MISSION_REACHED_WAYPOINT:
        if (snapshot_.system == SystemState::MISSION && snapshot_.mission == MissionState::GOTO_WP) {
            enterMissionState(MissionState::PREP, ts);
        }
        break;
    case CommandType::CMD_MISSION_PREP_DONE:
        if (snapshot_.system == SystemState::MISSION && snapshot_.mission == MissionState::PREP) {
            enterMissionState(MissionState::RINSE, ts);
        }
        break;
    case CommandType::CMD_MISSION_RINSE_DONE:
        if (snapshot_.system == SystemState::MISSION && snapshot_.mission == MissionState::RINSE) {
            enterMissionState(MissionState::SAMPLE, ts);
        }
        break;
    case CommandType::CMD_MISSION_SAMPLE_DONE:
        if (snapshot_.system == SystemState::MISSION && snapshot_.mission == MissionState::SAMPLE) {
            enterMissionState(MissionState::POST, ts);
        }
        break;
    case CommandType::CMD_MISSION_POST_DONE:
        if (snapshot_.system == SystemState::MISSION && snapshot_.mission == MissionState::POST) {
            snapshot_.current_waypoint += 1;
            if (snapshot_.current_waypoint >= route_size_) {
                snapshot_.system = SystemState::RETURN_HOME;
                enterMissionState(MissionState::DONE, ts);
                stage_start_ = ts;
            } else {
                enterMissionState(MissionState::GOTO_WP, ts);
            }
        }
        break;
    case CommandType::CMD_RETURN_HOME_DONE:
        if (snapshot_.system == SystemState::RETURN_HOME) {
            snapshot_.system = SystemState::IDLE;
            snapshot_.mission = MissionState::NONE;
            snapshot_.last_error.clear();
        }
        break;
    }
    (void)ts;
    return actions;
}

std::vector<Action> BackendEngine::handleTelemetry(const TelemetryEvent& tel) {
    std::vector<Action> actions;
    switch (tel.type) {
    case TelemetryType::TEL_POSE:
        snapshot_.pose = tel.pose;
        break;
    case TelemetryType::TEL_HEALTH:
        snapshot_.health = tel.health;
        break;
    case TelemetryType::TEL_BATTERY:
        snapshot_.battery = tel.battery;
        break;
    case TelemetryType::TEL_SAMPLER_STATUS:
        snapshot_.sampler = tel.sampler;
        break;
    }
    return actions;
}

std::vector<Action> BackendEngine::handleTimer(const TimerEvent& tim,
                                               const std::chrono::steady_clock::time_point& ts) {
    std::vector<Action> actions;
    switch (tim.type) {
    case TimerType::TICK:
        if (snapshot_.system == SystemState::RETURN_HOME) {
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(ts - stage_start_);
            if (elapsed >= return_home_duration_ && snapshot_.mission != MissionState::DONE && snapshot_.mission != MissionState::ABORTED) {
                snapshot_.system = SystemState::IDLE;
                snapshot_.mission = MissionState::NONE;
                snapshot_.last_error.clear();
            }
        }
        break;
    case TimerType::TIMEOUT_GOTO:
        snapshot_.mission = MissionState::FAILED;
        snapshot_.system = SystemState::FAULT;
        snapshot_.last_error = "GOTO timeout";
        actions.push_back({ActionType::ACT_FAIL, snapshot_.last_error, 0.0, 0});
        break;
    case TimerType::TIMEOUT_PREP:
        snapshot_.mission = MissionState::FAILED;
        snapshot_.system = SystemState::FAULT;
        snapshot_.last_error = "PREP timeout";
        actions.push_back({ActionType::ACT_FAIL, snapshot_.last_error, 0.0, 0});
        break;
    case TimerType::TIMEOUT_RINSE:
        snapshot_.mission = MissionState::FAILED;
        snapshot_.system = SystemState::FAULT;
        snapshot_.last_error = "RINSE timeout";
        actions.push_back({ActionType::ACT_FAIL, snapshot_.last_error, 0.0, 0});
        break;
    case TimerType::TIMEOUT_SAMPLE:
        snapshot_.mission = MissionState::FAILED;
        snapshot_.system = SystemState::FAULT;
        snapshot_.last_error = "SAMPLE timeout";
        actions.push_back({ActionType::ACT_FAIL, snapshot_.last_error, 0.0, 0});
        break;
    }
    return actions;
}

void BackendEngine::enterMissionState(MissionState next,
                                      const std::chrono::steady_clock::time_point& ts) {
    snapshot_.mission = next;
    stage_start_ = ts;
}

std::vector<Action> BackendEngine::missionTick(const std::chrono::steady_clock::time_point& ts) {
    std::vector<Action> actions;
    (void)ts;
    return actions;
}

} // namespace usv::backend
