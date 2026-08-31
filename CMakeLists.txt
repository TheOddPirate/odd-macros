cmake_minimum_required(VERSION 3.16)

project(odd-macros 
    VERSION 0.0.1 
    DESCRIPTION "A small daemon that registers global shortcuts from your config file and executes selected macro on trigger" 
    HOMEPAGE_URL "https://theoddpirate.com" 
    LANGUAGES CXX
)

include(GNUInstallDirs)

add_definitions(-DPROJECT_DEVELOPERS="TheOddPirate")

# For automatic firmware updates in connected node
add_definitions(-DPROJECT_NAME="${PROJECT_NAME}")
add_definitions(-DPROJECT_DESCRIPTION="${PROJECT_DESCRIPTION}")
add_definitions(-DPROJECT_VERSION="${PROJECT_VERSION}")
add_definitions(-DPROJECT_DOMAIN="${PROJECT_HOMEPAGE_URL}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(ECM 5.90.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

# Packages and dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(XKBCOMMON REQUIRED xkbcommon)
find_package(Qt6 REQUIRED COMPONENTS Core Gui)
find_package(KF6 REQUIRED COMPONENTS GlobalAccel Config)

# Define executable
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/messagehandler.cpp
    src/shortcutmanager.cpp
    src/virtualkeyboarddevice.cpp
    src/platformhelper.cpp
    src/platformhelper.h
    src/shortcutmanager.h
    src/virtualkeyboarddevice.h
    src/messagehandler.h

)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE ${XKBCOMMON_INCLUDE_DIRS})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt6::Core
        Qt6::Gui
        KF6::GlobalAccel
        KF6::ConfigCore
        ${XKBCOMMON_LIBRARIES}
)

# Configure the systemd user service file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}-daemon.service.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-daemon.service
    @ONLY
)

# -----------------------------------------------------------------------------
# Installation Rules for PKGBUILD / `make install`
# -----------------------------------------------------------------------------

# 1. Install binary to /usr/local/bin/odd-macros
install(TARGETS ${PROJECT_NAME}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# 2. Install user systemd service to /usr/lib/systemd/user/odd-macros-daemon.service
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-daemon.service
    DESTINATION lib/systemd/user
)