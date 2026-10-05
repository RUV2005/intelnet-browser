cmake_minimum_required(VERSION 3.21)
project(IntelNetBrowser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ===== Qt 配置 =====
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    WebEngineWidgets
    Network
)

# ===== Rust 库配置 =====
set(RUST_CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../rust-core")
set(RUST_CORE_LIB "${RUST_CORE_DIR}/target/release")

if(WIN32)
    set(RUST_LIB_NAME "intelnet_core.dll.lib")
    set(RUST_DLL_NAME "intelnet_core.dll")
elseif(APPLE)
    set(RUST_LIB_NAME "libintelnet_core.dylib")
else()
    set(RUST_LIB_NAME "libintelnet_core.so")
endif()

# ===== 源文件 =====
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/browser_widget.cpp
    src/browser_page.cpp
    src/voice_panel.cpp
    src/accessible_toolbar.cpp
    src/rust_bridge.cpp
    src/theme.cpp
)

set(HEADERS
    src/mainwindow.h
    src/browser_widget.h
    src/browser_page.h
    src/voice_panel.h
    src/accessible_toolbar.h
    src/rust_bridge.h
    src/theme.h
)

# ===== 资源（图标 + 样式表） =====
set(RESOURCES
    resources.qrc
)

# ===== 可执行文件 =====
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

# ===== 包含目录 =====
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${RUST_CORE_DIR}/include
)

# ===== 链接库 =====
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::Network
    ${RUST_CORE_LIB}/${RUST_LIB_NAME}
)

# Windows 特定设置
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )

    # 复制 Rust DLL
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${RUST_CORE_LIB}/${RUST_DLL_NAME}"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${RUST_DLL_NAME}"
    )
endif()

# ===== 编译 Rust 库 =====
add_custom_target(rust_core ALL
    COMMAND cargo build --release
    WORKING_DIRECTORY ${RUST_CORE_DIR}
    COMMENT "Building Rust core library..."
)

add_dependencies(${PROJECT_NAME} rust_core)

# ===== Qt 部署 =====
if(WIN32)
    # Windows 平台使用 windeployqt
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS ${Qt6_DIR}/../../../bin)
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${WINDEPLOYQT_EXECUTABLE} --no-translations "$<TARGET_FILE:${PROJECT_NAME}>"
            COMMENT "Running windeployqt..."
        )
    endif()
endif()
