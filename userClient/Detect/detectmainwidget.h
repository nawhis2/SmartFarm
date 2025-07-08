#ifndef DETECTMAINWIDGET_H
#define DETECTMAINWIDGET_H

#include "detectcorewidget.h"
#include <QImage>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>

namespace Ui {
class DetectMainWidget;
}

class DetectMainWidget : public DetectCoreWidget
{
    Q_OBJECT

public:
    explicit DetectMainWidget(QStackedWidget *stack,QWidget *parent = nullptr);
    ~DetectMainWidget();

signals:
    void frameReady(const QImage &img);

private slots:
    // 페이지 전환
    void showFirePage();
    void showGrowthPage();
    void showIntrusionPage();
    void showSensorPage();

    // 스트림 제어
    void setupVideoPlayer();
    void onNewFrame(const QImage &img);

private:
    Ui::DetectMainWidget *ui;
    GstElement    *pipeline = nullptr;
    std::string ipAddress;
    // appsink new-sample 콜백
    static GstFlowReturn onNewSample(GstAppSink *sink, gpointer user_data);
    // 버스 에러 메시지 콜백
    static void onBusMessage(GstBus *bus, GstMessage *msg, gpointer user_data);
};

#endif // DETECTMAINWIDGET_H
