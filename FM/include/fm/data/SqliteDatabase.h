#pragma once

#include <sqlite3.h>

#include <cstdint>
#include <string>

class SqliteStatement;

class SqliteDatabase {
private:
    sqlite3* db = nullptr;

public:
    SqliteDatabase() = default;
    explicit SqliteDatabase(const std::string& dbPath);

    SqliteDatabase(const SqliteDatabase&) = delete;
    SqliteDatabase& operator=(const SqliteDatabase&) = delete;

    SqliteDatabase(SqliteDatabase&& other) noexcept;
    SqliteDatabase& operator=(SqliteDatabase&& other) noexcept;

    ~SqliteDatabase();

    void open(const std::string& dbPath);
    bool isOpen() const;
    sqlite3* handle() const;

    SqliteStatement prepare(const std::string& sql) const;
};

class SqliteStatement {
private:
    sqlite3_stmt* stmt = nullptr;

public:
    SqliteStatement() = default;
    explicit SqliteStatement(sqlite3_stmt* preparedStmt);

    SqliteStatement(const SqliteStatement&) = delete;
    SqliteStatement& operator=(const SqliteStatement&) = delete;

    SqliteStatement(SqliteStatement&& other) noexcept;
    SqliteStatement& operator=(SqliteStatement&& other) noexcept;

    ~SqliteStatement();

    sqlite3_stmt* handle() const;

    void bindInt(int index, int value);
    void bindInt64(int index, std::int64_t value);
    void bindText(int index, const std::string& value);

    bool stepRow();
    void stepDone();
    void reset();

    int columnInt(int column) const;
    std::int64_t columnInt64(int column) const;
    std::string columnText(int column) const;
};
