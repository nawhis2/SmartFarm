#ifndef VIDEOSTREAMHANDLER_H
#define VIDEOSTREAMHANDLER_H

#include <QObject>
#include <QImage>
#include <QLabel>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>

class VideoStreamHandler : public QObject {
    Q_OBJECT

public:
    explicit VideoStreamHandler(const QString &rtspUrl, QLabel *targetLabel, QObject *parent = nullptr);
    ~VideoStreamHandler();

    void start();
    void stop();

private:
    QString url;
    QLabel *label;
    GstElement *pipeline = nullptr;

    static GstFlowReturn onNewSample(GstAppSink *sink, gpointer user_data);
    static void onBusMessage(GstBus *bus, GstMessage *msg, gpointer user_data);

signals:
    void frameReady(const QImage &img);

private slots:
    void onNewFrame(const QImage &img);
};

#endif // VIDEOSTREAMHANDLER_H
