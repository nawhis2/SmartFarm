/*
    [기능 이름]: 데이터 receive
    [작성자]: 조찬우 (chanwoo)
    [파일 설명]:
      - TCP receive 기능

    [TODO]:
      - [ ] json 형태로 전환

    [참고 사항]:
        데이터 저장위치 루트 안에 들어있는 var폴더 내에 생성
*/
#include "receive.h"
#include "db.h"
#include "userclientsend.h"

// 클라이언트가 보낸 단일 JSONL 한 줄을 버퍼에 저장
static char *jsonl_buf = NULL;
static int jsonl_size = 0;

void store_event_to_mysql(json_t *obj)
{
    // 1) 필드 추출
    const char *etype = json_string_value(json_object_get(obj, "event_type"));
    long ts = json_integer_value(json_object_get(obj, "timestamp"));

    json_t *data = json_object_get(obj, "data");
    json_t *jt;
    const char *class_type =
        (jt = json_object_get(data, "class_type")) && json_is_string(jt)
            ? json_string_value(jt)
            : NULL;
    const char *image_url =
        (jt = json_object_get(data, "image_url")) && json_is_string(jt)
            ? json_string_value(jt)
            : NULL;
    int feature =
        (jt = json_object_get(data, "feature")) && json_is_integer(jt)
            ? (int)json_integer_value(jt)
            : 0;

    // 2) 원본 obj를 JSON_COMPACT 한 줄로 직렬화
    char *raw = json_dumps(obj, JSON_COMPACT);
    if (!raw)
    {
        fprintf(stderr, "json_dumps failed\n");
        return;
    }

    // 3) Prepared Statement
    MYSQL_STMT *stmt = mysql_stmt_init(g_conn);
    const char *sql =
        "INSERT INTO events "
        "(event_type, ts_epoch, class_type, image_url, feature, raw_payload) "
        "VALUES (?, ?, ?, ?, ?, ?)";
    if (mysql_stmt_prepare(stmt, sql, strlen(sql)))
    {
        fprintf(stderr, "Stmt prepare error: %s\n", mysql_stmt_error(stmt));
        free(raw);
        mysql_stmt_close(stmt);
        return;
    }

    MYSQL_BIND bind[6];
    memset(bind, 0, sizeof(bind));

    // event_type
    unsigned long etype_len = strlen(etype);
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (char *)etype;
    bind[0].buffer_length = etype_len;
    bind[0].length = &etype_len;

    // ts_epoch
    bind[1].buffer_type = MYSQL_TYPE_LONG;
    bind[1].buffer = &ts;
    bind[1].is_unsigned = 1;

    // class_type
    my_bool class_null = (class_type == NULL);
    unsigned long class_len = class_type ? strlen(class_type) : 0;
    bind[2].buffer_type = MYSQL_TYPE_STRING;
    bind[2].buffer = (char *)(class_type ? class_type : "");
    bind[2].buffer_length = class_len;
    bind[2].is_null = &class_null;
    bind[2].length = &class_len;

    // image_url
    my_bool url_null = (image_url == NULL);
    unsigned long url_len = image_url ? strlen(image_url) : 0;
    bind[3].buffer_type = MYSQL_TYPE_STRING;
    bind[3].buffer = (char *)(image_url ? image_url : "");
    bind[3].buffer_length = url_len;
    bind[3].is_null = &url_null;
    bind[3].length = &url_len;

    // feature
    bind[4].buffer_type = MYSQL_TYPE_LONG;
    bind[4].buffer = &feature;

    // raw_payload
    my_bool raw_null = 0; // raw는 절대 NULL 아님
    unsigned long raw_len = strlen(raw);
    bind[5].buffer_type = MYSQL_TYPE_STRING;
    bind[5].buffer = raw;
    bind[5].buffer_length = raw_len;
    bind[5].is_null = &raw_null;
    bind[5].length = &raw_len;

    // 4) 실행
    mysql_stmt_bind_param(stmt, bind);
    if (mysql_stmt_execute(stmt))
    {
        fprintf(stderr, "MySQL insert error: %s\n", mysql_stmt_error(stmt));
    }
    mysql_stmt_close(stmt);

    // 5) 메모리 해제
    free(raw);
}

// jsonl을 json으로 저장
static int write_event_with_filepath(const char *buf, int size, const char *file_path)
{
    json_error_t error;
    // 줄바꿈 제거
    char *line = strndup(buf, size);
    if (!line)
        return -1;

    // JSON 파싱
    json_t *obj = json_loads(line, 0, &error);
    free(line);
    if (!obj)
    {
        fprintf(stderr, "JSON parse error: %s at line %d\n", error.text, error.line);
        return -1;
    }

    // data 객체 가져오기
    json_t *data = json_object_get(obj, "data");
    if (!json_is_object(data))
    {
        json_decref(obj);
        return -1;
    }

    // image_url 필드 설정 (파일 시스템 상대 경로)
    // image_url 필드 설정 (절대 파일 경로)
    json_object_set_new(data, "image_url", json_string(file_path));
    json_object_del(obj, "has_file");

    // JSON 파일로 저장
    store_event_to_mysql(obj);

    printf("[EVENT JSON] saved to mysql\n");
    json_decref(obj);
    return 0;
}

