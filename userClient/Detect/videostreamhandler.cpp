#include "videostreamhandler.h"
#include <QPixmap>
#include <QDebug>
#include <QMetaObject>

VideoStreamHandler::VideoStreamHandler(const QString &rtspUrl, QLabel *targetLabel, QObject *parent)
    : QObject(parent), url(rtspUrl), label(targetLabel)
{}

VideoStreamHandler::~VideoStreamHandler() {
    stop();
}

void VideoStreamHandler::start() {
    qDebug() << "Starting stream for:" << url;
    if (pipeline) {
        qDebug() << "⚠️ Pipeline already exists.";
        return;
    }

    gst_init(nullptr, nullptr);
    std::string launch = "rtspsrc location=" + url.toStdString() + " latency=0 ! "
                                                                   "decodebin ! videoconvert ! video/x-raw,format=RGB ! appsink name=sink";
    qDebug() << "📦 GStreamer launch string:" << QString::fromStdString(launch);

    GError *error = nullptr;
    pipeline = gst_parse_launch(launch.c_str(), &error);
    if (!pipeline) {
        qWarning() << "❌ 파이프라인 생성 실패:" << (error ? error->message : "Unknown error");
        if (error) g_error_free(error);
        return;
    } else {
        qDebug() << "✅ 파이프라인 생성 성공!";
    }

    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(bus, "message::error", G_CALLBACK(VideoStreamHandler::onBusMessage), this);
    gst_object_unref(bus);
    qDebug() << "📡 Bus 연결 완료";

    GstElement *appsink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    if (!appsink) {
        qWarning() << "❌ appsink를 찾지 못함!";
        return;
    }
    gst_app_sink_set_emit_signals(GST_APP_SINK(appsink), TRUE);
    g_signal_connect(appsink, "new-sample", G_CALLBACK(VideoStreamHandler::onNewSample), this);
    gst_object_unref(appsink);
    qDebug() << "🎯 appsink 연결 완료";

    connect(this, &VideoStreamHandler::frameReady, this, &VideoStreamHandler::onNewFrame, Qt::QueuedConnection);
    qDebug() << "🔗 frameReady 시그널 연결 완료";

    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    qDebug() << "▶️ GStreamer state change result:" << ret;
}

void VideoStreamHandler::stop() {
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        pipeline = nullptr;
    }
}

GstFlowReturn VideoStreamHandler::onNewSample(GstAppSink *sink, gpointer user_data) {
    qDebug() << "📥 onNewSample called!";
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
    qDebug() << "🖼 onNewFrame called!";
    if (label) {
        label->setPixmap(QPixmap::fromImage(img).scaled(label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}
