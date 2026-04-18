#pragma once

#include "fm/data/BootstrapDtos.h"
#include "fm/data/SqliteDatabase.h"

#include <string>
#include <vector>

class SqliteBootstrapRepository {
private:
    SqliteDatabase database;

public:
    explicit SqliteBootstrapRepository(const std::string& databasePath);

    std::vector<LeagueSeedData> loadLeagues() const;
};
