#include "ConfigManager.h"
#include "DebugPrint.h"

#include <iostream>
#include <algorithm>
#include <filesystem>

namespace MINEASMALM 
{
    // 기본 무장 제원 정의
    const WeaponSpecification ConfigManager::s_defaultWeaponSpec;

    ConfigManager& ConfigManager::GetInstance() {
        static ConfigManager instance;
        return instance;
    }

    bool ConfigManager::LoadFromFile(const std::string& configFilePath) {
        std::lock_guard<std::mutex> lock(m_mutex);

        ConfigReader config;
        if (!config.LoadFromFile(configFilePath)) {
            DEBUG_ERROR_STREAM(CONFIGMANAGER) << "Failed to load configuration file: " << configFilePath << std::endl;
            return false;
        }

        try {
            // 각 섹션별 설정 로드
            LoadSystemInfraConfig(config);
            LoadBusinessLogicConfig(config);
            LoadWeaponSpecs(config);

            m_loaded = true;

            DEBUG_STREAM(CONFIGMANAGER) << "ConfigManager: All configurations loaded successfully from " << configFilePath << std::endl;
            DEBUG_STREAM(CONFIGMANAGER) << "- System infrastructure settings loaded" << std::endl;
            DEBUG_STREAM(CONFIGMANAGER) << "- Business logic settings loaded" << std::endl;
            DEBUG_STREAM(CONFIGMANAGER) << "- Weapon specifications loaded: " << m_weaponSpecs.size() << " types" << std::endl;

            return true;
        }
        catch (const std::exception& e) {
            DEBUG_ERROR_STREAM(CONFIGMANAGER) << "Error loading configuration: " << e.what() << std::endl;
            return false;
        }
    }

    void ConfigManager::LoadSystemInfraConfig(const ConfigReader& config) {
        // 시스템 인프라 설정 로드
        m_systemInfraConfig.totalTubes = config.GetInt("System", "TotalTubes", 6);

        // DDS 설정
        m_systemInfraConfig.ddsDomainId = config.GetInt("Network", "DDSDomainId", 83);
    }

    void ConfigManager::LoadBusinessLogicConfig(const ConfigReader& config) {
        // 업데이트 주기 설정
        m_businessLogicConfig.engagementPlanUpdateInterval_sec = config.GetDouble("BusinessLogic", "EngagementPlanUpdateInterval", 1.0);
        m_businessLogicConfig.weaponStatusUpdateInterval_sec = config.GetDouble("BusinessLogic", "WeaponStatusUpdateInterval", 1.0);
    }

    void ConfigManager::LoadWeaponSpecs(const ConfigReader& config) {
        // 시스템 섹션들 (무장이 아닌 섹션들)
        std::set<std::string> systemSections = {
            "System", "BusinessLogic", "Network", "Debug", "Logging", "DDS", "General"
        };

        // 모든 섹션을 가져와서 시스템 섹션이 아닌 것들을 무장으로 간주
        auto allSections = config.GetAllSections();

        //std::cout << "Scanning for weapon sections..." << std::endl;

        for (const std::string& sectionName : allSections) {
            // 시스템 섹션은 건너뛰기
            if (systemSections.find(sectionName) != systemSections.end()) {
                continue;
            }

            // 섹션 이름을 무장 종류로 변환
            EN_WPN_KIND weaponKind = StringToWeaponKind(sectionName);
            if (weaponKind == EN_WPN_KIND::WPN_KIND_NA) {             
                continue;
            }

            // 무장 제원 로드
            WeaponSpecification spec;

            spec.name = config.GetString(sectionName, "Name", sectionName);
            spec.maxRange_km = config.GetDouble(sectionName, "MaxRange", 50.0);
            spec.maxSpeed_mps = config.GetDouble(sectionName, "MaxSpeed", 100.0);
            spec.cruiseSpeed_mps = config.GetDouble(sectionName, "CruiseSpeed", 80.0);
            spec.launchDelay_sec = config.GetDouble(sectionName, "LaunchDelay", 3.0);
            spec.maxDepth_m = config.GetDouble(sectionName, "MaxDepth", 200.0);
            spec.maxAltitude_m = config.GetDouble(sectionName, "MaxAltitude", 1000.0);
            spec.maxWaypoints = config.GetInt(sectionName, "MaxWaypoints", 8);
            spec.requiresWaypoints = config.GetBool(sectionName, "RequiresWaypoints", true);
            spec.description = config.GetString(sectionName, "Description", "");

            // 자항기뢰는 특별 처리 (경로점 필요 없음)
            if (weaponKind == EN_WPN_KIND::WPN_KIND_M_MINE) {
                spec.requiresWaypoints = false;
            }

            m_weaponSpecs[weaponKind] = spec;

            DEBUG_STREAM(CONFIGMANAGER) << "Loaded weapon spec: " << spec.name
                << " [" << sectionName << "]"
                << " (Range: " << spec.maxRange_km << "km, "
                << "Speed: " << spec.maxSpeed_mps << "m/s)" << std::endl;
        }

        if (m_weaponSpecs.empty()) {
            DEBUG_WARNING_STREAM(CONFIGMANAGER) << "Warning: No weapon specifications loaded!" << std::endl;
        }
        else {
            DEBUG_STREAM(CONFIGMANAGER) << "Total weapons loaded: " << m_weaponSpecs.size() << std::endl;
        }
    }

