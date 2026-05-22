# Match Engine Design

This document captures the future match engine direction. It is design-only for this phase and does not implement the real engine.

## Match Engine Vision

The long-term goal is a realistic but lightweight football simulation. The engine should be understandable, deterministic for debugging, and rich enough for tactical and player-quality differences to matter.

A future mini-pitch UI can visualize player markers, ball flow, and event progression. This phase does not add 2D or 3D match graphics and does not add a live simulation stream.

## Future Engine Inputs

- Home and away `TeamSheet`.
- `PlayerAttributes`.
- `PlayerRoleFit`.
- `PlayerConditionState`: form, fitness, and morale.
- `TacticalSetup`: mentality, tempo, and future width, pressing, line height, and risk controls.
- `HeadCoach` and `TacticalPreferences`.
- Home advantage.
- Match context: date, competition, and importance.
- Deterministic random seed.

## Future Engine Outputs

- `MatchReport`.
- Score.
- Team stats.
- Player reports and ratings.
- Match events.
- Future possession and ball-flow snapshots for mini-pitch visualization.
- Condition and fatigue impacts.

## Simulation Principles

- Better players should have an advantage, not guaranteed success.
- Decisions are probabilistic and weighted by context.
- High Decisions, Vision, and Teamwork should improve action selection.
- Technical attributes should improve action execution.
- Physical attributes should affect movement, duels, and fatigue.
- Mental attributes should affect pressure, consistency, risk, discipline, and leadership.
- Randomness should exist, but it should be weighted and deterministic from the match seed.

## Example Action Model

1. A player receives the ball.
2. The player evaluates options such as pass, dribble, shoot, cross, hold, or clear.
3. The engine chooses an action from attributes, tactics, context, and deterministic randomness.
4. The engine resolves action success from relevant attributes, opponent pressure, role fit, and condition.

## Deterministic Random

The same match id, date, teams, team sheets, and seed should reproduce the same result. This is required for debugging, save/load stability, and future regression testing.

## Deferred Work

- Full match engine implementation.
- Mini-pitch UI.
- Live event stream.
- Injuries.
- Substitutions.
- Advanced tactical instructions.
- Player development and progression.
- Set-piece specialist selection.
