/*
 * mod-custom-races - more playable races for Conquest of Azeroth.
 * Released under GNU AGPL v3: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 *
 * The game client of Conquest of Azeroth already carries the models, skins and hair of races no player can choose.
 * This module makes them playable; its client patch (built by AFK Realm from the player's own client) puts them on
 * the race choice. Which races there are is not fixed here: they are the races of the game client, which the setup
 * page of AFK Realm reads and writes into mod_custom_races.conf (CustomRaces.<race>.*). Each new race leans on a
 * stock race, its base race: it starts where that race starts, with its outfits, stats and racial abilities, and may
 * do whatever that race may do (quests, items, reputations, skills).
 *
 * Everything here can be taken back. At every start the module writes its rows for the races that are on and
 * removes those of the races that are off; none of these races has rows of its own in the world database. What a
 * base race may do is lent to a new race in memory only, never written. A character of a race that is switched off
 * becomes its base race - with its old race kept aside, so that it gets it back when the race is switched on again.
 *
 * A race may also bring racial abilities of its own (CustomRaces.<race>.Racials). The rows of those spells are not
 * written by the module: AFK Realm reads them from the server data folder of the server pack and writes them into a
 * small file per race (afk-realm\races\<race>.sql). At every start the module runs the file of each race that is on,
 * and takes the rows back out when the race or the server data goes off. What it gave out is kept in
 * `custom_races_racials`, so a run can take back what an earlier one wrote, whatever the settings are by then.
 */

#include "AscensionRacialAbilities.h"
#include "Cell.h"
#include "CellImpl.h"
#include "Config.h"
#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Log.h"
#include "MotionMaster.h"
#include "Player.h"
#include "ObjectMgr.h"
#include "QuestDef.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "StringConvert.h"
#include "Tokenize.h"
#include "WorldSession.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <list>
#include <map>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

