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
4. Confirm seller/buyer budgets change according to current backend budget behavior.
5. Confirm the accepted offer no longer appears as pending.
6. Confirm the transferred player is removed from managed starters/substitutes.
7. Confirm any managed-team slot vacated by the sale remains empty and is not silently auto-filled.
8. Press Auto Select and confirm the empty managed-team slot can be filled by user action.
9. Confirm the AI/non-managed buyer TeamSheet remains valid/current.
10. Close and reopen the app.
11. Load/continue the save.
12. Confirm player ownership, `teamId`, budgets, accepted offer status, and affected TeamSheets remain consistent.

## Save Metadata / Validation Checks

1. Confirm old incompatible saves may be skipped with clear messages such as missing runtime team finance state or missing SQL league rules rows.
2. Confirm no transfer offer, roster ownership, budget state, or autosave setting is stored in `save_metadata`.
3. Confirm `runtime_transfer_offers`, `runtime_team_finances`, `runtime_player_roster_state`, and `runtime_save_settings` contain their expected data.
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
- Free-agent pool persistence, completed season archives, completed transfer history, deeper finance ledgers, and automated save/load regression tests are future work.
