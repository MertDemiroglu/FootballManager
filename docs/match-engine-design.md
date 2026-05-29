# Match Engine Coordinate Simulation Design

This document captures the current architecture for the coordinate-based match engine. The coordinate engine is now the default playable match path, while the lightweight engine remains available as a fallback and compatibility mode.

## PR87 Layered Architecture

PR87 is a behavior-preserving architecture cleanup. It organizes the current coordinate match engine into clearer layers before future tactical and decision complexity is added. It should not be used for gameplay tuning unless a small compile fix requires it.

### Public API

The stable public entry point remains `MatchEngine::simulate`. Callers build `MatchEngineInput`, choose `MatchSimulationDetail` through the input options, and consume `MatchEngineResult`. `MatchEngine.h`, `MatchEngineInput.h`, `MatchEngineResult.h`, and core shared DTOs remain at the top of `fm/match_engine`.

Routing remains unchanged:

- `BackgroundSummary` delegates to `simulation/FastMatchSummarySimulator`.
- `WatchedMatch` delegates to `simulation/CoordinateMatchSimulator`.
- `DebugFullTrace` delegates to `simulation/CoordinateMatchSimulator`.

Managed and user-visible matches should continue using the detailed coordinate path. Background AI fixtures should continue using the fast summary path. The lightweight fallback remains outside this engine as the compatibility safety path, and `League::applyMatchReport` remains the authoritative report application flow.

### Runners

`simulation/CoordinateMatchSimulator` owns the detailed user/watched/debug simulation loop. It wires the layer helpers together but should not become the home for every future constant or decision rule.

`simulation/FastMatchSummarySimulator` owns the aggregate AI/background simulation path. It remains separate from the detailed simulator so background fixture processing stays cheap.

### Layers

- `decision`: ball-carrier candidate generation, action selection, player intent resolution, action planning, defensive responsibility, and future decision tuning DTOs.
- `ball`: ball trajectory creation, trajectory DTOs, trajectory sampling, deflection helpers, and open-play shot quality/xG helpers.
- `contest`: interception candidate resolution, duel/save/contest resolution, and contest type enums.
- `geometry`: pitch dimensions, goal/box helpers, distance helpers, and tactical zone definitions.
- `movement`: player movement resolution and team shape target modeling.
- `reporting`: `MatchEngineResult` to `MatchReport` mapping, including team stats, player reports, and official event mapping.
- `tuning`: a small initial `MatchEngineTuningProfile` home for future centralized constants.

### Follow-Up Plan

- PR88 Passing Decision Layer
- PR89 Carry + Shot Decision Layer
- PR90 Pressing & Marking Interaction
- PR91 ActionPlan System
- PR92 Detailed Coordinate Match Tuning

Future PRs should move decision constants into the decision/tuning layers gradually instead of scattering unexplained magic numbers through simulation runner code.

## Current Status

The first compile-time core type layer for the future engine includes:

- `PitchGeometry`
- `PitchPoint`
- `MatchSimulationDetail`
- `MatchTraceFrame`
- `BallTrajectory`
- intent, action, and contest enums
- `ActionPlan`, reassessment triggers, `PerceptionModel`, `BallCarrierDecisionModel`, and `ActionSelector`
- `ContestResolver`
- `TacticalZone`, `DefensiveResponsibility`, `PlayerIntentResolver`, and `MovementResolver`
- the default playable coordinate match simulator
- `FastMatchSummarySimulator` for scalable background fixtures

These layers now power the default runtime match flow. `MatchEngine::simulate` uses copied snapshots and routes by `MatchSimulationDetail`: `BackgroundSummary` runs the fast aggregate summary runner, while `WatchedMatch` and `DebugFullTrace` run the full coordinate loop. Background and watched/debug share the same input contract and report contract, but they do not share the same internal runner or cost profile. The caller owns the detail policy: AI/background fixtures use `BackgroundSummary`; managed/user-visible matches, including fast-forwarded user matches, use `WatchedMatch`; developer diagnostics use `DebugFullTrace`. `PlayMatchCommandHandler` validates the returned `MatchReport` before passing it to `League::applyMatchReport`. If the coordinate report is missing or unsafe, the handler falls back to the lightweight `MatchSimulation`.

