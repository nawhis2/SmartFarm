#ifndef DB_H
#define DB_H

#include <mysql/mysql.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
extern MYSQL *g_conn;

void init_mysql(const char *host,
                const char *user,
                const char *pass,
                const char *db,
                unsigned int port);
void close_mysql(void);
void query_and_send(SSL*, const char *);
#endif