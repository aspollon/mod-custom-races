#!/usr/bin/env python3
"""
mod-custom-races: what the module knows about races beyond the game client.

The races themselves come from the game client: AFK Realm reads its race table, finds the races with a player model
and writes what the server needs of each into mod_custom_races.conf (CustomRaces.<race>.Source, .Id, .Name, .Prefix,
.File, .MaleDisplay, .FemaleDisplay, .MaleOnly, .Base, .Copy). This table only adds what the client does not say: a
better portrait, the base race that fits, a rating - and races made from another (the High Elf, from the Blood
Elf). A race of the client that is not in it is offered all the same.

Writes:
  races.json                         what the setup page of AFK Realm knows of each race
  afk-realm.json                     the client patch: one set of changes, done for each race the realm chose
  client/CustomRaces.lua             the part between "-- @KNOWN BEGIN" and "-- @KNOWN END"
  data/sql/remove/*.sql              what removing the module takes back

Run it after changing the table: python3 tools/races.py
"""
import json, os, re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CONF = 'mod_custom_races.conf'

# The stock races a race can lean on: id, name in the settings, faction template, language, side (0 Alliance,
# 1 Horde), exploration sound, file name of the client.
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

# The classes the character creation offers each race: the classic classes (with mod-classic-classes) and the 21
# classes of Conquest of Azeroth - 30, the most the character creation takes for a race.
CLASSES = [1, 2, 3, 4, 5, 7, 8, 9, 11] + list(range(12, 33))

# Race numbers a race without one of its own can get: the free numbers of the CoA client, up to 31 (races are bits of
# a 32-bit mask). AFK Realm gives out the ones its client has free; this is the list for the CoA client.
SLOTS = [14, 15, 17, 18] + list(range(21, 32))

# What is known of races, by their key (their English name without spaces). looks: "armor" shows armour, male and
# female; "male" only a male model; "noarmor" no armour; "untested" not tried. rating: "" or "poor" (a warning on the
# setup page, with "note"). portrait: ("old", left, right, top, bottom of the male portrait in
# UI-CHARACTERCREATE-RACESROUND; the female one is two rows lower), ("icon", male icon, female icon - shown round), or
# none: the round portrait of its base race. virtual: a race made from the row of another (source) under its own name.
KNOWN = [
    dict(key='Goblin', base='Orc', looks='armor', portrait=('old', 0.625, 0.75, 0.25, 0.5)),
    dict(key='ZandalariTroll', base='Troll', looks='armor',
         portrait=('icon', 'achievement_alliedrace_zandalaritroll', 'achievement_alliedrace_zandalaritroll')),
    dict(key='Worgen', base='Human', looks='armor', portrait=('old', 0.625, 0.75, 0.0, 0.25)),
    dict(key='KulTiran', base='Human', looks='armor',
         portrait=('icon', 'achievement_alliedrace_kultiranhuman', 'achievement_alliedrace_kultiranhuman')),
    dict(key='Vulpera', base='Tauren', looks='armor',
         portrait=('icon', 'achievement_alliedrace_vulpera', 'achievement_alliedrace_vulpera')),
    dict(key='VulperaEsteria', base='Draenei', looks='armor',
         portrait=('icon', 'achievement_alliedrace_vulpera', 'achievement_alliedrace_vulpera')),
    dict(key='Pandaren', base='Human', looks='armor',
         portrait=('icon', 'achievement_character_pandaren_female', 'achievement_character_pandaren_female')),
    dict(key='HighElf', virtual=dict(source=10, name='High Elf'), base='Human', looks='armor',
         portrait=('round', 'BLOODELF')),
    dict(key='FelOrc', base='Orc', looks='male',
         portrait=('icon', 'achievement_boss_hellfire_felorccouncil', 'achievement_boss_hellfire_felorccouncil')),
    dict(key='Broken', base='Draenei', looks='male',
         portrait=('icon', 'achievement_boss_triumvirate_voidbrokenbrute', 'achievement_boss_triumvirate_voidbrokenbrute')),
    dict(key='Earthen', base='Dwarf', looks='armor'),
    dict(key='DrakkariTroll', base='Troll', looks='armor'),
    dict(key='Vrykul', base='Human', looks='male', portrait=('icon', 'inv_misc_head_vrykul', 'inv_misc_head_vrykul')),
    dict(key='Tuskarr', base='Dwarf', looks='male', portrait=('icon', 'inv_misc_head_tuskarr', 'inv_misc_head_tuskarr')),
    dict(key='Taunka', base='Tauren', looks='male'),
    dict(key='ForestTroll', base='Troll', looks='male'),
    dict(key='IceTroll', base='Troll', looks='male'),
    dict(key='Skeleton', base='Undead', looks='male'),
    dict(key='Naga', base='Undead', looks='noarmor', portrait=('icon', 'achievement_boss_nagazone', 'achievement_boss_nagazone')),
    dict(key='NorthrendSkeleton', base='Undead', looks='male'),
    dict(key='LeperTroll', base='Troll', looks='untested'),
    dict(key='Troglodyte', base='Dwarf', looks='male', portrait=('icon', 'inv_misc_head_kobold_01', 'inv_misc_head_kobold_01')),
    dict(key='ThinHuman', base='Human', looks='male'),
    dict(key='HumanCultist', base='Human', looks='untested'),
]

