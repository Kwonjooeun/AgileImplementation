#include <memory>
#include <functional>
#include <map>

#include "EngagementManagers/IEngagementManager.h"

namespace MINEASMALM {
	class EngagementPlanningFactory
	{
	public:

		static EngagementPlanningFactory& GetInstance();

        // 교전계획 관리자 생성
        std::unique_ptr<IEngagementManager> CreateEngagementManager(
            WeaponAssignmentInfo weaponAssignInfo,
            std::shared_ptr<DdsComm> ddsComm
        );

    private:
        // Singleton 패턴
        EngagementPlanningFactory() = default;
        ~EngagementPlanningFactory() = default;
        EngagementPlanningFactory(const EngagementPlanningFactory&) = delete;
        EngagementPlanningFactory& operator=(const EngagementPlanningFactory&) = delete;

        // 내부 팩토리 메서드들
        std::unique_ptr<IEngagementManager> CreateMineEngagementManager(
            WeaponAssignmentInfo weaponAssignInfo, std::shared_ptr<DdsComm> ddsComm);

        bool IsWeaponKindSupported(EN_WPN_KIND weaponKind) const;

    };
} // namespace AIEP
