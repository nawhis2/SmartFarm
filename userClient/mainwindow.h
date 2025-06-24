#pragma once
#include <QApplication>
#include <QMainWindow>
//#include <opencv2/opencv.hpp>
//#include <QTimer>
#include <QLabel>
#include <QApplication>
#include <QMediaPlayer>
#include <QVideoWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT
    //QLabel* videoLabel;

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void showFirePage();
    void showGrowthPage();
    void showIntrusionPage();
    void showSensorPage();
    void showMainMenu();
   // void startCamera();
   // void readFrame();
    //void grabFrame();


private:
    Ui::MainWindow *ui;
   // cv::VideoCapture cap;
    //QTimer      *timer;
    QImage imgBuffer;
    QMediaPlayer        *player;
    QVideoWidget        *videoWidget;
    void setupVideoPlayer();

};
