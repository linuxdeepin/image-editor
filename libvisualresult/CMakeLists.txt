#定义需要的cmake版本
cmake_minimum_required(VERSION 3.10)
set(CMD_NAME visualresult)
project(libvisualresult)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

option(DOTEST "option for test" OFF)
add_definitions(-DENABLE_ACCESSIBILITY=1)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

find_package(PkgConfig REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(${PROJECT_BINARY_DIR})
include_directories(${PROJECT_SOURCE_DIR})

add_definitions( -DLITE_DIV )
add_definitions( -DUSE_UNIONIMAGE )
add_definitions( -DCMAKE_BUILD )

#find_package(Qt5 REQUIRED ${QtModule})

#文件夹包含
include_directories(src)

aux_source_directory(src allSource)

set(LINK_DIR lib)
link_directories(${LINK_DIR})

add_executable(${PROJECT_NAME} ${SRCS} ${allSource} )

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
link_directories(${OpenCV_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})

#编译为库
add_library(${CMD_NAME} SHARED ${SRCS} ${allHeaders} ${allSource})

include(GNUInstallDirs)

configure_file(libvisualresult.pc.in ${PROJECT_BINARY_DIR}/libvisualresult.pc @ONLY)

set_target_properties(${CMD_NAME} PROPERTIES VERSION 0.1.0 SOVERSION 0.1)
install(TARGETS ${CMD_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(FILES 
        src/lut.h 
        DESTINATION include/libvisualresult)

install(FILES ${PROJECT_BINARY_DIR}/libvisualresult.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

#set(QtModule Core Gui Widgets DBus LinguistTools)
# 加速编译优化参数
if (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "mips64")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wl,-O1 -Wl,--as-needed -Wl,-E -fPIE -ffunction-sections -fdata-sections -Wl,--gc-sections -ftree-vectorize -march=loongson3a -mhard-float -mno-micromips -mno-mips16 -flax-vector-conversions -mloongson-ext2 -mloongson-mmi -z noexecstack")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wl,-O1 -Wl,--as-needed -Wl,-E -fPIE -ffunction-sections -fdata-sections -Wl,--gc-sections -ftree-vectorize -march=loongson3a -mhard-float -mno-micromips -mno-mips16 -flax-vector-conversions -mloongson-ext2 -mloongson-mmi -z noexecstack")
    SET(CMAKE_EXE_LINKER_FLAGS  "-pie")
else()
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wl,-O1 -Wl,--as-needed -Wl,-E -fPIE -ffunction-sections -fdata-sections -Wl,--gc-sections")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3  -Wl,-O1 -Wl,--as-needed -Wl,-E -fPIE -ffunction-sections -fdata-sections -Wl,--gc-sections")
    SET(CMAKE_EXE_LINKER_FLAGS  "-pie")
endif()
