#include "M_MINE_DroppingPlanManager.h"
namespace AIEP {
	// Helper: Convert ST_WEAPON_WAYPOINT  to JSON
	json M_MineDroppingPlanManager::pointToJson(const ST_WEAPON_WAYPOINT& pt)
	{
		json j;
		j["dLatitude"] = pt.dLatitude();
		j["dLongitude"] = pt.dLongitude();
		j["fDepth"] = pt.fDepth();
		j["fSpeed"] = pt.fSpeed();
		j["bValid"] = pt.bValid();
		return j;
	}

	// Helper: Convert ST_M_MINE_PLAN_OWNSHIP_WAYPOINT to JSON
	json M_MineDroppingPlanManager::waypointToJson(const ST_M_MINE_PLAN_OWNSHIP_WAYPOINT& wpt)
	{
		json j;
		j["dLatitude"] = wpt.dLatitude();
		j["dLongitude"] = wpt.dLongitude();
		j["fDepth"] = wpt.fDepth();
		j["fSpeed"] = wpt.fSpeed();
		j["fHeading"] = wpt.fHeading();
		j["bLaunchPoint"] = wpt.bLaunchPoint();
		j["usListID"] = wpt.usListID();
		return j;
	}

	json M_MineDroppingPlanManager::createEmptyPlanJson()
	{
		json j;
		// stMsgHeader 처리: 여기서는 빈 객체로 처리 (필요하면 필드 추가)
		j["stMsgHeader"] = json::object();
		j["usPlanListCnt"] = 0;
		j["stMinePlanList"] = json::array();

		json jEmptyMinePoint;
		jEmptyMinePoint["dLatitude"] = 0;
		jEmptyMinePoint["dLongitude"] = 0;
		jEmptyMinePoint["fDepth"] = 0.;
		jEmptyMinePoint["fSpeed"] = 0.;
		jEmptyMinePoint["bValid"] = 0.;

		json jEmptyOwnshipPoint;
		jEmptyOwnshipPoint["dLatitude"] = 0;
		jEmptyOwnshipPoint["dLongitude"] = 0;
		jEmptyOwnshipPoint["fDepth"] = 0.;
		jEmptyOwnshipPoint["fSpeed"] = 0.;
		jEmptyOwnshipPoint["fHeading"] = 0.;
		jEmptyOwnshipPoint["bLaunchPoint"] = 0;
		jEmptyOwnshipPoint["usListID"] = 0;

		// 배열의 전체 슬롯은 15개로 고정
		for (int i = 0; i < 15; i++) {
			json jPlan;

			jPlan["chDescription"] = "";
			jPlan["sListID"] = 0;
			jPlan["usOwnshipWaypointCnt"] = 0;

			// 하위 부설계획 (최대 15개)
			jPlan["stPlan"] = json::array();
			for (int j = 0; j < 15; j++) {
				json jInfo;

				jInfo["sListID"] = 0;
				jInfo["usDroppingPlanNumber"] = 0;
				jInfo["ePlanState"] = 0;
				jInfo["usWeaponID"] = 0;
				jInfo["cAdditionalText"] = "";
				// stDropPos, stLaunchPos
				jInfo["stDropPos"] = jEmptyMinePoint;
				jInfo["stLaunchPos"] = jEmptyMinePoint;
				jInfo["usWaypointCnt"] = 0;
				// stWaypoint: 배열 8개
				jInfo["stWaypoint"] = json::array();
				for (int k = 0; k < 8; k++) {
					jInfo["stWaypoint"].push_back(jEmptyMinePoint);
				}
				jPlan["stPlan"].push_back(jInfo);
			}
			// 자함 변침점 (Ownship Waypoint): 배열 40개
			jPlan["stOwnshipWaypoint"] = json::array();
			for (int j = 0; j < 40; j++) {
				jPlan["stOwnshipWaypoint"].push_back(jEmptyOwnshipPoint);
			}
			j["stMinePlanList"].push_back(jPlan);
		}
		return j;
	}

