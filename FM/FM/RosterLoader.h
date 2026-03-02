#pragma once
#include <string>

class League;

class RosterLoader {
public:
    // CSV benzeri dosyadan (senin formatýn) takýmlarý + oyuncularý yükler
    // Format: Name,Age,Position,Team,S1,S2,S3,S4,S5
    static void loadFromFile(League& league, const std::string& filePath);
};