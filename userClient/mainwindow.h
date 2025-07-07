#pragma once
#include <QMainWindow>
#include <QImage>
#include <QTimer>
#include <QThread>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void frameReady(const QImage &img);

private slots:
    // 페이지 전환
    void showFirePage();
    void showGrowthPage();
    void showIntrusionPage();
    void showSensorPage();
    void showMainMenu();

    // 스트림 제어
    void setupVideoPlayer();
    void onNewFrame(const QImage &img);

private:
    Ui::MainWindow *ui;
    GstElement    *pipeline = nullptr;

    // appsink new-sample 콜백
    static GstFlowReturn onNewSample(GstAppSink *sink, gpointer user_data);
    // 버스 에러 메시지 콜백
    static void onBusMessage(GstBus *bus, GstMessage *msg, gpointer user_data);
};