`TacticalSetup` V1 now provides tactical input to both coordinate paths. Mentality, tempo, width, defensive line, and passing directness are used as first-pass inputs; tuning remains ongoing.

## Current Implementation

The `MatchEngine` interface now exists as a compile-safe, Qt-free core boundary. It consumes `MatchEngineInput`, which is snapshot-based and uses value DTOs rather than mutable `Team`, `Footballer`, `League`, `World`, standing, fixture, or save-state references.

`MatchEngineResult` is output-only. It carries the coordinate `MatchReport`, team/player simulation stats, official match events, optional trace frames, and the simulated match clock. The simulator returns local team/player stats and official events independently from trace output. A focused `MatchEngineReportAdapter` converts the simulator result metadata, score, lineup snapshots, player reports, official events, and basic team stats into `MatchReport`.

`MatchEngine` does not mutate domain objects. `PlayMatchCommandHandler` defaults to `Coordinate`, builds `MatchEngineInput` through `MatchEngineInputBuilder`, runs `MatchEngine::simulate`, and uses `result.report` only after validating ids, metadata, goals, lineup team ids, and player-report team ids against the command. Its default coordinate detail remains `BackgroundSummary` for scalable AI/runtime fixture processing, while `Game::playPendingPreMatch` passes `WatchedMatch` for managed-team matches. Future Instant Result can explicitly choose fast summary, but user-visible watched or fast-forwarded matches should stay on `CoordinateMatchSimulator`. Any unsafe build, missing report, or mismatched report falls back to the lightweight `MatchSimulation` report. `League::applyMatchReport` remains the authoritative apply path.

`TeamShapeModel` now exists as the first tactical-positioning helper. It is a Qt-free, read-only model layer that converts `TeamSheet`, `TacticalSetup`, pitch context, and attacking direction into per-player target positions. It does not mutate teams, players, leagues, fixtures, standings, reports, history, save state, or UI state.

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

`BallCarrierDecisionModel` now produces the deterministic `BallCarrierActionType` candidate list from shared decision context: hold, multiple pass options, carry, shoot, and clear where appropriate. `BallCarrierActionType` represents only the decision made by the player currently controlling the ball. `PlayerIntentType` represents off-ball movement, pass reception, defensive shape, pressing/marking, loose-ball reaction, or emergency intent. Pass, carry, and shot option information is delegated to focused decision evaluators, while the ball-carrier model combines category-level suitability with option-level data. `ActionSelector` ranks candidates by score and chooses through weighted deterministic selection, with higher Decisions sharpening the choice toward stronger candidates, lower Decisions allowing more variance, and passing/shooting/carrying attributes nudging action choice without making outcomes deterministic.

## PR88 Passing Decision Layer

PR88 introduces `decision/PassOptionEvaluator` as the first focused ball-carrier decision layer on top of the PR87 architecture. The evaluator creates scored pass options instead of relying on nearest-team-mate candidate generation inside the ball-carrier candidate assembly path.

The passing layer can evaluate safe passes, back passes, progressive passes, switches, through balls, crosses, and cutbacks when the pitch context supports them. Not every kind exists in every possession: crosses and cutbacks require suitable wide/final-third contexts, switches need a meaningful wide-side target, and through balls need a plausible receiver/run/space.

Option scoring is influenced by passer Passing, Vision, Decisions, Technique, and Composure; the carrier's formation role and risk profile; tactical mentality, tempo, passing directness, and width; pressure on the carrier; receiver pressure; pass distance; lane risk; progression value; receiver role/zone; and the current pitch zone. Defensive and low-tempo setups increase safety and support circulation without forbidding progression. Attacking, direct, high-tempo, or wide setups raise progressive, through, switch, cross, and cutback desire without forcing low-quality choices every time.

`BallCarrierDecisionModel` now converts evaluator output into `ActionCandidate` values, while `ActionSelector` keeps the existing weighted deterministic selection model. Higher-scored pass options are more likely, but the best score is not guaranteed. Better Decisions sharpen the choice toward stronger options; lower Decisions leave more variance. Same input and same seed still produce the same selected behavior.

