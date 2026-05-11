#pragma once

#include"fm/core/GameBootstrapOptions.h"

#include<QString>

namespace BootstrapPaths {
    QString schemaAssetPath();
    QString seedAssetPath();
    GameBootstrapOptions createGameBootstrapOptions();
}
