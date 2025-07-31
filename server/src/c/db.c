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

int store_email_config(const char *msg)
{
    if (mysql_query(g_conn, "TRUNCATE TABLE smartfarm.email_config;")) {
        fprintf(stderr,
                "[EmailConfig] TRUNCATE failed: %s\n",
                mysql_error(g_conn));
        return -1;
    }
    
    // 1) msg 복사 및 분리
    char *buf = strdup(msg);
    if (!buf) {
        fprintf(stderr, "[EmailConfig] strdup failed\n");
        return -1;
    }
    char *sep = strchr(buf, '|');
    if (!sep) {
        fprintf(stderr, "[EmailConfig] invalid format, no '|'\n");
        free(buf);
        return -1;
    }
    *sep = '\0';
    const char *email    = buf;
    const char *password = sep + 1;

    // 2) Prepared Statement 초기화
    MYSQL_STMT *stmt = mysql_stmt_init(g_conn);
    if (!stmt) {
        fprintf(stderr, "[EmailConfig] mysql_stmt_init failed\n");
        free(buf);
        return -1;
    }
    const char *sql =
        "INSERT INTO smartfarm.email_config (email, password) VALUES (?, ?);";
    if (mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql))) {
        fprintf(stderr,
                "[EmailConfig] prepare failed: %s\n",
                mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        free(buf);
        return -1;
    }

    // 3) 파라미터 바인딩
    MYSQL_BIND bind[2];
    memset(bind, 0, sizeof(bind));
    unsigned long email_len    = (unsigned long)strlen(email);
    unsigned long password_len = (unsigned long)strlen(password);

    bind[0].buffer_type   = MYSQL_TYPE_STRING;
    bind[0].buffer        = (char *)email;
    bind[0].buffer_length = email_len;
    bind[0].length        = &email_len;

    bind[1].buffer_type   = MYSQL_TYPE_STRING;
    bind[1].buffer        = (char *)password;
    bind[1].buffer_length = password_len;
    bind[1].length        = &password_len;

    if (mysql_stmt_bind_param(stmt, bind)) {
        fprintf(stderr,
                "[EmailConfig] bind_param failed: %s\n",
                mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        free(buf);
        return -1;
    }

    // 4) 실행
    if (mysql_stmt_execute(stmt)) {
        fprintf(stderr,
                "[EmailConfig] execute failed: %s\n",
                mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        free(buf);
        return -1;
    }

    // 5) 정리
    mysql_stmt_close(stmt);
    free(buf);
    return 0;
}

int send_config_email(SSL *sock_fd)
{
    // 1) SQL 실행
    const char *sql = 
        "SELECT email "
        "FROM smartfarm.email_config "
        "LIMIT 1;";
    if (mysql_query(g_conn, sql)) {
        fprintf(stderr,
                "[EmailConfig] SELECT error: %s\n",
                mysql_error(g_conn));
        return -1;
    }

    // 2) 결과 저장
    MYSQL_RES *res = mysql_store_result(g_conn);
    if (!res) {
        fprintf(stderr,
                "[EmailConfig] store_result error: %s\n",
                mysql_error(g_conn));
        return -1;
    }

    // 3) 행이 있는지 확인
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row || !row[0]) {
        // 저장된 이메일이 없음
        mysql_free_result(res);
        // 빈 문자열로라도 보내 주거나 에러로 처리
        const char *empty = "NO EMAIL";

        sendData(sock_fd, empty);
        return 0;
    }

    // 4) 이메일 전송
    const char *email = row[0];
    sendData(sock_fd, email);

    // 5) 정리
    mysql_free_result(res);
    return 0;
}

MailUser check_email_pass(void) {
    MailUser user;
    user.email[0] = '\0';
    user.pass[0]  = '\0';

    // 1) SQL 실행 ("SELECT email, password")
    const char *sql =
        "SELECT email, password "
        "FROM smartfarm.email_config "
        "LIMIT 1;";

    if (mysql_query(g_conn, sql)) {
        fprintf(stderr,
                "[EmailConfig] SELECT error: %s\n",
                mysql_error(g_conn));
        return user;
    }

    // 2) 결과 저장
    MYSQL_RES *res = mysql_store_result(g_conn);
    if (!res) {
        fprintf(stderr,
                "[EmailConfig] store_result error: %s\n",
                mysql_error(g_conn));
        return user;
    }

    // 3) 행이 있는지 확인
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row || !row[0] || !row[1]) {
        mysql_free_result(res);
        return user;
    }

    // 4) 값을 struct에 복사 (strncpy로 안전하게)
    strncpy(user.email, row[0], sizeof(user.email)-1);
    user.email[sizeof(user.email)-1] = '\0';
    strncpy(user.pass,  row[1], sizeof(user.pass)-1);
    user.pass[sizeof(user.pass)-1]   = '\0';

    mysql_free_result(res);
    return user;
}

