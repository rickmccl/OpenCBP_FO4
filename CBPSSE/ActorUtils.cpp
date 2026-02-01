#include <algorithm>

#include "ActorUtils.h"
#include "log.h"
#include "SimObj.h" // for boneNames & debonedActors

#include "f4se/GameExtraData.h"
#include "f4se/GameObjects.h"
#include "f4se/GameRTTI.h"
#include "f4se/NiNodes.h" // for NiNode and GetObjectByName

std::string actorUtils::GetActorRaceEID(Actor* actor) {
    return std::string(actor->race->editorId.c_str());
}

bool actorUtils::IsActorMale(Actor *actor)
{
    TESNPC* actorNPC = DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);

    auto npcSex = actorNPC ? CALL_MEMBER_FN(actorNPC, GetSex)() : 1;

    if (npcSex == 0) //Actor is male
        return true;
    else
        return false;
}

bool actorUtils::IsActorInPowerArmor(Actor* actor) {
    if (!actor)
        return false;
	return actor->extraDataList->HasType(kExtraData_PowerArmor);  // removed negation instead of rename function RM 1/30/26
}


bool actorUtils::IsActorTorsoArmorEquipped(Actor* actor)
{
    auto biped = actor->biped.get();
    if (!biped || !biped->object)
        return false;

    auto& torsoSlot = biped->object[11];
    auto torsoItem = torsoSlot.parent.object;

    if (!torsoItem)
        return false;

    // ignore list logic preserved
    auto torsoFormID = torsoItem->formID;
    if (armorIgnore.find(torsoFormID) != armorIgnore.end())
        return false;

    // bodyFormID - 1 fallback
    auto& bodySlot = biped->object[3];
    if (bodySlot.parent.object)
    {
        auto bodyFormID = bodySlot.parent.object->formID;
        if (torsoFormID == bodyFormID - 1 &&
            (armorIgnore.find(bodyFormID) != armorIgnore.end()))
            return false;
    }

    return true;
}

/*
bool actorUtils::IsActorTrackable(Actor* actor) {
    bool inRaceWhitelist = find(raceWhitelist.begin(), raceWhitelist.end(), actorUtils::GetActorRaceEID(actor)) != raceWhitelist.end();
    return IsActorInPowerArmor(actor) &&
            (!playerOnly || (actor->formID == 0x14 && playerOnly)) &&
            (!maleOnly || (IsActorMale(actor) && maleOnly)) &&
            (!femaleOnly || (!IsActorMale(actor) && femaleOnly)) &&
            (!npcOnly || (actor->formID != 0x14 && npcOnly)) &&
            (!useWhitelist || (inRaceWhitelist && useWhitelist));
}
*/

bool actorUtils::IsActorTrackable(Actor* actor) 
// Selects actors for processing based on INI file parameters.
{
    if (!actor)
        return false;

    // 1. Special exclusion: Power Armor
    if (IsActorInPowerArmor(actor))
        return false;

    // 2. Player/NPC filtering
    bool isPlayer = (actor->formID == 0x14);
    if (playerOnly && !isPlayer)
        return false;
    if (npcOnly && isPlayer)
        return false;

    // 3. Gender filtering
    bool isMale = IsActorMale(actor);
    if (maleOnly && !isMale)
        return false;
    if (femaleOnly && isMale)
        return false;

    // 4. Race whitelist
    if (useWhitelist) {
        std::string race = GetActorRaceEID(actor);
        bool inList = std::find(raceWhitelist.begin(), raceWhitelist.end(), race) != raceWhitelist.end();
        if (!inList)
            return false;
    }

    // 5. Optional autowhitelist: exclude actors that do not contain any configured bones.
    //    Use a cache (debonedActors) so we only probe each actor once per session/cell.
    if (autoWhitelist) {
        // If already known to be deboned, skip immediately
        if (debonedActors.find(actor->formID) != debonedActors.end())
            return false;

        // If no bones configured, treat as "no filter" => accept actor
        if (!boneNames.empty()) {
            auto loadedState = actor->unkF0;
            if (!loadedState || !loadedState->rootNode) {
                // Not loaded enough to check bones — don't track now
                return false;
            }

            bool hasAnyBone = false;
            auto root = loadedState->rootNode;
            for (const auto &b : boneNames) {
                if (b.empty()) continue;
                BSFixedString cs(b.c_str());
                if (root->GetObjectByName(&cs)) {
                    hasAnyBone = true;
                    break;
                }
            }
            if (!hasAnyBone) {
                // remember this actor as deboned so we don't probe again until config reload / cell change
                debonedActors.insert(actor->formID);
                logger.Info("autowhitelist: marking actor %08x as deboned (no configured bones)\n", actor->formID);
                return false;
            }
        }
    }

    return true;   // passed all filters
}

bool actorUtils::IsBoneInWhitelist(Actor* actor, std::string boneName) {
    if (!actor)
        return false;

    auto raceEID = actorUtils::GetActorRaceEID(actor);

    auto wbIt = whitelist.find(boneName);
    if (wbIt == whitelist.end())
        return false;

    const auto& racesMap = wbIt->second;
    auto rmIt = racesMap.find(raceEID);
    if (rmIt == racesMap.end())
        return false;

    const whitelistSex &sexFlags = rmIt->second;
    return IsActorMale(actor) ? sexFlags.male : sexFlags.female;
}