// In DBCStores.cpp, added by patches/core-custom-races.patch: a new race starts in the outfits of its base race and
// may have the skills that race may have. Declared here rather than in DBCStores.h, which nearly every file of the
// core includes - a header the patch changed would make every build of the server compile almost all of it again.
void SetRaceBase(uint8 race, uint8 base);
// The looks a race keeps beyond the five bytes (0 none, 1 a sixth byte, 2 a 64-bit number), for EsteriaAppearance.dll.
void SetExtraAppearanceKind(uint8 race, uint8 kind);

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
        char const* Name;
    };

    BaseRace const BASES[] =
    {
        { 1, 1, 7, 0, 4140, "Human" },
        { 2, 2, 1, 1, 4141, "Orc" },
        { 3, 3, 7, 0, 4147, "Dwarf" },
        { 4, 4, 7, 0, 4145, "NightElf" },
        { 5, 5, 1, 1, 4142, "Undead" },
        { 6, 6, 1, 1, 4143, "Tauren" },
        { 7, 115, 7, 0, 4146, "Gnome" },
        { 8, 116, 1, 1, 4144, "Troll" },
        { 10, 1610, 1, 1, 4142, "BloodElf" },
        { 11, 1629, 7, 0, 4140, "Draenei" },
    };

    /// A race of this realm, as the game client has it: what the setup page of AFK Realm read from the client and
    /// wrote into mod_custom_races.conf (CustomRaces.<race>.*).
    struct RaceDef
    {
        std::string Key;
        uint8 Id = 0;                                    // its number on this realm (0: none - never on)
        uint32 Source = 0;                               // its row in ChrRaces.dbc of the client
        std::string Name, Prefix, File;
        uint32 MaleDisplay = 0, FemaleDisplay = 0;
        uint8 Base = 1;
        bool MaleOnly = false;
        bool FemaleOnly = false;                         // CustomRaces.<race>.FemaleOnly: no man of its own (Banshee)
        bool OwnHelmets = true;                          // the client has helmet models for it
        uint8 Appearance = 0;                            // CustomRaces.<race>.Appearance: looks beyond the five bytes
        std::vector<uint32> RacialSpells;                // CustomRaces.<race>.Racials: what its characters learn
    };

    constexpr uint32 PLAYABLE_FLAGS = 0xC;
    // The values every playable race of the client has: humanoid, Resurrection Sickness, the splash sound.
    constexpr uint32 CREATURE_TYPE_HUMANOID_ID = 7;
    constexpr uint32 RESURRECTION_SICKNESS = 15007;
    constexpr uint32 SPLASH_SOUND = 1096;
    constexpr char const* PREFIX = "CustomRaces.";

    /// The bit of a race in race masks; races above 32 have none (with the core patch, they use the bit of their base race).
    constexpr uint32 Bit(uint8 race) { return race >= 1 && race <= 32 ? 1u << (race - 1) : 0; }

    BaseRace const* FindBase(uint8 id)
    {
        for (BaseRace const& base : BASES)
            if (base.Id == id)
                return &base;
        return nullptr;
    }

    /// The highest race number: 63, or 127 for the game of CoA Custom (CustomRaces.HighRaces) - its own exe knows the
    /// race numbers it gives its races, up to 103.
    uint32 MaxRace() { return sConfigMgr->GetOption<bool>("CustomRaces.HighRaces", false) ? 127 : 63; }

    /// A race number the module may use: 1 to MaxRace(), none of the stock races. Numbers above 31 need the 64-race
    /// patch of the game client (CustomRaces.MoreRaces) or CoA Custom's game; without them the client cannot show them.
    bool Usable(uint32 id) { return id >= 1 && id <= MaxRace() && !FindBase(uint8(id)); }

    std::string Lower(std::string text)
    {
        std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
        return text;
    }

    std::string Unquote(std::string text)
    {
        if (text.size() >= 2 && text.front() == '"' && text.back() == '"')
            text = text.substr(1, text.size() - 2);
        return text;
    }

    std::string Text(std::string const& key, char const* name, std::string const& fallback = "")
    {
        return Unquote(sConfigMgr->GetOption<std::string>(PREFIX + key + "." + name, fallback, false));
    }

    uint32 Number(std::string const& key, char const* name, uint32 fallback = 0)
    {
        return sConfigMgr->GetOption<uint32>(PREFIX + key + "." + name, fallback, false);
    }

    /// For SQL text: no quote or backslash can end the string early.
    std::string Sql(std::string text)
    {
        text.erase(std::remove_if(text.begin(), text.end(), [](char c) { return c == '\'' || c == '\\'; }), text.end());
        return text;
    }

    /// A list of spell ids as the settings write it: "110600 110602" or "110600, 110602".
    std::vector<uint32> SpellIds(std::string const& text)
    {
        std::vector<uint32> spells;
        std::string list = text;
        std::replace(list.begin(), list.end(), ',', ' ');
        for (std::string_view token : Acore::Tokenize(list, ' ', false))
        {
            std::string const word(token);
            char* end = nullptr;
            unsigned long value = std::strtoul(word.c_str(), &end, 10);
            if (end && *end == '\0' && value > 0 && value < 0x80000000ul)
                spells.push_back(uint32(value));
        }
        return spells;
    }

    /// The stock race CustomRaces.<race>.Base names, or the Human.
    uint8 ParseBase(RaceDef const& race)
    {
        std::string const wanted = Lower(Text(race.Key, "Base"));
        for (BaseRace const& base : BASES)
            if (Lower(base.Name) == wanted || (wanted == "night elf" && base.Id == 4) || (wanted == "blood elf" && base.Id == 10) ||
                (wanted == "scourge" && base.Id == 5))
                return base.Id;
        LOG_WARN("module", "mod-custom-races: CustomRaces.{}.Base is \"{}\", which is not a stock race; the {} leans on the Human",
            race.Key, wanted, race.Name);
        return 1;
    }

    std::vector<RaceDef> races;                          // every race the settings describe
    std::vector<RaceDef const*> active;                  // the ones on

    /// Every race the settings describe (each with CustomRaces.<race>.Source), its number checked: one a new race can
    /// have, and no two races sharing one. A race left without a number stays off.
    void ReadRaces()
    {
        races.clear();
        std::set<uint8> taken;
        std::string const suffix = ".Source";
        std::string const prefix = PREFIX;
        for (std::string const& option : sConfigMgr->GetKeysByString(prefix))
        {
            if (option.size() <= prefix.size() + suffix.size() || option.compare(option.size() - suffix.size(), suffix.size(), suffix))
                continue;
            RaceDef race;
            race.Key = option.substr(prefix.size(), option.size() - prefix.size() - suffix.size());
            if (race.Key.empty() || race.Key.find('.') != std::string::npos)
                continue;
            race.Source = Number(race.Key, "Source");
            race.Name = Text(race.Key, "Name", race.Key);
            race.Prefix = Text(race.Key, "Prefix");
            race.File = Text(race.Key, "File", race.Key);
            race.MaleDisplay = Number(race.Key, "MaleDisplay");
            race.FemaleDisplay = Number(race.Key, "FemaleDisplay");
            race.FemaleOnly = Number(race.Key, "FemaleOnly") != 0;
            race.MaleOnly = !race.FemaleOnly && Number(race.Key, "MaleOnly") != 0;
            race.OwnHelmets = Number(race.Key, "OwnHelmets", 1) != 0;
            race.RacialSpells = SpellIds(Text(race.Key, "Racials"));
            race.Appearance = uint8(std::min<uint32>(Number(race.Key, "Appearance"), 2));
            race.Base = ParseBase(race);

            uint32 const id = Number(race.Key, "Id", race.Source < 32 ? race.Source : 0);
            if (id && !Usable(id))
                LOG_ERROR("module", "mod-custom-races: CustomRaces.{}.Id is {}, which no new race can have (1 to {}, not a stock race); "
                    "the {} stays off", race.Key, id, MaxRace(), race.Name);
            else if (id && taken.count(uint8(id)))
                LOG_ERROR("module", "mod-custom-races: CustomRaces.{}.Id is {}, which another race has; the {} stays off", race.Key, id, race.Name);
            else if (id)
            {
                race.Id = uint8(id);
                taken.insert(race.Id);
            }
            races.push_back(race);
        }
    }

    /// The races CustomRaces.Races names, while CustomRaces.Enable is on.
    std::vector<RaceDef const*> Wanted()
    {
        std::vector<RaceDef const*> wanted;
        if (!sConfigMgr->GetOption<bool>("CustomRaces.Enable", true))
            return wanted;
        std::string list = sConfigMgr->GetOption<std::string>("CustomRaces.Races", "");
        std::replace(list.begin(), list.end(), ',', ' ');
        for (std::string_view token : Acore::Tokenize(list, ' ', false))
        {
            std::string const name = Lower(std::string(token));
            RaceDef const* found = nullptr;
            for (RaceDef const& race : races)
                if (Lower(race.Key) == name)
                    found = &race;
            if (!found)
                LOG_WARN("module", "mod-custom-races: CustomRaces.Races names \"{}\", which has no CustomRaces.{}.Source; left out", token, token);
            else if (!found->Id)
                LOG_WARN("module", "mod-custom-races: {} is in CustomRaces.Races but has no race number (CustomRaces.{}.Id); it stays off",
                    found->Name, found->Key);
            else if (found->Id > 31 && !sConfigMgr->GetOption<bool>("CustomRaces.MoreRaces", false)
                && !sConfigMgr->GetOption<bool>("CustomRaces.HighRaces", false))
                LOG_WARN("module", "mod-custom-races: {} has race number {}, above 31, but CustomRaces.MoreRaces is off; it stays off",
                    found->Name, found->Id);
            else if (std::find(wanted.begin(), wanted.end(), found) == wanted.end())
                wanted.push_back(found);
        }
        return wanted;
    }

    // ---------------------------------------------------------------------------------- the world database

    void DeleteRows(uint8 id)
    {
        WorldDatabase.DirectExecute("DELETE FROM `chrraces_dbc` WHERE `ID` = {}", id);
        WorldDatabase.DirectExecute("DELETE FROM `playercreateinfo` WHERE `race` = {}", id);
        WorldDatabase.DirectExecute("DELETE FROM `playercreateinfo_action` WHERE `race` = {}", id);
        WorldDatabase.DirectExecute("DELETE FROM `playercreateinfo_item` WHERE `race` = {}", id);
        WorldDatabase.DirectExecute("DELETE FROM `player_race_stats` WHERE `Race` = {}", id);
        WorldDatabase.DirectExecute("DELETE FROM `player_totem_model` WHERE `RaceID` = {}", id);
        if (Bit(id))                                     // a mask of 0 is every race
        {
            WorldDatabase.DirectExecute("DELETE FROM `playercreateinfo_skills` WHERE `raceMask` = {}", Bit(id));
            WorldDatabase.DirectExecute("DELETE FROM `playercreateinfo_spell_custom` WHERE `racemask` = {}", Bit(id));
        }
        WorldDatabase.DirectExecute("DELETE FROM `ascension_custom_class_race` WHERE `race` = {}", id);
        WorldDatabase.DirectExecute("DELETE FROM `emotetextsound_dbc` WHERE `RaceId` = {}", id);
    }

    /// The rows of one race: removed always, written again while the race is on.
    void WriteRace(RaceDef const& race, bool on)
    {
        DeleteRows(race.Id);
        if (!on)
            return;

        BaseRace const* base = FindBase(race.Base);
        uint32 const bit = Bit(race.Id);
        uint32 const baseBit = Bit(race.Base);
        std::string names;
        std::string values;
        for (char const* locale : { "enUS", "enGB", "koKR", "frFR", "deDE", "enCN", "zhCN", "enTW", "zhTW", "esES", "esMX", "ruRU",
                                    "ptPT", "ptBR", "itIT", "Unk" })
        {
            names += std::string(", `Name_Lang_") + locale + "`";
            values += ", '" + Sql(race.Name) + "'";
        }
        // A row of the world database overrides the row of the client table with the same id; the server's own client
        // tables have no row for most of these numbers, so the whole row comes from here.
        WorldDatabase.DirectExecute("INSERT INTO `chrraces_dbc` (`ID`, `Flags`, `FactionID`, `ExplorationSoundID`, `MaleDisplayId`, "
            "`FemaleDisplayId`, `ClientPrefix`, `BaseLanguage`, `CreatureType`, `ResSicknessSpellID`, `SplashSoundID`, `ClientFilestring`, "
            "`CinematicSequenceID`, `Alliance`{}, `Name_Lang_Mask`, `Required_Expansion`) "
            "VALUES ({}, {}, {}, {}, {}, {}, '{}', {}, {}, {}, {}, '{}', 0, {}{}, 0, 0)",
            names, race.Id, PLAYABLE_FLAGS, base->FactionTemplate, base->ExplorationSound, race.MaleDisplay, race.FemaleDisplay,
            Sql(race.Prefix), base->Language, CREATURE_TYPE_HUMANOID_ID, RESURRECTION_SICKNESS, SPLASH_SOUND, Sql(race.File), base->Alliance, values);
        WorldDatabase.DirectExecute("INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, "
            "`position_z`, `orientation`) SELECT {}, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation` "
            "FROM `playercreateinfo` WHERE `race` = {}", race.Id, race.Base);
        WorldDatabase.DirectExecute("INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) "
            "SELECT {}, `class`, `button`, `action`, `type` FROM `playercreateinfo_action` WHERE `race` = {}", race.Id, race.Base);
        WorldDatabase.DirectExecute("INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`, `Note`) "
            "SELECT {}, `class`, `itemid`, `amount`, `Note` FROM `playercreateinfo_item` WHERE `race` = {}", race.Id, race.Base);
        WorldDatabase.DirectExecute("INSERT INTO `player_race_stats` (`Race`, `Strength`, `Agility`, `Stamina`, `Intellect`, `Spirit`) "
            "SELECT {}, `Strength`, `Agility`, `Stamina`, `Intellect`, `Spirit` FROM `player_race_stats` WHERE `Race` = {}", race.Id, race.Base);
        // The core reads the totem models of a shaman by exact race; a race of ours shows the totems of its base race.
        WorldDatabase.DirectExecute("INSERT INTO `player_totem_model` (`TotemID`, `RaceID`, `ModelID`) "
            "SELECT `TotemID`, {}, `ModelID` FROM `player_totem_model` WHERE `RaceID` = {}", race.Id, race.Base);
        // A race above 32 has no bit of its own: the core gives it the skills and spells of its base race's bit.
        if (bit)
            WorldDatabase.DirectExecute("INSERT IGNORE INTO `playercreateinfo_skills` (`raceMask`, `classMask`, `skill`, `rank`, `comment`) "
            "SELECT {}, `classMask`, `skill`, `rank`, `comment` FROM `playercreateinfo_skills` WHERE `raceMask` <> 0 AND "
            "(`raceMask` & {}) <> 0", bit, baseBit);
        if (bit)
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

    /// A number no race has any more: a race was given up (CustomRaces.<race>.Id set to 0) or left the settings. Its
    /// characters stay the base race they had for good, and its rows go.
    void FreeNumbers()
    {
        for (uint8 id = 1; id < 128; ++id)
        {
            if (!Usable(id))
                continue;
            bool used = false;
            for (RaceDef const& race : races)
                used |= race.Id == id;
            if (used)
                continue;
            DeleteRows(id);
            QueryResult count = CharacterDatabase.Query("SELECT COUNT(*) FROM `characters` c JOIN `custom_races_base` b ON b.`race` = c.`race` "
                "WHERE c.`race` = {}", id);
            if (count && (*count)[0].Get<uint64>())
            {
                CharacterDatabase.DirectExecute("UPDATE `characters` c JOIN `custom_races_base` b ON b.`race` = c.`race` SET c.`race` = b.`base`, "
                    "c.`skin` = 0, c.`face` = 0, c.`hairStyle` = 0, c.`hairColor` = 0, c.`facialStyle` = 0 WHERE c.`race` = {}", id);
                LOG_INFO("module", "mod-custom-races: race number {} was given up; its {} character(s) keep their base race",
                    id, (*count)[0].Get<uint64>());
            }
            CharacterDatabase.DirectExecute("DELETE FROM `custom_races_moved` WHERE `race` = {}", id);
            CharacterDatabase.DirectExecute("DELETE FROM `custom_races_base` WHERE `race` = {}", id);
        }
    }

    // ---------------------------------------------------------------------------------- the racial rows from the server data folder

    /// The racial rows one race brings, as AFK Realm wrote them (afk-realm\races\<key>.sql of the server data folder):
    /// the statements to run, and the spell ids they are for - the file names them in a comment line, so that a later
    /// run can take them back out.
    struct RacialFile
    {
        std::vector<uint32> Spells;
        std::vector<std::string> Statements;
    };

    /// Where a statement of a racial file stands after one more line: whether a text in quotes is still open. A text
    /// may run over several lines (older files of AFK Realm wrote the line breaks of spell texts as they are), so
    /// the lines are joined until the statement is complete.
    bool QuoteOpenAfter(std::string const& text, bool open)
    {
        for (size_t i = 0; i < text.size(); ++i)
        {
            char const c = text[i];
            if (open && c == '\\')
                ++i;                                         // \' and \\ inside a text
            else if (c == '\'')
                open = !open;                                // '' inside a text closes and opens again: still open
        }
        return open;
    }

    /// The statements of a racial file. A statement the database could not read at all (a text without its closing
    /// quote, no closing semicolon) is left out with an error in the log: the core stops the whole server on such an
    /// error, and a broken file must not keep the realm from starting.
    bool ReadRacialFile(std::filesystem::path const& path, RacialFile& file)
    {
        std::ifstream in(path);
        if (!in)
            return false;

        std::string line, statement;
        bool open = false;                                   // a text in quotes runs on into the next line
        uint32 skipped = 0;
        while (std::getline(in, line))
        {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            if (statement.empty())
            {
                size_t const first = line.find_first_not_of(" \t");
                if (first == std::string::npos)
                    continue;
                line.erase(0, first);

                std::string const marker = "-- spells ";
                if (line.compare(0, marker.size(), marker) == 0)
                {
                    file.Spells = SpellIds(line.substr(marker.size()));
                    continue;
                }
                if (line.compare(0, 2, "--") == 0)
                    continue;
                statement = line;
            }
            else
                statement += (open ? "\\n" : " ") + line;    // inside a text: the line break, written as SQL reads it

            open = QuoteOpenAfter(line, open);
            size_t const last = statement.find_last_not_of(" \t");
            if (!open && last != std::string::npos && statement[last] == ';')
            {
                file.Statements.push_back(statement);
                statement.clear();
            }
        }
        if (!statement.empty())
            ++skipped;
        if (skipped)
            LOG_ERROR("module", "mod-custom-races: {} ends in the middle of a statement; that statement is left out (start AFK Realm's races page once to write the file again)", path.string());
        return !file.Statements.empty();
    }

    void DropRacialSpell(uint32 spell)
    {
        WorldDatabase.DirectExecute("DELETE FROM `spell_dbc` WHERE `ID` = {}", spell);
        WorldDatabase.DirectExecute("DELETE FROM `spell_script_names` WHERE `spell_id` = {}", spell);
        WorldDatabase.DirectExecute("DELETE FROM `spell_proc` WHERE `SpellId` = {}", spell);
    }

    /// Runs the racial files of the races that are on and keeps their spells in `custom_races_racials`; the rows of
    /// races that are off, gone from the settings or no longer in their file are taken back out. Without a server data
    /// folder nothing is touched on either side - a wrong path must not throw the rows away.
    void ApplyRacialRows()
    {
        WorldDatabase.DirectExecute("CREATE TABLE IF NOT EXISTS `custom_races_racials` (`race` TINYINT UNSIGNED NOT NULL, "
            "`spell` INT UNSIGNED NOT NULL, PRIMARY KEY (`race`, `spell`)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 "
            "COMMENT='mod-custom-races: racial rows of each race, as taken from the server data folder'");

        // Where AFK Realm wrote the files: CustomRaces.RacialsFolder (it writes it there since it reads the racials of CoA
        // Custom), or else afk-realm\\races of the server data folder, as before.
        std::string const own = sConfigMgr->GetOption<std::string>("CustomRaces.RacialsFolder", "", false);
        std::string const folder = sConfigMgr->GetOption<std::string>("CustomRaces.ServerDataFolder", "", false);
        if (own.empty() && folder.empty())
        {
            LOG_INFO("module", "mod-custom-races: neither CustomRaces.RacialsFolder nor CustomRaces.ServerDataFolder is set; the races bring no racial rows of their own");
            return;
        }

        std::filesystem::path const dir = !own.empty() ? std::filesystem::path(own) : std::filesystem::path(folder) / "afk-realm" / "races";
        std::error_code ec;
        if (!std::filesystem::is_directory(dir, ec))
        {
            LOG_WARN("module", "mod-custom-races: {} is no folder of racial rows; the races bring no racial rows of their own", dir.string());
            return;
        }

        std::map<uint8, RacialFile> files;
        for (RaceDef const* race : active)
        {
            RacialFile file;
            if (ReadRacialFile(dir / (race->Key + ".sql"), file) && !file.Spells.empty())
                files[race->Id] = std::move(file);
        }

        auto inFile = [&files](uint8 race, uint32 spell)
        {
            auto it = files.find(race);
            return it != files.end() && std::find(it->second.Spells.begin(), it->second.Spells.end(), spell) != it->second.Spells.end();
        };
        auto inAnyFile = [&files](uint32 spell)
        {
            for (auto const& pair : files)
                if (std::find(pair.second.Spells.begin(), pair.second.Spells.end(), spell) != pair.second.Spells.end())
                    return true;
            return false;
        };

        uint32 dropped = 0;
        if (QueryResult rows = WorldDatabase.Query("SELECT `race`, `spell` FROM `custom_races_racials`"))
            do
            {
                uint8 const race = (*rows)[0].Get<uint8>();
                uint32 const spell = (*rows)[1].Get<uint32>();
                if (inFile(race, spell))
                    continue;

                WorldDatabase.DirectExecute("DELETE FROM `custom_races_racials` WHERE `race` = {} AND `spell` = {}", race, spell);
                if (!inAnyFile(spell))
                {
                    DropRacialSpell(spell);
                    ++dropped;
                }
            } while (rows->NextRow());

        uint32 taken = 0;
        for (auto const& pair : files)
        {
            for (std::string const& statement : pair.second.Statements)
                WorldDatabase.DirectExecute(statement);

            WorldDatabase.DirectExecute("DELETE FROM `custom_races_racials` WHERE `race` = {}", pair.first);
            for (uint32 spell : pair.second.Spells)
                WorldDatabase.DirectExecute("REPLACE INTO `custom_races_racials` (`race`, `spell`) VALUES ({}, {})", pair.first, spell);
            ++taken;
        }

        if (taken || dropped)
            LOG_INFO("module", "mod-custom-races: racial rows of {} race(s) taken over from {}, {} spell(s) given back", taken, dir.string(), dropped);
    }

    // ---------------------------------------------------------------------------------- the voices of the emotes

    /// One row of a voice table of the server's data folder (EmotesTextSound.dbc, HDEmotesTextSound.dbc): the sound
    /// one race speaks at one emote, in one sex.
    struct VoiceRow
    {
        uint32 Id, EmotesTextId, RaceId, SexId, SoundId;
    };

    /// The rows of one voice table of the server's data folder, as the core reads the classic one a moment later. The
    /// file is nothing but numbers: a header of five counts, then rows of five numbers - id, emote, race, sex (0 a
    /// man, 1 a woman), sound.
    std::vector<VoiceRow> ReadVoiceFile(std::string const& file, bool complain)
    {
        std::string path = sConfigMgr->GetOption<std::string>("DataDir", "./");
        if (path.empty() || (path.back() != '/' && path.back() != '\\'))
            path.push_back('/');
        path += "dbc/" + file;

        std::ifstream in(path, std::ios::binary);
        uint32 header[5] = {};
        if (!in || !in.read(reinterpret_cast<char*>(header), sizeof(header)) ||
            header[0] != 0x43424457 || header[2] != 5 || header[3] != sizeof(VoiceRow))
        {
            if (complain)
                LOG_WARN("module", "mod-custom-races: {} is not an EmotesTextSound table; the races speak no voices of their base races", path);
            return {};
        }

        std::vector<VoiceRow> rows;
        for (uint32 i = 0; i < header[1]; ++i)
        {
            uint32 row[5] = {};
            if (!in.read(reinterpret_cast<char*>(row), sizeof(row)))
                break;
            rows.push_back({ row[0], row[1], row[2], row[3], row[4] });
        }
        return rows;
    }

    /// The voice rows of the server's own tables: EmotesTextSound.dbc, and the HD table where the data folder has it
    /// (HDEmotesTextSound.dbc - it holds the lines an HD client speaks, the laughter and the crying the classic table
    /// has none of). Of rows that stand in both, the HD one wins: its sound is the one the client plays for the
    /// models these races wear.
    std::vector<VoiceRow> ReadVoiceTable()
    {
        std::vector<VoiceRow> rows = ReadVoiceFile("EmotesTextSound.dbc", true);
        for (VoiceRow const& row : ReadVoiceFile("HDEmotesTextSound.dbc", false))
        {
            auto same = [&row](VoiceRow const& other)
            {
                return other.EmotesTextId == row.EmotesTextId && other.RaceId == row.RaceId && other.SexId == row.SexId;
            };
            if (auto it = std::find_if(rows.begin(), rows.end(), same); it != rows.end())
                it->SoundId = row.SoundId;
            else
                rows.push_back(row);
        }
        return rows;
    }

    /// The voice of every emote a character says: the game client speaks those of the stock races itself, from its
    /// own table - but it has none for a race of ours, whatever rows a patch brings it (its voice table ends at the
    /// stock races). So the rows of the base race are lent to the new race in the world database, in the table the
    /// core reads the rows of the client's voice table from (emotetextsound_dbc) - the same way the rows of the race
    /// itself are lent. They are written again at every start; DeleteRows takes them back for a race that is off,
    /// that leaves the settings or that is given up, and the sound of a said emote is sent by CustomRacesPlayer.
    void LendVoices()
    {
        std::vector<VoiceRow> const table = ReadVoiceTable();
        if (table.empty())
            return;

        // New rows get numbers above every number of the table and of the rows others wrote into it, so no row of
        // the client's own table is ever overridden.
        uint32 nextId = 1;
        for (VoiceRow const& row : table)
            nextId = std::max(nextId, row.Id + 1);
        if (QueryResult highest = WorldDatabase.Query("SELECT `Id` FROM `emotetextsound_dbc` ORDER BY `Id` DESC LIMIT 1"))
            nextId = std::max(nextId, (*highest)[0].Get<uint32>() + 1);

        uint32 lent = 0;
        for (RaceDef const* race : active)
            for (VoiceRow const& row : table)
                if (row.RaceId == race->Base)
                {
                    WorldDatabase.DirectExecute("REPLACE INTO `emotetextsound_dbc` (`Id`, `EmotesTextId`, `RaceId`, `SexId`, `SoundId`) "
                        "VALUES ({}, {}, {}, {}, {})", nextId++, row.EmotesTextId, race->Id, row.SexId, row.SoundId);
                    ++lent;
                }

        if (lent)
            LOG_INFO("module", "mod-custom-races: {} voice(s) of their base races lent to {} race(s)", lent, active.size());
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
        if (!bit)
        {
            LOG_INFO("module", "mod-custom-races: {} ({}) plays as its base race does: the core gives it its base race's mask", race.Name, race.Id);
            return;
        }

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
        active.clear();
        ReadRaces();
        std::vector<RaceDef const*> const wanted = Wanted();
        // The base race of each race, for data/sql/remove: removing the module turns their characters into it.
        CharacterDatabase.DirectExecute("CREATE TABLE IF NOT EXISTS `custom_races_base` (`race` TINYINT UNSIGNED NOT NULL, "
            "`base` TINYINT UNSIGNED NOT NULL, PRIMARY KEY (`race`)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 "
            "COMMENT='mod-custom-races: the base race of each race'");
        CharacterDatabase.DirectExecute("CREATE TABLE IF NOT EXISTS `custom_races_moved` ("
            "`guid` INT UNSIGNED NOT NULL, `race` TINYINT UNSIGNED NOT NULL, `base` TINYINT UNSIGNED NOT NULL, "
            "`skin` TINYINT UNSIGNED NOT NULL, `face` TINYINT UNSIGNED NOT NULL, `hairStyle` TINYINT UNSIGNED NOT NULL, "
            "`hairColor` TINYINT UNSIGNED NOT NULL, `facialStyle` TINYINT UNSIGNED NOT NULL, PRIMARY KEY (`guid`)) "
            "ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod-custom-races: characters of a race switched off, and their looks'");
        // The voices of the emotes are lent in the world database, in the table the core reads the voice rows of the
        // client's own table from. The core's files create it as well; this is for a server whose database never saw
        // them, so that the rows are carried either way.
        WorldDatabase.DirectExecute("CREATE TABLE IF NOT EXISTS `emotetextsound_dbc` (`Id` INT NOT NULL DEFAULT '0', "
            "`EmotesTextId` INT NOT NULL DEFAULT '0', `RaceId` INT NOT NULL DEFAULT '0', `SexId` INT NOT NULL DEFAULT '0', "
            "`SoundId` INT NOT NULL DEFAULT '0', PRIMARY KEY (`Id`) USING BTREE) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 "
            "COMMENT='mod-custom-races: the voices of its races, lent from their base races'");
        FreeNumbers();
        for (RaceDef const& race : races)
        {
            if (!race.Id)
                continue;                                    // no number on this realm: never on, no rows, no characters
            CharacterDatabase.DirectExecute("REPLACE INTO `custom_races_base` (`race`, `base`) VALUES ({}, {})", race.Id, race.Base);
            bool const on = std::find(wanted.begin(), wanted.end(), &race) != wanted.end();
            WriteRace(race, on);
            MoveCharacters(race, on);
            if (on)
                active.push_back(&race);
        }

        // The voices every emote of a race of ours says: the client has none for it, so the base race lends them.
        LendVoices();

        // A race above 32 has the mask of its base race from the start: the core reads the races, their skills and
        // spells right after this.
        for (RaceDef const* race : active)
            if (race->Id > 32)
                SetRaceBase(race->Id, race->Base);

        // The racial rows the races bring of their own; the core reads the spell tables right after this.
        ApplyRacialRows();

        std::string names;
        for (RaceDef const* race : active)
            names += (names.empty() ? "" : ", ") + race->Name + " (" + std::to_string(race->Id) + ")";
        LOG_INFO("module", "mod-custom-races: playable races added: {}", names.empty() ? "none" : names);
    }

    /// The core has read everything; the new races lean on their base races from here on.
    void OnBeforeWorldInitialized() override
    {
        for (uint8 race = 0; race < AscensionRacialAbilities::BorrowedRacials.size(); ++race)
        {
            AscensionRacialAbilities::BorrowedRacials[race] = 0;
            SetRaceBase(race, 0);
            SetExtraAppearanceKind(race, 0);
        }
        for (RaceDef const* race : active)
        {
            AscensionRacialAbilities::BorrowedRacials[race->Id] = race->Base;
            SetRaceBase(race->Id, race->Base);
            SetExtraAppearanceKind(race->Id, race->Appearance);
            LendMasks(*race);
        }
    }
};

