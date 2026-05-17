# Manual Save/Load Regression Checklist

Use this as a practical manual test checklist for current checkpoint-based save/load behavior. Do not run generated smoke executables while the local antivirus warning remains relevant; use the app manually and inspect the save DB only when needed.

## Fresh New Game / Load Game

1. Close the app.
2. Clear or rename the AppData saves folder.
3. Open the app.
4. Verify Continue is disabled.
5. Open Load Game and verify it is empty.
6. Confirm opening Main Menu does not create `game.db`.
7. Create New Game.
8. Confirm Dashboard opens on July 1, 2025 with the selected club/team state.
9. Close the app, reopen it, and confirm Load Game shows the new save.
10. Load/continue the save and confirm the game resumes through Dashboard/current safe checkpoint.

## Manual Save

1. From Dashboard, click Save Game.
2. Confirm the UI gives small success feedback.
3. Close and reopen the app.
4. Load/continue the save.
5. Confirm the in-game date, managed club, selected team state, and dashboard data are preserved.
6. Confirm no new save slot was created; Manual Save overwrites the current save slot.

## Autosave Policy

1. Open Dashboard Settings.
2. Confirm the autosave options are visible: Manual only, Daily, Every 3 days, Weekly, Every 2 weeks.
3. Confirm the default visible selection is Weekly on a new save.
4. Set autosave to Weekly.
5. Advance fewer than 7 game days and confirm scheduled autosave does not create new save slots or spam disk writes.
6. Advance at least 7 game days and confirm autosave updates the current save slot.
7. Set autosave to Manual only.
8. Advance several days and confirm scheduled autosave does not run.
9. Click Save Game and confirm manual save still works.
10. Inspect the DB if needed: `runtime_save_settings` owns autosave settings; `save_metadata` does not.

## Lineup / Tactics Persistence

1. Open Lineup Editor.
2. Pick 4-4-2 and Auto Select.
3. Change mentality to Defensive.
4. Change tempo to High.
5. Confirm Starting XI is assigned 11/11 and Substitutes shows up to 10 real players.
6. Close and reopen the app.
7. Load/continue the save.
8. Open Lineup Editor and confirm formation, Starting XI, substitutes, mentality, and tempo were restored.
9. Change to 4-3-3 and Auto Select.
10. Confirm the pitch still shows DM behind two CMs and substitutes have no starter/substitute overlap.
11. Close, reopen, and confirm the 4-3-3 lineup/tactics are restored.

## Match Lifecycle / Result Persistence

1. Create New Game.
2. Advance to the first match day.
3. Confirm pre-match appears.
4. If checkpoint behavior is relevant, close/reopen before playing.
5. Load/continue and confirm the same match can still be reached/played.
6. Enter pre-match and confirm it uses the selected lineup rather than a freshly generated lineup.
7. Play the match.
8. Confirm the fixture is marked played.
9. Confirm the result is visible.
10. Confirm standings updated once.
11. Confirm recent form updated.
12. Confirm post-match/report data appears.
13. Close and reopen the app.
14. Load/continue the save.
15. Confirm fixture/result/report/standings/recent form remain consistent.
16. Advance additional days and confirm the same match is not replayed.
17. Confirm no skipped fixture was caused by stale `event_enqueued`.

## Transfer Offer Persistence

1. Ensure Transfer Room opens during a transfer window.
2. Create or generate a pending transfer offer if the current debug flow supports it.
3. Confirm the pending offer appears in Transfer Room/UI.
4. Close and reopen the app.
5. Load/continue the save and confirm the same pending offer still appears.
6. Reject the offer.
7. Close and load/continue again.
8. Confirm the rejected offer does not reappear as pending.
9. Create another offer after reload and confirm the new offer id does not collide with previous ids.
10. Advance days until an offer expires.
11. Close and load/continue again.
12. Confirm the expired offer does not reappear as pending.

## Accepted Transfer / Roster Mutation Persistence

