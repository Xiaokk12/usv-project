#include "MockHardware.h"
#include "BackendProtocol.h"

#include <algorithm>
#include <iostream>

namespace usv::backend {

namespace {

constexpr auto waypointDelay = std::chrono::milliseconds(300);
constexpr auto prepDelay = std::chrono::milliseconds(500);
constexpr auto rinseDelay = std::chrono::milliseconds(600);
constexpr auto sampleDelay = std::chrono::milliseconds(800);
constexpr auto postDelay = std::chrono::milliseconds(500);
constexpr auto returnDelay = std::chrono::milliseconds(800);

} // namespace

void MockHardware::submit(const Action& action,
                          const Snapshot& snapshot,
                          const std::chrono::steady_clock::time_point& now) {
    std::cout << "[MOCK_HW] accept action=" << static_cast<int>(action.type)
              << " wp=" << action.index
              << " mission=" << protocol::toString(snapshot.mission) << "\n";

    switch (action.type) {
    case ActionType::ACT_GOTO_WAYPOINT:
        scheduleCommand(CommandType::CMD_MISSION_REACHED_WAYPOINT, waypointDelay, now, "WAYPOINT_REACHED");
        break;
    case ActionType::ACT_PREPARE_WATER_SAMPLING: {
        TelemetryEvent tel;
        tel.type = TelemetryType::TEL_SAMPLER_STATUS;
        tel.sampler = {true, false, false, false};
        scheduleTelemetry(tel, std::chrono::milliseconds(0), now, "SAMPLER_PREP");
        scheduleCommand(CommandType::CMD_MISSION_PREP_DONE, prepDelay, now, "PREPARE_OK");
        break;
    }
    case ActionType::ACT_START_RINSE: {
        TelemetryEvent tel;
        tel.type = TelemetryType::TEL_SAMPLER_STATUS;
        tel.sampler = {true, true, true, false};
        scheduleTelemetry(tel, std::chrono::milliseconds(0), now, "RINSE_ON");
        scheduleCommand(CommandType::CMD_MISSION_RINSE_DONE, rinseDelay, now, "FLUSH_DONE");
        break;
    }
    case ActionType::ACT_START_SAMPLING: {
        TelemetryEvent tel;
        tel.type = TelemetryType::TEL_SAMPLER_STATUS;
        tel.sampler = {true, true, false, true};
        scheduleTelemetry(tel, std::chrono::milliseconds(0), now, "SAMPLING_ON");
        scheduleCommand(CommandType::CMD_MISSION_SAMPLE_DONE, sampleDelay, now, "WATER_DELIVERED");
        break;
    }
    case ActionType::ACT_POST_PROCESS: {
        TelemetryEvent tel;
        tel.type = TelemetryType::TEL_SAMPLER_STATUS;
        tel.sampler = {true, false, false, false};
        scheduleTelemetry(tel, std::chrono::milliseconds(0), now, "POST_PROCESS");
        scheduleCommand(CommandType::CMD_MISSION_POST_DONE, postDelay, now, "RECOVER_DONE");
        break;
    }
    case ActionType::ACT_RECOVER_PROBE:
        scheduleCommand(CommandType::CMD_MISSION_POST_DONE, postDelay, now, "RECOVER_DONE");
        break;
    case ActionType::ACT_RETURN_HOME:
        scheduleCommand(CommandType::CMD_RETURN_HOME_DONE, returnDelay, now, "RETURN_HOME_DONE");
        break;
    case ActionType::ACT_EMERGENCY_STOP: {
        TelemetryEvent tel;
        tel.type = TelemetryType::TEL_SAMPLER_STATUS;
        tel.sampler = {false, false, false, false};
        scheduleTelemetry(tel, std::chrono::milliseconds(0), now, "EMERGENCY_STOPPED");
        break;
    }
    case ActionType::ACT_QUERY_STATUS: {
        TelemetryEvent health;
        health.type = TelemetryType::TEL_HEALTH;
        health.health = {true, true, true, true};
        scheduleTelemetry(health, std::chrono::milliseconds(0), now, "STATUS_HEALTH");

        TelemetryEvent sampler;
        sampler.type = TelemetryType::TEL_SAMPLER_STATUS;
        sampler.sampler = {false, false, false, false};
        scheduleTelemetry(sampler, std::chrono::milliseconds(0), now, "STATUS_SAMPLER");
        break;
    }
    case ActionType::ACT_SET_THRUST:
    case ActionType::ACT_SET_RUDDER:
    case ActionType::ACT_WINCH_TO_DEPTH:
    case ActionType::ACT_PUMP_ON:
    case ActionType::ACT_PUMP_OFF:
    case ActionType::ACT_NOTIFY_UI:
    case ActionType::ACT_FAIL:
        break;
    }
}

std::vector<Event> MockHardware::tick(const std::chrono::steady_clock::time_point& now) {
    std::vector<Event> ready;
    auto it = pending_.begin();
    while (it != pending_.end()) {
        if (it->due <= now) {
            std::cout << "[MOCK_HW] emit " << it->label << "\n";
            ready.push_back(std::move(it->event));
            it = pending_.erase(it);
        } else {
            ++it;
        }
    }
    return ready;
}

void MockHardware::scheduleCommand(CommandType type,
                                   std::chrono::milliseconds delay,
                                   const std::chrono::steady_clock::time_point& now,
                                   const std::string& label) {
    pending_.push_back({
        now + delay,
        Event{now + delay, CommandEvent{type}},
        label
    });
}

void MockHardware::scheduleTelemetry(TelemetryEvent telemetry,
                                     std::chrono::milliseconds delay,
                                     const std::chrono::steady_clock::time_point& now,
                                     const std::string& label) {
    pending_.push_back({
        now + delay,
        Event{now + delay, telemetry},
        label
    });
}

} // namespace usv::backend
