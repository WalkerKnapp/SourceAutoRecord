#pragma once
#include <vector>

#include "TimerRule.hpp"

#include "Game.hpp"

using _Rules = std::vector<TimerRule*>;

class TimerCategory {
public:
    int gameVersion;
    const char* name;
    std::vector<TimerRule*> rules;

public:
    static std::vector<TimerCategory*>& GetList();

public:
    TimerCategory(int gameVersion, const char* name, std::vector<TimerRule*> rules);
    ~TimerCategory();

    static int FilterByGame(Game* game);
    static void ResetAll();
};

#define SAR_CATEGORY(gameVersion, name, rules) \
    TimerCategory gameVersion##_##name(SourceGame_##gameVersion, #name, rules)
