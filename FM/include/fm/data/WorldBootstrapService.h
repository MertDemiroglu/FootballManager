#pragma once

#include <string>

class World;

class WorldBootstrapService {
public:
    static void loadIntoWorldFromSqlite(World& world, const std::string& dbPath, int initialSeasonYear);

    static void initializeAndLoadIntoWorldFromSqlite(
        World& world,
        const std::string& dbPath,
        const std::string& schemaSqlPath,
        const std::string& seedSqlPath,
        int initialSeasonYear);

    static void resetAndLoadIntoWorldFromSqlite(
        World& world,
        const std::string& dbPath,
        const std::string& schemaSqlPath,
        const std::string& seedSqlPath,
        int initialSeasonYear);
};
