#include "config.h"
#include "INIReader.h"
#include "log.h"
#include "SimObj.h"
#include "Thing.h"

#include <iostream>
#include <iterator>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <fstream>

#include "f4se/GameObjects.h"
#include "f4se/GameRTTI.h"
#include "f4se_common/Utilities.h"

#define DEBUG 0
#pragma warning(disable : 4996)


bool playerOnly = false;
bool femaleOnly = false;
bool maleOnly = false;
bool npcOnly = false;
bool detectArmor = false;
bool useWhitelist = false;
bool autoWhitelist = false;
bool loggingEnabled = true;
bool logConsolidationEnabled = true;
float physic_distance_enable = 7500.0f;
float physic_distance_disable = 8500.0f;
int max_active_actors = 10;
int autoMode = 0;
int targetFPS = 60;
int autoExceptions = 1;

config_t config;
config_t configArmor;
configOverrides_t configOverrides;
configOverrides_t configArmorOverrides;

FILETIME lastMainINITime = {0};
FILETIME lastMCMINITime = {0};

// TODO data structure these
whitelist_t whitelist;
std::vector<std::string> raceWhitelist;
std::unordered_map<UInt32, bool> armorIgnore;

bool LoadConfig() {
    logger.Info("CONFIG: Load config\n");

    std::set<std::string> bonesSet;

    bool reloadActors = false;
    auto playerOnlyOld = playerOnly;
    auto femaleOnlyOld = femaleOnly;
    auto maleOnlyOld = maleOnly;
    auto npcOnlyOld = npcOnly;
    auto useWhitelistOld = useWhitelist;

    boneNames.clear();
    config.clear();
    configArmor.clear();
    configOverrides.clear();
    configArmorOverrides.clear();
    armorIgnore.clear();

    // Note: Using INIReader results in a slight double read
    INIReader configReader("Data\\F4SE\\Plugins\\ocbp.ini");
    if (configReader.ParseError() < 0) {
        logger.Error("CONFIG: Can't load 'ocbp.ini'\n");
    }
    logger.Error("CONFIG: Reading CBP Config\n");

    // Check for MCM override settings
    INIReader mcmReader("Data\\MCM\\Settings\\OCBP.ini");
    bool hasMCM = (mcmReader.ParseError() >= 0);
    if (hasMCM) {
        logger.Error("CONFIG: Found MCM settings, applying overrides\n");
    }

    // Read logging settings first (affects subsequent log output)
    loggingEnabled = configReader.GetBoolean("General", "loggingEnabled", true);
    logger.SetLoggingEnabled(loggingEnabled);

    logConsolidationEnabled = configReader.GetBoolean("General", "logConsolidationEnabled", true);
    logger.SetConsolidationEnabled(logConsolidationEnabled);

    logger.Info("CONFIG: Logging: %s | Consolidation: %s\n",
                loggingEnabled ? "enabled" : "disabled",
                logConsolidationEnabled ? "enabled" : "disabled");

    // Read defaults from config (INI)
    bool cfg_playerOnly = configReader.GetBoolean("General", "playerOnly", false);
    bool cfg_npcOnly = configReader.GetBoolean("General", "npcOnly", false);
    bool cfg_femaleOnly = configReader.GetBoolean("General", "femaleOnly", false);
    bool cfg_maleOnly = configReader.GetBoolean("General", "maleOnly", false);
    bool cfg_useWhitelist = configReader.GetBoolean("General", "useWhitelist", false);
    bool cfg_autoWhitelist = configReader.GetBoolean("General", "autoWhitelist", false);
    bool cfg_detectArmor = configReader.GetBoolean("General", "detectArmor", false);

    float cfg_physic_distance_enable = configReader.GetFloat("General", "physic_distance_enable", 7500.0f);
    float cfg_physic_distance_disable = configReader.GetFloat("General", "physic_distance_disable", 8500.0f);
    int   cfg_max_active_actors = configReader.GetInteger("General", "max_active_actors", 10);

    int cfg_autoMode = configReader.GetInteger("General", "autoMode", 0);
    int cfg_targetFPS = configReader.GetInteger("General", "targetFPS", 60);
    int cfg_autoExceptions = configReader.GetInteger("General", "autoExceptions", 1);

    // Determine if MCM provided a [General] section and helper to test for key presence
    bool mcmHasGeneral = hasMCM && (mcmReader.Sections().count("General") > 0);

    auto mcmHasKey = [&](const char* key) -> bool {
        if (!mcmHasGeneral) return false;
        try {
            return mcmReader.Section("General").count(key) > 0;
        } catch (...) {
            return false;
        }
    };

    // Apply per-key precedence: INI defaults first, then MCM only if the key exists there
    if (mcmHasKey("bPlayerOnly")) {
        playerOnly = mcmReader.GetBoolean("General", "bPlayerOnly", cfg_playerOnly);
    } else {
        playerOnly = cfg_playerOnly;
    }
    logger.Info("CONFIG: playerOnly=%s (source: %s)\n",
                playerOnly ? "true" : "false",
                mcmHasKey("bPlayerOnly") ? "MCM" : "INI");

    if (mcmHasKey("bNpcOnly")) {
        npcOnly = mcmReader.GetBoolean("General", "bNpcOnly", cfg_npcOnly);
    } else {
        npcOnly = cfg_npcOnly;
    }
    logger.Info("CONFIG: npcOnly=%s (source: %s)\n",
                npcOnly ? "true" : "false",
                mcmHasKey("bNpcOnly") ? "MCM" : "INI");

    if (mcmHasKey("bFemaleOnly")) {
        femaleOnly = mcmReader.GetBoolean("General", "bFemaleOnly", cfg_femaleOnly);
    } else {
        femaleOnly = cfg_femaleOnly;
    }
    logger.Info("CONFIG: femaleOnly=%s (source: %s)\n",
                femaleOnly ? "true" : "false",
                mcmHasKey("bFemaleOnly") ? "MCM" : "INI");

    if (mcmHasKey("bMaleOnly")) {
        maleOnly = mcmReader.GetBoolean("General", "bMaleOnly", cfg_maleOnly);
    } else {
        maleOnly = cfg_maleOnly;
    }
    logger.Info("CONFIG: maleOnly=%s (source: %s)\n",
                maleOnly ? "true" : "false",
                mcmHasKey("bMaleOnly") ? "MCM" : "INI");

    if (mcmHasKey("bUseWhitelist")) {
        useWhitelist = mcmReader.GetBoolean("General", "bUseWhitelist", cfg_useWhitelist);
    } else {
        useWhitelist = cfg_useWhitelist;
    }
    logger.Info("CONFIG: useWhitelist=%s (source: %s)\n",
                useWhitelist ? "true" : "false",
                mcmHasKey("bUseWhitelist") ? "MCM" : "INI");

    if (mcmHasKey("bautoWhitelist")) {
        autoWhitelist = mcmReader.GetBoolean("General", "bautoWhitelist", cfg_autoWhitelist);
    } else {
        autoWhitelist = cfg_autoWhitelist;
    }
    logger.Info("CONFIG: autoWhitelist=%s (source: %s)\n",
                autoWhitelist ? "true" : "false",
                mcmHasKey("autoWhitelist") ? "MCM" : "INI");

    if (mcmHasKey("bDetectArmor")) {
        detectArmor = mcmReader.GetBoolean("General", "bDetectArmor", cfg_detectArmor);
    } else {
        detectArmor = cfg_detectArmor;
    }
    logger.Info("CONFIG: detectArmor=%s (source: %s)\n",
                detectArmor ? "true" : "false",
                mcmHasKey("bDetectArmor") ? "MCM" : "INI");

    if (mcmHasKey("fPhysicDistanceEnable")) {
        physic_distance_enable = mcmReader.GetFloat("General", "fPhysicDistanceEnable", cfg_physic_distance_enable);
    } else {
        physic_distance_enable = cfg_physic_distance_enable;
    }
    logger.Info("CONFIG: physic_distance_enable=%.1f (source: %s)\n",
                physic_distance_enable,
                mcmHasKey("fPhysicDistanceEnable") ? "MCM" : "INI");

    if (mcmHasKey("fPhysicDistanceDisable")) {
        physic_distance_disable = mcmReader.GetFloat("General", "fPhysicDistanceDisable", cfg_physic_distance_disable);
    } else {
        physic_distance_disable = cfg_physic_distance_disable;
    }
    logger.Info("CONFIG: physic_distance_disable=%.1f (source: %s)\n",
                physic_distance_disable,
                mcmHasKey("fPhysicDistanceDisable") ? "MCM" : "INI");

    if (mcmHasKey("iMaxActiveActors")) {
        max_active_actors = (int)mcmReader.GetInteger("General", "iMaxActiveActors", cfg_max_active_actors);
    } else {
        max_active_actors = cfg_max_active_actors;
    }
    logger.Info("CONFIG: max_active_actors=%d (source: %s)\n",
                max_active_actors,
                mcmHasKey("iMaxActiveActors") ? "MCM" : "INI");

    // numeric fallbacks (only override when MCM key exists)
    if (mcmHasKey("iAutoMode")) {
        autoMode = (int)mcmReader.GetInteger("General", "iAutoMode", cfg_autoMode);
    } else {
        autoMode = cfg_autoMode;
    }
    logger.Info("CONFIG: autoMode=%d (source: %s)\n",
                autoMode,
                mcmHasKey("iAutoMode") ? "MCM" : "INI");

    if (mcmHasKey("iTargetFPS")) {
        targetFPS = (int)mcmReader.GetInteger("General", "iTargetFPS", cfg_targetFPS);
    } else {
        targetFPS = cfg_targetFPS;
    }
    logger.Info("CONFIG: targetFPS=%d (source: %s)\n",
                targetFPS,
                mcmHasKey("iTargetFPS") ? "MCM" : "INI");

    if (mcmHasKey("iAutoExceptions")) {
        autoExceptions = (int)mcmReader.GetInteger("General", "iAutoExceptions", cfg_autoExceptions);
    } else {
        autoExceptions = cfg_autoExceptions;
    }
    logger.Info("CONFIG: autoExceptions=%d (source: %s)\n",
                autoExceptions,
                mcmHasKey("iAutoExceptions") ? "MCM" : "INI");

    reloadActors = (playerOnly ^ playerOnlyOld) ||
                    (femaleOnly ^ femaleOnlyOld) ||
                    (maleOnly ^ maleOnlyOld) ||
                    (npcOnly ^ npcOnlyOld) ||
                    (useWhitelist ^ useWhitelistOld);

    logger.Info("CONFIG: Reloadactors %s, playerOnly %s, NPCOnly %s, femaleOnly %s, maleOnly %s, useWhiteList %s\n",
        reloadActors ? "true" : "false",
        playerOnly ? "true" : "false",
        npcOnly ? "true" : "false",
        femaleOnly ? "true" : "false",
        maleOnly ? "true" : "false",
        useWhitelist ? "true" : "false");

    // Validate distance settings
    if (physic_distance_disable <= physic_distance_enable) {
        logger.Error("CONFIG WARNING: physic_distance_disable (%f) must be greater than physic_distance_enable (%f)\n",
                     physic_distance_disable, physic_distance_enable);
        logger.Error("CONFIG: Auto-correcting: setting physic_distance_disable to %f\n", physic_distance_enable + 1000.0f);
        physic_distance_disable = physic_distance_enable + 1000.0f;
    }

    // Validate max actors setting
    if (max_active_actors < 1) {
        logger.Error("CONFIG WARNING: max_active_actors (%d) must be at least 1. Setting to 1\n", max_active_actors);
        max_active_actors = 1;
    } else if (max_active_actors > 50) {
        logger.Error("CONFIG WARNING: max_active_actors (%d) is very high! This may cause performance issues\n", max_active_actors);
    }

    logger.Info("CONFIG: Physics settings: enable=%.1f, disable=%.1f, max_actors=%d\n",
                physic_distance_enable, physic_distance_disable, max_active_actors);

    //Read armorIgnore
    auto armorIgnoreStr = configReader.Get("General", "armorIgnore", "");
    {
        size_t commaPos;
        do {
            commaPos = armorIgnoreStr.find_first_of(",");
            auto token = armorIgnoreStr.substr(0, commaPos);
            UInt32 formID;
            std::stringstream ss;
            ss << std::hex << token;
            ss >> formID;
            armorIgnore[formID] = true;
            armorIgnoreStr = armorIgnoreStr.substr(commaPos + 1);

            //logger.Info("<token:> %s, <rest:> %s, <commaPos:> %d, <colonPos:> %d\n", token.c_str(), whitelistName.c_str(), commaPos >= 0, colonPos < 0);
        } while (commaPos != -1);
    }

    // Read sections
    auto sections = configReader.Sections();
    for (auto sectionsIter = sections.begin(); sectionsIter != sections.end(); ++sectionsIter) {

        // Split for override section check
        auto overrideStr = std::string("Override:");
        auto splitStr = std::mismatch(overrideStr.begin(), overrideStr.end(), sectionsIter->begin());

        auto overrideAStr = std::string("Override.A:");
        auto splitAStr = std::mismatch(overrideAStr.begin(), overrideAStr.end(), sectionsIter->begin());

        if (*sectionsIter == std::string("Attach")) {
            // Get section contents
            auto sectionMap = configReader.Section(*sectionsIter);
            for (auto& valuesIter : sectionMap) {
                auto& boneName = valuesIter.first;
                auto& attachName = valuesIter.second;
                boneNames.push_back(boneName);
                // Find specified bone section and insert map values into config
                if (sections.find(attachName) != sections.end()) {
                    auto attachMapSection = configReader.Section(attachName);
                    for (auto& attachIter : attachMapSection) {
                        auto& keyName = attachIter.first;
                        config[boneName][keyName] = configReader.GetFloat(attachName, keyName, 0.0);
                    }
                }
            }
        }
        else if (*sectionsIter == std::string("Attach.A") && detectArmor) {
            // Get section contents
            auto sectionMap = configReader.Section(*sectionsIter);
            for (auto &valuesIter : sectionMap) {
                auto &boneName = valuesIter.first;
                auto &attachName = valuesIter.second;
                boneNames.push_back(boneName);
                // Find specified bone section and insert map values into configArmor
                if (sections.find(attachName) != sections.end()) {
                    auto attachMapSection = configReader.Section(attachName);
                    for (auto &attachIter : attachMapSection) {
                        auto& keyName = attachIter.first;
                        configArmor[boneName][keyName] = configReader.GetFloat(attachName, keyName, 0.0);
                    }
                }
            }
        }
        else if (*sectionsIter == std::string("Whitelist") && useWhitelist) {
            whitelist.clear();
            raceWhitelist.clear();

            // Get section contents
            auto sectionMap = configReader.Section(*sectionsIter);
            for (auto& valuesIter : sectionMap) {
                auto& boneName      = valuesIter.first;
                auto& whitelistName = valuesIter.second;

                size_t commaPos;
                do {
                    commaPos = whitelistName.find_first_of(",");
                    auto token = whitelistName.substr(0, commaPos);
                    size_t colonPos = token.find_last_of(":");
                    auto raceName = token.substr(0, colonPos);
                    auto genderStr = token.substr(colonPos + 1);

                    if (colonPos == -1) {
                        whitelist[boneName][token].male = true;
                        whitelist[boneName][token].female = true;
                        raceWhitelist.push_back(token);
                    }
                    else if (genderStr == "male") {
                        whitelist[boneName][raceName].male = true;
                        raceWhitelist.push_back(raceName);
                    }
                    else if (genderStr == "female") {
                        whitelist[boneName][raceName].female = true;
                        raceWhitelist.push_back(raceName);
                    }
                    whitelistName = whitelistName.substr(commaPos + 1);

                    //logger.Info("<token:> %s, <rest:> %s, <commaPos:> %d, <colonPos:> %d\n", token.c_str(), whitelistName.c_str(), commaPos >= 0, colonPos < 0);
                } while (commaPos != -1);
            }
        }
        else if (splitStr.first == overrideStr.end()) {
            // If section name is prefixed with "Override:", grab other half of name for bone
            auto boneName = std::string(splitStr.second, sectionsIter->end());

            // Get section contents
            auto sectionMap = configReader.Section(*sectionsIter); 
            for (auto &valuesIt : sectionMap) {
                configOverrides[boneName][valuesIt.first] = configReader.GetFloat(*sectionsIter, valuesIt.first, 0.0);
            }
        }
        else if (splitAStr.first == overrideAStr.end()) {
            // If section name is prefixed with "Override:", grab other half of name for bone
            auto boneName = std::string(splitAStr.second, sectionsIter->end());

            // Get section contents
            auto sectionMap = configReader.Section(*sectionsIter);
            for (auto& valuesIt : sectionMap) {
                configArmorOverrides[boneName][valuesIt.first] = configReader.GetFloat(*sectionsIter, valuesIt.first, 0.0);
            }
        }
    }

    // replace configs with override settings (if any)
    for (auto &boneIter : configOverrides) {
        if (config.count(boneIter.first) > 0) {
            for (auto settingIter : boneIter.second) {
                config[boneIter.first][settingIter.first] = settingIter.second;
            }
        }
    }

    // replace armor configs with override settings (if any)
    for (auto& boneIter : configArmorOverrides) {
        if (configArmor.count(boneIter.first) > 0) {
            for (auto settingIter : boneIter.second) {
                configArmor[boneIter.first][settingIter.first] = settingIter.second;
            }
        }
    }

    // Remove duplicate entries
    bonesSet = std::set<std::string>(boneNames.begin(), boneNames.end());
    boneNames.assign(bonesSet.begin(), bonesSet.end());

    // Check if physic_distance exists in INI, if not add it with default value
    // deprecating 20260125 RM // CheckAndAddMissingINIEntries();
    
    // Update file modification times after loading
    GetFileModificationTime("Data\\F4SE\\Plugins\\ocbp.ini", &lastMainINITime);
    GetFileModificationTime("Data\\MCM\\Settings\\OCBP.ini", &lastMCMINITime);
    
    logger.Error("CONFIG: Finished CBP Config\n");
    return reloadActors;
}

