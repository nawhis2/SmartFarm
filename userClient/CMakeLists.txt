cmake_minimum_required(VERSION 3.14)
project(SmartFarmDashboard LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)  # <- Qt Meta-Object Compiler 자동 처리
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Qt 설정
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# OpenCV 설정
set(OpenCV_DIR "C:/opencv/build")
find_package(OpenCV REQUIRED COMPONENTS core imgproc videoio highgui)

# GStreamer (Windows, MSVC)
set(GSTREAMER_ROOT_X86_64 "C:/gstreamer/1.0/msvc_x86_64")

# 헤더 디렉터리 등록
# 3-a) include 디렉토리
include_directories(
    ${OpenCV_INCLUDE_DIRS}
    "${GSTREAMER_ROOT_X86_64}/include/gstreamer-1.0"
    "${GSTREAMER_ROOT_X86_64}/include/glib-2.0"
    "${GSTREAMER_ROOT_X86_64}/lib/glib-2.0/include"
)

# 3-b) 링크할 .lib 목록
set(GSTREAMER_LIBS
    "${GSTREAMER_ROOT_X86_64}/lib/gstreamer-1.0.lib"
    "${GSTREAMER_ROOT_X86_64}/lib/gstapp-1.0.lib"
    "${GSTREAMER_ROOT_X86_64}/lib/gstvideo-1.0.lib"
    "${GSTREAMER_ROOT_X86_64}/lib/gstbase-1.0.lib"
    "${GSTREAMER_ROOT_X86_64}/lib/gstallocators-1.0.lib"
    "${GSTREAMER_ROOT_X86_64}/lib/gstaudio-1.0.lib"
    "${GSTREAMER_ROOT_X86_64}/lib/gstpbutils-1.0.lib"
    "${GSTREAMER_ROOT_X86_64}/lib/gobject-2.0.lib"
    "${GSTREAMER_ROOT_X86_64}/lib/glib-2.0.lib"
    "${GSTREAMER_ROOT_X86_64}/lib/gthread-2.0.lib"
)

# 실행 파일 설정
add_executable(${PROJECT_NAME}
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)

# include 경로 자동 추가됨
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets

    ${OpenCV_LIBS}

    # GStreamer (imported targets)
    ${GSTREAMER_LIBS}
)
