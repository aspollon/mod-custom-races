/*
 * mod-custom-races - more playable races for Conquest of Azeroth.
 * Released under GNU AGPL v3: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 *
 * The core calls Add<folder>Scripts, with the dashes of the folder name as underscores.
 */

void AddSC_custom_races();
void AddSC_coa_custom_racials();
void AddSC_hide_armor();
void AddSC_fading_island();

void Addmod_custom_racesScripts()
{
    AddSC_custom_races();
    AddSC_coa_custom_racials();
    AddSC_hide_armor();
    AddSC_fading_island();
}
