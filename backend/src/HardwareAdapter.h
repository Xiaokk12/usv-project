#pragma once

#include "Types.h"

#include <chrono>
#include <vector>

namespace usv::backend {

class HardwareAdapter {
public:
    virtual ~HardwareAdapter() = default;

    virtual void submit(const Action& action,
                        const Snapshot& snapshot,
                        const std::chrono::steady_clock::time_point& now) = 0;
    virtual std::vector<Event> tick(const std::chrono::steady_clock::time_point& now) = 0;
};

} // namespace usv::backend
