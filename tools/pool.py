#!/usr/bin/env python3
"""
mod-custom-races: the races of the module, in one place.

Writes from the table below:
  src/RacePool.h                     the pool for the server
  conf/mod_custom_races.conf.dist    the settings of each race
  afk-realm.json                     the client patch AFK Realm builds (only the races and base races a realm chose)
  client/CustomRaces.lua             the part between "-- @POOL BEGIN" and "-- @POOL END"
  data/sql/remove/*.sql              what removing the module takes back

Run it after changing the table: python3 tools/pool.py
"""
import json, os, re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CONF = 'mod_custom_races.conf'

# The stock races a new race can lean on: id, name in the settings, faction template, language, side (0 Alliance,
# 1 Horde), exploration sound, file name of the client (keys of its tables in the character creation).
BASES = [
    (1, 'Human', 1, 7, 0, 4140, 'Human'),
    (2, 'Orc', 2, 1, 1, 4141, 'Orc'),
    (3, 'Dwarf', 3, 7, 0, 4147, 'Dwarf'),
    (4, 'NightElf', 4, 7, 0, 4145, 'NightElf'),
    (5, 'Undead', 5, 1, 1, 4142, 'Scourge'),
    (6, 'Tauren', 6, 1, 1, 4143, 'Tauren'),
    (7, 'Gnome', 115, 7, 0, 4146, 'Gnome'),
    (8, 'Troll', 116, 1, 1, 4144, 'Troll'),
    (10, 'BloodElf', 1610, 1, 1, 4142, 'BloodElf'),
    (11, 'Draenei', 1629, 7, 0, 4140, 'Draenei'),
]
BASE = {b[1]: b for b in BASES}

# The classes the character creation offers each new race: the classic classes (with mod-classic-classes) and the
# 21 classes of Conquest of Azeroth - 30, the most the character creation takes for a race.
CLASSES = [1, 2, 3, 4, 5, 7, 8, 9, 11] + list(range(12, 33))

