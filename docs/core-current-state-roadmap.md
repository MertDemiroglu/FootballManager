# Core Current State Roadmap

This living document describes the current core/backend shape and the next backend priorities. Keep detailed planning here; keep `manual-save-load-regression.md` as a test checklist.

## Current Core Architecture

- `Game` is the orchestrator for simulation flow, save checkpoints, user selection, interaction routing, date advancement, and high-level domain service calls.
- `World` owns `LeagueContext`s and cross-league services such as transfer rooms, transfer offers, id allocation, and domain event publishing.
- `LeagueContext` owns league-scoped orchestration: rules, season plan, fixture/match command handling, and rollover guard state.
- `League` owns competition state: teams, fixtures, standings/projections, current season history, and match reports.
- `Team` owns roster membership, player index, colors, coach relation, a `TeamFinance` snapshot, and the selected/current `TeamSheet`.
- `HeadCoach` owns `TacticalPreferences`, which are coach/team defaults rather than a concrete match squad.
- `TeamSheet` owns formation, starting XI slot assignments, substitutes, and active `TacticalSetup`.
- `TacticalSetup` V1 contains Mentality, Tempo, Width, DefensiveLine, PressingIntensity, MarkingStyle, and PassingDirectness. These are match-squad inputs for future coordinate simulation, not current result modifiers.
- `Footballer` owns permanent `PlayerAttributes` on a 0-100 core scale. Condition, form, and morale remain runtime state in `PlayerConditionState`.
- QML/UI must not own gameplay source-of-truth. Gameplay state comes from core models exposed through `GameFacade`.

## Implemented Backend Systems

- Checkpoint-based save/load.
- Full-world/all-leagues runtime save model.
- `SaveMetadata` as display/cache state only.
- LeagueRules SQL-backed for the current Super Lig seed.
- Fixtures, results, and match reports persistence.
- Match lifecycle is owned by core: scheduler creates league-scoped commands, `PlayMatchCommandHandler` applies reports, and `League` updates fixture/result/standings/history together.
- Player condition persistence.
- `TeamSheet`, tactics, and substitutes persistence, including TacticalSetup V1 fields on `runtime_team_sheets`.
- Transfer offer persistence.
- Accepted transfer roster movement, centralized `TeamFinance`, and contract snapshot persistence.
- Free agent pool persistence through `runtime_free_agents`.
- Finance Foundation: `TeamFinance` owns cash balance, transfer budget, annual wage budget limit, `ClubFinancialStrategy`, and `ClubFinancialHealth`.
- Player Attribute Model foundation: Technical, Mental, Physical, and Goalkeeper attributes, attribute-based base `totalPower()`, SQL `player_attributes` support, and deterministic legacy `s1..s5` fallback.
- Match Engine Coordinate Simulation Design documented in `docs/match-engine-design.md`; the real match engine implementation remains future work.
- Match Engine Core Types foundation: Qt-free pitch geometry, trace, trajectory, intent, action, and contest DTO/enums for future coordinate simulation.
- TacticalSetup V1 Expansion: TeamSheet tactics now persist Mentality, Tempo, Width, DefensiveLine, PressingIntensity, MarkingStyle, and PassingDirectness. Current match behavior is unchanged and More Options UI exposure is future work.
- MatchEngine interface/skeleton: Qt-free snapshot input DTOs, output-only result DTOs, and a non-integrated `MatchEngine::simulate` boundary exist for future coordinate simulation.
- MatchEngineInputBuilder: Qt-free snapshot builder creates `MatchEngineInput` values from existing teams and team sheets, fills a deterministic seed when missing, and validates obvious structural roster/team-sheet issues. It is not integrated into runtime match play yet.
- MatchEngine internal state skeleton: Qt-free `MatchSimulationState`, `PlayerSimState`, `TeamSimState`, `BallState`, and `PossessionState` containers exist for future in-memory coordinate simulation. They are not persisted and are not integrated into runtime match play yet.
- TeamShapeModel skeleton: Qt-free tactical-positioning helper converts TeamSheet starters and TacticalSetup context into clamped pitch target positions. It applies only small mentality, width, and defensive-line adjustments and is not integrated into runtime match play yet.
- BallTrajectoryBuilder + InterceptionResolver skeleton: Qt-free helper layers now build deterministic ball trajectories from intended targets, keep intended and actual targets distinct through a simple quality/pressure/seed error model, sample trajectories linearly, and produce path-interception candidates without resolving final contest outcomes or changing runtime match behavior.
- ActionPlan / ActionCandidate / PlayerIntent skeleton: Qt-free planning helpers now represent ball-carrier plans, reassessment triggers, option perception, basic action candidate generation, and deterministic weighted action selection. They are not integrated into runtime match play yet.
- Manual save and autosave policy.
- Managed-vs-AI `TeamSheet` reconciliation policy after roster restore/mutation.

