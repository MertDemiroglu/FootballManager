# Match Engine Coordinate Simulation Design

This document captures the final architecture direction for the future coordinate-based match engine. It is design-only. It does not implement the real engine, add simulation code, change match result behavior, add mini-pitch UI, or change save/load behavior.

## Current Skeleton Status

The first compile-time core type layer for the future engine includes:

- `PitchGeometry`
- `PitchPoint`
- `MatchSimulationDetail`
- `MatchTraceFrame`
- `BallTrajectory`
- intent, action, and contest enums
- `ActionPlan`, reassessment triggers, `PerceptionModel`, `ActionCandidateGenerator`, and `ActionSelector`
- `ContestResolver`
- `TacticalZone`, `DefensiveResponsibility`, `PlayerIntentResolver`, and `MovementResolver`
- a minimal non-runtime coordinate simulation prototype

These are still foundation layers, not the runtime match system. A bounded prototype loop now wires the helpers together inside `MatchEngine::simulate`, but no full match simulation, match integration, UI rendering, report application, or save/load behavior exists yet.

`TacticalSetup` V1 now provides the tactical input fields needed by the future engine, but these fields do not affect current match results.

## Current Skeleton Implementation

The `MatchEngine` interface now exists as a compile-safe, Qt-free core boundary. It consumes `MatchEngineInput`, which is snapshot-based and uses value DTOs rather than mutable `Team`, `Footballer`, `League`, `World`, standing, fixture, or save-state references.

`MatchEngineResult` is output-only. It can carry a future authoritative `MatchReport`, team/player simulation stats, and optional trace frames. The current prototype returns local team/player stats and optional trace frames from its bounded coordinate loop. It intentionally leaves `report` empty and does not apply that result anywhere.

`MatchEngine` does not mutate domain objects and is not integrated into match playing yet. Existing match behavior remains unchanged: `PlayMatchCommandHandler` still uses the current lightweight match flow, and `League::applyMatchReport` remains the authoritative apply path.

`TeamShapeModel` now exists as the first tactical-positioning helper. It is a Qt-free, read-only model layer that converts `TeamSheet`, `TacticalSetup`, pitch context, and attacking direction into per-player target positions. It does not mutate teams, players, leagues, fixtures, standings, reports, history, save state, or UI state, and it is not wired into current live match play.

The shape model creates a base position from formation slot layout on the 105m x 68m pitch, applies small tactical adjustments for mentality, width, and defensive line, and returns `PlayerShapeTarget` values containing base, tactical, and final target positions. Defensive line affects only defensive-line and screen roles: out of possession it represents defensive block height, during defensive transition it represents recovery line height, in possession it represents rest-defense/back-line support height, and during attacking transition it represents how aggressively the back line follows the attack. For now, `finalTarget` equals the tactical target because local intent adjustments are a later phase.

`BallTrajectoryBuilder` now exists as a Qt-free helper layer for constructing deterministic `BallTrajectory` values from an intended target. The skeleton clamps safe pitch coordinates, keeps `intendedTarget` and `actualTarget` distinct, applies a simple target-error model from execution quality, pressure, and seed, reports target error as the final post-clamp deviation from intended to actual target, computes a basic speed and arrival time by trajectory type, and assigns a simple vertical flight profile plus apex height. Height is still a skeleton-level parabolic profile, not real ball physics. It also provides trajectory sampling and height-at-time helpers for future path checks. It does not model full ball physics, decide action outcomes, mutate match state, or affect current runtime match results.

`InterceptionResolver` now exists as a Qt-free helper layer for finding possible path-interception candidates along a sampled ball trajectory. It compares defender ETA from `PlayerSimState::position` with ball arrival at each sample, records candidate margins and simple quality scores, and exposes an optional best candidate. It does not decide final interception success or failure, mutate player or ball state, emit match events, or apply results.

`ContestResolver` now exists as the first local outcome resolver. It consumes copied contest participants, contest type, contest point, optional interception candidate data, timing/context values, and a deterministic seed, then returns the contest/action winner, loser, winning side, resolution type, physical ball outcome, optional clean post-contest controller, possession-change flag, attacking-success flag, defending-action-success flag, loose-ball/deflection flags, winning margin, and per-contestant score details. Its deterministic contest score uses attributes, base overall fallback, timing/arrival context, distance, fatigue, pressure on the same 0-100 convention as the rest of the skeleton, and execution quality. Winner selection then converts those non-random scores into weighted deterministic selection weights: the better contestant is more likely to win, but is not guaranteed to win, and the same seed/input gives the same selected winner. Randomness is not added to the score. It does not mutate player state, ball state, simulation state, domain objects, reports, fixtures, standings, history, save/load state, or UI state.

