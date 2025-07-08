#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include "sendData.h"

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

void query_and_send(SSL* sock_fd, const char *event_type) {
    char query[256];
    snprintf(query, sizeof(query),
             "SELECT id, ts_epoch, class_type, image_url, created_at FROM smartfarm.events WHERE event_type = '%s'",
             event_type);

    if (mysql_query(g_conn, query)) {
        fprintf(stderr, "쿼리 실패: %s\n", mysql_error(g_conn));
        return;
    }

    MYSQL_RES *result = mysql_store_result(g_conn);
    MYSQL_ROW row;

    while ((row = mysql_fetch_row(result))) {
        char line[1024];
        snprintf(line, sizeof(line), "%s|%s|%s|%s|%s\n", row[0], row[1], row[2], row[3], row[4]);
        sendData(sock_fd, line);
    }

    mysql_free_result(result);
}

void close_mysql(void)
{
    if (g_conn) {
        mysql_close(g_conn);
        g_conn = NULL;
    }
}