# Every race the module could have given a number to: those with one of their own in the CoA client below 32 that are
# not stock races, and the free numbers. Removing the module takes back the rows of all of them.
OWN = [9, 12, 13, 16, 19, 20, 74]


def bit(race):
    return 1 << (race - 1)


# ------------------------------------------------------------------------------------------------- setup page

def write_races_json():
    known = {}
    for r in KNOWN:
        entry = {'base': r['base'], 'looks': r['looks'], 'rating': r.get('rating', ''), 'note': r.get('note', '')}
        if 'virtual' in r:
            entry['virtual'] = r['virtual']
        known[r['key']] = entry
    # Keep what the setup page does not generate (the CoA Custom racial table and other hand-maintained data).
    path = os.path.join(ROOT, 'races.json')
    data = {}
    if os.path.exists(path):
        with open(path) as f:
            data = json.load(f)
    data['slots'] = SLOTS
    data['bases'] = [b[1] for b in BASES]
    data['known'] = known
    with open(path, 'w') as f:
        json.dump(data, f, indent=1)
        f.write('\n')


# ------------------------------------------------------------------------------------------------- client patch

def setting(name, text=False, default=0):
    value = {'setting': 'CustomRaces.{each}.' + name, 'conf': CONF, 'default': default}
    if text:
        value['text'] = True
        value['default'] = ''
    return value


def cond(name, **test):
    c = {'conf': CONF, 'key': 'CustomRaces.{each}.' + name, 'default': ''}
    c.update(test)
    return c


EACH = {'conf': CONF, 'key': 'CustomRaces.Races', 'default': ''}
NUMBERED = cond('Id', notIn=['', '0'])


def client_edits():
    edits = [{'file': 'Interface\\GlueXML\\GlueXML.toc', 'insertAfter': 'CharacterCreate.xml', 'line': 'CustomRaces.lua'}]
    race_id, source = setting('Id'), setting('Source')

    def each(edit, *conditions):
        edit['forEach'] = EACH
        edit['when'] = [NUMBERED] + list(conditions)
        edits.append(edit)

    for bid, bname, faction, lang, side, sound, _ in BASES:
        values = {'1': 12, '2': faction, '3': sound, '7': lang, '13': side}
        # A race with a row of its own: playable for the faction of its base race.
        each({'file': 'DBFilesClient\\ChrRaces.dbc', 'setRows': {'match': {'0': source}, 'set': values}},
             cond('Copy', **{'is': '0'}), cond('Import', notIn=['1']), cond('Base', **{'in': [bname]}))
        # A race with a new number: the row of its looks copied there, under its own name.
        copied = dict(values)
        copied.update({'0': race_id, '14': setting('Name', True), '31': setting('Name', True), '48': setting('Name', True)})
        each({'file': 'DBFilesClient\\ChrRaces.dbc', 'copyRows': {'match': {'0': source}, 'set': copied, 'key': [0]}},
             cond('Copy', **{'is': '1'}), cond('Base', **{'in': [bname]}))
        # Names of its base race for the random name button, and what the character creation dresses it in.
        each({'file': 'DBFilesClient\\NameGen.dbc', 'copyRows': {'match': {'2': bid}, 'set': {'2': race_id}, 'newId': True}},
             cond('Base', **{'in': [bname]}))
        each({'file': 'DBFilesClient\\CharStartOutfit.dbc',
              'cloneRows': {'match': {'4': bid}, 'set': {'4': race_id}, 'key': [4, 5, 6], 'replace': True}},
             cond('Base', **{'in': [bname]}))
        each({'file': 'DBFilesClient\\CharacterCreationClassDetails.dbc',
              'cloneRows': {'match': {'8': bid}, 'set': {'8': race_id}, 'key': [4, 8, 12], 'replace': True}},
             cond('Base', **{'in': [bname]}))
    # A race with a row of its own but no helmet models of its own (Import = 0): its ChrRaces row names the helmet
    # prefix of another race (CustomRaces.<race>.HelmetPrefix), as CoA Custom does it.
    each({'file': 'DBFilesClient\\ChrRaces.dbc',
          'setRows': {'match': {'0': race_id}, 'set': {'6': setting('HelmetPrefix', True)}}},
         cond('Import', notIn=['1'], default='0'), cond('HelmetPrefix', notIn=['']))
    # A race from another client (CustomRaces.<race>.Import = 1): its rows from the race folder, its files in an archive
    # of their own (AFK Realm, RaceImport).
    each({'file': 'DBFilesClient\\ChrRaces.dbc', 'importRace': {'folder': {'setting': 'CustomRaces.ImportFolder', 'conf': CONF, 'default': '', 'text': True},
          'key': '{each}', 'id': race_id, 'base': setting('Base', True), 'name': setting('Name', True),
          'helmetPrefix': setting('HelmetPrefix', True)}}, cond('Import', **{'is': '1'}))
    # Its icon, from the race folder (_Icons), when the setup page found one.
    each({'file': 'Interface\\Icons\\CoA_Race_{each}.blp', 'addFile': {'from': {'setting': 'CustomRaces.{each}.IconFile', 'conf': CONF, 'default': '', 'text': True}}},
         cond('IconFile', notIn=['']))
    # Its classes.
    each({'file': 'DBFilesClient\\CharBaseInfo.dbc', 'addRows': {'cross': [[race_id], CLASSES], 'maxPerRace': 30}})
    # Its looks copied to its new number: skins, faces, hair and beards, in the plain and the HD tables.
    for table, field, new_row_id in (('CharSections', 1, True), ('HDCharSections', 1, True), ('CharHairGeosets', 1, True),
                                     ('HDCharHairGeosets', 1, True), ('CharacterFacialHairStyles', 0, False),
                                     ('HDCharacterFacialHairStyles', 1, True), ('CharHairTextures', 1, True)):
        spec = {'match': {str(field): source}, 'set': {str(field): race_id}}
        if table.replace('HD', '') in ('CharSections', 'CharHairGeosets'):
            # The HD table pairs with the plain one by id: a copied row and its HD copy need the same new id.
            spec['idGroup'] = table.replace('HD', '')
            spec['idKey'] = race_id
        elif new_row_id:
            spec['newId'] = True
        each({'file': 'DBFilesClient\\%s.dbc' % table, 'copyRows': spec}, cond('Copy', **{'is': '1'}))
    # The rules of the character creation for the rows of every added race (AFK Realm, RaceImport.SectionRules): last.
    each({'file': 'DBFilesClient\\CharSections.dbc', 'sectionRules': {'race': race_id, 'maleOnly': setting('MaleOnly')}})
    return edits


