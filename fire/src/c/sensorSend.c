#include "sensorSend.h"

SensorData makeSensorData(const float co2, const float humid, const float temp){
    SensorData newSensorData;
    newSensorData.co2Value = co2;
    newSensorData.humidValue = humid;
    newSensorData.tempValue = temp;

    return newSensorData;
}

void sendSensorData(SSL *sensor, const SensorData sensorData){
    const char *buf = (const char *)&sensorData;
    size_t total    = sizeof(SensorData);
    size_t sent     = 0;

    while (sent < total) {
        int n = SSL_write(sensor, buf + sent, (int)(total - sent));
        if (n <= 0) {
            int err = SSL_get_error(sensor, n);
            fprintf(stderr, "SSL_write failed: %d\n", err);
            return;
        }
        sent += (size_t)n;
    }
}