namespace
{
    RaceDef const* ActiveRace(Player const* player)
    {
        for (RaceDef const* race : active)
            if (race->Id == player->getRace(true))
                return race;
        return nullptr;
    }

    /// A race without helmet models of its own would show a placeholder for every helmet: its characters keep their
    /// helmets hidden - set when they are made and at every login, which covers the random bots of mod-playerbots.
    /// A player may show the helmet again for the session (Interface options).
    /// A race that has helmets (its own, or those of its base race through CustomRaces.<race>.HelmetPrefix) shows them.
    /// Earlier versions hid the helmets of such races too, and the game keeps that choice with the character for good:
    /// those characters walked without helmets ever after. The old mark is taken away once per character
    /// (custom_races_helmet), so a player who hides the helmet himself afterwards keeps it hidden; a bot never chooses
    /// it, so a bot always shows it.
    void HideHelmet(Player* player, bool login)
    {
        RaceDef const* race = ActiveRace(player);
        if (!race)
            return;
        if (!race->OwnHelmets)
        {
            player->SetPlayerFlag(PLAYER_FLAGS_HIDE_HELM);
            return;
        }
        if (!login || !player->HasPlayerFlag(PLAYER_FLAGS_HIDE_HELM))
            return;

        WorldSession* session = player->GetSession();
        bool bot = session && session->IsBot();
        ObjectGuid::LowType guid = player->GetGUID().GetCounter();
        if (!bot && CharacterDatabase.Query("SELECT 1 FROM custom_races_helmet WHERE guid = {}", guid))
            return;
        player->RemovePlayerFlag(PLAYER_FLAGS_HIDE_HELM);
        if (!bot)
            CharacterDatabase.Execute("REPLACE INTO custom_races_helmet (guid) VALUES ({})", guid);
    }

