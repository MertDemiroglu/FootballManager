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
- `runtime_player_roster_state` stores team-owned players only; `runtime_free_agents` stores free agent pool ownership.
- Permanent player attributes are seed/static data for now. `player_attributes` is the preferred explicit source, with deterministic `s1..s5` fallback for transitional seed compatibility.
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
- Implemented shape: `runtime_team_finances` stores full-world team finance snapshots; `runtime_player_roster_state` stores team-owned player ownership, contract wage/years, and current season year.

### 5. Save Backend Runtime Persistence

- Status: largely complete for the current checkpoint model.
- Persisted today: game date, fixtures, match results/reports, player condition, selected `TeamSheet`s, TacticalSetup V1 fields, substitutes, LeagueRules config, transfer offers, accepted transfer roster movement, `TeamFinance`, and contract snapshots.
- Current behavior: Continue/Load resumes through Dashboard/safe checkpoint rather than restoring exact transient modals.
- Remaining risk: full runtime snapshots may become costly with broader multi-league worlds; dirty/incremental save work can be considered later.

### 6. Manual Save + Auto Save Policy

- Status: implemented.
- Scope: Dashboard Manual Save, autosave frequency settings, game-date scheduled autosave, `runtime_save_settings`, save request batching/coalescing, and documentation cleanup.
- Default autosave frequency: Weekly.
- Stable options: `manual_only`, `daily`, `every_3_days`, `weekly`, `every_2_weeks`.
- Do not mix in: active interaction persistence, Save As, rolling autosave slots, dirty/incremental saves, Dashboard redesign, match engine behavior, transfer logic, or LeagueRules migration.

### 7. Match Lifecycle / Standings / History Audit

- Status: implemented.
- Scope: verify the existing fixture -> pre-match -> match application -> played fixture/result -> report -> standings/history/recent form -> save checkpoint flow.
- Current verified shape: `Game` orchestrates; `MatchScheduler` queues league-scoped commands; `PlayMatchCommandHandler` applies the match; `League::applyMatchReport` is the authoritative completion path for fixture/result/report/standings/current-season history.
- `event_enqueued` remains a transient scheduler guard, not authoritative persisted active-interaction state.
- Do not mix in: match engine rewrite, tactical effects, active interaction persistence, free-agent persistence, completed season archives, multi-league UI, rolling autosaves, or Save As.

### 8. Free Agent Persistence

- Status: implemented.
- Scope: persist `TransferRoom::freeAgents` through `runtime_free_agents`, restore those players into TransferRoom ownership, keep `runtime_player_roster_state` team-owned only, and validate disjoint player ownership.

### 9. Finance Foundation

- Status: implemented.
- Scope: replace primitive `Team` budget fields with `TeamFinance`, which owns cash balance, transfer budget, annual wage budget limit, `ClubFinancialStrategy`, and `ClubFinancialHealth`.
- Behavior: allocation is two-stage and happens when budgets are initialized/reallocated, not dynamically on every cash change. Health determines the sporting allocation envelope from cash balance; strategy splits that envelope between wage and transfer budget. Remaining cash stays as operating reserve/future expenses/profit buffer.
- Strategy model: supported strategies are Balanced, DevelopmentFocused, StarFocused, and ValueTrading. `Conservative` was removed because financial caution belongs to health, not strategy; all strategy wage/transfer splits now total 100%.
- Transfer behavior: paid transfers spend cash and transfer budget. Sales increase cash by the full fee and increase transfer budget by final sale retention, calculated from health base retention plus strategy modifier clamped to 10%-100%. Wage affordability derives current annual wage spend from contracts.
- Save mapping: `runtime_team_finances.total_budget` maps to cash balance, `transfer_budget` to transfer budget, `wage_budget` to wage budget limit, `financial_strategy` to the stable strategy code, and `financial_health` to the stable health code.
- Future tuning: strategy changes, direct custom wage/transfer sliders, and board-approved adjustment bands are future board/manager-trust work, not part of this PR.
- Do not mix in: finance UI, transfer AI, player valuation, deep ledgers, debt/liabilities, ticket/sponsor/prize/shirt revenue streams, stadium/wage/debt/general operating expense streams, taxes, scouting/data editor work, match engine changes, rolling autosaves, or Save As.

### 10. Player Attribute Model + Match Engine Design Foundation

- Status: implemented.
- Scope: add `PlayerAttributes` with Technical, Mental, Physical, and Goalkeeper groups on a 0-100 core scale; make `Footballer` own attributes; route base `totalPower()` through position-weighted attributes; add SQL `player_attributes`; keep legacy `s1..s5` fallback for current seed compatibility.
- Position fit: `PlayerRoleFit` remains the current primary-position based fit system. Player-specific position familiarity is future work.
- Design docs: `docs/player-attribute-model.md` and `docs/match-engine-design.md`.
- Do not mix in: real match engine implementation, mini-pitch UI, live event stream, player development, scouting, valuation overhaul, transfer AI, or per-player position familiarity tables.

