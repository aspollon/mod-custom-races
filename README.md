# mod-custom-races

**English** · [Deutsch](README.de.md) · [Français](README.fr.md) · [Español](README.es.md) · [Português (Brasil)](README.pt-BR.md) · [Русский](README.ru.md) · [简体中文](README.zh-Hans.md)

More playable races for **Conquest of Azeroth** (CoA), an AzerothCore module.

The game client of Conquest of Azeroth already carries the models, skins and hair of many races no player can choose:
Goblins, Worgen, Pandaren, Vulpera, Vrykul and more. This module makes them playable. It brings no models or textures
of its own: its client patch is built from the player's own game client.

**Status: test version.** The Goblin was played from character creation into the world; the other races are built the
same way and wait for their test.

## The races

The races are not fixed by the module: they are the races of the game client. The setup page of AFK Realm reads them
from the player's own client - every race beyond the ten stock races it has a body for - and writes what the server
and the client patch need of each into `mod_custom_races.conf` (`CustomRaces.<race>.*`). After an update of the game,
opening the page again reads them anew. None is on by default: each realm chooses its own (`CustomRaces.Races`).

In the CoA client these are 22 races, and the module adds the High Elf, made from the Blood Elf:

| Shows armour, male and female | Male only (the game has no female model) | Shows no armour | Not tried yet |
|---|---|---|---|
| Goblin, Zandalari Troll, Worgen, Kul Tiran, Vulpera, Pandaren, High Elf, Earthen, Drakkari Troll | Fel Orc, Broken, Vrykul, Tuskarr, Taunka, Forest Troll, Ice Troll, Skeleton, Northrend Skeleton, Troglodyte, Thin Human | Naga | Leper Troll, Human Cultist |

A race the module knows nothing about (from another client, or a later version of this one) is offered all the same,
under "Not tried yet", leaning on the stock race its name suggests.

**Race numbers.** Races are bits of a 32-bit mask, so the game knows race numbers up to 31. A race whose row in the
client's race table is below 32 keeps that number (six in the CoA client). The others take one of the numbers no row
has (15 in the CoA client: 14, 15, 17, 18, 21-31) when they are switched on (`CustomRaces.<race>.Id`): at most that
many of them can be on at once. A race switched off keeps its number, so its characters get their race back; giving
up the number (setting it to 0) frees it for another race, and the characters of the race stay their base race for
good.

**Helmets** are models made for each race. Most races of the client have none of their own and show a placeholder
for every helmet, so their characters - players and bots - keep their helmets hidden (set when they are made and at
every login; `CustomRaces.<race>.OwnHelmets`, which the setup page sets from what the client carries). The showcase of
the character creation still shows it.

