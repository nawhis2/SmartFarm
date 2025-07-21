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


static std::vector<std::string> parseSensorData(std::string uartData) {
    std::vector<std::string> sensorData;
    size_t pos = 0;

    std::cout << "Parsing sensor data: " << uartData << std::endl;
    while ((pos = uartData.find('|')) != std::string::npos) 
    {
        sensorData.push_back(uartData.substr(0, pos));
        uartData.erase(0, pos + 1);
    }
    if (!uartData.empty()) {
        sensorData.push_back(uartData); // Add the last part
    }
    std::cout << "Parsed sensor data size: " << sensorData.size() << std::endl;
    for (auto e : sensorData) {
        std::cout << e << " ";
    }
    std::cout << std::endl;
    return sensorData;
}

static void *uartReceiveLoop(void* arg) {
    char buffer[128];
    UARTdevice uartDevice;
    if (uartDevice.getFd() < 0) {
        std::cerr << "UART device not initialized." << std::endl;
        {
            std::lock_guard<std::mutex> lock(m);
            running = false;    
        }
        return nullptr;
    }   
    
    std::cout << "UART receive loop started." << std::endl;
    while (running) {
        ssize_t bytesRead = read(uartDevice.getFd(), buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0'; // Null-terminate the string
            std::lock_guard<std::mutex> lock(m);
            latest_data = std::string(buffer, bytesRead);
            std::cout << "Received data: " << buffer << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "UART receive loop finished." << std::endl;
    pthread_exit(NULL); // Exit the thread
    return nullptr; // Exit the thread
}

static void *sendEveryMinute(void* arg) {
    SSL* sensor = (SSL*)arg;
    std::cout << "Sensor data sending thread started." << std::endl;
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        std::vector<std::string> parsedData;
        {
            std::lock_guard<std::mutex> lock(m);
            parsedData = parseSensorData(latest_data); // Parse the latest data
        }

        std::cout << "Parsed sensor data: ";
        for (const auto& data : parsedData) {
            std::cout << data << " ";
        }
        std::cout << std::endl;

        // copy한 최신 데이터로 SensorData 구조체 생성
        if (parsedData.size() < 3) {
            std::cerr << "Invalid sensor data received." << std::endl;
            continue; // Skip if data is not valid
        }

        try {
            SensorData sensorData = makeSensorData(
                std::stof(parsedData.at(0)),
                std::stof(parsedData.at(1)),
                std::stof(parsedData.at(2)));
            std::cout << "**** SensorData created: "
                      << "CO2: " << sensorData.co2Value
                      << ", Humidity: " << sensorData.humidValue
                      << ", Temperature: " << sensorData.tempValue
                      << std::endl;
            sendSensorData(sensor, sensorData);
            std::cout << "SENDING complete in try block!" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Exception parsing sensor data: " << e.what() << std::endl;
            continue;
        }
        // 데이터 전송 후 로그 출력
        std::cout << "sending completed !" << std::endl;
    }
    
    std::cout << "Sensor data sending thread finished." << std::endl;
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
    std::cout << "[SSL*] " << sensor << std::endl;
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