#include "Thing.h"
#include "log.h"
#include "config.h"
#include "f4se\NiNodes.h"
#include "f4se\GameReferences.h"
#include "f4se\GameForms.h"
#include "f4se\GameRTTI.h"
#include <time.h>

constexpr auto DEG_TO_RAD = 3.14159265 / 180;
const char* skeletonNif_boneName = "skeleton.nif";
const char* COM_boneName = "COM";

// TODO Make these logger macros
//#define DEBUG 1
//#define TRANSFORM_DEBUG 1

// <BoneName <ActorFormID, data>>
//pos_map origLocalPos;
//rot_map origLocalRot;

pos_map Thing::origLocalPos;
rot_map Thing::origLocalRot;

void Thing::ShowPos(NiPoint3& p) {
    logger.Info("%8.4f %8.4f %8.4f\n", p.x, p.y, p.z);
}

void Thing::ShowRot(NiMatrix43& r) {
	logger.Debug("Rotation Matrix:\n");
    logger.Debug("%8.4f %8.4f %8.4f %8.4f\n", r.data[0][0], r.data[0][1], r.data[0][2], r.data[0][3]);
    logger.Debug("%8.4f %8.4f %8.4f %8.4f\n", r.data[1][0], r.data[1][1], r.data[1][2], r.data[1][3]);
    logger.Debug("%8.4f %8.4f %8.4f %8.4f\n", r.data[2][0], r.data[2][1], r.data[2][2], r.data[2][3]);
}

Thing::Thing(NiAVObject* obj, BSFixedString& name)
    : boneName(name)
    , velocity(NiPoint3(0, 0, 0)) {
    
    // Set initial positions
    oldWorldPos = obj->m_worldTransform.pos;
    time = clock();
}

Thing::~Thing() {
}

void Thing::UpdateConfig(configEntry_t & centry) {
    stiffness = centry["stiffness"];
    stiffness2 = centry["stiffness2"];
    damping = centry["damping"];

    maxOffsetX = centry["maxoffsetX"];
    maxOffsetY = centry["maxoffsetY"];
    maxOffsetZ = centry["maxoffsetZ"];

    linearX = centry["linearX"];
    linearY = centry["linearY"];
    linearZ = centry["linearZ"];

    rotationalX = centry["rotationalX"];
    rotationalY = centry["rotationalY"];
    rotationalZ = centry["rotationalZ"];

    rotateLinearX = centry["rotateLinearX"];
    rotateLinearY = centry["rotateLinearY"];
    rotateLinearZ = centry["rotateLinearZ"];

    rotateRotationX = centry["rotateRotationX"];
    rotateRotationY = centry["rotateRotationY"];
    rotateRotationZ = centry["rotateRotationZ"];

    timeTick = centry["timetick"];

    if (centry.find("timeStep") != centry.end())
        timeStep = centry["timeStep"];
    else 
        timeStep = 0.016f;

    gravityBias = centry["gravityBias"];
    gravityCorrection = centry["gravityCorrection"];
    cogOffsetY = centry["cogOffsetY"];
    cogOffsetX = centry["cogOffsetX"];
    cogOffsetZ = centry["cogOffsetZ"];
    if (timeTick <= 1)
        timeTick = 1;

    absRotX = centry["absRotX"] != 0.0;
}

static float clamp(float val, float min, float max) {
    if (val < min) return min;
    else if (val > max) return max;
    return val;
}

void Thing::Reset(Actor *actor) {
    auto loadedState = actor->unkF0;
    if (!loadedState || !loadedState->rootNode) {
        logger.Error("No loaded state for actor %08x\n", actor->formID);
        return;
    }
    auto obj = loadedState->rootNode->GetObjectByName(&boneName);

    if (!obj) {
        logger.Error("Couldn't get name for loaded state for actor %08x\n", actor->formID);
        return;
    }

    obj->m_localTransform.pos = origLocalPos[boneName.c_str()][actor->formID];
    obj->m_localTransform.rot = origLocalRot[boneName.c_str()][actor->formID];
}

