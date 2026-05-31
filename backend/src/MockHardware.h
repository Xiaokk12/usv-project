#pragma once

#include "HardwareAdapter.h"

#include <deque>
#include <string>

namespace usv::backend {

class MockHardware final : public HardwareAdapter {
public:
    void submit(const Action& action,
                const Snapshot& snapshot,
                const std::chrono::steady_clock::time_point& now) override;
    std::vector<Event> tick(const std::chrono::steady_clock::time_point& now) override;

private:
    struct PendingEvent {
        std::chrono::steady_clock::time_point due;
        Event event;
        std::string label;
    };

    void scheduleCommand(CommandType type,
                         std::chrono::milliseconds delay,
                         const std::chrono::steady_clock::time_point& now,
                         const std::string& label);
    void scheduleTelemetry(TelemetryEvent telemetry,
                           std::chrono::milliseconds delay,
                           const std::chrono::steady_clock::time_point& now,
                           const std::string& label);

    std::deque<PendingEvent> pending_;
};

} // namespace usv::backend
