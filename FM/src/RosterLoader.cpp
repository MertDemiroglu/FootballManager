#include"RosterLoader.h"

#include<fstream>
#include<sstream>
#include<stdexcept>
#include<string>
#include<unordered_map>
#include<memory>

#include"League.h"
#include"fm/roster/Team.h"
#include"fm/roster/FootballerFactory.h"
#include"fm/common/Types.h"

static std::string trim(const std::string& s) {
    const auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    const auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static bool isCommentOrEmpty(const std::string& line) {
    for (char c : line) {
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;
        return c == '#';
    }
    return true;
}

static int toIntOrThrow(const std::string& s, const char* fieldName) {
    try {
        size_t idx = 0;
        int v = std::stoi(s, &idx);
        if (idx != s.size()) throw std::runtime_error("not pure int");
        return v;
    }
    catch (...) {
        throw std::runtime_error(std::string("Invalid int for field: ") + fieldName + " value=" + s);
    }
}

static std::vector<std::string> splitCsvSimple(const std::string& line) {
    // Senin örneklerde týrnak yok, virgül ayýrýcý basit split yeter
    std::vector<std::string> out;
    std::stringstream ss(line);
    std::string item;
    while (std::getline(ss, item, ',')) {
        out.push_back(trim(item));
    }
    return out;
}

void RosterLoader::loadFromFile(League& league, const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("RosterLoader: Cannot open file: " + filePath);
    }

    // Team objelerini önce map’te biriktirelim
    std::unordered_map<std::string, std::unique_ptr<Team>> teamsByName;

    std::string line;
    int lineNo = 0;

    while (std::getline(file, line)) {
        lineNo++;

        if (trim(line).empty()) continue;
        if (isCommentOrEmpty(line)) continue;

        auto parts = splitCsvSimple(line);

        // Beklenen alan sayýsý: 9
        // Name, Age, Position, Team, s1..s5
        if (parts.size() != 9) {
            throw std::runtime_error("RosterLoader: Bad column count at line " + std::to_string(lineNo)
                + " (expected 9, got " + std::to_string(parts.size()) + ")");
        }

        const std::string name = parts[0];
        const int age = toIntOrThrow(parts[1], "Age");
        const std::string position = parts[2];
        const std::string teamName = parts[3];

        const int s1 = toIntOrThrow(parts[4], "s1");
        const int s2 = toIntOrThrow(parts[5], "s2");
        const int s3 = toIntOrThrow(parts[6], "s3");
        const int s4 = toIntOrThrow(parts[7], "s4");
        const int s5 = toIntOrThrow(parts[8], "s5");

        // Team yoksa oluþtur
        Team* teamPtr = nullptr;
        auto it = teamsByName.find(teamName);
        if (it == teamsByName.end()) {
            auto t = std::make_unique<Team>(teamName);
            teamPtr = t.get();
            teamsByName.emplace(teamName, std::move(t));
        }
        else {
            teamPtr = it->second.get();
        }

        // Player uret
        auto player = FootballerFactory::create(name, age, position, teamName, s1, s2, s3, s4, s5);
        if (!player) {
            throw std::runtime_error("RosterLoader: Unknown position '" + position
                + "' at line " + std::to_string(lineNo));
        }

        // Butun oyunculara default kontrat atamasi (engine invarianti)
        player->signContract(Money(1000), 3);

        // Team’e ekle (Team API'si sende nasýl bilmiyorum)
        // Aþaðýdakilerden biri sende vardýr:
        // teamPtr->addPlayer(std::move(player));
        // teamPtr->addFootballer(std::move(player));
        // teamPtr->add(std::move(player));
        teamPtr->addPlayer(std::move(player));
    }

    // League'e aktar
    // League API'si sende nasýl bilmiyorum, ama muhtemelen:
    // league.addTeam(std::move(teamUniquePtr));
    // veya league.addTeam(name, std::move(teamUniquePtr))
    for (auto& [name, teamUptr] : teamsByName) {
        league.addTeam(std::move(teamUptr));
    }
}