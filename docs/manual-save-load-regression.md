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
17. Enter pre-match.
18. Edit lineup if applicable.
19. Open Lineup Editor.
20. Pick 4-4-2 and Auto Select.
21. Change mentality to Defensive.
22. Change tempo to High.
23. Confirm Starting XI is assigned 11/11.
24. Confirm the Substitutes tab shows up to 10 real players.
25. Close the app.
26. Reopen and load/continue the save.
27. Open Lineup Editor.
28. Confirm formation, Starting XI, substitutes, mentality, and tempo were restored.
29. Change to 4-3-3 and Auto Select.
30. Confirm the pitch still shows DM behind two CMs.
31. Confirm substitutes regenerated without starter/substitute overlap.
32. Close the app.
33. Reopen and load/continue the save.
34. Confirm 4-3-3, starters, substitutes, mentality, and tempo were restored.
35. Go to match day and enter pre-match.
36. Confirm pre-match shows the selected lineup rather than a freshly generated lineup.
37. Play the match.
38. Continue to dashboard.
39. Close the app.
40. Reopen and load/continue the save.
41. Confirm the fixture remains played.
42. Confirm the result remains visible.
43. Confirm standings, recent form, and upcoming matches remain consistent.
44. Confirm player fitness/form/morale survive load.
45. Confirm the Load Game card displays manager-team and the in-game date.
46. Confirm no real-world date appears as Game Date unless the simulation actually reaches that date.
47. Confirm `created_at_utc` and `updated_at_utc` are shown only as real-world save-card timestamps.
48. Confirm git status remains clean after the manual app run, except for intentionally ignored runtime files.

## Known Not-Yet-Supported Scenarios

- Closing while an active pre-match interaction is open may not restore the exact pre-match screen yet.
- Closing while a post-match report screen is open may not restore the exact post-match screen yet.
- Closing while a transfer decision is open may not restore the exact transfer decision screen yet.
- Transfer offer persistence is future work.
- Accepted/rejected transfer decisions and roster mutations need dedicated persistence.
- Tactical setup is persisted but does not affect match simulation yet.
- Coach-driven tactical identity is future work.
- Completed season archives/history are future work.
- LeagueRules are still core-code driven and should move to data/SQL before broad multi-league expansion.
