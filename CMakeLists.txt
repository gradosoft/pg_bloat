# Установите минимальную версию CMake
cmake_minimum_required(VERSION 3.10)

# Установите имя проекта
project(pg_bloat C)

# Установите стандарты C
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Укажите флаги компиляции
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -Wconversion -Wdeprecated -O2")

# Определяем операционную систему
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Пути для PostgreSQL в WSL/Linux
    set(PostgreSQL_INCLUDE_DIR "/usr/include/postgresql")
    set(PostgreSQL_LIBRARY "/usr/lib/x86_64-linux-gnu/libpq.so")
    
    if(EXISTS ${PostgreSQL_INCLUDE_DIR} AND EXISTS ${PostgreSQL_LIBRARY})
        message(STATUS "Found PostgreSQL in WSL/Linux")
    else()
        message(WARNING "PostgreSQL paths not found. Adjust for your distribution.")
    endif()
elseif(APPLE)
    # macOS Apple Silicon: путь к libpq через brew --prefix
    execute_process(
        COMMAND brew --prefix libpq
        OUTPUT_VARIABLE LIBPQ_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    
    if(LIBPQ_PREFIX)
        set(PostgreSQL_INCLUDE_DIR "${LIBPQ_PREFIX}/include")
        set(PostgreSQL_LIBRARY "${LIBPQ_PREFIX}/lib/libpq.dylib")
        message(STATUS "Found libpq at ${LIBPQ_PREFIX}")
    else()
        # Fallback: прямой путь в Cellar
        set(PostgreSQL_INCLUDE_DIR "/opt/homebrew/Cellar/libpq/18.4/include")
        set(PostgreSQL_LIBRARY "/opt/homebrew/Cellar/libpq/18.4/lib/libpq.dylib")
        message(STATUS "Using fallback path to libpq")
    endif()
    
    if(EXISTS ${PostgreSQL_INCLUDE_DIR}/libpq-fe.h AND EXISTS ${PostgreSQL_LIBRARY})
        message(STATUS "libpq found successfully")
    else()
        message(FATAL_ERROR "libpq not found. Please install: brew install libpq")
    endif()
else()
    # Windows и другие системы
    find_package(PostgreSQL REQUIRED)
    message(STATUS "Found PostgreSQL via find_package")
endif()

# Добавляем include директории
include_directories(${PostgreSQL_INCLUDE_DIR})

# Укажите исходные файлы
set(SRCS src/main.c src/pg_bloat.c)
set(HEADERS src/pg_bloat.h)

# Создайте исполняемый файл
add_executable(pg_bloat ${SRCS} ${HEADERS})

# Добавьте линковку с PostgreSQL
target_link_libraries(pg_bloat ${PostgreSQL_LIBRARY})
