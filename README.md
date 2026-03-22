# USV Project

模块化布局，便于上位机、后端、仿真和可视化协同开发。

## 目录
- client/          上位机应用（Qt/QML Ground Station）
- backend/         后端服务占位（REST/任务管理等，待实现）
- simulator/       仿真程序占位（ROS/自研模拟，待实现）
- visualization/   可视化占位（3D/图表/地图，待实现）
- common/          公共协议/模型/配置，待整理
- docs/            文档与设计
- tests/           自动化测试

## 快速指令
使用 `manage.py` 调用：
- `python manage.py run-client` 启动上位机（调用 client/USV_GS_Test 的构建/运行脚本）
- `python manage.py run-backend` / `run-simulator` / `run-visualization` 预留


#2026.2.1
- 目前仅能运行上位机