	/**
	 * @brief  std::string 내용을 fixed?length C?string 배열에 안전하게 복사합니다.
	 * @param  dest    복사 대상 배열 (예: plan.chDescription())
	 * @param  maxLen  dest 배열의 전체 크기 (예: sizeof(plan.chDescription()))
	 * @param  src     복사할 std::string
	 */
	inline void safeSetDescription(char* dest, size_t maxLen, const std::string& src)
	{
    		if (maxLen == 0 || dest == nullptr) return;

		// 복사할 길이는 src.size()와 maxLen-1 중 작은 쪽
		size_t copyLen = std::min(src.size(), (maxLen - 1));

		// 메모리 복사
		std::memcpy(dest, src.data(), copyLen);

		// 널 종단
		dest[copyLen] = '\0';

		// 나머지 바이트를 0으로 패딩 (선택 사항)
		if (copyLen + 1 < maxLen) {
			std::memset(dest + copyLen + 1, 0, maxLen - copyLen - 1);
		}
	}

	// JSON 파일이 없으면 빈 JSON 파일 생성
	void M_MineDroppingPlanManager::ensure_json_file_exists(const std::string& filename)
	{
		std::ifstream infile(filename);
		if (!infile.good()) { // 파일이 존재하지 않으면
			json empty_json = createEmptyPlanJson(); // 빈 JSON 구조
			std::ofstream outfile(filename);
			outfile << empty_json.dump(4); // 들여쓰기 4칸
			outfile.close();
			std::cout << "Created empty mine plan file: " << filename << std::endl;
		}
	}

	// DDS 메시지를 JSON 객체로 변환하는 함수
	json M_MineDroppingPlanManager::ddsMessageToJson(const CMSHCI_AIEP_M_MINE_EDITED_PLAN_LIST& msg) {
		json j;
		// stMsgHeader 처리: 여기서는 빈 객체로 처리 (필요하면 필드 추가)
		j["stMsgHeader"] = json::object();
		j["usPlanListCnt"] = msg.usPlanListCnt();
		j["stMinePlanList"] = json::array();

		// 배열의 전체 슬롯은 15개로 고정
		for (int i = 0; i < 15; i++) {
			json jPlan;
			const ST_M_MINE_PLAN_LIST& plan = msg.stMinePlanList()[i];
			jPlan["chDescription"] = plan.chDescription();
			jPlan["sListID"] = plan.sListID();
			jPlan["usOwnshipWaypointCnt"] = plan.usOwnshipWaypointCnt();

			// 하위 부설계획 (최대 15개)
			jPlan["stPlan"] = json::array();
			for (int j = 0; j < 15; j++) {
				json jInfo;
				const ST_M_MINE_PLAN_INFO& info = plan.stPlan()[j];
				jInfo["sListID"] = info.sListID();
				jInfo["usDroppingPlanNumber"] = info.usDroppingPlanNumber();
				jInfo["ePlanState"] = info.ePlanState();
				jInfo["usWeaponID"] = info.usWeaponID();
				jInfo["cAdditionalText"] = info.cAdditionalText();
				// stDropPos, stLaunchPos
				jInfo["stDropPos"] = pointToJson(info.stDropPos());
				jInfo["stLaunchPos"] = pointToJson(info.stLaunchPos());
				jInfo["usWaypointCnt"] = info.usWaypointCnt();
				// stWaypoint: 배열 8개
				jInfo["stWaypoint"] = json::array();
				for (int k = 0; k < 8; k++) {
					jInfo["stWaypoint"].push_back(pointToJson(info.stWaypoint()[k]));
				}
				jPlan["stPlan"].push_back(jInfo);
			}
			// 자함 변침점 (Ownship Waypoint): 배열 40개
			jPlan["stOwnshipWaypoint"] = json::array();
			for (int j = 0; j < 40; j++) {
				jPlan["stOwnshipWaypoint"].push_back(waypointToJson(plan.stOwnshipWaypoint()[j]));
			}
			j["stMinePlanList"].push_back(jPlan);
		}
		return j;
	}

