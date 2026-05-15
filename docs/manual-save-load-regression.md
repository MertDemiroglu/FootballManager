# Manual Save/Load Regression Checklist

Do not run generated smoke executables when the local antivirus warning is still relevant. Use the app manually and inspect the save DB only when needed.

## Core Flow

1. Close the app.
2. Clear or rename the AppData saves folder.
3. Open the app.
4. Verify Continue is disabled.
5. Open Load Game and verify it is empty.
6. Confirm opening Main Menu does not create `game.db`.
7. Create New Game.
8. Confirm the dashboard starts on July 1, 2025.
9. Close the app.
10. Reopen and load/continue the save.
11. Confirm the exact same in-game date is restored.
12. Advance several days.
13. Close the app.
14. Reopen and load/continue the save.
15. Confirm the exact same advanced in-game date is restored.
16. Advance to the first match.
17. Confirm the first fixtures still appear in August 2025 from SQL-loaded league rules.
18. Enter pre-match.
19. Edit lineup if applicable.
20. Open Lineup Editor.
21. Pick 4-4-2 and Auto Select.
22. Change mentality to Defensive.
23. Change tempo to High.
24. Confirm Starting XI is assigned 11/11.
25. Confirm the Substitutes tab shows up to 10 real players.
26. Close the app.
27. Reopen and load/continue the save.
28. Open Lineup Editor.
29. Confirm formation, Starting XI, substitutes, mentality, and tempo were restored.
30. Change to 4-3-3 and Auto Select.
31. Confirm the pitch still shows DM behind two CMs.
32. Confirm substitutes regenerated without starter/substitute overlap.
33. Close the app.
34. Reopen and load/continue the save.
35. Confirm 4-3-3, starters, substitutes, mentality, and tempo were restored.
36. Go to match day and enter pre-match.
37. Confirm pre-match shows the selected lineup rather than a freshly generated lineup.
38. Play the match.
39. Continue to dashboard.
40. Close the app.
41. Reopen and load/continue the save.
42. Confirm the fixture remains played.
43. Confirm the result remains visible.
44. Confirm standings, recent form, and upcoming matches remain consistent.
45. Confirm player fitness/form/morale survive load.
46. Confirm RuntimeSaveValidator accepts the valid save.
47. Confirm the Load Game card displays manager-team and the in-game date.
48. Confirm no real-world date appears as Game Date unless the simulation actually reaches that date.
49. Confirm `created_at_utc` and `updated_at_utc` are shown only as real-world save-card timestamps.
50. Confirm git status remains clean after the manual app run, except for intentionally ignored runtime files.

## Known Not-Yet-Supported Scenarios

- Closing while an active pre-match interaction is open may not restore the exact pre-match screen yet.
- Closing while a post-match report screen is open may not restore the exact post-match screen yet.
- Closing while a transfer decision is open may not restore the exact transfer decision screen yet.
- Transfer offer persistence is future work.
- Accepted/rejected transfer decisions and roster mutations need dedicated persistence.
- Tactical setup is persisted but does not affect match simulation yet.
- Coach-driven tactical identity is future work.
- Completed season archives/history are future work.
- LeagueRules are SQL-backed for the current Super Lig seed, but additional leagues still need their own rule rows before broad multi-league expansion.
