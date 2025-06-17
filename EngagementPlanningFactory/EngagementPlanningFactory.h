#include <memory>
#include <functional>
#include <map>

#include "Weapons/IWeapon.h"
#include "EngagementManagers/EngagementManagerBase.h"

namespace MINEASMALM {
	class EngagementPlanningFactory
	{
	public:
		static EngagementPlanningFactory& GetInstance();
		EngagementManagerBase CreateEngagementManager(EN_WPN_KIND WeaponKind);
	};


} // namespace MINEASMALM
