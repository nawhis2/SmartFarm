#include "db.h"
#include <stdio.h>
#include <stdlib.h>

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

void close_mysql(void)
{
    if (g_conn) {
        mysql_close(g_conn);
        g_conn = NULL;
    }
}