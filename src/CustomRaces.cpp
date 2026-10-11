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
 */

#include "AscensionRacialAbilities.h"
#include "Config.h"
#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "Player.h"
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
        bool OwnHelmets = true;                          // the client has helmet models for it
    };

    constexpr uint32 PLAYABLE_FLAGS = 0xC;
    // The values every playable race of the client has: humanoid, Resurrection Sickness, the splash sound.
    constexpr uint32 CREATURE_TYPE_HUMANOID_ID = 7;
    constexpr uint32 RESURRECTION_SICKNESS = 15007;
    constexpr uint32 SPLASH_SOUND = 1096;
    constexpr char const* PREFIX = "CustomRaces.";

    constexpr uint32 Bit(uint8 race) { return 1u << (race - 1); }

    BaseRace const* FindBase(uint8 id)
    {
        for (BaseRace const& base : BASES)
            if (base.Id == id)
                return &base;
        return nullptr;
    }

    /// A race number the module may use: 1 to 31 (races are bits of a 32-bit mask), and none of the stock races.
    bool Usable(uint32 id) { return id >= 1 && id <= 31 && !FindBase(uint8(id)); }

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
            race.MaleOnly = Number(race.Key, "MaleOnly") != 0;
            race.OwnHelmets = Number(race.Key, "OwnHelmets", 1) != 0;
            race.Base = ParseBase(race);

            uint32 const id = Number(race.Key, "Id", race.Source < 32 ? race.Source : 0);
            if (id && !Usable(id))
                LOG_ERROR("module", "mod-custom-races: CustomRaces.{}.Id is {}, which no new race can have (1 to 31, not a stock race); "
                    "the {} stays off", race.Key, id, race.Name);
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
        WorldDatabase.DirectExecute("DELETE FROM `playercreateinfo_skills` WHERE `raceMask` = {}", Bit(id));
        WorldDatabase.DirectExecute("DELETE FROM `playercreateinfo_spell_custom` WHERE `racemask` = {}", Bit(id));
        WorldDatabase.DirectExecute("DELETE FROM `ascension_custom_class_race` WHERE `race` = {}", id);
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
        for (uint8 id = 1; id < 32; ++id)
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
        }
        for (RaceDef const* race : active)
        {
            AscensionRacialAbilities::BorrowedRacials[race->Id] = race->Base;
            SetRaceBase(race->Id, race->Base);
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
    void HideHelmet(Player* player)
    {
        RaceDef const* race = ActiveRace(player);
        if (race && !race->OwnHelmets)
            player->SetPlayerFlag(PLAYER_FLAGS_HIDE_HELM);
    }
}

/// A race without a female model is male: a woman of it has no body. Players cannot choose female for it in the
/// character creation; the random bots of mod-playerbots choose a gender at random, so they are made men here.
class CustomRacesPlayer : public PlayerScript
{
public:
    CustomRacesPlayer() : PlayerScript("CustomRacesPlayer", { PLAYERHOOK_ON_CREATE_INITIAL_ITEMS, PLAYERHOOK_ON_LOGIN }) { }

    bool OnPlayerCreateInitialItems(Player* player, bool& /*handled*/) override
    {
        if (!player)
            return true;
        HideHelmet(player);
        RaceDef const* race = ActiveRace(player);
        if (race && race->MaleOnly && player->getGender() == GENDER_FEMALE)
        {
            player->SetByteValue(PLAYER_BYTES_3, 0, GENDER_MALE);
            player->SetByteValue(UNIT_FIELD_BYTES_0, 2, GENDER_MALE);   // race, class, gender, power
            player->SetUInt32Value(PLAYER_BYTES, 0);             // skin, face, hair style and colour of a man
            player->SetByteValue(PLAYER_BYTES_2, 0, 0);          // no beard
            player->InitDisplayIds();
        }
        return true;
    }

    void OnPlayerLogin(Player* player) override
    {
        if (player)
            HideHelmet(player);
    }
};

void AddSC_custom_races()
{
    new CustomRacesWorld();
    new CustomRacesPlayer();
}
