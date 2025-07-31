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
static UARTdevice uartDevice;
extern bool ledState; // LED 상태

static std::vector<std::string> parseSensorData(std::string uartData) {
    std::vector<std::string> sensorData;
    size_t pos = 0;

    while ((pos = uartData.find('|')) != std::string::npos) 
    {
        sensorData.push_back(uartData.substr(0, pos));
        uartData.erase(0, pos + 1);
    }
    if (!uartData.empty()) {
        sensorData.push_back(uartData); // Add the last part
    }
    return sensorData;
}

static void *uartReceiveLoop(void* arg) {
    char buffer[128];
    if (uartDevice.getFd() < 0) {
        std::cerr << "UART device not initialized." << std::endl;
        {
            std::lock_guard<std::mutex> lock(m);
            running = false;    
        }
        return nullptr;
    }   
    
    while (running) {
        ssize_t bytesRead = read(uartDevice.getFd(), buffer, sizeof(buffer) - 1);
        // std::cout << "[Sensor] : " << buffer << '\n';
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0'; // Null-terminate the string
            std::lock_guard<std::mutex> lock(m);
            latest_data = std::string(buffer, bytesRead);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    pthread_exit(NULL); // Exit the thread
    return nullptr; // Exit the thread
}

static void *sendEveryMinute(void* arg) {
    SSL* sensor = (SSL*)arg;
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        std::vector<std::string> parsedData;
        {
            std::lock_guard<std::mutex> lock(m);
            std::cout << "[Sensor] : " << latest_data << std::endl;
            parsedData = parseSensorData(latest_data); // Parse the latest data
        }

        // copy한 최신 데이터로 SensorData 구조체 생성
        if (parsedData.size() < 3) {
            std::cerr << "Invalid sensor data received." << std::endl;
            continue; // Skip if data is not valid
        }

        try {
            float co2Value = std::stof(parsedData.at(0));
            float humidValue = std::stof(parsedData.at(1));
            float tempValue = std::stof(parsedData.at(2));

            if (co2Value > 2000 && !ledState)
            {
                std::cout << "High CO2 level detected: " << co2Value << " ppm" << std::endl;
                ledState = true; // LED 상태 변경

                write(uartDevice.getFd(), "L1", 2);
            }
            else if (co2Value <= 2000 && ledState)
            {
                std::cout << "CO2 level back to normal: " << co2Value << " ppm" << std::endl;
                ledState = false; // LED 상태 변경
                write(uartDevice.getFd(), "L0", 2);
            }

            SensorData sensorData = makeSensorData(
                co2Value,
                humidValue,
                tempValue
            );
            sendSensorData(sensor, sensorData);
        } catch (const std::exception& e) {
            std::cerr << "Exception parsing sensor data: " << e.what() << std::endl;
            continue;
        }
    }
    pthread_exit(NULL); // 스레드 종료
    return nullptr; // 스레드 종료
}

// SSL* 데이터
void *sensorThreadFunc(void* arg) 
{
    // 주어진 인자를 해석가능하게 형 변환
    //signal(SIGINT, handleSigint); // SIGINT 핸들러 설정
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
    return nullptr; // 스레드 종료
}