#pragma once

enum class ContestType {
    PassingLaneInterception,
    GroundCrossInterception,
    ReceptionDuel,
    DribbleDuel,
    TackleAttempt,
    ShotBlock,
    GoalkeeperSave,
    AerialDuel,
    LooseBallRace
};

enum class ContestSide {
    None,
    Attacking,
    Defending,
    Neutral
};

enum class ContestResolutionType {
    NoContest,
    AttackerWon,
    DefenderWon,
    KeeperSaved,
    ShotBeatsKeeper,
    BallDeflected,
    BallLoose,
    OutOfPlay
};

enum class ContestBallOutcome {
    None,
    AttackerKeepsControl,
    DefenderControls,
    KeeperControls,
    BallDeflected,
    BallLoose,
    ShotContinues,
    OutOfPlay
};
