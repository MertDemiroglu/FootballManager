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
9. Click Save Game from the Dashboard sidebar.
10. Close the app.
11. Reopen.
12. Open Load Game and confirm the save is listed.
13. Load/continue the save.
14. Confirm the exact same in-game date and team state are restored.
15. Change autosave frequency to Weekly.
16. Advance fewer than 7 game days.
17. Confirm scheduled autosave does not spam disk writes or create new save slots.
18. Advance at least 7 game days.
19. Confirm autosave updates the current save slot.
20. Set autosave to Manual only.
21. Advance several days.
22. Confirm scheduled autosave does not run or create new save slots.
23. Click Save Game again and confirm manual save still works.
24. Close the app.
25. Reopen.
26. Load/continue the save.
27. Confirm the exact same advanced in-game date is restored.
28. Confirm no active interaction exact restore was added.
29. Advance to the first match.
30. Confirm the first fixtures still appear in August 2025 from SQL-loaded league rules.
31. Enter pre-match.
32. Edit lineup if applicable.
33. Open Lineup Editor.
34. Pick 4-4-2 and Auto Select.
35. Change mentality to Defensive.
36. Change tempo to High.
37. Confirm Starting XI is assigned 11/11.
38. Confirm the Substitutes tab shows up to 10 real players.
39. Close the app.
40. Reopen and load/continue the save.
41. Open Lineup Editor.
42. Confirm formation, Starting XI, substitutes, mentality, and tempo were restored.
43. Change to 4-3-3 and Auto Select.
44. Confirm the pitch still shows DM behind two CMs.
45. Confirm substitutes regenerated without starter/substitute overlap.
46. Close the app.
47. Reopen and load/continue the save.
48. Confirm 4-3-3, starters, substitutes, mentality, and tempo were restored.
49. Go to match day and enter pre-match.
50. Confirm pre-match shows the selected lineup rather than a freshly generated lineup.
51. Play the match.
52. Continue to dashboard.
53. Close the app.
54. Reopen and load/continue the save.
55. Confirm the fixture remains played.
56. Confirm the result remains visible.
57. Confirm standings, recent form, and upcoming matches remain consistent.
58. Confirm player fitness/form/morale survive load.
59. Ensure Transfer Room opens during a transfer window.
60. Create or generate a pending transfer offer if the current debug flow supports it.
61. Confirm the pending offer appears in Transfer Room/UI.
62. Close the app.
63. Reopen and load/continue the save.
64. Confirm the same pending offer still appears.
65. Reject the offer.
66. Close and load/continue again.
67. Confirm the rejected offer does not reappear as pending.
68. Create another offer after reload.
69. Confirm the new offer id does not collide with previous ids.
70. Advance days until an offer expires.
71. Close and load/continue again.
72. Confirm the expired offer does not reappear as pending.
73. Put a likely sold player in the managed team's Starting XI.
74. Create or load a pending transfer offer for that player and accept it.
75. Confirm in the current session that the player is removed from the seller team.
76. Confirm the managed-team starting slot remains empty and is not silently auto-filled.
77. Confirm the transferred player is removed from managed starters/substitutes.
78. Press Auto Select and confirm the empty managed-team slot can be filled by user action.
79. Repeat with a substitute if possible and confirm the player is removed from the bench without silently filling the managed bench.
80. Confirm the player appears in the buyer team.
81. Confirm the AI/non-managed buyer TeamSheet remains valid/current.
82. Confirm buyer budget is reduced and seller budget is increased according to current backend budget behavior.
83. Confirm the accepted offer no longer appears as pending.
84. Close the app.
85. Reopen.
86. Open Load Game and confirm the accepted-transfer save is listed.
87. Load/continue the save.
88. Confirm the player is still on the buyer team.
89. Confirm the player is not on the seller team.
90. Confirm the player's `teamId` matches the buyer team.
91. Confirm buyer/seller budgets remain changed.
92. Confirm the accepted offer does not reappear as pending.
93. Open Lineup Editor for the seller if possible and confirm the transferred-away player is not in starters/substitutes.
94. Confirm any managed-team slot vacated by the sale remains empty after load until Auto Select or manual assignment.
95. Open Lineup Editor for the buyer if possible and confirm the transferred-in player can be selected.
96. Confirm old incompatible saves may be skipped with clear messages such as missing runtime team finance state or missing SQL league rules rows.
97. Create/reject/decide-later another offer and verify transfer offer persistence still works.
98. Confirm date/fixture/result/team-sheet/player-condition persistence still works.
99. Confirm no transfer offer, roster ownership, budget state, or autosave setting is stored in `save_metadata`.
100. Confirm `runtime_transfer_offers`, `runtime_team_finances`, `runtime_player_roster_state`, and `runtime_save_settings` contain their expected data.
101. Confirm RuntimeSaveValidator accepts the valid save.
102. Confirm the Load Game card displays manager-team and the in-game date.
103. Confirm no real-world date appears as Game Date unless the simulation actually reaches that date.
104. Confirm `created_at_utc` and `updated_at_utc` are shown only as real-world save-card timestamps.
105. Confirm no Transfer Room UI redesign occurred.
106. Confirm product-roadmap-backend is updated.
107. Confirm git status remains clean after the manual app run, except for intentionally ignored runtime files.

## Known Not-Yet-Supported Scenarios

- Closing while an active pre-match interaction is open may not restore the exact pre-match screen yet.
- Closing while a post-match report screen is open may not restore the exact post-match screen yet.
- Closing while a transfer decision is open may not restore the exact transfer decision screen yet.
- Transfer offer state and accepted offer roster/budget effects are persisted, but active transfer decision interaction persistence is future work.
- Save As, rolling autosave slots, and incremental/dirty table-level saves are not implemented yet.
- Managed-team incomplete lineups are preserved after roster mutations; a future "lineup requires attention" interaction should block kickoff more explicitly.
- Deeper finance ledgers, contract renewal UI, negotiation/counter offers, and completed transfer history UI are future work.
- Tactical setup is persisted but does not affect match simulation yet.
- Coach-driven tactical identity is future work.
- Completed season archives/history are future work.
- LeagueRules are SQL-backed for the current Super Lig seed, but additional leagues still need their own rule rows before broad multi-league expansion.
