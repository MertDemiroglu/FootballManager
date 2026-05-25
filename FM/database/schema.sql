PRAGMA foreign_keys = ON;
PRAGMA user_version = 11;

CREATE TABLE IF NOT EXISTS save_metadata (
    id INTEGER PRIMARY KEY CHECK (id = 1),
    save_slot_id TEXT NOT NULL,
    save_name TEXT NOT NULL,
    manager_name TEXT NOT NULL,
    managed_league_id INTEGER NOT NULL DEFAULT 0,
    managed_team_id INTEGER NOT NULL DEFAULT 0,
    "current_date" TEXT NOT NULL,
    created_at_utc TEXT NOT NULL,
    updated_at_utc TEXT NOT NULL,
    schema_version INTEGER NOT NULL DEFAULT 11,
    world_version INTEGER NOT NULL DEFAULT 1
);

CREATE TABLE IF NOT EXISTS runtime_save_settings (
    id INTEGER PRIMARY KEY CHECK (id = 1),
    auto_save_frequency TEXT NOT NULL,
    last_auto_save_date TEXT
);

CREATE TABLE IF NOT EXISTS game_state (
    id INTEGER PRIMARY KEY CHECK (id = 1),
    "current_date" TEXT NOT NULL,
    current_state INTEGER NOT NULL DEFAULT 0,
    updated_at_utc TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS league_runtime_state (
    league_id INTEGER PRIMARY KEY,
    season_year INTEGER NOT NULL,
    is_fixture_generated INTEGER NOT NULL,
    last_season_rollover_year INTEGER NOT NULL DEFAULT -1,
    FOREIGN KEY (league_id) REFERENCES leagues(id)
);

CREATE TABLE IF NOT EXISTS runtime_fixtures (
    match_id INTEGER PRIMARY KEY,
    league_id INTEGER NOT NULL,
    season_year INTEGER NOT NULL,
    matchweek INTEGER NOT NULL,
    match_date TEXT NOT NULL,
    home_team_id INTEGER NOT NULL,
    away_team_id INTEGER NOT NULL,
    played INTEGER NOT NULL,
    event_enqueued INTEGER NOT NULL DEFAULT 0,
    home_goals INTEGER NOT NULL DEFAULT -1,
    away_goals INTEGER NOT NULL DEFAULT -1,
    FOREIGN KEY (league_id) REFERENCES leagues(id),
    FOREIGN KEY (home_team_id) REFERENCES teams(id),
    FOREIGN KEY (away_team_id) REFERENCES teams(id)
);

CREATE TABLE IF NOT EXISTS runtime_match_reports (
    match_id INTEGER PRIMARY KEY,
    league_id INTEGER NOT NULL,
    season_year INTEGER NOT NULL,
    match_date TEXT NOT NULL,
    home_team_id INTEGER NOT NULL,
    away_team_id INTEGER NOT NULL,
    matchweek INTEGER NOT NULL,
    home_goals INTEGER NOT NULL,
    away_goals INTEGER NOT NULL,
    home_coach_id INTEGER NOT NULL,
    home_formation INTEGER NOT NULL,
    home_starting_player_ids TEXT NOT NULL,
    away_coach_id INTEGER NOT NULL,
    away_formation INTEGER NOT NULL,
    away_starting_player_ids TEXT NOT NULL,
    home_shots INTEGER NOT NULL DEFAULT 0,
    home_shots_on_target INTEGER NOT NULL DEFAULT 0,
    home_passes_attempted INTEGER NOT NULL DEFAULT 0,
    home_passes_completed INTEGER NOT NULL DEFAULT 0,
    home_tackles_attempted INTEGER NOT NULL DEFAULT 0,
    home_tackles_won INTEGER NOT NULL DEFAULT 0,
    home_interceptions INTEGER NOT NULL DEFAULT 0,
    home_fouls INTEGER NOT NULL DEFAULT 0,
    home_corners INTEGER NOT NULL DEFAULT 0,
    home_yellow_cards INTEGER NOT NULL DEFAULT 0,
    home_red_cards INTEGER NOT NULL DEFAULT 0,
    home_possession_share REAL NOT NULL DEFAULT 0.0,
    home_expected_goals REAL NOT NULL DEFAULT 0.0,
    away_shots INTEGER NOT NULL DEFAULT 0,
    away_shots_on_target INTEGER NOT NULL DEFAULT 0,
    away_passes_attempted INTEGER NOT NULL DEFAULT 0,
    away_passes_completed INTEGER NOT NULL DEFAULT 0,
    away_tackles_attempted INTEGER NOT NULL DEFAULT 0,
    away_tackles_won INTEGER NOT NULL DEFAULT 0,
    away_interceptions INTEGER NOT NULL DEFAULT 0,
    away_fouls INTEGER NOT NULL DEFAULT 0,
    away_corners INTEGER NOT NULL DEFAULT 0,
    away_yellow_cards INTEGER NOT NULL DEFAULT 0,
    away_red_cards INTEGER NOT NULL DEFAULT 0,
    away_possession_share REAL NOT NULL DEFAULT 0.0,
    away_expected_goals REAL NOT NULL DEFAULT 0.0,
    FOREIGN KEY (league_id) REFERENCES leagues(id)
);

CREATE TABLE IF NOT EXISTS runtime_match_player_reports (
    match_id INTEGER NOT NULL,
    player_id INTEGER NOT NULL,
    team_id INTEGER NOT NULL,
    started INTEGER NOT NULL,
    minutes_played INTEGER NOT NULL,
    goals INTEGER NOT NULL,
    assists INTEGER NOT NULL,
    yellow_cards INTEGER NOT NULL,
    red_cards INTEGER NOT NULL,
    PRIMARY KEY (match_id, player_id),
    FOREIGN KEY (match_id) REFERENCES runtime_match_reports(match_id),
    FOREIGN KEY (player_id) REFERENCES players(id),
    FOREIGN KEY (team_id) REFERENCES teams(id)
);

CREATE TABLE IF NOT EXISTS runtime_match_events (
    match_id INTEGER NOT NULL,
    event_index INTEGER NOT NULL,
    minute INTEGER NOT NULL,
    kind INTEGER NOT NULL,
    team_id INTEGER NOT NULL,
    primary_player_id INTEGER NOT NULL,
    secondary_player_id INTEGER NOT NULL,
    PRIMARY KEY (match_id, event_index),
    FOREIGN KEY (match_id) REFERENCES runtime_match_reports(match_id),
    FOREIGN KEY (team_id) REFERENCES teams(id)
);

CREATE TABLE IF NOT EXISTS player_runtime_state (
    player_id INTEGER PRIMARY KEY,
    form INTEGER NOT NULL,
    fitness INTEGER NOT NULL,
    morale INTEGER NOT NULL,
    FOREIGN KEY (player_id) REFERENCES players(id)
);

CREATE TABLE IF NOT EXISTS runtime_team_sheets (
    league_id INTEGER NOT NULL,
    team_id INTEGER NOT NULL,
    formation TEXT NOT NULL,
    mentality TEXT NOT NULL,
    tempo TEXT NOT NULL,
    width TEXT NOT NULL DEFAULT 'balanced',
    defensive_line TEXT NOT NULL DEFAULT 'standard',
    pressing_intensity TEXT NOT NULL DEFAULT 'normal',
    marking_style TEXT NOT NULL DEFAULT 'mixed',
    passing_directness TEXT NOT NULL DEFAULT 'balanced',
    PRIMARY KEY (league_id, team_id),
    FOREIGN KEY (league_id) REFERENCES leagues(id),
    FOREIGN KEY (team_id) REFERENCES teams(id)
);

CREATE TABLE IF NOT EXISTS runtime_team_sheet_starters (
    league_id INTEGER NOT NULL,
    team_id INTEGER NOT NULL,
    slot_index INTEGER NOT NULL,
    slot_role TEXT NOT NULL,
    player_id INTEGER NOT NULL,
    PRIMARY KEY (league_id, team_id, slot_index),
    FOREIGN KEY (league_id, team_id) REFERENCES runtime_team_sheets(league_id, team_id),
    FOREIGN KEY (player_id) REFERENCES players(id)
);

CREATE TABLE IF NOT EXISTS runtime_team_sheet_substitutes (
    league_id INTEGER NOT NULL,
    team_id INTEGER NOT NULL,
    substitute_order INTEGER NOT NULL,
    player_id INTEGER NOT NULL,
    PRIMARY KEY (league_id, team_id, substitute_order),
    FOREIGN KEY (league_id, team_id) REFERENCES runtime_team_sheets(league_id, team_id),
    FOREIGN KEY (player_id) REFERENCES players(id)
);

CREATE TABLE IF NOT EXISTS runtime_transfer_offers (
    offer_id INTEGER PRIMARY KEY,
    created_at TEXT NOT NULL,
    last_valid_date TEXT NOT NULL,
    expiry_policy TEXT NOT NULL,
    seller_league_id INTEGER NOT NULL,
    seller_team_id INTEGER NOT NULL,
    buyer_league_id INTEGER NOT NULL,
    buyer_team_id INTEGER NOT NULL,
    player_id INTEGER NOT NULL,
    fee INTEGER NOT NULL,
    status TEXT NOT NULL,
    resolution TEXT,
    FOREIGN KEY (seller_league_id) REFERENCES leagues(id),
    FOREIGN KEY (buyer_league_id) REFERENCES leagues(id),
    FOREIGN KEY (seller_team_id) REFERENCES teams(id),
    FOREIGN KEY (buyer_team_id) REFERENCES teams(id),
    FOREIGN KEY (player_id) REFERENCES players(id)
);

CREATE TABLE IF NOT EXISTS runtime_team_finances (
    league_id INTEGER NOT NULL,
    team_id INTEGER NOT NULL,
    total_budget INTEGER NOT NULL,
    transfer_budget INTEGER NOT NULL,
    wage_budget INTEGER NOT NULL,
    financial_strategy TEXT NOT NULL DEFAULT 'balanced',
    financial_health TEXT NOT NULL DEFAULT 'stable',
    PRIMARY KEY (league_id, team_id),
    FOREIGN KEY (league_id) REFERENCES leagues(id),
    FOREIGN KEY (team_id) REFERENCES teams(id)
);

CREATE TABLE IF NOT EXISTS runtime_player_roster_state (
    player_id INTEGER PRIMARY KEY,
    league_id INTEGER NOT NULL,
    team_id INTEGER NOT NULL,
    wage INTEGER,
    contract_years INTEGER,
    current_season_year INTEGER,
    FOREIGN KEY (league_id) REFERENCES leagues(id),
    FOREIGN KEY (team_id) REFERENCES teams(id),
    FOREIGN KEY (player_id) REFERENCES players(id)
);

CREATE TABLE IF NOT EXISTS runtime_free_agents (
    player_id INTEGER PRIMARY KEY,
    previous_league_id INTEGER,
    previous_team_id INTEGER,
    became_free_agent_date TEXT,
    wage INTEGER,
    contract_years INTEGER,
    current_season_year INTEGER,
    FOREIGN KEY (player_id) REFERENCES players(id),
    FOREIGN KEY (previous_league_id) REFERENCES leagues(id),
    FOREIGN KEY (previous_team_id) REFERENCES teams(id)
);

CREATE TABLE IF NOT EXISTS leagues (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS league_rules (
    league_id INTEGER PRIMARY KEY,
    league_code TEXT NOT NULL,
    league_name TEXT NOT NULL,
    team_count INTEGER NOT NULL,
    matchdays_per_season INTEGER NOT NULL,
    first_half_rounds INTEGER NOT NULL,
    preseason_start_month INTEGER NOT NULL,
    preseason_start_day INTEGER NOT NULL,
    next_preseason_year_offset INTEGER NOT NULL,
    next_preseason_start_month INTEGER NOT NULL,
    next_preseason_start_day INTEGER NOT NULL,
    kickoff_rule TEXT NOT NULL,
    kickoff_month INTEGER NOT NULL,
    kickoff_day INTEGER,
    kickoff_weekday INTEGER,
    kickoff_week_of_month INTEGER,
    winter_break_enabled INTEGER NOT NULL,
    winter_break_length_days INTEGER NOT NULL,
    winter_break_after_round_index INTEGER NOT NULL,
    match_spacing_days INTEGER NOT NULL,
    FOREIGN KEY (league_id) REFERENCES leagues(id)
);

CREATE TABLE IF NOT EXISTS league_transfer_windows (
    league_id INTEGER NOT NULL,
    window_code TEXT NOT NULL,
    start_year_offset INTEGER NOT NULL,
    start_month INTEGER NOT NULL,
    start_day INTEGER NOT NULL,
    end_year_offset INTEGER NOT NULL,
    end_month INTEGER NOT NULL,
    end_day INTEGER NOT NULL,
    PRIMARY KEY (league_id, window_code),
    FOREIGN KEY (league_id) REFERENCES leagues(id)
);

CREATE TABLE IF NOT EXISTS league_matchday_distribution_offsets (
    league_id INTEGER NOT NULL,
    offset_index INTEGER NOT NULL,
    offset_days INTEGER NOT NULL,
    PRIMARY KEY (league_id, offset_index),
    FOREIGN KEY (league_id) REFERENCES leagues(id)
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
    primary_color TEXT NOT NULL DEFAULT '#22c55e',
    secondary_color TEXT NOT NULL DEFAULT '#0f172a',
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

CREATE TABLE IF NOT EXISTS player_attributes (
    player_id INTEGER PRIMARY KEY,
    shooting INTEGER NOT NULL,
    passing INTEGER NOT NULL,
    first_touch INTEGER NOT NULL,
    technique INTEGER NOT NULL,
    tackling INTEGER NOT NULL,
    dribbling INTEGER NOT NULL,
    crossing INTEGER NOT NULL,
    marking INTEGER NOT NULL,
    heading INTEGER NOT NULL,
    set_pieces INTEGER NOT NULL,
    decisions INTEGER NOT NULL,
    vision INTEGER NOT NULL,
    positioning INTEGER NOT NULL,
    off_the_ball INTEGER NOT NULL,
    composure INTEGER NOT NULL,
    concentration INTEGER NOT NULL,
    work_rate INTEGER NOT NULL,
    teamwork INTEGER NOT NULL,
    leadership INTEGER NOT NULL,
    aggression INTEGER NOT NULL,
    pace INTEGER NOT NULL,
    acceleration INTEGER NOT NULL,
    stamina INTEGER NOT NULL,
    strength INTEGER NOT NULL,
    agility INTEGER NOT NULL,
    jumping_reach INTEGER NOT NULL,
    shot_stopping INTEGER NOT NULL,
    handling INTEGER NOT NULL,
    aerial_ability INTEGER NOT NULL,
    one_on_ones INTEGER NOT NULL,
    distribution INTEGER NOT NULL,
    FOREIGN KEY (player_id) REFERENCES players(id)
);
