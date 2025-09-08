#pragma once
namespace AIEP {
	struct SPOINT_ENU
	{
		double E;
		double N;
		double U;
	};

	struct SPOINT_WEAPON_ENU
	{
		double E;
		double N;
		double U;
		double Speed;
		bool Validation;
	};

	struct GEO_POINT_2D
	{
		double latitude;
		double longitude;
	};
}
