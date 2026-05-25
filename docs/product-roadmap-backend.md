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
- Permanent player attributes load from seed/static `player_attributes`, with deterministic `s1..s5` fallback for transitional seed compatibility. Runtime saves persist detailed attributes in `runtime_player_attributes` for rostered players and free agents; old saves without that table still load safely.
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
- Current behavior: the coordinate engine is now the default playable runtime path; lightweight simulation remains as fallback/compatibility.
- Next step: watched/background mode separation, tuning/stability, event richness, and UI playback preparation.
- Do not mix in: full coordinate engine implementation, mini-pitch UI, tactical UI, substitutions, injuries, save/load schema changes, or match result behavior changes.

### 12. Match Engine Core Types

- Status: implemented.
- Scope: add Qt-free core skeleton types for future coordinate simulation: `PitchPoint`, `PitchGeometry`, `MatchSimulationDetail`, trace frame DTOs, ball trajectory DTOs, and intent/action/contest enums.
- Current role: these types are consumed by the coordinate simulator. The lightweight `MatchSimulation` remains available as a fallback, and `League::applyMatchReport` remains the authoritative application path.
- Next step: keep expanding match event richness, watched/background behavior, and tuning.
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
- Current role: `MatchEngine::simulate` is the pure coordinate simulation entry point. It does not mutate domain objects, standings, fixtures, reports, history, UI, or save/load state.
- Future work: add `BallTrajectoryBuilder` and `InterceptionResolver` skeletons before implementing real coordinate simulation behavior.
- Do not mix in: full coordinate engine implementation, tactical effects, mini-pitch UI, fixture/standings/report application changes, save/load schema changes, or current match result behavior changes.

### 15. MatchEngineInputBuilder / Snapshot Builder

- Status: implemented.
- Scope: add the Qt-free read-only `MatchEngineInputBuilder` layer that converts existing `Team`, `Footballer`, and `TeamSheet` domain state into snapshot-based `MatchEngineInput` values. It copies team/player/team-sheet data, uses `TeamSheet.tacticalSetup` as the tactical source of truth, validates obvious structural roster and team-sheet issues, and fills a deterministic seed when the caller does not provide one.
- Current role: the builder feeds the default coordinate handler path with copied snapshots. It does not call `MatchEngine::simulate`, does not call `League::applyMatchReport`, and does not mutate domain, save/load, UI, standings, fixtures, reports, or event state.
- Future work: add `BallTrajectoryBuilder` and `InterceptionResolver` skeletons before implementing real coordinate simulation behavior.
- Do not mix in: full coordinate engine implementation, tactical effects, mini-pitch UI, fixture/standings/report application changes, save/load schema changes, or current match result behavior changes.

### 16. MatchEngine Internal State Skeleton

- Status: implemented.
- Scope: add Qt-free in-memory containers for future coordinate simulation state: `MatchSimulationState`, `PlayerSimState`, `TeamSimState`, `BallState`, and `PossessionState`, plus small linear search helpers.
- Current role: the in-memory state powers the coordinate simulator and remains unpersisted. It does not call `League::applyMatchReport` and does not replace `MatchReport`.
- Future work: add `BallTrajectoryBuilder` and `InterceptionResolver` skeletons before implementing real coordinate simulation behavior.
- Do not mix in: full coordinate engine implementation, movement resolution, player intent resolution, ball trajectory building, contest resolution, tactical effects, mini-pitch UI, fixture/standings/report application changes, save/load schema changes, or current match result behavior changes.

### 17. TeamShapeModel Skeleton

- Status: implemented.
- Scope: add a Qt-free tactical-positioning helper that converts `TeamSheet` starting assignments, `TacticalSetup`, pitch context, and attacking direction into `PlayerShapeTarget` values. The skeleton maps formation slots to 105m x 68m pitch coordinates, mirrors away-team shape, clamps targets to pitch boundaries, and applies simple mentality, width, and defensive-line adjustments.
- Current role: `TeamShapeModel` is consumed by the coordinate simulator. It does not replace `MatchSimulation` by itself, does not mutate domain or save state, and does not update UI, fixtures, standings, reports, or history.
- Future work: add `BallTrajectoryBuilder` and `InterceptionResolver` skeletons before implementing real coordinate simulation behavior.
- Do not mix in: full coordinate engine implementation, tactical effects in current match flow, mini-pitch UI, fixture/standings/report application changes, save/load schema changes, or current match result behavior changes.

### 18. BallTrajectoryBuilder + InterceptionResolver Skeleton

- Status: implemented.
- Scope: add Qt-free helper layers that build deterministic `BallTrajectory` values from intended targets, separate intended and actual target points through a simple execution-quality/pressure/seed error model, sample trajectories linearly, and evaluate possible path-interception candidates from defender positions.
- Current role: these helpers are consumed by the coordinate simulator. They do not replace `MatchSimulation` by themselves and do not mutate player state, ball state, domain objects, fixtures, standings, reports, history, save/load state, or UI.
- Follow-on work: action-planning helpers and `ContestResolver` now exist; tune the coordinate simulator and enrich events.
- Do not mix in: final contest resolution, full coordinate engine implementation, tactical effects in current match flow, mini-pitch UI, fixture/standings/report application changes, save/load schema changes, or current match result behavior changes.

