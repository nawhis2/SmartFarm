cmake_minimum_required(VERSION 3.14)
project(SmartFarmDashboard LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)  # <- Qt Meta-Object Compibler 자동 처리
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
add_definitions(-D__restrict= -DOSSL_CRYPTO_ALLOC=)


# Qt 설정
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Concurrent)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(OpenSSL REQUIRED)

include_directories(
    "${GSTREAMER_ROOT}/include/gstreamer-1.0"
    "${GSTREAMER_ROOT}/lib/gstreamer-1.0/include"
    "${GSTREAMER_ROOT}/include/glib-2.0"
    "${GSTREAMER_ROOT}/lib/glib-2.0/include"
)

link_directories("${GSTREAMER_ROOT}/lib")

# GStreamer (Windows, MSVC)
set(GSTREAMER_ROOT_X86_64 "C:/gstreamer/1.0/msvc_x86_64")

# 헤더 디렉터리 등록
# 3-a) include 디렉토리
include_directories(
    "."
    "./Detect"
    "./Network"
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

set(OPENSSL_CONF "D:/openssl/OpenSSL-Win64")
include_directories("${OPENSSL_CONF}/include")

set(OPENSSL_LIBS
    "${OPENSSL_CONF}/lib/VC/x64/MD/libssl.lib"
    "${OPENSSL_CONF}/lib/VC/x64/MD/libcrypto.lib"
)

# 실행 파일 설정
add_executable(${PROJECT_NAME}
    main.cpp
    main.h
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui

    # Network func
    Network/network.cpp
    Network/network.h
    Network/handshake.cpp
    Network/handshake.h
    Network/clientUtil.cpp
    Network/clientUtil.h

    # Detect Widget
    Detect/intrusionwidget.h
    Detect/intrusionwidget.cpp
    Detect/firedetectwidget.h
    Detect/firedetectwidget.cpp
    Detect/strawberrywidget.h
    Detect/strawberrywidget.cpp
    Detect/detectcorewidget.h
    Detect/detectcorewidget.cpp
    Detect/detectmainwidget.h
    Detect/detectmainwidget.cpp
    Detect/detectmainwidget.ui
    Detect/intrusionwidget.ui
    Detect/firedetectwidget.ui
    Detect/strawberrywidget.ui
    Detect/videostreamhandler.h
    Detect/videostreamhandler.cpp
    imagepushbutton.h
    imagepushbutton.cpp
    imagedialog.h imagedialog.cpp imagedialog.ui
    streamthread.h
)


target_link_libraries(SmartFarmDashboard PRIVATE Qt6::Widgets)
# include 경로 자동 추가됨
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
    # OpenSSL
    ${OPENSSL_LIBS}

    # GStreamer (imported targets)
    ${GSTREAMER_LIBS}
    ${GST_APP_LIBRARIES}
)
