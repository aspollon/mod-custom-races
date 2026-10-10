# mod-custom-races

More playable races for **Conquest of Azeroth** (CoA), an AzerothCore module.

The game client of Conquest of Azeroth already carries the models, skins and hair of many races no player can choose:
Goblins, Worgen, Pandaren, Vulpera and more. This module makes them playable. It brings no models or textures of its
own: its client patch is built from the player's own game client.

**Status: proof of concept with one race, the Goblin.** More races follow once the Goblin works from character
creation to the open world.

## What a new race is

Each new race leans on a stock race of its faction, its **base race**:

| Race | Faction | Base race | Classes |
|---|---|---|---|
| Goblin | Horde | Orc | those of the Orc: the classes of CoA, and the classic classes with mod-classic-classes |

- **Start**: where its base race starts, with its first items and action bars.
- **Stats and racial abilities**: those of its base race, and the racial abilities of CoA its base race gets.
- **Outfits**: the start outfits of its base race, in the character creation and in the game.
- **Quests, items, reputations and skills**: whatever its base race may do, the new race may do too. This is lent in
  memory when the server starts; the world database is not changed for it.
- **Looks**: its own, from the game client.

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
| `CustomRaces.Races` | (none) | The races that are playable, by name. None is chosen by default: each realm picks its own. |

## Install

**With AFK Realm** (0.9.1 or newer): *Modules*, add the module's address, build. AFK Realm applies the patch, and
*Modules › Game client › Install* builds the client patch from your game client and puts it into your game. Every
player needs the client patch (*Export as ZIP* gives it to the others).

**By hand:**

1. Copy the module into `modules/` of the CoA core and apply `patches/core-custom-races.patch` to the core.
2. Build the server. The module writes its rows at the start of the worldserver.
3. The client: an MPQ patch that loads after the client's own (for example `Data\patch-Y.MPQ`) with
   - `Interface\GlueXML\CustomRaces.lua` from `client/`,
   - `Interface\GlueXML\GlueXML.toc` of your client with the line `CustomRaces.lua` after `CharacterCreate.xml`,
   - `DBFilesClient\ChrRaces.dbc` of your client, the Goblin's row (9) playable for the Horde: flags 12, faction
     template 2, exploration sound 4141, language 1, side 1,
   - `DBFilesClient\CharBaseInfo.dbc` of your client with a row for the Goblin and each class of the Orc (two bytes a
     row: race, class). The character creation takes at most 30 classes per race,
   - `DBFilesClient\CharStartOutfit.dbc` of your client with the outfits of the Orc for the Goblin.

## Status

Test version. Compiled against the CoA core of 10 October 2026 together with its patch; the client patch built from
the CoA client of the same date. Not yet played.