`TacticalZone` now exists as a simple 3x3 model from a team's attacking perspective: defensive/middle/attacking thirds crossed with left/center/right lanes. It uses `PitchGeometry` dimensions, mirrors `AwayToHome` correctly, and exposes lane/third helpers for intent and pressing decisions.

`DefensiveResponsibility` now maps a player id and `FormationSlotRole` to a primary zone, secondary zones, and naturally pressable opponent roles. It is deliberately small: zone or same/adjacent-lane relevance is required before pressing can even be considered, and `MarkingStyle` can adjust willingness without making an impossible role/zone mismatch valid.

`PlayerIntentResolver` now produces `ResolvedPlayerIntent` DTOs from copied player state, tactical setup, team shape targets, ball state, optional contest outcome, tactical zones, and role assignments. It chooses stable highest-score intents, not weighted random intents. Loose or deflected balls assign `RecoverLooseBall` only to the top few relevant nearby players; attacking teams choose off-ball support/run/shape intents by role; defending teams choose hold, cover, mark, contain, press, tackle, block, protect, or recover intents with role/zone/distance constraints. It does not mutate `MatchSimulationState`, domain objects, reports, fixtures, standings, history, save/load, or UI state.

`MovementResolver` now exists as a non-mutating movement helper. Given a copied `PlayerSimState`, `PlayerIntent`, target, and `deltaSeconds`, it returns the previous position, target, clamped new position, distance before movement, distance moved, and whether the target was reached. Speed uses `effectivePace` when available, a safe 6.0 m/s fallback otherwise, fatigue penalty, and an urgency multiplier.

`ActionPlan` now exists as the first ball-carrier planning skeleton. It stores the current plan type, objective target, start time, duration limit, periodic reassessment interval, and last scan time. The reassessment helper represents both event-triggered scans, such as receiving the ball, entering a new zone, pressure changes, tackle-range danger, passing or shooting windows, dangerous teammate runs, and control worsening, plus periodic scans and max-duration expiry.

`PerceptionModel` now separates option existence from option awareness. It uses Vision, Decisions, Composure, Teamwork, pressure, ball-control difficulty, option quality, and a deterministic seed influence to answer whether the player notices an available option. The scoring is intentionally simple and is not tuned as real football behavior yet.

`ActionCandidateGenerator` now produces a small, deterministic `BallCarrierActionType` candidate list from broad pitch context: hold, back pass, short pass, carry, low cross, shoot, and clear where appropriate. `BallCarrierActionType` represents only the decision made by the player currently controlling the ball. `PlayerIntentType` represents off-ball movement, defensive shape, pressing/marking, loose-ball reaction, or emergency intent. The generator uses `PitchGeometry` for rough areas and only light `TacticalSetup` hints for safe, direct, or attacking options. `ActionSelector` ranks candidates by score and chooses through weighted deterministic selection, with higher Decisions sharpening the choice toward stronger candidates and lower Decisions allowing more variance.

`BallTrajectoryBuilder` also exposes a deterministic deflected-ball trajectory helper. It creates a short `BallTrajectoryType::Deflection` path from contact point, incoming path, strength, start second, and seed, with a low or medium vertical profile based on deflection strength. It only produces a future trajectory DTO; it does not apply `BallState`, decide recovery, or change possession.

These planning helpers, contest resolver, intent resolver, movement resolver, tactical-zone layers, and deflection helper are now wired by the minimal coordinate simulation prototype. They remain outside live match play: the prototype does not replace `MatchSimulation`, produce reports, update fixtures, standings, history, save/load, or UI state. Existing match behavior remains unchanged.

## Minimal Coordinate Simulation Prototype

`CoordinateSimulationPrototype` is the first small non-runtime loop behind `MatchEngine::simulate`. For valid snapshot input, `MatchEngine::simulate` delegates to the prototype; invalid input still returns a default result.

