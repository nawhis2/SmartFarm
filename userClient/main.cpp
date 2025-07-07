#include <QApplication>
#include <QLabel>
#include <QTimer>
#include <QImage>
#include <QVBoxLayout>
#include <QWidget>
#include <mainwindow.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>

GstElement *pipeline = nullptr;
GstElement *appsink = nullptr;
QLabel *videoLabel = nullptr;

QImage gstSampleToQImage(GstSample *sample) {
    if (!sample) return QImage();

    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstCaps *caps = gst_sample_get_caps(sample);
    if (!buffer || !caps) return QImage();

    GstStructure *s = gst_caps_get_structure(caps, 0);
    int width, height;
    gst_structure_get_int(s, "width", &width);
    gst_structure_get_int(s, "height", &height);

    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) return QImage();

    QImage img((uchar *)map.data, width, height, QImage::Format_RGB888);
    QImage copy = img.copy();

    gst_buffer_unmap(buffer, &map);
    return copy;
}

void updateFrame() {
    GstSample *sample = gst_app_sink_try_pull_sample(GST_APP_SINK(appsink), 10000);
    if (!sample) return;

    QImage img = gstSampleToQImage(sample);
    if (!img.isNull() && videoLabel) {
        videoLabel->setPixmap(QPixmap::fromImage(img).scaled(videoLabel->size(), Qt::KeepAspectRatio));
    }

    gst_sample_unref(sample);
}

void init(void)
{
    gst_init(nullptr, nullptr);
    qputenv("GST_PLUGIN_PATH", "C:/gstreamer/1.0/msvc_x86_64/lib/gstreamer-1.0");
    qputenv("GST_PLUGIN_SYSTEM_PATH", "C:/gstreamer/1.0/msvc_x86_64/lib/gstreamer-1.0");
    qputenv("GST_PLUGIN_SCANNER", "C:/gstreamer/1.0/msvc_x86_64/libexec/gstreamer-1.0/gst-plugin-scanner.exe");
    qputenv("PATH", qgetenv("PATH") + ";C:/gstreamer/1.0/msvc_x86_64/bin");
}

int main(int argc, char *argv[]) {
    init();
    QApplication app(argc, argv);
    MainWindow w;
    w.setWindowTitle("🌱 스마트팜 통합 모니터링 시스템");
    w.resize(1000, 700);
    w.show();

    return app.exec();
}
