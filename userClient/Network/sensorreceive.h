#ifndef SENSORRECEIVE_H
#define SENSORRECEIVE_H
#include <openssl/ssl.h>
#include <openssl/err.h>

extern int sensorStop;
class QThread;
// 센서 구조체 타입 정의
struct SensorData {
    float co2Value;
    float humidValue;
    float tempValue;
};

extern SensorData sensorData;

QThread* startReceiveSensorThread(SSL* sensor);
void receiveSensor(SSL* sensor);

#endif // SENSORRECEIVE_H
