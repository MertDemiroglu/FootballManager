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
- Saves are checkpoint-based. Manual Save and scheduled autosave write the current safe runtime state to the current save slot; they do not restore exact transient popup/screen state.
- Continue/Load enters gameplay through Dashboard/current safe checkpoint.
- Manual Save overwrites the current save slot. Scheduled autosave also overwrites the current save slot and does not create rolling autosave folders.
- Autosave is based on game-date intervals, not wall-clock time. The default frequency is Weekly.
- Important world mutations can request a save, and `Game` coalesces pending requests so one safe flow does not spam full snapshot writes.
- The current format is a full runtime snapshot. Dirty/incremental table-level writes are a future optimization if multi-league snapshot cost becomes high.
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
- Runtime DB tables are the source of persisted game runtime state. `game_state`, league runtime rows, fixtures, match reports, player runtime state, team-owned roster ownership, free agent ownership, team finances, transfer offers, and team-sheet tables restore the playable world.
- `runtime_save_settings` owns runtime/app save policy such as autosave frequency. Do not store autosave settings in `save_metadata`.
- Runtime roster ownership, free agent ownership, and team finance snapshots are the source of truth for accepted transfer and free-agent effects after load. Resolved transfer offers are restored as offer state/history only; they are not replayed as commands.
- QML is presentation only. It may hold transient UI selection/highlight state, but gameplay source of truth must come from `GameFacade`/core models and mutations must write back through backend methods.

## Match Lifecycle Checkpoint Flow

- `Game` orchestrates match-day progression. `MatchScheduler` scans all league contexts for fixtures on the current game date and emits league-scoped `PlayMatchCommand`s.
- Managed-team fixtures create a pre-match interaction through core state exposed by `GameFacade`; QML does not own the match source of truth.
- `Game::playPendingPreMatch` and AI/non-managed matchday processing both delegate application to `PlayMatchCommandHandler`.
- `League::applyMatchReport` is the authoritative completion path. It marks the fixture played, writes the result, clears `eventEnqueued`, updates standings/projections, appends current-season history, updates team/player stats, and stores the current-season report together.
- `MatchPlayedEvent` is published after the match report is applied. It is not a second match-application path.
- On restore, fixtures are loaded first, then current-season reports are restored through `League::restoreMatchReport`. This rebuilds standings/recent form from the restored played-match state. If an older checkpoint has played goals without a report, `Game::restoreRuntimeState` falls back to `League::restoreFixtureResult` so the result still affects standings/history once.
- Match completion requests a runtime save and `Game` flushes it at a safe checkpoint. Manual Save and scheduled autosave still overwrite the same current save slot.
- Completed season archives, active interaction exact restore, and tactical effects in simulation remain future work.

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
- Saved when: runtime state is flushed by Manual Save, scheduled autosave, or a coalesced important save request.
- Authority: authoritative persisted game date.
- Multi-league implication: one game date drives all leagues in the saved world.

### `runtime_save_settings`

Stores singleton save policy settings: autosave frequency and the game-date checkpoint used for the next scheduled autosave calculation.

- Writer: `SqliteGameStateRepository::saveSaveSettings`, called by `Game` after save-policy changes and runtime save flushes.
- Reader: `SqliteGameStateRepository::loadSaveSettings`, called by `Game` during bootstrap.
- Default: `auto_save_frequency = weekly`; `last_auto_save_date` may be null until the first save/settings write initializes it.
- Stable codes: `manual_only`, `daily`, `every_3_days`, `weekly`, and `every_2_weeks`.
- Authority: authoritative save policy for the current save slot.
- Scope note: this table is intentionally separate from `save_metadata`; metadata remains display/cache state only.

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
- Authority: authoritative league transfer-window configuration used by transfer offer expiry/update logic.
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
- Note: `event_enqueued` exists in the schema/DTO but is intentionally non-authoritative until active blocking interaction persistence exists. It is an in-memory duplicate-enqueue guard for scheduler passes and is currently reset on save/load so a reload from a pre-match pause cannot permanently skip the match.

### `runtime_match_reports`

Stores match report header data for played matches: match id, league id, season year, match date, teams, matchweek, goals, coaches, formations, and starting player ids.

- Writer: `Game::persistRuntimeState`.
- Reader/restorer: `SqliteGameStateRepository::loadMatchReports`, then `League::restoreMatchReport`.
- Saved when: played match reports exist and runtime state is persisted.
- Authority: authoritative played match report summary for current visible save/load scope.
- Multi-league implication: reports include `league_id` and must be restored into the correct league context.
- Consistency rule: when present, a report must match a played fixture by match id, league id, teams, date, matchweek, and goals.

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
- Ordering: events are saved with `event_index` and restored in that deterministic order.

### `player_runtime_state`

Stores mutable player condition state: form, fitness, and morale.

