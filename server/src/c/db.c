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
    if (!g_conn)
    {
        fprintf(stderr, "mysql_init() failed\n");
        exit(EXIT_FAILURE);
    }
    if (!mysql_real_connect(g_conn,
                            host, user, pass,
                            db, port, NULL, 0))
    {
        fprintf(stderr, "MySQL connect error: %s\n",
                mysql_error(g_conn));
        exit(EXIT_FAILURE);
    }
}

void close_mysql(void)
{
    if (g_conn)
    {
        mysql_close(g_conn);
        g_conn = NULL;
    }
}

void query_and_send(SSL *sock_fd, const char *payload)
{
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

    if (mysql_query(g_conn, query))
    {
        fprintf(stderr, "쿼리 실패: %s\n", mysql_error(g_conn));
        return;
    }

    MYSQL_RES *res = mysql_store_result(g_conn);
    int rows = mysql_num_rows(res);
    bool hasNext = (rows == fetchSize);
    int sendRows = hasNext ? limit : rows;

    // 실제로 보여줄 부분만 보내고
    MYSQL_ROW row;
    for (int i = 0; i < sendRows; ++i)
    {
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
void query_and_send_daily(SSL *sock_fd, const char *event_type)
{
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

    if (mysql_query(g_conn, sql))
    {
        fprintf(stderr, "[Weekly] MySQL error: %s\n", mysql_error(g_conn));
        return;
    }

    MYSQL_RES *res = mysql_store_result(g_conn);
    MYSQL_ROW row;

    while ((row = mysql_fetch_row(res)))
    {
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
void query_and_send_weekly(SSL *sock_fd, const char *event_type)
{
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

    if (mysql_query(g_conn, sql))
    {
        fprintf(stderr, "[Weekly] MySQL error: %s\n", mysql_error(g_conn));
        return;
    }

    MYSQL_RES *res = mysql_store_result(g_conn);
    MYSQL_ROW row;

    while ((row = mysql_fetch_row(res)))
    {
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

static void query_and_send_map(SSL *ssl)
{
    const char *map_sql =
        "SELECT ts_epoch, center_x, center_y, right_x, right_y, left_x, left_y, created_at "
        "FROM smartfarm.map "
        "ORDER BY ts_epoch ASC;";
    if (mysql_query(g_conn, map_sql))
    {
        fprintf(stderr, "[Map] MySQL error: %s\n", mysql_error(g_conn));
        return;
    }

    MYSQL_RES *res = mysql_store_result(g_conn);
    MYSQL_ROW row;

    while ((row = mysql_fetch_row(res)))
    {
        // row[0]=ts_epoch, [1]=center_x, [2]=center_y, [3]=right_x, [4]=right_y, [5]=left_x, [6]=left_y, [7]=created_at
        char line[256];
        snprintf(line, sizeof(line),
                 "%s|%s|%s|%s|%s|%s|%s|%s\n",
                 row[0] ? row[0] : "",
                 row[1] ? row[1] : "",
                 row[2] ? row[2] : "",
                 row[3] ? row[3] : "",
                 row[4] ? row[4] : "",
                 row[5] ? row[5] : "",
                 row[6] ? row[6] : "",
                 row[7] ? row[7] : "");
        sendData(ssl, line);
    }
    mysql_free_result(res);
}

// 2) 첫·마지막 strawberry 이벤트 바운드 전송
static void query_and_send_bounds(SSL *ssl)
{
    // 1) SQL 정의 (event_type 고정)
    static const char *sql =
        "SELECT e.ts_epoch, e.class_type, e.feature "
        "FROM smartfarm.events AS e "
        "WHERE e.event_type = 'strawberry_detected' "
        "  AND e.ts_epoch BETWEEN "
        "    (SELECT MIN(ts_epoch) FROM smartfarm.map) "
        "  AND "
        "    (SELECT MAX(ts_epoch) FROM smartfarm.map) "
        "ORDER BY e.ts_epoch ASC;";

    // 2) 쿼리 실행
    if (mysql_query(g_conn, sql))
    {
        fprintf(stderr, "[Bounds] MySQL error: %s\n",
                mysql_error(g_conn));
        return;
    }

    // 3) 결과 전송
    MYSQL_RES *res = mysql_store_result(g_conn);
    if (!res)
    {
        fprintf(stderr, "[Bounds] mysql_store_result failed\n");
        return;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)))
    {
        // row[0]=ts_epoch, row[1]=class_type, row[2]=feature
        char line[128];
        snprintf(line, sizeof(line),
                 "%s|%s|%s\n",
                 row[0] ? row[0] : "",
                 row[1] ? row[1] : "",
                 row[2] ? row[2] : "");
        sendData(ssl, line);
    }
    mysql_free_result(res);
}

// 3) 위 두 함수를 연달아 호출하는 래퍼
void query_and_send_map_and_bounds(SSL *ssl)
{
    query_and_send_map(ssl);
    // 중간 체크
    sendData(ssl, "SPLIT\n");

    query_and_send_bounds(ssl);
    // 끝마커
    sendData(ssl, "END\n");
}

void clear_map_data()
{
    // 1) 테이블 비우기 (빠른 완전 삭제)
    const char *sql = "TRUNCATE TABLE smartfarm.map;";
    if (mysql_query(g_conn, sql))
    {
        fprintf(stderr, "[ClearMap] MySQL error: %s\n", mysql_error(g_conn));
        return;
    }
}
