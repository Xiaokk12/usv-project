#include "BackendEngine.h"
#include "BackendProtocol.h"

#include <arpa/inet.h>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/select.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

using namespace usv::backend;

namespace {

void printSnapshot(const Snapshot& snap) {
    std::cout << "[SNAPSHOT] sys=" << protocol::toString(snap.system)
              << " mission=" << protocol::toString(snap.mission)
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

int createListenSocket(uint16_t port) {
    const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    int reuse = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt");
        ::close(fd);
        return -1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("bind");
        ::close(fd);
        return -1;
    }

    if (listen(fd, 1) < 0) {
        perror("listen");
        ::close(fd);
        return -1;
    }

    return fd;
}

bool sendLine(int fd, const std::string& line) {
    const std::string payload = line + "\n";
    const char* data = payload.c_str();
    std::size_t total = 0;
    while (total < payload.size()) {
        const ssize_t sent = ::send(fd, data + total, payload.size() - total, 0);
        if (sent <= 0) {
            return false;
        }
        total += static_cast<std::size_t>(sent);
    }
    return true;
}

void runDemo() {
    BackendEngine engine;
    auto now = std::chrono::steady_clock::now();

    engine.enqueue({now, CommandEvent{CommandType::CMD_START}});
    engine.enqueue({now, CommandEvent{CommandType::CMD_SET_AUTO}});
    engine.enqueue({now, CommandEvent{CommandType::CMD_UPLOAD_ROUTE}});
    engine.enqueue({now, CommandEvent{CommandType::CMD_START_MISSION}});

    const auto tickInterval = std::chrono::milliseconds(100);
    for (int i = 0; i < 300; ++i) {
        const auto ts = std::chrono::steady_clock::now();
        engine.enqueue({ts, TimerEvent{TimerType::TICK}});
        const auto actions = engine.step();
        if (!actions.empty()) {
            printActions(actions);
        }
        printSnapshot(engine.snapshot());
        std::this_thread::sleep_for(tickInterval);
    }
}

std::optional<CommandEvent> parseCommandLine(const std::string& line) {
    std::istringstream in(line);
    std::string prefix;
    std::string name;
    if (!(in >> prefix)) {
        return std::nullopt;
    }

    if (prefix == "CMD" && (in >> name)) {
        CommandType type{};
        if (protocol::parseCommandType(name, &type)) {
            return CommandEvent{type};
        }
    }

    return std::nullopt;
}

bool handleClientLine(BackendEngine& engine, const std::string& line) {
    if (line.empty()) {
        return true;
    }

    std::istringstream in(line);
    std::string prefix;
    in >> prefix;

    if (prefix == "CMD") {
        const auto cmd = parseCommandLine(line);
        if (!cmd) {
            std::cout << "[NET] Invalid command line: " << line << "\n";
            return false;
        }
        engine.enqueue({std::chrono::steady_clock::now(), *cmd});
    } else if (prefix == "ROUTE") {
        int routeSize = 0;
        if (!(in >> routeSize)) {
            std::cout << "[NET] Invalid route line: " << line << "\n";
            return false;
        }
        engine.setRouteSize(routeSize);
        std::cout << "[NET] Route size set to " << routeSize << "\n";
        return true;
    } else if (prefix == "TEL_HEALTH") {
        int gnss = 0;
        int motor = 0;
        int rudder = 0;
        int sampler = 0;
        if (!(in >> gnss >> motor >> rudder >> sampler)) {
            std::cout << "[NET] Invalid health line: " << line << "\n";
            return false;
        }
        TelemetryEvent tel;
        tel.type = TelemetryType::TEL_HEALTH;
        tel.health = {gnss != 0, motor != 0, rudder != 0, sampler != 0};
        engine.enqueue({std::chrono::steady_clock::now(), tel});
    } else if (prefix == "TEL_BATTERY") {
        double soc = 0.0;
        if (!(in >> soc)) {
            std::cout << "[NET] Invalid battery line: " << line << "\n";
            return false;
        }
        TelemetryEvent tel;
        tel.type = TelemetryType::TEL_BATTERY;
        tel.battery = {soc};
        engine.enqueue({std::chrono::steady_clock::now(), tel});
    } else if (prefix == "TEL_SAMPLER") {
        int winch = 0;
        int pump = 0;
        int rinsing = 0;
        int sampling = 0;
        if (!(in >> winch >> pump >> rinsing >> sampling)) {
            std::cout << "[NET] Invalid sampler line: " << line << "\n";
            return false;
        }
        TelemetryEvent tel;
        tel.type = TelemetryType::TEL_SAMPLER_STATUS;
        tel.sampler = {winch != 0, pump != 0, rinsing != 0, sampling != 0};
        engine.enqueue({std::chrono::steady_clock::now(), tel});
    } else if (prefix == "TEL_POSE") {
        double lat = 0.0;
        double lon = 0.0;
        double heading = 0.0;
        double speed = 0.0;
        if (!(in >> lat >> lon >> heading >> speed)) {
            std::cout << "[NET] Invalid pose line: " << line << "\n";
            return false;
        }
        TelemetryEvent tel;
        tel.type = TelemetryType::TEL_POSE;
        tel.pose = {lat, lon, heading, speed};
        engine.enqueue({std::chrono::steady_clock::now(), tel});
    } else if (prefix == "TICK") {
        engine.enqueue({std::chrono::steady_clock::now(), TimerEvent{TimerType::TICK}});
    } else {
        std::cout << "[NET] Unknown line: " << line << "\n";
        return false;
    }

    const auto actions = engine.step();
    if (!actions.empty()) {
        printActions(actions);
    }
    return true;
}

int runServer(uint16_t port) {
    BackendEngine engine;
    const int listenFd = createListenSocket(port);
    if (listenFd < 0) {
        return 1;
    }

    std::cout << "[NET] backend_app listening on 0.0.0.0:" << port << "\n";
    std::cout << "[NET] start the Mac client with USV_BACKEND_HOST=<pi-ip>\n";

    int clientFd = -1;
    std::string recvBuffer;
    auto nextTick = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);

