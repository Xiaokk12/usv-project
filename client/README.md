# Client

Qt/QML 上位机（Ground Station）源代码，当前目录包含 `USV_GS_Test` 项目。

## 项目位置
- 项目根：`client/USV_GS_Test/USV_GS_Test`
- 入口：`main.cpp` / `Main.qml`

## Build
```bash
cmake -S client/USV_GS_Test/USV_GS_Test -B build/client -G Ninja -DCMAKE_PREFIX_PATH=/Users/xiaokk/Qt/6.9.3/macos
cmake --build build/client -j
```

## Run

### 本机模式
不设置环境变量时，上位机使用本机进程内的 `BackendEngine`：

```bash
./build/client/appUSV_GS_Test.app/Contents/MacOS/appUSV_GS_Test
```

### 连接树莓派后端
设置 `USV_BACKEND_HOST` 后，上位机会切换到远端模式，通过 TCP 连接树莓派后端：

```bash
USV_BACKEND_HOST=192.168.1.82 USV_BACKEND_PORT=45454 ./build/client/appUSV_GS_Test.app/Contents/MacOS/appUSV_GS_Test
```

说明：
- `USV_BACKEND_HOST`：树莓派 IP
- `USV_BACKEND_PORT`：树莓派后端监听端口，默认 `45454`

## 联调链路
- QML 按钮操作先进入 [SamplingTaskBridge.cpp](/Users/xiaokk/Projects/usv_project/client/USV_GS_Test/USV_GS_Test/SamplingTaskBridge.cpp)
- 本机模式下直接驱动本地 `BackendEngine`
- 远端模式下通过 TCP 把命令、航点事件和采样事件发给树莓派后端
- 上位机收到树莓派回传的 `SNAPSHOT` 后更新 UI
