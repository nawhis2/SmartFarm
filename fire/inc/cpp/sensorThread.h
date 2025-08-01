#ifndef __SENSOR_THREAD_H__
#define __SENSOR_THREAD_H__

#include "sensorSend.h"
#include "commSTM32.h"
#include <pthread.h>
#include <mutex>
#include <chrono>  
#include <thread>
#include <atomic>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <vector>

std::vector<std::string> parseSensorData(std::string uartData);
void *sensorThreadFunc(void* arg) ;

#endif // __SENSOR_THREAD_H__