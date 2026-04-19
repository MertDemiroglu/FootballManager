#include "fm/data/SqliteDatabase.h"

#include <stdexcept>

namespace {
    std::string makeSqliteError(sqlite3* db, const std::string& context) {
        const char* err = db ? sqlite3_errmsg(db) : "unknown sqlite error";
        return context + ": " + (err ? err : "unknown sqlite error");
    }

    std::string makeStatementError(sqlite3_stmt* stmt, const std::string& context) {
        sqlite3* db = stmt ? sqlite3_db_handle(stmt) : nullptr;
        return makeSqliteError(db, context);
    }
}

SqliteDatabase::SqliteDatabase(const std::string& dbPath) {
    open(dbPath);
}

SqliteDatabase::SqliteDatabase(SqliteDatabase&& other) noexcept : db(other.db) {
    other.db = nullptr;
}

SqliteDatabase& SqliteDatabase::operator=(SqliteDatabase&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    if (db != nullptr) {
        sqlite3_close(db);
    }

    db = other.db;
    other.db = nullptr;
    return *this;
}

SqliteDatabase::~SqliteDatabase() {
    if (db != nullptr) {
        sqlite3_close(db);
    }
}

void SqliteDatabase::open(const std::string& dbPath) {
    if (db != nullptr) {
        sqlite3_close(db);
        db = nullptr;
    }

    const int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::string error = makeSqliteError(db, "failed to open sqlite database at path '" + dbPath + "'");
        if (db != nullptr) {
            sqlite3_close(db);
            db = nullptr;
        }
        throw std::runtime_error(error);
    }
}

bool SqliteDatabase::isOpen() const {
    return db != nullptr;
}

sqlite3* SqliteDatabase::handle() const {
    if (db == nullptr) {
        throw std::runtime_error("sqlite database handle is not open");
    }
    return db;
}

void SqliteDatabase::execute(const std::string& sql) const {
    char* errorMessage = nullptr;
    const int rc = sqlite3_exec(handle(), sql.c_str(), nullptr, nullptr, &errorMessage);
    if (rc != SQLITE_OK) {
        const std::string sqliteMessage = errorMessage ? errorMessage : "unknown sqlite error";
        if (errorMessage != nullptr) {
            sqlite3_free(errorMessage);
        }
        throw std::runtime_error("failed to execute SQL script: " + sqliteMessage);
    }
}

SqliteStatement SqliteDatabase::prepare(const std::string& sql) const {
    sqlite3_stmt* stmt = nullptr;
    const int rc = sqlite3_prepare_v2(handle(), sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        throw std::runtime_error(makeSqliteError(handle(), "failed to prepare statement: " + sql));
    }

    return SqliteStatement(stmt);
}

SqliteStatement::SqliteStatement(sqlite3_stmt* preparedStmt) : stmt(preparedStmt) {}

SqliteStatement::SqliteStatement(SqliteStatement&& other) noexcept : stmt(other.stmt) {
    other.stmt = nullptr;
}

SqliteStatement& SqliteStatement::operator=(SqliteStatement&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    if (stmt != nullptr) {
        sqlite3_finalize(stmt);
    }

    stmt = other.stmt;
    other.stmt = nullptr;
    return *this;
}

SqliteStatement::~SqliteStatement() {
    if (stmt != nullptr) {
        sqlite3_finalize(stmt);
    }
}

sqlite3_stmt* SqliteStatement::handle() const {
    if (stmt == nullptr) {
        throw std::runtime_error("sqlite statement handle is null");
    }
    return stmt;
}

void SqliteStatement::bindInt(int index, int value) {
    const int rc = sqlite3_bind_int(handle(), index, value);
    if (rc != SQLITE_OK) {
        throw std::runtime_error(makeStatementError(handle(), "failed to bind int at index " + std::to_string(index)));
    }
}

void SqliteStatement::bindInt64(int index, std::int64_t value) {
    const int rc = sqlite3_bind_int64(handle(), index, static_cast<sqlite3_int64>(value));
    if (rc != SQLITE_OK) {
        throw std::runtime_error(makeStatementError(handle(), "failed to bind int64 at index " + std::to_string(index)));
    }
}

void SqliteStatement::bindText(int index, const std::string& value) {
    const int rc = sqlite3_bind_text(handle(), index, value.c_str(), static_cast<int>(value.size()), SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        throw std::runtime_error(makeStatementError(handle(), "failed to bind text at index " + std::to_string(index)));
    }
}

bool SqliteStatement::stepRow() {
    const int rc = sqlite3_step(handle());
    if (rc == SQLITE_ROW) {
        return true;
    }
    if (rc == SQLITE_DONE) {
        return false;
    }

    throw std::runtime_error(makeStatementError(handle(), "failed to step statement"));
}

void SqliteStatement::stepDone() {
    const int rc = sqlite3_step(handle());
    if (rc != SQLITE_DONE) {
        throw std::runtime_error(makeStatementError(handle(), "expected SQLITE_DONE while stepping statement"));
    }
}

void SqliteStatement::reset() {
    const int rc = sqlite3_reset(handle());
    if (rc != SQLITE_OK) {
        throw std::runtime_error(makeStatementError(handle(), "failed to reset statement"));
    }

    const int clearRc = sqlite3_clear_bindings(handle());
    if (clearRc != SQLITE_OK) {
        throw std::runtime_error(makeStatementError(handle(), "failed to clear statement bindings"));
    }
}

int SqliteStatement::columnInt(int column) const {
    return sqlite3_column_int(handle(), column);
}

std::int64_t SqliteStatement::columnInt64(int column) const {
    return static_cast<std::int64_t>(sqlite3_column_int64(handle(), column));
}

std::string SqliteStatement::columnText(int column) const {
    const unsigned char* text = sqlite3_column_text(handle(), column);
    if (text == nullptr) {
        return "";
    }

    return reinterpret_cast<const char*>(text);
}
