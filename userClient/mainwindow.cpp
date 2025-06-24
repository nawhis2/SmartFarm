#include "mainwindow.h"
#include "ui_mainwindow.h"  // 이 파일은 자동 생성됨
#include <QTimer>
#include <opencv2/opencv.hpp>
#include <QDebug>

cv::VideoCapture cap;
QTimer *timer;
//git?? test~~

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    //timer(new QTimer(this)),
    player(new QMediaPlayer(this)),
    videoWidget(new QVideoWidget(this))
{
    ui->setupUi(this);
    // player = new QMediaPlayer;
    // QVideoWidget* view  = new QVideoWidget;
    // player->setVideoOutput(view);

    connect(ui->btnFire, &QPushButton::clicked, this, &MainWindow::showFirePage);
    connect(ui->btnGrowth, &QPushButton::clicked, this, &MainWindow::showGrowthPage);
    connect(ui->btnIntrusion, &QPushButton::clicked, this, &MainWindow::showIntrusionPage);
    connect(ui->btnSensor, &QPushButton::clicked, this, &MainWindow::showSensorPage);
    //connect(ui->startCameraButton, &QPushButton::clicked, this, &MainWindow::startCamera);
    connect(ui->startCameraButton, &QPushButton::clicked, this, &MainWindow::setupVideoPlayer);

    // 뒤로가기 버튼들
    connect(ui->btnBackFromFire, &QPushButton::clicked, this, &MainWindow::showMainMenu);
    connect(ui->btnBackFromGrowth, &QPushButton::clicked, this, &MainWindow::showMainMenu);
    connect(ui->btnBackFromIntrusion, &QPushButton::clicked, this, &MainWindow::showMainMenu);
    connect(ui->btnBackFromSensor, &QPushButton::clicked, this, &MainWindow::showMainMenu);
   // connect(timer, &QTimer::timeout, this, &MainWindow::grabFrame);
    ui->videoWidget->setFixedSize(640, 480);
    imgBuffer = QImage(640, 480, QImage::Format_RGB888);

    // 타이머 준비
   // timer = new QTimer(this);
    //connect(timer, &QTimer::timeout, this, &MainWindow::grabFrame);
    videoWidget->setMinimumSize(640, 480);
    ui->videoLayout->addWidget(videoWidget);

    // 2) MediaPlayer 셋업
    player->setVideoOutput(videoWidget);

    // 3) 버튼 시그널 연결 (예: Start 버튼)
    connect(ui->startCameraButton, &QPushButton::clicked, this, &MainWindow::setupVideoPlayer);
}

MainWindow::~MainWindow() {
    if (cap.isOpened()) cap.release();
    if (timer) timer->stop();
    delete ui;
}

// void MainWindow::readFrame() {
//     cv::Mat frame;
//     cap.read(frame);
//     if (frame.empty()) return;

//     cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
//     QImage qimg(frame.data, frame.cols, frame.rows, frame.step, QImage::Format_RGB888);
//     //ui->videoWidget->setPixmap(QPixmap::fromImage(qimg).scaled(ui->videoLabel->size(), Qt::KeepAspectRatio));
// }

// void MainWindow::startCamera() {
//     timer->stop();
//     if (cap.isOpened()) cap.release();

//     std::string url = "rtsp://192.168.0.20:8554/test";
//     if (!cap.open(url, cv::CAP_FFMPEG)) {
//         qWarning() << "RTSP 연결 실패:" << QString::fromStdString(url);
//         return;
//     }

//     // 해상도 설정
//     cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
//     cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

//     // 타이머 재시작 (약 30fps)
//     timer->start(33);
// }

// void MainWindow::grabFrame() {
//     cv::Mat frame;
//     if (!cap.read(frame) || frame.empty()) return;

//     // BGR → RGB 변환
//     cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);

//     // QImage 생성
//     QImage img(frame.data,
//                frame.cols,
//                frame.rows,
//                static_cast<int>(frame.step),
//                QImage::Format_RGB888);


//     // QLabel에 렌더링
//     ui->videoLabel->setPixmap(
//         QPixmap::fromImage(img)
//             .scaled(ui->videoLabel->size(),
//                     Qt::KeepAspectRatio,
//                     Qt::SmoothTransformation));
// }
void MainWindow::setupVideoPlayer()
{
    // 이전 스트림이 있으면 정리
    player->stop();

    // RTSP URL 설정 (필요하면 인증 정보 포함)
    QUrl url = QUrl::fromUserInput("rtsp://192.168.0.20:8554/test");
    player->setSource(url);

    // 재생 시작
    player->play();
}
void MainWindow::showFirePage() {
    ui->stackedWidget->setCurrentWidget(ui->pageFire);
}
void MainWindow::showGrowthPage() {
    ui->stackedWidget->setCurrentWidget(ui->pageGrowth);
}
void MainWindow::showIntrusionPage() {
    ui->stackedWidget->setCurrentWidget(ui->pageIntrusion);
}
void MainWindow::showSensorPage() {
    ui->stackedWidget->setCurrentWidget(ui->pageSensor);
}
void MainWindow::showMainMenu() {
    ui->stackedWidget->setCurrentWidget(ui->pageMainMenu);
     //startCamera();
}