    /// Bots made before their race had sections (or by a playerbots version that knew nothing of the race) carry every
    /// appearance byte 0. When the race has no section for that look, the game draws them as untextured white models.
    /// This holds for every race of the module, not only the male-only ones it was first seen on (Gnoll): such a
    /// character gets the first valid look of its race and sex. A look of zeros the race does have stays, and so does
    /// every other look - the Esteria races keep packed values the sections do not list (a repair by the sections reset
    /// them), so only the case of nothing at all is touched.
    bool RepairBlankLook(Player* player, RaceDef const* race)
    {
        if (player->GetByteValue(PLAYER_BYTES, 0) != 0 || player->GetByteValue(PLAYER_BYTES, 1) != 0 ||
            player->GetByteValue(PLAYER_BYTES, 2) != 0 || player->GetByteValue(PLAYER_BYTES, 3) != 0 ||
            player->GetByteValue(PLAYER_BYTES_2, 0) != 0)
            return false;

        uint8 gender = player->getGender();
        bool any = false, zeroSkin = false, zeroFace = false;
        for (CharSectionsEntry const* section : sCharSectionsStore)
        {
            if (section->Race != race->Id || section->Gender != gender)
                continue;
            any = true;
            if (section->GenType == SECTION_TYPE_SKIN && section->Color == 0)
                zeroSkin = true;
            if (section->GenType == SECTION_TYPE_FACE && section->Type == 0 && section->Color == 0)
                zeroFace = true;
        }
        if (!any || (zeroSkin && zeroFace))
            return false;

        for (CharSectionsEntry const* section : sCharSectionsStore)
        {
            if (section->Race != race->Id || section->Gender != gender)
                continue;
            switch (section->GenType)
            {
                case SECTION_TYPE_SKIN: player->SetByteValue(PLAYER_BYTES, 0, section->Color); break;
                case SECTION_TYPE_FACE: player->SetByteValue(PLAYER_BYTES, 1, section->Type); break;
                case SECTION_TYPE_HAIR: player->SetByteValue(PLAYER_BYTES, 2, section->Type); player->SetByteValue(PLAYER_BYTES, 3, section->Color); break;
                case SECTION_TYPE_FACIAL_HAIR: player->SetByteValue(PLAYER_BYTES_2, 0, section->Type); break;
                default: break;
            }
        }
        return true;
    }

