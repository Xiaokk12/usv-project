cmake_minimum_required(VERSION 3.16)
project(USV_GS_Test VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick WebEngineQuick WebChannel Positioning Network)

qt_standard_project_setup(REQUIRES 6.5)

# ===============================
# 1️⃣ 可执行文件
# ===============================
qt_add_executable(appUSV_GS_Test
    main.cpp
    RouteSaver.cpp
    RouteSaver.h
    SamplingTaskBridge.cpp
    SamplingTaskBridge.h
    ../../../backend/src/BackendEngine.cpp
    BoatLocationProvider.cpp
    BoatLocationProvider.h
    BoatSimulator.cpp
    BoatSimulator.h
    EnvironmentDataProvider.cpp
    EnvironmentDataProvider.h
)

# ===============================
# 2️⃣ QML Module（只包含 Main.qml）
# ===============================
qt_add_qml_module(appUSV_GS_Test
    URI USV_GS_Test
    VERSION 1.0
    QML_FILES
        Main.qml
        ManualControl.qml
        SamplingControl.qml
        RouteAutoPlanner.js
)

# ===============================
# 3️⃣ 资源文件（HTML、PNG）
# 使用 qrc（最干净最安全）
# ===============================
qt_add_resources(appUSV_GS_Test "app_resources"
    PREFIX "/"
    FILES
        map_gaode.html
        ship.png
)

# ===============================
# 4️⃣ 链接 Qt
# ===============================
target_link_libraries(appUSV_GS_Test
    PRIVATE Qt6::Quick
            Qt6::WebEngineQuick
            Qt6::WebChannel
            Qt6::Positioning
            Qt6::Network
)

target_include_directories(appUSV_GS_Test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../backend/src
)

# ===============================
# 5️⃣ macOS 打包参数
# ===============================
set_target_properties(appUSV_GS_Test PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
)

include(GNUInstallDirs)
install(TARGETS appUSV_GS_Test
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
