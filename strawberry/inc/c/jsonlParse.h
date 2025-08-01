#ifndef JSONLPARSE_H
#define JSONLPARSE_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <openssl/ssl.h>

typedef unsigned char uchar;
#define JSONL_DIR "../log"

// JSONL 이벤트 구조체
typedef struct
{
    char event_type[64]; // 이벤트 타입
    int has_file;        // 0: 데이터 없음, 1: DATA 전송 예정
    char class_type[64]; // optional
    int has_class_type;  // 0: null, 1: valid
    int feature;         // optional
    int has_feature;     // 0: null, 1: valid
} JSONLEvent;

// jsonl 구조체 생성
JSONLEvent create_jsonl_event(const char *, int, const char *, int, int, int);
// jsonl 생성
char *make_jsonl_event(const JSONLEvent *);
// jsonl과 데이터 전송
void send_jsonl_event(const char *, int, const char *, int, int, int, const uchar *, int, const char *);
// jsonl 해제
void free_jsonl_event(char *);

#endif
