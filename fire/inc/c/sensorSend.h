#ifndef SENSORSEND_H
#define SENSORSEND_H

#include <atomic>
#include <openssl/ssl.h>
#include <openssl/err.h>

typedef struct{
    float co2Value;
    float humidValue;
    float tempValue;
}SensorData;

SensorData makeSensorData(const float, const float, const float);
void sendSensorData(SSL*, const SensorData);

#endif