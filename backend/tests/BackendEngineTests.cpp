#include "BackendEngine.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

using namespace usv::backend;

namespace {

using Clock = std::chrono::steady_clock;

std::vector<Action> processEvent(BackendEngine& engine,
                                 const Clock::time_point& ts,
                                 CommandType type) {
    engine.enqueue({ts, CommandEvent{type}});
    return engine.step();
}

std::vector<Action> processEvent(BackendEngine& engine,
                                 const Clock::time_point& ts,
                                 TimerType type) {
    engine.enqueue({ts, TimerEvent{type}});
    return engine.step();
}

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAIL] " << message << "\n";
    }
    assert(condition && "backend engine test assertion failed");
}

void testMissionProgression() {
    BackendEngine engine;
    const auto t0 = Clock::now();
    engine.setRouteSize(4);

    processEvent(engine, t0, CommandType::CMD_START);
    processEvent(engine, t0, CommandType::CMD_SET_AUTO);
    processEvent(engine, t0, CommandType::CMD_START_MISSION);

    const Snapshot& started = engine.snapshot();
    expect(started.system == SystemState::MISSION, "system should enter MISSION");
    expect(started.mission == MissionState::GOTO_WP, "mission should enter GOTO_WP");

    processEvent(engine, t0, CommandType::CMD_MISSION_REACHED_WAYPOINT);
    expect(engine.snapshot().mission == MissionState::PREP, "arrival should move mission to PREP");

    processEvent(engine, t0, CommandType::CMD_MISSION_PREP_DONE);
    expect(engine.snapshot().mission == MissionState::RINSE, "prep done should move mission to RINSE");

    processEvent(engine, t0, CommandType::CMD_MISSION_RINSE_DONE);
    expect(engine.snapshot().mission == MissionState::SAMPLE, "rinse done should move mission to SAMPLE");

    processEvent(engine, t0, CommandType::CMD_MISSION_SAMPLE_DONE);
    expect(engine.snapshot().mission == MissionState::POST, "sample done should move mission to POST");

    processEvent(engine, t0, CommandType::CMD_MISSION_POST_DONE);
    expect(engine.snapshot().mission == MissionState::GOTO_WP, "post done should move to next GOTO_WP");
    expect(engine.snapshot().current_waypoint == 1, "post done should increment waypoint");
}

void testPauseAndResume() {
    BackendEngine engine;
    const auto t0 = Clock::now();

    processEvent(engine, t0, CommandType::CMD_START);
    processEvent(engine, t0, CommandType::CMD_SET_AUTO);
    processEvent(engine, t0, CommandType::CMD_START_MISSION);
    expect(engine.snapshot().system == SystemState::MISSION, "system should be in MISSION before pause");

    processEvent(engine, t0, CommandType::CMD_PAUSE);
    expect(engine.snapshot().system == SystemState::PAUSED, "pause should move system to PAUSED");

    processEvent(engine, t0, CommandType::CMD_RESUME);
    expect(engine.snapshot().system == SystemState::AUTO_NAV, "resume should move system back to AUTO_NAV");
}

void testTimeoutFaultHandling() {
    BackendEngine engine;
    const auto t0 = Clock::now();

    processEvent(engine, t0, CommandType::CMD_START);
    processEvent(engine, t0, CommandType::CMD_SET_AUTO);
    processEvent(engine, t0, CommandType::CMD_START_MISSION);

    auto actions = processEvent(engine, t0, TimerType::TIMEOUT_SAMPLE);
    expect(engine.snapshot().system == SystemState::FAULT, "timeout should move system to FAULT");
    expect(engine.snapshot().mission == MissionState::FAILED, "timeout should mark mission as FAILED");
    expect(engine.snapshot().last_error == "SAMPLE timeout", "timeout should record the correct error");
    expect(!actions.empty(), "timeout should emit a fail action");
    expect(actions.front().type == ActionType::ACT_FAIL, "timeout action should be ACT_FAIL");
}

} // namespace

int main() {
    testMissionProgression();
    testPauseAndResume();
    testTimeoutFaultHandling();
    std::cout << "[PASS] backend_engine_tests\n";
    return 0;
}