The prototype initializes a local `MatchSimulationState` from `MatchEngineInput`: match id, league id, first-half phase, local home/away `TeamSimState`, starter `PlayerSimState` values from the snapshot/team sheet, ball state, and possession state. It uses `TeamShapeModel` to place starters at their initial shape targets, gives the home team deterministic kickoff possession, and mutates only that local state copy.

Each bounded action step wires the existing helper layers in order: `TeamShapeModel`, `PlayerIntentResolver`, `MovementResolver`, `ActionCandidateGenerator`, `ActionSelector`, `BallTrajectoryBuilder`, `InterceptionResolver`, and `ContestResolver`. Controlled balls can hold, carry/dribble, pass/cross/clear, or shoot into a trajectory. In-flight balls can be intercepted, deflected, become loose, reach a nearby intended receiver, route high balls toward aerial/reach-aware contests, or resolve an on-target shot through a local goalkeeper-save contest. Loose balls can be recovered by a nearby player.

Shot/save/goal handling now exists only inside this local prototype. Prototype goals update `MatchEngineResult` team/player stats and trace frames, then reset the local ball for a simple kickoff. They do not create `MatchReport`, do not call `League::applyMatchReport`, and do not update fixtures, standings, history, save/load state, or UI. Rebounds and deflections remain simplified local trajectories or loose balls.

The prototype returns approximate possession, pass, shot, interception, and per-player stats plus watched/debug trace frames through `MatchEngineResult`. It does not create or apply `MatchReport`, does not call `League::applyMatchReport`, does not emit domain events, and does not mutate `Team`, `League`, `World`, fixtures, standings, reports, history, save/load, or UI state.

## MatchEngineInputBuilder

`MatchEngineInputBuilder` is the read-only boundary between current domain state and future snapshot-based match simulation. It converts existing `Team`, `Footballer`, and `TeamSheet` state into immutable `MatchEngineInput` values without transferring ownership or storing mutable domain references inside the match engine input.

The builder creates `MatchTeamSnapshot` and `MatchPlayerSnapshot` values from the current team rosters and the provided team sheets. `TeamSheet.tacticalSetup` is the tactical source of truth for the snapshot. The builder does not read UI state, save metadata, standings, fixtures, or league history.

The builder is intentionally not a simulation entry point. It does not call `MatchEngine::simulate`, does not produce or apply match reports, does not publish events, and does not mutate teams, players, team sheets, leagues, world state, fixtures, standings, history, or save state. Current runtime match behavior remains unchanged.

When the caller leaves `MatchEngineOptions::deterministicSeed` as zero, the builder fills it with a stable deterministic seed derived from match id, league id, match date, and home/away team ids. It does not depend on wall-clock time.

The builder validates obvious structural problems before returning input: non-zero match, league, and team ids; distinct home and away teams; valid team sheets for the provided teams; roster membership for referenced starters and substitutes; duplicate player ids inside snapshots; and players appearing in both home and away snapshots.

## Internal Simulation State Skeleton

`MatchSimulationState` is the future in-memory state container for the coordinate-based match engine. It holds the live clock and phase, home and away team state, player state, ball state, possession state, and optional trace frames for watched/debug output.

`PlayerSimState` stores a player's live coordinate position, tactical or intent target position, current intent, ball ownership flag, future live fatigue/derived physical values, and the snapshot base overall for quick debugging.

`TeamSimState` stores the side, tactical setup, live goals accumulator, future possession accumulator, and the team's player simulation states. `BallState` stores the current ball control mode, position, future carrier ids, and optional trajectory. `PossessionState` stores the current team/player possession identifiers and possession phase metadata.

This state is not persisted, does not replace `MatchReport`, and is not wired into the current match flow. `MatchReport` remains the authoritative output for completed matches, and existing match result behavior remains unchanged.

Future integration path:

```text
PlayMatchCommandHandler
  -> MatchEngineInputBuilder::build
  -> MatchEngine::simulate
  -> MatchReport
  -> League::applyMatchReport
```

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

The current `TeamShapeModel` skeleton provides those default target positions for off-ball movement. It maps starters from `TeamSheet` formation slots to `PitchPoint` targets, mirrors the target shape for away-team attacking direction, clamps output to pitch boundaries, and keeps the result deterministic. This is structural foundation only; it does not implement player movement, contests, action selection, or match result changes.

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