	// 파일에 DDS 메시지를 JSON으로 저장하는 함수
	bool M_MineDroppingPlanManager::saveDDSMessageToJsonFile(const CMSHCI_AIEP_M_MINE_EDITED_PLAN_LIST& msg, const std::string& filename) {
		json j = ddsMessageToJson(msg);
		std::ofstream ofs(filename);
		if (!ofs.is_open()) {
			std::cerr << "Error opening file for writing: " << filename << std::endl;
			return false;
		}
		ofs << j.dump(4);
		ofs.close();
		std::cout << "DDS message saved successfully to " << filename << std::endl;
		return true;
	}

	// JSON 객체를 DDS 메시지로 복원
	void M_MineDroppingPlanManager::jsonToDdsMessage(const json& j, AIEP_CMSHCI_M_MINE_ALL_PLAN_LIST& msg)
	{
		// --- 1) Top?level count ---
		unsigned short cnt = j.value("usPlanListCnt", 0u);
		msg.usPlanListCnt(cnt);

		// --- 2) Plan List 배열 복원 ---
		auto const& jPlanList = j.value("stMinePlanList", json::array());
		size_t planCount = std::min<size_t>(jPlanList.size(), 15);

		for (size_t i = 0; i < planCount; ++i)
		{
			const auto& jPlan = jPlanList[i];
			auto& plan = msg.stMinePlanList()[i];

			// 2.1) chDescription (50바이트 char[])
			{
				std::string desc;
				// chDescription가 배열인지 확인합니다.
				if (jPlan["chDescription"].is_array()) {
					// 배열의 각 정수를 문자로 변환하고, null(0)이 나오면 종료합니다.
					for (const auto& ch : jPlan["chDescription"]) {
						int c = ch.get<int>();
						if (c == 0) break;
						desc.push_back(static_cast<char>(c));
					}
				}
				else {
					// 만약 문자열이면 바로 읽습니다.
					desc = jPlan.value("chDescription", "");
				}

				char a[50];
				std::strncpy(a, desc.c_str(), sizeof(plan.chDescription()));
				for (int i = 0; i < 50; i++)
				{
					plan.chDescription()[i] = a[i];//static_cast<dds::core::array< char, 50L>> (*a);
				}
				plan.chDescription()[sizeof(plan.chDescription()) - 1] = '\0';
			}

			// 2.2) sListID
			plan.sListID(jPlan.value("sListID", 0));

			// --- 3) 하위 부설계획(ST_M_MINE_PLAN_INFO) 복원 ---
			auto const& jSubplans = jPlan.value("stPlan", json::array());
			size_t subplanCount = std::min<size_t>(jSubplans.size(), 15);
			for (size_t j = 0; j < subplanCount; ++j)
			{
				const auto& jInfo = jSubplans[j];
				auto& info = plan.stPlan()[j];

				info.sListID(jInfo.value("sListID", 0));
				info.usDroppingPlanNumber(jInfo.value("usDroppingPlanNumber", 0u));
				info.ePlanState(static_cast<uint32_t>(jInfo.value("ePlanState", 0)));
				info.usWeaponID(jInfo.value("usWeaponID", 0u));

				// 3.1) cAdditionalText (50바이트 char[])
				{
					std::string addText;
					// cAdditionalText가 배열인지 확인합니다.
					if (jInfo["cAdditionalText"].is_array()) {
						// 배열의 각 정수를 문자로 변환하고, null(0)이 나오면 종료합니다.
						for (const auto& ch : jInfo["cAdditionalText"]) {
							int c = ch.get<int>();
							if (c == 0) break;
							addText.push_back(static_cast<char>(c));
						}
					}
					else {
						// 만약 문자열이면 바로 읽습니다.
						addText = jInfo.value("cAdditionalText", "");
					}

					char a[50];
					std::strncpy(a, addText.c_str(), sizeof(info.cAdditionalText()));
					for (int i = 0; i < 50; i++)
					{
						info.cAdditionalText()[i] = a[i];//static_cast<dds::core::array< char, 50L>> (*a);
					}
					info.cAdditionalText()[sizeof(info.cAdditionalText()) - 1] = '\0';
				}
				if (jInfo.contains("stDropPos")) {
					auto const& jd = jInfo["stDropPos"];
					info.stDropPos().dLatitude() = jd.value("dLatitude", 0.0);
					info.stDropPos().dLongitude() = jd.value("dLongitude", 0.0);
					info.stDropPos().fDepth() = jd.value("fDepth", 0.0f);
					info.stDropPos().fSpeed() = jd.value("fSpeed", 0.0f);
					info.stDropPos().bValid() = jd.value("bValid", 0u);
				}

				// 3.3) stLaunchPos (ST_WEAPON_WAYPOINT )
				if (jInfo.contains("stLaunchPos")) {
					auto const& jl = jInfo["stLaunchPos"];
					info.stLaunchPos().dLatitude() = jl.value("dLatitude", 0.0);
					info.stLaunchPos().dLongitude() = jl.value("dLongitude", 0.0);
					info.stLaunchPos().fDepth() = jl.value("fDepth", 0.0f);
					info.stLaunchPos().fSpeed() = jl.value("fSpeed", 0.0f);
					info.stLaunchPos().bValid() = jl.value("bValid", 0u);
				}

				// 3.4) usWaypointCnt
				info.usWaypointCnt(jInfo.value("usWaypointCnt", 0u));

				// 3.5) stWaypoint[8]
				{
					auto const& jw = jInfo.value("stWaypoint", json::array());
					size_t wpCount = std::min<size_t>(jw.size(), 8);
					for (size_t k = 0; k < wpCount; ++k) {
						auto const& wj = jw[k];
						auto& wpt = info.stWaypoint()[k];
						wpt.dLatitude() = wj.value("dLatitude", 0.0);
						wpt.dLongitude() = wj.value("dLongitude", 0.0);
						wpt.fDepth() = wj.value("fDepth", 0.0f);
						wpt.fSpeed() = wj.value("fSpeed", 0.0f);
						wpt.bValid() = wj.value("bValid", 0u);
					}
					// 남은 슬롯은 0으로 초기화
					for (size_t k = wpCount; k < 8; ++k) {
						std::memset(&info.stWaypoint()[k], 0, sizeof(ST_WEAPON_WAYPOINT));
					}
				}
			}

			// 3.6) 남은 ST_M_MINE_PLAN_INFO 슬롯 초기화
			for (size_t j = subplanCount; j < 15; ++j) {
				std::memset(&plan.stPlan()[j], 0, sizeof(ST_M_MINE_PLAN_INFO));
			}

			// --- 4) 자함 변침점 (OWNSHIP_WAYPOINT) 복원 ---
			plan.usOwnshipWaypointCnt(jPlan.value("usOwnshipWaypointCnt", 0u));

			auto const& jOwn = jPlan.value("stOwnshipWaypoint", json::array());
			size_t ownCount = std::min<size_t>(jOwn.size(), 40);
			for (size_t k = 0; k < ownCount; ++k) {
				auto const& oj = jOwn[k];
				auto& ow = plan.stOwnshipWaypoint()[k];
				ow.dLatitude() = oj.value("dLatitude", 0.0);
				ow.dLongitude() = oj.value("dLongitude", 0.0);
				ow.fDepth() = oj.value("fDepth", 0.0f);
				ow.fSpeed() = oj.value("fSpeed", 0.0f);
				ow.fHeading() = oj.value("fHeading", 0.0f);
				ow.bLaunchPoint() = oj.value("bLaunchPoint", 0u);
				ow.usListID() = oj.value("usListID", 0u);
			}
			for (size_t k = ownCount; k < 40; ++k) {
				std::memset(&plan.stOwnshipWaypoint()[k], 0, sizeof(ST_M_MINE_PLAN_OWNSHIP_WAYPOINT));
			}
		}

		// --- 5) 남은 PlanList 슬롯 초기화 ---
		for (size_t i = planCount; i < 15; ++i) {
			std::memset(&msg.stMinePlanList()[i], 0, sizeof(ST_M_MINE_PLAN_LIST));
		}
	}