**Races from other clients.** Other game clients and repacks bring races the CoA client does not have (Void Elf,
Dark Iron Dwarf, Lightforged, Nightborne and more). Gathered into a race folder - one folder per race, with its rows in
`race.json` and its models and textures in `files\` - AFK Realm offers them on the setup page next to the races of
the client (`CustomRaces.ImportFolder`, `CustomRaces.<race>.Import = 1`). Their rows go into the client patch, their
files into an archive of their own for each race (`Data\patch-ZY<number>.MPQ`). The module ships none of these
files: the race folder is the player's own. AFK Realm makes it from any client the player has ("Create race packs from
a client"), so the big clients can go afterwards.

**More than 31 races.** The game client knows race numbers up to 31. With a 64-race patch of the client (the
`dinput8.dll` of CoA Custom, based on wxl-races-patcher by Dokman, from the original module by Furioz420) it knows up
to 63: `CustomRaces.MoreRaces = 1` and `CustomRaces.MoreRacesFile`. AFK Realm puts the DLL next to `Ascension.exe`
while this is on; the module ships no DLL. Race masks are 32 bits wide, so the core patch gives a race above 32 the bit
of its base race (`RaceMaskOf`): it may do what its base race may do. Without MoreRaces a race above 31 stays off.

**The character creation of the Ascension client** needs rules for the skins, faces, hair and underwear of every
added race, or it crashes or paints wrong textures: every row playable, a face and an underwear for every skin colour,
women's rows for races that only have men, one sorted block per race, HD rows on the ids of their plain rows, and
empty textures pointing at a NUL byte (offset 0 of the client's string block is the Human male skin). These were found
by ilusixn for CoA Custom (https://github.com/ilusixn/azerothcore-wotlk-coa); the client patch applies them to every
race of the module.

**Icons** on the race choice: the race's own icon from the race folder (`_Icons\CoA_Race_<race>.blp`), shown round.
**Placement**: the stock races keep a column at the edge of their side, the added races get columns of their own next
to it, from the top.

**Portraits** on the race choice are round like those of the stock races: an older round portrait of the game (Goblin,
Worgen), an icon of the game cut round, or the round portrait of its base race.

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
- **Looks**: its own, from the game client (the core patch lets a character wear a model above number 65535, as
  most of these races have). Races without a female model are male: the character creation offers no
  female, and the random bots of the race are made men.
- **Voice**: the voice of its base race at the emotes (`/silly`, `/joke`, `/laugh` and so on). The game client speaks
  voices for the stock races only, none for a race of the module, whatever a patch brings it - so the voice rows of its
  base race are lent (`emotetextsound_dbc`) and the server sends the sound. Read are `EmotesTextSound.dbc` and, where
  the data folder has it, the client's HD voice table `HDEmotesTextSound.dbc` (the laughter and crying lines sit
  there); the HD row is the one that counts.
- **Camera**: the character creation shows a race in the scene of its base race, with its camera; a race much taller
  than its base race (the Highmountain Tauren) stood half out of the picture. AFK Realm measures how tall the models
  stand (from their model files) and the camera goes a little back and up by how much taller the race is, in the
  choice of race and class, in the customization and in the character selection; the zoom onto the face goes less
  far.

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
| `CustomRaces.Races` | (none) | The races that are playable, by key (the English name without spaces), for example `Goblin Worgen Vrykul`. |
| `CustomRaces.<race>.Base` | by name | The stock race it leans on: start, quests, stats, racial abilities and faction. |
| `CustomRaces.<race>.Id` | its own | Its race number: its own below 32, otherwise one of the free numbers. |
| `CustomRaces.<race>.Source`, `.Copy`, `.Name`, `.Prefix`, `.File`, `.MaleDisplay`, `.FemaleDisplay`, `.MaleOnly` | from the client | Its row in the client's ChrRaces.dbc and what it holds; written by the setup page. |

## Install

**With AFK Realm** (0.9.1 or newer): *Modules*, add the module, build. *Modules › Custom races* chooses the races and
their base races and gives them their numbers; *Modules › Game client › Install* builds the client patch from your
game client and puts it into your game. Every player needs the client patch (*Export as ZIP* gives it to the others).

**By hand:** copy the module into `modules/` of the CoA core, apply `patches/core-custom-races.patch`, build, set the
options (the values of each race from the client's `ChrRaces.dbc`; the `.conf.dist` shows them for the Goblin). The client patch is an MPQ that loads after the client's own (for example `Data\patch-Y.MPQ`); what it holds
for each chosen race is described in `afk-realm.json` (`client.patch`, one set of changes done `forEach` race): the race's row of `ChrRaces.dbc` made playable for
the faction of its base race (copied to its number when it has none of its own), its classes in `CharBaseInfo.dbc`,
its skins, hair and beards copied to its number (`CharSections`, `CharHairGeosets`, `CharacterFacialHairStyles`,
`CharHairTextures` and their HD tables), names in `NameGen.dbc`, and the outfits of its base race
(`CharStartOutfit.dbc`, `CharacterCreationClassDetails.dbc`), and `client/CustomRaces.lua` after `CharacterCreate.xml`
in `Interface\GlueXML\GlueXML.toc`.

## For the module's makers

What the module knows beyond the client is kept in one table, `tools/races.py`: the base race that fits, how
a race looks in game, a rating, a better portrait, and races made from another (the High Elf). Running it writes
`races.json` (for the setup page of AFK Realm), the client patch in `afk-realm.json`, the part of
`client/CustomRaces.lua` between `@KNOWN BEGIN` and `@KNOWN END`, and `data/sql/remove`.

## Status

Test version. Compiled against the CoA core of 10 October 2026 together with its patch; the client patch built from
the CoA client of the same date.

## Languages

`lang/<code>.json` carries the texts AFK Realm shows of this module (its option descriptions, the names and reasons of `afk-realm.json`, the race names) in German, French, Spanish, Portuguese (Brazil), Russian and Simplified Chinese. AFK Realm's `tools/lang.py --module <folder> check` lists what a change left untranslated.
