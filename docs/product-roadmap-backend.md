# Backend Product Roadmap

This roadmap keeps future save/load work scoped. Each phase should preserve the full-world/all-leagues save model and avoid pushing runtime validation or persistence ownership into QML/UI code.

## 0. State Ownership / Legacy Audit

- Purpose: keep ownership boundaries explicit after save/load and selected match squad persistence.
- Status: implemented.
- Verified shape: `Game` is an orchestrator; `Team` owns selected/current `TeamSheet`; `HeadCoach` owns `TacticalPreferences`; `SaveMetadata` remains save-card display/cache state only; QML remains presentation-only.
- Do not mix in: feature-sized persistence, schema migrations, match engine behavior, or UI redesign.

## 1. Selected Lineup and Tactic Persistence

- Purpose: persist selected match squad state across reloads: formation, starting XI slot assignments, substitutes, and tactical setup.
- Depends on: current save slot and runtime game-state foundation.
- Status: implemented in runtime save state.
- Current domain shape: `TeamSheet` represents starting XI + substitutes + `TacticalSetup`; tactical setup currently supports mentality and tempo only.
- Ownership shape: `Team` owns the current/default selected `TeamSheet`; `HeadCoach` owns `TacticalPreferences`; `Game` orchestrates create/update/ensure/resolve and does not own a global selected-team-sheet registry.
- Persisted shape: `runtime_team_sheets`, `runtime_team_sheet_starters`, and `runtime_team_sheet_substitutes`, keyed by `league_id`/`team_id`.
- Why first: active pre-match and match play depend on stable lineup state.
- Match engine note: mentality and tempo do not affect simulation yet; the future match engine rewrite should consume `TacticalSetup`.
- Future work: Coach tactical profile should drive formation, mentality, tempo, and selection preferences.
- Active interaction note: pending pre-match sheets are match-specific frozen snapshots and should move into `PreMatchInteraction` / active interaction persistence later.
- Do not mix in: transfer persistence, UI redesign, or completed season archives.

## 2. LeagueRules / SeasonConfig SQL Migration

- Purpose: make season windows, kickoff dates, matchday counts, transfer windows, and fixture rules data-driven per league.
- Depends on: runtime validation and state ownership boundaries.
- Status: implemented for the current Super Lig seed.
- Implemented shape: `league_rules`, `league_transfer_windows`, and `league_matchday_distribution_offsets` hold league-specific rule data. `SqliteLeagueRulesRepository` loads that data into core `LeagueRules` for bootstrap, `SeasonPlan`, fixture generation, and runtime validation.
- Current seed scope: Super Lig only. Future multi-league work should add more league rows/rules rather than hardcoding new constants in core.
- Remaining cleanup: `LeagueRules` still exposes function fields as a transitional API; it should eventually become fully value-config based.
- Do not mix in: transfer/interaction implementation or save UI redesign.

## 3. Transfer Offer Persistence

- Purpose: persist pending offers, expiry dates, buyer/seller clubs, player ids, fees, and statuses.
- Depends on: stable runtime save DB and clear active interaction boundaries.
- Status: implemented for offer state.
- Implemented shape: `runtime_transfer_offers` stores full-world offer rows with stable expiry/status/resolution codes. `TransferOfferService` exports/restores offers and recovers `nextOfferId` from max persisted offer id + 1.
- Affected areas: `TransferOfferService`, `SqliteGameStateRepository`, `RuntimeSaveValidator`, and `Game` persistence flush paths.
- Why here: transfer offers are mutable gameplay state that can exist across many days.
- Not covered: roster mutation persistence, budget/finance persistence, active transfer decision interaction persistence, negotiation/counter offers, and completed transfer history UI.

## 4. Roster Mutation Persistence

- Purpose: persist player-team membership, contract updates, finance effects, and roster changes after transfers or season events.
- Depends on: transfer offer status/resolution persistence.
- Status: implemented for team-owned roster moves, team budget snapshots, and player contract snapshots.
- Implemented shape: `runtime_team_finances` stores full-world team budgets; `runtime_player_roster_state` stores player ownership, contract wage/years, and current season year. Restore moves players through `Team::releasePlayer` / `Team::addPlayer`, updates `player.teamId`, restores contracts, and reconciles selected team sheets after roster restore.
- Affected areas: `Game`, `SqliteGameStateRepository`, `RuntimeSaveValidator`, `TransferRoom`, `Team`/`Footballer` contract access.
- Why next: accepted offers currently mutate rosters in memory, but long-term roster and budget effects need their own runtime source of truth rather than replaying accepted offers on load.
- Not covered: active interaction persistence, completed transfer history UI, deeper finance ledgers, contract renewal UI, negotiation/counter offers, free-agent UI, or match engine changes.

## 5. Active Interaction Persistence

- Purpose: restore exact blocking states after closing during pre-match, post-match, transfer decision, or other modal flows.
- Depends on: lineup persistence for pre-match and durable mutable transfer/roster state for transfer decisions.
- Status: next recommended phase unless a new backend blocker appears.
- Likely affected areas: `InteractionManager`, interaction DTOs, `Game`, `GameFacade`, active interaction payload tables.
- Why after core mutable state: interactions reference domain objects that must already be persisted.
- Do not mix in: broad QML redesign. Restore behavior first, polish later.

## 6. Completed Season Archive/History

- Purpose: persist completed seasons, final standings, historical fixtures, champions, and long-term records.
- Depends on: current season state, LeagueRules, and rollover behavior being stable.
- Likely affected areas: `League`, standings services, match reports, archive tables, history presentation APIs.
- Why after LeagueRules: archives should carry correct league/year context across multiple leagues.
- Do not mix in: dashboard visual redesign.

## 7. Save/Load Automated Regression Tests

- Purpose: cover save creation, date restore, fixture/result restore, standings rebuild, player condition restore, and invalid save rejection.
- Depends on: stable persistence APIs and test-safe DB bootstrap paths.
- Likely affected areas: core test executables, app smoke tests, test fixture builders.
- Why here: persistence surfaces should be stable enough for durable tests.
- Do not mix in: new gameplay features.

## 8. Save UI Polish / Dark Theme

- Purpose: improve save list, load/continue affordances, empty states, and visual consistency.
- Depends on: backend save/load correctness and metadata semantics.
- Likely affected areas: Main Menu QML and presentation helpers.
- Why last: visual polish should not hide backend persistence gaps.
- Do not mix in: persistence schema work or gameplay changes.
