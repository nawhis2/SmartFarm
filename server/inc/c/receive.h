#ifndef RECEIVE_H
#define RECEIVE_H


#include <jansson.h>
#include "main.h"
#include "storage.h"

// 로그 및 저장 디렉토리 절대 경로 설정
#define RECEIVED_ROOT "/home/chanwoo/git/SmartFarm/received"

// CCTV
void receiveCCTV(SSL*);
int receiveCCTVPacket(SSL*);

// User
void receiveUser(SSL*);
int receiveUserPacket(SSL*);

#endif