// Returns 
template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

NiAVObject* Thing::IsActorValid(Actor* actor) {
    auto loadedState = actor->unkF0;
    if (!loadedState || !loadedState->rootNode) {
        logger.Error("No loaded state for actor %08x\n", actor->formID);
        return NULL;
    }
    auto obj = loadedState->rootNode->GetObjectByName(&boneName);

    if (!obj) {
        logger.Error("Couldn't get name for loaded state for actor %08x\n", actor->formID);
        return NULL;
    }

    if (!obj->m_parent) {
        logger.Error("Couldn't get bone %s parent for actor %08x\n", boneName.c_str(), actor->formID);
        return NULL;
    }

    return obj;
}

void Thing::Update(Actor *actor) {
    /*LARGE_INTEGER startingTime, endingTime, elapsedMicroseconds;
    LARGE_INTEGER frequency;

    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&startingTime);*/

    auto newTime = clock();
    auto deltaT = newTime - time;

    time = newTime;
    if (deltaT > 64) deltaT = 64;
    if (deltaT < 8) deltaT = 8;
    
    auto obj = IsActorValid(actor);
    if (!obj) {
        return;
    }
    


#if TRANSFORM_DEBUG
    auto sceneObj = obj;
    while (sceneObj->m_parent && sceneObj->m_name != "skeleton.nif")
    {
        logger.Info(sceneObj->m_name);
        logger.Info("\n---\n");
        logger.Error("Actual m_localTransform.pos: ");
        ShowPos(sceneObj->m_localTransform.pos);
        logger.Error("Actual m_worldTransform.pos: ");
        ShowPos(sceneObj->m_worldTransform.pos);
        logger.Info("---\n");
        //logger.Error("Actual m_localTransform.rot Matrix:\n");
        ShowRot(sceneObj->m_localTransform.rot);
        //logger.Error("Actual m_worldTransform.rot Matrix:\n");
        ShowRot(sceneObj->m_worldTransform.rot);
        logger.Info("---\n");
        //if (sceneObj->m_parent) {
        //	logger.Error("Calculated m_worldTransform.pos: ");
        //	ShowPos((sceneObj->m_parent->m_worldTransform.rot.Transpose() * sceneObj->m_localTransform.pos) + sceneObj->m_parent->m_worldTransform.pos);
        //	logger.Error("Calculated m_worldTransform.rot Matrix:\n");
        //	ShowRot(sceneObj->m_localTransform.rot * sceneObj->m_parent->m_worldTransform.rot);
        //}
        sceneObj = sceneObj->m_parent;	
    }
#endif

    // Save the bones' original local values if they already haven't
    auto origLocalPos_iter = origLocalPos.find(boneName.c_str());
    auto origLocalRot_iter = origLocalRot.find(boneName.c_str());

    if (origLocalPos_iter == origLocalPos.end()) {
#ifdef DEBUG
        logger.Error("for bone %s, actor %08x: \n", boneName.c_str(), actor->formID);
        logger.Error("firstRun pos Set: \n");
#endif  
        origLocalPos[boneName.c_str()][actor->formID] = obj->m_localTransform.pos;
        ShowPos(obj->m_localTransform.pos);
    }
    else {
        auto actorPosMap = origLocalPos.at(boneName.c_str());
        auto actor_iter = actorPosMap.find(actor->formID);
        if (actor_iter == actorPosMap.end()) {
#ifdef DEBUG
            logger.Error("for bone %s, actor %08x: \n", boneName.c_str(), actor->formID);
            logger.Error("firstRun pos Set: \n");
#endif
            origLocalPos[boneName.c_str()][actor->formID] = obj->m_localTransform.pos;
            ShowPos(obj->m_localTransform.pos);
        }
    }
    if (origLocalRot_iter == origLocalRot.end()) {
#ifdef DEBUG
        logger.Error("for bone %s, actor %08x: \n", boneName.c_str(), actor->formID);
        logger.Error("firstRun rot Set:\n");
#endif // DEBUG
        origLocalRot[boneName.c_str()][actor->formID] = obj->m_localTransform.rot;
        ShowRot(obj->m_localTransform.rot);
    }
    else {
        auto actorRotMap = origLocalRot.at(boneName.c_str());
        auto actor_iter = actorRotMap.find(actor->formID);
        if (actor_iter == actorRotMap.end()) {
#ifdef DEBUG
            logger.Error("for bone %s, actor %08x: \n", boneName.c_str(), actor->formID);
            logger.Error("firstRun rot Set: \n");
#endif // DEBUG
            origLocalRot[boneName.c_str()][actor->formID] = obj->m_localTransform.rot;
            ShowRot(obj->m_localTransform.rot);
        }
    }

    auto skeletonObj = obj;
    NiAVObject * comObj;
    bool skeletonFound = false;
    while (skeletonObj->m_parent)
    {
        if (skeletonObj->m_parent->m_name == BSFixedString(COM_boneName)) {
            comObj = skeletonObj->m_parent;
        }
        else if (skeletonObj->m_parent->m_name == BSFixedString(skeletonNif_boneName)) {
            skeletonObj = skeletonObj->m_parent;
            skeletonFound = true;
            break;
        }
        skeletonObj = skeletonObj->m_parent;
    }
    if (skeletonFound == false) {
        // common event while game is loading, therefore not an error
        logger.Debug("Couldn't find skeleton for actor %08x\n", actor->formID);
        return;
    }

#if DEBUG
    logger.Error("bone %s for actor %08x with parent %s\n", boneName.c_str(), actor->formID, skeletonObj->m_name.c_str());
    ShowRot(skeletonObj->m_worldTransform.rot);
    //ShowPos(obj->m_parent->m_worldTransform.rot.Transpose() * obj->m_localTransform.pos);
#endif
    NiMatrix43 targetRot = skeletonObj->m_localTransform.rot.Transpose();
    NiPoint3 origWorldPos = (obj->m_parent->m_worldTransform.rot.Transpose() * origLocalPos[boneName.c_str()][actor->formID]) +  obj->m_parent->m_worldTransform.pos;

    // Offset to move Center of Mass make rotational motion more significant
    NiPoint3 target = (targetRot * NiPoint3(cogOffsetX, cogOffsetY, cogOffsetZ)) + origWorldPos;

#if DEBUG
    logger.Error("World Position: ");
    ShowPos(obj->m_worldTransform.pos);
    //logger.Error("Parent World Position difference: ");
    //ShowPos(obj->m_worldTransform.pos - obj->m_parent->m_worldTransform.pos);
    logger.Error("Target Rotation * cogOffsetY %8.4f: ", cogOffsetY);
    ShowPos(targetRot * NiPoint3(cogOffsetX, cogOffsetY, cogOffsetZ));
    //logger.Error("Target Rotation:\n");
    //ShowRot(targetRot);
    logger.Error("cogOffset x Transformation:");
    ShowPos(targetRot * NiPoint3(cogOffsetX, 0, 0));
    logger.Error("cogOffset y Transformation:");
    ShowPos(targetRot * NiPoint3(0, cogOffsetY, 0));
    logger.Error("cogOffset z Transformation:");
    ShowPos(targetRot * NiPoint3(0, 0, cogOffsetZ));
#endif

    // diff is Difference in position between old and new world position
    NiPoint3 diff = target - oldWorldPos;

    // Move up in for gravity correction
    diff += targetRot * NiPoint3(0, 0, gravityCorrection);

#if DEBUG
    logger.Error("Diff after gravity correction %f: ", gravityCorrection);
    ShowPos(diff);
#endif

    if (fabs(diff.x) > 100 || fabs(diff.y) > 100 || fabs(diff.z) > 100) {
        logger.Error("transform reset\n");
        obj->m_localTransform.pos = origLocalPos[boneName.c_str()][actor->formID];
        oldWorldPos = target;
        velocity = NiPoint3(0, 0, 0);
        time = clock();
    } else {

        NiPoint3 posDelta = NiPoint3(0, 0, 0);

        float originalDeltaT = deltaT;
        float realFPS = 1000.0f / originalDeltaT;
        float targetFPS = 60.0f;
        
        // Normalize timestep to 60 FPS reference (never exceed 60 FPS simulation speed)
        // This ensures physics behaves identically at 30 FPS, 60 FPS, 120 FPS, or 160 FPS
        float normalizedTimeStep = timeStep;
        float normalizedTimeTick = timeTick;
        
        if (realFPS > targetFPS) {
            // High FPS: reduce simulation timestep but clamp to prevent numerical instability
            float fpsRatio = targetFPS / realFPS;  // At 120 FPS: 0.5, At 160 FPS: 0.375
            float clampedRatio = fmax(fpsRatio, 0.667f);  // Never go below 66.7% to prevent jello effect
            normalizedTimeStep = timeStep * clampedRatio;
            normalizedTimeTick = timeTick * clampedRatio;
        } else if (realFPS < targetFPS) {
            // Low FPS: increase simulation timestep to maintain physics stability
            float fpsRatio = targetFPS / realFPS;  // At 30 FPS: 2.0, At 45 FPS: 1.33
            normalizedTimeStep = timeStep * fpsRatio;
            normalizedTimeTick = timeTick * fpsRatio;
        }
        
        do {
            // Per-iteration spring force recalculation
            // Current position = oldWorldPos + accumulated posDelta
            NiPoint3 currentWorldPos = oldWorldPos + posDelta;
            NiPoint3 iterationDiff = target - currentWorldPos;
            
            // Apply gravity correction before scaling to match old behavior
            iterationDiff += targetRot * NiPoint3(0, 0, gravityCorrection);
            
            // Scale based on timeTick to maintain force magnitude independence from timeTick
            iterationDiff *= timeTick / 16.0f;
            
            // Compute spring force based on current displacement (not frame-dependent)
            NiPoint3 diff2(iterationDiff.x * iterationDiff.x * sgn(iterationDiff.x), 
                           iterationDiff.y * iterationDiff.y * sgn(iterationDiff.y), 
                           iterationDiff.z * iterationDiff.z * sgn(iterationDiff.z));
            NiPoint3 force = (iterationDiff * stiffness) + (diff2 * stiffness2) - (targetRot * NiPoint3(0, 0, gravityBias));

#if DEBUG
            logger.Error("Iteration Diff: ");
            ShowPos(iterationDiff);
            logger.Error("Force with stiffness %f, stiffness2 %f, gravity bias %f: ", stiffness, stiffness2, gravityBias);
            ShowPos(force);
#endif

            // Apply force with normalized timestep
            velocity = (velocity + (force * normalizedTimeStep)) - (velocity * (damping * normalizedTimeStep));

            // New position accounting for time
            posDelta += (velocity * normalizedTimeStep);
            deltaT -= normalizedTimeTick;
        } while (deltaT >= normalizedTimeTick);

        NiPoint3 newPos = oldWorldPos + posDelta;

        oldWorldPos = newPos;

#if DEBUG
        //logger.Error("posDelta: ");
        //ShowPos(posDelta);
        logger.Error("newPos: ");
        ShowPos(newPos);
#endif
        // clamp the difference to stop the breast severely lagging at low framerates
        diff = newPos - target;

        diff.x = clamp(diff.x, -maxOffsetX, maxOffsetX);
        diff.y = clamp(diff.y, -maxOffsetY, maxOffsetY);
        diff.z = clamp(diff.z - gravityCorrection, -maxOffsetZ, maxOffsetZ) + gravityCorrection;

#if DEBUG
        logger.Error("diff from newPos: ");
        ShowPos(diff);
        //logger.Error("oldWorldPos: ");
        //ShowPos(oldWorldPos);
#endif

        // move the bones based on the supplied weightings
        // Convert the world translations into local coordinates

        NiMatrix43 rotateLinear;
        rotateLinear.SetEulerAngles(rotateLinearX* DEG_TO_RAD,
                                    rotateLinearY* DEG_TO_RAD,
                                    rotateLinearZ* DEG_TO_RAD);

        NiMatrix43 invRot = rotateLinear * obj->m_parent->m_worldTransform.rot;

        auto localDiff = diff;
        localDiff = skeletonObj->m_localTransform.rot * localDiff;
        localDiff.x *= linearX;
        localDiff.y *= linearY;
        localDiff.z *= linearZ;

        auto rotDiff = localDiff;
        localDiff = skeletonObj->m_localTransform.rot.Transpose() * localDiff;

        localDiff = invRot * localDiff;
        oldWorldPos = diff + target;
#if DEBUG
        logger.Error("invRot x=10 Transformation:");
        ShowPos(invRot * NiPoint3(10, 0, 0));
        logger.Error("invRot y=10 Transformation:");
        ShowPos(invRot * NiPoint3(0, 10, 0));
        logger.Error("invRot z=10 Transformation:");
        ShowPos(invRot * NiPoint3(0, 0, 10));
        logger.Error("oldWorldPos: ");
        ShowPos(oldWorldPos);
        logger.Error("localTransform.pos: ");
        ShowPos(obj->m_localTransform.pos);
        logger.Error("localDiff: ");
        ShowPos(localDiff);
        logger.Error("rotDiff: ");
        ShowPos(rotDiff);

#endif
        // scale positions from config
        NiPoint3 newLocalPos = NiPoint3((localDiff.x) + origLocalPos[boneName.c_str()][actor->formID].x,
                                        (localDiff.y) + origLocalPos[boneName.c_str()][actor->formID].y,
                                        (localDiff.z) + origLocalPos[boneName.c_str()][actor->formID].z
        );
        obj->m_localTransform.pos = newLocalPos;
        
        if (absRotX) rotDiff.x = fabs(rotDiff.x);

        rotDiff.x *= rotationalX;
        rotDiff.y *= rotationalY;
        rotDiff.z *= rotationalZ;


#if DEBUG
        logger.Error("localTransform.pos after: ");
        ShowPos(obj->m_localTransform.pos);
        logger.Error("origLocalPos:");
        ShowPos(origLocalPos[boneName.c_str()][actor->formID]);
        logger.Error("origLocalRot:");
        ShowRot(origLocalRot[boneName.c_str()][actor->formID]);

#endif
        // Do rotation.
        NiMatrix43 rotateRotation;
        rotateRotation.SetEulerAngles(rotateRotationX * DEG_TO_RAD,
                                      rotateRotationY * DEG_TO_RAD,
                                      rotateRotationZ * DEG_TO_RAD);

        NiMatrix43 standardRot;

        rotDiff = rotateRotation * rotDiff;
        standardRot.SetEulerAngles(rotDiff.x, rotDiff.y, rotDiff.z);
        obj->m_localTransform.rot = standardRot * origLocalRot[boneName.c_str()][actor->formID];
    }
#if DEBUG
    logger.Error("end update()\n");
#endif

    //logger.Error("end update()\n");
    /*QueryPerformanceCounter(&endingTime);
    elapsedMicroseconds.QuadPart = endingTime.QuadPart - startingTime.QuadPart;
    elapsedMicroseconds.QuadPart *= 1000000000LL;
    elapsedMicroseconds.QuadPart /= frequency.QuadPart;
    _MESSAGE("Thing.update() Update Time = %lld ns\n", elapsedMicroseconds.QuadPart);*/

}