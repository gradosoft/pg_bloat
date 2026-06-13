#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pg_bloat.h"


int main(int argc, char* argv[]) {
    ConnectionConfig cfg;

    // Init
    memset(&cfg, 0, sizeof(cfg));
    strcpy(cfg.host, "localhost");
    strcpy(cfg.port, "5432");
    cfg.format_json = 0;

    // arguments parser
    if (!parse_arguments(argc, argv, &cfg)) {
        if (cfg.format_json) {
            fprintf(stderr, "{\"status\":\"error\",\"message\":\"Invalid arguments\"}\n");
        }
        else {
            print_help(argv[0]);
        }
        return 1;
    }

    // check user
    if (strlen(cfg.user) == 0) {
        if (cfg.format_json) {
            fprintf(stderr, "{\"status\":\"error\",\"message\":\"User is required (use -U)\"}\n");
        }
        else {
            fprintf(stderr, "Error: User is required (use -U)\n");
            print_help(argv[0]);
        }
        return 1;
    }

    // Connection
    PGconn* conn = connect_database(&cfg);
    if (!conn) {
        if (cfg.format_json) {
            fprintf(stderr, "{\"status\":\"error\",\"message\":\"Connection failed: invalid host, port, or credentials\"}\n");
        }
        else {
            fprintf(stderr, "Failed to connect to database.\n");
        }
        return 1;
    }

    // If human format - show DB info
    if (!cfg.format_json) {
        printf("Connected to %s:%s/%s as %s\n",
            cfg.host, cfg.port, cfg.dbname, cfg.user);
        show_database_info(conn);
    }

    // Bloat analyze (if JSON format - show json only)
    analyze_bloat(conn, cfg.format_json);

    PQfinish(conn);
    return 0;
}