cmake_minimum_required(VERSION 3.0.0)

project(imageeditor)

execute_process(COMMAND uname -m OUTPUT_VARIABLE MACH
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

set(CMAKE_C_FLAGS "-fPIC")
set(CMAKE_CXX_FLAGS "-fPIC")
set(CMAKE_EXPORT_COMPILE_COMMANDS on)
# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# check Qt version, support Qt5 default
find_package(Qt6 QUIET)
if (Qt6_FOUND)
    set(SUPPORT_QT6 TRUE)
    set(QT_VERSION_MAJOR 6)
    set(DTK_VERSION_MAJOR 6)
    # libimageeditor major version
    set(IMGE_VERSION_MAJOR 6)
else()
    set(QT_VERSION_MAJOR 5)
endif()

find_package(PkgConfig REQUIRED)

include_directories(${PROJECT_BINARY_DIR})
include_directories(${PROJECT_SOURCE_DIR})

add_subdirectory(libimageviewer)
add_subdirectory(libimagevisualresult)

option(DOTEST "option for test" OFF)
if(DOTEST OR CMAKE_BUILD_TYPE STREQUAL "Debug")
    # add_subdirectory(tests)
endif()
