/*
 * mod-custom-races - more playable races for Conquest of Azeroth.
 * Released under GNU AGPL v3: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 *
 * The game client of Conquest of Azeroth already carries the models, skins and hair of races no player can choose.
 * This module makes some of them playable; its client patch (built by AFK Realm from the player's own client) puts
 * them on the race choice. Each new race leans on a stock race of its faction, its base race: it starts where that
 * race starts, with its outfits, stats and racial abilities, and may do whatever that race may do (quests, items,
 * reputations, skills).
 *
 * Everything here can be taken back. At every start the module writes its rows for the races that are on and
 * removes those of the races that are off; none of these races has rows of its own in the world database. What a
 * base race may do is lent to a new race in memory only, never written. A character of a race that is switched off
 * becomes its base race - with its old race kept aside, so that it gets it back when the race is switched on again.
 */

#include "AscensionRacialAbilities.h"
#include "Config.h"
#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "QuestDef.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
#include "StringConvert.h"
#include "Tokenize.h"

#include <algorithm>
#include <cctype>
#include <set>
#include <string>
#include <vector>

// In DBCStores.cpp, added by patches/core-custom-races.patch: a new race starts in the outfits of its base race and
// may have the skills that race may have. Declared here rather than in DBCStores.h, which nearly every file of the
// core includes - a header the patch changed would make every build of the server compile almost all of it again.
void SetRaceBase(uint8 race, uint8 base);

namespace
{
    /// What a new race takes over from the stock race it leans on: the values of that race's row in ChrRaces.dbc.
    struct BaseRace
    {
        uint8 Id;
        uint32 FactionTemplate;
        uint32 Language;
        uint32 Alliance;                                 // 0 Alliance, 1 Horde
        uint32 ExplorationSound;
    };

    BaseRace const BASES[] =
    {
        { 1, 1, 7, 0, 4140 },       // Human
        { 2, 2, 1, 1, 4141 },       // Orc
        { 3, 3, 7, 0, 4147 },       // Dwarf
        { 4, 4, 7, 0, 4145 },       // Night Elf
        { 5, 5, 1, 1, 4142 },       // Undead
        { 6, 6, 1, 1, 4143 },       // Tauren
        { 7, 115, 7, 0, 4146 },     // Gnome
        { 8, 116, 1, 1, 4144 },     // Troll
        { 10, 1610, 1, 1, 4142 },   // Blood Elf
        { 11, 1629, 7, 0, 4140 },   // Draenei
    };

    /// A race of the client that the module can make playable. Its id is the one of its row in ChrRaces.dbc of the
    /// Conquest of Azeroth client; races are numbered up to 31 (they are bits of a 32-bit race mask).
    struct RaceDef
    {
        char const* Key;
        uint8 Id;
        char const* Name;
        uint32 MaleDisplay;
        uint32 FemaleDisplay;
        uint8 Base;
    };

    RaceDef const POOL[] =
    {
        { "Goblin", 9, "Goblin", 6894, 6895, 2 },
    };

    constexpr uint32 PLAYABLE_FLAGS = 0xC;
    // The values every playable race of the client has: humanoid, Resurrection Sickness, the splash sound.
    constexpr uint32 CREATURE_TYPE_HUMANOID_ID = 7;
    constexpr uint32 RESURRECTION_SICKNESS = 15007;
    constexpr uint32 SPLASH_SOUND = 1096;

    constexpr uint32 Bit(uint8 race) { return 1u << (race - 1); }

    BaseRace const* FindBase(uint8 id)
    {
        for (BaseRace const& base : BASES)
            if (base.Id == id)
                return &base;
        return nullptr;
    }

    std::string Lower(std::string text)
    {
        std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
        return text;
    }

