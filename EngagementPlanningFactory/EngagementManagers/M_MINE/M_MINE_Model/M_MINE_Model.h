#pragma once
#include <memory>
#include <functional>
#include "M_MINE_TYPES.h"
#include "../../utils/CCalcMethod.h"
#include "../../../../Common/Utils/ConfigManager.h"
namespace AIEP {

	class M_MINE_Model
	{
	public:
		M_MINE_Model();
		~M_MINE_Model();

		void SetDropPosition(const SPOINT_WEAPON_ENU i_stDropPos);
		void SetLaunchPosition(const SPOINT_WEAPON_ENU i_stLaunchPos);
		void SetMineWayPointInfo(const std::vector<SPOINT_WEAPON_ENU>& i_stWP);

		void SetFullRoutePoints(const std::vector<SPOINT_WEAPON_ENU>& i_stFullRoutes);

		// dead reckoning을 고려하지 않은 교전계획 산출 - 궤적만 산출 (position만 계산)
		// 현재 위치(currentPos)에서 목표(o_NextWPToGo)까지 단위시간(unitTIme)만큼 기동 후의 위치를 반환
		bool runWaypoints(const float unitTIme, int& o_NextWPToGo, SPOINT_ENU& currentPos);
		void runKinematicTowardWaypoint(const float unitTIme, int& o_NextWPToGo, bool& o_bWaypointReached, SPOINT_ENU& currentPos);

	private:
		WeaponSpecification m_weaponSpec;

		SPOINT_WEAPON_ENU m_DropPos;
		SPOINT_WEAPON_ENU m_LaunchPos;
		std::vector< SPOINT_WEAPON_ENU> m_ENUwaypoints;

		std::vector<SPOINT_WEAPON_ENU> m_FullRoutePoints; // Launch point - Waypoints - Drop point / 경로 산출에 쓰이므로, valid point만 저장
		uint32_t m_weaponKind;
	};
}
