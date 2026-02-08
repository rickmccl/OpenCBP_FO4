#include "f4se/NiNodes.h"
#include "f4se/GameForms.h"
#include "f4se/GameRTTI.h"

#include "ActorUtils.h"
#include "config.h"
#include "log.h"
#include "PapyrusOCBP.h"
#include "SimObj.h"

using actorUtils::IsBoneInWhitelist;

// Note we don't ref count the nodes becasue it's ignored when the Actor is deleted, and calling Release after that can corrupt memory
std::vector<std::string> boneNames;

SimObj::SimObj(Actor *actor, config_t &config)
    : things(4) {
    id = actor->formID;
}

SimObj::~SimObj() {
}


bool SimObj::Bind(Actor *actor, std::vector<std::string>& boneNames, config_t &config)
{
//	logger.error("bind\n");

    auto loadedData = actor->unkF0;
    if (loadedData && loadedData->rootNode) {
        bound = true;

        things.clear();
        for (std::string b : boneNames) {
            const char* bone_c_str = b.c_str();
            BSFixedString cs(bone_c_str);
            auto bone = loadedData->rootNode->GetObjectByName(&cs);
            if (!bone) {
				// these messages are common and don't indicate a problem, so use debug level
                logger.Debug("Failed to find Bone %s for actor %08x\n", b.c_str(), actor->formID);
            } else {
                logger.Debug("Doing Bone %s for actor %08x\n", b, actor->formID);
                things.emplace(b, Thing(bone, cs));
            }
        }
        UpdateConfig(config);
        return  true;
    }
    return false;
}

bool SimObj::ActorValid(Actor *actor) {
    if (actor->flags & TESForm::kFlag_IsDeleted)
        return false;
    if (actor && actor->unkF0 && actor->unkF0->rootNode)
        return true;
    return false;
}

void SimObj::Update(Actor *actor) {
    if (!bound)
        return;
        
    // Check distance from player with hysteresis to prevent stuttering
    auto player = DYNAMIC_CAST(LookupFormByID(0x14), TESForm, Actor);
    if (player && player->unkF0 && actor->formID != 0x14) { // Skip distance check for player themselves
        NiPoint3 playerPos = player->unkF0->rootNode->m_worldTransform.pos;
        NiPoint3 actorPos = actor->unkF0->rootNode->m_worldTransform.pos;
        
        float distanceSquared = (playerPos.x - actorPos.x) * (playerPos.x - actorPos.x) +
                                (playerPos.y - actorPos.y) * (playerPos.y - actorPos.y) +
                                (playerPos.z - actorPos.z) * (playerPos.z - actorPos.z);
        
        // Use hysteresis: different distances for enabling and disabling physics
        float enableDistanceSquared = physic_distance_enable * physic_distance_enable;
        float disableDistanceSquared = physic_distance_disable * physic_distance_disable;
        
        if (physicsActive) {
            // Physics is currently active, check if we should disable it
            if (distanceSquared > disableDistanceSquared) {
                physicsActive = false;
                return; // Skip physics processing
            }
        } else {
            // Physics is currently inactive, check if we should enable it
            if (distanceSquared > enableDistanceSquared) {
                return; // Still too far, skip physics processing
            } else {
                physicsActive = true; // Enable physics
            }
        }
    } else {
        // Player or invalid state, keep physics active
        physicsActive = true;
    }
    
    //logger.error("update\n");
    for (auto &t : things) {

        // Might be a better way to do this
        if (boneIgnores.find(actor->formID) != boneIgnores.end()) {
            auto actorBoneMap = boneIgnores.at(actor->formID);
            if (actorBoneMap.find(t.first) != actorBoneMap.end()) {
                if (actorBoneMap.at(t.first)) {
                    continue;
                }
            }
        }

        if (!useWhitelist || (IsBoneInWhitelist(actor, t.first) && useWhitelist)) {
            t.second.Update(actor);
        }
    }
    //logger.error("end SimObj update\n");
}

bool SimObj::UpdateConfig(config_t & config) {
    for (auto &thing : things) {
        thing.second.UpdateConfig(config[std::string(thing.first)]);
    }
    return true;
}