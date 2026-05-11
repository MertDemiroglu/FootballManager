#pragma once

#include<string>

enum class GameBootstrapMode {
    Sqlite
};

enum class DatabaseOpenMode {
    OpenExisting,
    CreateFromSeedIfMissing,
    ResetFromSeed
};

struct GameBootstrapOptions {
    GameBootstrapMode mode = GameBootstrapMode::Sqlite;
    DatabaseOpenMode databaseOpenMode = DatabaseOpenMode::CreateFromSeedIfMissing;
    std::string sqliteDbPath;
    std::string sqliteSchemaPath;
    std::string sqliteSeedPath;
};
