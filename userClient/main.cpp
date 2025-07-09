#include <QApplication>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <mainwindow.h>
#include "main.h"
#include "network.h"
#include "handshake.h"

GstElement *pipeline = nullptr;
GstElement *appsink = nullptr;
QLabel *videoLabel = nullptr;

int main(int argc, char *argv[]) {
    gst_init(nullptr, nullptr);

    init_openssl();
    SSL_CTX *ctx = create_context();

    int sockfd = socketNetwork("192.168.0.46");
    if(network(sockfd, ctx) < 1){
        perror("SSL");
        exit(1);
    }
    handshakeClient("USER", NULL);

    QApplication app(argc, argv);
    MainWindow w;
    w.setWindowTitle("🌱 스마트팜 통합 모니터링 시스템");
    w.resize(1000, 700);
    w.show();

    QObject::connect(&w, &QObject::destroyed, [=]() {
        if (sock_fd) {
        #ifdef _WIN32
            closesocket(sockfd);
        #else
            close(sockfd);
        #endif
            SSL_shutdown(sock_fd);
            SSL_CTX_free(ctx);
            SSL_free(sock_fd);
        }
    });
    return app.exec();
}

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

// OpenSSL 초기화
void init_openssl() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

// SSL 컨텍스트 생성 (클라이언트용)
SSL_CTX *create_context() {
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = SSLv23_client_method();
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("SSL 컨텍스트 생성 실패");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}
