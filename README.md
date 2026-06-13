# pg_bloat - PostgreSQL Table Bloat Analysis Tool

pg_bloat is a command-line utility for analyzing table bloat in PostgreSQL databases. It helps you identify tables with excessive dead tuples and provides actionable insights for database maintenance.

## Features

- Quick bloat detection - Identifies tables with >5% dead tuples
- Human-readable and JSON output - Both console-friendly and machine-readable formats
- Secure password handling - Supports PGPASSWORD environment variable or interactive prompt
- PostgreSQL 9.2+ compatible - Works with all modern PostgreSQL versions
- Lightweight - Single binary with no external dependencies beyond libpq

## Installation

### Prerequisites

- PostgreSQL client libraries (libpq)
- CMake 3.10+
- C compiler (GCC or Clang)

### Ubuntu / Debian / WSL

sudo apt update
sudo apt install -y build-essential cmake libpq-dev

### macOS

brew install postgresql cmake

### Windows (WSL2 recommended)

For Windows, using WSL2 with Ubuntu is the recommended approach:

1. Install WSL2 and Ubuntu 22.04/24.04
2. Follow the Ubuntu installation steps inside WSL

## Building from Source

git clone https://github.com/gradosoft/pg_bloat.git
cd pg_bloat
mkdir build && cd build
cmake ..
make

### Build with Visual Studio (WSL)

If using Visual Studio 2022 with WSL:

1. Open the project folder in Visual Studio
2. Select Linux Debug (WSL) configuration
3. Build with Ctrl+Shift+B

## Usage

### Basic Syntax

./pg_bloat [OPTIONS] [DBNAME]

### Connection Options

| Option | Description | Default |
|--------|-------------|---------|
| -h HOST | PostgreSQL server host | localhost |
| -p PORT | PostgreSQL server port | 5432 |
| -U USER | Database user | (required) |
| -W | Prompt for password (or use PGPASSWORD env) | - |
| -d DBNAME | Database name to connect to | Same as user |
| DBNAME | Database name (positional argument) | - |

### Output Options

| Option | Description |
|--------|-------------|
| --format json | Output in JSON format (for programmatic use) |
| --format 1 | Same as --format json |
| --help | Show help message |

### Examples

#### Interactive password prompt

pg_bloat -h localhost -U postgres -d mydb -W

#### Using PGPASSWORD environment variable

export PGPASSWORD="secret"
pg_bloat -h localhost -U postgres -d mydb

#### JSON output for programmatic use

pg_bloat -h localhost -U postgres -d mydb --format json

#### One-liner with password

PGPASSWORD=secret pg_bloat -h localhost -U postgres -d mydb

## Output Examples

### Human-readable Format

Connected to localhost:5432/testdb as postgres

Database Info:
  PostgreSQL: PostgreSQL 18.4 (Ubuntu 18.4)
  Database:   testdb
  User:       postgres

Tables with bloat (>5% dead tuples) - showing 1 of 5 total tables:

Schema                    Table                     Total Size   Live         Dead         Dead%    Bloat        Last Vacuum
------------------------------------------------------------------------------------------------------------------------
public                    test_bloat                188 MB       333334       833333       71.43    132 MB       2024-01-15 10:30:00

Note: Only tables with >5% dead tuples are shown.

### JSON Format

{
  "status": "success",
  "min_dead_percent": 5,
  "tables": [
    {
      "schemaname": "public",
      "tablename": "test_bloat",
      "total_size": "188 MB",
      "live_tuples": 333334,
      "dead_tuples": 833333,
      "dead_percent": 71.43,
      "bloat_size": "132 MB",
      "last_vacuum_time": "2024-01-15 10:30:00"
    }
  ],
  "total_tables": 1
}

## Understanding the Output

| Column | Description |
|--------|-------------|
| Total Size | Current table size on disk (including bloat) |
| Live | Number of live (visible) tuples |
| Dead | Number of dead tuples (awaiting vacuum) |
| Dead% | Percentage of dead tuples - higher values indicate more bloat |
| Bloat | Estimated wasted space that could be reclaimed |
| Last Vacuum | Time of last VACUUM or AUTOVACUUM |

## What to Do About Bloat

1. For high bloat (>50%):

   VACUUM FULL table_name;

   Or use pg_repack for minimal locking.

2. For moderate bloat (10-50%):

   VACUUM table_name;

3. Preventative measures:
   - Tune autovacuum settings
   - Increase autovacuum_vacuum_scale_factor for large tables
   - Schedule regular VACUUM during maintenance windows

## Exit Codes

| Code | Meaning |
|------|---------|
| 0 | Success |
| 1 | Error (connection failed, invalid arguments, etc.) |

## Troubleshooting

### Connection failed

- Verify PostgreSQL is running: sudo service postgresql status
- Check credentials and permissions
- Ensure pg_hba.conf allows connections from localhost

### Query failed: permission denied

- User needs SELECT privileges on pg_stat_user_tables

### Build error: libpq-fe.h not found

Install PostgreSQL development libraries:

Ubuntu/Debian:
sudo apt install libpq-dev

macOS:
brew install postgresql

## License

MIT License

## Author

Vladislav Romenskiy

## Contributing

Issues and pull requests are welcome on GitHub.