The evaluator includes simple lane-risk and receiver-pressure estimates. Lane risk checks defenders near the segment from ball to target and accounts for distance/pass kind; receiver pressure checks nearest and nearby opponents. This is deliberately not a full pressing or marking overhaul. PR89 should move Carry and Shot decisions into comparable focused layers, and PR90 should deepen pressing/marking interaction.

`BallTrajectoryBuilder` also exposes a deterministic deflected-ball trajectory helper. It creates a short `BallTrajectoryType::Deflection` path from contact point, incoming path, strength, start second, and seed, with a low or medium vertical profile based on deflection strength. It only produces a future trajectory DTO; it does not apply `BallState`, decide recovery, or change possession.

These planning helpers, contest resolver, intent resolver, movement resolver, tactical-zone layers, and deflection helper are wired by the coordinate match simulator. The simulator itself still does not update fixtures, standings, history, save/load, or UI state; the handler applies a validated report through `League::applyMatchReport`.

## PR89 Carry + Shot Decision Layer

PR89 introduces `decision/CarryOptionEvaluator` and `decision/ShotDecisionEvaluator` so the three ball-carrier action categories now have focused decision-layer components. It also adds `decision/BallCarrierDecisionModel` as the central combination layer for pass, carry, shot, hold, and emergency clear options. `ActionCandidateGenerator` was removed so it can no longer drift into the tactical decision brain.

`CarryOptionEvaluator` evaluates safe carry, progressive carry, and dribble options. Carry scoring depends on formation role, current zone, target zone, available space, nearest-opponent pressure, control difficulty, Dribbling, Technique, Decisions, Pace, Acceleration, Composure, mentality, tempo, and passing directness. Role/zone limits are soft decision biases rather than hard action-plan locks: defenders are penalized for carrying too far beyond their reasonable build-up zones, midfielders can progress through midfield, wide defenders and wide attackers are rewarded for wide-lane carries, and attacking midfielders/wingers/strikers can carry more aggressively in advanced areas.

`ShotDecisionEvaluator` evaluates open-play shot desire separately from shot conversion. It uses `ShotQualityModel` for estimated open-play xG and then scores whether the current player wants to shoot based on xG, distance, real goal angle, pressure, role, Shooting, Technique, Composure, Decisions, mentality, tempo, and pitch zone. Roles and tactics are contextual biases, not hard locks: strikers, attacking midfielders, wingers, and midfielders can all shoot from reasonable contexts, while defensive roles are naturally discouraged from unrealistic long shots by chance quality, distance, pressure, and role preference. `ShotQualityModel` remains the chance-quality model: it answers how valuable the shot location/context is. `ShotDecisionEvaluator` answers whether this player should select the shot as an action. Goal conversion remains in the simulator's shot/save flow.

The first follow-up implementation tried to control behavior through cross-action score normalization tables and possession-depth urgency inside `ActionCandidateGenerator`. That approach was removed. Candidate assembly should not force safer actions down or progressive/terminal actions up through global action-depth score tables. Long possession, pressure, fatigue, space, and tactics should influence player decisions through named context models and evaluator profiles rather than scattered post-processing numbers.

`DecisionContext.h` adds shared read-only contexts: `MatchContext`, `PossessionContext`, `DefensiveContext`, `TeamDecisionContext`, and `PlayerDecisionContext`. The existing possession `actionDepth` is exposed to decision layers as `possessionActionCount`, meaning the number of controlled actions in the current possession. It is informational context, not a global penalty table. The coordinate simulator now keeps minimal possession memory (`possessionStartPoint`, `lastMeaningfulProgressionPoint`, and `lastMeaningfulProgressionSecond`) so `PossessionContext` can report real `secondsInPossession`, `secondsSinceLastMeaningfulProgression`, `ballProgression`, and `recentProgression`.

`PossessionContext` now computes progression and circulation availability from current teammate positions, opponent pressure, and simple lane pressure rather than static placeholder values. It also estimates opponent block compactness from nearby defender density and spread around the ball. Short-pass loop prevention is therefore handled through context and defensive response: long non-progressive possession can make the defending team more compact and locally active, which changes pressure and lane quality, but there is no hidden "after N passes, punish short pass" score table.