	// JSON 객체를 DDS 메시지로 복원
	void M_MineDroppingPlanManager::jsonToDdsMessage(const json& j, CMSHCI_AIEP_M_MINE_EDITED_PLAN_LIST& msg)
	{
		// --- 1) Top?level count ---
		unsigned short cnt = j.value("usPlanListCnt", 0u);
		msg.usPlanListCnt(cnt);

		// --- 2) Plan List 배열 복원 ---
		auto const& jPlanList = j.value("stMinePlanList", json::array());
		size_t planCount = std::min<size_t>(jPlanList.size(), 15);

		for (size_t i = 0; i < planCount; ++i)
		{
			const auto& jPlan = jPlanList[i];
			auto& plan = msg.stMinePlanList()[i];

			// 2.1) chDescription (50바이트 char[])
			{
				std::string desc;
				// chDescription가 배열인지 확인합니다.
				if (jPlan["chDescription"].is_array()) {
					// 배열의 각 정수를 문자로 변환하고, null(0)이 나오면 종료합니다.
					for (const auto& ch : jPlan["chDescription"]) {
						int c = ch.get<int>();
						if (c == 0) break;
						desc.push_back(static_cast<char>(c));
					}
				}
				else {
					// 만약 문자열이면 바로 읽습니다.
					desc = jPlan.value("chDescription", "");
				}

				char a[50];
				std::strncpy(a, desc.c_str(), sizeof(plan.chDescription()));
				for (int i = 0; i < 50; i++)
				{
					plan.chDescription()[i] = a[i];//static_cast<dds::core::array< char, 50L>> (*a);
				}
				plan.chDescription()[sizeof(plan.chDescription()) - 1] = '\0';
			}

			// 2.2) sListID
			plan.sListID(jPlan.value("sListID", 0));

			// --- 3) 하위 부설계획(ST_M_MINE_PLAN_INFO) 복원 ---
			auto const& jSubplans = jPlan.value("stPlan", json::array());
			size_t subplanCount = std::min<size_t>(jSubplans.size(), 15);
			for (size_t j = 0; j < subplanCount; ++j)
			{
				const auto& jInfo = jSubplans[j];
				auto& info = plan.stPlan()[j];

				info.sListID(jInfo.value("sListID", 0));
				info.usDroppingPlanNumber(jInfo.value("usDroppingPlanNumber", 0u));
				info.ePlanState(static_cast<uint32_t>(jInfo.value("ePlanState", 0)));
				info.usWeaponID(jInfo.value("usWeaponID", 0u));

				// 3.1) cAdditionalText (50바이트 char[])
				{
					std::string addText;
					// cAdditionalText가 배열인지 확인합니다.
					if (jInfo["cAdditionalText"].is_array()) {
						// 배열의 각 정수를 문자로 변환하고, null(0)이 나오면 종료합니다.
						for (const auto& ch : jInfo["cAdditionalText"]) {
							int c = ch.get<int>();
							if (c == 0) break;
							addText.push_back(static_cast<char>(c));
						}
					}
					else {
						// 만약 문자열이면 바로 읽습니다.
						addText = jInfo.value("cAdditionalText", "");
					}

					char a[50];
					std::strncpy(a, addText.c_str(), sizeof(info.cAdditionalText()));
					for (int i = 0; i < 50; i++)
					{
						info.cAdditionalText()[i] = a[i];//static_cast<dds::core::array< char, 50L>> (*a);
					}
					info.cAdditionalText()[sizeof(info.cAdditionalText()) - 1] = '\0';
				}

				// 3.2) stDropPos (ST_WEAPON_WAYPOINT )
				if (jInfo.contains("stDropPos")) {
					auto const& jd = jInfo["stDropPos"];
					info.stDropPos().dLatitude() = jd.value("dLatitude", 0.0);
					info.stDropPos().dLongitude() = jd.value("dLongitude", 0.0);
					info.stDropPos().fDepth() = jd.value("fDepth", 0.0f);
					info.stDropPos().fSpeed() = jd.value("fSpeed", 0.0f);
					info.stDropPos().bValid() = jd.value("bValid", 0u);
				}

				// 3.3) stLaunchPos (ST_WEAPON_WAYPOINT )
				if (jInfo.contains("stLaunchPos")) {
					auto const& jl = jInfo["stLaunchPos"];
					info.stLaunchPos().dLatitude() = jl.value("dLatitude", 0.0);
					info.stLaunchPos().dLongitude() = jl.value("dLongitude", 0.0);
					info.stLaunchPos().fDepth() = jl.value("fDepth", 0.0f);
					info.stLaunchPos().fSpeed() = jl.value("fSpeed", 0.0f);
					info.stLaunchPos().bValid() = jl.value("bValid", 0u);
				}

				// 3.4) usWaypointCnt
				info.usWaypointCnt(jInfo.value("usWaypointCnt", 0u));

				// 3.5) stWaypoint[8]
				{
					auto const& jw = jInfo.value("stWaypoint", json::array());
					size_t wpCount = std::min<size_t>(jw.size(), 8);
					for (size_t k = 0; k < wpCount; ++k) {
						auto const& wj = jw[k];
						auto& wpt = info.stWaypoint()[k];
						wpt.dLatitude() = wj.value("dLatitude", 0.0);
						wpt.dLongitude() = wj.value("dLongitude", 0.0);
						wpt.fDepth() = wj.value("fDepth", 0.0f);
						wpt.fSpeed() = wj.value("fSpeed", 0.0f);
						wpt.bValid() = wj.value("bValid", 0u);
					}
					// 남은 슬롯은 0으로 초기화
					for (size_t k = wpCount; k < 8; ++k) {
						std::memset(&info.stWaypoint()[k], 0, sizeof(ST_WEAPON_WAYPOINT));
					}
				}
			}

			// 3.6) 남은 ST_M_MINE_PLAN_INFO 슬롯 초기화
			for (size_t j = subplanCount; j < 15; ++j) {
				std::memset(&plan.stPlan()[j], 0, sizeof(ST_M_MINE_PLAN_INFO));
			}

			// --- 4) 자함 변침점 (OWNSHIP_WAYPOINT) 복원 ---
			plan.usOwnshipWaypointCnt(jPlan.value("usOwnshipWaypointCnt", 0u));

			auto const& jOwn = jPlan.value("stOwnshipWaypoint", json::array());
			size_t ownCount = std::min<size_t>(jOwn.size(), 40);
			for (size_t k = 0; k < ownCount; ++k) {
				auto const& oj = jOwn[k];
				auto& ow = plan.stOwnshipWaypoint()[k];
				ow.dLatitude() = oj.value("dLatitude", 0.0);
				ow.dLongitude() = oj.value("dLongitude", 0.0);
				ow.fDepth() = oj.value("fDepth", 0.0f);
				ow.fSpeed() = oj.value("fSpeed", 0.0f);
				ow.fHeading() = oj.value("fHeading", 0.0f);
				ow.bLaunchPoint() = oj.value("bLaunchPoint", 0u);
				ow.usListID() = oj.value("usListID", 0u);
			}
			for (size_t k = ownCount; k < 40; ++k) {
				std::memset(&plan.stOwnshipWaypoint()[k], 0, sizeof(ST_M_MINE_PLAN_OWNSHIP_WAYPOINT));
			}
		}

		// --- 5) 남은 PlanList 슬롯 초기화 ---
		for (size_t i = planCount; i < 15; ++i) {
			std::memset(&msg.stMinePlanList()[i], 0, sizeof(ST_M_MINE_PLAN_LIST));
		}
	}
	// 파일에서 DDS 메시지를 JSON 형식으로 읽어와 복원하는 함수
	bool M_MineDroppingPlanManager::loadPlanFromFile(AIEP_CMSHCI_M_MINE_ALL_PLAN_LIST& msg, const std::string& filename)
	{
		ensure_json_file_exists(filename);

		std::ifstream ifs(filename);
		if (!ifs.is_open()) {
			std::cerr << "Error opening file for reading: " << filename << std::endl;
			return false;
		}

		json j;
		try {
			ifs >> j;
		}
		catch (const std::exception& e) {
			std::cerr << "Error parsing JSON file: " << e.what() << std::endl;
			return false;
		}
		ifs.close();
		jsonToDdsMessage(j, msg);
		//std::cout << "Plan loaded successfully from " << filename << std::endl;
		return true;
	}

