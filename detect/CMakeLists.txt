cmake_minimum_required(VERSION 3.10)
project(YOLO_RTSP_SERVER)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0>=1.14)
pkg_check_modules(GST_APP REQUIRED gstreamer-app-1.0>=1.14)
pkg_check_modules(GST_RTSP_SERVER REQUIRED gstreamer-rtsp-server-1.0>=1.14)

include_directories(${OpenCV_INCLUDE_DIRS} ${GST_INCLUDE_DIRS} ${GST_APP_INCLUDE_DIRS} ${GST_RTSP_SERVER_INCLUDE_DIRS})
link_directories(${GST_LIBRARY_DIRS} ${GST_APP_LIBRARY_DIRS} ${GST_RTSP_SERVER_LIBRARY_DIRS})

add_executable(demo main.cpp DetectionUtils.cpp)
target_link_libraries(demo
    ${OpenCV_LIBS}
    ${GST_LIBRARIES}
    ${GST_APP_LIBRARIES}
    ${GST_RTSP_SERVER_LIBRARIES}
)