### 11. Match Engine Coordinate Simulation Design

- Status: documented.
- Scope: define the future lightweight real-football simulation model: real pitch dimensions, possession-based simulation steps, tactical shape, action plans, perception, action candidates, intended-vs-actual ball trajectories, path interception, arrival contests, local contests, watched traces, and background summaries.
- V1 tactical inputs needed by the future engine: Mentality, Tempo, Width, DefensiveLine, PressingIntensity, MarkingStyle, and PassingDirectness.
- Current behavior: no runtime engine implementation was added; current match result behavior, save/load behavior, and UI behavior remain unchanged.
- Next step: create the `MatchEngine` interface/skeleton and output contracts before implementing the full simulation engine.
- Do not mix in: full coordinate engine implementation, mini-pitch UI, tactical UI, substitutions, injuries, save/load schema changes, or match result behavior changes.

### 12. Match Engine Core Types

- Status: implemented.
- Scope: add Qt-free core skeleton types for future coordinate simulation: `PitchPoint`, `PitchGeometry`, `MatchSimulationDetail`, trace frame DTOs, ball trajectory DTOs, and intent/action/contest enums.
- Current behavior: runtime match behavior is unchanged. The existing lightweight `MatchSimulation`, `PlayMatchCommandHandler`, `League::applyMatchReport`, UI, and save/load paths are not integrated with these types yet.
- Next step: add the `MatchEngine` interface/skeleton and output contracts before any real simulation behavior.
- Do not mix in: match engine implementation, player movement simulation, tactical UI, mini-pitch UI, save/load schema changes, or match result behavior changes.

### 13. TacticalSetup V1 Expansion

- Status: implemented.
- Scope: expand Qt-free `TacticalSetup` with Mentality, Tempo, Width, DefensiveLine, PressingIntensity, MarkingStyle, and PassingDirectness; add stable codes/display text; persist the fields on `runtime_team_sheets`.
- Current behavior: existing mentality/tempo UI remains compatible. New tactical fields default on TeamSheets and persist through save/load, but do not affect current match results.
- Future work: More Options UI exposure, coach-driven full tactical identity, tactical AI, and real match engine effects.
- Do not mix in: match engine implementation, mini-pitch UI, tactical effects, tactical AI, QML gameplay ownership, or match result behavior changes.

### 14. MatchEngine Interface Skeleton

- Status: implemented.
- Scope: add the Qt-free `MatchEngine` boundary, snapshot-based `MatchEngineInput`, team/player snapshot DTOs, output-only `MatchEngineResult`, future team/player simulation stats, and optional trace/report containers.
- Current behavior: runtime match behavior is unchanged. The skeleton `MatchEngine::simulate` is not wired into `PlayMatchCommandHandler`, does not call the existing `MatchSimulation`, does not apply reports, and does not mutate domain objects, standings, fixtures, reports, history, UI, or save/load state.
- Future work: add `BallTrajectoryBuilder` and `InterceptionResolver` skeletons before implementing real coordinate simulation behavior.
- Do not mix in: full coordinate engine implementation, tactical effects, mini-pitch UI, fixture/standings/report application changes, save/load schema changes, or current match result behavior changes.

### 15. MatchEngineInputBuilder / Snapshot Builder

- Status: implemented.
- Scope: add the Qt-free read-only `MatchEngineInputBuilder` layer that converts existing `Team`, `Footballer`, and `TeamSheet` domain state into snapshot-based `MatchEngineInput` values. It copies team/player/team-sheet data, uses `TeamSheet.tacticalSetup` as the tactical source of truth, validates obvious structural roster and team-sheet issues, and fills a deterministic seed when the caller does not provide one.
- Current behavior: runtime match behavior is unchanged. The builder is not wired into `PlayMatchCommandHandler`, does not call `MatchEngine::simulate`, does not call `League::applyMatchReport`, and does not mutate domain, save/load, UI, standings, fixtures, reports, or event state.
- Future work: add `BallTrajectoryBuilder` and `InterceptionResolver` skeletons before implementing real coordinate simulation behavior.
- Do not mix in: full coordinate engine implementation, tactical effects, mini-pitch UI, fixture/standings/report application changes, save/load schema changes, or current match result behavior changes.

### 16. MatchEngine Internal State Skeleton

- Status: implemented.
- Scope: add Qt-free in-memory containers for future coordinate simulation state: `MatchSimulationState`, `PlayerSimState`, `TeamSimState`, `BallState`, and `PossessionState`, plus small linear search helpers.
- Current behavior: runtime match behavior is unchanged. The state skeleton is not persisted, not wired into `PlayMatchCommandHandler`, does not call `MatchEngine::simulate`, does not call `League::applyMatchReport`, and does not replace `MatchReport`.
- Future work: add `BallTrajectoryBuilder` and `InterceptionResolver` skeletons before implementing real coordinate simulation behavior.
- Do not mix in: full coordinate engine implementation, movement resolution, player intent resolution, ball trajectory building, contest resolution, tactical effects, mini-pitch UI, fixture/standings/report application changes, save/load schema changes, or current match result behavior changes.

