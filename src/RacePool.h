// Written by tools/pool.py - change the table there and run it again.
#ifndef MOD_CUSTOM_RACES_POOL_H
#define MOD_CUSTOM_RACES_POOL_H

namespace CustomRacesPool
{
    /// A race of the module: settings key, its own id (0: it takes a free number, CustomRaces.<key>.Id), its
    /// row in ChrRaces.dbc of the client, name,
    /// male and female display of that row, its client prefix and file name, its default base race, male only.
    struct RaceDef
    {
        char const* Key;
        unsigned char Id;
        unsigned char Source;
        char const* Name;
        unsigned int MaleDisplay;
        unsigned int FemaleDisplay;
        char const* Prefix;
        char const* File;
        unsigned char Base;
        bool MaleOnly;
    };

    inline constexpr RaceDef POOL[] =
    {
        { "Goblin", 9, 9, "Goblin", 6894, 6895, "Go", "Goblin", 2, false },
        { "Zandalari", 12, 12, "Zandalari Troll", 141225, 141226, "Za", "Zandalari Troll", 8, false },
        { "Worgen", 13, 13, "Worgen", 142817, 142818, "Wo", "Worgen", 1, false },
        { "KulTiran", 16, 16, "Kul Tiran", 141685, 141686, "Kt", "KulTiran", 1, false },
        { "Vulpera", 19, 19, "Vulpera", 141254, 141255, "Vu", "Vulpera", 6, false },
        { "Pandaren", 20, 20, "Pandaren", 141687, 141688, "Pa", "Pandaren", 1, false },
        { "HighElf", 0, 10, "High Elf", 15476, 15475, "Be", "BloodElf", 1, false },
        { "FelOrc", 0, 34, "Fel Orc", 16981, 16980, "Fo", "FelOrc", 2, true },
        { "Broken", 0, 36, "Broken", 17576, 17577, "Br", "Broken", 11, true },
        { "Earthen", 0, 62, "Earthen", 255050, 255052, "Dw", "Npc_Earthen", 3, false },
        { "Drakkari", 0, 44, "Drakkari Troll", 142663, 142664, "Dk", "DrakkariTroll", 8, false },
        { "Vrykul", 0, 38, "Vrykul", 21685, 21686, "Vr", "Vrykul", 1, true },
        { "Tuskarr", 0, 39, "Tuskarr", 21780, 21781, "Tu", "Tuskarr", 3, true },
        { "Taunka", 0, 41, "Taunka", 26316, 26317, "Wt", "Taunka", 6, true },
        { "ForestTroll", 0, 40, "Forest Troll", 21963, 21964, "Ft", "ForestTroll", 8, true },
        { "IceTroll", 0, 43, "Ice Troll", 26873, 26874, "It", "IceTroll", 8, true },
        { "Skeleton", 0, 37, "Skeleton", 17578, 17579, "Sk", "Skeleton", 5, true },
        { "Naga", 0, 35, "Naga", 17402, 17403, "Na", "Naga_", 5, false },
        { "NorthrendSkeleton", 0, 42, "Northrend Skeleton", 26871, 26872, "NS", "NorthrendSkeleton", 5, true },
        { "LeperTroll", 0, 45, "Leper Troll", 142819, 142820, "Dk", "LeperTroll", 8, false },
        { "Troglodyte", 0, 46, "Troglodyte", 142832, 142833, "Dk", "Troglodyte", 3, true },
        { "ThinHuman", 0, 33, "Thin Human", 141109, 141109, "Th", "ThinHuman", 1, true },
        { "HumanCultist", 0, 63, "Human Cultist", 254500, 254501, "Hu", "Npc_HumanCultist", 1, false },
    };

    /// The free race ids of the client a race without one of its own can take.
    inline constexpr unsigned char SLOTS[] = { 14, 15, 17, 18, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31 };
}

#endif
