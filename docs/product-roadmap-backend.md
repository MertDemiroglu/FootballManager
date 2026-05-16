# Backend Product Roadmap

This is the high-level backend roadmap. It keeps save/load and core gameplay work scoped, while the living detail documents carry current-state notes and UI planning.

Related documents:

- `docs/core-current-state-roadmap.md` is the detailed backend/current-state living document.
- `docs/ui-current-state-roadmap.md` is the UI current-state and UI roadmap living document.
- `docs/manual-save-load-regression.md` is a practical manual regression checklist, not the main planning document.
- `docs/save-load-state-map.md` remains the save/load ownership and persistence map.

## Current Save Direction

- Save/load is checkpoint-based. The game persists safe runtime snapshots, not exact transient popup or screen state.
- The app may start from Main Menu. Continue and Load Game enter normal gameplay through Dashboard/current safe checkpoint.
- `SaveMetadata` remains save-card display/cache state only. Autosave settings live in `runtime_save_settings`.
- `runtime_save_settings.last_auto_save_date` is currently used as the last saved checkpoint date for scheduled autosave calculations, so manual and critical saves may update it too.
- TODO: consider renaming `last_auto_save_date` to `last_checkpoint_save_date` in a future schema cleanup.
- Manual Save overwrites the current save slot. Scheduled autosave also overwrites the current save slot.
- Autosave is game-date interval based, defaults to Weekly, and does not create rolling slots.
- Important events may request a save, but full snapshot writes should be batched/coalesced at safe checkpoints where possible.
- The current save format is still a full runtime snapshot. Dirty/incremental table-level saves are a future optimization if multi-league snapshot cost becomes high.
- Active interaction exact restore is intentionally deferred. Do not add `runtime_active_interaction` tables in the current phase.
- Save As, multiple named manual saves, and rolling autosave slots are not planned for this phase.

## Completed Backend Phases

### 0. State Ownership / Legacy Audit

- Status: implemented.
- Verified shape: `Game` is an orchestrator; `Team` owns selected/current `TeamSheet`; `HeadCoach` owns `TacticalPreferences`; `SaveMetadata` remains save-card display/cache state only; QML remains presentation-only.

### 1. Selected Lineup and Tactic Persistence

- Status: implemented in runtime save state.
- Implemented shape: `runtime_team_sheets`, `runtime_team_sheet_starters`, and `runtime_team_sheet_substitutes`, keyed by `league_id`/`team_id`.
- Active interaction note: pending pre-match sheets are match-specific frozen snapshots and should move into active interaction persistence later.

### 2. LeagueRules / SeasonConfig SQL Migration

- Status: implemented for the current Super Lig seed.
- Implemented shape: `league_rules`, `league_transfer_windows`, and `league_matchday_distribution_offsets`.
- Remaining cleanup: `LeagueRules` still exposes function fields as a transitional API; it should eventually become fully value-config based.

### 3. Transfer Offer Persistence

- Status: implemented for offer state.
- Implemented shape: `runtime_transfer_offers` stores full-world offer rows with stable expiry/status/resolution codes.

### 4. Roster Mutation Persistence

- Status: implemented for team-owned roster moves, team budget snapshots, and player contract snapshots.
- Implemented shape: `runtime_team_finances` stores full-world team budgets; `runtime_player_roster_state` stores player ownership, contract wage/years, and current season year.

### 5. Save Backend Runtime Persistence

- Status: largely complete for the current checkpoint model.
- Persisted today: game date, fixtures, match results/reports, player condition, selected `TeamSheet`s, tactics, substitutes, LeagueRules config, transfer offers, accepted transfer roster movement, budgets, and contract snapshots.
- Current behavior: Continue/Load resumes through Dashboard/safe checkpoint rather than restoring exact transient modals.
- Remaining risk: full runtime snapshots may become costly with broader multi-league worlds; dirty/incremental save work can be considered later.

## Active Backend Phase

### 6. Manual Save + Auto Save Policy

- Status: active PR phase.
- Scope: Dashboard Manual Save, autosave frequency settings, game-date scheduled autosave, `runtime_save_settings`, save request batching/coalescing, and documentation cleanup.
- Default autosave frequency: Weekly.
- Stable options: `manual_only`, `daily`, `every_3_days`, `weekly`, `every_2_weeks`.
- Do not mix in: active interaction persistence, Save As, rolling autosave slots, dirty/incremental saves, Dashboard redesign, match engine behavior, transfer logic, or LeagueRules migration.

## Next Backend Phases

1. Match Lifecycle / Standings / History Audit
2. Free Agent Persistence
3. Automated regression tests when stable enough
4. Multi-league expansion preparation
5. Match engine/tactical effects later

## Deferred / Later Backend Work

- Active Interaction Persistence.
- Rolling autosave slots.
- Save As / multiple named saves.
- Deep finance/contract systems.
- Completed season archives.
- Broader data editor/scouting systems.
