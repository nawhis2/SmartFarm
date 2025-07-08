#include "detectmainwidget.h"
#include "ui_detectmainwidget.h"
#include <QtConcurrent>
#include "clientUtil.h"
#include "network.h"
#include <QPixmap>
#include <QDebug>

DetectMainWidget::DetectMainWidget(QStackedWidget *stack, QWidget *parent)
    : DetectCoreWidget(stack, parent)
    , ui(new Ui::DetectMainWidget)
{
    ui->setupUi(this);
    // UI 버튼
    connect(ui->btnFire,       &QPushButton::clicked, this, &DetectMainWidget::showFirePage);
    connect(ui->btnGrowth,     &QPushButton::clicked, this, &DetectMainWidget::showGrowthPage);
    connect(ui->btnIntrusion,  &QPushButton::clicked, this, &DetectMainWidget::showIntrusionPage);
    connect(ui->btnMap,     &QPushButton::clicked, this, &DetectMainWidget::showSensorPage);
    connect(ui->startCameraButton, &QPushButton::clicked, this, &DetectMainWidget::setupVideoPlayer);
}

DetectMainWidget::~DetectMainWidget()
{
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
    }
    delete ui;
}

void DetectMainWidget::showFirePage()    { changePage(1); }
void DetectMainWidget::showGrowthPage()  { changePage(3); }
void DetectMainWidget::showIntrusionPage(){ changePage(2);
    /*sendFile("Intrusion\n", "IP");
    //sendFile("Strawberry\n", "IP");
    (void)QtConcurrent::run([this]() {
        char buffer[1024];
        int n = SSL_read(sock_fd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            QString ip = QString::fromUtf8(buffer);
            qDebug() << "[TEST] intrusion ip:" << ip;

            // UI 업데이트는 메인 스레드에서
            QMetaObject::invokeMethod(this, [this, ip]() {
                ipAddress = ip.toStdString();
                // 필요 시 라벨 갱신 등 UI 처리
            }, Qt::QueuedConnection);
        } else {
            qDebug() << "SSL_read failed or no data.";
        }
    });*/
}
void DetectMainWidget::showSensorPage()  { /*ui->stackedWidget->setCurrentWidget(ui->pageSensor)*/; }

void DetectMainWidget::setupVideoPlayer()
{
    if (pipeline) return;
    gst_init(nullptr, nullptr);
    // RTSP -> depay -> parse -> decodebin -> convert -> appsink
    const std::string launch =
        "rtspsrc location=rtsp://" + ipAddress + ":8554/test latency=0 ! "
                                                 "decodebin ! videoconvert ! video/x-raw,format=RGB ! appsink name=sink";

    GError *error = nullptr;
    pipeline = gst_parse_launch(launch.c_str(), &error);
    if (!pipeline) {
        qWarning() << "파이프라인 실패:" << error->message;
        return;
    }
    // 버스 에러 모니터링
    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(bus, "message::error",
                     G_CALLBACK(DetectMainWidget::onBusMessage), this);
    gst_object_unref(bus);

    GstElement *appsink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    gst_app_sink_set_emit_signals(GST_APP_SINK(appsink), TRUE);
    g_signal_connect(appsink, "new-sample",
                     G_CALLBACK(DetectMainWidget::onNewSample), this);
    gst_object_unref(appsink);

    connect(this, &DetectMainWidget::frameReady,
            this, &DetectMainWidget::onNewFrame,
            Qt::QueuedConnection);

    auto ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    qDebug() << "GStreamer state change return:" << ret;
}

GstFlowReturn DetectMainWidget::onNewSample(GstAppSink *sink, gpointer user_data) {
    qDebug() << "onNewSample called!";
    auto *self = static_cast<DetectMainWidget*>(user_data);
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

void DetectMainWidget::onBusMessage(GstBus *bus, GstMessage *msg, gpointer user_data) {
    GError *err = nullptr; gchar *dbg = nullptr;
    gst_message_parse_error(msg, &err, &dbg);
    qWarning() << "GStreamer Error:" << err->message;
    g_error_free(err); g_free(dbg);
}

void DetectMainWidget::onNewFrame(const QImage &img) {
    ui->videoLabel->setPixmap(
        QPixmap::fromImage(img)
            .scaled(ui->videoLabel->size(),
                    Qt::KeepAspectRatio,
                    Qt::SmoothTransformation));
}
