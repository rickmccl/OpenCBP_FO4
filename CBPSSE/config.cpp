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

int configReloadCount = 60;
bool playerOnly = false;
bool femaleOnly = false;
bool maleOnly = false;
bool npcOnly = false;
bool detectArmor = false;
bool useWhitelist = false;
bool loggingEnabled = true;
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
    logger.Info("loadConfig\n");

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
        logger.Error("Can't load 'ocbp.ini'\n");
    }
    logger.Error("Reading CBP Config\n");

    // Check for MCM override settings
    INIReader mcmReader("Data\\MCM\\Settings\\OCBP.ini");
    bool hasMCM = (mcmReader.ParseError() >= 0);
    if (hasMCM) {
        logger.Error("Found MCM settings, applying overrides\n");
    }

    // Read logging setting first (affects subsequent log output)
    loggingEnabled = configReader.GetBoolean("General", "loggingEnabled", true);
    logger.SetLoggingEnabled(loggingEnabled);
    logger.Info("Logging: %s\n", loggingEnabled ? "enabled" : "disabled");

    // Read general settings with MCM overrides
    if (hasMCM) {
        int actorFilter = mcmReader.GetInteger("General", "iActorFilter", -1);
        int genderFilter = mcmReader.GetInteger("General", "iGenderFilter", -1);
        
        if (actorFilter >= 0) {
            switch (actorFilter) {
                case 0:
                    playerOnly = false;
                    npcOnly = false;
                    break;
                case 1:
                    playerOnly = true;
                    npcOnly = false;
                    break;
                case 2:
                    playerOnly = false;
                    npcOnly = true;
                    break;
                default:
                    playerOnly = configReader.GetBoolean("General", "playerOnly", false);
                    npcOnly = configReader.GetBoolean("General", "npcOnly", false);
                    break;
            }
        } else {
            playerOnly = configReader.GetBoolean("General", "playerOnly", false);
            npcOnly = configReader.GetBoolean("General", "npcOnly", false);
        }
        
        if (genderFilter >= 0) {
            switch (genderFilter) {
                case 0:
                    femaleOnly = false;
                    maleOnly = false;
                    useWhitelist = false;
                    break;
                case 1:
                    femaleOnly = true;
                    maleOnly = false;
                    useWhitelist = false;
                    break;
                case 2:
                    femaleOnly = false;
                    maleOnly = true;
                    useWhitelist = false;
                    break;
                case 3:
                    femaleOnly = false;
                    maleOnly = false;
                    useWhitelist = true;
                    break;
                default:
                    femaleOnly = configReader.GetBoolean("General", "femaleOnly", false);
                    maleOnly = configReader.GetBoolean("General", "maleOnly", false);
                    useWhitelist = configReader.GetBoolean("General", "useWhitelist", false);
                    break;
            }
        } else {
            femaleOnly = configReader.GetBoolean("General", "femaleOnly", false);
            maleOnly = configReader.GetBoolean("General", "maleOnly", false);
            useWhitelist = configReader.GetBoolean("General", "useWhitelist", false);
        }
    } else {
        playerOnly = configReader.GetBoolean("General", "playerOnly", false);
        npcOnly = configReader.GetBoolean("General", "npcOnly", false);
        femaleOnly = configReader.GetBoolean("General", "femaleOnly", false);
        maleOnly = configReader.GetBoolean("General", "maleOnly", false);
        useWhitelist = configReader.GetBoolean("General", "useWhitelist", false);
    }

    reloadActors = (playerOnly ^ playerOnlyOld) ||
                    (femaleOnly ^ femaleOnlyOld) ||
                    (maleOnly ^ maleOnlyOld) ||
                    (npcOnly ^ npcOnlyOld) ||
                    (useWhitelist ^ useWhitelistOld);

    detectArmor = hasMCM ? mcmReader.GetBoolean("General", "bDetectArmor", configReader.GetBoolean("General", "detectArmor", false)) : configReader.GetBoolean("General", "detectArmor", false);
    physic_distance_enable = hasMCM ? mcmReader.GetFloat("General", "fPhysicDistanceEnable", configReader.GetFloat("General", "physic_distance_enable", 7500.0f)) : configReader.GetFloat("General", "physic_distance_enable", 7500.0f);
    physic_distance_disable = hasMCM ? mcmReader.GetFloat("General", "fPhysicDistanceDisable", configReader.GetFloat("General", "physic_distance_disable", 8500.0f)) : configReader.GetFloat("General", "physic_distance_disable", 8500.0f);
    max_active_actors = hasMCM ? mcmReader.GetInteger("General", "iMaxActiveActors", configReader.GetInteger("General", "max_active_actors", 10)) : configReader.GetInteger("General", "max_active_actors", 10);
    autoMode = hasMCM ? mcmReader.GetInteger("General", "iAutoMode", 0) : 0;
    targetFPS = hasMCM ? mcmReader.GetInteger("General", "iTargetFPS", 60) : 60;
    autoExceptions = hasMCM ? mcmReader.GetInteger("General", "iAutoExceptions", 1) : 1;
    
    // Validate distance settings
    if (physic_distance_disable <= physic_distance_enable) {
        logger.Error("WARNING: physic_distance_disable (%f) must be greater than physic_distance_enable (%f)\n", 
                     physic_distance_disable, physic_distance_enable);
        logger.Error("Auto-correcting: setting physic_distance_disable to %f\n", physic_distance_enable + 1000.0f);
        physic_distance_disable = physic_distance_enable + 1000.0f;
    }
    
    // Validate max actors setting
    if (max_active_actors < 1) {
        logger.Error("WARNING: max_active_actors (%d) must be at least 1. Setting to 1\n", max_active_actors);
        max_active_actors = 1;
    } else if (max_active_actors > 50) {
        logger.Error("WARNING: max_active_actors (%d) is very high! This may cause performance issues\n", max_active_actors);
    }
    
    logger.Info("Physics settings: enable=%.1f, disable=%.1f, max_actors=%d\n", 
                physic_distance_enable, physic_distance_disable, max_active_actors);
    
    configReloadCount = configReader.GetInteger("Tuning", "rate", 0);

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
    CheckAndAddMissingINIEntries();
    
    // Update file modification times after loading
    GetFileModificationTime("Data\\F4SE\\Plugins\\ocbp.ini", &lastMainINITime);
    GetFileModificationTime("Data\\MCM\\Settings\\OCBP.ini", &lastMCMINITime);
    
    logger.Error("Finished CBP Config\n");
    return reloadActors;
}