    while (true) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(listenFd, &readfds);
        int maxFd = listenFd;
        if (clientFd >= 0) {
            FD_SET(clientFd, &readfds);
            maxFd = std::max(maxFd, clientFd);
        }

        const auto now = std::chrono::steady_clock::now();
        auto wait = std::chrono::duration_cast<std::chrono::milliseconds>(nextTick - now);
        if (wait.count() < 0) {
            wait = std::chrono::milliseconds(0);
        }

        timeval timeout{};
        timeout.tv_sec = static_cast<long>(wait.count() / 1000);
        timeout.tv_usec = static_cast<long>((wait.count() % 1000) * 1000);

        const int ready = ::select(maxFd + 1, &readfds, nullptr, nullptr, &timeout);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("select");
            break;
        }

        if (FD_ISSET(listenFd, &readfds)) {
            sockaddr_in clientAddr{};
            socklen_t clientLen = sizeof(clientAddr);
            const int accepted = ::accept(listenFd, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
            if (accepted >= 0) {
                if (clientFd >= 0) {
                    ::close(clientFd);
                }
                clientFd = accepted;
                recvBuffer.clear();
                std::cout << "[NET] client connected from " << inet_ntoa(clientAddr.sin_addr) << ":"
                          << ntohs(clientAddr.sin_port) << "\n";
                sendLine(clientFd, protocol::encodeSnapshotLine(engine.snapshot()));
            }
        }

        if (clientFd >= 0 && FD_ISSET(clientFd, &readfds)) {
            char buffer[2048];
            const ssize_t received = ::recv(clientFd, buffer, sizeof(buffer), 0);
            if (received <= 0) {
                std::cout << "[NET] client disconnected\n";
                ::close(clientFd);
                clientFd = -1;
                recvBuffer.clear();
            } else {
                recvBuffer.append(buffer, static_cast<std::size_t>(received));
                std::size_t newlinePos = std::string::npos;
                while ((newlinePos = recvBuffer.find('\n')) != std::string::npos) {
                    std::string line = recvBuffer.substr(0, newlinePos);
                    recvBuffer.erase(0, newlinePos + 1);
                    if (!line.empty() && line.back() == '\r') {
                        line.pop_back();
                    }
                    if (!handleClientLine(engine, line)) {
                        continue;
                    }
                    if (clientFd >= 0 && !sendLine(clientFd, protocol::encodeSnapshotLine(engine.snapshot()))) {
                        std::cout << "[NET] failed to write snapshot to client\n";
                        ::close(clientFd);
                        clientFd = -1;
                        recvBuffer.clear();
                    }
                }
            }
        }

        const auto tickNow = std::chrono::steady_clock::now();
        while (tickNow >= nextTick) {
            engine.enqueue({tickNow, TimerEvent{TimerType::TICK}});
            const auto actions = engine.step();
            if (!actions.empty()) {
                printActions(actions);
            }
            if (clientFd >= 0 && !sendLine(clientFd, protocol::encodeSnapshotLine(engine.snapshot()))) {
                std::cout << "[NET] failed to write tick snapshot to client\n";
                ::close(clientFd);
                clientFd = -1;
                recvBuffer.clear();
            }
            nextTick += std::chrono::milliseconds(100);
        }
    }

    if (clientFd >= 0) {
        ::close(clientFd);
    }
    ::close(listenFd);
    return 1;
}

} // namespace

int main(int argc, char** argv) {
    bool demoMode = false;
    uint16_t port = 45454;

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "--demo") {
            demoMode = true;
        } else if (arg == "--port" && i + 1 < argc) {
            port = static_cast<uint16_t>(std::stoi(argv[++i]));
        }
    }

    if (demoMode) {
        runDemo();
        return 0;
    }

    return runServer(port);
}
