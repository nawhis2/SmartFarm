#ifndef STORAGE_H
#define STORAGE_H

#include "main.h"

#define STORAGE_PATH  "/tmp/active_cctvs.bin"

typedef struct cctv{
    char name[32];   // CCTV 고유 이름
    char ip[16];     // IP
} ActiveCCTV;

// 파일 초기화
void init_storage();

// IP 뽑아오기
int register_cctv(SSL *, const char *);

// 새로운 CCTV 등록
int add_active_cctv(const ActiveCCTV *);

// 이름으로 CCTV 제거
int remove_active_cctv(const char *);

// 이름으로 CCTV 조회. 찾으면 1, 못 찾으면 0 반환.
int lookup_active_cctv(const char *, char *, size_t);

#endif