`TacticalSetup` V1 now stores the required tactical inputs as core TeamSheet state:

- Mentality
- Tempo
- Width
- DefensiveLine
- PressingIntensity
- MarkingStyle
- PassingDirectness

These fields are persisted with selected `TeamSheet` runtime state, but they do not yet affect current match results. The current UI still only exposes mentality and tempo. Future tactical UI can expose More Options without changing the principle that core owns match decisions and QML only edits user-facing setup.

`DefensiveLine` is intentionally a positional shape adjustment, not pressing behavior. In `OutOfPossession` it raises or drops the defensive block, in `DefensiveTransition` it shifts the recovery line, in `InPossession` it shifts rest-defense/back-line support, and in `AttackingTransition` it controls how far the back line follows the attack.

`PressingIntensity` is intentionally not movement yet. It will later influence local intents such as `PressBallCarrier`, `ContainBallCarrier`, and defensive recovery urgency.

`MarkingStyle` is intentionally not target selection yet. It will later influence whether defenders protect zones, combine zone and runner tracking, or follow opponents more tightly.

`PassingDirectness` is intentionally not action scoring yet. It will later influence `ActionCandidate` generation and weighting for shorter support play, balanced progression, or more direct forward passes.

## 6. Marking Style

`MarkingStyle` should behave like an enum with these values:

- `Zonal`
- `Mixed`
- `ManOriented`

`Zonal` prioritizes space and passing lanes. It gives better shape stability and compactness, but can lose individual runners when an attacker times movement well.

`Mixed` is the default balanced model. It combines space coverage and runner tracking. Example: one centre-back tracks the striker while another protects the passing or crossing lane.

`ManOriented` uses tighter opponent following. It improves runner tracking, but may open exploitable space and consume more stamina.

The current `TeamShapeModel` does not select man or zone targets yet. Future behavior should be:

- `Zonal`: protect zones and passing lanes.
- `Mixed`: allow one defender to track a runner while another cuts a lane.
- `ManOriented`: apply tighter opponent tracking.

Marking style affects:

- defender intents
- whether a centre-back tracks the striker or protects a zone
- whether a defensive midfielder blocks a passing lane or follows a runner
- defensive compactness
- cross and cutback reactions

## 7. Player Intent Model

Every player gets an intent each step, but only relevant players enter detailed contests.

`PlayerIntentType` is one enum for now, grouped by meaning:

- Neutral/fallback: `None`
- Attacking off-ball: `HoldAttackingShape`, `MoveToSupport`, `DropForPass`, `MakeRunBehind`, `AttackNearPost`, `AttackFarPost`, `AttackCutbackZone`, `OccupyWidth`, `OccupyHalfSpace`
- Defensive: `HoldDefensiveShape`, `CoverSpace`, `MarkOpponent`, `PressBallCarrier`, `ContainBallCarrier`, `BlockPassingLane`, `InterceptBallPath`, `AttemptTackle`, `RecoverToGoal`, `ProtectGoalZone`
- Loose/deflected reaction: `RecoverLooseBall`
- Emergency: `ClearDanger`

Intent examples:

- `HoldAttackingShape`
- `HoldDefensiveShape`
- `MoveToSupport`
- `MakeRunBehind`
- `AttackNearPost`
- `AttackFarPost`
- `AttackCutbackZone`
- `DropForPass`
- `OccupyWidth`
- `OccupyHalfSpace`
- `PressBallCarrier`
- `ContainBallCarrier`
- `MarkOpponent`
- `CoverSpace`
- `ProtectGoalZone`
- `BlockPassingLane`
- `InterceptBallPath`
- `AttemptTackle`
- `RecoverToGoal`
- `RecoverLooseBall`
- `ClearDanger`

Off-ball players still act. Nearby or relevant players make richer decisions, while distant players mostly follow tactical shape, rest defense, or support positions.

The current `PlayerIntentResolver` skeleton applies this grouping without running a full simulation loop. It uses team mode, role, ball zone, shape target, distance, `DefensiveResponsibility`, `MarkingStyle`, `PressingIntensity`, and optional contest outcome to choose an intent and final target. It returns DTOs only and leaves recovery application, ball state updates, reports, and trace generation to future phases.

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

