#include "videostreamhandler.h"
#include <QDebug>
#include <QMetaObject>
#include <QApplication>
#include <gst/app/gstappsink.h>

VideoStreamHandler::VideoStreamHandler(int idx, const QString &rtspUrl, QObject *parent)
    : QObject(parent), index(idx), url(rtspUrl), pipeline(nullptr),
    loop(nullptr), isFirstFrame(false), streamThread(nullptr)
{
    retryTimer = new QTimer(this);
    retryTimer->setInterval(1000);
    connect(retryTimer, &QTimer::timeout, this, &VideoStreamHandler::tryStart);

    watchdogTimer = new QTimer(this);
    watchdogTimer->setInterval(5000);
    connect(watchdogTimer, &QTimer::timeout, this, &VideoStreamHandler::onStall);

    connect(qApp, &QCoreApplication::aboutToQuit,
            this, &VideoStreamHandler::onApplicationQuit);

    retryTimer->start();
}

VideoStreamHandler::~VideoStreamHandler() {
    retryTimer->stop();
    watchdogTimer->stop();
    stop();

    if (streamThread && streamThread->isRunning()) {
        debugIdx(qDebug, index) << "[~Handler] streamThread 종료 대기";
        streamThread->requestInterruption();
        streamThread->quit();
        streamThread->wait();
    }
    delete streamThread;

    if (loop) {
        if (g_main_loop_is_running(loop))
            g_main_loop_quit(loop);
        g_main_loop_unref(loop);
    }
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
    }
}

void VideoStreamHandler::initialize() {
    if (loop) {
        if (g_main_loop_is_running(loop))
            g_main_loop_quit(loop);
        g_main_loop_unref(loop);
        loop = nullptr;
    }

    std::string launch = "rtspsrc location=" + url.toStdString() + " tls-validation-flags=0 latency=100 protocols=udp retry=3 timeout=5000000 "
                                                                   "! queue max-size-buffers=10 leaky=downstream ! rtph264depay "
                                                                   "! queue max-size-buffers=10 leaky=downstream ! h264parse config-interval=1 "
                                                                   "! queue max-size-buffers=10 leaky=downstream ! d3d11h264dec "
                                                                   "! videoconvert "
                                                                   "! video/x-raw,format=RGB "
                                                                   "! appsink name=sink ";
    GError *error = nullptr;
    pipeline = gst_parse_launch(launch.c_str(), &error);
    if (!pipeline) {
        debugIdx(qWarning, index) << "[Init] 파이프라인 생성 실패:" << (error ? error->message : "Unknown");
        if (error) g_error_free(error);
        return;
    }

    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(bus, "message::error",
                     G_CALLBACK(VideoStreamHandler::onBusMessage), this);
    gst_object_unref(bus);

    GstElement *appsink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    if (appsink) {
        gst_app_sink_set_emit_signals(GST_APP_SINK(appsink), TRUE);
        g_signal_connect(appsink, "new-sample",
                         G_CALLBACK(VideoStreamHandler::onNewSample), this);
        gst_object_unref(appsink);
    } else {
        debugIdx(qWarning, index) << "[Init] appsink 없음";
    }

    loop = g_main_loop_new(nullptr, FALSE);
}

void VideoStreamHandler::tryStart() {
    if (!pipeline) initialize();
    if (!pipeline) return;

    retryTimer->stop();

    if (streamThread && streamThread->isRunning()) {
        debugIdx(qDebug, index) << "[Try] 이전 스트림 스레드 동작 중, 중지 요청";
        streamThread->requestInterruption();
        streamThread->quit();
        streamThread->wait();
        delete streamThread;
        streamThread = nullptr;
    }

    streamThread = new StreamThread(pipeline, loop);
    connect(streamThread, &QThread::finished,
            this, &VideoStreamHandler::onStreamThreadFinished);
    streamThread->start();
}

void VideoStreamHandler::stop() {
    QMetaObject::invokeMethod(watchdogTimer, &QTimer::stop, Qt::QueuedConnection);

    if (loop && g_main_loop_is_running(loop)) {
        debugIdx(qDebug, index) << "[Stop] main loop quit";
        g_main_loop_quit(loop);
    }
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
    }
}

void VideoStreamHandler::onStreamThreadFinished() {
    debugIdx(qDebug, index) << "[StreamThread] 종료 감지, 리소스 정리 및 재연결";

    if (pipeline) {
        gst_object_unref(pipeline);
        pipeline = nullptr;
    }
    if (loop) {
        g_main_loop_unref(loop);
        loop = nullptr;
    }
    isFirstFrame = false;

    QTimer::singleShot(100, this, [this]() {
        initialize();
        tryStart();
    });
}

void VideoStreamHandler::onApplicationQuit() {
    debugIdx(qDebug, index) << "[AppQuit] 애플리케이션 종료, 스트림 정리 시작";
    stop();
    if (streamThread && streamThread->isRunning()) {
        debugIdx(qDebug, index) << "[AppQuit] streamThread 종료 대기";
        streamThread->requestInterruption();
        streamThread->quit();
        streamThread->wait();
    }
    debugIdx(qDebug, index) << "[AppQuit] 스트림 리소스 정리 완료";
}

GstFlowReturn VideoStreamHandler::onNewSample(GstAppSink *sink, gpointer user_data) {
    auto *self = static_cast<VideoStreamHandler*>(user_data);
    GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
    GstBuffer *buf = gst_sample_get_buffer(sample);
    GstMapInfo info;
    gst_buffer_map(buf, &info, GST_MAP_READ);

    GstCaps *caps = gst_sample_get_caps(sample);
    GstStructure *s = gst_caps_get_structure(caps, 0);
    int width, height;
    gst_structure_get_int(s, "width", &width);
    gst_structure_get_int(s, "height", &height);

    QImage img(reinterpret_cast<const uchar*>(info.data),
               width, height, QImage::Format_RGB888);
    QImage frame = img.copy();

    QMetaObject::invokeMethod(self, [self, frame]() {
        emit self->frameReady(self->index, frame);
        if (!self->isFirstFrame) self->isFirstFrame = true;
        self->watchdogTimer->start();
    }, Qt::QueuedConnection);

    gst_buffer_unmap(buf, &info);
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

void VideoStreamHandler::onBusMessage(GstBus *, GstMessage *msg, gpointer user_data) {
    auto *self = static_cast<VideoStreamHandler*>(user_data);
    if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
        GError *err; gchar *dbg;
        gst_message_parse_error(msg, &err, &dbg);
        debugIdx(qWarning, self->index) << "[Bus] Error:" << err->message;
        g_error_free(err); g_free(dbg);
        QMetaObject::invokeMethod(self, "restart", Qt::QueuedConnection);
    }
}

void VideoStreamHandler::onStall() {
    if (!isFirstFrame) return;
    debugIdx(qWarning, index) << "[Stall] 수신 지연, 재시작";
    QMetaObject::invokeMethod(this, "restart", Qt::QueuedConnection);
}

void VideoStreamHandler::restart() {
    debugIdx(qDebug, index) << "[Restart] 재연결 트리거";
    stop();
    // onStreamThreadFinished() 가 처리합니다
}
