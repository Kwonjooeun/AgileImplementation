#pragma once
#include "M_MINE_TYPES.h"
namespace MINEASMALM {
	class CMineDynamics
	{
	public:
		void SetLayingPosition(SPOINT_WEAPON_ENU i_stLayingPos);
		void SetLaunchPosition(SPOINT_WEAPON_ENU i_stLaunchPos);
		//void SetMineWayPointInfo(HCI_M_MINE_WAYPOINT_CMD i_stWP);

	private:
		TrajInfo m_stTrajInfo = { 0, };
	};
}// namespace AIEP