## Current Save Model

- Save/load is checkpoint-based.
- Starting from Main Menu is acceptable.
- Continue/Load enters Dashboard/current safe checkpoint.
- Exact active popup/screen restore is deferred.
- Manual Save overwrites the current save slot.
- Scheduled autosave overwrites the current save slot.
- Default autosave frequency is Weekly.
- Important save requests can be coalesced before a safe checkpoint flush.
- Team-owned player ownership is stored in `runtime_player_roster_state`; free agent ownership is stored separately in `runtime_free_agents`.
- Team finance is stored in `runtime_team_finances`: `total_budget` maps to cash balance, `transfer_budget` to the active transfer allocation, `wage_budget` to the annual wage budget limit, `financial_strategy` to the stable strategy code, and `financial_health` to the stable health code.
- Permanent player attributes are seed/static data. `player_attributes` is the preferred explicit data source, while legacy `players.s1..s5` remains a transitional fallback. Runtime saves do not store player attribute progression yet.
- Finance allocation is two-stage. `ClubFinancialHealth` determines how much cash can be allocated to football spending, then `ClubFinancialStrategy` splits that sporting envelope between wage and transfer budget. Remaining cash is operating reserve, future expenses, or profit buffer.
- Transfer sale revenue increases cash by the full fee and increases transfer budget by health-based retention plus the strategy modifier; financially strong clubs can retain up to 100% for reinvestment.
- `Conservative` is not a strategy; financial caution belongs to `ClubFinancialHealth`. Direct wage/transfer sliders and runtime strategy changes are future board/manager-trust work.
- Current wage spend is derived from player contracts and is not persisted as a team finance field.
- Wage payments currently reduce cash balance and the MVP avoids negative cash; debt/liability handling is a future finance phase.
- Current save format is a full runtime snapshot.
- Dirty/incremental saves are a future optimization if multi-league snapshot cost becomes high.

## Current Match Lifecycle

- `Game::updateDaily` is the orchestrator. It asks `MatchScheduler` to scan every `LeagueContext` for fixtures on the current game date.
- `MatchScheduler` emits league-scoped `PlayMatchCommand`s and sets fixture `eventEnqueued` only as an in-memory duplicate-enqueue guard.
- Managed-team matches become pre-match interactions through `Game`; QML only presents the interaction exposed by `GameFacade`.
- `Game::playPendingPreMatch` resolves the pending pre-match interaction and delegates match application to `PlayMatchCommandHandler`.
- AI/non-managed matches are applied through the same `PlayMatchCommandHandler` path during the safe daily update flow.
- `PlayMatchCommandHandler` validates the command against the fixture and builds one `MatchReport`.
- `League::applyMatchReport` is the authoritative completion point: it marks the fixture played, clears `eventEnqueued`, stores goals, updates standings, appends current-season history, updates team/player stats, and stores the current-season report.
- `MatchPlayedEvent` is published after the report is applied. The current standings/recent form shown in Dashboard come from `League` state rebuilt from played fixtures/reports on restore.
- Match completion requests a runtime save and `Game` flushes it at a safe checkpoint, so one matchday/update flow can coalesce important save requests instead of writing a full snapshot for every sub-step.
- Runtime restore rebuilds current standings/history by restoring reports first and falling back to persisted fixture results only when an older checkpoint has a played fixture without a detailed report.
- `event_enqueued` is non-authoritative persisted state until active interaction persistence exists. Save/load resets/ignores it so reloading before playing a pre-match cannot permanently skip the fixture.