### 17. TeamShapeModel Skeleton

- Status: implemented.
- Scope: add a Qt-free tactical-positioning helper that converts `TeamSheet` starting assignments, `TacticalSetup`, pitch context, and attacking direction into `PlayerShapeTarget` values. The skeleton maps formation slots to 105m x 68m pitch coordinates, mirrors away-team shape, clamps targets to pitch boundaries, and applies simple mentality, width, and defensive-line adjustments.
- Current behavior: runtime match behavior is unchanged. `TeamShapeModel` is not wired into `PlayMatchCommandHandler`, does not call `MatchEngine::simulate`, does not replace `MatchSimulation`, does not mutate domain or save state, and does not update UI, fixtures, standings, reports, or history.
- Future work: add `BallTrajectoryBuilder` and `InterceptionResolver` skeletons before implementing real coordinate simulation behavior.
- Do not mix in: full coordinate engine implementation, tactical effects in current match flow, mini-pitch UI, fixture/standings/report application changes, save/load schema changes, or current match result behavior changes.

### 18. BallTrajectoryBuilder + InterceptionResolver Skeleton

- Status: implemented.
- Scope: add Qt-free helper layers that build deterministic `BallTrajectory` values from intended targets, separate intended and actual target points through a simple execution-quality/pressure/seed error model, sample trajectories linearly, and evaluate possible path-interception candidates from defender positions.
- Current behavior: runtime match behavior is unchanged. These helpers are not wired into `PlayMatchCommandHandler`, do not call `MatchEngine::simulate`, do not replace `MatchSimulation`, do not mutate player state, ball state, domain objects, fixtures, standings, reports, history, save/load state, or UI.
- Follow-on work: action-planning helpers and `ContestResolver` now exist; add Movement/Intent resolver skeletons before implementing a minimal coordinate simulation prototype.
- Do not mix in: final contest resolution, full coordinate engine implementation, tactical effects in current match flow, mini-pitch UI, fixture/standings/report application changes, save/load schema changes, or current match result behavior changes.

### 19. ActionPlan / ActionCandidate / PlayerIntent Skeleton

- Status: implemented.
- Scope: add Qt-free helper layers for future ball-carrier planning: `ActionPlan` types, reassessment trigger evaluation, `PerceptionModel` option awareness, basic `ActionCandidateGenerator` output, and weighted deterministic `ActionSelector` choice.
- Current behavior: runtime match behavior is unchanged. These helpers are not wired into `PlayMatchCommandHandler`, do not call `MatchEngine::simulate`, do not replace `MatchSimulation`, do not execute selected actions, and do not mutate player state, ball state, domain objects, fixtures, standings, reports, history, save/load state, or UI.
- Follow-on work: `ContestResolver` now exists; add MovementResolver / PlayerIntentResolver before implementing a minimal coordinate simulation prototype.
- Do not mix in: contest resolution, movement resolution, full coordinate engine implementation, tactical effects in current match flow, mini-pitch UI, fixture/standings/report application changes, save/load schema changes, or current match result behavior changes.

### 20. ContestResolver Skeleton

- Status: implemented.
- Scope: add a Qt-free local contest outcome resolver with copied `ContestParticipant` inputs, deterministic attribute/timing/context scoring, explicit winner/loser and winning side output, and resolution flags for attacking success, possession changes, loose balls, and deflections.
- Current behavior: runtime match behavior is unchanged. `ContestResolver` is not wired into `PlayMatchCommandHandler`, does not call `MatchEngine::simulate`, does not replace `MatchSimulation`, does not mutate player state, ball state, domain objects, fixtures, standings, reports, history, save/load state, or UI.
- Goalkeeper note: `GoalkeeperSave` can return `KeeperSaved` or `ShotBeatsKeeper`, but goals and score/report application remain future shot/save/goal prototype work.
- Future work: add MovementResolver + PlayerIntentResolver skeletons before implementing a minimal coordinate simulation prototype.
- Do not mix in: movement resolution, player intent resolution, full coordinate engine implementation, tactical effects in current match flow, mini-pitch UI, fixture/standings/report application changes, save/load schema changes, or current match result behavior changes.

## Next Backend Phases

1. MovementResolver + PlayerIntentResolver skeleton
2. Minimal Coordinate Simulation Prototype
3. Transfer World design
4. Automated regression tests when stable enough
5. Team screen / Transfer room / Finance UI rework
6. Multi-league expansion preparation

## Deferred / Later Backend Work

- Active Interaction Persistence.
- Rolling autosave slots.
- Save As / multiple named saves.
- Deep finance ledgers, debt/liabilities, revenue and expense streams, finance UI, transfer AI, player valuation, and richer contract systems.
- Completed season archives.
- Broader data editor/scouting systems.
