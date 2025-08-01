#include "sensorreceive.h"
#include "firedetectwidget.h" //추가
#include <winsock2.h>
#include <windows.h>
#include <QDebug>
#include <QThread>
#include <QMetaObject>  // invokeMethod

extern FireDetectWidget* fireWidget;

int sensorStop = 0;

SensorData sensorData = {
    0.f,  // co2Value
    0.f,  // humidValue
    0.f   // tempValue
};

QThread* startReceiveSensorThread(SSL* sensor) {
    sensorStop = 0;

    // QThread::create 은 Qt 5.10+ 에서 사용 가능
    QThread* thread = QThread::create([sensor]() {
        receiveSensor(sensor);
    });

    // 쓰레드가 끝나면 스스로 정리
    QObject::connect(thread, &QThread::finished,
                     thread, &QObject::deleteLater);

    // 쓰레드 시작
    thread->start();

    return thread;
}

void receiveSensor(SSL* ssl) {
    // 1) SSL 에서 소켓 FD 얻기
    SOCKET s = (SOCKET)SSL_get_fd(ssl);

    // 2) 논블로킹 모드로 설정
    u_long mode = 1; // 1 = non-blocking
    if (ioctlsocket(s, FIONBIO, &mode) != 0) {
        qWarning() << "ioctlsocket(FIONBIO) failed:" << WSAGetLastError();
        return;
    }

    // 3) 읽기 루프
    while (!sensorStop) {
        char* buf = reinterpret_cast<char*>(&sensorData);
        int total    = sizeof(SensorData);
        int received = 0;

        // 구조체 크기만큼 반복 읽기
        while (received < total && !sensorStop) {
            int n = SSL_read(ssl, buf + received, total - received);
            if (n > 0) {
                received += n;
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

        // 4) 온전히 한 덩어리 다 읽었을 때만 처리
        if (received == total) {
            float co2 = sensorData.co2Value;
            float temp = sensorData.tempValue;

            QString data = QString("%1 %2").arg(co2).arg(temp);

            if (fireWidget) {
                QMetaObject::invokeMethod(fireWidget, "onSensorDataReceivedWrapper", Qt::QueuedConnection);
            }
            qDebug() << "co2   =" << sensorData.co2Value
                     << "humid =" << sensorData.humidValue
                     << "temp  =" << sensorData.tempValue;
        }
    }
}
