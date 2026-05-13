# Save/Load State Map

This document locks the current save/load ownership model after the save-slot work. A save slot is a full-world, all-leagues save database. It is not scoped to the current team, the managed club, or a single active league.

`SaveMetadata` describes the save card and identity only. It is not the source of truth for mutable world content. The managed league/team ids only point to the user's managed club inside the saved world.

## Authoritative Date Model

- `Game::date` is the authoritative in-memory game date.
- `game_state."current_date"` is the authoritative persisted game date.
- `save_metadata."current_date"` is a display/cache copy that mirrors `game_state."current_date"`.
- `created_at_utc` and `updated_at_utc` are real-world timestamps only. They must never be used as game dates, fixture dates, season years, shell dates, dashboard dates, or scheduler input.

## Save Slot Model

- Save slot folder: `AppData/.../saves/<saveSlotId>/game.db`
- Runtime DB contents represent the whole football world across all leagues.
- `SaveSlotPaths` resolves app/save-slot paths in the FM_UI app layer.
- `Game/core` receives explicit `GameBootstrapOptions`; it does not resolve AppData or source-tree paths.
- `GameFacade` starts without an active game and creates/loads a `Game` only after explicit New Game, Continue, or Load Game actions.
- `SaveSlotService` lists folders and metadata in the app layer, but delegates runtime validation to `RuntimeSaveValidator` in core/data.

## Persisted State

### `save_metadata`

Stores save-slot identity/card data: slot id, display name, manager name, managed club pointer, display current date, real-world timestamps, schema version, and world version.

- Writer: `SqliteSaveMetadataRepository`, called by `Game::ensureSaveMetadata`, `Game::setUserTeam`, `Game::setSaveManagerName`, and date update paths.
- Reader: `Game::getSaveMetadata`, `SaveSlotService::listSaveSlots`, `GameFacade::loadGameSave`.
- Saved when: save is created/opened, managed club changes, manager name changes, game date changes.
- Authority: cache/display for current date; authoritative for save-card identity and managed club pointer.
- Multi-league implication: `managedLeagueId`/`managedTeamId` identify the user's club only; they do not limit the saved world scope.

### `game_state`

Stores singleton runtime game state, currently including `current_date` and current `GameState`.

- Writer: `SqliteGameStateRepository::saveRuntimeState`, called by `Game::persistRuntimeState`.
- Reader: `SqliteGameStateRepository::loadCurrentDate`, called by `Game::restoreRuntimeState` and `RuntimeSaveValidator`.
- Saved when: runtime state is flushed, day advances, matches are queued/played, and other current save lifecycle paths persist state.
- Authority: authoritative persisted game date.
- Multi-league implication: one game date drives all leagues in the saved world.

### `league_runtime_state`

Stores per-league runtime season information: league id, season year, fixture-generated flag, and last rollover year.

- Writer: `Game::persistRuntimeState`.
- Reader/restorer: `Game::restoreRuntimeState`, with validation in `RuntimeSaveValidator`.
- Saved when: runtime state is persisted.
- Authority: authoritative per-league season runtime state.
- Multi-league implication: every league context must have its own row. Future leagues must not infer state from the managed league.

### `runtime_fixtures`

Stores generated fixtures and played/unplayed result state: match id, league id, season year, matchweek, match date, teams, played flag, transient `event_enqueued`, and goals.

- Writer: `Game::persistRuntimeState`.
- Reader/restorer: `SqliteGameStateRepository::loadFixtures`, then `Game::restoreRuntimeState`.
- Saved when: fixtures are generated, matches are queued/played, or runtime state is flushed.
- Authority: authoritative fixture schedule and played-result state, except `event_enqueued`.
- Multi-league implication: fixtures are stored for all leagues using `league_id`.
- Note: `event_enqueued` exists in the schema/DTO but is intentionally non-authoritative until active blocking interaction persistence exists. It is currently reset on save/load so a reload from a pre-match pause cannot permanently skip the match.

### `runtime_match_reports`

Stores match report header data for played matches: match id, league id, season year, match date, teams, matchweek, goals, coaches, formations, and starting player ids.

- Writer: `Game::persistRuntimeState`.
- Reader/restorer: `SqliteGameStateRepository::loadMatchReports`, then `League::restoreMatchReport`.
- Saved when: played match reports exist and runtime state is persisted.
- Authority: authoritative played match report summary for current visible save/load scope.
- Multi-league implication: reports include `league_id` and must be restored into the correct league context.

### `runtime_match_player_reports`

Stores per-player match report details: player id, team id, started flag, minutes, goals, assists, yellow cards, and red cards.

- Writer: `SqliteGameStateRepository::saveRuntimeState`.
- Reader/restorer: `SqliteGameStateRepository::loadMatchReports`.
- Saved when: match reports are persisted.
- Authority: authoritative persisted player report details for played matches.
- Multi-league implication: player ids must resolve inside the restored world, not only the managed team.

### `runtime_match_events`

Stores ordered match events for reports: match id, event index, minute, kind, team id, primary player id, and secondary player id.

- Writer: `SqliteGameStateRepository::saveRuntimeState`.
- Reader/restorer: `SqliteGameStateRepository::loadMatchReports`.
- Saved when: match reports with event timelines are persisted.
- Authority: authoritative persisted match event timeline for played matches.
- Multi-league implication: events are attached through match reports and league-specific match ids.

