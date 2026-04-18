#include "fm/data/SqliteBootstrapDatabaseInitializer.h"

#include "fm/data/SqliteDatabase.h"

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {
    std::string readTextFileOrThrow(const std::string& filePath) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            throw std::runtime_error("failed to open SQL file: " + filePath);
        }

        return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }

    void runInit(const std::string& dbPath, const std::string& schemaSqlPath, const std::string* seedSqlPath) {
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
