#pragma once

#include "LaunchTubeManager.h"
#include "Common/Communication/DdsComm.h"
#include "dds_message/AIEP_AIEP_.hpp"
#include <memory>
#include <atomic>
#include <chrono>

namespace AIEP {
	class LaunchTubeManager;

	class TubeMessageReceiver {
	public:
		TubeMessageReceiver(int tubeNumber, LaunchTubeManager* launchtubemanager, std::shared_ptr<AIEP::DdsComm> ddsComm);
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
		void OnPAInfoReceived(const CMSHCI_AIEP_PA_INFO& message);
		void OnWeaponControlCommandReceived(const CMSHCI_AIEP_WPN_CTRL_CMD& message);
		void OnWaypointsReceived(const CMSHCI_AIEP_WPN_GEO_WAYPOINTS& message);
		void OnOwnShipInfoReceived(const NAVINF_SHIP_NAVIGATION_INFO& message);
		void OnTargetInfoReceived(const AIEP_WPN_CTRL_STATUS_INFO& message);
		void OnAIWaypointsInferenceRequestReceived(const CMSHCI_AIEP_AI_WAYPOINTS_INFERENCE_REQ& message);
		void OnAIWaypointsInferenceResultReceived(const AIEP_INTERNAL_INFER_RESULT_WP& message);

		// 멤버 변수
		int m_tubeNumber;
		LaunchTubeManager* m_launchtubemanager;
		std::shared_ptr<AIEP::DdsComm> m_ddsComm;

		std::atomic<bool> m_initialized;
		std::atomic<bool> m_shutdown;
	};
} // namespace AIEP
