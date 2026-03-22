#include "BackendEngine.h"

#include <chrono>
#include <iostream>
#include <thread>

using namespace usv::backend;

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

void printSnapshot(const Snapshot& snap) {
    std::cout << "[SNAPSHOT] sys=" << toString(snap.system)
              << " mission=" << toString(snap.mission)
              << " wp=" << snap.current_waypoint
              << " soc=" << snap.battery.soc
              << " last_error=" << (snap.last_error.empty() ? "none" : snap.last_error)
              << "\n";
}

void printActions(const std::vector<Action>& actions) {
    for (const auto& a : actions) {
        std::cout << "[ACTION] type=" << static_cast<int>(a.type)
                  << " msg=" << (a.message.empty() ? "-" : a.message)
                  << " value=" << a.value
                  << " index=" << a.index << "\n";
    }
}

} // namespace

int main() {
    BackendEngine engine;
    auto now = std::chrono::steady_clock::now();

    // Demo: BOOT -> IDLE -> AUTO_NAV -> MISSION -> DONE -> RETURN_HOME -> IDLE
    engine.enqueue({now, CommandEvent{CommandType::CMD_START}});
    engine.enqueue({now, CommandEvent{CommandType::CMD_SET_AUTO}});
    engine.enqueue({now, CommandEvent{CommandType::CMD_UPLOAD_ROUTE}});
    engine.enqueue({now, CommandEvent{CommandType::CMD_START_MISSION}});

    const auto tick_interval = std::chrono::milliseconds(100);
    for (int i = 0; i < 300; ++i) {
        auto ts = std::chrono::steady_clock::now();
        engine.enqueue({ts, TimerEvent{TimerType::TICK}});
        auto actions = engine.step();
        if (!actions.empty()) {
            printActions(actions);
        }
        printSnapshot(engine.snapshot());
        std::this_thread::sleep_for(tick_interval);
    }

    return 0;
}
