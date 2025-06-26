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

    return app.exec();
}