- Writer: `Game::persistRuntimeState`.
- Reader/restorer: `Game::restoreRuntimeState`.
- Saved when: daily updates, matches, and runtime flushes persist state.
- Authority: authoritative persisted condition override over seed player data.
- Multi-league implication: player condition is saved for all team-owned players in all loaded league contexts and for `TransferRoom` free agents.

### `runtime_team_finances`

Stores mutable team budget snapshots: total budget, transfer budget, and wage budget for each league/team.

- Writer: `Game::persistRuntimeState`, via `SqliteGameStateRepository::saveRuntimeState`.
- Reader/restorer: `SqliteGameStateRepository::loadTeamFinanceStates`, then `Game::restoreRuntimeState` calls `Team::setBudgetSnapshot`.
- Saved when: runtime state is persisted, including accepted transfers and monthly wage/payment changes.
- Authority: authoritative mutable finance snapshot after load. Seed team budgets are bootstrap defaults only.
- Multi-league implication: rows are keyed by `league_id`/`team_id`; this is full-world state and is not managed-team-only.

### `runtime_player_roster_state`

Stores mutable player ownership and contract snapshot for team-owned players: player id, owning league/team, wage, contract years, and current season year.

- Writer: `Game::persistRuntimeState`, one row for every player currently owned by every team.
- Reader/restorer: `SqliteGameStateRepository::loadPlayerRosterStates`, then `Game::restoreRuntimeState` moves players between teams with `Team::releasePlayer` / `Team::addPlayer`.
- Saved when: runtime state is persisted, including accepted transfer roster moves and contract-year changes.
- Authority: authoritative runtime roster membership after load. Accepted transfer effects survive reload through this table, not by replaying accepted transfer offers.
- Validation note: bootstrap player/team ids are used only for existence checks. Current player ownership is validated from `runtime_player_roster_state` plus `runtime_free_agents`, and current checkpoint saves require every bootstrap player to appear exactly once across those ownership tables.
- Contract note: when wage/contract years are present, restore calls `Footballer::signContract`; partial contract snapshots are invalid.
- Team sheet note: after roster restore, selected `TeamSheet`s are validated and reconciled so transferred-away players are removed from starters/substitutes while preserving formation and tactical setup where possible. Managed-team sheets preserve manual gaps; AI/non-managed sheets are auto-filled.
- Multi-league implication: ownership is stored with both `league_id` and `team_id`, so cross-league moves have a durable destination.

### `runtime_free_agents`

Stores free agent pool ownership for players currently owned by `TransferRoom::freeAgents`: player id, optional previous league/team metadata, optional became-free-agent date, optional contract snapshot, and current season year.

- Writer: `Game::persistRuntimeState`, using `TransferRoom::getFreeAgents`.
- Reader/restorer: `SqliteGameStateRepository::loadFreeAgentStates`, then `Game::restoreRuntimeState` releases the player from any bootstrapped/restored team, sets `teamId = 0`, restores contract/stat context, and moves the object into `TransferRoom::freeAgents`.
- Saved when: runtime state is persisted, including season rollover free-agent collection and free-agent signing mutations.
- Authority: authoritative persisted free agent pool. Free agents are not represented by `team_id = 0` rows in `runtime_player_roster_state`.
- Ownership rule: a player must appear exactly once in either `runtime_player_roster_state` or `runtime_free_agents`, never both.
- Team sheet note: after free agent restore, selected `TeamSheet`s are reconciled using the existing managed-vs-AI policy so free agent players are removed. Managed-team gaps are preserved; AI sheets may auto-fill.

### `runtime_team_sheets`

Stores selected match squad headers per league/team: formation, mentality, and tempo.

- Writer: `Game::persistRuntimeState`.
- Reader/restorer: `SqliteGameStateRepository::loadTeamSheetStates`, then `Game::restoreRuntimeState`.
- Saved when: runtime state is persisted, including lineup editor changes and auto-select updates.
- Authority: `Team` owns the current/default selected `TeamSheet`. `Game` only orchestrates create/update/ensure/resolve flow and persistence snapshots. `save_metadata` must not store lineup or tactical state.
- Multi-league implication: rows are keyed by `league_id`/`team_id`; this is full-world state, not managed-team-only state.
- Roster reconciliation note: accepted transfers reconcile affected seller/buyer selected sheets before persisting, and load-time restore repairs any remaining invalid selected sheets after runtime roster restoration. Managed-team reconciliation removes invalid sold/transferred-away players but does not silently fill missing starters or substitutes; the Lineup Editor keeps those positions empty until the user assigns a player or presses Auto Select. AI/non-managed reconciliation auto-fills so those sheets remain current. The future managed-team UX should surface a "lineup requires attention" interaction before kickoff.
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

### `runtime_transfer_offers`

Stores transfer offer runtime state: offer id, created date, last valid date, expiry policy, seller/buyer league and team ids, player id, fee, status, and optional resolution.

