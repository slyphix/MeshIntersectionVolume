cmake_minimum_required(VERSION 3.10)
project(visualize)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_AUTOMOC ON)

find_package(Qt5Core)
find_package(Qt5Widgets)
find_package(Qt5Svg)

qt5_add_resources(UI_RESOURCES shaders/shaders.qrc)

set(SOURCE_FILES visualize.cpp visualize.h include/visualize.h)

add_library(visualize ${SOURCE_FILES} ${UI_RESOURCES})

target_link_libraries(visualize Qt5::Core Qt5::Widgets Qt5::Svg)

target_include_directories(visualize PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
        $<INSTALL_INTERFACE:/>
        )
