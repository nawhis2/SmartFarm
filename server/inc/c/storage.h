#ifndef STORAGE_H
#define STORAGE_H

#include "main.h"

#define STORAGE_PATH  "/tmp/active_cctvs.bin"

typedef struct cctv{
    char name[32];   // CCTV 고유 이름
    char ip[16];     // IP
} ActiveCCTV;

// 파일 초기화
void initStorage();

// IP 뽑아오기
int registerCCTV(SSL *, const char *);

// 새로운 CCTV 등록
int addActiveCCTV(const ActiveCCTV *);

// 이름으로 CCTV 제거
int removeActiveCCTV(const char *);

// 이름으로 CCTV 조회. 찾으면 1, 못 찾으면 0 반환.
int lookupActiveCCTV(const char *, char *);

#endif