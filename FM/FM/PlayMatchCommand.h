#pragma once

#include"Date.h"
#include"Types.h"

struct PlayMatchCommand {
    int seasonYear = 0;
    Date date;
    TeamId homeId = 0;
    TeamId awayId = 0;
    int matchweek = 0;
};