# Match Engine Coordinate Simulation Design

This document captures the final architecture direction for the future coordinate-based match engine. It is design-only. It does not implement the real engine, add simulation code, change match result behavior, add mini-pitch UI, or change save/load behavior.

## 1. Vision

The project goal is not a simple score formula. The future match engine should be a realistic but lightweight football simulation where players, the ball, tactics, and local contests create the match outcome.

The long-term user experience should allow a watched match to be viewed through a sped-up mini-pitch. Player markers and ball movement in that view should reflect actual simulation state, not fake UI animation layered on top of unrelated results.

The engine remains core/backend-owned. QML should render output produced by the backend, such as trace frames, match events, reports, and summary state. QML must not own authoritative match state or invent simulation outcomes.

## 2. Core Simulation Style

The future engine should not be minute-by-minute. It should run possession-by-possession, with simulation steps inside each possession.

Each possession contains repeated simulation steps. A step conceptually updates:

- player intents
- player movement
- ball state
- possible action decisions
- local contests
- possession or phase transitions

Watched matches may use smaller step intervals so marker movement and ball travel are smooth enough for a sped-up mini-pitch stream. Background matches may use larger step intervals and lower output detail to protect multi-league performance.

The same core principles should apply to both modes. The difference is output detail and step granularity, not a separate fake result generator.

## 3. Pitch Geometry

Core coordinates are meter-based, not only abstract 0-100 values.

Standard internal dimensions:

- Length: 105m
- Width: 68m

The UI can normalize these coordinates later to 0-100, screen pixels, or any mini-pitch layout. The core should preserve real football geometry so football actions can reason about distance, angle, and zones.

The core should know important pitch areas:

- penalty area
- six-yard box
- penalty spot
- goal mouth
- touchlines
- byline
- central lanes
- wide lanes

This matters for penalties, goalkeeper decisions, crosses, cutbacks, shot locations, defensive recovery, and whether a player can realistically reach or block a ball path.

## 4. Match State

Future match state should be explicit enough to drive both reports and watched traces. Conceptual structures:

```text
MatchSimulationState
  time
  score
  currentPhase
  ballState
  playerSimStates
  teamSimStates
  currentPossession

PlayerSimState
  playerId
  teamId
  currentPosition
  targetPosition
  currentIntent
  staminaFatigueState
  hasBall

BallState
  currentPosition
  carrierId
  currentTrajectory
  looseBallState
```

`MatchSimulationState` owns the live simulation clock, score, phase, ball, player states, team states, and current possession.

`PlayerSimState` owns the current coordinate position, tactical target, current intent, fatigue state, and whether the player controls the ball.

`BallState` owns current ball position, the carrier when controlled, trajectory data when moving, and loose-ball state when nobody controls it.

## 5. Tactical Shape Model

Tactical shape is the default positioning system. Players do not move randomly. Each player's target position starts from the team shape and is then modified by local intent.

Team shape depends on:

- formation
- in-possession shape
- out-of-possession shape
- transition shape
- mentality
- tempo
- width
- defensive line
- pressing intensity
- marking style
- passing directness

Required tactical inputs for the V1 design:

- Mentality
- Tempo
- Width
- DefensiveLine
- PressingIntensity
- MarkingStyle
- PassingDirectness

The current UI already has a "More Options" direction. Future tactical UI can expose these inputs without changing the principle that core owns match decisions and QML only edits user-facing setup.

## 6. Marking Style

`MarkingStyle` should behave like an enum with these values:

- `Zonal`
- `Mixed`
- `ManOriented`

`Zonal` prioritizes space and passing lanes. It gives better shape stability and compactness, but can lose individual runners when an attacker times movement well.

`Mixed` is the default balanced model. It combines space coverage and runner tracking. Example: one centre-back tracks the striker while another protects the passing or crossing lane.

`ManOriented` uses tighter opponent following. It improves runner tracking, but may open exploitable space and consume more stamina.

Marking style affects:

- defender intents
- whether a centre-back tracks the striker or protects a zone
- whether a defensive midfielder blocks a passing lane or follows a runner
- defensive compactness
- cross and cutback reactions

## 7. Player Intent Model

Every player gets an intent each step, but only relevant players enter detailed contests.

Intent examples:

