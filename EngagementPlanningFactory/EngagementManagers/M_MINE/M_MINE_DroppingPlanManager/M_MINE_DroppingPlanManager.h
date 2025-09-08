#pragma once

#include <fstream>
#include <cstring>
#include <map>
#include <array>
#include <iostream>
#include <thread>
#include <cassert>
#include <algorithm>
#include <cmath>
#include <cstdio>

#define NOMINMAX
#include "../../../../dds_message/AIEP_AIEP_.hpp"
#include "../../../../dds_library/dds.h"
#include "json.hpp"
//#include "AIEP_Defines.h"
#undef NOMINMAX  // now std::min/std::max work again

namespace AIEP {
using json = nlohmann::json;

	class M_MineDroppingPlanManager
	{
	public:
		json pointToJson(const ST_WEAPON_WAYPOINT& pt);
		json waypointToJson(const ST_M_MINE_PLAN_OWNSHIP_WAYPOINT& wpt);
		json createEmptyPlanJson();
		void ensure_json_file_exists(const std::string& filename);
		json ddsMessageToJson(const CMSHCI_AIEP_M_MINE_EDITED_PLAN_LIST& msg);
		bool saveDDSMessageToJsonFile(const CMSHCI_AIEP_M_MINE_EDITED_PLAN_LIST& msg, const std::string& filename);
		void jsonToDdsMessage(const json& j, AIEP_CMSHCI_M_MINE_ALL_PLAN_LIST& msg);
		void jsonToDdsMessage(const json& j, CMSHCI_AIEP_M_MINE_EDITED_PLAN_LIST& msg);
		bool loadPlanFromFile(AIEP_CMSHCI_M_MINE_ALL_PLAN_LIST& msg, const std::string& filename);
		bool loadPlanFromFile(CMSHCI_AIEP_M_MINE_EDITED_PLAN_LIST& msg, const std::string& filename);
		bool readDroppingPlanfromFile(const std::string& filename, int DroppingPlanListIdx, int DroppingPlanIdx, ST_M_MINE_PLAN_INFO& DropPlan);
		bool savePlanToFile(const CMSHCI_AIEP_M_MINE_EDITED_PLAN_LIST& msg, const std::string& filename);
		bool readDropPosFromJson(
			const std::string& filePath,
			int planListIndex,
			int planIndex,
			double& outLatitude,
			double& outLongitude,
			float& outDepth,
			float& outSpeed,
			bool& outValid
		);
		bool readLaunchPosFromJson(
			const std::string& filePath,
			int planListIndex,
			int planIndex,
			double& outLatitude,
			double& outLongitude,
			float& outDepth,
			float& outSpeed,
			bool& outValid
		);
		bool updatePlanState(const std::string& filename,
			int planListIndex,
			int planIndex,
			EN_M_MINE_PLAN_STATE newState);
		bool getPlanState(const std::string& filename,
			int planListIndex,
			int planIndex,
			EN_M_MINE_PLAN_STATE& outState);

	};
}