`DefensiveContext` now feeds the existing `PlayerIntentResolver` at a basic level. It derives opponent possession duration, opponent phase, threat level, local press opportunity, block compactness, and press triggers from the current state. When the opponent has stale possession or possession near dangerous zones, only lane/zone-relevant defenders become more likely to press, contain, cover, or block lanes; players far from their responsibility still mostly hold shape.

`BallCarrierDecisionModel` is the new home for combining evaluator outputs. In this PR it remains deliberately simple: it collects `PassOptionEvaluator`, `CarryOptionEvaluator`, and `ShotDecisionEvaluator` output, evaluates pass/carry/shoot category suitability from `PlayerDecisionContext`, and adapts options into weighted `ActionCandidate` values for `ActionSelector`. Category suitability reads phase, progression availability, safe circulation availability, recent progression, local pressure, role, tactics, and ball location without fixed category percentages.

Defensive response now affects pass execution rather than just intent labels. Pressing, containing, blocking, covering, and marking defenders can increase contextual pass pressure, qualify as routine-pass interception candidates through lane/receiver/carrier relevance, and feed `PassResolutionModel` at the pass arrival point. That pass-resolution seam separates clean receive, contested receive, defender intercept, deflected loose ball, and misplaced loose ball outcomes so a defensive intervention cannot also become completed-pass credit on the same pass. Repeated short circulation becomes less comfortable because defenders move, contest lanes, and can create real turnovers or loose-ball recovery races, not because a global counter punishes safe passes.

`ShotQualityModel` remains a chance-quality model, not a scoreline-balancing device. It estimates open-play xG from shot location, real goal angle, and pressure, without shooter skill, and keeps a realistic open-play upper bound rather than hard-capping chances at 0.20. If match-level xG becomes inflated, the right fixes are shot selection, defensive pressure, chance creation, positioning, and contest behavior, not an unrealistic xG clamp.

`ShotTargetModel` is introduced as a small seam between deciding to shoot and deciding where to aim. The first implementation remains simple and deterministic, but shot targeting no longer has to live permanently inside `ShotDecisionEvaluator`. `ShotOutcomeResolver` is also introduced as a light seam for final shot outcome probability, keeping the conceptual separation between shot desire, chance quality/xG, shot target/placement, and conversion/save outcome.

`PlayerRatingModel` centralizes the first-pass player rating calculation from available match stats. It keeps ratings approximate for now, but removes rating arithmetic from the coordinate simulator and gives future reporting work a focused home.

The selector remains weighted and deterministic. Evaluators score options; `ActionSelector` still turns candidate scores into a deterministic weighted choice where higher scores are more likely, better Decisions sharpen selection, lower Decisions allow more variance, and the top score is not automatically chosen.

PR90 should deepen pressing and marking interaction using `DefensiveContext`. PR91 should add full `ActionPlan` and reassessment behavior so carry plans can be revisited as the player enters new zones, pressure changes, or passing/shooting windows open. Future work should also deepen phase modeling, real rebounds/corners/fouls, goalkeeper modeling, and shot outcome handling.

## Coordinate Match Simulator

`CoordinateMatchSimulator` is the small playable detailed coordinate loop behind `MatchEngine::simulate` for `WatchedMatch` and `DebugFullTrace`. For `BackgroundSummary`, `MatchEngine::simulate` instead delegates to `FastMatchSummarySimulator`; invalid input still returns a default result.

The simulator initializes a local `MatchSimulationState` from `MatchEngineInput`: match id, league id, first-half phase, local home/away `TeamSimState`, starter `PlayerSimState` values from the snapshot/team sheet, ball state, and possession state. It uses `TeamShapeModel` to place starters at their initial shape targets, gives the home team deterministic kickoff possession, and mutates only that local state copy.

The simulator now runs until the local `matchSecond` reaches regulation time, with a safety action counter only as infinite-loop protection. It is possession/action/contest based rather than minute-by-minute or frame-by-frame. Controlled actions, in-flight trajectories, loose-ball races, contests, and simple goal restarts each return elapsed simulated seconds, and the clock advances by those natural action durations.

