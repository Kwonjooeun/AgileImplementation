#include "EngagementPlanningFactory.h"
#include "EngagementManagers/M_MINE/MineEngagementManager.h"
#include "EngagementManagers/Missile/MissileEngagementManager.h"
#include "../Common/Utils/DebugPrint.h"
#include "../Common/Utils/ConfigManager.h"

namespace AIEP {

    // 지원되는 무장 종류들 정의
    const std::set<EN_WPN_KIND> s_supportedWeaponKinds = {
        EN_WPN_KIND::WPN_KIND_M_MINE,   // 자항기뢰
        EN_WPN_KIND::WPN_KIND_ALM,      // 잠대지유도탄
        EN_WPN_KIND::WPN_KIND_ASM,      // 잠대함유도탄
        EN_WPN_KIND::WPN_KIND_AAM,      // 잠대공유도탄
        EN_WPN_KIND::WPN_KIND_WGT       // 선유도중어뢰
    };

    EngagementPlanningFactory& EngagementPlanningFactory::GetInstance() {
        static EngagementPlanningFactory instance;
        return instance;
    }

    std::unique_ptr<IEngagementManager> EngagementPlanningFactory::CreateEngagementManager(
        WeaponAssignmentInfo weaponAssignInfo,
        std::shared_ptr<DdsComm> ddsComm) {

        if (!ddsComm) {
            DEBUG_ERROR_STREAM(WEAPONFACTORY) << "DdsComm cannot be null" << std::endl;
            return nullptr;
        }
        EN_WPN_KIND weaponKind = weaponAssignInfo.weaponKind;
        int tubeNumber = weaponAssignInfo.tubeNumber;

        if (!IsWeaponKindSupported(weaponKind)) {
            DEBUG_ERROR_STREAM(WEAPONFACTORY) << "Unsupported weapon kind: "
                << static_cast<int>(weaponKind) << std::endl;
            return nullptr;
        }

        // ConfigManager에서 무장 지원 여부 확인
        auto& configMgr = ConfigManager::GetInstance();
        if (!configMgr.IsWeaponSupported(weaponKind)) {
            DEBUG_ERROR_STREAM(WEAPONFACTORY) << "Weapon kind not configured: "
                << static_cast<int>(weaponKind) << std::endl;
            return nullptr;
        }

        DEBUG_STREAM(WEAPONFACTORY) << "Creating EngagementManager for Tube " << tubeNumber
            << ", Weapon: " << configMgr.GetWeaponName(weaponKind)
            << " (" << static_cast<int>(weaponKind) << ")" << std::endl;

        try {
            std::unique_ptr<IEngagementManager> manager;

            switch (weaponKind) {
            case EN_WPN_KIND::WPN_KIND_M_MINE:
                manager = CreateMineEngagementManager(weaponAssignInfo, ddsComm);
                break;

            case EN_WPN_KIND::WPN_KIND_ALM:
            case EN_WPN_KIND::WPN_KIND_ASM:
            case EN_WPN_KIND::WPN_KIND_AAM:
            case EN_WPN_KIND::WPN_KIND_WGT:
                //manager = CreateMissileEngagementManager(weaponAssignInfo, ddsComm);
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

    bool EngagementPlanningFactory::IsWeaponKindSupported(EN_WPN_KIND weaponKind) const {
        return s_supportedWeaponKinds.find(weaponKind) != s_supportedWeaponKinds.end();
    }

    std::unique_ptr<IEngagementManager> EngagementPlanningFactory::CreateMineEngagementManager(
        WeaponAssignmentInfo weaponAssignInfo, std::shared_ptr<DdsComm> ddsComm) {

        try {
            std::unique_ptr<IEngagementManager> manager = std::make_unique<MineEngagementManager>(weaponAssignInfo, ddsComm);

            DEBUG_STREAM(WEAPONFACTORY) << "MineEngagementManager created for Tube " << weaponAssignInfo.tubeNumber << std::endl;

            return std::move(manager);
        }
        catch (const std::exception& e) {
            DEBUG_ERROR_STREAM(WEAPONFACTORY) << "Failed to create MineEngagementManager: " << e.what() << std::endl;
            return nullptr;
        }
    }
}
