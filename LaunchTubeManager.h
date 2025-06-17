#pragma once
#include "Common/Utils/DebugPrint.h"
#include "EngagementPlanningFactory/EngagementPlanningFactory.h"
#include "dds_message/AIEP_AIEP_.hpp"
#include <memory>
#include <mutex>
#include <atomic>

namespace MINEASMALM {

	class LaunchTubeManager
	{
	public:
		explicit LaunchTubeManager(int tubeNumber);
		~LaunchTubeManager();

		// 초기화 및 종료
		bool Initialize();
		void Shutdown();

		// 무장 할당 관리
		bool AssignWeapon(const TEWA_ASSIGN_CMD& assignCmd);
		bool UnassignWeapon();

	private:
		// 멤버 변수
		int m_tubeNumber;
		std::atomic<bool> m_initialized;

		// 할당 정보
		TEWA_ASSIGN_CMD m_assignmentInfo;
		bool m_isAssigned;
		EN_WPN_KIND m_weaponKind;

		// 환경 정보
		NAVINF_SHIP_NAVIGATION_INFO m_ownShipInfo;
		std::map<uint32_t, TRKMGR_SYSTEMTARGET_INFO> m_targetInfoMap;

		// 상태
		std::atomic<bool> m_shutdown;

		// 동기화
		mutable std::mutex m_mutex;
	};
} // namespace MINEASMALM