Each action step wires the existing helper layers in order: `TeamShapeModel`, `PlayerIntentResolver`, `MovementResolver`, `BallCarrierDecisionModel`, `ActionSelector`, `BallTrajectoryBuilder`, `InterceptionResolver`, and `ContestResolver`. Controlled balls can hold, carry/dribble, pass/cross/clear, or shoot into a trajectory. In-flight balls can be intercepted, deflected, become loose, reach a nearby intended receiver, route high balls toward aerial/reach-aware contests, or resolve an on-target shot through a local goalkeeper-save contest. Intended receivers now get explicit receive-pass behavior and a dynamic reception range based on pass type, execution quality, first touch/technique/composure, and pressure. Routine back/short passes also require higher interception quality than through balls or crosses, and routine interception contests are biased toward defenders with relevant lane/block/intercept behavior rather than every defender freely contesting every pass. Loose balls can be recovered by a nearby player.

Shot/save/goal handling exists inside this local simulator. Coordinate goals update `MatchEngineResult` team/player stats, record assists from recent intentional same-team passes where applicable, append official `MatchEventRecord` goal events, then reset the local ball for a simple kickoff after a deterministic restart delay. Watched/debug traces may also record goal markers, but official report events do not depend on trace frames. The simulator does not apply the report; `PlayMatchCommandHandler` validates it and calls `League::applyMatchReport`. Rebounds and deflections remain simplified local trajectories or loose balls. First-pass guardrails now gate low-value shot selection, use `ShotQualityModel` to estimate open-play xG from shot location, real goal angle, and pressure, and keep shooter/goalkeeper attributes out of the xG number. `ShotTargetModel` handles intended target/placement, and `ShotOutcomeResolver` uses xG, placement, shooter attributes, goalkeeper strength, pressure, and seed for the final conversion seam.

Assist tracking remains conservative: only recent completed same-team intentional passes can become assists, opponent touches/turnovers/loose-ball recoveries clear the assist context, and rebounds are not automatically assisted. Better reception/pass completion should make possession-created goals surface assists more naturally without inventing assists for solo turnover goals. The detailed simulator also keeps internal pass-failure counters for intercepted, deflected, loose, and receiver-out-of-range passes so future tuning can separate failure causes without exposing those diagnostics to the UI yet.

The simulator returns approximate possession, pass, shot, interception, expected-goal, and per-player stats plus watched/debug trace frames through `MatchEngineResult`. The adapter maps supported report fields: metadata, score, lineups, player report basics including ratings, official goal events including assists, and basic home/away team stats. Player ratings are first-pass and team-result aware: goals/assists remain important, volume-only action rewards are capped, defenders/goalkeepers receive clean-sheet/conceded-goal context, and ordinary players are capped below 10.0 unless they produce exceptional goal/assist output. The simulator itself does not apply `MatchReport`, does not call `League::applyMatchReport`, does not emit domain events, and does not mutate `Team`, `League`, `World`, fixtures, standings, reports, history, save/load, or UI state.

## Fast Background Summary

`FastMatchSummarySimulator` is the scalable `BackgroundSummary` path. It is not the old lightweight random score generator; it is a cheaper aggregate version of the coordinate model that consumes the same `MatchEngineInput` snapshots, detailed `PlayerAttributes`, starting XI roles/formation, broad formation shape, goalkeeper quality, home/away context, deterministic seed, and `TacticalSetup`.

The summary computes directional team strengths for attack, chance creation, finishing, defending, goalkeeper prevention, possession, tempo, pass volume, pass accuracy, corners, volatility, and risk. It then produces bounded deterministic shots, shots on target, expected goals, goals, possession, passing, corners, fouls, official goal events, scorer/assist player stats, minutes, and ratings. It does not emit marker trace or run the full movement/trajectory/contest loop, which keeps AI and multi-league fixtures cheap while preserving the same report schema as detailed coordinate matches.

Background and watched/debug modes therefore share the same input contract and report contract, but not the same internal runner or cost profile. `BackgroundSummary` is the fast aggregate AI/background path. `WatchedMatch` and `DebugFullTrace` use the detailed coordinate simulator. User-managed, watched, or fast-forwarded matches should remain detailed coordinate unless a future explicit Instant Result option chooses summary.

Fast summary tactical sensitivity is intentionally bounded but visible:

