# 🌱 VEDA Final Project - SmartFarm

본 프로젝트는 **스마트팜** 환경에서 **실시간 영상 분석**, **침입/화재 감지**, **생장 분석**, **환경 제어**를 통합적으로 수행하는 시스템입니다.  
라즈베리파이, CCTV, 센서, Qt 기반 클라이언트, OpenSSL 기반 TCP 통신 등을 이용하여 구현되었습니다.

---

## 📌 프로젝트 개요

### 🎯 핵심 목표
- CCTV 및 센서를 활용한 **스마트팜 보안/환경 관리**
- **실시간 침입 감지** (IR 카메라 + OpenCV 모션 분석)
- **화재 감지** (화염 딥러닝 모델 + CO₂ 센서)
- **딸기 생장 분석 및 질병 관리** (YOLOv5 기반)
- **환경 제어** (토양, 온도, 조도 센서 데이터 기반 장치 제어)
- **보안 통신** (OpenSSL 기반 TLS)

---

## 🛠 주요 기능
1. **메인 서버**
   - TCP + OpenSSL 보안 통신
   - 이벤트 및 이미지 데이터 수신/저장 (MySQL 연동)
   - 클라이언트 명령 처리

2. **CCTV RTSP 서버**
   - GStreamer 기반 RTSP 스트리밍
   - TLS 적용 RTSP 전송
   - 실시간 영상 처리 및 감지 결과 전송

3. **Qt 클라이언트**
   - 실시간 이벤트 모니터링
   - 이미지/영상 보기
   - 제어 명령 전송

---

## 📦 빌드 방법

### 1. 메인 서버 빌드
```bash
g++ main.c src/c/* src/cpp/*     -o main     -I inc/c -I inc/cpp     -lssl -lcrypto -ljansson -lmysqlclient -lcurl
```

### 2. 기타(영상 처리/RTSP 서버 등) 빌드
```bash
g++ main.c src/c/* src/cpp/*     -o main     -I inc/c/ -I inc/cpp/     $(pkg-config --cflags --libs opencv4 gstreamer-1.0 gstreamer-app-1.0 gstreamer-rtsp-server-1.0)     -lssl -lcrypto -ljansson -lgio-2.0 -lgobject-2.0 -lglib-2.0
```

---

## 🔐 OpenSSL 인증서 발급

### 1. 메인 서버용 인증서
```bash
# server-key.pem, server-cert.pem 생성
openssl req -new -newkey rsa:2048 -nodes -keyout server-key.pem -out server.csr
openssl x509 -req -days 365 -in server.csr -signkey server-key.pem -out server-cert.pem
```

### 2. CCTV RTSP 서버용 인증서
```bash
openssl req -x509 -nodes -newkey rsa:2048     -keyout key.pem     -out cert.pem     -days 365     -subj "/C=KR/ST=Seoul/O=SmartFarm/CN="     -addext "subjectAltName=IP:<RTSP_서버_IP>"
```
> `<RTSP_서버_IP>`를 실제 서버 IP로 변경

---

## 🚀 실행 방법

1. **서버와 RTSP용 OpenSSL 인증서 발급**
    - 위의 절차에 따라 메인 서버와 RTSP 서버 각각 인증서 생성
2. **메인 서버 실행**
    ```bash
    ./main
    ```
3. **클라이언트 접속**
    - 클라이언트 실행 시 서버 IP와 포트를 지정하여 접속
   ```bash
   ./main <서버 주소>
   ```
4. **RTSP 서버 실행**
    - 인증서와 함께 GStreamer RTSP 서버 실행

---

## 📌 개발 환경
- **언어**: C, C++
- **플랫폼**: Linux (Raspberry Pi 4 포함)
- **라이브러리**:
  - OpenSSL, MySQL Client, cURL, Jansson
  - OpenCV 4.x
  - GStreamer 1.0 (app, rtsp-server)
- **IDE**: Qt Creator, VS Code

---

## 📷 시스템 구조
```
[CCTV RTSP 서버] --(TLS/RTSP)--> [메인 서버] --(TCP/TLS)--> [Qt 클라이언트]
                      |                                ↑
                      └--------> [MySQL 이벤트 로그]   |
```

---

## 📞 문의
- 프로젝트명: VEDA Final Project
- 담당: ps496@naver.com
