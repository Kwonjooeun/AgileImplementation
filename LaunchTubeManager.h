#pragma once
#include "Common/Utils/DebugPrint.h"
#include "EngagementPlanningFactory/EngagementPlanningFactory.h"
#include "WpnStatusCtrl/WpnStatusCtrlManager.h"
#include "dds_message/AIEP_AIEP_.hpp"
#include <memory>
#include <mutex>
#include <atomic>

namespace AIEP {
	class LaunchTubeManager
	{
	public:
		explicit LaunchTubeManager(int tubeNumber, std::shared_ptr<AIEP::DdsComm> ddsComm);
		~LaunchTubeManager();

		// 초기화 및 종료
		bool Initialize();
		void Shutdown();

		// 적재정보 업데이트
		void UpdateLoadedWeaponKind(EN_WPN_KIND weaponKind);

		// 할당 가능 여부 확인
		bool IsWeaponTypeCompatible(const TEWA_ASSIGN_CMD& assignCmd) const;

		// 무장 할당 관리
		bool AssignWeapon(const TEWA_ASSIGN_CMD& assignCmd);
		bool UnassignWeapon();

		// 수신 명령의 무장 정보 확인
		bool IsCommandForThisWeapon(uint32_t weaponKindInCmd);

		// 금지구역 정보 수신
		bool ProcessPAInfo(const CMSHCI_AIEP_PA_INFO& paInfo);

		// 무장 상태 통제
		bool ProcessWeaponControlCommand(const CMSHCI_AIEP_WPN_CTRL_CMD& command);

		// 경로점 수정
		bool ProcessWaypointCommand(const CMSHCI_AIEP_WPN_GEO_WAYPOINTS& command);

		// 자함 정보
		bool ProcessOwnshipInfo(const NAVINF_SHIP_NAVIGATION_INFO& ownshipInfo);

		// 시스템 표적 정보
		bool ProcessSystemTargetInfo(const TRKMGR_SYSTEMTARGET_INFO& systemtargetInfo);

		// AI 경로점 요청
		bool ProcessAIWaypointsInferenceRequest(const CMSHCI_AIEP_AI_WAYPOINTS_INFERENCE_REQ& command);

		// AI 경로점 결과 후처리 후 전송
		bool ProcessAIWaypointsInferenceResult(const AIEP_INTERNAL_INFER_RESULT_WP& command);
	private:
		// 멤버 변수
		int m_tubeNumber;
		std::shared_ptr<AIEP::DdsComm> m_ddsComm;
		std::thread m_wpnStatusCtrlThread;

		// 적재된 무장 종류만 저장 (원자적 연산)
		std::atomic<uint32_t> m_loadedWeaponKind{ static_cast<uint32_t>(EN_WPN_KIND::WPN_KIND_NA) };

		// 할당 정보
		TEWA_ASSIGN_CMD m_assignmentInfo;
		bool m_isAssigned;
		uint32_t m_weaponKind;

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
		void OnWeaponLaunched(std::chrono::steady_clock::time_point launchTime);
	};
} // namespace AIEP