### `player_runtime_state`

Stores mutable player condition state: form, fitness, and morale.

- Writer: `Game::persistRuntimeState`.
- Reader/restorer: `Game::restoreRuntimeState`.
- Saved when: daily updates, matches, and runtime flushes persist state.
- Authority: authoritative persisted condition override over seed player data.
- Multi-league implication: player condition is saved for all players in all loaded league contexts.

## Validation Ownership

`RuntimeSaveValidator` owns runtime save validation in the core/data layer. It checks:

- `game_state` row exists and date is parseable.
- `league_runtime_state` exists.
- `runtime_fixtures` are present when fixture generation says they should be.
- `player_runtime_state` exists.
- Fixture league and season match league runtime state.
- Fixture ids, matchweeks, and team ids are valid.
- Current game date is within the current temporary Super Lig season-window assumptions.
- Metadata current date mirrors `game_state."current_date"` when metadata is provided.

TODO: validation currently uses core Super Lig assumptions for season windows. When LeagueRules move to SQL/multi-league data, validation should read league-specific rules instead of assuming July 1 preseason boundaries.

## Not Yet Persisted / Later Phases

### Selected lineup / tactical setup

- Why it matters: users expect the selected match squad to survive reloads. `TeamSheet` now means starting XI, up to 10 substitutes, and `TacticalSetup`.
- Current shape: tactical setup supports only mentality and tempo, defaulting to Balanced and Normal for every team.
- Current risk: selected squad/tactical edits may need to be rebuilt or lost after load unless they are already part of active match reports.
- Suggested priority: high, first backend phase after this roadmap.
- Storage direction: `team_lineup_state`, lineup slot assignment tables, substitute tables, and tactical setup columns keyed by `league_id`/`team_id`.
- Next persistence PR: persist selected match squad and tactical setup to SQL/runtime save.
- Match engine note: tactical setup is inert today; the future match engine rewrite should consume `TacticalSetup`.

### Pending/active blocking interactions

- Why it matters: closing during pre-match, post-match, or transfer decisions should restore the exact blocking state.
- Current risk: reload may resume at dashboard/world state rather than the exact interaction screen.
- Suggested priority: high after lineup persistence.
- Storage direction: interaction table with kind, league/team/match/offer ids, and payload version.

### Pre-match interaction state

- Why it matters: pre-match uses generated team sheets and lineup edits before playing a match.
- Current risk: closing during pre-match can lose the exact pending pre-match UI state.
- Suggested priority: high, coupled with active interaction persistence.
- Storage direction: active interaction payload plus selected team sheets.

### Post-match interaction/report screen state

- Why it matters: users may close before dismissing a post-match report.
- Current risk: played match persists, but the exact post-match blocking screen may not reopen.
- Suggested priority: medium.
- Storage direction: active interaction payload referencing persisted match report id.

### Transfer offers

- Why it matters: pending offers are mutable gameplay state.
- Current risk: generated offers and decisions can be lost across reloads.
- Suggested priority: high after active interaction basics.
- Storage direction: transfer offer runtime table with status, buyer/seller ids, player id, fee, dates, and expiry policy.

### Transfer decisions

- Why it matters: accepted/rejected decisions mutate roster and finances.
- Current risk: decisions may not replay or persist reliably without offer and roster mutation persistence.
- Suggested priority: after transfer offer persistence.
- Storage direction: transfer decision/status columns and transaction records.

### Roster mutations

- Why it matters: transfers and contracts change team membership over time.
- Current risk: seed rosters may override runtime roster changes on load.
- Suggested priority: after transfer decisions.
- Storage direction: runtime player-team membership and contract tables, or domain event snapshots.

### Budgets/finance

- Why it matters: wages, transfer budgets, payments, and income affect gameplay decisions.
- Current risk: financial changes may reset to seed values.
- Suggested priority: with roster/transfer persistence.
- Storage direction: team finance runtime table keyed by `league_id`/`team_id`.

### Contract changes

- Why it matters: contract years decrease and free-agent collection mutates season state.
- Current risk: contract state may diverge from saved season progress.
- Suggested priority: with roster mutation persistence.
- Storage direction: player contract runtime table.

### Completed season archive/history

- Why it matters: past standings, champions, and long-term history need durable records.
- Current risk: current season data is saved, but completed season history may not be queryable after rollover.
- Suggested priority: after current-season mutable state is stable.
- Storage direction: season archive tables for standings, fixtures, reports, and awards.

### LeagueRules/data-driven rules

- Why it matters: multi-league saves need league-specific season windows, matchday rules, transfer windows, and fixture logic.
- Current risk: validation and scheduling assume current core Super Lig rules.
- Suggested priority: before adding multiple real leagues.
- Storage direction: SQL-backed league rule tables and versioned rule loaders.

### Runtime ID counters

- Why it matters: match ids, offer ids, and other generated identifiers must not collide after load.
- Current risk: counters must be advanced from persisted ids; any missing counter can cause duplicate ids.
- Suggested priority: as each runtime entity type is persisted.
- Storage direction: runtime counters table or deterministic max-id recovery per entity table.