### 19. ActionPlan / ActionCandidate / PlayerIntent Skeleton

- Status: implemented.
- Scope: add Qt-free helper layers for future ball-carrier planning: `ActionPlan` types, reassessment trigger evaluation, `PerceptionModel` option awareness, basic `ActionCandidateGenerator` output, and weighted deterministic `ActionSelector` choice.
- Current role: these helpers are consumed by the coordinate simulator. They do not replace `MatchSimulation` by themselves and do not mutate player state, ball state, domain objects, fixtures, standings, reports, history, save/load state, or UI.
- Follow-on work: `ContestResolver` now exists; tune movement/intent behavior inside the coordinate simulator.
- Do not mix in: contest resolution, movement resolution, full coordinate engine implementation, tactical effects in current match flow, mini-pitch UI, fixture/standings/report application changes, save/load schema changes, or current match result behavior changes.

### 20. ContestResolver Skeleton

- Status: implemented.
- Scope: add a Qt-free local contest outcome resolver with copied `ContestParticipant` inputs, deterministic attribute/timing/context scoring, weighted deterministic contest winner selection, explicit winner/loser and winning side output, and separate ball-outcome reporting for clean control, deflections, loose balls, and shot continuation.
- Current role: `ContestResolver` is consumed by the coordinate simulator. It does not replace `MatchSimulation` by itself and does not mutate player state, ball state, domain objects, fixtures, standings, reports, history, save/load state, or UI.
- Semantics note: contest/action winner is separate from post-action ball control. Clean interceptions, deflections, loose balls, defender-won tackles with loose/deflected outcomes, and goalkeeper save-and-hold/save-and-parry outcomes are represented without assigning future recovery.
- Goalkeeper note: `GoalkeeperSave` can save-and-hold, save-and-parry, leave a loose rebound, or be beaten. Local simulator goal/stat/trace handling consumes this outcome in `CoordinateMatchSimulator`; report application remains owned by the handler and `League::applyMatchReport`.
- Future work: TacticalZone, DefensiveResponsibility, MovementResolver, PlayerIntentResolver, and deflected trajectory helpers now exist; the next step is tuning and event richness.
- Do not mix in: movement resolution, player intent resolution, full coordinate engine implementation, tactical effects in current match flow, mini-pitch UI, fixture/standings/report application changes, save/load schema changes, or current match result behavior changes.

### 21. Tactical Zones + Player Intent Resolver + Movement Resolver Skeleton

- Status: implemented.
- Scope: add Qt-free helper layers for the next coordinate-engine foundation: `TacticalZone` as a 3x3 attacking-perspective zone model, `DefensiveResponsibility` as role/zone/pressable-opponent responsibility data, `PlayerIntentResolver` as deterministic role/zone/distance constrained intent selection, `MovementResolver` as non-mutating movement toward an intent target over `deltaSeconds`, and a deterministic deflected-ball trajectory helper on `BallTrajectoryBuilder`.
- Current role: these helpers are consumed by the coordinate simulator. They do not replace `MatchSimulation` by themselves, do not apply authoritative world state, and do not mutate player state, domain objects, fixtures, standings, reports, history, save/load state, or UI.
- Semantics note: `BallCarrierActionType` is the current ball carrier decision enum. `PlayerIntentType` is the off-ball, defensive, loose-ball reaction, and emergency intent enum. Marking style affects willingness but does not override role/zone eligibility for pressing.
- Future work: tune watched/background behavior and event richness.
- Do not mix in: full coordinate engine implementation, tactical effects in current match flow, mini-pitch UI, live playback, shots/goals/cards/injuries/reports, fixture/standings/report application changes, save/load schema changes, or current match result behavior changes.

### 22. Minimal coordinate match simulator

- Status: implemented as a matchSecond/action-duration coordinate simulator.
- Scope: add `CoordinateMatchSimulator` behind valid `MatchEngine::simulate` input. The simulator initializes local `MatchSimulationState` from snapshots, places starters through `TeamShapeModel`, resolves intents and movement, selects controlled-ball actions, builds trajectories, handles minimal in-flight/deflected/loose ball outcomes, advances the match clock by action/trajectory/race/restart durations until regulation time, and returns simulation stats/events/traces through `MatchEngineResult`.
- Current behavior: the simulator powers watched/debug coordinate detail through `MatchEngine::simulate`; `BackgroundSummary` is routed to the fast aggregate coordinate summary. It creates an optional `MatchReport` but still does not apply it, call `League::applyMatchReport`, or mutate domain objects, fixtures, standings, reports, history, save/load state, or UI.
- Future work: deeper tuning and UI playback preparation.
- Do not mix in: mini-pitch UI, fixture/standing/report application changes, or final tuning.

### 23. Shot / Save / Goal local simulator + Ball Vertical Profile

