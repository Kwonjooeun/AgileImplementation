#pragma once
#include "Common/Utils/DebugPrint.h"
#include "EngagementPlanningFactory/EngagementPlanningFactory.h"
#include "WpnStatusCtrl/WpnStatusCtrlManager.h"
#include "dds_message/AIEP_AIEP_.hpp"
#include <memory>
#include <mutex>
#include <atomic>

namespace MINEASMALM {

	class LaunchTubeManager
	{
	public:
		explicit LaunchTubeManager(int tubeNumber, std::shared_ptr<DdsComm> ddsComm);
		~LaunchTubeManager();

		// 초기화 및 종료
		bool Initialize();
		void Shutdown();

		// 적재정보 업데이트
		void UpdateLoadedWeaponKind(EN_WPN_KIND weaponKind);

		// 할당 가능 여부 확인
		bool CanAssignWeapon(const TEWA_ASSIGN_CMD& assignCmd) const;

		// 무장 할당 관리
		bool AssignWeapon(const TEWA_ASSIGN_CMD& assignCmd);
		bool UnassignWeapon();

		// 무장 상태 통제
		bool ProcessWeaponControlCommand(const CMSHCI_AIEP_WPN_CTRL_CMD& command);

	private:
		// 멤버 변수
		int m_tubeNumber;
		std::shared_ptr<DdsComm> m_ddsComm;
		std::thread m_wpnStatusCtrlThread;

		// 적재된 무장 종류만 저장 (원자적 연산)
		std::atomic<uint32_t> m_loadedWeaponKind{ static_cast<uint32_t>(EN_WPN_KIND::WPN_KIND_NA) };

		// 할당 정보
		TEWA_ASSIGN_CMD m_assignmentInfo;
		bool m_isAssigned;
		EN_WPN_KIND m_weaponKind;

		// 무장 상태 통제 관리자
		std::unique_ptr<WpnStatusCtrlManager> m_wpnStatusCtrlManager;
		std::unique_ptr<IEngagementManager> m_engagementManager;

		// 환경 정보
		NAVINF_SHIP_NAVIGATION_INFO m_ownShipInfo;
		std::map<uint32_t, TRKMGR_SYSTEMTARGET_INFO> m_targetInfoMap;

		// 상태
		std::atomic<bool> m_initialized;
		std::atomic<bool> m_shutdown;

		// 동기화
		mutable std::mutex m_mutex;
		WeaponAssignmentInfo ConvertFromDdsMessage(const TEWA_ASSIGN_CMD& ddsMsg);
		void OnWeaponLaunched(std::chrono::steady_clock::time_point launchTime);
	};
} // namespace MINEASMALM
