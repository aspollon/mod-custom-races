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
#include "RacePool.h"
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
#include <iterator>
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

    /// The races of the module (src/RacePool.h, written by tools/pool.py).
    using RaceDef = CustomRacesPool::RaceDef;
    using CustomRacesPool::POOL;

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
        std::string list = sConfigMgr->GetOption<std::string>("CustomRaces.Races", "");
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
    uint8 baseOf[32] = {};
    uint8 idOf[std::size(POOL)] = {};

    /// The number of a race on this realm: its own, or the free number CustomRaces.<race>.Id gave it (0: none).
    uint8 IdOf(RaceDef const& race) { return idOf[&race - POOL]; }

    /// The numbers of the races, checked: a race without a number of its own needs one of the free numbers, and no
    /// two races may share one. A race left without a number is off.
    void ChooseIds()
    {
        std::set<uint8> taken;
        for (RaceDef const& race : POOL)
            if (race.Id)
                taken.insert(race.Id);
        for (RaceDef const& race : POOL)
        {
            uint8 id = race.Id;
            if (!id)
            {
                uint32 const wanted = sConfigMgr->GetOption<uint32>(std::string("CustomRaces.") + race.Key + ".Id", 0);
                if (wanted && std::find(std::begin(CustomRacesPool::SLOTS), std::end(CustomRacesPool::SLOTS), wanted) ==
                    std::end(CustomRacesPool::SLOTS))
                    LOG_ERROR("module", "mod-custom-races: CustomRaces.{}.Id is {}, which is not one of the free race numbers; the {} "
                        "stays off", race.Key, wanted, race.Name);
                else if (wanted && taken.count(uint8(wanted)))
                    LOG_ERROR("module", "mod-custom-races: CustomRaces.{}.Id is {}, which another race has; the {} stays off",
                        race.Key, wanted, race.Name);
                else
                    id = uint8(wanted);
                if (id)
                    taken.insert(id);
            }
            idOf[&race - POOL] = id;
        }
    }

    /// The stock race a new race leans on: CustomRaces.<race>.Base, by name, or the race's own default. It decides
    /// the faction, the start, the quests and the racial abilities of the race.
    uint8 ChooseBase(RaceDef const& race)
    {
        std::string const wanted = Lower(sConfigMgr->GetOption<std::string>(std::string("CustomRaces.") + race.Key + ".Base", ""));
        if (wanted.empty())
            return race.Base;
        for (BaseRace const& base : BASES)
            if (Lower(base.Name) == wanted || (wanted == "night elf" && base.Id == 4) || (wanted == "blood elf" && base.Id == 10) ||
                (wanted == "scourge" && base.Id == 5))
                return base.Id;
        LOG_WARN("module", "mod-custom-races: CustomRaces.{}.Base names \"{}\", which is not a stock race; {} keeps its default", race.Key,
            wanted, race.Name);
        return race.Base;
    }

    uint8 BaseFor(RaceDef const& race) { return IdOf(race) && IdOf(race) < 32 && baseOf[IdOf(race)] ? baseOf[IdOf(race)] : race.Base; }

    bool IsWanted(std::vector<RaceDef const*> const& wanted, RaceDef const& race)
    {
        return std::find(wanted.begin(), wanted.end(), &race) != wanted.end();
    }

    // ---------------------------------------------------------------------------------- the world database

    /// The rows of one race: removed always, written again while the race is on.
    void WriteRace(RaceDef const& race, bool on)
    {
        uint32 const bit = Bit(IdOf(race));
        WorldDatabase.DirectExecute("DELETE FROM `chrraces_dbc` WHERE `ID` = {}", IdOf(race));
        WorldDatabase.DirectExecute("DELETE FROM `playercreateinfo` WHERE `race` = {}", IdOf(race));
        WorldDatabase.DirectExecute("DELETE FROM `playercreateinfo_action` WHERE `race` = {}", IdOf(race));
        WorldDatabase.DirectExecute("DELETE FROM `playercreateinfo_item` WHERE `race` = {}", IdOf(race));
        WorldDatabase.DirectExecute("DELETE FROM `player_race_stats` WHERE `Race` = {}", IdOf(race));
        WorldDatabase.DirectExecute("DELETE FROM `playercreateinfo_skills` WHERE `raceMask` = {}", bit);
        WorldDatabase.DirectExecute("DELETE FROM `playercreateinfo_spell_custom` WHERE `racemask` = {}", bit);
        WorldDatabase.DirectExecute("DELETE FROM `ascension_custom_class_race` WHERE `race` = {}", IdOf(race));
        if (!on)
            return;

        BaseRace const* base = FindBase(BaseFor(race));
        uint32 const baseBit = Bit(BaseFor(race));
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
        // A race with a number of its own on this realm has no row in the client table: its texts come from here.
        WorldDatabase.DirectExecute("INSERT INTO `chrraces_dbc` (`ID`, `Flags`, `FactionID`, `ExplorationSoundID`, `MaleDisplayId`, "
            "`FemaleDisplayId`, `ClientPrefix`, `BaseLanguage`, `CreatureType`, `ResSicknessSpellID`, `SplashSoundID`, `ClientFilestring`, "
            "`CinematicSequenceID`, `Alliance`{}, `Name_Lang_Mask`, `Required_Expansion`) "
            "VALUES ({}, {}, {}, {}, {}, {}, '{}', {}, {}, {}, {}, '{}', 0, {}{}, 0, 0)",
            names, IdOf(race), PLAYABLE_FLAGS, base->FactionTemplate, base->ExplorationSound, race.MaleDisplay, race.FemaleDisplay,
            race.Prefix, base->Language, CREATURE_TYPE_HUMANOID_ID, RESURRECTION_SICKNESS, SPLASH_SOUND, race.File, base->Alliance, values);
        WorldDatabase.DirectExecute("INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, "
            "`position_z`, `orientation`) SELECT {}, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation` "
            "FROM `playercreateinfo` WHERE `race` = {}", IdOf(race), BaseFor(race));
        WorldDatabase.DirectExecute("INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) "
            "SELECT {}, `class`, `button`, `action`, `type` FROM `playercreateinfo_action` WHERE `race` = {}", IdOf(race), BaseFor(race));
        WorldDatabase.DirectExecute("INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`, `Note`) "
            "SELECT {}, `class`, `itemid`, `amount`, `Note` FROM `playercreateinfo_item` WHERE `race` = {}", IdOf(race), BaseFor(race));
        WorldDatabase.DirectExecute("INSERT INTO `player_race_stats` (`Race`, `Strength`, `Agility`, `Stamina`, `Intellect`, `Spirit`) "
            "SELECT {}, `Strength`, `Agility`, `Stamina`, `Intellect`, `Spirit` FROM `player_race_stats` WHERE `Race` = {}",
            IdOf(race), BaseFor(race));
        WorldDatabase.DirectExecute("INSERT IGNORE INTO `playercreateinfo_skills` (`raceMask`, `classMask`, `skill`, `rank`, `comment`) "
            "SELECT {}, `classMask`, `skill`, `rank`, `comment` FROM `playercreateinfo_skills` WHERE `raceMask` <> 0 AND "
            "(`raceMask` & {}) <> 0", bit, baseBit);
        WorldDatabase.DirectExecute("INSERT IGNORE INTO `playercreateinfo_spell_custom` (`racemask`, `classmask`, `Spell`, `Note`) "
            "SELECT {}, `classmask`, `Spell`, `Note` FROM `playercreateinfo_spell_custom` WHERE `racemask` <> 0 AND "
            "(`racemask` & {}) <> 0", bit, baseBit);
        WorldDatabase.DirectExecute("INSERT IGNORE INTO `ascension_custom_class_race` (`class`, `race`) "
            "SELECT `class`, {} FROM `ascension_custom_class_race` WHERE `race` = {}", IdOf(race), BaseFor(race));
    }

    // ---------------------------------------------------------------------------------- the characters

    /// A character of a race that is off becomes its base race; one whose race is on again gets it back.
    void MoveCharacters(RaceDef const& race, bool on)
    {


        if (on)
        {
            QueryResult back = CharacterDatabase.Query("SELECT COUNT(*) FROM `custom_races_moved` m JOIN `characters` c ON c.`guid` = m.`guid` "
                "WHERE m.`race` = {} AND c.`race` = m.`base`", IdOf(race));
            CharacterDatabase.DirectExecute("UPDATE `characters` c JOIN `custom_races_moved` m ON c.`guid` = m.`guid` SET c.`race` = m.`race`, "
                "c.`skin` = m.`skin`, c.`face` = m.`face`, c.`hairStyle` = m.`hairStyle`, c.`hairColor` = m.`hairColor`, "
                "c.`facialStyle` = m.`facialStyle` WHERE m.`race` = {} AND c.`race` = m.`base`", IdOf(race));
            CharacterDatabase.DirectExecute("DELETE FROM `custom_races_moved` WHERE `race` = {}", IdOf(race));
            if (back && (*back)[0].Get<uint64>())
                LOG_INFO("module", "mod-custom-races: {} character(s) are {} again", (*back)[0].Get<uint64>(), race.Name);
            return;
        }

        QueryResult count = CharacterDatabase.Query("SELECT COUNT(*) FROM `characters` WHERE `race` = {}", IdOf(race));
        if (!count || !(*count)[0].Get<uint64>())
            return;
        CharacterDatabase.DirectExecute("REPLACE INTO `custom_races_moved` (`guid`, `race`, `base`, `skin`, `face`, `hairStyle`, `hairColor`, "
            "`facialStyle`) SELECT `guid`, `race`, {}, `skin`, `face`, `hairStyle`, `hairColor`, `facialStyle` FROM `characters` "
            "WHERE `race` = {}", BaseFor(race), IdOf(race));
        CharacterDatabase.DirectExecute("UPDATE `characters` SET `race` = {}, `skin` = 0, `face` = 0, `hairStyle` = 0, `hairColor` = 0, "
            "`facialStyle` = 0 WHERE `race` = {}", BaseFor(race), IdOf(race));
        LOG_INFO("module", "mod-custom-races: {} is off; {} character(s) of it play their base race until it is on again",
            race.Name, (*count)[0].Get<uint64>());
    }

    /// A free number no race has any more: a race was removed from this realm (CustomRaces.<race>.Id set back to 0).
    /// Its characters stay the base race they had for good, and its rows go.
    void FreeSlots()
    {
        for (uint8 slot : CustomRacesPool::SLOTS)
        {
            bool used = false;
            for (RaceDef const& race : POOL)
                used |= IdOf(race) == slot;
            if (used)
                continue;
            WorldDatabase.DirectExecute("DELETE FROM `chrraces_dbc` WHERE `ID` = {}", slot);
            WorldDatabase.DirectExecute("DELETE FROM `playercreateinfo` WHERE `race` = {}", slot);
            WorldDatabase.DirectExecute("DELETE FROM `playercreateinfo_action` WHERE `race` = {}", slot);
            WorldDatabase.DirectExecute("DELETE FROM `playercreateinfo_item` WHERE `race` = {}", slot);
            WorldDatabase.DirectExecute("DELETE FROM `player_race_stats` WHERE `Race` = {}", slot);
            WorldDatabase.DirectExecute("DELETE FROM `playercreateinfo_skills` WHERE `raceMask` = {}", Bit(slot));
            WorldDatabase.DirectExecute("DELETE FROM `playercreateinfo_spell_custom` WHERE `racemask` = {}", Bit(slot));
            WorldDatabase.DirectExecute("DELETE FROM `ascension_custom_class_race` WHERE `race` = {}", slot);
            QueryResult count = CharacterDatabase.Query("SELECT COUNT(*) FROM `characters` c JOIN `custom_races_base` b ON b.`race` = c.`race` "
                "WHERE c.`race` = {}", slot);
            if (count && (*count)[0].Get<uint64>())
            {
                CharacterDatabase.DirectExecute("UPDATE `characters` c JOIN `custom_races_base` b ON b.`race` = c.`race` SET c.`race` = b.`base`, "
                    "c.`skin` = 0, c.`face` = 0, c.`hairStyle` = 0, c.`hairColor` = 0, c.`facialStyle` = 0 WHERE c.`race` = {}", slot);
                LOG_INFO("module", "mod-custom-races: race number {} was given up; its {} character(s) keep their base race",
                    slot, (*count)[0].Get<uint64>());
            }
            CharacterDatabase.DirectExecute("DELETE FROM `custom_races_moved` WHERE `race` = {}", slot);
            CharacterDatabase.DirectExecute("DELETE FROM `custom_races_base` WHERE `race` = {}", slot);
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
        uint32 const bit = Bit(IdOf(race));
        uint32 const baseBit = Bit(BaseFor(race));
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
        // The base race of each race, for data/sql/remove: removing the module turns their characters into it.
        CharacterDatabase.DirectExecute("CREATE TABLE IF NOT EXISTS `custom_races_base` (`race` TINYINT UNSIGNED NOT NULL, "
            "`base` TINYINT UNSIGNED NOT NULL, PRIMARY KEY (`race`)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 "
            "COMMENT='mod-custom-races: the base race of each race'");
        CharacterDatabase.DirectExecute("CREATE TABLE IF NOT EXISTS `custom_races_moved` ("
            "`guid` INT UNSIGNED NOT NULL, `race` TINYINT UNSIGNED NOT NULL, `base` TINYINT UNSIGNED NOT NULL, "
            "`skin` TINYINT UNSIGNED NOT NULL, `face` TINYINT UNSIGNED NOT NULL, `hairStyle` TINYINT UNSIGNED NOT NULL, "
            "`hairColor` TINYINT UNSIGNED NOT NULL, `facialStyle` TINYINT UNSIGNED NOT NULL, PRIMARY KEY (`guid`)) "
            "ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod-custom-races: characters of a race switched off, and their looks'");
        ChooseIds();
        FreeSlots();
        for (RaceDef const& race : POOL)
            if (!IdOf(race) && IsWanted(wanted, race))
                LOG_WARN("module", "mod-custom-races: {} is in CustomRaces.Races but has no race number (CustomRaces.{}.Id); it stays off",
                    race.Name, race.Key);
        for (RaceDef const& race : POOL)
        {
            if (!IdOf(race))
                continue;                                    // no number on this realm: never on, no rows, no characters
            baseOf[IdOf(race)] = ChooseBase(race);
            CharacterDatabase.DirectExecute("REPLACE INTO `custom_races_base` (`race`, `base`) VALUES ({}, {})", IdOf(race), BaseFor(race));
            bool const on = IsWanted(wanted, race) && FindBase(BaseFor(race));
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
            AscensionRacialAbilities::BorrowedRacials[IdOf(*race)] = BaseFor(*race);
            SetRaceBase(IdOf(*race), BaseFor(*race));
            LendMasks(*race);
        }
    }
};