    const SystemInfraConfig& ConfigManager::GetSystemInfraConfig() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_systemInfraConfig;
    }

    const BusinessLogicConfig& ConfigManager::GetBusinessLogicConfig() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_businessLogicConfig;
    }

    const WeaponSpecification& ConfigManager::GetWeaponSpec(EN_WPN_KIND weaponKind) const {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_weaponSpecs.find(weaponKind);
        if (it != m_weaponSpecs.end()) {
            return it->second;
        }

        return s_defaultWeaponSpec;
    }

    std::vector<EN_WPN_KIND> ConfigManager::GetSupportedWeaponKinds() const {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::vector<EN_WPN_KIND> supportedKinds;
        for (const auto& pair : m_weaponSpecs) {
            supportedKinds.push_back(pair.first);
        }

        return supportedKinds;
    }

    bool ConfigManager::IsWeaponSupported(EN_WPN_KIND weaponKind) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_weaponSpecs.find(weaponKind) != m_weaponSpecs.end();
    }

    std::string ConfigManager::GetWeaponName(EN_WPN_KIND weaponKind) const {
        const auto& spec = GetWeaponSpec(weaponKind);
        return spec.name;
    }

    void ConfigManager::PrintAllConfigs() const {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::cout << "\n========== System Infrastructure Configuration ==========" << std::endl;
        std::cout << "[Basic Settings]" << std::endl;
        std::cout << "  Total Tubes: " << m_systemInfraConfig.totalTubes << std::endl;

        std::cout << "\n[DDS Settings]" << std::endl;
        std::cout << "  Domain ID: " << m_systemInfraConfig.ddsDomainId << std::endl;

        std::cout << "\n========== Business Logic Configuration ==========" << std::endl;
        std::cout << "[Update Intervals]" << std::endl;
        std::cout << "  Engagement Plan Update: " << m_businessLogicConfig.engagementPlanUpdateInterval_sec << " sec" << std::endl;
        std::cout << "  Weapon Status Update: " << m_businessLogicConfig.weaponStatusUpdateInterval_sec << " sec" << std::endl;

        std::cout << "\n========== Weapon Specifications ==========" << std::endl;
        if (m_weaponSpecs.empty()) {
            std::cout << "No weapon specifications loaded." << std::endl;
        }
        else {
            for (const auto& pair : m_weaponSpecs) {
                const auto& spec = pair.second;
                std::cout << "\n[" << WeaponKindToString(pair.first) << "] " << spec.name << std::endl;
                std::cout << "  Max Range: " << spec.maxRange_km << " km" << std::endl;
                std::cout << "  Max Speed: " << spec.maxSpeed_mps << " m/s" << std::endl;
                std::cout << "  Cruise Speed: " << spec.cruiseSpeed_mps << " m/s" << std::endl;
                std::cout << "  Launch Delay: " << spec.launchDelay_sec << " sec" << std::endl;
                std::cout << "  Max Depth: " << spec.maxDepth_m << " m" << std::endl;
                std::cout << "  Max Altitude: " << spec.maxAltitude_m << " m" << std::endl;
                std::cout << "  Max Waypoints: " << spec.maxWaypoints << std::endl;
                std::cout << "  Requires Waypoints: " << (spec.requiresWaypoints ? "Yes" : "No") << std::endl;
                if (!spec.description.empty()) {
                    DEBUG_STREAM(CONFIGMANAGER) << "  Description: " << spec.description << std::endl;
                }
            }
        }

        std::cout << "=================================================================" << std::endl;
    }

    std::string ConfigManager::WeaponKindToString(EN_WPN_KIND kind) const {
        switch (kind) {
        case EN_WPN_KIND::WPN_KIND_ALM: return "ALM";
        case EN_WPN_KIND::WPN_KIND_ASM: return "ASM";
        case EN_WPN_KIND::WPN_KIND_AAM: return "AAM";
        case EN_WPN_KIND::WPN_KIND_WGT: return "WGT";
        case EN_WPN_KIND::WPN_KIND_M_MINE: return "MINE";
        default: return "UNKNOWN";
        }
    }

    EN_WPN_KIND ConfigManager::StringToWeaponKind(const std::string& str) const {
        std::string upperStr = str;
        std::transform(upperStr.begin(), upperStr.end(), upperStr.begin(), ::toupper);

        // 직접 매칭
        if (upperStr == "ALM") return EN_WPN_KIND::WPN_KIND_ALM;
        if (upperStr == "ASM") return EN_WPN_KIND::WPN_KIND_ASM;
        if (upperStr == "AAM") return EN_WPN_KIND::WPN_KIND_AAM;
        if (upperStr == "WGT") return EN_WPN_KIND::WPN_KIND_WGT;
        if (upperStr == "MINE") return EN_WPN_KIND::WPN_KIND_M_MINE;

        // 별칭들도 지원
        if (upperStr == "M_MINE" || upperStr == "MMINE") return EN_WPN_KIND::WPN_KIND_M_MINE;
        if (upperStr == "TORPEDO" || upperStr == "WGT_TORPEDO") return EN_WPN_KIND::WPN_KIND_WGT;
        if (upperStr == "ANTI_LAND" || upperStr == "LAND_ATTACK") return EN_WPN_KIND::WPN_KIND_ALM;
        if (upperStr == "ANTI_SHIP" || upperStr == "SHIP_ATTACK") return EN_WPN_KIND::WPN_KIND_ASM;
        if (upperStr == "ANTI_AIR" || upperStr == "AIR_DEFENSE") return EN_WPN_KIND::WPN_KIND_AAM;

        // 무장 이름으로도 매칭 시도
        if (upperStr.find("ALM") != std::string::npos || upperStr.find("LAND") != std::string::npos) {
            return EN_WPN_KIND::WPN_KIND_ALM;
        }
        if (upperStr.find("ASM") != std::string::npos || upperStr.find("SHIP") != std::string::npos) {
            return EN_WPN_KIND::WPN_KIND_ASM;
        }
        if (upperStr.find("AAM") != std::string::npos || upperStr.find("AIR") != std::string::npos) {
            return EN_WPN_KIND::WPN_KIND_AAM;
        }
        if (upperStr.find("TORPEDO") != std::string::npos || upperStr.find("WGT") != std::string::npos) {
            return EN_WPN_KIND::WPN_KIND_WGT;
        }
        if (upperStr.find("MINE") != std::string::npos) {
            return EN_WPN_KIND::WPN_KIND_M_MINE;
        }

        return EN_WPN_KIND::WPN_KIND_NA;
    }

} // namespace MINEASMALM