def client_fills():
    line = ('RACES[#RACES + 1] = { key = "{each}", name = "{conf:CustomRaces.{each}.Name}", file = "{conf:CustomRaces.{each}.File}", '
            'base = "{conf:CustomRaces.{each}.Base}", maleOnly = "{conf:CustomRaces.{each}.MaleOnly}" == "1", '
            'icon = "{conf:CustomRaces.{each}.Icon}", racials = "{conf:CustomRaces.{each}.Racials}", '
            'id = "{conf:CustomRaces.{each}.Id}", height = { "{height:{conf:CustomRaces.{each}.Id}:0}", '
            '"{height:{conf:CustomRaces.{each}.Id}:1}" } } -- @RACES@')
    return [{'marker': '@RACES@', 'forEach': EACH, 'line': line}]


def write_manifest():
    path = os.path.join(ROOT, 'afk-realm.json')
    data = json.load(open(path))
    data['client'] = {
        'addons': ['client/AddOns/AFKHideGear'],
        'clearCache': True,
        'patch': {
            'files': [
                {'from': 'client/CustomRaces.lua', 'to': 'Interface\\GlueXML\\CustomRaces.lua', 'fill': client_fills()},
            ],
            'edits': client_edits(),
            # The 64-race patch of the game client (race numbers above 31): a DLL of the user's own, from CoA Custom,
            # which AFK Realm puts next to Ascension.exe while CustomRaces.MoreRaces is on.
            'rootFiles': [
                {'to': 'dinput8.dll',
                 'from': {'setting': 'CustomRaces.MoreRacesFile', 'conf': CONF, 'default': '', 'text': True},
                 'when': [{'conf': CONF, 'key': 'CustomRaces.MoreRaces', 'is': '1', 'default': '0'}]},
                {'to': '', 'gameExe': True,
                 'from': {'setting': 'CustomRaces.ClientExe', 'conf': CONF, 'default': '', 'text': True},
                 'when': [{'conf': CONF, 'key': 'CustomRaces.ClientExe', 'default': '', 'notIn': ['']}]},
            ],
            # Files next to the game .exe that a race needs as a whole (the Esteria appearance files, for example).
            'rootFolders': [{'from': {'setting': 'CustomRaces.ClientRootFolder', 'conf': CONF, 'default': '', 'text': True},
                             'zip': True}],
        },
        'why': 'The race choice of the character creation shows the races chosen on the setup page (CustomRaces.Races), '
               'each with the faction of its base race: the game client already carries their models, skins and hair, '
               'and the patch makes them playable for 30 classes, with names, start outfits and the showcase of the '
               'classes of Conquest of Azeroth of their base race.',
    }
    with open(path, 'w') as f:
        json.dump(data, f, indent=1)
        f.write('\n')


