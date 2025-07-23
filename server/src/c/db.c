#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include "sendData.h"
#include "userclientsend.h"

MYSQL *g_conn = NULL;

void init_mysql(const char *host,
                const char *user,
                const char *pass,
                const char *db,
                unsigned int port)
{
    g_conn = mysql_init(NULL);
    if (!g_conn) {
        fprintf(stderr, "mysql_init() failed\n");
        exit(EXIT_FAILURE);
    }
    if (!mysql_real_connect(g_conn,
                            host, user, pass,
                            db, port, NULL, 0)) {
        fprintf(stderr, "MySQL connect error: %s\n",
                mysql_error(g_conn));
        exit(EXIT_FAILURE);
    }
}

void query_and_send(SSL* sock_fd, const char *payload) {
    char event_type[128];
    int limit, offset;
    // payload 파싱
    sscanf(payload, "%127[^|]|%d|%d",
           event_type, &limit, &offset);

    // event_type 이스케이프
    char evt_esc[128];
    mysql_real_escape_string(g_conn,
                             evt_esc,
                             event_type,
                             strlen(event_type));

    // LIMIT+1: 실제 클라이언트에 보여줄 limit보다 1개 더 읽어서
    // 다음 페이지 존재 여부를 판단
    int fetchSize = limit + 1;
    char query[512];
    snprintf(query, sizeof(query),
      "SELECT created_at, class_type, image_url "
      "FROM smartfarm.events "
      "WHERE event_type = '%s' "
      "ORDER BY created_at DESC "
      "LIMIT %d OFFSET %d;",
      evt_esc, fetchSize, offset);

    if (mysql_query(g_conn, query)) {
        fprintf(stderr, "쿼리 실패: %s\n", mysql_error(g_conn));
        return;
    }

    MYSQL_RES *res = mysql_store_result(g_conn);
    int rows = mysql_num_rows(res);
    bool hasNext = (rows == fetchSize);
    int sendRows = hasNext ? limit : rows;

    // 실제로 보여줄 부분만 보내고
    MYSQL_ROW row;
    for (int i = 0; i < sendRows; ++i) {
        row = mysql_fetch_row(res);
        char line[1024];
        snprintf(line, sizeof(line), "%s|%s\n", row[0], row[1]);
        sendData(sock_fd, line);
        imageSend(sock_fd, row[2]);
    }
    mysql_free_result(res);

    // 마지막으로 플래그만 보내면 클라이언트가 바로 알 수 있음
    sendData(sock_fd, hasNext ? "HAS_NEXT" : "NO_MORE");
}

// 하루치 통계 전송 (파이 그래프용: YYYY-MM-DD|class_type|cnt)
void query_and_send_daily(SSL *sock_fd, const char *event_type) {
    // event_type 이스케이프
    char evt_esc[128];
    mysql_real_escape_string(g_conn,
                             evt_esc,
                             event_type,
                             strlen(event_type));

    // 7일치 통계를 한 번에 서브쿼리로 처리
    char sql[1024];
    snprintf(sql, sizeof(sql),
      "SELECT DATE_FORMAT(created_at,'%%Y-%%m-%%d') AS day, "
      "       class_type, COUNT(*) AS cnt "
      "FROM smartfarm.events "
      "WHERE event_type = '%s' "
      "  AND created_at BETWEEN "
      "      (SELECT MAX(created_at) FROM smartfarm.events WHERE event_type = '%s') - INTERVAL 1 DAY "
      "  AND (SELECT MAX(created_at) FROM smartfarm.events WHERE event_type = '%s') "
      "GROUP BY class_type ",
      evt_esc, evt_esc, evt_esc);

    if (mysql_query(g_conn, sql)) {
        fprintf(stderr, "[Weekly] MySQL error: %s\n", mysql_error(g_conn));
        return;
    }

    MYSQL_RES *res = mysql_store_result(g_conn);
    MYSQL_ROW row;

    while ((row = mysql_fetch_row(res))) {
        // row[0]=day, row[1]=class_type, row[2]=cnt
        char line[256];
        snprintf(line, sizeof(line),
                          "%s|%s|%s\n",
                          row[0], row[1], row[2]);
        sendData(sock_fd, line);
    }
    mysql_free_result(res);
    sendData(sock_fd, "END");
}

// 일주일치 통계 전송 (그래프용: YYYY-MM-DD|class_type|cnt)
void query_and_send_weekly(SSL *sock_fd, const char *event_type) {
    // event_type 이스케이프
    char evt_esc[128];
    mysql_real_escape_string(g_conn,
                             evt_esc,
                             event_type,
                             strlen(event_type));

    // 7일치 통계를 한 번에 서브쿼리로 처리
    char sql[1024];
    snprintf(sql, sizeof(sql),
      "SELECT DATE_FORMAT(created_at,'%%Y-%%m-%%d') AS day, "
      "       class_type, COUNT(*) AS cnt "
      "FROM smartfarm.events "
      "WHERE event_type = '%s' "
      "  AND created_at BETWEEN "
      "      (SELECT MAX(created_at) FROM smartfarm.events WHERE event_type = '%s') - INTERVAL 7 DAY "
      "  AND (SELECT MAX(created_at) FROM smartfarm.events WHERE event_type = '%s') "
      "GROUP BY day, class_type "
      "ORDER BY day ASC, class_type ASC;",
      evt_esc, evt_esc, evt_esc);

    if (mysql_query(g_conn, sql)) {
        fprintf(stderr, "[Weekly] MySQL error: %s\n", mysql_error(g_conn));
        return;
    }

    MYSQL_RES *res = mysql_store_result(g_conn);
    MYSQL_ROW row;

    while ((row = mysql_fetch_row(res))) {
        // row[0]=day, row[1]=class_type, row[2]=cnt
        char line[256];
        snprintf(line, sizeof(line),
                          "%s|%s|%s\n",
                          row[0], row[1], row[2]);
        sendData(sock_fd, line);
    }
    mysql_free_result(res);
    sendData(sock_fd, "END");
}

void close_mysql(void)
{
    if (g_conn) {
        mysql_close(g_conn);
        g_conn = NULL;
    }
}