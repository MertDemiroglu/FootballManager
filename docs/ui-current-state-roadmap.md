# UI Current State Roadmap

This living document describes the current UI state and planned UI work. Keep gameplay ownership in core/backend; QML should present and call backend APIs.

## Current UI Screens

- Main Menu.
- New Game / team selection.
- Load Game / Continue.
- Dashboard.
- Lineup Editor.
- Pre-Match.
- Post-Match.
- Transfer offer decision/current transfer UI.
- Save Game button.
- Minimal Settings autosave popup.

## Implemented UI Features

- Dashboard shell/sidebar.
- Full-screen app startup.
- Selected club identity/colors.
- Next match card.
- Season stats card.
- Schedule overview.
- Lineup Editor with shared pitch/token system.
- Pre/Post Match shared pitch/token system.
- Manual Save Game action.
- Autosave frequency selection.

## Known Not-Yet-Supported UI Scenarios

- Polished full Settings screen.
- Save/Load polish and invalid-save messaging.
- Team screen rework.
- Transfer room rework.
- Finance/budget UI.
- Player detail polish.
- Match report detailed screen polish.
- Completed transfer history UI.
- Completed season archive UI.
- Lineup requires attention interaction.
- Active interaction exact restore UI, deferred.
- Multi-league UI.

## Near-Term UI Roadmap

1. Save/Load UX polish after backend settles
2. Team screen rework
3. Transfer room rework
4. Finance/budget display
5. Match report polish

## UI Architecture Rules

- QML is presentation only.
- Gameplay state comes from `GameFacade`/core.
- Avoid fake duplicated QML state for gameplay data.
- Use shared pitch/token components.
- Keep Dashboard sidebar only on Dashboard.
- Pre/Post Match are full-screen screens, not modals.