	// 파일에서 DDS 메시지를 JSON 형식으로 읽어와 복원하는 함수, 경로점만 수정하기 위한 오버로딩
	bool M_MineDroppingPlanManager::loadPlanFromFile(CMSHCI_AIEP_M_MINE_EDITED_PLAN_LIST& msg, const std::string& filename)
	{
		ensure_json_file_exists(filename);

		std::ifstream ifs(filename);
		if (!ifs.is_open()) {
			std::cerr << "Error opening file for reading: " << filename << std::endl;
			return false;
		}

		json j;
		try {
			ifs >> j;
		}
		catch (const std::exception& e) {
			std::cerr << "Error parsing JSON file: " << e.what() << std::endl;
			return false;
		}
		ifs.close();

		jsonToDdsMessage(j, msg);
		//std::cout << "Plan loaded successfully from " << filename << std::endl;
		return true;
	}

	// 파일에 DDS 메시지를 JSON 형식으로 저장하는 함수
	bool M_MineDroppingPlanManager::savePlanToFile(const CMSHCI_AIEP_M_MINE_EDITED_PLAN_LIST& msg, const std::string& filename) {
		json j = ddsMessageToJson(msg);
		std::ofstream ofs(filename);
		if (!ofs.is_open()) {
			std::cerr << "Error opening file for writing: " << filename << std::endl;
			return false;
		}
		ofs << j.dump(4); // indent=4
		ofs.close();
		std::cout << "Plan saved successfully to " << filename << std::endl;
		return true;
	}
	bool M_MineDroppingPlanManager::readDroppingPlanfromFile(const std::string& filename, int DroppingPlanListIdx, int DroppingPlanIdx, ST_M_MINE_PLAN_INFO& DropPlan)
	{
		ensure_json_file_exists(filename);

		std::ifstream ifs(filename);
		if (!ifs.is_open()) {
			std::cerr << "Error opening file for reading: " << filename << std::endl;
			return false;
		}

		json j;
		try {
			ifs >> j;
		}
		catch (const std::exception& e) {
			std::cerr << "Error parsing JSON file: " << e.what() << std::endl;
			return false;
		}
		ifs.close();

		CMSHCI_AIEP_M_MINE_EDITED_PLAN_LIST msg;
		jsonToDdsMessage(j, msg);
		DropPlan = msg.stMinePlanList()[DroppingPlanListIdx].stPlan()[DroppingPlanIdx];
		//std::cout << "Plan loaded successfully from " << filename << std::endl;
		return true;
	}
	bool M_MineDroppingPlanManager::readDropPosFromJson(
		const std::string& filePath,
		int planListIndex,
		int planIndex,
		double& outLatitude,
		double& outLongitude,
		float& outDepth,
		float& outSpeed,
		bool& outValid
	) {
		try {
			// JSON 파일 열기
			std::ifstream file(filePath);
			if (!file.is_open()) {
				std::cerr << "Error: Could not open file " << filePath << std::endl;
				return false;
			}

			// JSON 파일 파싱
			json j;
			file >> j;
			file.close();

			// 전체 계획 개수 확인
			int planListCnt = j["usPlanListCnt"];
			if (planListIndex >= planListCnt || planListIndex < 0) {
				std::cerr << "Error: planListIndex " << planListIndex << " is out of range (0-" << (planListCnt - 1) << ")" << std::endl;
				return false;
			}

			// stMinePlanList 배열 검증
			if (!j.contains("stMinePlanList") || !j["stMinePlanList"].is_array() || j["stMinePlanList"].size() <= planListIndex) {
				std::cerr << "Error: Invalid stMinePlanList or index out of range" << std::endl;
				return false;
			}

			// stPlan 배열 검증
			if (!j["stMinePlanList"][planListIndex].contains("stPlan") ||
				!j["stMinePlanList"][planListIndex]["stPlan"].is_array() ||
				j["stMinePlanList"][planListIndex]["stPlan"].size() <= planIndex) {
				std::cerr << "Error: Invalid stPlan or index out of range" << std::endl;
				return false;
			}

			// stDropPos 정보 추출
			const auto& dropPos = j["stMinePlanList"][planListIndex]["stPlan"][planIndex]["stDropPos"];

			if (!dropPos.is_object()) {
				std::cerr << "Error: stDropPos is not a valid object" << std::endl;
				return false;
			}

			// 필드 값 추출
			outLatitude = dropPos["dLatitude"];
			outLongitude = dropPos["dLongitude"];
			outDepth = dropPos["fDepth"];
			outSpeed = dropPos["fSpeed"];
			outValid = dropPos["bValid"] != 0;

			return true;
		}
		catch (const json::exception& e) {
			std::cerr << "JSON error: " << e.what() << std::endl;
			return false;
		}
		catch (const std::exception& e) {
			std::cerr << "Error: " << e.what() << std::endl;
			return false;
		}
	}