# The pool. key: name in the settings; id: the race's number on this realm (races are bits of a 32-bit mask, so at
# most 31); source: its row in ChrRaces.dbc of the CoA client, whose looks it has; name; base: the stock race it
# leans on unless the settings say otherwise; looks: what to expect of the model; portrait for the race choice;
# names: the stock race whose names the random name button gives it.
#   looks: "armor" - shows armour, male and female; "male" - only a male model (the female one has no body);
#          "noarmor" - a model that shows no armour.
#   portrait: ("round", file key in RACE_ICON_TCOORDS of the stock races), ("old", left, right, top, bottom of the male
#   portrait in UI-CHARACTERCREATE-RACESROUND; the female one is two rows lower) or ("icon", male icon, female icon).
POOL = [
    dict(key='Goblin', id=9, source=9, name='Goblin', base='Orc', looks='armor', names='Gnome',
         portrait=('old', 0.625, 0.75, 0.25, 0.5),
         info='Goblins are cunning merchants and inventors, and the best of them sell to everyone.'),
    dict(key='Zandalari', id=12, source=12, name='Zandalari Troll', base='Troll', looks='armor', names='Troll',
         portrait=('icon', 'achievement_alliedrace_zandalaritroll', 'achievement_alliedrace_zandalaritroll'),
         info='The Zandalari are the oldest of the trolls, proud keepers of their gods and their empire.'),
    dict(key='Worgen', id=13, source=13, name='Worgen', base='Human', looks='armor', names='Human',
         portrait=('old', 0.625, 0.75, 0.0, 0.25),
         info='Cursed people of Gilneas who keep their minds in the shape of a wolf.'),
    dict(key='KulTiran', id=16, source=16, name='Kul Tiran', base='Human', looks='armor', names='Human',
         portrait=('icon', 'achievement_alliedrace_kultiranhuman', 'achievement_alliedrace_kultiranhuman'),
         info='Sturdy sailors of the island kingdom of Kul Tiras.'),
    dict(key='Vulpera', id=19, source=19, name='Vulpera', base='Tauren', looks='armor', names='Gnome',
         portrait=('icon', 'achievement_alliedrace_vulpera', 'achievement_alliedrace_vulpera'),
         info='Small desert foxes, scavengers and travellers who make a home wherever their caravan stops.'),
    dict(key='Pandaren', id=20, source=20, name='Pandaren', base='Human', looks='armor', names='Human',
         portrait=('icon', 'achievement_character_pandaren_female', 'achievement_character_pandaren_female'),
         info='Wanderers from the Wandering Isle who seek balance - and good food.'),
    dict(key='HighElf', id=0, source=10, name='High Elf', base='Human', looks='armor', names='BloodElf',
         portrait=('round', 'BLOODELF'),
         info='The quel\'dorei who stayed true to the Alliance after the fall of Quel\'Thalas.'),
    dict(key='FelOrc', id=0, source=34, name='Fel Orc', base='Orc', looks='male', names='Orc',
         portrait=('icon', 'achievement_boss_hellfire_felorccouncil', 'achievement_boss_hellfire_felorccouncil'),
         info='Orcs who drank the blood of demons, and kept its fury.'),
    dict(key='Broken', id=0, source=36, name='Broken', base='Draenei', looks='male', names='Draenei',
         portrait=('icon', 'achievement_boss_triumvirate_voidbrokenbrute', 'achievement_boss_triumvirate_voidbrokenbrute'),
         info='Draenei changed by the fel magic of the orcs, who still remember who they were.'),
    dict(key='Earthen', id=0, source=62, name='Earthen', base='Dwarf', looks='armor', names='Dwarf',
         portrait=('round', 'DWARF'),
         info='Living stone shaped by the titans, the forefathers of the dwarves.'),
    dict(key='Drakkari', id=0, source=44, name='Drakkari Troll', base='Troll', looks='armor', names='Troll',
         portrait=('round', 'TROLL'),
         info='The ice trolls of Zul\'Drak, an empire that fell to the Scourge.'),
    dict(key='Vrykul', id=0, source=38, name='Vrykul', base='Human', looks='male', names='Human',
         portrait=('icon', 'inv_misc_head_vrykul', 'inv_misc_head_vrykul'),
         info='Giant warriors of Northrend, made by the titans and fond of battle.'),
    dict(key='Tuskarr', id=0, source=39, name='Tuskarr', base='Dwarf', looks='male', names='Dwarf',
         portrait=('icon', 'inv_misc_head_tuskarr', 'inv_misc_head_tuskarr'),
         info='Fishers and hunters of the Northrend coasts, friendly to all who come in peace.'),
    dict(key='Taunka', id=0, source=41, name='Taunka', base='Tauren', looks='male', names='Tauren',
         portrait=('round', 'TAUREN'),
         info='The northern cousins of the tauren, hardened by the cold of Northrend.'),
    dict(key='ForestTroll', id=0, source=40, name='Forest Troll', base='Troll', looks='male', names='Troll',
         portrait=('round', 'TROLL'),
         info='The trolls of the forests of Lordaeron and the Hinterlands.'),
    dict(key='IceTroll', id=0, source=43, name='Ice Troll', base='Troll', looks='male', names='Troll',
         portrait=('round', 'TROLL'),
         info='The trolls of the frozen north, tough as the ice they live on.'),
    dict(key='Skeleton', id=0, source=37, name='Skeleton', base='Undead', looks='male', names='Undead',
         portrait=('round', 'SCOURGE'),
         info='Bones that walk on their own, free of the Lich King.'),
    dict(key='Naga', id=0, source=35, name='Naga', base='Undead', looks='noarmor', names='BloodElf',
         portrait=('icon', 'achievement_boss_nagazone', 'achievement_boss_nagazone'),
         info='Serpent people of the deep sea, once night elves of Zin-Azshari.'),
    dict(key='NorthrendSkeleton', id=0, source=42, name='Northrend Skeleton', base='Undead', looks='male', names='Undead',
         portrait=('round', 'SCOURGE'),
         info='The bones of Northrend, risen in the cold and left without a master.'),
    dict(key='LeperTroll', id=0, source=45, name='Leper Troll', base='Troll', looks='untested', names='Troll',
         portrait=('round', 'TROLL'),
         info='Trolls marked by a wasting sickness, and the harder for it.'),
    dict(key='Troglodyte', id=0, source=46, name='Troglodyte', base='Dwarf', looks='male', names='Dwarf',
         portrait=('icon', 'inv_misc_head_kobold_01', 'inv_misc_head_kobold_01'),
         info='Cave dwellers of the deep places of the world.'),
    dict(key='ThinHuman', id=0, source=33, name='Thin Human', base='Human', looks='male', names='Human',
         portrait=('round', 'HUMAN'),
         info='A lean folk of humans, quick and wiry.'),
    dict(key='HumanCultist', id=0, source=63, name='Human Cultist', base='Human', looks='untested', names='Human',
         portrait=('round', 'HUMAN'),
         info='Humans who gave themselves to dark powers - and lived to tell of it.'),
]

