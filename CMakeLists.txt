cmake_minimum_required(VERSION 3.20)
project(Flashshot VERSION 2.4.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_PREFIX_PATH "G:/Qt/6.11.0/mingw_64")

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Multimedia)

# 如果使用 MinGW，编译 RC 文件并嵌入 manifest 和图标
if(MINGW)
    set(RC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/src/resources.rc)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/resources.o
        COMMAND windres ${RC_FILE} -o ${CMAKE_CURRENT_BINARY_DIR}/resources.o
        DEPENDS ${RC_FILE}
        COMMENT "Compiling resources"
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
    user32
    gdi32
    shell32
    ole32
)

install(TARGETS Flashshot_x64 RUNTIME DESTINATION bin)