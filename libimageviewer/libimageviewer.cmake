#定义需要的cmake版本
cmake_minimum_required(VERSION 3.10)

#设置工程名字
project(libimageviewer${IMGE_VERSION_MAJOR} VERSION 0.1.0)
set(TARGET_NAME imageviewer${IMGE_VERSION_MAJOR})
message("\n\n>>> Project: ${PROJECT_NAME}")

#Check dtkwidget version
find_package(Dtk${DTK_VERSION_MAJOR} REQUIRED COMPONENTS Widget)

if(DEFINED DtkWidget_VERSION)
    if(${DtkWidget_VERSION} VERSION_LESS "5.6.9")
        message(STATUS "Use specific dtk watermark, Version < 5.6.9")
        add_definitions(-DUSE_SPECIFIC_DTK_WATERMARK)
    else()
        message(STATUS "Use master dtk watermark, Version >= 5.6.9")
    endif()
else()
    message(STATUS "Not defined dtk version, use master dtk watermark.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_C_FLAGS "-fPIC")
set(CMAKE_CXX_FLAGS "-fPIC")

#先查找到这些qt相关的模块以供链接使用
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Svg DBus Concurrent PrintSupport LinguistTools)

if(${QT_VERSION_MAJOR} EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS SvgWidgets)
endif()

#包含目录
include_directories(${CMAKE_INCLUDE_CURRENT_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_definitions(-DLITE_DIV)
add_definitions(-DCMAKE_BUILD)

#库目录
aux_source_directory(imageviewer allSources)

find_package(PkgConfig REQUIRED)
find_package(TIFF)
pkg_check_modules(3rd_lib REQUIRED
    dtk${DTK_VERSION_MAJOR}widget
    dtk${DTK_VERSION_MAJOR}core
    dtk${DTK_VERSION_MAJOR}gui
    gobject-2.0
    libmediainfo
)

#find dfm - io
if(${QT_VERSION_MAJOR} EQUAL 6)
    pkg_check_modules(dfm-io_lib dfm6-io)
else()
    pkg_check_modules(dfm-io_lib dfm-io)
endif()

if(${dfm-io_lib_FOUND})
    message("--- Found ${dfm-io_lib_LIBRARIES}, enable MTP file copy optimization.")
    add_definitions(-DUSE_DFM_IO)
else()
    message("--- Not found dfm-io, use base api copy MTP file.")
endif()

#需要打开的头文件
FILE(GLOB allHeaders "*.h" "*/*.h" "*/*/*.h")

#需要打开的代码文件
FILE(GLOB_RECURSE allSources "*.cpp" "*.c")
file(GLOB_RECURSE RESOURCES "*.qrc")

#-- -- -- -- -- -- -- -- -- 添加第三方库begins -- -- -- -- -- -- -- -- -- -- -- -- -- #
file(GLOB tiffToolsSources ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/tiff-tools/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/tiff-tools/*.h)

# 使用第三方库需要用到的一个包
set(INC_DIR /usr/include/)
set(LINK_DIR /usr/lib/)

include_directories(${INC_DIR})
link_directories(${LINK_DIR})

set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)

# transaltions: in qt5, translations need to be generated before the target is set,
# while Qt6 does the opposite
if(${QT_VERSION_MAJOR} EQUAL 5)
    file(GLOB TS LIST_DIRECTORIES false translations/libimageviewer*.ts)
    set_source_files_properties(${TS} PROPERTIES OUTPUT_LOCATION ${PROJECT_SOURCE_DIR}/translations)

    qt5_create_translation(QM ${allSources} ${GLOB})

    if(NOT(${CMAKE_BUILD_TYPE} MATCHES "Debug"))
        qt5_create_translation(QM ${allSources} ${TS})
    endif()
endif()

# 设置不删除生成的文件夹内容文件
set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)

# 编译为库
add_library(${TARGET_NAME} SHARED ${allHeaders} ${allSources} ${RESOURCES} ${QM} ${tiffToolsSources})

# 将库安装到指定位置
set_target_properties(${TARGET_NAME} PROPERTIES VERSION 0.1.0 SOVERSION 0.1)

target_include_directories(${TARGET_NAME} PUBLIC ${3rd_lib_INCLUDE_DIRS} ${TIFF_INCLUDE_DIRS} ${dfm-io_lib_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME}
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::DBus
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::PrintSupport
    ${3rd_lib_LIBRARIES}
    ${TIFF_LIBRARIES}
    ${dfm-io_lib_LIBRARIES}
    dl)

if(${QT_VERSION_MAJOR} EQUAL 6)
    target_link_libraries(${TARGET_NAME} Qt6::SvgWidgets)
endif()

# get standard install paths
include(GNUInstallDirs)
configure_file(libimageviewer.pc.in ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.pc @ONLY)
install(FILES ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

# lib and headers
install(TARGETS ${TARGET_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES
    imageengine.h
    imageviewer.h
    image-viewer_global.h
    movieservice.h
    quickprint.h
    DESTINATION include/${PROJECT_NAME})

# translations, set load path for translations
if(${QT_VERSION_MAJOR} EQUAL 6)
    file(COPY ${PROJECT_SOURCE_DIR}/translations/
        DESTINATION ${PROJECT_SOURCE_DIR}/translations${IMGE_VERSION_MAJOR}
        FILES_MATCHING PATTERN "*.ts")

    file(GLOB TS6 LIST_DIRECTORIES false translations${IMGE_VERSION_MAJOR}/libimageviewer*.ts)
    set_source_files_properties(${TS6} PROPERTIES OUTPUT_LOCATION ${PROJECT_SOURCE_DIR}/translations${IMGE_VERSION_MAJOR})

    qt6_add_translations(${TARGET_NAME}
        TS_FILES ${TS6}
        SOURCES ${allSources}
    )
endif()

# copy .qm translation file to same path "usr/shar/${PROJECT_NAME}/translations"
install(DIRECTORY ${PROJECT_SOURCE_DIR}/translations${IMGE_VERSION_MAJOR}/
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/translations
    FILES_MATCHING PATTERN "*.qm")
target_compile_definitions(${TARGET_NAME} PRIVATE -DPLUGIN_QM_TRANS_PATH=${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/translations)

# Accelerated compilation of optimized parameters
if(NOT(CMAKE_BUILD_TYPE STREQUAL "Debug"))
    if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "mips64")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wl,-O1 -Wl,-E -fPIE -ffunction-sections -fdata-sections -Wl,--gc-sections -ftree-vectorize -march=loongson3a -mhard-float -mno-micromips -mno-mips16 -flax-vector-conversions -mloongson-ext2 -mloongson-mmi -z noexecstack")
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wl,-O1 -Wl,-E -fPIE -ffunction-sections -fdata-sections -Wl,--gc-sections -ftree-vectorize -march=loongson3a -mhard-float -mno-micromips -mno-mips16 -flax-vector-conversions -mloongson-ext2 -mloongson-mmi -z noexecstack")
        SET(CMAKE_EXE_LINKER_FLAGS "-pie")
    else()
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wl,-O1 -Wl,-E -fPIE -ffunction-sections -fdata-sections -Wl,--gc-sections")
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3  -Wl,-O1 -Wl,-E -fPIE -ffunction-sections -fdata-sections -Wl,--gc-sections")
        SET(CMAKE_EXE_LINKER_FLAGS "-pie")
    endif()
endif()

if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "sw_64")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mieee")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mieee")
    add_definitions(-DISSW_64)
endif()
