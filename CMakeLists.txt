cmake_minimum_required(VERSION 3.20)
project(Flashshot VERSION 2.4.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 设置 Qt 路径（根据你的实际安装调整）
set(CMAKE_PREFIX_PATH "G:/Qt/6.11.0/mingw_64")

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Multimedia)

add_executable(Flashshot WIN32
    src/main.cpp
    src/MainWindow.cpp
    src/HotkeyManager.cpp
    src/ReplayBuffer.cpp
    src/NotificationManager.cpp
    src/ConfigManager.cpp
    src/ScreenshotTask.cpp
    src/SetupWizard.cpp
)

target_link_libraries(Flashshot
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Multimedia
)

# Windows 特定链接库
target_link_libraries(Flashshot
    user32
    gdi32
    shell32
    ole32
)

# 可选：安装规则
install(TARGETS Flashshot RUNTIME DESTINATION bin)