# The numbers a race without one of its own can get on a realm: free race ids of the client, up to 31.
SLOTS = [14, 15, 17, 18] + list(range(21, 32))

LOOKS_TEXT = {
    'untested': 'not tried yet',
    'armor': 'shows armour, male and female',
    'male': 'male only (the game has no female model)',
    'noarmor': 'shows no armour (its model has no places for it)',
}


def bit(race):
    return 1 << (race - 1)


def check():
    ids = [r['id'] for r in POOL if r['id']]
    assert len(ids) == len(set(ids)), 'two races share an id'
    assert all(1 <= i <= 31 for i in ids), 'race ids go up to 31'
    stock = {b[0] for b in BASES} | {6}
    assert not stock & set(ids), 'a race takes the id of a stock race'
    assert not set(ids) & set(SLOTS), 'a race takes a free number'
    for r in POOL:
        assert r['base'] in BASE and r['names'] in BASE, r['key']


# ------------------------------------------------------------------------------------------------- server

def write_header():
    lines = ['// Written by tools/pool.py - change the table there and run it again.', '#ifndef MOD_CUSTOM_RACES_POOL_H',
             '#define MOD_CUSTOM_RACES_POOL_H', '', 'namespace CustomRacesPool', '{',
             '    /// A race of the module: settings key, its own id (0: it takes a free number, CustomRaces.<key>.Id), its',
             '    /// row in ChrRaces.dbc of the client, name,',
             '    /// male and female display of that row, its client prefix and file name, its default base race, male only.',
             '    struct RaceDef', '    {', '        char const* Key;', '        unsigned char Id;', '        unsigned char Source;',
             '        char const* Name;', '        unsigned int MaleDisplay;', '        unsigned int FemaleDisplay;',
             '        char const* Prefix;', '        char const* File;', '        unsigned char Base;', '        bool MaleOnly;', '    };', '',
             '    inline constexpr RaceDef POOL[] =', '    {']
    for r in POOL:
        d = DISPLAYS[r['source']]
        lines.append('        { "%s", %d, %d, "%s", %d, %d, "%s", "%s", %d, %s },' % (
            r['key'], r['id'], r['source'], r['name'], d[0], d[1], d[2], d[3], BASE[r['base']][0],
            'true' if r['looks'] == 'male' else 'false'))
    lines += ['    };', '',
              '    /// The free race ids of the client a race without one of its own can take.',
              '    inline constexpr unsigned char SLOTS[] = { %s };' % ', '.join(str(x) for x in SLOTS),
              '}', '', '#endif', '']
    open(os.path.join(ROOT, 'src', 'RacePool.h'), 'w').write('\n'.join(lines))


# The male and female display, prefix and file name of the source rows in ChrRaces.dbc of the CoA client (10 October
# 2026); the server writes them for a race whose id has no row of its own there.
DISPLAYS = {
    9: (6894, 6895, 'Go', 'Goblin'), 10: (15476, 15475, 'Be', 'BloodElf'), 12: (141225, 141226, 'Za', 'Zandalari Troll'),
    13: (142817, 142818, 'Wo', 'Worgen'), 16: (141685, 141686, 'Kt', 'KulTiran'), 19: (141254, 141255, 'Vu', 'Vulpera'),
    20: (141687, 141688, 'Pa', 'Pandaren'), 34: (16981, 16980, 'Fo', 'FelOrc'), 35: (17402, 17403, 'Na', 'Naga_'),
    36: (17576, 17577, 'Br', 'Broken'), 37: (17578, 17579, 'Sk', 'Skeleton'), 38: (21685, 21686, 'Vr', 'Vrykul'),
    39: (21780, 21781, 'Tu', 'Tuskarr'), 40: (21963, 21964, 'Ft', 'ForestTroll'), 41: (26316, 26317, 'Wt', 'Taunka'),
    43: (26873, 26874, 'It', 'IceTroll'), 44: (142663, 142664, 'Dk', 'DrakkariTroll'), 62: (255050, 255052, 'Dw', 'Npc_Earthen'),
    33: (141109, 141109, 'Th', 'ThinHuman'), 42: (26871, 26872, 'NS', 'NorthrendSkeleton'), 45: (142819, 142820, 'Dk', 'LeperTroll'),
    46: (142832, 142833, 'Dk', 'Troglodyte'), 63: (254500, 254501, 'Hu', 'Npc_HumanCultist'),
}


