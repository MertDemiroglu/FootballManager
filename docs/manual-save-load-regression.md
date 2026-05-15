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
46. Ensure Transfer Room opens during a transfer window.
47. Create or generate a pending transfer offer if the current debug flow supports it.
48. Confirm the pending offer appears in Transfer Room/UI.
49. Close the app.
50. Reopen and load/continue the save.
51. Confirm the same pending offer still appears.
52. Reject the offer.
53. Close and load/continue again.
54. Confirm the rejected offer does not reappear as pending.
55. Create another offer after reload.
56. Confirm the new offer id does not collide with previous ids.
57. Advance days until an offer expires.
58. Close and load/continue again.
59. Confirm the expired offer does not reappear as pending.
60. Create or load a pending transfer offer and accept it.
61. Confirm in the current session that the player is removed from the seller team.
62. Confirm the player appears in the buyer team.
63. Confirm buyer budget is reduced and seller budget is increased according to current backend budget behavior.
64. Confirm the accepted offer no longer appears as pending.
65. Close the app.
66. Reopen and load/continue the save.
67. Confirm the player is still on the buyer team.
68. Confirm the player is not on the seller team.
69. Confirm the player's `teamId` matches the buyer team.
70. Confirm buyer/seller budgets remain changed.
71. Confirm the accepted offer does not reappear as pending.
72. Open Lineup Editor for the seller if possible and confirm the transferred-away player is not in starters/substitutes.
73. Open Lineup Editor for the buyer if possible and confirm the transferred-in player can be selected.
74. Create/reject/decide-later another offer and verify transfer offer persistence still works.
75. Confirm date/fixture/result/team-sheet/player-condition persistence still works.
76. Confirm no transfer offer, roster ownership, or budget state is stored in `save_metadata`.
77. Confirm `runtime_transfer_offers`, `runtime_team_finances`, and `runtime_player_roster_state` contain full-world league/team ids, not managed-team-only rows.
78. Confirm RuntimeSaveValidator accepts the valid save.
79. Confirm the Load Game card displays manager-team and the in-game date.
80. Confirm no real-world date appears as Game Date unless the simulation actually reaches that date.
81. Confirm `created_at_utc` and `updated_at_utc` are shown only as real-world save-card timestamps.
82. Confirm no Transfer Room UI redesign occurred.
83. Confirm git status remains clean after the manual app run, except for intentionally ignored runtime files.

## Known Not-Yet-Supported Scenarios

- Closing while an active pre-match interaction is open may not restore the exact pre-match screen yet.
- Closing while a post-match report screen is open may not restore the exact post-match screen yet.
- Closing while a transfer decision is open may not restore the exact transfer decision screen yet.
- Transfer offer state and accepted offer roster/budget effects are persisted, but active transfer decision interaction persistence is future work.
- Deeper finance ledgers, contract renewal UI, negotiation/counter offers, and completed transfer history UI are future work.
- Tactical setup is persisted but does not affect match simulation yet.
- Coach-driven tactical identity is future work.
- Completed season archives/history are future work.
- LeagueRules are SQL-backed for the current Super Lig seed, but additional leagues still need their own rule rows before broad multi-league expansion.
