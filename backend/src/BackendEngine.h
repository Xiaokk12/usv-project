#pragma once

#include "Types.h"

#include <chrono>
#include <queue>
#include <vector>

namespace usv::backend {

class BackendEngine {
public:
    BackendEngine();
    void setRouteSize(int count);

    // Push an external event into the queue (command / telemetry / timer)
    void enqueue(Event evt);

    // Process one event if available; returns actions generated.
    // If no events, returns empty vector.
    std::vector<Action> step();

    // Access current snapshot (read-only)
    const Snapshot& snapshot() const { return snapshot_; }

private:
    // Internal helpers
    std::vector<Action> handleCommand(const CommandEvent& cmd, const std::chrono::steady_clock::time_point& ts);
    std::vector<Action> handleTelemetry(const TelemetryEvent& tel);
    std::vector<Action> handleTimer(const TimerEvent& tim, const std::chrono::steady_clock::time_point& ts);

    void enterMissionState(MissionState next, const std::chrono::steady_clock::time_point& ts);
    std::vector<Action> missionTick(const std::chrono::steady_clock::time_point& ts);

    std::queue<Event> queue_;
    Snapshot snapshot_;

    // mission timing
    std::chrono::steady_clock::time_point stage_start_{};

    // simple simulated route
    int route_size_{0};

    // durations for mission stages (simulated)
    std::chrono::milliseconds return_home_duration_{2000};
};

} // namespace usv::backend