- Status: implemented as a matchSecond/action-duration coordinate simulator.
- Scope: add `BallFlightProfile` and apex height to `BallTrajectory`, expose simple ball-height helpers, map trajectory types to ground/low/high/lofted/shot profiles, use temporary attribute-based reach checks for high balls and aerial situations, route high crosses/clearances toward aerial contest handling, and resolve on-target local simulator shots through `GoalkeeperSave`.
- Current behavior: simulator saves, rebounds, and goals update `MatchEngineResult` stats and official events, with watched/debug trace markers kept separate. Report application still happens only in `PlayMatchCommandHandler` via `League::applyMatchReport`.
- Future work: richer event metadata and UI playback preparation.
- Do not mix in: `League::applyMatchReport` changes, mini-pitch UI, fixture/standing/report application changes, or final tuning.

### 24. MatchEngineResult -> MatchReport Adapter

- Status: implemented as a non-runtime conversion layer.
- Scope: add a Qt-free pure adapter that builds `MatchReport` from `MatchEngineInput` and `MatchEngineResult`. It maps match metadata, season year, score, lineup snapshots, player report basics including ratings, official events including assists, and basic home/away team stats.
- Current behavior: the adapter feeds the default coordinate match flow. Trace frames are presentation/debug data and are no longer the authoritative source for official report events. The adapter does not apply the report to `League`, game state, fixtures, standings, history, save/load, domain events, or UI.
- Future work: richer official event metadata and UI playback preparation.
- Do not mix in: `League::applyMatchReport` changes, mini-pitch UI, or fixture/standing/report application changes.

### 25. First Playable Coordinate Match Flow

- Status: implemented.
- Scope: make `Coordinate` the default `PlayMatchCommandHandler` mode, rename engine mode and simulator terminology away from old experimental wording, keep explicit `Lightweight` mode, validate coordinate reports, and persist detailed `PlayerAttributes` through runtime save/load. `fm_match_engine_smoke` now covers deterministic result/report behavior, report metadata consistency, starter player reports, invalid-input defaults, background/watched/debug no-crash execution, default handler mode selection, fixture/standings/report application, `MatchPlayedEvent` publication, and explicit lightweight mode.
- Current behavior: normal handler construction now tries the coordinate engine first. `Coordinate` mode builds input with `MatchEngineInputBuilder`, runs `MatchEngine`, uses `result.report` only when ids, metadata, goals, lineup team ids, and player-report team ids are safe, and falls back to lightweight simulation when unsafe. `Lightweight` remains a compatibility mode.
- Authoritative apply: `League::applyMatchReport` remains the only match application point, followed by condition effects and `MatchPlayedEvent` publication.
- Save/load: detailed `PlayerAttributes` are persisted in runtime saves for rostered players and free agents. Old saves without detailed runtime attributes still load and keep seed/bootstrap or legacy `s1..s5` fallback attributes.
- Do not mix in: UI settings, new domain events, removal of `MatchSimulation`, or changes to `League::applyMatchReport` semantics.

### 26. MatchSecond Coordinate Loop + Report Stats

- Status: implemented.
- Scope: replace the temporary fixed action-count coordinate loop with a regulation match clock. Controlled actions, in-flight balls, loose-ball races, and goal restarts now return elapsed simulated seconds. Official match events are stored separately from debug trace, and `MatchReport` now carries basic team stats.
- Current behavior: `WatchedMatch` and `DebugFullTrace` use the full action-duration coordinate simulator. Goal scorers are official events rather than trace-derived, post-match statistics read from `MatchReport`, and saved reports persist basic team stats while old saves without those columns remain safe.
- Do not mix in: full real-time physics, stoppage time, live QML playback, lightweight removal, or changes to `League::applyMatchReport` semantics.

### 27. Fast Background Summary + First Tuning Guardrails

- Status: implemented as a PR86 follow-up.
- Scope: route `BackgroundSummary` to `FastMatchSummarySimulator`, keep `WatchedMatch`/`DebugFullTrace` on the full `CoordinateMatchSimulator`, add first-pass shot/scoring guardrails to the detailed loop, track simple assists, carry player match ratings into `MatchReport` and UI-facing lineup data, and expand smoke/calibration checks.
- Current behavior: background fixtures use a deterministic aggregate coordinate summary based on the same snapshots, detailed attributes, starting XI roles/formation, and tactical setup. It emits official events, team stats, player goals/assists/minutes/ratings, and no marker trace. Watched/debug matches remain available through the full coordinate action-duration loop.
- Do not mix in: QML redesign, lightweight removal, final tuning, or changes to `League::applyMatchReport` semantics.

## Next Backend Phases

1. Tuning/stability pass
2. Match event richness and UI playback preparation
3. Managed-match watched-mode policy refinement
4. Eventual lightweight deprecation/cleanup after confidence

## Deferred / Later Backend Work

- Active Interaction Persistence.
- Rolling autosave slots.
- Save As / multiple named saves.
- Deep finance ledgers, debt/liabilities, revenue and expense streams, finance UI, transfer AI, player valuation, and richer contract systems.
- Completed season archives.
- Broader data editor/scouting systems.
