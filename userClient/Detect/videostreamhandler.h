#ifndef VIDEOSTREAMHANDLER_H
#define VIDEOSTREAMHANDLER_H

#include <QObject>
#include <QTimer>
#include <QtConcurrent>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <QFutureWatcher>
#include <QCoreApplication>

class VideoStreamHandler : public QObject
{
    Q_OBJECT
public:
    VideoStreamHandler(int idx, const QString &rtspUrl, QObject *parent = nullptr);
    ~VideoStreamHandler();

    void initialize();
    void tryStart();
    void stop();

signals:
    void frameReady(int index, const QImage &frame);

private slots:
    static GstFlowReturn onNewSample(GstAppSink *sink, gpointer user_data);
    static void onBusMessage(GstBus *bus, GstMessage *msg, gpointer user_data);
    void onStall();
    void restart();
    void onStreamThreadFinished();
    void onApplicationQuit();

private:
    QString url;
    GstElement *pipeline;
    GMainLoop *loop;
    int index;
    bool isFirstFrame;
    QTimer *retryTimer;
    QTimer *watchdogTimer;

    QFuture<void>     streamFuture;
    QFutureWatcher<void> *streamWatcher;
};

#endif // VIDEOSTREAMHANDLER_H