    /// The races CustomRaces.Races names, while CustomRaces.Enable is on.
    std::vector<RaceDef const*> Wanted()
    {
        std::vector<RaceDef const*> races;
        if (!sConfigMgr->GetOption<bool>("CustomRaces.Enable", true))
            return races;
        std::string list = sConfigMgr->GetOption<std::string>("CustomRaces.Races", "Goblin");
        std::replace(list.begin(), list.end(), ',', ' ');
        for (std::string_view token : Acore::Tokenize(list, ' ', false))
        {
            std::string const name = Lower(std::string(token));
            RaceDef const* found = nullptr;
            for (RaceDef const& race : POOL)
                if (Lower(race.Key) == name)
                    found = &race;
            if (!found)
                LOG_WARN("module", "mod-custom-races: CustomRaces.Races names \"{}\", which is not one of its races; left out", token);
            else if (std::find(races.begin(), races.end(), found) == races.end())
                races.push_back(found);
        }
        return races;
    }

    std::vector<RaceDef const*> active;

    bool IsWanted(std::vector<RaceDef const*> const& wanted, RaceDef const& race)
    {
        return std::find(wanted.begin(), wanted.end(), &race) != wanted.end();
    }

    // ---------------------------------------------------------------------------------- the world database

    /// The rows of one race: removed always, written again while the race is on.
    void WriteRace(RaceDef const& race, bool on)
    {
        uint32 const bit = Bit(race.Id);
        WorldDatabase.DirectExecute("DELETE FROM `chrraces_dbc` WHERE `ID` = {}", race.Id);
        WorldDatabase.DirectExecute("DELETE FROM `playercreateinfo` WHERE `race` = {}", race.Id);
        WorldDatabase.DirectExecute("DELETE FROM `playercreateinfo_action` WHERE `race` = {}", race.Id);
        WorldDatabase.DirectExecute("DELETE FROM `playercreateinfo_item` WHERE `race` = {}", race.Id);
        WorldDatabase.DirectExecute("DELETE FROM `player_race_stats` WHERE `Race` = {}", race.Id);
        WorldDatabase.DirectExecute("DELETE FROM `playercreateinfo_skills` WHERE `raceMask` = {}", bit);
        WorldDatabase.DirectExecute("DELETE FROM `playercreateinfo_spell_custom` WHERE `racemask` = {}", bit);
        WorldDatabase.DirectExecute("DELETE FROM `ascension_custom_class_race` WHERE `race` = {}", race.Id);
        if (!on)
            return;

        BaseRace const* base = FindBase(race.Base);
        uint32 const baseBit = Bit(race.Base);
        std::string names;
        std::string values;
        for (char const* locale : { "enUS", "enGB", "koKR", "frFR", "deDE", "enCN", "zhCN", "enTW", "zhTW", "esES", "esMX", "ruRU",
                                    "ptPT", "ptBR", "itIT", "Unk" })
        {
            names += std::string(", `Name_Lang_") + locale + "`";
            values += std::string(", '") + race.Name + "'";
        }
        // A row of the world database overrides the row of the client table with the same id; its empty texts keep
        // those of the client table.
        WorldDatabase.DirectExecute("INSERT INTO `chrraces_dbc` (`ID`, `Flags`, `FactionID`, `ExplorationSoundID`, `MaleDisplayId`, "
            "`FemaleDisplayId`, `BaseLanguage`, `CreatureType`, `ResSicknessSpellID`, `SplashSoundID`, `CinematicSequenceID`, "
            "`Alliance`{}, `Name_Lang_Mask`, `Required_Expansion`) VALUES ({}, {}, {}, {}, {}, {}, {}, {}, {}, {}, 0, {}{}, 0, 0)",
            names, race.Id, PLAYABLE_FLAGS, base->FactionTemplate, base->ExplorationSound, race.MaleDisplay, race.FemaleDisplay,
            base->Language, CREATURE_TYPE_HUMANOID_ID, RESURRECTION_SICKNESS, SPLASH_SOUND, base->Alliance, values);
        WorldDatabase.DirectExecute("INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, "
            "`position_z`, `orientation`) SELECT {}, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation` "
            "FROM `playercreateinfo` WHERE `race` = {}", race.Id, race.Base);
        WorldDatabase.DirectExecute("INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) "
            "SELECT {}, `class`, `button`, `action`, `type` FROM `playercreateinfo_action` WHERE `race` = {}", race.Id, race.Base);
        WorldDatabase.DirectExecute("INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`, `Note`) "
            "SELECT {}, `class`, `itemid`, `amount`, `Note` FROM `playercreateinfo_item` WHERE `race` = {}", race.Id, race.Base);
        WorldDatabase.DirectExecute("INSERT INTO `player_race_stats` (`Race`, `Strength`, `Agility`, `Stamina`, `Intellect`, `Spirit`) "
            "SELECT {}, `Strength`, `Agility`, `Stamina`, `Intellect`, `Spirit` FROM `player_race_stats` WHERE `Race` = {}",
            race.Id, race.Base);
        WorldDatabase.DirectExecute("INSERT IGNORE INTO `playercreateinfo_skills` (`raceMask`, `classMask`, `skill`, `rank`, `comment`) "
            "SELECT {}, `classMask`, `skill`, `rank`, `comment` FROM `playercreateinfo_skills` WHERE `raceMask` <> 0 AND "
            "(`raceMask` & {}) <> 0", bit, baseBit);
        WorldDatabase.DirectExecute("INSERT IGNORE INTO `playercreateinfo_spell_custom` (`racemask`, `classmask`, `Spell`, `Note`) "
            "SELECT {}, `classmask`, `Spell`, `Note` FROM `playercreateinfo_spell_custom` WHERE `racemask` <> 0 AND "
            "(`racemask` & {}) <> 0", bit, baseBit);
        WorldDatabase.DirectExecute("INSERT IGNORE INTO `ascension_custom_class_race` (`class`, `race`) "
            "SELECT `class`, {} FROM `ascension_custom_class_race` WHERE `race` = {}", race.Id, race.Base);
    }