- Writer: `SqliteGameStateRepository::saveRuntimeState`, using `TransferOfferService::exportOffers`.
- Reader/restorer: `SqliteGameStateRepository::loadTransferOfferStates`, then `TransferOfferService::restoreOffers`.
- Saved when: runtime state is persisted, including offer creation, accept/reject paths, and offer expiry paths invoked through `Game`.
- Authority: authoritative persisted transfer offer state. Pending offers survive load; accepted/rejected/expired offers restore as resolved and do not reappear as pending.
- Stable codes: expiry policy uses `fourteen_day_limit` / `window_close_limit`; status uses `pending` / `resolved`; resolution uses `accepted`, `rejected`, `expired_by_deadline`, or `expired_by_window_close`.
- ID recovery: restore replaces the in-memory offer map and sets `nextOfferId` to max persisted offer id + 1, so new offers created after load do not collide.
- Multi-league implication: seller and buyer league/team ids are persisted for every offer. The table is full-world state and is not scoped to the managed team.
- Scope note: accepted offer status/resolution persists separately from roster/budget state. Restore never replays accepted offers; `runtime_player_roster_state` and `runtime_team_finances` are the source of truth for accepted transfer effects. Active transfer decision UI state still requires Active Interaction Persistence.

## Validation Ownership

`RuntimeSaveValidator` owns runtime save validation in the core/data layer. It checks:

- `game_state` row exists and date is parseable.
- `league_runtime_state` exists.
- `runtime_fixtures` are present when fixture generation says they should be.
- `player_runtime_state` exists and may reference team-owned or free agent players.
- Fixture league and season match league runtime state.
- Fixture ids, matchweeks, team ids, played flags, result goals, and duplicate ids are valid.
- Match reports, when present, reference known leagues/teams/fixtures and match the played fixture result.
- Match player report rows reference known players and one of the two match teams, with structurally valid stat values.
- Match event rows reference known teams/players, valid event kinds, and deterministic persisted ordering.
- Runtime team sheet formation, slot roles, starter ids, substitutes, and tactical stable codes are valid.
- Runtime team sheets have no duplicate starters, duplicate substitutes, or starter/substitute overlap.
- Runtime team sheets do not exceed 10 substitutes.
- Runtime team sheets may be incomplete when structurally valid; missing managed-team assignments represent empty lineup slots and are not stored as `player_id = 0`.
- Runtime team finance rows reference known leagues/teams and have non-negative budgets.
- Runtime player roster rows reference known players/leagues/teams, have no duplicate player ids, and have valid contract snapshots.
- Runtime free agent rows reference known players, have no duplicate player ids, are disjoint from runtime roster rows, have valid optional previous-team metadata, and have valid optional contract/stat snapshots.
- `runtime_player_roster_state` plus `runtime_free_agents` covers every bootstrap player exactly once.
- Runtime team sheets do not reference players outside the team identified by `runtime_player_roster_state`; free agent players are invalid in TeamSheets.
- Runtime transfer offer ids, dates, fees, seller/buyer ids, player ids, stable codes, pending/resolved resolution rules, duplicate pending seller/buyer/player rows, and pending last-valid dates are valid.
- Current game date is within the SQL-loaded league season window for each league runtime row.
- Fixture dates fit their SQL-loaded league kickoff/season window.
- Metadata current date mirrors `game_state."current_date"` when metadata is provided.

Validation intentionally tolerates played fixtures without a detailed report for older checkpoint compatibility. Restore applies those rows through `League::restoreFixtureResult`; new match completions should normally have a matching `runtime_match_reports` row.

## Not Yet Persisted / Later Phases

### Pending/active blocking interactions

- Why it matters: closing during pre-match, post-match, or transfer decisions should restore the exact blocking state.
- Current behavior: reload resumes at dashboard/world state rather than the exact interaction screen.
- Suggested priority: deferred/low priority after match lifecycle audit, free-agent persistence, and UI/finance work unless a product blocker appears.
- Storage direction: interaction table with kind, league/team/match/offer ids, and payload version.
- Current phase rule: do not add active interaction tables yet.

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

### Deeper finance/contract systems

- Current state: team budget snapshots and player contract wage/year snapshots persist for team-owned players and free agents.
- Remaining work: richer finance ledgers, contract renewal UI, contract negotiation history, and long-term transaction reporting.
- Suggested priority: after active interaction persistence unless transfer gameplay exposes a blocker.
- Storage direction: transaction/ledger tables and explicit free-agent runtime state.

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
- Current state: transfer offer ids recover from max persisted offer id + 1; match ids are advanced from restored fixtures/reports.
- Current risk: any future runtime id counter must be advanced from persisted ids or stored explicitly.
- Suggested priority: as each runtime entity type is persisted.
- Storage direction: runtime counters table or deterministic max-id recovery per entity table.