int receiveCCTVPacket(SSL *ssl)
{
    char type[6] = {0};
    int size = 0;

    // [1] TYPE 수신
    if (SSL_read(ssl, type, 5) <= 0)
        return -1;
    type[5] = '\0';

    // [2] SIZE 수신
    if (SSL_read(ssl, &size, sizeof(int)) <= 0 || size <= 0)
        return -1;

    // [3] TEXT 처리
    if (strncmp(type, "TEXT", 4) == 0)
    {
        char *msg = (char *)malloc(size + 1);
        if (!msg)
            return -1;
        int rec = 0;
        while (rec < size)
        {
            int n = SSL_read(ssl, msg + rec, size - rec);
            if (n <= 0)
            {
                free(msg);
                return -1;
            }
            rec += n;
        }
        msg[size] = '\0';
        printf("[TEXT] %s\n", msg);
        free(msg);
        return 0;
    }

    // [4] JSONL 한 줄 수신: has_file 플래그 확인 후 처리
    if (strncmp(type, "JSON", 4) == 0)
    {
        // 이전 JSONL 버퍼 해제
        if (jsonl_buf)
            free(jsonl_buf);
        // 새 JSONL 수신
        jsonl_buf = (char *)malloc(size + 1);
        if (!jsonl_buf)
            return -1;
        int rec = 0;
        while (rec < size)
        {
            int n = SSL_read(ssl, jsonl_buf + rec, size - rec);
            if (n <= 0)
            {
                free(jsonl_buf);
                jsonl_buf = NULL;
                return -1;
            }
            rec += n;
        }
        jsonl_buf[size] = '\0';
        jsonl_size = size;
        printf("[JSONL buffered] %s", jsonl_buf);

        // has_file 플래그 확인
        json_error_t error;
        json_t *obj = json_loads(jsonl_buf, 0, &error);
        if (!obj)
        {
            fprintf(stderr, "JSON parse error: %s at line %d", error.text, error.line);
            free(jsonl_buf);
            jsonl_buf = NULL;
            jsonl_size = 0;
            return -1;
        }
        json_t *jf = json_object_get(obj, "has_file");
        int has_file = json_integer_value(jf);
        printf("%d\n", has_file);
        json_decref(obj);

        if (!has_file)
        {
            // 데이터 전송 없이 JSON 이벤트만 저장
            write_event_with_filepath(jsonl_buf, jsonl_size, NULL);
            free(jsonl_buf);
            jsonl_buf = NULL;
            jsonl_size = 0;
        }
        return 0;
    }

    // [5] DATA 처리 (이미지/비디오)
    int ext_len = 0;
    if (SSL_read(ssl, &ext_len, sizeof(int)) <= 0 || ext_len <= 0 || ext_len >= 16)
        return -1;
    char ext[16] = {0};
    if (SSL_read(ssl, ext, ext_len) <= 0)
        return -1;
    ext[ext_len] = '\0';

    // 디렉토리 준비 (절대 경로 사용)
    char subdir[512];
    snprintf(subdir, sizeof(subdir), "%s/%s", RECEIVED_ROOT,
             (strcmp(ext, ".jpeg") == 0) ? "images" : "videos");
    mkdir(RECEIVED_ROOT, 0755);
    mkdir(subdir, 0755);

    // 파일명 생성
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm *tm_info = localtime(&tv.tv_sec);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y%m%d_%H%M%S", tm_info);
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s_%06ld%s",
             subdir, ts, (long)tv.tv_usec, ext);

    // 바이너리 데이터 저장
    FILE *fp = fopen(filepath, "wb");
    if (!fp)
    {
        perror("fopen");
        return -1;
    }
    int rec = 0;
    char buf[2048];
    while (rec < size)
    {
        int to_read = (size - rec) < (int)sizeof(buf) ? (size - rec) : sizeof(buf);
        int n = SSL_read(ssl, buf, to_read);
        if (n <= 0)
        {
            fclose(fp);
            return -1;
        }
        fwrite(buf, 1, n, fp);
        rec += n;
    }
    fclose(fp);
    printf("[DATA] saved to %s (%d bytes)\n", filepath, size);

    // [6] JSON 파일 생성: file_path 필드 추가
    if (jsonl_buf && jsonl_size > 0)
    {
        write_event_with_filepath(jsonl_buf, jsonl_size, filepath);
        free(jsonl_buf);
        jsonl_buf = NULL;
        jsonl_size = 0;
    }
    return 0;
}

void receiveCCTV(SSL *ssl)
{
    while (1)
    {
        if (receiveCCTVPacket(ssl) < 0)
        {
            printf("Client disconnected.\n");
            break;
        }
    }
}

int receiveUserPacket(SSL *ssl)
{
    char type[10] = {0};
    int size = 0;

    // [1] TYPE 수신
    if (SSL_read(ssl, type, 5) <= 0)
        return -1;

    // [2] SIZE 수신
    if (SSL_read(ssl, &size, sizeof(int)) <= 0 || size <= 0)
        return -1;

    char *msg = (char *)malloc(size + 1);
    if (!msg)
        return -1;

    int received = 0;
    while (received < size)
    {
        int n = SSL_read(ssl, msg + received, size - received);
        if (n <= 0)
        {
            free(msg);
            return -1;
        }
        received += n;
    }
    msg[size] = '\0';
    printf("[Msg] : %s\n", msg);
    if (strncmp(type, "IP", 2) == 0)
    {
        ipSend(ssl, msg);
        return 0;
    }
    else if (strncmp(type, "DATA", 4) == 0)
    {
        jsonSend(ssl, msg);
        return 0;
    }
    else if (strncmp(type, "IMAGE", 5) == 0)
    {
        imageSend(ssl, msg);
        return 0;
    }

    free(msg);
    return -1;
}

void receiveUser(SSL *ssl)
{
    while (1)
    {
        int bytes = receiveUserPacket(ssl);
        if (bytes < 0)
        {
            printf("Client disconnected.\n");
            break;
        }
    }
}
