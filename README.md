Veda Final 프로젝트

빌드 방법

메인 서버
- g++ main.c src/c/* src/cpp/* -o main -I inc/c -I inc/cpp -lssl -lcrypto -ljansson -lmysqlclient -lcurl

그 외
- g++ main.c src/c/* src/cpp/* -o main -I inc/c/ -I inc/cpp/ $(pkg-config --cflags --libs opencv4 gstreamer-1.0 gstreamer-app-1.0 gstreamer-rtsp-server-1.0) -lssl -lcrypto -ljansson -lgio-2.0 -lgobject-2.0 -lglib-2.0

openssl 인증서 발급

메인서버
openssl x509 -req -days 365 -in server.csr -signkey server-key.pem -out server-cert.pem

cctv rtsp서버
openssl req -x509 -nodes -newkey rsa:2048 \
  -keyout <server>-key.pem \
  -out <server>-cert.pem \
  -days 365 \
  -subj "/C=KR/ST=Seoul/O=SmartFarm/CN=<IP addr>" \
  -addext "subjectAltName=IP:<IP addr>"

실행 방법
1. 서버와 RTSP용 openssl 인증서 발급
2. 메인 서버 실행 후 클라이언트 접속
