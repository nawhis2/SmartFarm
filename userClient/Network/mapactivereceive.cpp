#include "mapactivereceive.h"
#include "sensorreceive.h"
#include <QThread>
#include <QDebug>
int mapActiveStop = 0;

QThread* startReceiveMapThread(SSL* sensor) {
    mapActiveStop = 0;

    // QThread::create 은 Qt 5.10+ 에서 사용 가능
    QThread* thread = QThread::create([sensor]() {
        receiveMapActive(sensor);
    });

    // 쓰레드가 끝나면 스스로 정리
    QObject::connect(thread, &QThread::finished,
                     thread, &QObject::deleteLater);

    // 쓰레드 시작
    thread->start();

    return thread;
}

void receiveMapActive(SSL* ssl) {
    // 1) SSL 에서 소켓 FD 얻기
    SOCKET s = (SOCKET)SSL_get_fd(ssl);

    // 2) 논블로킹 모드로 설정
    u_long mode = 1; // 1 = non-blocking
    if (ioctlsocket(s, FIONBIO, &mode) != 0) {
        qWarning() << "ioctlsocket(FIONBIO) failed:" << WSAGetLastError();
        return;
    }

    // 3) 읽기 루프
    while (!mapActiveStop) {
        char buf[10];

        int n;
        if((n = SSL_read(ssl, buf, strlen(buf))) > 0){
            qDebug() << buf;
            continue;
        }
        int err = SSL_get_error(ssl, n);
        // 데이터가 아직 준비되지 않았거나, 논블로킹 모드에서 데이터 없음
        if (err == SSL_ERROR_WANT_READ   ||
            err == SSL_ERROR_WANT_WRITE  ||
            WSAGetLastError() == WSAEWOULDBLOCK)
        {
            // 잠깐 쉬고 플래그 재검사
            QThread::msleep(10);
            continue;
        }

        qWarning() << "SSL_read fatal error or connection closed, err=" << err;
        return;
    }
}
