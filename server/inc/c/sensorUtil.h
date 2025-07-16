#ifndef SENSORUTIL_H
#define SENSORUTIL_H

#include <openssl/ssl.h>
#include <openssl/err.h>

#define SENSOR_PIPE "/tmp/sensor_pipe"

extern int stop_pipe;

void *sensorFire(void *arg);
void *sensorUser(void *arg);

pthread_t regisSensorFire(SSL*);
pthread_t regisSensorUser(SSL*);

#endif