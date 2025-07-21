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

static std::string latest_data;
static std::mutex m;
static std::atomic<bool> running(true);

static void handleSigint(int)
{
    std::cout << "\n[INFO] Signal received, stopping threads..." << std::endl;
    running = false; // Stop the threads
}

static std::vector<std::string> parseSensorData(std::string uartData) {
    std::vector<std::string> sensorData;
    size_t pos = 0;
    while ((pos = uartData.find(',')) != std::string::npos) {
        sensorData.push_back(uartData.substr(0, pos));
        uartData.erase(0, pos + 1);
    }
    if (!uartData.empty()) {
        sensorData.push_back(uartData); // Add the last part
    }
    return sensorData;
}

static void *uartReceiveLoop(void* arg) {
    char buffer[64];
    UARTdevice uartDevice;
    if (uartDevice.getFd() < 0) {
        std::cerr << "UART device not initialized." << std::endl;
        return;
    }   

    while (running) {
        ssize_t bytesRead = read(uartDevice.getFd(), buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0'; // Null-terminate the string
            std::lock_guard<std::mutex> lock(m);
            latest_data = std::string(buffer, bytesRead);
            //std::cout << "Received data: " << buffer << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "UART receive loop finished." << std::endl;
    pthread_exit(NULL); // Exit the thread
}

static void *sendEveryMinute(void* arg) {
    SSL* sensor = (SSL*)arg;
    while (running) {
        std::this_thread::sleep_for(std::chrono::minutes(1));
        std::vector<std::string> parsedData;
        {
            std::lock_guard<std::mutex> lock(m);
            parsedData = parseSensorData(latest_data); // Parse the latest data
        }

        // copy한 최신 데이터로 SensorData 구조체 생성
        SensorData sensorData = makeSensorData(std::stof(parsedData.at(0)),
                                            std::stof(parsedData.at(1)),
                                            std::stof(parsedData.at(2))); // 예시 데이터
        sendSensorData(sensor, sensorData); // 센서 데이터 전송
    }
    
    std::cout << "Sensor data sending thread finished." << std::endl;
    pthread_exit(NULL); // 스레드 종료
}

// SSL* 데이터
void *sensorThreadFunc(void* arg) 
{
    // 주어진 인자를 해석가능하게 형 변환
    signal(SIGINT, handleSigint); // SIGINT 핸들러 설정
    if (arg == nullptr) {
        std::cerr << "Invalid argument passed to sensorThreadFunc." << std::endl;
        pthread_exit(NULL);
    }
    SSL* sensor = (SSL*)arg;

    // UART 데이터를 수신하는 로직
    pthread_t uartThread;
    pthread_t sensorSendThread;
    if (pthread_create(&uartThread, NULL, uartReceiveLoop, NULL) != 0) {
        perror("UART Thread Create");
    }   
    if (pthread_create(&sensorSendThread, NULL, sendEveryMinute, (void*)sensor) != 0) {
        perror("Sensor Send Thread Create");
    }

    // 스레드가 종료될 때까지 기다림
    pthread_join(uartThread, NULL);
    pthread_join(sensorSendThread, NULL);

    // cleanup -> main 함수에서 SSL 객체를 관리하므로 여기서는 필요 없음
    // SSL_free(sensor); // 이 부분은 main 함수에서 관리하므로 주석 처리
    // SSL_CTX_free(ctx); // 이 부분도 main 함수에서 관리하므로 주석 처리
    // close(uartDevice.getFd()); // UART 디바이스는 소멸자에서 자동으로 닫히므로 필요 없음
    std::cout << "Sensor thread finished." << std::endl;
    pthread_exit(NULL); // 스레드 종료  
}