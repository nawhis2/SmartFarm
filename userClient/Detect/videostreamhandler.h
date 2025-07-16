#ifndef VIDEOSTREAMHANDLER_H
#define VIDEOSTREAMHANDLER_H

#include <QObject>
#include <QTimer>
#include <QImage>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include "streamthread.h"

#define debugIdx(type, idx) type() << "["<< idx << "]"

class VideoStreamHandler : public QObject {
    Q_OBJECT
public:
    VideoStreamHandler(int idx, const QString &rtspUrl, QObject *parent = nullptr);
    ~VideoStreamHandler();

    void tryStart();
    void stop();

signals:
    void frameReady(int index, const QImage &frame);

private slots:
    void onStreamThreadFinished();
    void onApplicationQuit();
    void onStall();
    void restart();

private:
    void initialize();
    static GstFlowReturn onNewSample(GstAppSink *sink, gpointer user_data);
    static void onBusMessage(GstBus *bus, GstMessage *msg, gpointer user_data);

    int index;
    QString url;
    GstElement *pipeline;
    GMainLoop *loop;
    bool isFirstFrame;
    QTimer *retryTimer;
    QTimer *watchdogTimer;
    StreamThread *streamThread;
};

#endif // VIDEOSTREAMHANDLER_H
