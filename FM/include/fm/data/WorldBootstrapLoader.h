#pragma once

class World;
class SqliteBootstrapRepository;
class SqliteLeagueRulesRepository;

class WorldBootstrapLoader {
private:
    SqliteBootstrapRepository& repository;
    SqliteLeagueRulesRepository& rulesRepository;

public:
    WorldBootstrapLoader(SqliteBootstrapRepository& repository, SqliteLeagueRulesRepository& rulesRepository);

    void load(World& world, int initialSeasonYear) const;
};
