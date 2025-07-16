#include "videostreamhandler.h"
#include <QDebug>
#include <QtConcurrent>
#include <QImage>
#include <QMetaObject>
#include <QApplication>

VideoStreamHandler::VideoStreamHandler(int idx, const QString &rtspUrl, QObject *parent)
    : QObject(parent)
    , url(rtspUrl)
    , pipeline(nullptr)
    , loop(nullptr)
    , index(idx)
    , isFirstFrame(false)
    , retryTimer(new QTimer(this))
    , watchdogTimer(new QTimer(this))
    , streamWatcher(new QFutureWatcher<void>(this))
{
    // retry
    retryTimer->setInterval(1000);
    connect(retryTimer, &QTimer::timeout, this, &VideoStreamHandler::tryStart);

    // stall
    watchdogTimer->setInterval(5000);
    connect(watchdogTimer, &QTimer::timeout, this, &VideoStreamHandler::onStall);

    // thread watcher
    connect(streamWatcher, &QFutureWatcher<void>::finished,
            this, &VideoStreamHandler::onStreamThreadFinished);

    connect(qApp, &QCoreApplication::aboutToQuit,
            this, &VideoStreamHandler::onApplicationQuit);

    retryTimer->start();
}

VideoStreamHandler::~VideoStreamHandler() {
    retryTimer->stop();
    watchdogTimer->stop();

    // 스트림 정지
    stop();

    // 쓰레드가 끝날 때까지 대기
    if (streamWatcher->isRunning()) {
        qDebug() << "[~Handler] streamWatcher 대기 중...";
        streamWatcher->waitForFinished();
    }

    delete streamWatcher;

    // 루프, 파이프라인 정리
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
    // 이전 루프 정리 (stop()로 이미 quit/_unref 됐을 수도 있음)
    if (loop) {
        if (g_main_loop_is_running(loop))
            g_main_loop_quit(loop);
        g_main_loop_unref(loop);
        loop = nullptr;
    }

    // 파이프라인 생성
    std::string launch = "rtspsrc location=" + url.toStdString() +
                         " tls-validation-flags=0 latency=100 ! decodebin ! "
                         "videoconvert ! video/x-raw,format=RGB ! appsink name=sink";
    GError *error = nullptr;
    pipeline = gst_parse_launch(launch.c_str(), &error);
    if (!pipeline) {
        qWarning() << "[Init] 파이프라인 생성 실패:" << (error ? error->message : "Unknown");
        if (error) g_error_free(error);
        return;
    }

    // 버스 에러
    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(bus, "message::error",
                     G_CALLBACK(VideoStreamHandler::onBusMessage), this);
    gst_object_unref(bus);

    // appsink
    GstElement *appsink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    if (appsink) {
        gst_app_sink_set_emit_signals(GST_APP_SINK(appsink), TRUE);
        g_signal_connect(appsink, "new-sample",
                         G_CALLBACK(VideoStreamHandler::onNewSample), this);
        gst_object_unref(appsink);
    } else {
        qWarning() << "[Init] appsink 없음";
    }

    // 새 메인루프
    loop = g_main_loop_new(nullptr, FALSE);
}

void VideoStreamHandler::tryStart() {
    if (!pipeline) initialize();
    if (!pipeline) return;

    retryTimer->stop();
    // 기존에 돌던 쓰레드가 있으면 취소
    if (streamWatcher->isRunning()) {
        qDebug() << "[Try] 이전 스트림 스레드 동작 중, stop() 먼저 호출";
        stop();
    }

    // PLAYING + 루프
    streamFuture = QtConcurrent::run([this]() {
        qDebug() << "[StreamThread] PLAYING 시작";
        gst_element_set_state(pipeline, GST_STATE_PLAYING);
        if (loop && !g_main_loop_is_running(loop))
            g_main_loop_run(loop);
        qDebug() << "[StreamThread] 루프 종료";
    });
    streamWatcher->setFuture(streamFuture);
}

void VideoStreamHandler::stop() {
    // 타이머
    QMetaObject::invokeMethod(this, [this]() {
        watchdogTimer->stop();
    }, Qt::QueuedConnection);

    // main loop quit
    if (loop && g_main_loop_is_running(loop)) {
        qDebug() << "[Stop] main loop quit";
        g_main_loop_quit(loop);
    }
    // 파이프라인 PLAYING→NULL 전환 (비동기)
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
    }
}

void VideoStreamHandler::onStreamThreadFinished() {
    qDebug() << "[StreamWatcher] 스레드 종료 감지, 리소스 정리 및 재연결";
    // 1) 파이프라인 unref
    if (pipeline) {
        gst_object_unref(pipeline);
        pipeline = nullptr;
    }
    // 2) 루프 unref
    if (loop) {
        g_main_loop_unref(loop);
        loop = nullptr;
    }
    isFirstFrame = false;

    // 3) 잠시 후 다시 시작
    QTimer::singleShot(100, this, [this]() {
        initialize();
        tryStart();
    });
}

void VideoStreamHandler::onApplicationQuit()
{
    qDebug() << "[AppQuit] 애플리케이션 종료, 스트림 정리 시작";
    // 1) 스트림 중지
    stop();

    // 2) 스트림 쓰레드가 돌아가고 있으면 대기
    if (streamWatcher->isRunning()) {
        qDebug() << "[AppQuit] 쓰레드 종료 대기";
        streamWatcher->waitForFinished();
    }
    qDebug() << "[AppQuit] 스트림 리소스 정리 완료";
}

GstFlowReturn VideoStreamHandler::onNewSample(GstAppSink *sink, gpointer user_data) {
    auto *self = static_cast<VideoStreamHandler*>(user_data);
    GstSample *sample = gst_app_sink_pull_sample(sink);
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
        qWarning() << "[Bus] Error:" << err->message;
        g_error_free(err); g_free(dbg);
        QMetaObject::invokeMethod(self, "restart", Qt::QueuedConnection);
    }
}

void VideoStreamHandler::onStall() {
    if (!isFirstFrame) return;
    qWarning() << "[Stall] 수신 지연, 재시작";
    QMetaObject::invokeMethod(this, "restart", Qt::QueuedConnection);
}

void VideoStreamHandler::restart() {
    qDebug() << "[Restart] 재연결 트리거";
    stop();
    // onStreamThreadFinished() 가 호출되어
    // 리소스 정리→잠 시 후 initialize+tryStart
}
