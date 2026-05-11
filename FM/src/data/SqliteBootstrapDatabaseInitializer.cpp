#include "fm/data/SqliteBootstrapDatabaseInitializer.h"

#include "fm/data/SqliteDatabase.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {
    void ensureParentDirectoryExists(const std::string& dbPath) {
        const std::filesystem::path path(dbPath);
        const std::filesystem::path parent = path.parent_path();
        if (parent.empty()) {
            return;
        }

        std::error_code error;
        if (!std::filesystem::create_directories(parent, error) && error) {
            throw std::runtime_error("failed to create sqlite database directory: " + parent.string() + ": " + error.message());
        }
    }

    std::string readTextFileOrThrow(const std::string& filePath) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            throw std::runtime_error("failed to open SQL file: " + filePath);
        }

        return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }

    void runInit(const std::string& dbPath, const std::string& schemaSqlPath, const std::string* seedSqlPath) {
        ensureParentDirectoryExists(dbPath);
        const std::string schemaSql = readTextFileOrThrow(schemaSqlPath);

        SqliteDatabase database(dbPath);
        database.execute("BEGIN TRANSACTION;");

        try {
            database.execute(schemaSql);
            if (seedSqlPath != nullptr) {
                const std::string seedSql = readTextFileOrThrow(*seedSqlPath);
                database.execute(seedSql);
            }
            database.execute("COMMIT;");
        }
        catch (...) {
            try {
                database.execute("ROLLBACK;");
            }
            catch (...) {
            }
            throw;
        }
    }
}

void SqliteBootstrapDatabaseInitializer::initialize(const std::string& dbPath, const std::string& schemaSqlPath) {
    runInit(dbPath, schemaSqlPath, nullptr);
}

void SqliteBootstrapDatabaseInitializer::initializeWithSeed(const std::string& dbPath, const std::string& schemaSqlPath, const std::string& seedSqlPath) {
    runInit(dbPath, schemaSqlPath, &seedSqlPath);
}

void SqliteBootstrapDatabaseInitializer::resetWithSeed(const std::string& dbPath, const std::string& schemaSqlPath, const std::string& seedSqlPath) {
    std::error_code error;
    if (std::filesystem::exists(dbPath, error)) {
        if (!std::filesystem::remove(dbPath, error)) {
            throw std::runtime_error("failed to remove existing sqlite database at path '" + dbPath + "': " + error.message());
        }
    } else if (error) {
        throw std::runtime_error("failed to inspect sqlite database at path '" + dbPath + "': " + error.message());
    }

    initializeWithSeed(dbPath, schemaSqlPath, seedSqlPath);
}
