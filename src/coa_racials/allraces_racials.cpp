// Racials of CoA Custom 1.9.9's AllRaces v12 races whose working needs code. CoA Custom's own code for them is not
// published (its worldserver.exe has it), so these two are written anew from RACIALS.md and its patch notes.

#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"

// Titan, Aegis of the Keepers (15000914): the ward absorbs as much as the Titan's full health (patch notes 1.9.9).
class spell_titan_aegis : public AuraScript
{
    PrepareAuraScript(spell_titan_aegis);

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& canBeRecalculated)
    {
        canBeRecalculated = false;
        if (Unit* caster = GetCaster())
            amount = int32(caster->GetMaxHealth());
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_titan_aegis::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
    }
};

// Fungarian, Sporebloom Pulse (15000935): the planted Sporebloom casts it every 2 seconds; everyone friendly to its
// planter within the spell's radius is healed for 8% of their health.
class spell_fungarian_sporebloom_pulse : public SpellScript
{
    PrepareSpellScript(spell_fungarian_sporebloom_pulse);

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* bloom = GetCaster();
        if (!bloom)
            return;
        Unit* planter = bloom->GetOwner() ? bloom->GetOwner() : bloom;

        float radius = GetSpellInfo()->Effects[EFFECT_1].CalcRadius();
        if (radius <= 0.0f)
            radius = 8.0f;

        std::list<Player*> players;
        Acore::AnyPlayerInObjectRangeCheck check(bloom, radius);
        Acore::PlayerListSearcher<Acore::AnyPlayerInObjectRangeCheck> searcher(bloom, players, check);
        Cell::VisitObjects(bloom, searcher, radius);
        for (Player* player : players)
            if (player->IsAlive() && planter->IsFriendlyTo(player))
            {
                HealInfo heal(bloom, player, player->CountPctFromMaxHealth(8), GetSpellInfo(), GetSpellInfo()->GetSchoolMask());
                bloom->HealBySpell(heal);
            }
    }

    void Register() override
    {
        OnEffectHit += SpellEffectFn(spell_fungarian_sporebloom_pulse::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

void AddSC_allraces_racials()
{
    RegisterSpellScript(spell_titan_aegis);
    RegisterSpellScript(spell_fungarian_sporebloom_pulse);
}
