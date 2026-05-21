cmake_minimum_required(VERSION 3.20)
project(Flashshot VERSION 1.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 强制使用动态链接
set(BUILD_SHARED_LIBS ON)

# 设置 Qt 路径（请修改为您的实际路径）
set(CMAKE_PREFIX_PATH "G:/Qt/6.11.0/mingw_64")

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Multimedia Concurrent)

# MinGW 特定：编译资源文件（嵌入管理员清单和程序图标）
if(MINGW)
    set(RC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/src/resources.rc)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/resources.o
        COMMAND windres ${RC_FILE} -o ${CMAKE_CURRENT_BINARY_DIR}/resources.o
        DEPENDS ${RC_FILE}
        COMMENT "Compiling Windows resources"
    )
    set(EXTRA_OBJECTS ${CMAKE_CURRENT_BINARY_DIR}/resources.o)
endif()

add_executable(Flashshot_x64 WIN32
    src/main.cpp
    src/MainWindow.cpp
    src/HotkeyManager.cpp
    src/ReplayBuffer.cpp
    src/NotificationManager.cpp
    src/ConfigManager.cpp
    src/ScreenshotTask.cpp
    src/SetupWizard.cpp
    src/Logger.cpp
    ${EXTRA_OBJECTS}
)

target_link_libraries(Flashshot_x64
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Multimedia
    Qt6::Concurrent
    user32
    gdi32
    shell32
    ole32
)

install(TARGETS Flashshot_x64 RUNTIME DESTINATION bin)