    /// A race without a female model is male: an old female character of it (made before the flag, or a bot
    /// created by an older playerbots version) becomes a man again at login; a race without a male model is female in
    /// the same way. A blank look of any race of the module is
    /// repaired as well (RepairBlankLook). Returns whether the character changed.
    bool MakeMale(Player* player)
    {
        RaceDef const* race = player ? ActiveRace(player) : nullptr;
        if (!race)
            return false;

        bool changed = false;
        if (race->MaleOnly && player->getGender() == GENDER_FEMALE)
        {
            player->SetByteValue(PLAYER_BYTES_3, 0, GENDER_MALE);
            player->SetByteValue(UNIT_FIELD_BYTES_0, 2, GENDER_MALE);   // race, class, gender, power
            player->SetUInt32Value(PLAYER_BYTES, 0);             // skin, face, hair style and colour of a man
            player->SetByteValue(PLAYER_BYTES_2, 0, 0);          // no beard
            changed = true;
        }
        // The same the other way round for a race with women only (CoA Custom's Banshee, Harpy, Alpha Goblin, whose
        // man has no head): a man of it - a random bot above all - becomes a woman with a look of her race.
        if (race->FemaleOnly && player->getGender() == GENDER_MALE)
        {
            player->SetByteValue(PLAYER_BYTES_3, 0, GENDER_FEMALE);
            player->SetByteValue(UNIT_FIELD_BYTES_0, 2, GENDER_FEMALE);
            player->SetUInt32Value(PLAYER_BYTES, 0);
            player->SetByteValue(PLAYER_BYTES_2, 0, 0);
            changed = true;
        }

        // the male-only races were repaired whenever their look was blank, as before; the others only when the
        // blank look is none of their race
        if (race->MaleOnly && player->GetUInt32Value(PLAYER_BYTES) == 0 && player->GetByteValue(PLAYER_BYTES_2, 0) == 0)
        {
            for (CharSectionsEntry const* section : sCharSectionsStore)
            {
                if (section->Race != race->Id || section->Gender != GENDER_MALE)
                    continue;
                switch (section->GenType)
                {
                    case SECTION_TYPE_SKIN: player->SetByteValue(PLAYER_BYTES, 0, section->Color); break;
                    case SECTION_TYPE_FACE: player->SetByteValue(PLAYER_BYTES, 1, section->Type); break;
                    case SECTION_TYPE_HAIR: player->SetByteValue(PLAYER_BYTES, 2, section->Type); player->SetByteValue(PLAYER_BYTES, 3, section->Color); break;
                    case SECTION_TYPE_FACIAL_HAIR: player->SetByteValue(PLAYER_BYTES_2, 0, section->Type); break;
                    default: break;
                }
            }
            changed = player->GetUInt32Value(PLAYER_BYTES) != 0 || changed;
        }
        else if (RepairBlankLook(player, race))
            changed = true;

        if (changed || race->MaleOnly || race->FemaleOnly)
            player->InitDisplayIds();
        return changed;
    }