- `HoldShape`
- `MoveToSupport`
- `MakeRunBehind`
- `AttackNearPost`
- `AttackFarPost`
- `AttackCutbackZone`
- `DropForPass`
- `PressBallCarrier`
- `ContainBallCarrier`
- `TrackRunner`
- `MarkOpponent`
- `CoverSpace`
- `ProtectGoalZone`
- `BlockPassingLane`
- `InterceptBallPath`
- `AttemptTackle`
- `RecoverToGoal`
- `ClearDanger`

Off-ball players still act. Nearby or relevant players make richer decisions, while distant players mostly follow tactical shape, rest defense, or support positions.

## 8. Action Plan and Decision Trigger Model

Ball carriers do not make full decisions every frame. They create an `ActionPlan`, then reassess when needed.

Action plan examples:

- `CarryToCrossingZone`
- `CutInside`
- `AdvanceToNextLine`
- `EscapePressure`
- `DrawPressure`
- `HoldForSupport`

A player reassesses when:

- receiving the ball
- entering a new pitch zone
- reaching the objective target
- pressure changes
- a defender enters tackle range
- a pass, cross, or shooting window opens
- a teammate starts a dangerous run
- a passing lane opens or closes
- ball control becomes poor
- tactical context changes
- max plan duration expires

Players should also perform periodic scans. Watched matches should scan frequently, for example every 0.5-1.0 simulated seconds. Background matches can scan less frequently, for example every 1.5-3.0 simulated seconds.

These values are design guidelines, not final constants.

## 9. Perception / Awareness Model

An option can exist without the player perceiving it.

Example: a striker makes a great cutback run. The winger only sees it if the perception score is high enough.

Perception factors:

- Vision
- Decisions
- Composure
- Teamwork
- pressure level
- current ball control difficulty
- body or angle abstraction
- deterministic random

High Vision and Decisions players should detect new options earlier. Low mental-quality players may continue dribbling and miss the better pass. This is how mid-dribble new pass options are supported without forcing every player to instantly know every pitch event.

## 10. Action Candidate Model

At each decision or reassessment point, the engine generates possible actions.

Candidate examples:

- `ShortPass`
- `BackPass`
- `ThroughBall`
- `SwitchPlay`
- `Carry`
- `Dribble`
- `CutInside`
- `LowCross`
- `HighCross`
- `Cutback`
- `Shoot`
- `Hold`
- `Clear`

Each `ActionCandidate` should conceptually include:

- action type
- intended target point
- target player if any
- tactical score
- context score
- mental/perception score
- skill confidence score
- pressure penalty
- final score

Selection should use a weighted deterministic choice. Better mental attributes sharpen selection toward stronger options. Lower decision quality creates more variance and more suboptimal choices.

## 11. Technical Execution Model

Decision and execution are separate.

Mental attributes mostly influence:

- what option is seen
- what option is selected
- risk management
- timing

Technical and physical attributes mostly influence:

- how well the selected action is executed
- whether the intended target is hit
- movement and arrival speed
- duels and fatigue

Technical and physical attributes can also influence action preference:

- high Dribbling and Acceleration increase willingness to carry or dribble
- high Crossing increases confidence in crosses
- high Shooting increases willingness to shoot

## 12. Ball Trajectory Model

Every pass, cross, and shot should create a `BallTrajectory` with:

- start point
- intended target
- actual target
- speed
- travel time
- trajectory type
- optional height profile

The intended target is what the player tried. The actual target is produced by execution quality, pressure, and deterministic randomness.

Poor players or pressured players can underhit, overhit, misdirect, or play the ball behind the runner.

Trajectory types:

- `GroundPass`
- `ThroughBall`
- `LowCross`
- `HighCross`
- `Cutback`
- `Shot`
- `Clearance`

## 13. Path Interception and Arrival Contest

Ball resolution is two-stage.

### A. Path Interception Contest

Defenders can cut the ball before it reaches the target. The defender does not have to run to the final target point.

The engine samples or evaluates points along the trajectory. Defender ETA to an interception point is compared with ball ETA to that point.

Interception should consider:

- positioning
- concentration
- decisions
- marking
- acceleration
- agility
- tackling
- fitness
- deterministic randomness

### B. Arrival / Reception Contest

