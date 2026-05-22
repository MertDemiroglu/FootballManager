# Player Attribute Model

This document defines the core player attribute model used by roster evaluation and the future match engine foundation.

## Attribute Scale

- Core attributes use a 0-100 scale.
- UI may later display the raw 0-100 value, bars, or a converted 0-20 view.
- Runtime condition, form, and morale are not permanent attributes.

## Technical Attributes

- Shooting: finishing quality and shot execution.
- Passing: short and medium pass accuracy and weight.
- First Touch: ability to receive and control the ball cleanly.
- Technique: general ball skill and execution quality.
- Tackling: ability to win the ball in ground challenges.
- Dribbling: ability to carry the ball past pressure.
- Crossing: wide delivery into dangerous areas.
- Marking: ability to track and deny opponents space.
- Heading: aerial contact quality for attacking and defending.
- Set Pieces: free kick, corner, and dead-ball delivery quality.

## Mental Attributes

- Decisions: quality of action choice under match context.
- Vision: ability to see progressive or creative options.
- Positioning: defensive and structural positioning.
- Off The Ball: attacking movement without possession.
- Composure: execution quality under pressure.
- Concentration: consistency and focus across the match.
- Work Rate: willingness to keep moving and contributing.
- Teamwork: alignment with team shape and collective choices.
- Leadership: influence on teammates and stability.
- Aggression: intensity in duels and defensive actions.

## Physical Attributes

- Pace: top speed over open ground.
- Acceleration: first steps and short burst speed.
- Stamina: ability to sustain effort over time.
- Strength: contact strength and physical resistance.
- Agility: body control and quick changes of direction.
- Jumping Reach: effective aerial reach.

## Goalkeeper Attributes

- Shot Stopping: ability to save shots.
- Handling: ability to catch or secure the ball.
- Aerial Ability: command of crosses and high balls.
- One On Ones: effectiveness in close-range goalkeeper duels.
- Distribution: throwing, kicking, and restart quality.

## Runtime Modifiers

`PlayerConditionState` remains separate from permanent attributes. Form, fitness, and morale can modify match performance later, but they do not rewrite the player's base attribute values.

## Future Match Engine Usage

In the future coordinate-based match engine, mental attributes feed perception and decision quality. Vision, Decisions, Composure, Teamwork, Concentration, and related pressure context should affect which options a player notices, how early they notice them, and how sharply they select between candidates.

Technical and physical attributes feed execution and movement. Passing, Crossing, Shooting, First Touch, Tackling, Dribbling, Acceleration, Pace, Agility, Strength, Stamina, and goalkeeper attributes should affect whether selected actions are executed well, how quickly players arrive, and how duels resolve.

`PlayerRoleFit` remains important for future simulation. Role fit should affect local contests and movement/decision effectiveness without replacing the permanent attribute model.

## Position Fit

The current position-fit model remains `PlayerRoleFit`:

- Each player has one primary `PlayerPosition`.
- `PlayerRoleFit` maps the player's primary position plus a formation slot role into a fit tier.
- `calculateEffectivePowerForSlot` applies the role-fit multiplier to base `totalPower()`.

Player-specific position familiarity is deferred. A future table such as `player_position_familiarity(player_id, position, familiarity_score)` may overlay the current system, but it is intentionally not part of this phase.

## Legacy Fallback

The current seed still carries legacy `s1..s5` skill buckets. Until seed data is fully upgraded, players without an explicit `player_attributes` row get deterministic, position-aware attributes generated from `s1..s5`, primary position, and age.