## Future Match Engine Design Status

- The coordinate simulation design and initial skeleton boundary are documented and compile-safe. They do not change runtime match results, standings, save/load, or UI behavior.
- The current implemented MatchEngine skeleton includes core type DTOs/enums, pitch helpers, snapshot-based input, output-only result DTOs, a non-integrated `MatchEngine::simulate` entry point, a read-only `MatchEngineInputBuilder` snapshot builder, in-memory simulation state containers, a non-integrated `TeamShapeModel` tactical target helper, non-mutating `BallTrajectoryBuilder` / `InterceptionResolver` helper layers, and non-integrated action-planning helpers.
- The future V1 engine should use real pitch dimensions, tactical shape, action plan/reassessment, perception, ball trajectories, path interception, local contests, watched traces, and background summaries.
- V1 tactical inputs needed by the future engine are Mentality, Tempo, Width, DefensiveLine, PressingIntensity, MarkingStyle, and PassingDirectness.
- Runtime match behavior is unchanged. The next planned implementation phase should stay incremental: `ContestResolver` skeleton, then MovementResolver / PlayerIntentResolver, then the first minimal coordinate simulation prototype.

## Known Not-Yet-Supported Core Scenarios

- Active interaction exact restore.
- Completed season archive/history.
- Completed transfer history.
- Deep finance ledgers, debt/liabilities, ticket income, sponsorships, prize money, shirt sales, stadium maintenance, general operations, taxes, financial fair play, and finance UI.
- Contract renewal, negotiation, and counter offers.
- Tactical setup affecting match simulation.
- Real coordinate-based match engine implementation. Current `MatchSimulation` remains the existing lightweight behavior.
- Mini-pitch UI and live match event stream.
- Player-specific position familiarity overlay. `PlayerRoleFit` remains the primary-position based role-fit system for now.
- Coach-driven tactical identity.
- Broader multi-league expansion beyond the current seed.
- Save migration framework for old schema versions.
- Dirty/incremental saves.
- Automated save/load regression tests.

## Near-Term Core Roadmap

1. Finance Foundation (implemented)
2. Player Attribute Model and Match Engine design foundation (implemented)
3. Match Engine Core Types foundation (implemented)
4. TacticalSetup V1 expansion (implemented)
5. MatchEngine interface/skeleton for coordinate simulation output contracts (implemented)
6. MatchEngineInputBuilder / snapshot builder (implemented)
7. PlayerSimState + BallState + MatchSimulationState internal simulation state skeleton (implemented)
8. TeamShapeModel skeleton (implemented)
9. BallTrajectoryBuilder + InterceptionResolver skeleton (implemented)
10. ActionPlan / ActionCandidate / PlayerIntent skeleton (implemented)
11. ContestResolver skeleton
12. MovementResolver + PlayerIntentResolver skeleton
13. Minimal Coordinate Simulation Prototype
14. Automated regression tests when stable enough
15. Multi-league expansion preparation

## Deferred / Later Core Work

- Active Interaction Persistence.
- Rolling autosave slots.
- Save As / multiple named saves.
- Deep finance ledgers, debt/liabilities, revenue and expense streams, finance UI, transfer AI, player valuation, and richer contract systems.
- Completed season archives.
- Broader data editor/scouting systems.
- Full coordinate-based match engine implementation, including tactical shape, action plans, ball trajectories, path interception, local contests, watched traces, and background summaries.
