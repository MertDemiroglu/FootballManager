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

## State Ownership Rules

- `Game` orchestrates simulation flow. It owns the in-memory date, user/manager selection, the world instance, interaction manager, scheduler/queue wiring, and temporary pending interaction snapshots. It must not own team-specific runtime state such as a global selected-team-sheet registry.
- `World` owns league contexts and cross-league services.
- `LeagueContext` owns league-scoped orchestration state such as rules, season plan, rollover guard state, and match command handling.
- `League` owns competition runtime state: teams, fixtures, standings/projections, current season history, and match reports.
- `Team` owns roster membership, player index, colors, budgets, coach relation, and the current/default selected `TeamSheet`.
- `HeadCoach` owns `TacticalPreferences`: preferred formation, mentality, and tempo. These are defaults/identity, not a concrete match squad.
- `TeamSheet` owns the selected/current match squad: formation, starting assignments, substitutes, and active `TacticalSetup`.
- `PreMatchInteraction` should own match-specific frozen team sheets once active interaction persistence exists. Until then, `Game` may hold pending pre-match home/away snapshots as temporary orchestration state only.
- `SaveMetadata` is display/cache state for save cards and identity. It must not drive gameplay or hold mutable world state.
- Runtime DB tables are the source of persisted game runtime state. `game_state`, league runtime rows, fixtures, match reports, player runtime state, and team-sheet tables restore the playable world.
- QML is presentation only. It may hold transient UI selection/highlight state, but gameplay source of truth must come from `GameFacade`/core models and mutations must write back through backend methods.

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

### `league_rules`

Stores SQL-backed league/season configuration per league: stable league code/name, team count, matchday counts, preseason boundaries, kickoff rule parameters, winter break settings, and match spacing.

- Writer: bootstrap schema/seed data for the current seed world.
- Reader: `SqliteLeagueRulesRepository`, called by world bootstrap and `RuntimeSaveValidator`.
- Saved when: seeded/bootstrap data is initialized. This is configuration data, not mutable runtime progress.
- Authority: authoritative league season/rules data. Core consumes this data through `LeagueRules`; it should not own Super Lig-specific constants.
- Multi-league implication: every future league needs its own `league_rules` row.

### `league_transfer_windows`

Stores transfer-window date rules per league/window code. The current seed has `summer` and `winter` windows for Super Lig.

- Reader: `SqliteLeagueRulesRepository`, which converts SQL month/day/year-offset rules into `LeagueRules` transfer-window functions.
- Authority: authoritative league transfer-window configuration. Transfer offer persistence is still future work.
- Multi-league implication: rows are keyed by `league_id` and `window_code`.

### `league_matchday_distribution_offsets`

Stores deterministic fixture date distribution offsets per league.

- Reader: `SqliteLeagueRulesRepository`, then fixture generation through `LeagueRules`.
- Authority: authoritative matchday distribution data.
- Multi-league implication: rows are keyed by `league_id` and ordered by zero-based `offset_index`.

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

### `runtime_team_sheets`

Stores selected match squad headers per league/team: formation, mentality, and tempo.

- Writer: `Game::persistRuntimeState`.
- Reader/restorer: `SqliteGameStateRepository::loadTeamSheetStates`, then `Game::restoreRuntimeState`.
- Saved when: runtime state is persisted, including lineup editor changes and auto-select updates.
- Authority: `Team` owns the current/default selected `TeamSheet`. `Game` only orchestrates create/update/ensure/resolve flow and persistence snapshots. `save_metadata` must not store lineup or tactical state.
- Multi-league implication: rows are keyed by `league_id`/`team_id`; this is full-world state, not managed-team-only state.
- Tactical identity note: `HeadCoach` owns `TacticalPreferences`, which are coach/team defaults. `TacticalSetup` is the active match-squad setup persisted in `TeamSheet`.
- Match engine note: tactical setup currently supports mentality and tempo only. It persists, but does not affect simulation yet; the future match engine rewrite should consume `TacticalSetup`.

### `runtime_team_sheet_starters`

Stores selected starting XI slot assignments: slot index, slot role, and player id.

- Writer: `SqliteGameStateRepository::saveRuntimeState`.
- Reader/restorer: `SqliteGameStateRepository::loadTeamSheetStates`.
- Saved when: selected team sheets are persisted.
- Authority: authoritative selected starting XI for pre-match and lineup editor restore.
- Multi-league implication: starter rows are attached to `runtime_team_sheets` by `league_id`/`team_id`.

### `runtime_team_sheet_substitutes`

Stores selected substitute player ids in deterministic bench order. Substitute order is zero-based and capped at 10 in code.

- Writer: `SqliteGameStateRepository::saveRuntimeState`.
- Reader/restorer: `SqliteGameStateRepository::loadTeamSheetStates`.
- Saved when: selected team sheets are persisted.
- Authority: authoritative selected bench for lineup editor restore.
- Multi-league implication: substitute rows are attached to `runtime_team_sheets` by `league_id`/`team_id`.

## Validation Ownership

`RuntimeSaveValidator` owns runtime save validation in the core/data layer. It checks:

- `game_state` row exists and date is parseable.
- `league_runtime_state` exists.
- `runtime_fixtures` are present when fixture generation says they should be.
- `player_runtime_state` exists.
- Fixture league and season match league runtime state.
- Fixture ids, matchweeks, and team ids are valid.
- Runtime team sheet formation, slot roles, starter ids, substitutes, and tactical stable codes are valid.
- Runtime team sheets have no duplicate starters, duplicate substitutes, or starter/substitute overlap.
- Runtime team sheets do not exceed 10 substitutes.
- Current game date is within the SQL-loaded league season window for each league runtime row.
- Fixture dates fit their SQL-loaded league kickoff/season window.
- Metadata current date mirrors `game_state."current_date"` when metadata is provided.

## Not Yet Persisted / Later Phases

### Pending/active blocking interactions

- Why it matters: closing during pre-match, post-match, or transfer decisions should restore the exact blocking state.
- Current risk: reload may resume at dashboard/world state rather than the exact interaction screen.
- Suggested priority: high after selected match squad persistence.
- Storage direction: interaction table with kind, league/team/match/offer ids, and payload version.

### Pre-match interaction state

- Why it matters: pre-match uses generated team sheets and lineup edits before playing a match.
- Current risk: closing during pre-match can lose the exact pending pre-match UI state.
- Suggested priority: high, coupled with active interaction persistence.
- Storage direction: active interaction payload plus match-specific frozen team-sheet snapshots.
- Ownership note: `Team.selectedTeamSheet` is the current/default team-owned squad state. Pending pre-match home/away sheets are frozen snapshots for one interaction and should move into `PreMatchInteraction` / active interaction persistence later.

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

### LeagueRules/data-driven rules follow-up

- Current state: LeagueRules / SeasonConfig is SQL-backed through `league_rules`, `league_transfer_windows`, and `league_matchday_distribution_offsets`.
- Remaining work: add more league rows/rules when multi-league content expands, and eventually replace the transitional lambda fields inside `LeagueRules` with fully value-based rule configs.

### Runtime ID counters

- Why it matters: match ids, offer ids, and other generated identifiers must not collide after load.
- Current risk: counters must be advanced from persisted ids; any missing counter can cause duplicate ids.
- Suggested priority: as each runtime entity type is persisted.
- Storage direction: runtime counters table or deterministic max-id recovery per entity table.
