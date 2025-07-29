#ifndef DB_H
#define DB_H

#include <mysql/mysql.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

extern MYSQL *g_conn;

typedef struct {
    char email[32];
    char pass[32];
} MailUser;

void init_mysql(const char *host,
                const char *user,
                const char *pass,
                const char *db,
                unsigned int port);
void close_mysql(void);

int store_email_config(const char*);
int send_config_email(SSL*);
MailUser check_email_pass();

void query_and_send(SSL*, const char*);
void query_and_send_daily(SSL*, const char*);
void query_and_send_weekly(SSL*, const char*);

void query_and_send_map_and_bounds(SSL*);
void clear_map_data();

#endif