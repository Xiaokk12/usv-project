#pragma once

#include "Types.h"

#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string>
#include <unordered_map>

namespace usv::backend::protocol {

inline const char* toString(SystemState s) {
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

inline const char* toString(MissionState s) {
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

inline const char* toString(CommandType cmd) {
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

inline bool parseSystemState(const std::string& text, SystemState* out) {
    if (text == "BOOT") *out = SystemState::BOOT;
    else if (text == "IDLE") *out = SystemState::IDLE;
    else if (text == "MANUAL") *out = SystemState::MANUAL;
    else if (text == "AUTO_NAV") *out = SystemState::AUTO_NAV;
    else if (text == "MISSION") *out = SystemState::MISSION;
    else if (text == "RETURN_HOME") *out = SystemState::RETURN_HOME;
    else if (text == "PAUSED") *out = SystemState::PAUSED;
    else if (text == "FAULT") *out = SystemState::FAULT;
    else if (text == "SHUTDOWN") *out = SystemState::SHUTDOWN;
    else return false;
    return true;
}

inline bool parseMissionState(const std::string& text, MissionState* out) {
    if (text == "NONE") *out = MissionState::NONE;
    else if (text == "INIT") *out = MissionState::INIT;
    else if (text == "GOTO_WP") *out = MissionState::GOTO_WP;
    else if (text == "PREP") *out = MissionState::PREP;
    else if (text == "RINSE") *out = MissionState::RINSE;
    else if (text == "SAMPLE") *out = MissionState::SAMPLE;
    else if (text == "POST") *out = MissionState::POST;
    else if (text == "NEXT") *out = MissionState::NEXT;
    else if (text == "DONE") *out = MissionState::DONE;
    else if (text == "ABORTED") *out = MissionState::ABORTED;
    else if (text == "FAILED") *out = MissionState::FAILED;
    else return false;
    return true;
}

inline bool parseCommandType(const std::string& text, CommandType* out) {
    if (text == "CMD_START") *out = CommandType::CMD_START;
    else if (text == "CMD_STOP") *out = CommandType::CMD_STOP;
    else if (text == "CMD_SET_MANUAL") *out = CommandType::CMD_SET_MANUAL;
    else if (text == "CMD_SET_AUTO") *out = CommandType::CMD_SET_AUTO;
    else if (text == "CMD_UPLOAD_ROUTE") *out = CommandType::CMD_UPLOAD_ROUTE;
    else if (text == "CMD_START_MISSION") *out = CommandType::CMD_START_MISSION;
    else if (text == "CMD_PAUSE") *out = CommandType::CMD_PAUSE;
    else if (text == "CMD_RESUME") *out = CommandType::CMD_RESUME;
    else if (text == "CMD_ABORT") *out = CommandType::CMD_ABORT;
    else if (text == "CMD_RETURN_HOME") *out = CommandType::CMD_RETURN_HOME;
    else if (text == "CMD_MISSION_REACHED_WAYPOINT") *out = CommandType::CMD_MISSION_REACHED_WAYPOINT;
    else if (text == "CMD_MISSION_PREP_DONE") *out = CommandType::CMD_MISSION_PREP_DONE;
    else if (text == "CMD_MISSION_RINSE_DONE") *out = CommandType::CMD_MISSION_RINSE_DONE;
    else if (text == "CMD_MISSION_SAMPLE_DONE") *out = CommandType::CMD_MISSION_SAMPLE_DONE;
    else if (text == "CMD_MISSION_POST_DONE") *out = CommandType::CMD_MISSION_POST_DONE;
    else if (text == "CMD_RETURN_HOME_DONE") *out = CommandType::CMD_RETURN_HOME_DONE;
    else return false;
    return true;
}

inline std::string escapeToken(const std::string& value) {
    std::ostringstream out;
    for (unsigned char ch : value) {
        if (std::isalnum(ch) || ch == '_' || ch == '-' || ch == '.') {
            out << static_cast<char>(ch);
        } else {
            static const char* hex = "0123456789ABCDEF";
            out << '%' << hex[(ch >> 4) & 0xF] << hex[ch & 0xF];
        }
    }
    return out.str();
}

inline int fromHex(char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}

inline std::string unescapeToken(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '%' && i + 2 < value.size()) {
            const int hi = fromHex(value[i + 1]);
            const int lo = fromHex(value[i + 2]);
            if (hi >= 0 && lo >= 0) {
                out.push_back(static_cast<char>((hi << 4) | lo));
                i += 2;
                continue;
            }
        }
        out.push_back(value[i]);
    }
    return out;
}

inline std::string encodeSnapshotLine(const Snapshot& snapshot) {
    std::ostringstream out;
    out << "SNAPSHOT"
        << " system=" << toString(snapshot.system)
        << " mission=" << toString(snapshot.mission)
        << " waypoint=" << snapshot.current_waypoint
        << " soc=" << snapshot.battery.soc
        << " gnss=" << (snapshot.health.gnss_ok ? 1 : 0)
        << " motor=" << (snapshot.health.motor_ok ? 1 : 0)
        << " rudder=" << (snapshot.health.rudder_ok ? 1 : 0)
        << " sampler=" << (snapshot.health.sampler_ok ? 1 : 0)
        << " winch=" << (snapshot.sampler.winch_ready ? 1 : 0)
        << " pump=" << (snapshot.sampler.pump_ready ? 1 : 0)
        << " rinsing=" << (snapshot.sampler.rinsing ? 1 : 0)
        << " sampling=" << (snapshot.sampler.sampling ? 1 : 0)
        << " error=" << escapeToken(snapshot.last_error.empty() ? "none" : snapshot.last_error);
    return out.str();
}

inline bool parseBoolInt(const std::string& text, bool* out) {
    if (text == "1") {
        *out = true;
        return true;
    }
    if (text == "0") {
        *out = false;
        return true;
    }
    return false;
}

inline bool decodeSnapshotLine(const std::string& line, Snapshot* snapshot) {
    if (!snapshot) {
        return false;
    }

    std::istringstream in(line);
    std::string prefix;
    if (!(in >> prefix) || prefix != "SNAPSHOT") {
        return false;
    }

    std::unordered_map<std::string, std::string> kv;
    std::string token;
    while (in >> token) {
        const auto pos = token.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        kv[token.substr(0, pos)] = token.substr(pos + 1);
    }

    Snapshot parsed = *snapshot;
    if (!parseSystemState(kv["system"], &parsed.system)) {
        return false;
    }
    if (!parseMissionState(kv["mission"], &parsed.mission)) {
        return false;
    }

    parsed.current_waypoint = std::stoi(kv["waypoint"]);
    parsed.battery.soc = std::stod(kv["soc"]);

    if (!parseBoolInt(kv["gnss"], &parsed.health.gnss_ok)
        || !parseBoolInt(kv["motor"], &parsed.health.motor_ok)
        || !parseBoolInt(kv["rudder"], &parsed.health.rudder_ok)
        || !parseBoolInt(kv["sampler"], &parsed.health.sampler_ok)
        || !parseBoolInt(kv["winch"], &parsed.sampler.winch_ready)
        || !parseBoolInt(kv["pump"], &parsed.sampler.pump_ready)
        || !parseBoolInt(kv["rinsing"], &parsed.sampler.rinsing)
        || !parseBoolInt(kv["sampling"], &parsed.sampler.sampling)) {
        return false;
    }

    const std::string error = unescapeToken(kv["error"]);
    parsed.last_error = (error == "none") ? std::string{} : error;

    *snapshot = parsed;
    return true;
}

} // namespace usv::backend::protocol