void DumpConfigToLog()
{
    // Log contents of config
    logger.Info("***** Config Dump *****\n");
    for (auto section : config) {
        logger.Info("[%s]\n", section.first.c_str());
        for (auto setting : section.second) {
            logger.Info("%s=%f\n", setting.first.c_str(), setting.second);
        }
    }

    logger.Info("***** ConfigArmor Dump *****\n");
    for (auto section : configArmor) {
        logger.Info("[%s]\n", section.first.c_str());
        for (auto setting : section.second) {
            logger.Info("%s=%f\n", setting.first.c_str(), setting.second);
        }
    }
}

void CheckAndAddMissingINIEntries() {
    const char* iniPath = "Data\\F4SE\\Plugins\\ocbp.ini";
    
    // Read current INI to check if physic_distance exists
    INIReader reader(iniPath);
    if (reader.ParseError() < 0) {
        logger.Error("Can't check INI for missing entries\n");
        return;
    }
    
    // Check if physic distance entries exist in [General] section
    auto physicDistanceEnableStr = reader.Get("General", "physic_distance_enable", "");
    auto physicDistanceDisableStr = reader.Get("General", "physic_distance_disable", "");
    auto maxActiveActorsStr = reader.Get("General", "max_active_actors", "");
    
    std::string missingEntries = "";
    
    // Check if comments header already exists in the file
    std::ifstream file(iniPath);
    std::string line;
    bool hasPhysicsHeader = false;
    if (file.is_open()) {
        while (std::getline(file, line)) {
            if (line.find("===== PHYSICS DISTANCE SETTINGS =====") != std::string::npos) {
                hasPhysicsHeader = true;
                break;
            }
        }
        file.close();
    }
    
    // Add explanatory header if any entries are missing AND header doesn't exist
    bool needsEntries = physicDistanceEnableStr.empty() || physicDistanceDisableStr.empty() || maxActiveActorsStr.empty();
    bool needsHeader = needsEntries && !hasPhysicsHeader;
    
    if (needsHeader) {
        missingEntries += "\n; ===== PHYSICS DISTANCE SETTINGS =====";
        missingEntries += "\n; Controls when CBP physics are active based on distance from player";
        missingEntries += "\n; Smaller values = better performance, larger values = physics work farther away";
    }
    
    if (physicDistanceEnableStr.empty()) {
        missingEntries += "\n; Distance at which physics STARTS working (NPCs closer than this = physics ON)";
        missingEntries += "\nphysic_distance_enable=7500.0";
    }
    if (physicDistanceDisableStr.empty()) {
        missingEntries += "\n; Distance at which physics STOPS working (NPCs farther than this = physics OFF)";
        missingEntries += "\n; MUST be greater than physic_distance_enable to prevent performance issues!";
        missingEntries += "\nphysic_distance_disable=8500.0";
    }
    if (maxActiveActorsStr.empty()) {
        missingEntries += "\n; Maximum number of NPCs that can have active physics at the same time";
        missingEntries += "\n; Lower values = better performance in crowded areas (Diamond City etc.)";
        missingEntries += "\nmax_active_actors=10";
    }
    
    if (!missingEntries.empty()) {
        // Some entries don't exist, add them
        logger.Info("Adding missing physics distance entries to INI\n");
        
        // Read entire INI file
        std::ifstream iniFile(iniPath);
        std::string content((std::istreambuf_iterator<char>(iniFile)), std::istreambuf_iterator<char>());
        iniFile.close();
        
        // Find [General] section and add missing entries
        size_t generalPos = content.find("[General]");
        if (generalPos != std::string::npos) {
            // Find next section or end of file
            size_t nextSectionPos = content.find("\n[", generalPos + 9);
            if (nextSectionPos == std::string::npos) {
                nextSectionPos = content.length();
            }
            
            // Insert missing entries before next section
            content.insert(nextSectionPos, missingEntries);
            
            // Write back to file
            std::ofstream outFile(iniPath);
            outFile << content;
            outFile.close();
            
            logger.Info("Successfully added physics distance entries to [General] section\n");
        } else {
            logger.Error("Could not find [General] section in INI file\n");
        }
    }
}

void DumpWhitelistToLog() {
    logger.Info("***** Whitelist Dump *****\n");
    for (auto section : whitelist) {
        logger.Info("[%s]\n", section.first.c_str());
        for (auto setting : section.second) {
            logger.Info("%s= female: %d, male: %d\n", setting.first.c_str(), setting.second.female, setting.second.male);
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
            logger.Info("Main INI file changed, reloading config\n");
        }
    }
    
    if (GetFileModificationTime(mcmINIPath, &currentMCMTime)) {
        if (CompareFileTime(&currentMCMTime, &lastMCMINITime) != 0) {
            mcmINIChanged = true;
            logger.Info("MCM INI file changed, reloading config\n");
        }
    }
    
    return mainINIChanged || mcmINIChanged;
}