/*
 * mod-custom-races - more playable races for Conquest of Azeroth.
 * Released under GNU AGPL v3: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

// Fading Island (map 907, CoA Custom V2) has no AreaTable rows, so zone and area are 0 there. The core then skips the
// island's graveyard_zone row and sends the dead to the default graveyard; CoA Custom fixes that in its own worldserver.
// Here: where zone and area are both 0, the nearest graveyard on the same map is used.

#include "GameGraveyard.h"
#include "Map.h"
#include "MapMgr.h"
#include "Player.h"
#include "ScriptMgr.h"

class FadingIslandGraveyard : public PlayerScript
{
public:
    FadingIslandGraveyard() : PlayerScript("FadingIslandGraveyard", { PLAYERHOOK_ON_BEFORE_CHOOSE_GRAVEYARD }) { }

    void OnPlayerBeforeChooseGraveyard(Player* player, TeamId /*teamId*/, bool nearCorpse, uint32& graveyardOverride) override
    {
        if (graveyardOverride || player->GetMap()->Instanceable())
            return;

        WorldLocation loc = nearCorpse ? player->GetCorpseLocation() : player->GetWorldLocation();
        Map const* map = player->GetMapId() == loc.GetMapId() ? player->GetMap() : sMapMgr->FindBaseNonInstanceMap(loc.GetMapId());
        if (!map)
            return;

        uint32 zoneId = 0, areaId = 0;
        map->GetZoneAndAreaId(player->GetPhaseMask(), zoneId, areaId, loc.GetPositionX(), loc.GetPositionY(), loc.GetPositionZ());
        if (zoneId || areaId)
            return;

        float best = 0.0f;
        for (auto const& [id, grave] : sGraveyard->GetGraveyardData())
        {
            if (grave.Map != loc.GetMapId())
                continue;
            float const dx = grave.x - loc.GetPositionX(), dy = grave.y - loc.GetPositionY(), dz = grave.z - loc.GetPositionZ();
            float const dist = dx * dx + dy * dy + dz * dz;
            if (!graveyardOverride || dist < best)
            {
                graveyardOverride = id;
                best = dist;
            }
        }
    }
};

void AddSC_fading_island()
{
    new FadingIslandGraveyard();
}