- Attacking raises shot volume and xG while increasing risk conceded.
- Defensive lowers own volume and xG while improving defensive resistance.
- High tempo raises action/shot volume, lowers pass accuracy, and increases volatility.
- Low tempo improves pass accuracy/control while lowering shot volume.
- Wide play increases corners and wide-player involvement.
- Narrow play improves central possession and reduces crossing/corner volume.
- Direct passing lowers passes and pass accuracy while increasing shot volume and volatility.
- Short passing raises passes, pass accuracy, and possession while lowering direct shot volume.
- High defensive line increases pressure/chance creation and risk behind.
- Low defensive line lowers opponent shot quality and own chance volume.

Open-play scorer selection is role and attribute weighted. Strikers, wingers, and attacking midfielders are the main scorer pool; central midfielders can score occasionally; defenders are rare until explicit set-piece logic exists. Goalkeepers are excluded from open-play scorer and assist selection for now.

## MatchEngineInputBuilder

`MatchEngineInputBuilder` is the read-only boundary between current domain state and future snapshot-based match simulation. It converts existing `Team`, `Footballer`, and `TeamSheet` state into immutable `MatchEngineInput` values without transferring ownership or storing mutable domain references inside the match engine input.

The builder creates `MatchTeamSnapshot` and `MatchPlayerSnapshot` values from the current team rosters and the provided team sheets. `TeamSheet.tacticalSetup` is the tactical source of truth for the snapshot. The builder does not read UI state, save metadata, standings, fixtures, or league history.

The builder is intentionally not a simulation entry point. It does not call `MatchEngine::simulate`, does not produce or apply match reports, does not publish events, and does not mutate teams, players, team sheets, leagues, world state, fixtures, standings, history, or save state.

When the caller leaves `MatchEngineOptions::deterministicSeed` as zero, the builder fills it with a stable deterministic seed derived from match id, league id, match date, and home/away team ids. It does not depend on wall-clock time.

The builder validates obvious structural problems before returning input: non-zero match, league, and team ids; distinct home and away teams; valid team sheets for the provided teams; roster membership for referenced starters and substitutes; duplicate player ids inside snapshots; and players appearing in both home and away snapshots.

## Internal Simulation State Skeleton

`MatchSimulationState` is the future in-memory state container for the coordinate-based match engine. It holds the live clock and phase, home and away team state, player state, ball state, possession state, and optional trace frames for watched/debug output.

`PlayerSimState` stores a player's live coordinate position, tactical or intent target position, current intent, ball ownership flag, future live fatigue/derived physical values, and the snapshot base overall for quick debugging.

`TeamSimState` stores the side, tactical setup, live goals accumulator, future possession accumulator, and the team's player simulation states. `BallState` stores the current ball control mode, position, future carrier ids, and optional trajectory. `PossessionState` stores the current team/player possession identifiers and possession phase metadata.

This state is not persisted and does not replace `MatchReport`. `MatchReport` remains the authoritative output for completed matches.

Future integration path:

```text
PlayMatchCommandHandler
  -> default Coordinate mode
  -> MatchEngineInputBuilder::build
  -> MatchEngine::simulate
  -> validate result.report
  -> MatchReport or lightweight fallback
  -> League::applyMatchReport
```

## Deterministic Smoke Coverage

`fm_match_engine_smoke` covers the current simulator boundary without requiring a full realistic score. It verifies deterministic goals/report goals for repeated input and seed, report presence for valid input, report score/id/metadata consistency, starter player reports and ratings, regulation-time simulation, official goal events in `BackgroundSummary`, background no-trace behavior, background calibration ranges, assisted goal samples, no open-play goalkeeper scorers in fast summary, tactical sensitivity for mentality/tempo/directness, quality sensitivity for strong-vs-weak teams, coordinate `ShotQualityModel` ordering/clamps, decision-layer evaluator wiring, `BallCarrierDecisionModel` option combination, detailed coordinate rating/foul-corner placeholder sanity, basic team report stats, team-stat/player-rating persistence, safe default output for invalid input, no-crash execution for `BackgroundSummary`, `WatchedMatch`, and `DebugFullTrace`, default handler mode/detail selection, handler report application through `League`, fixture/standings updates, `MatchPlayedEvent` publication, explicit watched coordinate detail, and explicit lightweight mode.

