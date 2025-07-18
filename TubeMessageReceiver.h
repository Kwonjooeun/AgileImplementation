#pragma once

#include "LaunchTubeManager.h"
#include "Common/Communication/DdsComm.h"
#include "dds_message/AIEP_AIEP_.hpp"
#include <memory>
#include <atomic>
#include <chrono>


namespace MINEASMALM {
	class LaunchTubeManager;

	class TubeMessageReceiver {
	public:
		TubeMessageReceiver(int tubeNumber, LaunchTubeManager* launchtubemanager, std::shared_ptr<DdsComm> ddsComm);
		~TubeMessageReceiver();

		// 초기화 및 종료
		bool Initialize();
		void Shutdown();

	private:
		// 메시지 유효성 검사
		bool IsMessageForThisTube(int messageTargetTube) const;

		// DDS 메시지 수신 콜백들
		void OnLoadInfoReceived(const TEWA_WA_TUBE_LOAD_INFO& message);
		void OnAssignCommandReceived(const TEWA_ASSIGN_CMD& message);
		void OnWeaponControlCommandReceived(const CMSHCI_AIEP_WPN_CTRL_CMD& message);
		//void OnWaypointsReceived(const CMSHCI_AIEP_WPN_GEO_WAYPOINTS& message);
		//void OnOwnShipInfoReceived(const NAVINF_SHIP_NAVIGATION_INFO& message);
		//void OnTargetInfoReceived(const TRKMGR_SYSTEMTARGET_INFO& message);
		//void OnEditedPlanListReceived(const CMSHCI_AIEP_M_MINE_EDITED_PLAN_LIST& message);

		// 멤버 변수
		int m_tubeNumber;
		LaunchTubeManager* m_launchtubemanager;
		std::shared_ptr<DdsComm> m_ddsComm;

		std::atomic<bool> m_initialized;
		std::atomic<bool> m_shutdown;
	};
} // namespace MINEASMALM
