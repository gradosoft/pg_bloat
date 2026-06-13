#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  // for getpass()
#include "pg_bloat.h"

#define MAX_CMD_LEN 2048

void print_help(const char *program_name) {
    printf("Usage: %s [OPTIONS] [DBNAME]\n", program_name);
    printf("\n");
    printf("Options:\n");
    printf("  -h HOST       PostgreSQL server host (default: localhost)\n");
    printf("  -p PORT       PostgreSQL server port (default: 5432)\n");
    printf("  -U USER       PostgreSQL user (default: current user)\n");
    printf("  -W            Force password prompt (reads from PGPASSWORD env or stdin)\n");
    printf("  -d DBNAME     Database name to connect to (required)\n");
    printf("  --format FMT  Output format: 'json' or '1' for JSON (default: human-readable)\n");
    printf("  --help        Show this help message\n");
    printf("\n");
    printf("Output:\n");
    printf("  By default shows only tables with >5%% dead tuples.\n");
    printf("  Use --format json to get all data including tables with low bloat.\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s -h localhost -p 5432 -U postgres -d mydb\n", program_name);
    printf("  PGPASSWORD=secret %s -h localhost -U postgres -d mydb\n", program_name);
    printf("  %s -h localhost -U postgres -d mydb --format json\n", program_name);
}

int parse_arguments(int argc, char *argv[], ConnectionConfig *cfg) {
    // default values
    strcpy(cfg->host, "localhost");
    strcpy(cfg->port, "5432");
    strcpy(cfg->user, "");
    strcpy(cfg->password, "");
    strcpy(cfg->dbname, "");
    cfg->format_json = 0;  // human format for default
    
    int i = 1;
    
    // flags parse
    for (; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            if (i + 1 < argc) {
                strncpy(cfg->host, argv[++i], sizeof(cfg->host) - 1);
            } else {
                fprintf(stderr, "Error: -h requires an argument\n");
                return 0;
            }
        }
        else if (strcmp(argv[i], "-p") == 0) {
            if (i + 1 < argc) {
                strncpy(cfg->port, argv[++i], sizeof(cfg->port) - 1);
            } else {
                fprintf(stderr, "Error: -p requires an argument\n");
                return 0;
            }
        }
        else if (strcmp(argv[i], "-U") == 0) {
            if (i + 1 < argc) {
                strncpy(cfg->user, argv[++i], sizeof(cfg->user) - 1);
            } else {
                fprintf(stderr, "Error: -U requires an argument\n");
                return 0;
            }
        }
        else if (strcmp(argv[i], "-W") == 0) {
            //read from PGPASSWORD
            char *env_password = getenv("");
            if (env_password != NULL) {
                strncpy(cfg->password, env_password, sizeof(cfg->password) - 1);
            } else {
                // if PGPASSWORD not found - password promt
                // use getpass() — without echo
                char* pass = getpass("Password: ");
                if (pass != NULL) {
                    // remove new line symbol
                    strncpy(cfg->password, pass, sizeof(cfg->password) - 1);
                    cfg->password[sizeof(cfg->password) - 1] = '\0';
                }
            }
        }
        else if (strcmp(argv[i], "-d") == 0) {
            if (i + 1 < argc) {
                strncpy(cfg->dbname, argv[++i], sizeof(cfg->dbname) - 1);
            } else {
                fprintf(stderr, "Error: -d requires an argument\n");
                return 0;
            }
        }

        else if (strcmp(argv[i], "--format") == 0) {
            if (i + 1 < argc) {
                i++;
                if (strcmp(argv[i], "json") == 0 || strcmp(argv[i], "1") == 0) {
                    cfg->format_json = 1;
                }
                else {
                    fprintf(stderr, "Error: --format requires 'json' or '1'\n");
                    return 0;
                }
            }
            else {
                fprintf(stderr, "Error: --format requires an argument\n");
                return 0;
            }
        }

        else if (strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            exit(0);
        }
        else if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_help(argv[0]);
            return 0;
        }


    }
    
    // if dbname not found - use user name as dbname
    if (strlen(cfg->dbname) == 0 && strlen(cfg->user) > 0) {
        strcpy(cfg->dbname, cfg->user);
    }
    
    // check PGPASSWORD (if password not found)
    if (strlen(cfg->password) == 0) {
        char *env_password = getenv("PGPASSWORD");
        if (env_password != NULL) {
            strncpy(cfg->password, env_password, sizeof(cfg->password) - 1);
        }
    }
    
    return 1;
}

//Connect DB
PGconn* connect_database(const ConnectionConfig* cfg) {
    char conninfo[2048];

    int written = snprintf(conninfo, sizeof(conninfo),
        "host=%s port=%s user=%s password=%s dbname=%s",
        cfg->host, cfg->port, cfg->user, cfg->password, cfg->dbname);

    if (written < 0 || (size_t)written >= sizeof(conninfo)) {
        return NULL;
    }

    PGconn* conn = PQconnectdb(conninfo);

    if (PQstatus(conn) != CONNECTION_OK) {
        PQfinish(conn);
        return NULL;
    }

    return conn;
}

void show_database_info(PGconn *conn) {
    const char *query = 
        "SELECT "
        "  version(),"
        "  current_database(),"
        "  current_user";
    
    PGresult *res = PQexec(conn, query);
    
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        printf("\nDatabase Info:\n");
        printf("  PostgreSQL: %s\n", PQgetvalue(res, 0, 0));
        printf("  Database:   %s\n", PQgetvalue(res, 0, 1));
        printf("  User:       %s\n", PQgetvalue(res, 0, 2));
    }
    
    PQclear(res);
}