The current implementation contains the compile-safe DTOs and helpers for this model only. `evaluateReassessmentTriggers` reports explicit event triggers, `PeriodicScanDue`, and `MaxDurationExpired`, but it does not perform deep perception, tactical context analysis, or live state mutation yet.

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

The current `PerceptionModel` is a small deterministic skeleton. It keeps the option-awareness concept available to future action planning without affecting current match results.

## 10. Action Candidate Model

At each decision or reassessment point, the engine generates possible ball-carrier actions.

`BallCarrierActionType` candidate examples:

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

The current `ActionCandidateGenerator` and `ActionSelector` implement this as a future-expandable skeleton only. Candidate scoring is broad and deterministic, and selected actions are not executed by the live game.

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
- vertical flight profile and apex height

The intended target is what the player tried. The actual target is produced by execution quality, pressure, and deterministic randomness.

The current `BallTrajectoryBuilder` skeleton represents this principle in code. Its error model is intentionally simple: lower execution quality and higher pressure increase the possible deviation from the intended target, while a deterministic seed keeps repeat runs stable. Each trajectory also has a simple `BallFlightProfile` and `apexHeightMeters`; `ballHeightAtProgress` / `ballHeightAtSecond` use a parabolic placeholder so high crosses, clearances, lofted balls, shots, and low/ground balls no longer all behave as purely 2D path checks. The reported `targetErrorMeters` diagnostic is the actual final post-clamp distance between intended target and actual target.

Poor players or pressured players can underhit, overhit, misdirect, or play the ball behind the runner.

Trajectory types:

- `GroundPass`
- `ThroughBall`
- `LowCross`
- `HighCross`
- `Cutback`
- `Shot`
- `Clearance`
- `Deflection`

Current flight profiles are deliberately coarse: ground passes stay near the pitch, low crosses and through balls stay low, high crosses and clearances become reachable only by plausible aerial reach, shots use a simple medium-low placeholder, and deflections choose low/medium height from deflection strength. Because player height is not modeled yet, reach checks use temporary attribute-only formulas from Jumping Reach, Heading, Agility, Tackling, and goalkeeper Aerial Ability.

## 13. Path Interception and Arrival Contest

Ball resolution is two-stage.

### A. Path Interception Contest

Defenders can cut the ball before it reaches the target. The defender does not have to run to the final target point.

The engine samples or evaluates points along the trajectory. Defender ETA to an interception point is compared with ball ETA to that point.

The current `InterceptionResolver` skeleton performs this path sampling and ETA comparison. It returns candidates and a best candidate for contest resolution, but it does not decide whether a pass, cross, shot, or clearance is successfully intercepted. `CoordinateSimulationPrototype` now filters those local candidates against the trajectory height before creating a contest, so an avoidably unreachable high ball is not treated exactly like a ground pass. `ContestResolver` consumes local participants and optional interception candidate data to decide the local contest outcome.

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

Contest action winner selection uses ETA, position, role fit, attributes, condition, context, and deterministic weighted selection. The score remains deterministic; the deterministic seed is used only to roll against selection weights, so a stronger contestant has a higher chance without becoming an automatic winner.

The current `ContestResolver` skeleton separates the contest/action winner from the ball outcome after the winning action. `winner` means the player who won the contest action. `ballOutcome` describes what physically happened to the ball, and `cleanController` is set only when a player actually controls it after the contest. `possessionChanges` is true only when clean control changes side. `ballDeflected` means the ball was touched or redirected without an assigned clean controller. `ballBecomesLoose` means no clean controller exists and the outcome is not specifically a deflection.

This distinction matters for interceptions and tackles. A defender can win a passing-lane interception cleanly and control the ball, deflect it away, or knock it loose. A defender can also win a tackle attempt while the ball pops loose or deflects away; that is still a successful defensive action, not a no-winner close contest. Close scores are no longer universally converted into loose or deflected balls.

`GoalkeeperSave` can now represent save-and-hold, save-and-parry, loose rebound, or shot-beats-keeper outcomes. Inside the local prototype only, shot-beats-keeper on an on-target shot can increment prototype goals and append a `Goal` trace in `MatchEngineResult`. It still does not apply reports, assists, fixtures, standings, history, or runtime match completion. Deflected and loose-ball recovery remains simplified.

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
