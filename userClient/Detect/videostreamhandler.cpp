#include "videostreamhandler.h"
#include <QPixmap>
#include <QDebug>
#include <QMetaObject>
#include <QTimer>
int retryCount = 0;
const int maxRetries = 5;

VideoStreamHandler::VideoStreamHandler(const int idx, const QString &rtspUrl, QLabel *targetLabel, QObject *parent)
    : QObject(parent)
    , url(rtspUrl)
    , label(targetLabel)
    , pipeline(nullptr)
    , index(idx)
{}

VideoStreamHandler::~VideoStreamHandler() {
    stop();
}

void VideoStreamHandler::start() {
    qDebug() << "Starting stream for:" << url;
    // if (pipeline) {
    //     qDebug() << "Pipeline already exists.";
    //     return;
    // }

    gst_init(nullptr, nullptr);
    std::string launch = "rtspsrc location=" + url.toStdString() + " tls-validation-flags=0 latency=100 ! "
                                                                   "decodebin ! videoconvert ! video/x-raw,format=RGB ! appsink name=sink";
    qDebug() << "GStreamer launch string:" << QString::fromStdString(launch);

    GError *error = nullptr;
    pipeline = gst_parse_launch(launch.c_str(), &error);
    if (!pipeline) {
        qWarning() << "파이프라인 생성 실패:" << (error ? error->message : "Unknown error");
        if (error) g_error_free(error);
        return;
    } else {
        qDebug() << "파이프라인 생성 성공!";
    }

    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(bus, "message::error", G_CALLBACK(VideoStreamHandler::onBusMessage), this);
    gst_object_unref(bus);
    qDebug() << "Bus 연결 완료";

    GstElement *appsink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    if (!appsink) {
        qWarning() << "appsink를 찾지 못함!";
        return;
    }
    gst_app_sink_set_emit_signals(GST_APP_SINK(appsink), TRUE);
    g_signal_connect(appsink, "new-sample", G_CALLBACK(VideoStreamHandler::onNewSample), this);
    gst_object_unref(appsink);
    qDebug() << "appsink 연결 완료";

    connect(this, &VideoStreamHandler::frameReady, this, &VideoStreamHandler::onNewFrame, Qt::QueuedConnection);
    qDebug() << "frameReady 시그널 연결 완료";

    auto ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        // 명백한 실패
        retryCount++;
    }
    else if (ret == GST_STATE_CHANGE_ASYNC) {
        qDebug() << "Waiting for state change to complete...";
        QTimer::singleShot(5000, this, [this]() {
            GstState state, pending;
            qDebug() << retryCount << "th trying...\n";
            gst_element_get_state(pipeline, &state, &pending, GST_CLOCK_TIME_NONE);
            if (state != GST_STATE_PLAYING) {
                if (++retryCount >= maxRetries) {
                    qDebug() << "GStreamer Connection failed. Please try again.";
                    retryCount = 0;
                    return;
                }
                start();
            } else {
                qDebug() << "state change success\n";
                retryCount = 0; // 성공
            }
        });
        return;
    }
}

void VideoStreamHandler::stop() {
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        pipeline = nullptr;
    }
}

GstFlowReturn VideoStreamHandler::onNewSample(GstAppSink *sink, gpointer user_data) {
    auto *self = static_cast<VideoStreamHandler*>(user_data);
    GstSample *sample = gst_app_sink_pull_sample(sink);
    GstBuffer *buf = gst_sample_get_buffer(sample);
    GstMapInfo info;
    gst_buffer_map(buf, &info, GST_MAP_READ);

    GstCaps *caps = gst_sample_get_caps(sample);
    GstStructure *str = gst_caps_get_structure(caps, 0);
    int width = 0, height = 0;
    gst_structure_get_int(str, "width", &width);
    gst_structure_get_int(str, "height", &height);
    int bytesPerLine = width * 3;

    QImage img((const uchar*)info.data, width, height, bytesPerLine, QImage::Format_RGB888);
    emit self->frameReady(img.copy());

    gst_buffer_unmap(buf, &info);
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

void VideoStreamHandler::onBusMessage(GstBus *bus, GstMessage *msg, gpointer user_data) {
    GError *err = nullptr; gchar *dbg = nullptr;
    gst_message_parse_error(msg, &err, &dbg);
    qWarning() << "GStreamer Error:" << err->message;
    g_error_free(err); g_free(dbg);
}

void VideoStreamHandler::onNewFrame(const QImage &img) {
    if (label) {
        label->setPixmap(QPixmap::fromImage(img).scaled(label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}