	bool M_MineDroppingPlanManager::readLaunchPosFromJson(
		const std::string& filePath,
		int planListIndex,
		int planIndex,
		double& outLatitude,
		double& outLongitude,
		float& outDepth,
		float& outSpeed,
		bool& outValid
	) {
		try {
			// JSON 파일 열기
			std::ifstream file(filePath);
			if (!file.is_open()) {
				std::cerr << "Error: Could not open file " << filePath << std::endl;
				return false;
			}

			// JSON 파일 파싱
			json j;
			file >> j;
			file.close();

			// 전체 계획 개수 확인
			int planListCnt = j["usPlanListCnt"];
			if (planListIndex >= planListCnt || planListIndex < 0) {
				std::cerr << "Error: planListIndex " << planListIndex << " is out of range (0-" << (planListCnt - 1) << ")" << std::endl;
				return false;
			}

			// stMinePlanList 배열 검증
			if (!j.contains("stMinePlanList") || !j["stMinePlanList"].is_array() || j["stMinePlanList"].size() <= planListIndex) {
				std::cerr << "Error: Invalid stMinePlanList or index out of range" << std::endl;
				return false;
			}

			// stPlan 배열 검증
			if (!j["stMinePlanList"][planListIndex].contains("stPlan") ||
				!j["stMinePlanList"][planListIndex]["stPlan"].is_array() ||
				j["stMinePlanList"][planListIndex]["stPlan"].size() <= planIndex) {
				std::cerr << "Error: Invalid stPlan or index out of range" << std::endl;
				return false;
			}

			// stDropPos 정보 추출
			const auto& dropPos = j["stMinePlanList"][planListIndex]["stPlan"][planIndex]["stLaunchPos"];

			if (!dropPos.is_object()) {
				std::cerr << "Error: stDropPos is not a valid object" << std::endl;
				return false;
			}

			// 필드 값 추출
			outLatitude = dropPos["dLatitude"];
			outLongitude = dropPos["dLongitude"];
			outDepth = dropPos["fDepth"];
			outSpeed = dropPos["fSpeed"];
			outValid = dropPos["bValid"] != 0;

			return true;
		}
		catch (const json::exception& e) {
			std::cerr << "JSON error: " << e.what() << std::endl;
			return false;
		}
		catch (const std::exception& e) {
			std::cerr << "Error: " << e.what() << std::endl;
			return false;
		}
	}
}