    // ---------------------------------------------------------------------------------- the characters

    /// A character of a race that is off becomes its base race; one whose race is on again gets it back.
    void MoveCharacters(RaceDef const& race, bool on)
    {
        CharacterDatabase.DirectExecute("CREATE TABLE IF NOT EXISTS `custom_races_moved` ("
            "`guid` INT UNSIGNED NOT NULL, `race` TINYINT UNSIGNED NOT NULL, `base` TINYINT UNSIGNED NOT NULL, "
            "`skin` TINYINT UNSIGNED NOT NULL, `face` TINYINT UNSIGNED NOT NULL, `hairStyle` TINYINT UNSIGNED NOT NULL, "
            "`hairColor` TINYINT UNSIGNED NOT NULL, `facialStyle` TINYINT UNSIGNED NOT NULL, PRIMARY KEY (`guid`)) "
            "ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod-custom-races: characters of a race switched off, and their looks'");

        if (on)
        {
            QueryResult back = CharacterDatabase.Query("SELECT COUNT(*) FROM `custom_races_moved` m JOIN `characters` c ON c.`guid` = m.`guid` "
                "WHERE m.`race` = {} AND c.`race` = m.`base`", race.Id);
            CharacterDatabase.DirectExecute("UPDATE `characters` c JOIN `custom_races_moved` m ON c.`guid` = m.`guid` SET c.`race` = m.`race`, "
                "c.`skin` = m.`skin`, c.`face` = m.`face`, c.`hairStyle` = m.`hairStyle`, c.`hairColor` = m.`hairColor`, "
                "c.`facialStyle` = m.`facialStyle` WHERE m.`race` = {} AND c.`race` = m.`base`", race.Id);
            CharacterDatabase.DirectExecute("DELETE FROM `custom_races_moved` WHERE `race` = {}", race.Id);
            if (back && (*back)[0].Get<uint64>())
                LOG_INFO("module", "mod-custom-races: {} character(s) are {} again", (*back)[0].Get<uint64>(), race.Name);
            return;
        }

        QueryResult count = CharacterDatabase.Query("SELECT COUNT(*) FROM `characters` WHERE `race` = {}", race.Id);
        if (!count || !(*count)[0].Get<uint64>())
            return;
        CharacterDatabase.DirectExecute("REPLACE INTO `custom_races_moved` (`guid`, `race`, `base`, `skin`, `face`, `hairStyle`, `hairColor`, "
            "`facialStyle`) SELECT `guid`, `race`, {}, `skin`, `face`, `hairStyle`, `hairColor`, `facialStyle` FROM `characters` "
            "WHERE `race` = {}", race.Base, race.Id);
        CharacterDatabase.DirectExecute("UPDATE `characters` SET `race` = {}, `skin` = 0, `face` = 0, `hairStyle` = 0, `hairColor` = 0, "
            "`facialStyle` = 0 WHERE `race` = {}", race.Base, race.Id);
        LOG_INFO("module", "mod-custom-races: {} is off; {} character(s) of it play their base race until it is on again",
            race.Name, (*count)[0].Get<uint64>());
    }

