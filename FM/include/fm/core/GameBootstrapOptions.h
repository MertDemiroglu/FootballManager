#pragma once

#include<string>

enum class GameBootstrapMode {
    Sqlite
};

struct GameBootstrapOptions {
    GameBootstrapMode mode = GameBootstrapMode::Sqlite;
    std::string sqliteDbPath;
    std::string sqliteSchemaPath;
    std::string sqliteSeedPath;
    bool initializeDatabaseWithSeed = true;
};
