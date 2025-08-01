#include "jsonlParse.h"
#include "clientUtil.h"
JSONLEvent create_jsonl_event(const char *event_type,
                              int has_file,
                              const char *class_type,
                              int has_class_type,
                              int feature,
                              int has_feature)
{
    JSONLEvent ev;
    // event_type 복사 (안전하게 널 종료)
    strncpy(ev.event_type, event_type, sizeof(ev.event_type) - 1);
    ev.event_type[sizeof(ev.event_type) - 1] = '\0';

    // has_file
    ev.has_file = has_file ? 1 : 0;

    // class_type / has_class_type
    if (has_class_type && class_type)
    {
        strncpy(ev.class_type, class_type, sizeof(ev.class_type) - 1);
        ev.class_type[sizeof(ev.class_type) - 1] = '\0';
        ev.has_class_type = 1;
    }
    else
    {
        ev.class_type[0] = '\0';
        ev.has_class_type = 0;
    }

    // feature / has_feature
    if (has_feature)
    {
        ev.feature = feature;
        ev.has_feature = 1;
    }
    else
    {
        ev.feature = 0;
        ev.has_feature = 0;
    }

    return ev;
}

size_t calculate_jsonl_event_size(const JSONLEvent *ev)
{
    size_t len_event = strlen(ev->event_type);
    size_t len_class = ev->has_class_type
                           ? strlen(ev->class_type)
                           : strlen("null");

    // 타임스탬프 길이 (epoch seconds)
    time_t now = time(NULL);
    char tsbuf[32];
    int len_ts = snprintf(tsbuf, sizeof(tsbuf), "%ld", (long)now);

    // feature 길이
    int len_feat = ev->has_feature
                       ? snprintf(NULL, 0, "%d", ev->feature)
                       : strlen("null");

    // 2) 상수 오버헤드 길이 (키, 따옴표, 쉼표, 괄호 등)
    const size_t overhead =
        strlen("{\"event_type\":\"\",") + // {"event_type":"",
        strlen("\"timestamp\":,") +       // "timestamp":
        strlen("\"has_file\":,") +        // "has_file":
        strlen("\"data\":{") +            // "data":{
        strlen("\"class_type\":\"\",") +      // "class_type":
        strlen("\"feature\":") +          // "feature":
        strlen("}}\n");                   // }}\n

    // 3) 총합 + 널 종료
    return len_event + len_class + (size_t)len_ts + (size_t)len_feat + overhead + 1; // '\0'
}

void backup_jsonl(const char *line)
{
    // 1) 디렉터리 생성
    mkdir(JSONL_DIR, 0755);

    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm *tm_info = localtime(&tv.tv_sec);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y%m%d_%H%M%S", tm_info);

    char outpath[512];
    snprintf(outpath, sizeof(outpath), "%s/%s_%06ld.jsonl",
             JSONL_DIR, ts, (long)tv.tv_usec);

    // 2) 파일 열기
    FILE *fp = fopen(outpath, "a");
    if (!fp)
    {
        perror("fopen JSONL backup");
        return;
    }

    // 3) 한 줄 쓰기(개행 여부 체크)
    size_t len = strlen(line);
    if (len > 0 && line[len - 1] == '\n')
    {
        fputs(line, fp);
    }
    else
    {
        fprintf(fp, "%s\n", line);
    }

    fclose(fp);
}

char *make_jsonl_event(const JSONLEvent *ev)
{
    time_t now = time(NULL);
    // buffer size estimate
    size_t needed = calculate_jsonl_event_size(ev);
    char *buf = (char *)malloc(needed);
    if (!buf)
        return NULL;

    int off = 0;
    off += snprintf(buf + off, needed - off,
                    "{\"event_type\":\"%s\","
                    "\"timestamp\":%ld,"
                    "\"has_file\":%d,"
                    "\"data\":{",
                    ev->event_type, (long)now, ev->has_file);

    // class_type
    if (ev->has_class_type)
    {
        off += snprintf(buf + off, needed - off,
                        "\"class_type\":\"%s\",", ev->class_type);
    }
    else
    {
        off += snprintf(buf + off, needed - off,
                        "\"class_type\":null,");
    }

    // feature
    if (ev->has_feature)
    {
        off += snprintf(buf + off, needed - off,
                        "\"feature\":%d", ev->feature);
    }
    else
    {
        off += snprintf(buf + off, needed - off,
                        "\"feature\":null");
    }

    off += snprintf(buf + off, needed - off, "}}\n");

    backup_jsonl(buf);
    return buf;
}

void send_jsonl_event(const char *event_type,
                      int has_file,
                      const char *class_type,
                      int has_class_type,
                      int feature,
                      int has_feature,
                      const uchar *data,
                      int size,
                      const char *ext)
{
    JSONLEvent ev = create_jsonl_event(
        event_type,     // event_type
        has_file,       // has_file
        class_type,     // class_type
        has_class_type, // has_class_type
        feature,        // feature
        has_feature     // has_feature
    );

    char *jsonlFile = make_jsonl_event(&ev);
    sendFile(jsonlFile, "JSON");
    if (data)
        sendData(data, size, ext);
    free_jsonl_event(jsonlFile);
}

// make_jsonl_event()가 반환한 문자열을 해제합니다.
void free_jsonl_event(char *line)
{
    free(line);
}