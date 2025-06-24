cmake_minimum_required(VERSION 3.14)
project(SmartFarmDashboard LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)  # <- Qt Meta-Object Compiler 자동 처리
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Qt 설정
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Multimedia MultimediaWidgets)

# OpenCV 설정
set(OpenCV_DIR "C:/opencv/build")
find_package(OpenCV REQUIRED COMPONENTS core imgproc videoio highgui)

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
    Qt6::Multimedia
    Qt6::MultimediaWidgets

    ${OpenCV_LIBS}
    # OpenCV imported targets
    #OpenCV::opencv_core
    #OpenCV::opencv_imgproc
    #OpenCV::opencv_videoio
    #OpenCV::opencv_highgui
)
