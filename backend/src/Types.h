#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace usv::backend {

// -----------------------
// System-level enumerations
// -----------------------
enum class SystemState {
    BOOT,        // 启动/自检
    IDLE,        // 待命
    MANUAL,      // 手动遥控
    AUTO_NAV,    // 自动航行（未进入采样任务）
    MISSION,     // 任务执行中（采样流程）
    RETURN_HOME, // 自动返航
    PAUSED,      // 暂停
    FAULT,       // 故障保护
    SHUTDOWN     // 关闭/退出
};

enum class MissionState {
    NONE,    // 未开始或已结束
    INIT,    // 任务初始化
    GOTO_WP, // 前往采样点
    PREP,    // 到点准备（通知模块/放绞盘等）
    RINSE,   // 润洗阶段
    SAMPLE,  // 正式采样
    POST,    // 收尾整理
    NEXT,    // 判断下一个采样点
    DONE,    // 全部完成
    ABORTED, // 任务被中止
    FAILED   // 任务失败/超时
};

// -----------------------
// Commands & telemetry events
// -----------------------
enum class CommandType {
    CMD_START,
    CMD_STOP,
    CMD_SET_MANUAL,
    CMD_SET_AUTO,
    CMD_UPLOAD_ROUTE,
    CMD_START_MISSION,
    CMD_PAUSE,
    CMD_RESUME,
    CMD_ABORT,
    CMD_RETURN_HOME,
    CMD_MISSION_REACHED_WAYPOINT,
    CMD_MISSION_PREP_DONE,
    CMD_MISSION_RINSE_DONE,
    CMD_MISSION_SAMPLE_DONE,
    CMD_MISSION_POST_DONE,
    CMD_RETURN_HOME_DONE
};

enum class TelemetryType {
    TEL_POSE,
    TEL_HEALTH,
    TEL_BATTERY,
    TEL_SAMPLER_STATUS
};

enum class TimerType {
    TICK,
    TIMEOUT_GOTO,
    TIMEOUT_RINSE,
    TIMEOUT_SAMPLE,
    TIMEOUT_PREP
};

// Pose & telemetry snapshots (minimal)
struct Pose {
    double latitude{0.0};
    double longitude{0.0};
    double heading_deg{0.0};
    double speed_mps{0.0};
};

struct Health {
    bool gnss_ok{true};
    bool motor_ok{true};
    bool rudder_ok{true};
    bool sampler_ok{true};
};

struct Battery {
    double soc{1.0}; // 0.0 - 1.0
};

struct SamplerStatus {
    bool winch_ready{false};
    bool pump_ready{false};
    bool rinsing{false};
    bool sampling{false};
};

// Events
struct CommandEvent {
    CommandType type;
};

struct TelemetryEvent {
    TelemetryType type;
    Pose pose{};          // used when type == TEL_POSE
    Health health{};      // used when type == TEL_HEALTH
    Battery battery{};    // used when type == TEL_BATTERY
    SamplerStatus sampler{}; // used when type == TEL_SAMPLER_STATUS
};

struct TimerEvent {
    TimerType type;
};

struct Event {
    std::chrono::steady_clock::time_point timestamp;
    std::variant<CommandEvent, TelemetryEvent, TimerEvent> payload;
};

// -----------------------
// Actions (intent only)
// -----------------------
enum class ActionType {
    ACT_SET_THRUST,
    ACT_SET_RUDDER,
    ACT_GOTO_WAYPOINT,
    ACT_WINCH_TO_DEPTH,
    ACT_PUMP_ON,
    ACT_PUMP_OFF,
    ACT_NOTIFY_UI,
    ACT_FAIL
};

struct Action {
    ActionType type;
    std::string message;  // for ACT_NOTIFY_UI / ACT_FAIL
    double value{0.0};    // thrust/rudder/timeout/depth generic field
    int index{0};         // waypoint index
};

// -----------------------
// Snapshot (published state)
// -----------------------
struct Snapshot {
    SystemState system{SystemState::BOOT};
    MissionState mission{MissionState::NONE};
    int current_waypoint{0};
    Pose pose{};
    Battery battery{};
    Health health{};
    SamplerStatus sampler{};
    std::string last_error;
};

} // namespace usv::backend