If the ball is not intercepted on the path, players contest near the actual target.

At arrival:

- attacker may receive or shoot
- defender may block, tackle, or clear
- goalkeeper may claim or save depending on location

This supports midfield interceptions, cutback interceptions, crosses being blocked or cut out, and counterattacks from intercepted passes.

## 14. Local Contest Model

V1 contest types:

- `PassingLaneInterception`
- `GroundCrossInterception`
- `ReceptionDuel`
- `DribbleDuel`
- `TackleAttempt`
- `ShotBlock`
- `GoalkeeperSave`
- `AerialDuel`
- `LooseBallRace`

Only relevant players enter detailed contests. All players still move by shape and intent.

Contest outcome uses ETA, position, role fit, attributes, condition, context, and deterministic randomness.

## 15. Example Scenario: CM -> Winger -> Cutback to Striker

1. The central midfielder receives the ball in midfield.
2. The central midfielder generates candidates: carry, back pass, wide pass, and through ball.
3. The central midfielder picks the wide pass based on passing directness, Vision, Passing, pressure, tactical score, and deterministic randomness.
4. A `BallTrajectory` is created from the central midfielder to the winger.
5. Opponent midfielders may attempt passing lane interceptions by comparing their ETA to points along the trajectory with the ball ETA.
6. The winger receives the ball and first touch is resolved.
7. The winger chooses `CarryToCrossingZone` because of width, tempo, Dribbling, Acceleration, Crossing, and current context.
8. During the carry, periodic scans and event triggers occur.
9. The striker makes a cutback-zone run.
10. The winger may perceive the run depending on Vision, Decisions, Composure, Teamwork, pressure, ball control difficulty, and deterministic randomness.
11. The winger chooses a low cross or cutback if that candidate wins selection.
12. The intended target and actual target are separated. A pressured winger may aim at the striker but hit the ball slightly behind him.
13. Defenders choose intents based on `MarkingStyle`: `Zonal` protects the zone or intercepts the path, `Mixed` may have one defender track the runner while another cuts the lane, and `ManOriented` tracks the striker more tightly.
14. Path interception is attempted before the ball reaches the target.
15. If the cross is not cut out, the arrival contest resolves striker versus centre-back and possibly goalkeeper.
16. Outcomes include shot, block, interception, penalty, save, loose ball, missed connection, or danger cleared.
17. A watched match trace displays the movement and ball path produced by these states.

## 16. Watched vs Background Simulation Detail

All matches should use the same core engine principles. Output detail differs.

`WatchedMatch`:

- smaller step intervals
- trace frames for player markers and ball movement
- detailed events
- mini-pitch renderable stream

`BackgroundSummary`:

- larger step intervals allowed
- no full marker trace
- only result, report, stats, and key events
- avoids multi-league performance problems

`DebugFullTrace`:

- optional future mode
- can record action scores, RNG rolls, and contest details

## 17. Performance Strategy

The risk is not calculations alone. The larger risk is trace volume and over-detailed AI for every player in every match.

Rules:

- use local contests only for relevant players
- distant players follow shape and intent
- background matches avoid full trace
- watched matches can generate visual frames
- prefer deterministic, low-allocation data structures for future implementation
- do not use a full physics engine
- do not run 0.1s collision simulation
- do not store every frame for every background match

## 18. Integration with Existing Core

The future `MatchEngine` should produce a `MatchSimulationResult` / `MatchReport` output. `MatchReport` remains authoritative for persisted and applied match outcome data.

`League::applyMatchReport` remains the authoritative completion/apply path. The match engine must not update fixtures, standings, history, reports, or stats directly.

`Game` and `PlayMatchCommandHandler` orchestrate the flow. QML does not own match state and should only render backend output.

## 19. V1 / V2 / V3 Roadmap

V1:

- coordinate model
- tactical shape
- action plan and reassessment
- ball trajectory
- path interception
- local contests
- basic fatigue
- basic stats
- watched trace format skeleton

V2:

- better off-ball AI
- defensive line traps
- pressing traps
- richer set pieces
- substitutions
- injuries

V3:

- momentum
- leadership psychology
- advanced tactical instructions
- player-specific position familiarity
- richer match commentary
- advanced mini-pitch visualization
