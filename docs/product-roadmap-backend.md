# Backend Product Roadmap

This roadmap keeps future save/load work scoped. Each phase should preserve the full-world/all-leagues save model and avoid pushing runtime validation or persistence ownership into QML/UI code.

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

## 2. Transfer Offer Persistence

- Purpose: persist pending offers, expiry dates, buyer/seller clubs, player ids, fees, and statuses.
- Depends on: stable runtime save DB and clear active interaction boundaries.
- Likely affected areas: `TransferOfferService`, `TransferRoom`, `Game`, new transfer runtime tables.
- Why here: transfer offers are mutable gameplay state that can exist across many days.
- Do not mix in: roster mutation application beyond what is required to represent offer state.

## 3. Roster Mutation Persistence

- Purpose: persist player-team membership, contract updates, finance effects, and roster changes after transfers or season events.
- Depends on: transfer decisions/status persistence.
- Likely affected areas: `Team`, `Footballer`, contract model, transfer services, runtime roster/contract/finance tables.
- Why after offers: roster changes are consequences of transfer decisions and should not be persisted without a durable decision model.
- Do not mix in: save UI polish or league rules migration.

## 4. Active Interaction Persistence

- Purpose: restore exact blocking states after closing during pre-match, post-match, transfer decision, or other modal flows.
- Depends on: lineup persistence for pre-match and transfer persistence for transfer decisions.
- Likely affected areas: `InteractionManager`, interaction DTOs, `Game`, `GameFacade`, active interaction payload tables.
- Why after core mutable state: interactions reference domain objects that must already be persisted.
- Do not mix in: broad QML redesign. Restore behavior first, polish later.

## 5. LeagueRules SQL/Data Migration

- Purpose: make season windows, kickoff dates, matchday counts, transfer windows, and fixture rules data-driven per league.
- Depends on: runtime validation and current Super Lig assumptions being well documented.
- Likely affected areas: `LeagueRules`, `SeasonPlan`, bootstrap repositories, schema/seed data, `RuntimeSaveValidator`.
- Why before multi-league expansion: validation and scheduling must stop assuming a single hardcoded league calendar.
- Do not mix in: transfer/interaction implementation or save UI redesign.

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
