#include "jsonlParse.h"
#include "clientUtil.h"
// JSONLEvent 생성
JSONLEvent create_jsonl_event(const char *event_type,
                              int has_file,
                              const char *class_type,
                              int has_class_type,
                              int feature,
                              int has_feature)
{
    JSONLEvent ev;
    strncpy(ev.event_type, event_type, sizeof(ev.event_type) - 1);
    ev.event_type[sizeof(ev.event_type) - 1] = '\0';

    ev.has_file = has_file ? 1 : 0;

    if (has_class_type && class_type) {
        strncpy(ev.class_type, class_type, sizeof(ev.class_type) - 1);
        ev.class_type[sizeof(ev.class_type) - 1] = '\0';
        ev.has_class_type = 1;
    } else {
        ev.class_type[0] = '\0';
        ev.has_class_type = 0;
    }

    if (has_feature) {
        ev.feature = feature;
        ev.has_feature = 1;
    } else {
        ev.feature = 0;
        ev.has_feature = 0;
    }

    return ev;
}

// JSONLEvent 필요 buffer 크기 계산 (timestamp ms 기준)
size_t calculate_jsonl_event_size(const JSONLEvent *ev)
{
    size_t len_event = strlen(ev->event_type);
    size_t len_class = ev->has_class_type
                           ? strlen(ev->class_type)
                           : strlen("null");

    // ms단위 타임스탬프 길이
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long ms = (long long)tv.tv_sec * 1000LL + tv.tv_usec / 1000;
    char tsbuf[32];
    int len_ts = snprintf(tsbuf, sizeof(tsbuf), "%lld", ms);

    int len_feat = ev->has_feature
                       ? snprintf(NULL, 0, "%d", ev->feature)
                       : strlen("null");

    const size_t overhead =
        strlen("{\"event_type\":\"\",") +   // {"event_type":"",
        strlen("\"timestamp\":,") +         // "timestamp":
        strlen("\"has_file\":,") +          // "has_file":
        strlen("\"data\":{") +              // "data":{
        strlen("\"class_type\":,") +        // "class_type":
        strlen("\"feature\":") +            // "feature":
        strlen("}}\n");                     // }}\n

    return len_event + len_class + (size_t)len_ts + (size_t)len_feat + overhead + 1; // '\0'
}

// jsonl 백업
void backup_jsonl(const char *line)
{
    mkdir(JSONL_DIR, 0755);

    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm *tm_info = localtime(&tv.tv_sec);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y%m%d_%H%M%S", tm_info);

    char outpath[512];
    snprintf(outpath, sizeof(outpath), "%s/%s_%06ld.jsonl",
             JSONL_DIR, ts, (long)tv.tv_usec);

    FILE *fp = fopen(outpath, "a");
    if (!fp) {
        perror("fopen JSONL backup");
        return;
    }

    size_t len = strlen(line);
    if (len > 0 && line[len - 1] == '\n')
        fputs(line, fp);
    else
        fprintf(fp, "%s\n", line);

    fclose(fp);
}

// JSONLEvent로 jsonl 이벤트 스트링 생성 (timestamp는 ms)
char *make_jsonl_event(const JSONLEvent *ev)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long ms = (long long)tv.tv_sec * 1000LL + tv.tv_usec / 1000;

    // size_t needed = calculate_jsonl_event_size(ev);
    size_t needed = 256;
    char *buf = (char *)malloc(needed);
    if (!buf)
        return NULL;

    int off = 0;
    off += snprintf(buf + off, needed - off,
                    "{\"event_type\":\"%s\","
                    "\"timestamp\":%lld,"
                    "\"has_file\":%d,"
                    "\"data\":{",
                    ev->event_type, ms, ev->has_file);

    // class_type
    if (ev->has_class_type) {
        off += snprintf(buf + off, needed - off,
                        "\"class_type\":\"%s\",", ev->class_type);
    } else {
        off += snprintf(buf + off, needed - off,
                        "\"class_type\":null,");
    }

    // feature
    if (ev->has_feature) {
        off += snprintf(buf + off, needed - off,
                        "\"feature\":%d", ev->feature);
    } else {
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