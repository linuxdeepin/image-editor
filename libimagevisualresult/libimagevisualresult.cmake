# 定义需要的cmake版本
cmake_minimum_required(VERSION 3.10)

# Set the project name, imagevisualresult6 if Qt6, otherwise imagevisualresult
project(libimagevisualresult${IMGE_VERSION_MAJOR} VERSION 0.1.0)
set(TARGET_NAME imagevisualresult${IMGE_VERSION_MAJOR})
message("\n\n>>> Project: ${PROJECT_NAME}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)
set(CMAKE_C_FLAGS "-fPIC")
set(CMAKE_CXX_FLAGS "-fPIC")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(${PROJECT_BINARY_DIR})
include_directories(${PROJECT_SOURCE_DIR})

# 文件夹包含
set(SRCS
    src/lut.cpp
    src/lut.h
    src/utils.cpp
    src/utils.h
    src/visualresult.cpp
    src/visualresult.h
    src/main.c)
file(GLOB_RECURSE OUT_HEADERS ${CMAKE_CURRENT_LIST_DIR}/src/visualresult.h)

# -------------添加第三方库begins-----------------------#
set(INC_DIR /usr/include/)
set(LINK_DIR /usr/lib/)

include_directories(${INC_DIR})
link_directories(${LINK_DIR})

add_executable(${PROJECT_NAME} ${SRCS})

# 设置不删除生成的文件夹内容文件
set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)

# 编译为库
add_library(${TARGET_NAME} SHARED ${SRCS} ${allHeaders} ${allSource})

# 将库安装到指定位置
include(GNUInstallDirs)
set_target_properties(${TARGET_NAME} PROPERTIES VERSION 0.1.0 SOVERSION 0.1)

install(TARGETS ${TARGET_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${OUT_HEADERS} DESTINATION include/${PROJECT_NAME})

# config libiamgevisualresult${IMGE_VERSION_MAJOR}.pc.in
configure_file(libimagevisualresult.pc.in ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.pc @ONLY)
install(FILES ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

# CUBE 颜色表文件 安装到usr/share下， 方便其他应用访问
install(DIRECTORY ${PROJECT_SOURCE_DIR}/filter_cube
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}
    FILES_MATCHING PATTERN "*.CUBE")
install(DIRECTORY ${PROJECT_SOURCE_DIR}/filter_cube
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}
    FILES_MATCHING PATTERN "*.dat")

# 加速编译优化参数
if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "mips64")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wl,-O1 -Wl,--as-needed -Wl,-E -fPIE -ffunction-sections -fdata-sections -Wl,--gc-sections -ftree-vectorize -march=loongson3a -mhard-float -mno-micromips -mno-mips16 -flax-vector-conversions -mloongson-ext2 -mloongson-mmi -z noexecstack")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wl,-O1 -Wl,--as-needed -Wl,-E -fPIE -ffunction-sections -fdata-sections -Wl,--gc-sections -ftree-vectorize -march=loongson3a -mhard-float -mno-micromips -mno-mips16 -flax-vector-conversions -mloongson-ext2 -mloongson-mmi -z noexecstack")
    SET(CMAKE_EXE_LINKER_FLAGS "-pie")
else()
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wl,-O1 -Wl,--as-needed -Wl,-E -fPIE -ffunction-sections -fdata-sections -Wl,--gc-sections")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3  -Wl,-O1 -Wl,--as-needed -Wl,-E -fPIE -ffunction-sections -fdata-sections -Wl,--gc-sections")
    SET(CMAKE_EXE_LINKER_FLAGS "-pie")
endif()