void query_and_send_bar_data(SSL *sock_fd, const char *payload)
{
    // 1. 파싱
    char event_type[128], date[32];
    if (sscanf(payload, "%127[^|]|%31[^|]", event_type, date) != 2) {
        return;
    }

    // 2. 이스케이프
    char evt_esc[128], date_esc[32];
    mysql_real_escape_string(g_conn, evt_esc, event_type, strlen(event_type));
    mysql_real_escape_string(g_conn, date_esc, date, strlen(date));

    // 3. 쿼리 실행
    char query[512];
    snprintf(query, sizeof(query),
        "SELECT HOUR(created_at), COUNT(*) "
        "FROM smartfarm.events "
        "WHERE event_type = '%s' AND DATE(created_at) = '%s' "
        "GROUP BY HOUR(created_at);",
        evt_esc, date_esc);

    if (mysql_query(g_conn, query)) {
        return;
    }

    // 4. 결과 수신 및 24칸 배열 초기화
    int hourly[24] = {0};
    MYSQL_RES *res = mysql_store_result(g_conn);
    MYSQL_ROW row;

    while ((row = mysql_fetch_row(res))) {
        int hour = atoi(row[0]);
        int count = atoi(row[1]);
        if (hour >= 0 && hour < 24) {
            hourly[hour] = count;
        }
    }
    mysql_free_result(res);

    // 5. 값이 0이 아닌 시간만 전송
    for (int i = 0; i < 24; ++i) {
        if (hourly[i] == 0) continue;

        char line[64];
        snprintf(line, sizeof(line), "%d|%d", i, hourly[i]);
        sendData(sock_fd, line);
    }

    sendData(sock_fd, "END");
}

void query_and_send(SSL *sock_fd, const char *payload)
{
    // 1) payload 파싱
    //    strawberries: 4개 필드
    //    intrusion   : 5개 필드 (시작시간, 종료시간)
    char event_type[128], f1[128], f2[128];
    int  limit, offset;
    int  n = sscanf(payload,
        "%127[^|]|%d|%d|%127[^|]|%127[^|]",
        event_type, &limit, &offset, f1, f2);

    // 2) event_type 이스케이프
    char evt_esc[128];
    mysql_real_escape_string(g_conn,
                             evt_esc,
                             event_type,
                             strlen(event_type));

    // 3) 딸기일 때 class_type 필터 절
    char class_clause[256]  = "";
    if (strcmp(evt_esc, "strawberry_detected") == 0 && n >= 4) {
        char cls_esc[128];
        mysql_real_escape_string(g_conn, cls_esc, f1, strlen(f1));
        snprintf(class_clause, sizeof(class_clause),
                 " AND class_type = '%s'", cls_esc);
    }

    // 4) 침입일 때 시간 범위 필터 절
    char time_clause[256]   = "";
    if (strcmp(evt_esc, "intrusion_detected") == 0 && n >= 5) {
        char start_esc[64], end_esc[64];
        mysql_real_escape_string(g_conn,
                                 start_esc, f1, strlen(f1));
        mysql_real_escape_string(g_conn,
                                 end_esc,   f2, strlen(f2));
        snprintf(time_clause, sizeof(time_clause),
            " AND created_at BETWEEN '%s' AND '%s'",
            start_esc, end_esc);
    }

    // 5) 페이지네이션: LIMIT+1 로 다음 페이지 유무 판단
    int fetchSize = limit + 1;

    // 6) 최종 SQL 조립
    char query[1024];
    int ql = snprintf(query, sizeof(query),
        "SELECT created_at, class_type, image_url "
        "FROM smartfarm.events "
        "WHERE event_type = '%s'%s%s "
        "ORDER BY created_at DESC "
        "LIMIT %d OFFSET %d;",
        evt_esc,
        class_clause,
        time_clause,
        fetchSize, offset
    );
    if (ql < 0 || ql >= (int)sizeof(query)) {
        fprintf(stderr, "[Paging] SQL snprintf error\n");
        return;
    }

    // 7) 실행 및 전송 로직은 기존과 동일
    if (mysql_query(g_conn, query)) {
        fprintf(stderr, "쿼리 실패: %s\n", mysql_error(g_conn));
        return;
    }
    MYSQL_RES *res = mysql_store_result(g_conn);
    int rows    = mysql_num_rows(res);
    bool hasNext= (rows == fetchSize);
    int sendRows = hasNext ? limit : rows;

    MYSQL_ROW row;
    for (int i = 0; i < sendRows; ++i) {
        row = mysql_fetch_row(res);
        char line[1024];
        // created_at|class_type
        snprintf(line, sizeof(line), "%s|%s\n",
                 row[0] ? row[0] : "",
                 row[1] ? row[1] : "");
        sendData(sock_fd, line);
        imageSend(sock_fd, row[2]);
    }
    mysql_free_result(res);

    // 페이지네이션 플래그
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
