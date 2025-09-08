#include "AIEP_DataConverter.h"

namespace AIEP {

#define M_PI	3.14159265358979323846   // pi

	void DataConverter::convertLatLonToLocalEN(const GEO_POINT_2D center,
		const double latitude, const double longitude, double& e, double& n)
	{
		CPosition::getRelativePosition(center.latitude, center.longitude, latitude, longitude,
			&e, &n, GEO_CONVERT_GREAT_CIRCLE, GEO_COORDINATE_ENU);
	}

	void DataConverter::convertLocalENToLatLon(const GEO_POINT_2D center,
		const double e, const double n, double& latitude, double& longitude)
	{
		CPosition::getPositionFromXY(
			center.latitude, center.longitude, e, n, &latitude, &longitude, GEO_CONVERT_GREAT_CIRCLE, GEO_COORDINATE_ENU);
	}


	void DataConverter::convertLatLonAltToLocal(const GEO_POINT_2D center,
		const double latitude, const double longitude, const double altitude, SPOINT_WEAPON_ENU& o_sim_obj)
	{
		CPosition::getRelativePosition(center.latitude, center.longitude, latitude, longitude,
			&o_sim_obj.E, &o_sim_obj.N, GEO_CONVERT_GREAT_CIRCLE, GEO_COORDINATE_ENU);
		o_sim_obj.U = altitude;
		//o_sim_obj.Depth = -altitude;
	}


	void DataConverter::convertTrackInfoToLocal(const GEO_POINT_2D center,
		const TRKMGR_SYSTEMTARGET_INFO& trk_info, CAiepObject& o_sim_obj)
	{

		double N = 0., E = 0.;
		CPosition::getRelativePosition(center.latitude, center.longitude,
			trk_info.stGeodeticPosition().dLatitude(), trk_info.stGeodeticPosition().dLongitude(),
			&o_sim_obj.E, &o_sim_obj.N, GEO_CONVERT_GREAT_CIRCLE, GEO_COORDINATE_ENU);

		o_sim_obj.Depth = trk_info.stGeodeticPosition().fDepth();
		o_sim_obj.Speed = trk_info.stTarget2DPositionVelocity().fSpeed();
		o_sim_obj.Course = trk_info.stTarget2DPositionVelocity().fCourse();
		o_sim_obj.ID = trk_info.unTargetSystemID();
	}

	void DataConverter::convertGeoArrToLocal(const GEO_POINT_2D center, const std::vector<ST_3D_GEODETIC_POSITION> geo_pos_array, std::vector<SPOINT_ENU>& local_pos_vector)
	{
		local_pos_vector.clear();
		for (int i = 0; i < geo_pos_array.size(); i++)
		{
			SPOINT_ENU enu{ 0, };
			enu.U = -1.0 * geo_pos_array[i].fDepth();

			CPosition::getRelativePosition(
				center.latitude, center.longitude,
				geo_pos_array[i].dLatitude(), geo_pos_array[i].dLongitude(),
				&enu.E, &enu.N, GEO_CONVERT_GREAT_CIRCLE, GEO_COORDINATE_ENU);

			local_pos_vector.push_back(enu);
		}
	}

	void DataConverter::convertLocalArrToGeo(const GEO_POINT_2D center, const std::vector<SPOINT_WEAPON_ENU> local_pos_array, std::vector<ST_WEAPON_WAYPOINT>& geo_pos_array)
	{
		geo_pos_array.clear();
		ST_3D_GEODETIC_POSITION geo_pos;
		ST_WEAPON_WAYPOINT result_geo_pos;
		for (int i = 0; i < local_pos_array.size(); i++)
		{
			//geo_pos.fDepth() = local_pos_array[i].U * -1.0;
			CPosition::getPositionFromXY(
				center.latitude, center.longitude,
				local_pos_array[i].E, local_pos_array[i].N,
				&geo_pos.dLatitude(), &geo_pos.dLongitude(), GEO_CONVERT_GREAT_CIRCLE, GEO_COORDINATE_ENU);
			result_geo_pos.dLatitude() = geo_pos.dLatitude();
			result_geo_pos.dLongitude() = geo_pos.dLongitude();
			result_geo_pos.bValid() = local_pos_array[i].Validation;
			result_geo_pos.fDepth() = local_pos_array[i].U * -1.0;
			geo_pos_array.push_back(result_geo_pos);
		}
	}

	void DataConverter::convertLocalArrToGeo(const GEO_POINT_2D center, const std::vector<SPOINT_ENU> local_pos_array, std::vector<ST_3D_GEODETIC_POSITION>& geo_pos_array)
	{
    		geo_pos_array.clear();
		ST_3D_GEODETIC_POSITION geo_pos;

		for (int i = 0; i < local_pos_array.size(); i++)
		{
			geo_pos.fDepth() = local_pos_array[i].U * -1.0;
			CPosition::getPositionFromXY(
				center.latitude, center.longitude,
				local_pos_array[i].E, local_pos_array[i].N,
				&geo_pos.dLatitude(), &geo_pos.dLongitude(), GEO_CONVERT_GREAT_CIRCLE, GEO_COORDINATE_ENU);
			geo_pos_array.push_back(geo_pos);
		}
	}


	void DataConverter::convertGeoArrToLocal(const GEO_POINT_2D center, const std::vector<ST_WEAPON_WAYPOINT> geo_pos_array, std::vector<SPOINT_ENU>& local_pos_vector)
	{
		local_pos_vector.clear();
		for (int i = 0; i < geo_pos_array.size(); i++)
		{
			SPOINT_ENU enu{ 0, };
			enu.U = -1.0 * geo_pos_array[i].fDepth();

			CPosition::getRelativePosition(
				center.latitude, center.longitude,
				geo_pos_array[i].dLatitude(), geo_pos_array[i].dLongitude(),
				&enu.E, &enu.N, GEO_CONVERT_GREAT_CIRCLE, GEO_COORDINATE_ENU);

			local_pos_vector.push_back(enu);
		}
	}

}

