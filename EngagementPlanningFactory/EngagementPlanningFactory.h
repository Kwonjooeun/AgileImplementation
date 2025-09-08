#pragma once

#include <memory>
#include <functional>
#include <map>

#include "EngagementManagers/IEngagementManager.h"

namespace AIEP {
	class EngagementPlanningFactory
	{
	public:

		static EngagementPlanningFactory& GetInstance();

        // 교전계획 관리자 생성
        std::unique_ptr<IEngagementManager> CreateEngagementManager(
            ST_WA_SESSION weaponAssignInfo,
            std::shared_ptr<AIEP::DdsComm> ddsComm
        );

        /**
         * @brief 무장 지원 여부 확인
         * @param weaponKind 무장 종류
         * @return 지원 여부
         */
        //bool IsWeaponKindSupported(EN_WPN_KIND weaponKind);

        /**
         * @brief 무장 종류 이름 반환
         * @param weaponKind 무장 종류
         * @return 무장 종류 이름
         */
        //std::string GetWeaponTypeName(EN_WPN_KIND weaponKind);

    private:
        // Singleton 패턴
        EngagementPlanningFactory() = default;
        ~EngagementPlanningFactory() = default;
        EngagementPlanningFactory(const EngagementPlanningFactory&) = delete;
        EngagementPlanningFactory& operator=(const EngagementPlanningFactory&) = delete;

        // 내부 팩토리 메서드들
        std::unique_ptr<IEngagementManager> CreateMineEngagementManager(
            ST_WA_SESSION weaponAssignInfo, std::shared_ptr<AIEP::DdsComm> ddsComm);

        std::unique_ptr<IEngagementManager> CreateMissileEngagementManager(
            ST_WA_SESSION weaponAssignInfo, std::shared_ptr<AIEP::DdsComm> ddsComm);

        //static const std::set<EN_WPN_KIND> s_supportedWeaponKinds;
        bool IsWeaponKindSupported(uint32_t weaponKind) const;

    };
} // namespace AIEP