def write_conf():
    path = os.path.join(ROOT, 'conf', 'mod_custom_races.conf.dist')
    text = open(path).read()
    head = text.split('#\n#    CustomRaces.Races')[0]
    names = '\n'.join('#                     %-12s %s' % (r['key'], LOOKS_TEXT[r['looks']]) for r in POOL)
    out = head + '''#
#    CustomRaces.Races
#        Description: The races that are playable, by name, separated by spaces, for example "Goblin Worgen".
#                     The races of the module:
%s
#                     A race left out here is refused by the server; the race choice of the game client shows
#                     the races its client patch was built for (AFK Realm builds it anew after a change).
#                     None is chosen by default: each realm picks its own.
#        Default:     ""
#

CustomRaces.Races = ""
''' % names
    for r in POOL:
        if not r['id']:
            out += '''
#
#    CustomRaces.%(key)s.Id
#        Description: The number the %(name)s has on this realm: one of the free race numbers %(slots)s.
#                     Races are bits of a 32-bit mask, so only 15 races without a number of their own can be on at
#                     once. AFK Realm gives each race the next free number when it is chosen, and keeps it while the
#                     race is switched off, so that its characters get it back. 0: no number - the race is off.
#        Default:     0
#

CustomRaces.%(key)s.Id = 0
''' % dict(r, slots=', '.join(str(x) for x in SLOTS))
        out += '''
#
#    CustomRaces.%(key)s.Base
#        Description: The stock race the %(name)s leans on, by name: Human, Orc, Dwarf, NightElf, Undead, Tauren,
#                     Gnome, Troll, BloodElf or Draenei. It starts where a new character of that race starts, gets
#                     its quests, stats and racial abilities, and plays for its faction. Pick it before players make
#                     characters of the race: changed later, they keep their place but may change sides.
#        Default:     "%(base)s"
#

CustomRaces.%(key)s.Base = "%(base)s"
''' % r
    open(path, 'w').write(out)


def write_remove_sql():
    all_ids = sorted({r['id'] for r in POOL if r['id']} | set(SLOTS))
    ids = ', '.join(str(i) for i in all_ids)
    masks = ', '.join(str(bit(i)) for i in all_ids)
    world = '''-- mod-custom-races: run by AFK Realm when the module is removed (data/sql/remove). Written by tools/pool.py.
-- The module writes the rows of its races at every start, and removes those of the races that are off; these are the
-- rows of all its races, by race id and race mask.
DELETE FROM `chrraces_dbc` WHERE `ID` IN (%(ids)s);
DELETE FROM `playercreateinfo` WHERE `race` IN (%(ids)s);
DELETE FROM `playercreateinfo_action` WHERE `race` IN (%(ids)s);
DELETE FROM `playercreateinfo_item` WHERE `race` IN (%(ids)s);
DELETE FROM `player_race_stats` WHERE `Race` IN (%(ids)s);
DELETE FROM `playercreateinfo_skills` WHERE `raceMask` IN (%(masks)s);
DELETE FROM `playercreateinfo_spell_custom` WHERE `racemask` IN (%(masks)s);
DELETE FROM `ascension_custom_class_race` WHERE `race` IN (%(ids)s);
''' % dict(ids=ids, masks=masks)
    chars = '''-- mod-custom-races: run by AFK Realm when the module is removed (data/sql/remove). Written by tools/pool.py.
-- Characters of its races play a stock race from now on: the base race the module gave them (kept in
-- `custom_races_base` while it ran, by the number each race had). Their race and looks are kept aside in `custom_races_moved`;
-- the module, installed again, gives them back.
CREATE TABLE IF NOT EXISTS `custom_races_moved` (
  `guid` INT UNSIGNED NOT NULL,
  `race` TINYINT UNSIGNED NOT NULL,
  `base` TINYINT UNSIGNED NOT NULL,
  `skin` TINYINT UNSIGNED NOT NULL,
  `face` TINYINT UNSIGNED NOT NULL,
  `hairStyle` TINYINT UNSIGNED NOT NULL,
  `hairColor` TINYINT UNSIGNED NOT NULL,
  `facialStyle` TINYINT UNSIGNED NOT NULL,
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod-custom-races: characters of a race switched off, and their looks';

CREATE TABLE IF NOT EXISTS `custom_races_base` (
  `race` TINYINT UNSIGNED NOT NULL,
  `base` TINYINT UNSIGNED NOT NULL,
  PRIMARY KEY (`race`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod-custom-races: the base race of each race';
'''
    chars += '''
REPLACE INTO `custom_races_moved` (`guid`, `race`, `base`, `skin`, `face`, `hairStyle`, `hairColor`, `facialStyle`)
SELECT c.`guid`, c.`race`, b.`base`, c.`skin`, c.`face`, c.`hairStyle`, c.`hairColor`, c.`facialStyle`
FROM `characters` c JOIN `custom_races_base` b ON b.`race` = c.`race`;
UPDATE `characters` c JOIN `custom_races_base` b ON b.`race` = c.`race` SET c.`race` = b.`base`, c.`skin` = 0, c.`face` = 0,
  c.`hairStyle` = 0, c.`hairColor` = 0, c.`facialStyle` = 0;
'''
    open(os.path.join(ROOT, 'data', 'sql', 'remove', 'db-world', 'custom_races_remove.sql'), 'w').write(world)
    open(os.path.join(ROOT, 'data', 'sql', 'remove', 'db-characters', 'custom_races_remove.sql'), 'w').write(chars)