This coverage is intentionally small and regression-oriented. It does not assert detailed trace contents or exact scorelines because the coordinate simulator remains under active tuning.

## 1. Vision

The project goal is not a simple score formula. The future match engine should be a realistic but lightweight football simulation where players, the ball, tactics, and local contests create the match outcome.

The long-term user experience should allow a watched match to be viewed through a sped-up mini-pitch. Player markers and ball movement in that view should reflect actual simulation state, not fake UI animation layered on top of unrelated results.

The engine remains core/backend-owned. QML should render output produced by the backend, such as trace frames, match events, reports, and summary state. QML must not own authoritative match state or invent simulation outcomes.

## 2. Core Simulation Style

The engine is not minute-by-minute. It runs possession-by-possession, with action/contest steps inside each possession and a second-level match clock.

Each possession contains repeated simulation steps. A step conceptually updates:

- player intents
- player movement
- ball state
- possible action decisions
- local contests
- possession or phase transitions

Watched matches may store richer trace output so marker movement and ball travel are smooth enough for a sped-up mini-pitch stream. Background matches avoid full marker traces to protect multi-league performance.

The same simulation logic applies to background, watched, and debug modes. The difference is trace verbosity, not a separate result algorithm.

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

These fields are persisted with selected `TeamSheet` runtime state and now feed the coordinate summary/detail models. The current UI still only exposes mentality and tempo. Future tactical UI can expose More Options without changing the principle that core owns match decisions and QML only edits user-facing setup.

`DefensiveLine` is intentionally a positional shape adjustment, not pressing behavior. In `OutOfPossession` it raises or drops the defensive block, in `DefensiveTransition` it shifts the recovery line, in `InPossession` it shifts rest-defense/back-line support, and in `AttackingTransition` it controls how far the back line follows the attack.

`PressingIntensity` is intentionally not movement yet. It will later influence local intents such as `PressBallCarrier`, `ContainBallCarrier`, and defensive recovery urgency.

`MarkingStyle` is intentionally not target selection yet. It will later influence whether defenders protect zones, combine zone and runner tracking, or follow opponents more tightly.

`PassingDirectness` now influences pass option scoring through `PassOptionEvaluator`: short passing boosts safe support and back-pass circulation, balanced remains neutral, and direct passing raises progressive, through-ball, and switch-play desire while accepting more risk. It is still a first-pass decision weight, not a full tactical possession model.

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

The current `BallCarrierDecisionModel` and `ActionSelector` implement this as a small future-expandable model. Candidate scoring is broad and deterministic, and selected actions are executed only inside the coordinate simulator.

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

The current `InterceptionResolver` skeleton performs this path sampling and ETA comparison. It returns candidates and a best candidate for contest resolution, but it does not decide whether a pass, cross, shot, or clearance is successfully intercepted. `CoordinateMatchSimulator` now filters those local candidates against the trajectory height before creating a contest, so an avoidably unreachable high ball is not treated exactly like a ground pass. `ContestResolver` consumes local participants and optional interception candidate data to decide the local contest outcome.

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

`GoalkeeperSave` can now represent save-and-hold, save-and-parry, loose rebound, or shot-beats-keeper outcomes. Inside the local simulator only, an on-target shot can increment coordinate goals, record a recent-pass assist when applicable, append an official `Goal` event in `MatchEngineResult`, and optionally add watched/debug trace. It still does not apply reports, fixtures, standings, history, or runtime match completion. Deflected and loose-ball recovery remains simplified.

`MatchEngineReportAdapter` is the conversion layer from coordinate output into `MatchReport`. It is Qt-free, deterministic, and pure: it consumes only `MatchEngineInput`, `MatchEngineResult`, copied snapshots, and copied ids. It fills match metadata, season year and matchweek from the input metadata, score from team stats, lineup snapshots from input team sheets, player report basics from player stats plus missing starters, basic team stats, and official events from `MatchEngineResult::events`. Trace frames are presentation/debug data, not the authoritative source for report events.

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
- full `CoordinateMatchSimulator` action-duration loop

`BackgroundSummary`:

- fast aggregate coordinate summary
- no full marker trace
- only result, report, stats, and key events
- avoids multi-league performance problems
- uses detailed attributes, formation roles, and tactical setup without full movement/trajectory cost

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