def lua_string(s):
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'


def write_lua():
    path = os.path.join(ROOT, 'client', 'CustomRaces.lua')
    text = open(path).read()
    lines = ['-- @KNOWN BEGIN (written by tools/races.py)', 'local KNOWN = {']
    for r in KNOWN:
        p = r.get('portrait')
        if not p:
            portrait = 'nil'
        elif p[0] == 'round':
            portrait = '{ kind = "round", key = "%s" }' % p[1]
        elif p[0] == 'old':
            portrait = '{ kind = "old", male = { %s, %s, %s, %s }, female = { %s, %s, %s, %s } }' % (
                p[1], p[2], p[3], p[4], p[1], p[2], p[3] + 0.5, p[4] + 0.5)
        else:
            portrait = '{ kind = "icon", male = "Interface\\\\Icons\\\\%s", female = "Interface\\\\Icons\\\\%s" }' % (p[1], p[2])
        lines.append('\t%s = { portrait = %s },' % (r['key'], portrait))
    lines.append('}')
    lines.append('-- The stock races a race can lean on: name in the settings -> file name of the client.')
    lines.append('local BASE_FILES = { %s }' % ', '.join('%s = "%s"' % (b[1], b[6]) for b in BASES))
    lines.append('-- @KNOWN END')
    text = re.sub(r'-- @KNOWN BEGIN.*?-- @KNOWN END', lambda m: '\n'.join(lines), text, flags=re.S)
    open(path, 'w').write(text)


# ------------------------------------------------------------------------------------------------- removing

def write_remove_sql():
    # and the numbers above 31 (with the 64-race patch of the game client); those have no bit of their own
    ids = sorted(set(OWN) | set(SLOTS) | set(range(32, 64)))
    world = '''-- mod-custom-races: run by AFK Realm when the module is removed (data/sql/remove). Written by tools/races.py.
-- The module writes the rows of its races at every start, and removes those of the races that are off; these are the
-- rows of every race number it could have used, by race id and race mask.
DELETE FROM `chrraces_dbc` WHERE `ID` IN (%(ids)s);
DELETE FROM `playercreateinfo` WHERE `race` IN (%(ids)s);
DELETE FROM `playercreateinfo_action` WHERE `race` IN (%(ids)s);
DELETE FROM `playercreateinfo_item` WHERE `race` IN (%(ids)s);
DELETE FROM `player_race_stats` WHERE `Race` IN (%(ids)s);
DELETE FROM `player_totem_model` WHERE `RaceID` IN (%(ids)s);
DELETE FROM `playercreateinfo_skills` WHERE `raceMask` IN (%(masks)s);
DELETE FROM `playercreateinfo_spell_custom` WHERE `racemask` IN (%(masks)s);
DELETE FROM `ascension_custom_class_race` WHERE `race` IN (%(ids)s);
DELETE FROM `emotetextsound_dbc` WHERE `RaceId` IN (%(ids)s);
''' % dict(ids=', '.join(map(str, ids)), masks=', '.join(str(bit(i)) for i in ids if i <= 32))
    chars = '''-- mod-custom-races: run by AFK Realm when the module is removed (data/sql/remove). Written by tools/races.py.
-- Characters of its races play a stock race from now on: the base race the module gave them (kept in
-- `custom_races_base` while it ran, by the number each race had). Their race and looks are kept aside in
-- `custom_races_moved`; the module, installed again, gives them back.
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

REPLACE INTO `custom_races_moved` (`guid`, `race`, `base`, `skin`, `face`, `hairStyle`, `hairColor`, `facialStyle`)
SELECT c.`guid`, c.`race`, b.`base`, c.`skin`, c.`face`, c.`hairStyle`, c.`hairColor`, c.`facialStyle`
FROM `characters` c JOIN `custom_races_base` b ON b.`race` = c.`race`;
UPDATE `characters` c JOIN `custom_races_base` b ON b.`race` = c.`race` SET c.`race` = b.`base`, c.`skin` = 0, c.`face` = 0,
  c.`hairStyle` = 0, c.`hairColor` = 0, c.`facialStyle` = 0;
'''
    open(os.path.join(ROOT, 'data', 'sql', 'remove', 'db-world', 'custom_races_remove.sql'), 'w').write(world)
    open(os.path.join(ROOT, 'data', 'sql', 'remove', 'db-characters', 'custom_races_remove.sql'), 'w').write(chars)


if __name__ == '__main__':
    write_races_json()
    write_manifest()
    write_lua()
    write_remove_sql()
    print('%d known races written' % len(KNOWN))
