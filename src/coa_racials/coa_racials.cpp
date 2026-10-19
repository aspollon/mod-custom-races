/*
 * The racial abilities of CoA Custom (github.com/ilusixn/azerothcore-wotlk-coa, branch coa-custom-1.8,
 * src/server/coa/EsteriaRacials), taken over for mod-custom-races. The files of this folder are theirs with as few
 * changes as possible, so that a newer CoA Custom can be taken over the same way:
 *   - CoaRacialSync.cpp is left out: which racials a race has comes from CustomRaces.<race>.Racials, and the module
 *     teaches them at login.
 *   - Drakkin, Drakonid and Darkfallen are found by the racials they know instead of by CoA Custom's race numbers,
 *     which ours are not.
 * The rows of the spells, their spell_script_names and spell_proc rows come from data/coa-custom/racials.sql
 * (CoA Custom's files/sql/10_esteria_racials_1.8.sql): AFK Realm writes them into the file of each race that is on.
 * Released under GNU AGPL v3, as CoA Custom: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

void AddSC_race_racials();
void AddSC_drakkin_racials();
void AddSC_drakonid_racials();
void AddSC_darkfallen_racials();
void AddSC_broken_racials();

void AddSC_coa_custom_racials()
{
    AddSC_race_racials();
    AddSC_drakkin_racials();
    AddSC_drakonid_racials();
    AddSC_darkfallen_racials();
    AddSC_broken_racials();
}
