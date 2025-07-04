#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <QPixmap>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // UI 버튼
    connect(ui->btnFire,       &QPushButton::clicked, this, &MainWindow::showFirePage);
    connect(ui->btnGrowth,     &QPushButton::clicked, this, &MainWindow::showGrowthPage);
    connect(ui->btnIntrusion,  &QPushButton::clicked, this, &MainWindow::showIntrusionPage);
    connect(ui->btnSensor,     &QPushButton::clicked, this, &MainWindow::showSensorPage);
    connect(ui->btnBackFromFire,      &QPushButton::clicked, this, &MainWindow::showMainMenu);
    connect(ui->btnBackFromGrowth,    &QPushButton::clicked, this, &MainWindow::showMainMenu);
    connect(ui->btnBackFromIntrusion, &QPushButton::clicked, this, &MainWindow::showMainMenu);
    connect(ui->btnBackFromSensor,    &QPushButton::clicked, this, &MainWindow::showMainMenu);
    connect(ui->startCameraButton, &QPushButton::clicked,
            this, &MainWindow::setupVideoPlayer);
}

MainWindow::~MainWindow() {
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
    }
    delete ui;
}

void MainWindow::showFirePage()    { ui->stackedWidget->setCurrentWidget(ui->pageFire); }
void MainWindow::showGrowthPage()  { ui->stackedWidget->setCurrentWidget(ui->pageGrowth); }
void MainWindow::showIntrusionPage(){ ui->stackedWidget->setCurrentWidget(ui->pageIntrusion); }
void MainWindow::showSensorPage()  { ui->stackedWidget->setCurrentWidget(ui->pageSensor); }
void MainWindow::showMainMenu()    { ui->stackedWidget->setCurrentWidget(ui->pageMainMenu); }

void MainWindow::setupVideoPlayer()
{
    if (pipeline) return;
    gst_init(nullptr, nullptr);
    // RTSP -> depay -> parse -> decodebin -> convert -> appsink
    const char *launch =
        "rtspsrc location=rtsp://192.168.0.41:8554/test latency=100 ! "
        "rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! "
        "video/x-raw,format=RGB ! appsink name=sink";

    GError *error = nullptr;
    pipeline = gst_parse_launch(launch, &error);
    if (!pipeline) {
        qWarning() << "파이프라인 실패:" << error->message;
        return;
    }
    // 버스 에러 모니터링
    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(bus, "message::error",
                     G_CALLBACK(MainWindow::onBusMessage), this);
    gst_object_unref(bus);

    GstElement *appsink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    gst_app_sink_set_emit_signals(GST_APP_SINK(appsink), TRUE);
    g_signal_connect(appsink, "new-sample",
                     G_CALLBACK(MainWindow::onNewSample), this);
    gst_object_unref(appsink);

    connect(this, &MainWindow::frameReady,
            this, &MainWindow::onNewFrame,
            Qt::QueuedConnection);

    auto ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    qDebug() << "GStreamer state change return:" << ret;
}

GstFlowReturn MainWindow::onNewSample(GstAppSink *sink, gpointer user_data) {
    qDebug() << "onNewSample called!";
    auto *self = static_cast<MainWindow*>(user_data);
    GstSample *sample = gst_app_sink_pull_sample(sink);
    GstBuffer *buf = gst_sample_get_buffer(sample);
    GstMapInfo info;
    gst_buffer_map(buf, &info, GST_MAP_READ);

    // ── 여기서 caps를 읽어서 실제 width/height/stride 계산 ──
    GstCaps     *caps = gst_sample_get_caps(sample);
    GstStructure *str = gst_caps_get_structure(caps, 0);
    int width = 0, height = 0;
    gst_structure_get_int(str, "width", &width);
    gst_structure_get_int(str, "height", &height);

    // RGB888 포맷이므로 stride = width * 3
    int bytesPerLine = width * 3;

    // QImage 생성
    QImage img((const uchar*)info.data,
               width, height,
               bytesPerLine,
               QImage::Format_RGB888);

    // 깊은 복사 후 시그널
    emit self->frameReady(img.copy());

    gst_buffer_unmap(buf, &info);
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

void MainWindow::onBusMessage(GstBus *bus, GstMessage *msg, gpointer user_data) {
    GError *err = nullptr; gchar *dbg = nullptr;
    gst_message_parse_error(msg, &err, &dbg);
    qWarning() << "GStreamer Error:" << err->message;
    g_error_free(err); g_free(dbg);
}

void MainWindow::onNewFrame(const QImage &img) {
    ui->videoLabel->setPixmap(
        QPixmap::fromImage(img)
            .scaled(ui->videoLabel->size(),
                    Qt::KeepAspectRatio,
                    Qt::SmoothTransformation));
}
