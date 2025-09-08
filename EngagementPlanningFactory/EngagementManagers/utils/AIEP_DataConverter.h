#pragma once

#include <vector>

#include "AIEP_Defines.h"
#include "../../../dds_message/AIEP_AIEP_.hpp"
#include "./inc/CPosition.h"
#include "AiepObject.h"

namespace AIEP {
	static constexpr double WGS84_A = 6378137.0;            // semi-major axis (m)
	static constexpr double WGS84_F = 1.0 / 298.257223563;  // flattening
	static constexpr double WGS84_E2 = WGS84_F * (2.0 - WGS84_F);  // eccentricity^2

	class DataConverter
	{
	public:
		static void convertLatLonToLocalEN(const GEO_POINT_2D center,
			const double latitude, const double longitude, double& e, double& n);

		static void convertLocalENToLatLon(const GEO_POINT_2D center,
			const double e, const double n, double& latitude, double& longitude);

		static void convertLatLonAltToLocal(const GEO_POINT_2D center,
			const double latitude, const double longitude, const double altitude, SPOINT_WEAPON_ENU& o_sim_obj);

		static void convertTrackInfoToLocal(const GEO_POINT_2D center,
			const TRKMGR_SYSTEMTARGET_INFO& trk_info, CAiepObject& o_sim_obj);

		static void convertGeoArrToLocal(const GEO_POINT_2D center, const std::vector<ST_3D_GEODETIC_POSITION> geo_pos_array, std::vector<SPOINT_ENU>& local_pos_vector);
		static void convertGeoArrToLocal(const GEO_POINT_2D center, const std::vector<ST_WEAPON_WAYPOINT> geo_pos_array, std::vector<SPOINT_ENU>& local_pos_vector);

		static void convertLocalArrToGeo(const GEO_POINT_2D center, const std::vector<SPOINT_WEAPON_ENU> local_pos_array, std::vector<ST_WEAPON_WAYPOINT>& geo_pos_array);
		static void convertLocalArrToGeo(const GEO_POINT_2D center, const std::vector<SPOINT_ENU> local_pos_array, std::vector<ST_3D_GEODETIC_POSITION>& geo_pos_array);
	};
}// namespace AIEP