    /// The racials a character knows: the ones of the race it plays, learned at every login; the ones of every other
    /// race of this realm taken away - so a race switched off takes its racials from its characters at the first
    /// login. Spells the server does not know (rows not taken over) are left out.
    void LearnRacials(Player* player)
    {
        std::unordered_set<uint32> wanted;
        for (RaceDef const* race : active)
            if (race->Id == player->getRace(true))
                wanted.insert(race->RacialSpells.begin(), race->RacialSpells.end());

        for (RaceDef const& race : races)
            for (uint32 spell : race.RacialSpells)
                if (!wanted.count(spell) && player->HasSpell(spell))
                    player->removeSpell(spell, SPEC_MASK_ALL, false);

        for (uint32 spell : wanted)
            if (!player->HasSpell(spell) && sSpellMgr->GetSpellInfo(spell))
                player->learnSpell(spell);
    }
}

/// A race without a female model is male: a woman of it has no body. Players cannot choose female for it in the
/// character creation; the random bots of mod-playerbots choose a gender at random, so they are made men here.
class CustomRacesPlayer : public PlayerScript
{
public:
    CustomRacesPlayer() : PlayerScript("CustomRacesPlayer", { PLAYERHOOK_ON_CREATE_INITIAL_ITEMS, PLAYERHOOK_ON_LOGIN,
        PLAYERHOOK_ON_TEXT_EMOTE }) { }

