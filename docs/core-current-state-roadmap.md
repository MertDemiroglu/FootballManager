# Core Current State Roadmap

This living document describes the current core/backend shape and the next backend priorities. Keep detailed planning here; keep `manual-save-load-regression.md` as a test checklist.

## Current Core Architecture

- `Game` is the orchestrator for simulation flow, save checkpoints, user selection, interaction routing, date advancement, and high-level domain service calls.
- `World` owns `LeagueContext`s and cross-league services such as transfer rooms, transfer offers, id allocation, and domain event publishing.
- `LeagueContext` owns league-scoped orchestration: rules, season plan, fixture/match command handling, and rollover guard state.
- `League` owns competition state: teams, fixtures, standings/projections, current season history, and match reports.
- `Team` owns roster membership, player index, budgets, colors, coach relation, and the selected/current `TeamSheet`.
- `HeadCoach` owns `TacticalPreferences`, which are coach/team defaults rather than a concrete match squad.
- `TeamSheet` owns formation, starting XI slot assignments, substitutes, and active `TacticalSetup`.
- QML/UI must not own gameplay source-of-truth. Gameplay state comes from core models exposed through `GameFacade`.

## Implemented Backend Systems

- Checkpoint-based save/load.
- Full-world/all-leagues runtime save model.
- `SaveMetadata` as display/cache state only.
- LeagueRules SQL-backed for the current Super Lig seed.
- Fixtures, results, and match reports persistence.
- Match lifecycle is owned by core: scheduler creates league-scoped commands, `PlayMatchCommandHandler` applies reports, and `League` updates fixture/result/standings/history together.
- Player condition persistence.
- `TeamSheet`, tactics, and substitutes persistence.
- Transfer offer persistence.
- Accepted transfer roster movement, budget, and contract snapshot persistence.
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

## Known Not-Yet-Supported Core Scenarios

- Active interaction exact restore.
- Free agent pool persistence.
- Completed season archive/history.
- Completed transfer history.
- Richer finance ledgers.
- Contract renewal, negotiation, and counter offers.
- Tactical setup affecting match simulation.
- Coach-driven tactical identity.
- Broader multi-league expansion beyond the current seed.
- Save migration framework for old schema versions.
- Dirty/incremental saves.
- Automated save/load regression tests.

## Near-Term Core Roadmap

1. Match Lifecycle / Standings / History Audit (active/in progress)
2. Free Agent Persistence
3. Automated regression tests when stable enough
4. Multi-league expansion preparation
5. Match engine/tactical effects later

## Deferred / Later Core Work

- Active Interaction Persistence.
- Rolling autosave slots.
- Save As / multiple named saves.
- Deep finance/contract systems.
- Completed season archives.
- Broader data editor/scouting systems.
