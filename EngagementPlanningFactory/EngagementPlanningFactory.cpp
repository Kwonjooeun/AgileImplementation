#include "EngagementPlanningFactory.h"
#include "EngagementManagers/M_MINE/MineEngagementManager.h"
#include "EngagementManagers/Missile/CMslEngagementManager.h"
#include "../Common/Utils/DebugPrint.h"
#include "../Common/Utils/ConfigManager.h"

namespace AIEP {

    // 지원되는 무장 종류들 정의
    const std::set<uint32_t> s_supportedWeaponKinds = {
        static_cast<uint32_t>(EN_WPN_KIND::WPN_KIND_M_MINE),   // 자항기뢰
        static_cast<uint32_t>(EN_WPN_KIND::WPN_KIND_ALM),      // 잠대지유도탄
        static_cast<uint32_t>(EN_WPN_KIND::WPN_KIND_ASM),      // 잠대함유도탄
        static_cast<uint32_t>(EN_WPN_KIND::WPN_KIND_AAM),      // 잠대공유도탄
        static_cast<uint32_t>(EN_WPN_KIND::WPN_KIND_WGT)       // 선유도중어뢰
    };

    EngagementPlanningFactory& EngagementPlanningFactory::GetInstance() {
        static EngagementPlanningFactory instance;
        return instance;
    }

    std::unique_ptr<IEngagementManager> EngagementPlanningFactory::CreateEngagementManager(
        ST_WA_SESSION weaponAssignInfo,
        std::shared_ptr<AIEP::DdsComm> ddsComm) {

        if (!ddsComm) {
            DEBUG_ERROR_STREAM(WEAPONFACTORY) << "DdsComm cannot be null" << std::endl;
            return nullptr;
        }
        uint32_t weaponKind = weaponAssignInfo.enWeaponType();
        int tubeNumber = weaponAssignInfo.enTubeNum();

        if (!IsWeaponKindSupported(weaponKind)) {
            DEBUG_ERROR_STREAM(WEAPONFACTORY) << "Unsupported weapon kind: "
                << weaponKind << std::endl;
            return nullptr;
        }


        // ConfigManager에서 무장 지원 여부 확인
        auto& configMgr = ConfigManager::GetInstance();
        if (!configMgr.IsWeaponSupported(weaponKind)) {
            DEBUG_ERROR_STREAM(WEAPONFACTORY) << "Weapon kind not configured: "
                << weaponKind << std::endl;
            return nullptr;
        }

        DEBUG_STREAM(WEAPONFACTORY) << "Creating EngagementManager for Tube " << tubeNumber
            << ", Weapon: " << configMgr.GetWeaponName(weaponKind)
            << " (" << weaponKind << ")" << std::endl;

        try {
            std::unique_ptr<IEngagementManager> manager;

            switch (static_cast<EN_WPN_KIND>(weaponKind)) {
            case EN_WPN_KIND::WPN_KIND_M_MINE:
                manager = CreateMineEngagementManager(weaponAssignInfo, ddsComm);
                break;

            case EN_WPN_KIND::WPN_KIND_ALM:
            case EN_WPN_KIND::WPN_KIND_ASM:
            case EN_WPN_KIND::WPN_KIND_AAM:
            case EN_WPN_KIND::WPN_KIND_WGT:
                manager = CreateMissileEngagementManager(weaponAssignInfo, ddsComm);
                break;

            default:
                DEBUG_ERROR_STREAM(WEAPONFACTORY) << "Unhandled weapon kind in factory: "
                    << static_cast<int>(weaponKind) << std::endl;
                return nullptr;
            }

            if (manager) {
                DEBUG_STREAM(WEAPONFACTORY) << "Successfully created EngagementManager for Tube "
                    << tubeNumber << ", Weapon: "
                    << static_cast<int>(weaponKind) << std::endl;
            }
            else {
                DEBUG_ERROR_STREAM(WEAPONFACTORY) << "Failed to create EngagementManager for Tube "
                    << tubeNumber << ", Weapon: "
                    << static_cast<int>(weaponKind) << std::endl;
            }

            return manager;
        }
        catch (const std::exception& e) {
            DEBUG_ERROR_STREAM(WEAPONFACTORY) << "Exception creating EngagementManager: " << e.what() << std::endl;
            return nullptr;
        }
    }

    bool EngagementPlanningFactory::IsWeaponKindSupported(uint32_t weaponKind) const {
        return s_supportedWeaponKinds.find(weaponKind) != s_supportedWeaponKinds.end();
    }

    std::unique_ptr<IEngagementManager> EngagementPlanningFactory::CreateMineEngagementManager(
        ST_WA_SESSION weaponAssignInfo, std::shared_ptr<AIEP::DdsComm> ddsComm) {

        try {
            std::unique_ptr<IEngagementManager> manager = std::make_unique<MineEngagementManager>(weaponAssignInfo, ddsComm);

            DEBUG_STREAM(WEAPONFACTORY) << "MineEngagementManager created for Tube " << weaponAssignInfo.enTubeNum() << std::endl;

            return std::move(manager);
        }
        catch (const std::exception& e) {
            DEBUG_ERROR_STREAM(WEAPONFACTORY) << "Failed to create MineEngagementManager: " << e.what() << std::endl;
            return nullptr;
        }
    }

    std::unique_ptr<IEngagementManager> EngagementPlanningFactory::CreateMissileEngagementManager(
        ST_WA_SESSION weaponAssignInfo, std::shared_ptr<AIEP::DdsComm> ddsComm){
        auto manager = std::make_unique<CMslEngagementManager>(weaponAssignInfo, ddsComm);
        DEBUG_STREAM(WEAPONFACTORY) << "MissileEngagementManager created for Tube " << weaponAssignInfo.enTubeNum() << std::endl;
        return std::move(manager);
    }

} // namespace AIEP
