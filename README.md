# USV Project

模块化布局，便于上位机、后端、仿真和可视化协同开发。

当前已经支持两种运行方式：
- 本机单机模式：Mac 上位机内嵌 `BackendEngine`
- 联调模式：Mac 上位机通过 TCP 连接树莓派上的 `backend_app`

## 目录
- `client/` 上位机应用（Qt/QML Ground Station）
- `backend/` 后端状态机与 TCP 服务端
- `simulator/` 仿真程序占位（ROS/自研模拟，待实现）
- `visualization/` 可视化占位（3D/图表/地图，待实现）
- `common/` 公共协议/模型/配置，待整理
- `docs/` 文档与设计
- `tests/` 自动化测试

## 本机运行

### 后端
```bash
cmake -S backend -B build/backend -G Ninja
cmake --build build/backend -j
./build/backend/backend_app --demo
```

### 上位机
```bash
cmake -S client/USV_GS_Test/USV_GS_Test -B build/client -G Ninja -DCMAKE_PREFIX_PATH=/Users/xiaokk/Qt/6.9.3/macos
cmake --build build/client -j
./build/client/appUSV_GS_Test.app/Contents/MacOS/appUSV_GS_Test
```

## Mac 上位机 + 树莓派后端联调

### 1. 树莓派启动后端服务
```bash
cd ~/usv-project
cmake -S backend -B build/backend -G Ninja
cmake --build build/backend -j
./build/backend/backend_app --port 45454
```

### 2. Mac 启动上位机并连接树莓派
将 `192.168.1.82` 替换为你的树莓派 IP：

```bash
cd /Users/xiaokk/Projects/usv_project
cmake -S client/USV_GS_Test/USV_GS_Test -B build/client -G Ninja -DCMAKE_PREFIX_PATH=/Users/xiaokk/Qt/6.9.3/macos
cmake --build build/client -j
USV_BACKEND_HOST=192.168.1.82 USV_BACKEND_PORT=45454 ./build/client/appUSV_GS_Test.app/Contents/MacOS/appUSV_GS_Test
```

这样形成的链路是：
- Mac 上位机发送状态机命令和任务事件
- 树莓派后端处理状态转移
- 树莓派终端显示 `[STATE] ...`
- 上位机面板显示树莓派回传的最新状态

