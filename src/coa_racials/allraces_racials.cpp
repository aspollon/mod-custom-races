// Racials of CoA Custom's newer races (1.9.9 AllRaces v12, V2 Gilgoblin) whose working needs code. CoA Custom's own
// code for them is not published (its worldserver.exe has it), so these are written anew from their tooltips.

#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"

#include <algorithm>
#include <list>
#include <set>

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

// Gilgoblin (race 105, CoA Custom V2): three racials whose code is only in CoA Custom's own worldserver.exe, written anew
// from their tooltips. Current Rider and Brine Mending are passive dummy auras (226) without a period: they get a tick
// of one second here.
enum GilgoblinSpells
{
    SPELL_GILGOBLIN_SWIM_SPEED   = 15001056,  // Current Rider: swim speed +50%
    SPELL_GILGOBLIN_CASCADE_HEAL = 15001057,
    SPELL_GILGOBLIN_CASCADE_HIT  = 15001058,  // Frost
};

// Current Rider (15001051): swim speed +50% while out of combat.
class spell_gilgoblin_current_rider : public AuraScript
{
    PrepareAuraScript(spell_gilgoblin_current_rider);

    void CalcPeriodic(AuraEffect const* /*aurEff*/, bool& isPeriodic, int32& amplitude)
    {
        isPeriodic = true;
        amplitude = 1000;
    }

    void Tick(AuraEffect const* /*aurEff*/)
    {
        Unit* owner = GetUnitOwner();
        if (owner->IsInCombat())
            owner->RemoveAurasDueToSpell(SPELL_GILGOBLIN_SWIM_SPEED);
        else if (!owner->HasAura(SPELL_GILGOBLIN_SWIM_SPEED))
            owner->CastSpell(owner, SPELL_GILGOBLIN_SWIM_SPEED, true);
    }

    void Removed(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        GetUnitOwner()->RemoveAurasDueToSpell(SPELL_GILGOBLIN_SWIM_SPEED);
    }

    void Register() override
    {
        DoEffectCalcPeriodic += AuraEffectCalcPeriodicFn(spell_gilgoblin_current_rider::CalcPeriodic, EFFECT_0, SPELL_AURA_ANY);
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_gilgoblin_current_rider::Tick, EFFECT_0, SPELL_AURA_ANY);
        AfterEffectRemove += AuraEffectRemoveFn(spell_gilgoblin_current_rider::Removed, EFFECT_0, SPELL_AURA_ANY, AURA_EFFECT_HANDLE_REAL);
    }
};

// Brine Mending (15001054): 1% of maximum health every second while in water.
class spell_gilgoblin_brine_mending : public AuraScript
{
    PrepareAuraScript(spell_gilgoblin_brine_mending);

    void CalcPeriodic(AuraEffect const* /*aurEff*/, bool& isPeriodic, int32& amplitude)
    {
        isPeriodic = true;
        amplitude = 1000;
    }

    void Tick(AuraEffect const* /*aurEff*/)
    {
        Unit* owner = GetUnitOwner();
        if (!owner->IsAlive() || !owner->IsInWater() || owner->IsFullHealth())
            return;
        HealInfo heal(owner, owner, std::max<uint32>(1, owner->CountPctFromMaxHealth(1)), GetSpellInfo(), GetSpellInfo()->GetSchoolMask());
        owner->HealBySpell(heal);
    }

    void Register() override
    {
        DoEffectCalcPeriodic += AuraEffectCalcPeriodicFn(spell_gilgoblin_brine_mending::CalcPeriodic, EFFECT_0, SPELL_AURA_ANY);
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_gilgoblin_brine_mending::Tick, EFFECT_0, SPELL_AURA_ANY);
    }
};

// Brineborn Cascade (15001055): the brine hits its target, then leaps to the nearest ally or enemy not yet hit, up to
// five in all. Allies are healed for 15% of their maximum health (15001057), enemies take Frost damage of 15% of the
// caster's maximum health (15001058).
class spell_gilgoblin_brineborn_cascade : public SpellScript
{
    PrepareSpellScript(spell_gilgoblin_brineborn_cascade);

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        Unit* first = GetHitUnit();
        if (!caster || !first)
            return;

        float range = GetSpellInfo()->Effects[EFFECT_0].CalcRadius(caster);
        if (range <= 0.0f || range > 30.0f)
            range = 15.0f;

        std::set<ObjectGuid> hit;
        Unit* current = first;
        for (uint32 n = 0; current && n < 5; ++n)
        {
            hit.insert(current->GetGUID());
            if (caster->IsFriendlyTo(current))
            {
                int32 amount = int32(current->CountPctFromMaxHealth(15));
                caster->CastCustomSpell(current, SPELL_GILGOBLIN_CASCADE_HEAL, &amount, nullptr, nullptr, true);
            }
            else if (caster->IsValidAttackTarget(current))
            {
                int32 amount = int32(caster->CountPctFromMaxHealth(15));
                caster->CastCustomSpell(current, SPELL_GILGOBLIN_CASCADE_HIT, &amount, nullptr, nullptr, true);
            }

            std::list<Unit*> around;
            Acore::AnyUnitInObjectRangeCheck check(current, range);
            Acore::UnitListSearcher<Acore::AnyUnitInObjectRangeCheck> searcher(current, around, check);
            Cell::VisitObjects(current, searcher, range);

            Unit* next = nullptr;
            float best = 0.0f;
            for (Unit* unit : around)
            {
                if (!unit->IsAlive() || hit.count(unit->GetGUID()) || !caster->IsWithinLOSInMap(unit))
                    continue;
                if (!caster->IsFriendlyTo(unit) && !caster->IsValidAttackTarget(unit))
                    continue;
                float const distance = current->GetDistance(unit);
                if (!next || distance < best)
                {
                    next = unit;
                    best = distance;
                }
            }
            current = next;
        }
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_gilgoblin_brineborn_cascade::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

void AddSC_allraces_racials()
{
    RegisterSpellScript(spell_titan_aegis);
    RegisterSpellScript(spell_fungarian_sporebloom_pulse);
    RegisterSpellScript(spell_gilgoblin_current_rider);
    RegisterSpellScript(spell_gilgoblin_brine_mending);
    RegisterSpellScript(spell_gilgoblin_brineborn_cascade);
}
