#ifndef DETECTMAINWIDGET_H
#define DETECTMAINWIDGET_H

#include "detectcorewidget.h"
#include <QImage>
#include <QLabel>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include "videostreamhandler.h"
#include <map>
#include <string>
#include <QNetworkAccessManager>
#include <QNetworkReply>

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
    void showLogPage();
    void on_btnSync_clicked();
    void showMailDialog();

    // 스트림 제어
    void setupAllStreams();
    void setupStream(const char* cctvName, const int index);

    void handleWeatherData();      // 날씨 JSON 응답 처리
    void handleWeatherIcon();      // 날씨 아이콘 이미지 응답 처리

private:
    // appsink new-sample 콜백
    static GstFlowReturn onNewSample(GstAppSink *sink, gpointer user_data);
    // 버스 에러 메시지 콜백
    static void onBusMessage(GstBus *bus, GstMessage *msg, gpointer user_data);

private:
    Ui::DetectMainWidget *ui;

    QLabel* videoLabels[4];
    std::map<int, std::string> cctvNames;
    std::string ipAddress[4];
    VideoStreamHandler *Streamers[4];

    QNetworkAccessManager *weatherManager;
    void fetchWeather();  // 날씨 정보 요청 함수
    void fetchWeatherIcon(const QString& iconCode);};

#endif // DETECTMAINWIDGET_H