    bool OnPlayerCreateInitialItems(Player* player, bool& /*handled*/) override
    {
        if (!player)
            return true;
        HideHelmet(player, false);
        MakeMale(player);
        return true;
    }

    void OnPlayerLogin(Player* player) override
    {
        if (!player)
            return;

        HideHelmet(player, true);
        uint8 oldGender = player->getGender();
        MakeMale(player);
        if (oldGender != player->getGender())
            CharacterDatabase.Execute("UPDATE characters SET gender = {} WHERE guid = {}",
                player->GetByteValue(PLAYER_BYTES_3, 0), player->GetGUID().GetCounter());

        LearnRacials(player);
    }

    /// The voice of a text emote (/silly, /joke and so on): the game client speaks the voice of a stock race itself,
    /// from its own voice table - but it plays nothing for a race of ours, whatever rows a patch brings it. So the
    /// voice the base race would speak is sent from here, from the rows the base race lent (LendVoices); everyone
    /// around hears it, as they hear the voice of a stock race.
    void OnPlayerTextEmote(Player* player, uint32 textEmote, uint32 /*emoteNum*/, ObjectGuid /*guid*/) override
    {
        RaceDef const* race = player ? ActiveRace(player) : nullptr;
        if (!race)
            return;

        if (EmotesTextSoundEntry const* voice = FindTextSoundEmoteFor(textEmote, race->Id, player->getGender()))
            player->PlayDistanceSound(voice->SoundId);
    }
};

// The racial abilities of CoA Custom (Bull Rush and the passives of the Highmountain Tauren among them) are in
// coa_racials/, taken over from CoA Custom as a whole.

void AddSC_custom_races()
{
    new CustomRacesWorld();
    new CustomRacesPlayer();
}
