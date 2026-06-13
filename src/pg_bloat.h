#ifndef PG_BLOAT_H
#define PG_BLOAT_H

#include <libpq-fe.h>

typedef struct {
    char host[256];
    char port[8];
    char user[256];
    char password[256];
    char dbname[256];
    int format_json;
} ConnectionConfig;

//Arguments parser
int parse_arguments(int argc, char *argv[], ConnectionConfig *cfg);

//Connect to DB
PGconn* connect_database(const ConnectionConfig *cfg);

//Analyze bloat
void analyze_bloat(PGconn* conn, int format_json);

//Show DB info in human format
void show_database_info(PGconn *conn);

//If empty or wrong arguments (or --help arguments)
void print_help(const char *program_name);

#endif // PG_BLOAT_H