//Analyze bloat
void analyze_bloat(PGconn *conn, int format_json) {
    const char *query = 
        "SELECT "
        "  schemaname,"
        "  relname AS tablename,"
        "  pg_size_pretty(pg_total_relation_size(schemaname||'.'||relname)) AS total_size,"
        "  n_live_tup AS live_tuples,"
        "  n_dead_tup AS dead_tuples,"
        "  round(100.0 * n_dead_tup / NULLIF(n_live_tup + n_dead_tup, 0), 2) AS dead_percent,"
        "  pg_size_pretty( "
        "    GREATEST(0, pg_relation_size(schemaname||'.'||relname) - (n_live_tup * 40)) "
        "  ) AS bloat_size,"
        "  TO_CHAR(COALESCE(last_autovacuum, last_vacuum), 'YYYY-MM-DD HH24:MI:SS') AS last_vacuum_time"
        " FROM pg_stat_user_tables "
        " WHERE (n_live_tup + n_dead_tup) > 0 "
        " ORDER BY dead_percent DESC";
    
    PGresult *res = PQexec(conn, query);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        if (format_json) {
            fprintf(stderr, "{\"status\":\"error\",\"message\":\"%s\"}\n", 
                    PQerrorMessage(conn));
        } else {
            fprintf(stderr, "Query failed: %s\n", PQerrorMessage(conn));
        }
        PQclear(res);
        return;
    }
    
    int rows = PQntuples(res);
    int cols = PQnfields(res);
    
    if (format_json) {
        // JSON with filter as dead_percent >= 5%
        int filtered_count = 0;
        for (int i = 0; i < rows; i++) {
            double dead_pct = atof(PQgetvalue(res, i, 5));
            if (dead_pct >= 5.0) filtered_count++;
        }
        
        printf("{\n");
        printf("  \"status\": \"success\",\n");
        printf("  \"min_dead_percent\": 5,\n");
        printf("  \"tables\": [\n");
        
        int first = 1;
        for (int i = 0; i < rows; i++) {
            double dead_pct = atof(PQgetvalue(res, i, 5));
            if (dead_pct < 5.0) continue;
            
            if (!first) printf(",\n");
            first = 0;
            
            printf("    {\n");
            for (int j = 0; j < cols; j++) {
                char *colname = PQfname(res, j);
                char *value = PQgetvalue(res, i, j);
                
                printf("      \"%s\": ", colname);
                
                // Check types
                if (j == 3 || j == 4) {  // live_tuples, dead_tuples
                    printf("%s", value);
                } else if (j == 5) {  // dead_percent
                    printf("%s", value);
                } else if (j == 7 && strcmp(value, "") == 0) {  // last_vacuum_time NULL
                    printf("null");
                } else {
                    // escape quotes
                    printf("\"");
                    for (char *c = value; *c; c++) {
                        if (*c == '"' || *c == '\\') putchar('\\');
                        putchar(*c);
                    }
                    printf("\"");
                }
                
                if (j < cols - 1) printf(",");
                printf("\n");
            }
            printf("    }");
        }
        
        printf("\n  ],\n");
        printf("  \"total_tables\": %d\n", filtered_count);
        printf("}\n");
    } else {
        // human format
        if (rows == 0) {
            printf("\nNo tables found.\n");
            PQclear(res);
            return;
        }
        
        // tables count with dead% >= 5%
        int significant_count = 0;
        for (int i = 0; i < rows; i++) {
            double dead_pct = atof(PQgetvalue(res, i, 5));
            if (dead_pct >= 5.0) significant_count++;
        }
        
        if (significant_count == 0) {
            printf("\nNo significant bloat detected (dead tuples < 5%%).\n");
            PQclear(res);
            return;
        }
        
        printf("\n");
        printf("Tables with bloat (>5%% dead tuples) - showing %d of %d total tables:\n", 
               significant_count, rows);
        printf("\n");
        printf("%-15s %-30s %-10s %-15s %-15s %-6s %-10s %-19s\n",
            "Schema", "Table", "Size", "Live", "Dead", "Dead%", "Bloat", "Last Vacuum");
        printf("%s\n", "--------------------------------------------------------------------------------------------------------------------");
        for (int i = 0; i < rows; i++) {
            double dead_pct = atof(PQgetvalue(res, i, 5));
            if (dead_pct < 5.0) continue;
            
            char *last_vacuum = PQgetvalue(res, i, 7);
            if (strlen(last_vacuum) == 0) {
                last_vacuum = "never";
            }
            
            printf("%-15s %-30s %-10s %-15s %-15s %-6.2f %-10s %-19s\n",
                   PQgetvalue(res, i, 0),  // schemaname
                   PQgetvalue(res, i, 1),  // tablename
                   PQgetvalue(res, i, 2),  // total_size
                   PQgetvalue(res, i, 3),  // live_tuples
                   PQgetvalue(res, i, 4),  // dead_tuples
                   dead_pct,
                   PQgetvalue(res, i, 6),  // bloat_size
                   last_vacuum);            // last_vacuum_time
        }
        
        printf("\n");
        printf("Note: Only tables with >5%% dead tuples are shown.\n");
    }
    
    PQclear(res);
}