1. Put a likely sold player in the managed team's Starting XI or substitutes.
2. Create or load a pending transfer offer for that player and accept it.
3. Confirm in the current session that the player is removed from the seller team and appears on the buyer team.
4. Confirm buyer cash balance decreases by the transfer fee.
5. Confirm buyer transfer budget decreases by the transfer fee.
6. Confirm seller cash balance increases by the transfer fee.
7. Confirm seller transfer budget increases by the seller health-plus-strategy retention percentage.
8. Confirm wage affordability uses current player contracts rather than only comparing against the wage limit.
9. Confirm the accepted offer no longer appears as pending.
10. Confirm the transferred player is removed from managed starters/substitutes.
11. Confirm any managed-team slot vacated by the sale remains empty and is not silently auto-filled.
12. Press Auto Select and confirm the empty managed-team slot can be filled by user action.
13. Confirm the AI/non-managed buyer TeamSheet remains valid/current.
14. Save, close, reopen, and load/continue.
15. Confirm player ownership, `teamId`, finance values, accepted offer status, and affected TeamSheets remain consistent.

## Finance Foundation

1. Create New Game.
2. Confirm teams get finance state with strategy and health.
3. Confirm Balanced/Stable allocation uses the health sporting envelope, not full cash directly.
4. Accept a paid transfer.
5. Confirm buyer cash balance decreases.
6. Confirm buyer transfer budget decreases.
7. Confirm seller cash balance increases by the full fee.
8. Confirm seller transfer budget increases by health-plus-strategy sale retention.
9. Save/load.
10. Confirm cash, transfer budget, wage budget, strategy, and health persist.
11. Confirm RuntimeSaveValidator accepts valid saves.
12. Confirm wage affordability uses current contracts.
13. Sign or transfer a player with wage if the current flow supports it.
14. Confirm no duplicate ownership or transfer regression.
15. Confirm old transfer offer persistence still works.

## Free Agent Persistence

1. Clear or rename the AppData saves folder.
2. Create New Game.
3. Force or simulate a player becoming a free agent if the current debug flow supports it.
4. Confirm the player is removed from his team.
5. Confirm the player no longer appears in the previous team's Lineup Editor roster.
6. If the player was selected by the managed team, confirm the affected lineup slot becomes empty.
7. Save, close, reopen, and load/continue.
8. Confirm the player remains a free agent and is not restored to the old team.
9. Confirm the selected `TeamSheet` no longer references the free agent.
10. If `transferFreeAgent` can be triggered, sign the free agent to a team and confirm he leaves `freeAgents` and appears in the target team.
11. Save/load again and confirm the signed player's ownership remains correct.
12. Confirm normal accepted transfer persistence still works.
13. Confirm Load Game does not skip the save.
14. Confirm `RuntimeSaveValidator` accepts the save.

## Save Metadata / Validation Checks

1. Confirm old incompatible saves may be skipped with clear messages such as missing runtime team finance strategy/health, missing runtime team finance state, or missing SQL league rules rows.
2. Confirm no transfer offer, roster ownership, budget state, or autosave setting is stored in `save_metadata`.
3. Confirm `runtime_transfer_offers`, `runtime_team_finances`, `runtime_player_roster_state`, `runtime_free_agents`, and `runtime_save_settings` contain their expected data.
4. Confirm RuntimeSaveValidator accepts a valid save.
5. Confirm the Load Game card displays manager-team and the in-game date.
6. Confirm no real-world date appears as Game Date unless the simulation actually reaches that date.
7. Confirm `created_at_utc` and `updated_at_utc` are shown only as real-world save-card timestamps.
8. Confirm git status remains clean after the manual app run, except for intentionally ignored runtime files.

## Known Unsupported Save/Load Scenarios

- Closing while an active pre-match interaction is open may not restore the exact pre-match screen yet.
- Closing while a post-match report screen is open may not restore the exact post-match screen yet.
- Closing while a transfer decision is open may not restore the exact transfer decision screen yet.
- Save As, multiple named manual saves, rolling autosave slots, and incremental/dirty table-level saves are not implemented yet.
- Completed season archives, completed transfer history, deep finance ledgers, debt/liabilities, ticket/sponsor/prize/shirt revenue, stadium/wage/debt/general operating expenses, finance UI, transfer AI, player valuation, and automated save/load regression tests are future work.