void DumpConfigToLog()
{
    // Log contents of config
    logger.Info("CONFIG: ***** Config Dump *****\n");
    for (auto section : config) {
        logger.Info("CONFIG: [%s]\n", section.first.c_str());
        for (auto setting : section.second) {
            logger.Info("CONFIG: %s=%f\n", setting.first.c_str(), setting.second);
        }
    }

    logger.Info("CONFIG: ***** ConfigArmor Dump *****\n");
    for (auto section : configArmor) {
        logger.Info("CONFIG: [%s]\n", section.first.c_str());
        for (auto setting : section.second) {
            logger.Info("CONFIG: %s=%f\n", setting.first.c_str(), setting.second);
        }
    }
}

void DumpWhitelistToLog() {
    logger.Info("CONFIG: ***** Whitelist Dump *****\n");
    for (auto section : whitelist) {
        logger.Info("CONFIG: [%s]\n", section.first.c_str());
        for (auto setting : section.second) {
            logger.Info("CONFIG: %s= female: %d, male: %d\n", setting.first.c_str(), setting.second.female, setting.second.male);
        }
    }
}

bool GetFileModificationTime(const char* filepath, FILETIME* fileTime) {
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    if (GetFileAttributesExA(filepath, GetFileExInfoStandard, &fileInfo)) {
        *fileTime = fileInfo.ftLastWriteTime;
        return true;
    }
    return false;
}

bool CheckConfigFilesChanged() {
    const char* mainINIPath = "Data\\F4SE\\Plugins\\ocbp.ini";
    const char* mcmINIPath = "Data\\MCM\\Settings\\OCBP.ini";
    
    FILETIME currentMainTime = {0};
    FILETIME currentMCMTime = {0};
    
    bool mainINIChanged = false;
    bool mcmINIChanged = false;
    
    if (GetFileModificationTime(mainINIPath, &currentMainTime)) {
        if (CompareFileTime(&currentMainTime, &lastMainINITime) != 0) {
            mainINIChanged = true;
            logger.Info("CONFIG: Main INI file changed, reloading config\n");
        }
    }
    
    if (GetFileModificationTime(mcmINIPath, &currentMCMTime)) {
        if (CompareFileTime(&currentMCMTime, &lastMCMINITime) != 0) {
            mcmINIChanged = true;
            logger.Info("CONFIG: MCM INI file changed, reloading config\n");
        }
    }
    
    return mainINIChanged || mcmINIChanged;
}