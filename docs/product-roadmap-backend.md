# Backend Product Roadmap

This roadmap keeps future save/load work scoped. Each phase should preserve the checkpoint-based, full-world/all-leagues save model and avoid pushing runtime validation or persistence ownership into QML/UI code.

## Current Save Direction

- Save/load is checkpoint-based. The game persists safe runtime snapshots, not exact transient popup or screen state.
- The app may start from Main Menu. Continue and Load Game enter normal gameplay through Dashboard/current safe checkpoint.
- `SaveMetadata` remains save-card display/cache state only. Autosave settings live in `runtime_save_settings`.
- Manual Save overwrites the current save slot. Scheduled autosave also overwrites the current save slot.
- Autosave is game-date interval based, defaults to Weekly, and does not create rolling slots.
- Important events may request a save, but full snapshot writes should be batched/coalesced at safe checkpoints where possible.
- The current save format is still a full runtime snapshot. Dirty/incremental table-level saves are a future optimization if multi-league snapshot cost becomes high.
- Active interaction exact restore is intentionally deferred. Do not add `runtime_active_interaction` tables in the current phase.
- Save As, multiple named manual saves, and rolling autosave slots are not planned for this phase.

## 0. State Ownership / Legacy Audit

- Purpose: keep ownership boundaries explicit after save/load and selected match squad persistence.
- Status: implemented.
- Verified shape: `Game` is an orchestrator; `Team` owns selected/current `TeamSheet`; `HeadCoach` owns `TacticalPreferences`; `SaveMetadata` remains save-card display/cache state only; QML remains presentation-only.
- Do not mix in: feature-sized persistence, schema migrations, match engine behavior, or UI redesign.

## 1. Selected Lineup and Tactic Persistence

- Status: implemented in runtime save state.
- Implemented shape: `runtime_team_sheets`, `runtime_team_sheet_starters`, and `runtime_team_sheet_substitutes`, keyed by `league_id`/`team_id`.
- Current domain shape: `TeamSheet` represents starting XI + substitutes + `TacticalSetup`; tactical setup currently supports mentality and tempo only.
- Active interaction note: pending pre-match sheets are match-specific frozen snapshots and should move into active interaction persistence later.

## 2. LeagueRules / SeasonConfig SQL Migration

- Status: implemented for the current Super Lig seed.
- Implemented shape: `league_rules`, `league_transfer_windows`, and `league_matchday_distribution_offsets`.
- Remaining cleanup: `LeagueRules` still exposes function fields as a transitional API; it should eventually become fully value-config based.
- Do not mix in: transfer/interaction implementation or save UI redesign.

## 3. Transfer Offer Persistence

- Status: implemented for offer state.
- Implemented shape: `runtime_transfer_offers` stores full-world offer rows with stable expiry/status/resolution codes.
- Not covered: active transfer decision interaction persistence, negotiation/counter offers, and completed transfer history UI.

## 4. Roster Mutation Persistence

- Status: implemented for team-owned roster moves, team budget snapshots, and player contract snapshots.
- Implemented shape: `runtime_team_finances` stores full-world team budgets; `runtime_player_roster_state` stores player ownership, contract wage/years, and current season year.
- Not covered: free-agent persistence, richer finance ledgers, contract renewal UI, negotiation/counter offers, and active interaction persistence.

## 5. Save Backend Runtime Persistence

- Status: largely complete for the current checkpoint model.
- Persisted today: game date, fixtures, match results/reports, player condition, selected `TeamSheet`s, tactics, substitutes, LeagueRules config, transfer offers, accepted transfer roster movement, budgets, and contract snapshots.
- Current behavior: Continue/Load resumes through Dashboard/safe checkpoint rather than restoring exact transient modals.
- Remaining risk: full runtime snapshots may become costly with broader multi-league worlds; dirty/incremental save work can be considered later.

## 6. Manual Save + Auto Save Policy

- Status: active phase.
- Scope: Dashboard Manual Save, autosave frequency settings, game-date scheduled autosave, `runtime_save_settings`, and save request batching/coalescing.
- Default autosave frequency: Weekly.
- Stable options: `manual_only`, `daily`, `every_3_days`, `weekly`, `every_2_weeks`.
- Do not mix in: active interaction persistence, Save As, rolling autosave slots, dirty/incremental saves, Dashboard redesign, match engine behavior, transfer logic, or LeagueRules migration.

## Next Phases

1. Match Lifecycle / Standings / History Audit
2. Free Agent Persistence
3. Team Screen / Transfer / Finance UI Rework
4. Active Interaction Persistence, deferred/low priority