/// A race without a female model is male: a woman of it has no body. Players cannot choose female for it in the
/// character creation; the random bots of mod-playerbots choose a gender at random, so they are made men here.
class CustomRacesPlayer : public PlayerScript
{
public:
    CustomRacesPlayer() : PlayerScript("CustomRacesPlayer", { PLAYERHOOK_ON_CREATE_INITIAL_ITEMS }) { }

    bool OnPlayerCreateInitialItems(Player* player, bool& /*handled*/) override
    {
        if (!player || player->getGender() != GENDER_FEMALE)
            return true;
        for (RaceDef const* race : active)
            if (IdOf(*race) == player->getRace(true) && race->MaleOnly)
            {
                player->SetByteValue(PLAYER_BYTES_3, 0, GENDER_MALE);
                player->SetByteValue(UNIT_FIELD_BYTES_0, 2, GENDER_MALE);   // race, class, gender, power
                player->SetUInt32Value(PLAYER_BYTES, 0);         // skin, face, hair style and colour of a man
                player->SetByteValue(PLAYER_BYTES_2, 0, 0);      // no beard
                player->InitDisplayIds();
                break;
            }
        return true;
    }
};

void AddSC_custom_races()
{
    new CustomRacesWorld();
    new CustomRacesPlayer();
}
