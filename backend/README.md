# Backend

当前后端包含两部分：
- `BackendEngine`：最小可运行状态机核心
- `backend_app`：基于 TCP 的轻量服务端，可部署在树莓派

功能包含：事件队列 + 状态机流转 + Action/Snapshot 输出 + TCP 命令/快照收发。

## Features
- 两层状态：SystemState + MissionState
- 事件驱动：Command / Telemetry / Timer
- Demo：自动跑一段流程  
  BOOT -> IDLE -> AUTO_NAV -> MISSION(简化) -> DONE -> RETURN_HOME -> IDLE

  BOOT：启动自检阶段，尚未对外工作；收到 CMD_START 才进入待命。
  IDLE：待命/安全态，可接收模式切换、航线上传和任务启动。
  MANUAL：手动遥控模式（预留）；由 CMD_SET_MANUAL 进入。
  AUTO_NAV：自动航行模式，按航线巡航但未进入任务流程；从 IDLE/PAUSED 恢复后到此。
  MISSION：任务执行中（采样流程），内部再细分 MissionState（INIT/GOTO_WP/...）。

                                NONE：任务未开始或已结束的空状态。
                                INIT：任务初始化阶段（入口），进行基础准备。
                                GOTO_WP：驶向当前采样点/航点。
                                PREP：到点后准备采样（通知模块、放绞盘等）。
                                RINSE：润洗阶段，泵/流水预处理。
                                SAMPLE：正式采样阶段。
                                POST：收尾整理（关泵、收绞盘、记录）。
                                NEXT：决定是否有下一采样点，递增航点索引或结束。
                                DONE：全部采样点完成的终态（随后系统会转 RETURN_HOME/IDLE）。
                                ABORTED：任务被中止（外部命令或异常触发）。
                                FAILED：任务失败/超时/故障保护。

  RETURN_HOME：自动返航，任务结束或中止后进入此态（当前 demo 直接切回 IDLE）。
  PAUSED：任务/自动航行暂停，等待 CMD_RESUME；安全保持。
  FAULT：故障保护态（如超时/异常），停止正常动作，等待人工处理或复位。
  SHUTDOWN：关闭/退出态，表示进程准备终止或已终止。

- 输出：stdout 打印状态与 Action

## Build
```bash
cmake -S backend -B build/backend -G Ninja
cmake --build build/backend -j
```

## Run

### Demo 模式
本地演示状态机自动打印：

```bash
./build/backend/backend_app --demo
```

### TCP 服务端模式
默认启动 TCP 服务端，监听 `45454` 端口，供 Mac 上位机连接：

```bash
./build/backend/backend_app --port 45454
```

启动后会看到：

```text
[NET] backend_app listening on 0.0.0.0:45454
```

联调时：
- 上位机发送 `CMD ...`、`TICK`、`TEL_*` 到树莓派
- 后端在树莓派上处理状态机
- 后端回传 `SNAPSHOT ...` 给上位机
- 树莓派终端继续打印 `[STATE] ...`

## Test
```bash
ctest --test-dir build/backend --output-on-failure
```

## Notes
- 状态机核心在 [backend/src/BackendEngine.cpp](/Users/xiaokk/Projects/usv_project/backend/src/BackendEngine.cpp)。
- TCP 协议编解码在 [backend/src/BackendProtocol.h](/Users/xiaokk/Projects/usv_project/backend/src/BackendProtocol.h)。
- 服务端入口在 [backend/src/main.cpp](/Users/xiaokk/Projects/usv_project/backend/src/main.cpp)。
- 当前通信协议是轻量文本协议，目的是先打通 Mac 上位机和树莓派后端的小通路，后续可以替换成更正式的 HTTP/WebSocket/ROS 接口。
