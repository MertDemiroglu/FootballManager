#pragma once

#include <string>

class SqliteBootstrapDatabaseInitializer {
public:
    static void initialize(const std::string& dbPath, const std::string& schemaSqlPath);
    static void initializeWithSeed(const std::string& dbPath, const std::string& schemaSqlPath, const std::string& seedSqlPath);
};