# ------------------------------------------------------------------------------------------------- client

def on(r):
    when = [{'conf': CONF, 'key': 'CustomRaces.Races', 'default': '', 'has': r['key']}]
    if not r['id']:
        when.append({'conf': CONF, 'key': 'CustomRaces.%s.Id' % r['key'], 'default': '0', 'in': [str(x) for x in SLOTS]})
    return when


def rid(r):
    """The race's number: its own, or the one the realm gave it (a setting AFK Realm reads when it builds the patch)."""
    return r['id'] or {'setting': 'CustomRaces.%s.Id' % r['key'], 'conf': CONF, 'default': 0}


def based(r, names):
    return {'conf': CONF, 'key': 'CustomRaces.%s.Base' % r['key'], 'default': r['base'], 'in': names}


def client_edits():
    edits = [{'file': 'Interface\\GlueXML\\GlueXML.toc', 'insertAfter': 'CharacterCreate.xml', 'line': 'CustomRaces.lua'}]
    for r in POOL:
        new_id = r['id'] != r['source']
        target = rid(r)
        # The race's row: playable, for the faction of its base race.
        for base in BASES:
            bid, bname, faction, lang, side, sound, _ = base
            values = {'1': 12, '2': faction, '3': sound, '7': lang, '13': side}
            when = on(r) + [based(r, [bname])]
            if new_id:
                # A new number: the row of its looks, under its own name (enUS, female, male).
                values.update({'0': target, '14': r['name'], '31': r['name'], '48': r['name']})
                edits.append({'file': 'DBFilesClient\\ChrRaces.dbc', 'when': when,
                              'copyRows': {'match': {'0': r['source']}, 'set': values, 'key': [0]}})
            else:
                edits.append({'file': 'DBFilesClient\\ChrRaces.dbc', 'when': when,
                              'setRows': {'match': {'0': r['source']}, 'set': values}})
        # Its classes.
        edits.append({'file': 'DBFilesClient\\CharBaseInfo.dbc', 'when': on(r),
                      'addRows': {'cross': [[target], CLASSES], 'maxPerRace': 30}})
        # Its looks, copied to its new number: skins, faces, hair, beards - the plain tables and the HD ones.
        if new_id:
            for table, race_field, new_row_id in (('CharSections', 1, True), ('HDCharSections', 1, True),
                                                  ('CharHairGeosets', 1, True), ('HDCharHairGeosets', 1, True),
                                                  ('CharacterFacialHairStyles', 0, False),
                                                  ('HDCharacterFacialHairStyles', 1, True), ('CharHairTextures', 1, True)):
                spec = {'match': {str(race_field): r['source']}, 'set': {str(race_field): target}}
                if new_row_id:
                    spec['newId'] = True
                edits.append({'file': 'DBFilesClient\\%s.dbc' % table, 'when': on(r), 'copyRows': spec})
        # Names for the random name button.
        edits.append({'file': 'DBFilesClient\\NameGen.dbc', 'when': on(r),
                      'copyRows': {'match': {'2': BASE[r['names']][0]}, 'set': {'2': target}, 'newId': True}})
        # What the character creation dresses it in: the start outfits and the showcase of the classes of CoA of its
        # base race.
        for base in BASES:
            bid, bname = base[0], base[1]
            when = on(r) + [based(r, [bname])]
            edits.append({'file': 'DBFilesClient\\CharStartOutfit.dbc', 'when': when,
                          'cloneRows': {'match': {'4': bid}, 'set': {'4': target}, 'key': [4, 5, 6], 'replace': True}})
            edits.append({'file': 'DBFilesClient\\CharacterCreationClassDetails.dbc', 'when': when,
                          'cloneRows': {'match': {'8': bid}, 'set': {'8': target}, 'key': [4, 8, 12], 'replace': True}})
    return edits


