#pragma once
#include "f4se/GameReferences.h"

class ActorEntry {
    public:
    UInt32 id;
    Actor* actor;
    float distance; // Distance from player for prioritization
};