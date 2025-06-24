// main.cpp
#include <QApplication>
#include "mainwindow.h"
#include <opencv2/opencv.hpp>
using namespace cv;

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MainWindow w;
    w.setWindowTitle("🌱 스마트팜 통합 모니터링 시스템");
    w.resize(1000, 700);
    w.show();
    // // FFmpeg 백엔드 명시
    // std::string pipeline =
    //     "rtspsrc location=rtsp://192.168.0.20:8554/test latency=100 ! "
    //     "rtph264depay ! avdec_h264 ! videoconvert ! appsink";

    // VideoCapture cap("rtsp://192.168.0.20:8554/test", cv::CAP_FFMPEG);
    // if (!cap.isOpened()) {
    //     std::cerr << "RTSP 연결 실패\n";
    //     return -1;
    // }

    // Mat frame;
    // while (true) {
    //     if (!cap.read(frame) || frame.empty()) break;
    //     imshow("RTSP", frame);
    //     if (waitKey(1) == 'q') break;
    // }
    return app.exec();
}
