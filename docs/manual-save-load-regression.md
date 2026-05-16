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
14. Reopen.
15. Open Load Game and confirm the new save is listed.
16. Load/continue the save.
17. Confirm the exact same advanced in-game date is restored.
18. Advance to the first match.
19. Confirm the first fixtures still appear in August 2025 from SQL-loaded league rules.
20. Enter pre-match.
21. Edit lineup if applicable.
22. Open Lineup Editor.
23. Pick 4-4-2 and Auto Select.
24. Change mentality to Defensive.
25. Change tempo to High.
26. Confirm Starting XI is assigned 11/11.
27. Confirm the Substitutes tab shows up to 10 real players.
28. Close the app.
29. Reopen and load/continue the save.
30. Open Lineup Editor.
31. Confirm formation, Starting XI, substitutes, mentality, and tempo were restored.
32. Change to 4-3-3 and Auto Select.
33. Confirm the pitch still shows DM behind two CMs.
34. Confirm substitutes regenerated without starter/substitute overlap.
35. Close the app.
36. Reopen and load/continue the save.
37. Confirm 4-3-3, starters, substitutes, mentality, and tempo were restored.
38. Go to match day and enter pre-match.
39. Confirm pre-match shows the selected lineup rather than a freshly generated lineup.
40. Play the match.
41. Continue to dashboard.
42. Close the app.
43. Reopen and load/continue the save.
44. Confirm the fixture remains played.
45. Confirm the result remains visible.
46. Confirm standings, recent form, and upcoming matches remain consistent.
47. Confirm player fitness/form/morale survive load.
48. Ensure Transfer Room opens during a transfer window.
49. Create or generate a pending transfer offer if the current debug flow supports it.
50. Confirm the pending offer appears in Transfer Room/UI.
51. Close the app.
52. Reopen and load/continue the save.
53. Confirm the same pending offer still appears.
54. Reject the offer.
55. Close and load/continue again.
56. Confirm the rejected offer does not reappear as pending.
57. Create another offer after reload.
58. Confirm the new offer id does not collide with previous ids.
59. Advance days until an offer expires.
60. Close and load/continue again.
61. Confirm the expired offer does not reappear as pending.
62. Create or load a pending transfer offer and accept it.
63. Confirm in the current session that the player is removed from the seller team.
64. Confirm the transferred player is removed from the seller lineup/starters/substitutes.
65. Confirm the player appears in the buyer team.
66. Confirm buyer budget is reduced and seller budget is increased according to current backend budget behavior.
67. Confirm the accepted offer no longer appears as pending.
68. Close the app.
69. Reopen.
70. Open Load Game and confirm the accepted-transfer save is listed.
71. Load/continue the save.
72. Confirm the player is still on the buyer team.
73. Confirm the player is not on the seller team.
74. Confirm the player's `teamId` matches the buyer team.
75. Confirm buyer/seller budgets remain changed.
76. Confirm the accepted offer does not reappear as pending.
77. Open Lineup Editor for the seller if possible and confirm the transferred-away player is not in starters/substitutes.
78. Open Lineup Editor for the buyer if possible and confirm the transferred-in player can be selected.
79. Confirm old incompatible saves may be skipped with clear messages such as missing runtime team finance state or missing SQL league rules rows.
80. Create/reject/decide-later another offer and verify transfer offer persistence still works.
81. Confirm date/fixture/result/team-sheet/player-condition persistence still works.
82. Confirm no transfer offer, roster ownership, or budget state is stored in `save_metadata`.
83. Confirm `runtime_transfer_offers`, `runtime_team_finances`, and `runtime_player_roster_state` contain full-world league/team ids, not managed-team-only rows.
84. Confirm RuntimeSaveValidator accepts the valid save.
85. Confirm the Load Game card displays manager-team and the in-game date.
86. Confirm no real-world date appears as Game Date unless the simulation actually reaches that date.
87. Confirm `created_at_utc` and `updated_at_utc` are shown only as real-world save-card timestamps.
88. Confirm no Transfer Room UI redesign occurred.
89. Confirm git status remains clean after the manual app run, except for intentionally ignored runtime files.

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