    // ---------------------------------------------------------------------------------- what the base race lends

    /// A race mask that admits the base race admits the new race too: faction masks and masks of the base race alone.
    bool Lend(uint32& mask, uint32 baseBit, uint32 bit)
    {
        if (!mask || mask == 0xFFFFFFFF || !(mask & baseBit) || (mask & bit))
            return false;
        mask |= bit;
        return true;
    }

    /// Quest::AllowableRaces is protected; a derived class may name it.
    struct QuestRaces : Quest
    {
        static uint32& Of(Quest* quest) { return quest->*(&QuestRaces::AllowableRaces); }
    };

    void LendMasks(RaceDef const& race)
    {
        uint32 const bit = Bit(race.Id);
        uint32 const baseBit = Bit(race.Base);
        uint32 quests = 0, items = 0, factions = 0, skills = 0;

        for (auto const& [id, quest] : sObjectMgr->GetQuestTemplates())
            if (quest && Lend(QuestRaces::Of(quest), baseBit, bit))
                ++quests;
        for (ItemTemplate* item : *sObjectMgr->GetItemTemplateStoreFast())
            if (item && Lend(item->AllowableRace, baseBit, bit))
                ++items;
        for (FactionEntry const* faction : sFactionStore)
            if (faction)
                for (uint32& mask : const_cast<FactionEntry*>(faction)->BaseRepRaceMask)
                    if (Lend(mask, baseBit, bit))
                        ++factions;
        for (SkillLineAbilityEntry const* ability : sSkillLineAbilityStore)
            if (ability && Lend(const_cast<SkillLineAbilityEntry*>(ability)->RaceMask, baseBit, bit))
                ++skills;

        LOG_INFO("module", "mod-custom-races: {} plays as its base race does: {} quest(s), {} item(s), {} reputation(s), {} skill spell(s)",
            race.Name, quests, items, factions, skills);
    }
}

class CustomRacesWorld : public WorldScript
{
public:
    CustomRacesWorld() : WorldScript("CustomRacesWorld", { WORLDHOOK_ON_LOAD_CUSTOM_DATABASE_TABLE, WORLDHOOK_ON_BEFORE_WORLD_INITIALIZED }) { }

    /// Just before the core reads the client tables and their rows in the world database.
    void OnLoadCustomDatabaseTable() override
    {
        std::vector<RaceDef const*> const wanted = Wanted();
        active.clear();
        for (RaceDef const& race : POOL)
        {
            bool const on = IsWanted(wanted, race) && FindBase(race.Base);
            WriteRace(race, on);
            MoveCharacters(race, on);
            if (on)
                active.push_back(&race);
        }

        std::string names;
        for (RaceDef const* race : active)
            names += std::string(names.empty() ? "" : ", ") + race->Name;
        LOG_INFO("module", "mod-custom-races: playable races added: {}", names.empty() ? "none" : names);
    }

    /// The core has read everything; the new races lean on their base races from here on.
    void OnBeforeWorldInitialized() override
    {
        for (uint8 race = 0; race < AscensionRacialAbilities::BorrowedRacials.size(); ++race)
        {
            AscensionRacialAbilities::BorrowedRacials[race] = 0;
            SetRaceBase(race, 0);
        }
        for (RaceDef const* race : active)
        {
            AscensionRacialAbilities::BorrowedRacials[race->Id] = race->Base;
            SetRaceBase(race->Id, race->Base);
            LendMasks(*race);
        }
    }
};

void AddSC_custom_races()
{
    new CustomRacesWorld();
}
