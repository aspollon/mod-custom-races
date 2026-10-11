# mod-custom-races

More playable races for **Conquest of Azeroth** (CoA), an AzerothCore module.

The game client of Conquest of Azeroth already carries the models, skins and hair of many races no player can choose:
Goblins, Worgen, Pandaren, Vulpera, Vrykul and more. This module makes them playable. It brings no models or textures
of its own: its client patch is built from the player's own game client.

**Status: test version.** The Goblin was played from character creation into the world; the other races are built the
same way and wait for their test.

## The races

None is on by default: each realm chooses its own (`CustomRaces.Races`, or the setup page of AFK Realm).

| Shows armour, male and female | Male only (the game has no female model) | Shows no armour | Not tried yet |
|---|---|---|---|
| Goblin, Zandalari Troll, Worgen, Kul Tiran, Vulpera, Pandaren, High Elf, Earthen, Drakkari Troll | Fel Orc, Broken, Vrykul, Tuskarr, Taunka, Forest Troll, Ice Troll, Skeleton, Northrend Skeleton, Troglodyte, Thin Human | Naga | Leper Troll, Human Cultist |

**Race numbers.** Races are bits of a 32-bit mask, so the game knows race numbers up to 31. Goblin, Zandalari Troll,
Worgen, Kul Tiran, Vulpera and Pandaren have a number of their own. The others take one of the 15 free numbers (14,
15, 17, 18, 21-31) when they are switched on (`CustomRaces.<race>.Id`): at most 15 of them can be on at once. A race
switched off keeps its number, so its characters get their race back; giving up the number (setting it to 0) frees it
for another race, and the characters of the race stay their base race for good.

## What a new race is

Each race leans on a stock race, its **base race** (`CustomRaces.<race>.Base`, any of the ten stock races):

- **Start**: where a new character of its base race starts, with its first items and action bars.
- **Faction**: that of its base race. A Goblin leaning on the Human plays for the Alliance.
- **Stats and racial abilities**: those of its base race, and the racial abilities of CoA its base race gets.
- **Quests, items, reputations and skills**: whatever its base race may do, the new race may do too. This is lent in
  memory when the server starts; the world database is not changed for it.
- **Outfits**: the start outfits of its base race, and in the character creation the showcase of the classes of CoA.
- **Classes**: the 21 classes of CoA and the nine classic classes of mod-classic-classes - 30, the most the character
  creation takes for a race.
- **Looks**: its own, from the game client. Races without a female model are male: the character creation offers no
  female, and the random bots of the race are made men.

The random bots of mod-playerbots are made in the new races too: they take a race from the races the server offers,
and their side from the core's race table.

## Everything can be taken back

- **A race switched off** (or the whole module, `CustomRaces.Enable = 0`): at its next start the server removes the
  rows of that race. Its characters - players and bots - play their base race until the race is on again; their race
  and looks are kept aside in the table `custom_races_moved` of the character database, and they get them back when
  the race is switched on again.
- **The module removed** through AFK Realm: AFK Realm runs `data/sql/remove`, which does the same for all races of the
  module, and undoes the module's patch with the next build.
- **The game client**: the client patch only adds files to the patch AFK Realm builds (`Data\patch-Y.MPQ`); the
  client itself is never changed. Built again without the module, the patch no longer has the races.

## Settings (`mod_custom_races.conf`)

| Option | Default | |
|---|---|---|
| `CustomRaces.Enable` | 1 | The races of `CustomRaces.Races` can be created and played. |
| `CustomRaces.Races` | (none) | The races that are playable, by name, for example `Goblin Worgen Vrykul`. |
| `CustomRaces.<race>.Base` | per race | The stock race it leans on: start, quests, stats, racial abilities and faction. |
| `CustomRaces.<race>.Id` | 0 | For a race without a number of its own: its race number, one of the free numbers. |

## Install

**With AFK Realm** (0.9.1 or newer): *Modules*, add the module, build. *Modules › Custom races* chooses the races and
their base races and gives them their numbers; *Modules › Game client › Install* builds the client patch from your
game client and puts it into your game. Every player needs the client patch (*Export as ZIP* gives it to the others).

**By hand:** copy the module into `modules/` of the CoA core, apply `patches/core-custom-races.patch`, build, set the
options. The client patch is an MPQ that loads after the client's own (for example `Data\patch-Y.MPQ`); what it holds
for each race is described in `afk-realm.json` (`client.patch`): the race's row of `ChrRaces.dbc` made playable for
the faction of its base race (copied to its number when it has none of its own), its classes in `CharBaseInfo.dbc`,
its skins, hair and beards copied to its number (`CharSections`, `CharHairGeosets`, `CharacterFacialHairStyles`,
`CharHairTextures` and their HD tables), names in `NameGen.dbc`, and the outfits of its base race
(`CharStartOutfit.dbc`, `CharacterCreationClassDetails.dbc`), and `client/CustomRaces.lua` after `CharacterCreate.xml`
in `Interface\GlueXML\GlueXML.toc`.

## For the module's makers

The races are kept in one table, `tools/pool.py`. Running it writes `src/RacePool.h`, `races.json` (for the setup page
of AFK Realm), the settings in `conf/`, the client patch in `afk-realm.json`, the pool part of `client/CustomRaces.lua`
and `data/sql/remove`.

## Status

Test version. Compiled against the CoA core of 10 October 2026 together with its patch; the client patch built from
the CoA client of the same date.
