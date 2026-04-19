PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS leagues (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS coaches (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    preferred_formation INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS teams (
    id INTEGER PRIMARY KEY,
    league_id INTEGER NOT NULL,
    name TEXT NOT NULL,
    transfer_budget INTEGER NOT NULL,
    wage_budget INTEGER NOT NULL,
    total_budget INTEGER NOT NULL,
    coach_id INTEGER NOT NULL,
    FOREIGN KEY (league_id) REFERENCES leagues(id),
    FOREIGN KEY (coach_id) REFERENCES coaches(id)
);

CREATE TABLE IF NOT EXISTS players (
    id INTEGER PRIMARY KEY,
    team_id INTEGER NOT NULL,
    name TEXT NOT NULL,
    position TEXT NOT NULL,
    age INTEGER NOT NULL,
    wage INTEGER NOT NULL,
    contract_years INTEGER NOT NULL,
    s1 INTEGER NOT NULL,
    s2 INTEGER NOT NULL,
    s3 INTEGER NOT NULL,
    s4 INTEGER NOT NULL,
    s5 INTEGER NOT NULL,
    FOREIGN KEY (team_id) REFERENCES teams(id)
);
