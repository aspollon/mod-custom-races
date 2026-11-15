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
#include "Transport.h"

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

// The Hardluck ferries (gameobject 9900101/9900102 and the island elevators, 99001xx). A player who walked off the deck
// can stay a passenger on the server; when the ship leaves he is pulled after it, or across to the other map. CoA Custom
// fixes that in its own worldserver ("ferry detach"). Here: a passenger more than 40 yards from the ship's centre left it.
class FadingIslandFerryDetach : public PlayerScript
{
public:
    FadingIslandFerryDetach() : PlayerScript("FadingIslandFerryDetach", { PLAYERHOOK_ON_UPDATE }) { }

    void OnPlayerUpdate(Player* player, uint32 /*diff*/) override
    {
        Transport* transport = player->GetTransport();
        if (!transport || transport->GetEntry() < 9900100 || transport->GetEntry() > 9900199 || player->IsBeingTeleported())
            return;
        float const offset = player->m_movementInfo.transport.pos.GetExactDist2d(0.0f, 0.0f);
        float const fromShip = player->GetExactDist2d(transport);
        if (offset < 40.0f && fromShip < 40.0f)
            return;
        transport->RemovePassenger(player);
        player->SetTransport(nullptr);
        player->m_movementInfo.transport.Reset();
        player->RemoveUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT);
    }
};

void AddSC_fading_island()
{
    new FadingIslandGraveyard();
    new FadingIslandFerryDetach();
}