def client_fills():
    fills = [{'marker': '@RACES@', 'conf': CONF, 'key': 'CustomRaces.Races', 'default': '',
              'line': 'local ACTIVE = {words} -- @RACES@'}]
    for r in POOL:
        fills.append({'marker': '@BASE_%s@' % r['key'].upper(), 'conf': CONF, 'key': 'CustomRaces.%s.Base' % r['key'],
                      'default': r['base'], 'line': 'BASES.%s = "{text}" -- @BASE_%s@' % (r['key'], r['key'].upper())})
    return fills


def write_manifest():
    path = os.path.join(ROOT, 'afk-realm.json')
    data = json.load(open(path))
    data['client'] = {
        'clearCache': True,
        'patch': {
            'files': [{'from': 'client/CustomRaces.lua', 'to': 'Interface\\GlueXML\\CustomRaces.lua', 'fill': client_fills()}],
            'edits': client_edits(),
        },
        'why': 'The race choice of the character creation shows the races chosen in CustomRaces.Races, each with the '
               'faction of its base race: the game client already carries their models, skins and hair, and the patch '
               'makes them playable for 30 classes, with names, start outfits and the showcase of the classes of '
               'Conquest of Azeroth of their base race.',
    }
    with open(path, 'w') as f:
        json.dump(data, f, indent=1)
        f.write('\n')


def lua_string(s):
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'


def write_lua():
    path = os.path.join(ROOT, 'client', 'CustomRaces.lua')
    text = open(path).read()
    lines = ['-- @POOL BEGIN (written by tools/pool.py)',
             'local ACTIVE = {} -- @RACES@',
             'local BASES = {}']
    for r in POOL:
        lines.append('BASES.%s = "%s" -- @BASE_%s@' % (r['key'], r['base'], r['key'].upper()))
    lines.append('local POOL = {')
    for r in POOL:
        p = r['portrait']
        if p[0] == 'round':
            portrait = '{ kind = "round", key = "%s" }' % p[1]
        elif p[0] == 'old':
            portrait = '{ kind = "old", male = { %s, %s, %s, %s }, female = { %s, %s, %s, %s } }' % (
                p[1], p[2], p[3], p[4], p[1], p[2], p[3] + 0.5, p[4] + 0.5)
        else:
            portrait = '{ kind = "icon", male = "Interface\\\\Icons\\\\%s", female = "Interface\\\\Icons\\\\%s" }' % (p[1], p[2])
        lines.append('\t{ key = "%s", file = "%s", name = %s, maleOnly = %s, portrait = %s,\n\t\tinfo = %s },' % (
            r['key'], DISPLAYS[r['source']][3].upper(), lua_string(r['name']), 'true' if r['looks'] == 'male' else 'false',
            portrait, lua_string(r['info'])))
    lines.append('}')
    lines.append('-- The stock races a race can lean on: name in the settings -> file name of the client.')
    lines.append('local BASE_FILES = { %s }' % ', '.join('%s = "%s"' % (b[1], b[6]) for b in BASES))
    lines.append('-- @POOL END')
    block = '\n'.join(lines)
    text = re.sub(r'-- @POOL BEGIN.*?-- @POOL END', lambda m: block, text, flags=re.S)
    open(path, 'w').write(text)


def write_races_json():
    """The pool for the setup page of AFK Realm."""
    data = {'slots': SLOTS, 'bases': [b[1] for b in BASES],
            'races': [{'key': r['key'], 'name': r['name'], 'id': r['id'], 'source': r['source'], 'base': r['base'],
                       'looks': r['looks'], 'about': r['info']} for r in POOL]}
    with open(os.path.join(ROOT, 'races.json'), 'w') as f:
        json.dump(data, f, indent=1)
        f.write('\n')


if __name__ == '__main__':
    check()
    write_races_json()
    write_header()
    write_conf()
    write_remove_sql()
    write_manifest()
    write_lua()
    print('%d races written' % len(POOL))
