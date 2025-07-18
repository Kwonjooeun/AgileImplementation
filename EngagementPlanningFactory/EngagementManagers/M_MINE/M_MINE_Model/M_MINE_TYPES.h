#pragma once
#include <vector>
#include "../../utils/AIEP_Defines.h"

namespace MINEASMALM {
	//typedef unsigned char boolean;
	typedef unsigned char octet;
	typedef int EWF_TUBE_NUM;
	//typedef unsigned char uint8_t;

	// 교전계획 결과 (ENU)
	struct SAL_MINE_EP_RESULT
	{
		float time_to_destination;	// 부설 지점까지 총 소요 시간 [sec]
		float RemainingTime;		// 부설 지점까지 남은 시간 [sec]

		int BatteryCapacity_percentage;
		float BatteryTime_sec;

		int number_of_trajectory;
		std::vector<SPOINT_ENU> trajectory;
		std::vector<SPOINT_ENU> flightTimeOfTrajectory;

		int number_of_waypoint;		// 경로점 개수
		std::vector <SPOINT_WEAPON_ENU> waypoints;
		std::vector <float> waypointsArrivalTimes;	// 각 경로점까지의 소요시간

		SPOINT_WEAPON_ENU LaunchPoint;	// 발사 지점
		SPOINT_WEAPON_ENU DropPoint;	// 부설 지점

		int idxOfNextWP;	// 다음 경로점 Index
		float timeToNextWP;	// 다음 경로점까지 남은 시간 [sec]

		bool bValidMslDRPos{ false };	// 탄 위치 유효성
		SPOINT_ENU mslDRPos;	// 탄 위치

		void reset()
		{
			memset(this, 0, sizeof(SAL_MINE_EP_RESULT));
		}
	};

	struct CalcVariablesbyDynamics
	{
		double current_E;	// 현재위치 y
		double current_N;	// 현재위치 x
		double current_U;	// 현재위치 z
		double current_WP_E; // 현재 목적지 WP 정보
		double current_WP_N;
		double current_WP_U;
		bool isLaunched;
		int nextWaypointIdx;
		double cmd_Yaw;
		double Yawdiff;
		double current_Yaw;
		double cmd_Pitch;
		double Pitchdiff;
		double current_Pitch;
		bool WP_Flag;
		double ran_rad;
		SPOINT_WEAPON_ENU listofWaypoints[8];
		double DP_X, DP_Y, DP_Z;
		double TotalRunningDistance_ENU;
		double remainingDistance_ENU;
		int timeSinceLaunch;
		double InitSpeed;         //
		double I_X, I_Y, I_Z;
		int lastWP;
		double tempSPEED;
		// KSY Add
		double current_SPD;
		unsigned long ulLaunchTime;
		unsigned long ulLayingTime;
		unsigned long ulEstinatedDrivingTime;
	};

	struct TrajInfo
	{
		int WPSize;
		CalcVariablesbyDynamics tmpVar;   //계산 변수
		double INITIAL_DEPTH;
		double RUNNING_DISTANCE;
		double DESTINATION_LATITUDE;
		double DESTINATION_LONGITUDE;
		double DESTINATION_DEPTH;
		SPOINT_WEAPON_ENU stWPInfo[8];
		double I_VELOCITY_N;
		double I_VELOCITY_E;
		double I_VELOCITY_D;
		double I_ROLL;
		double I_PITCH;
		double I_HEADING;
		double I_PX;
		double I_QY;
		double I_RZ;
		double I_ACC_X;
		double I_ACC_Y;
		double I_ACC_Z;
		double I_LATITUDE;
		double I_LONGITUDE;
		double I_DEPTH;
		double I_LEVER_X;
		double I_LEVER_Y;
		double I_LEVER_Z;

		double I_LAUNCH_POS_LAT;
		double I_LAUNCH_POS_LON;
		double I_LAUNCH_POS_DEPTH;

		float fBatCap;
	};


	struct COMM_WPN_INTERNAL_OSD_INFO {
		unsigned long _id;
		unsigned long _count;
		long _status;
		long _reserved;
		unsigned short unSourcePositionData;
		double dblOwnLongitude;
		double dblOwnLatitude;
		unsigned short unSourceAttitudeData;
		float fHeading;
		float fRoll;
		float fPitch;
		float fHeadingRate;
		float fRollRate;
		float fPitchRate;
		unsigned short unSourceWaterSpeedData;
		float fWaterSpeedX_B;
		float fWaterSpeedY_B;
		float fWaterSpeedZ_B;
		unsigned short unSourceGroundSpeedData;
		float fGroundSpeedX_B;
		float fGroundSpeedY_B;
		float fGroundSpeedZ_B;
		unsigned short unSourceAccelerationData;
		float fXAcceleration_B;
		float fYAcceleration_B;
		float fZAcceleration_B;
		float fNorthAcceleration;
		float fEastAcceleration;
		float fDownAcceleration;
		unsigned short unSourceCourseData;
		float fCourse;
		unsigned short unSourceDivingDepthData;
		float fDivingDepth;
		unsigned short unSourceWaterDepthData;
		float fWaterDepth;
		float fVelocityN_INS;
		float fVelocityE_INS;
		float fVelocityD_INS;
		float fDivingDepth_INS;
		float fLatitude_INS;
		double dblRefLongitude;
		double dblRefLatitude;
		double dblDistance;
		double dblDistanceX;
		double dblDistanceY;
		double dblBearing;
		float fLongitude_INS;
		unsigned long long ullTimeTag1;
		unsigned long ulTimeTag2;
		octet byUpdateKind;
	};

	struct M_MINE_C_CAL_RESULT {
		unsigned long _id;
		unsigned long _count;
		long _status;
		long _reserved;
		EWF_TUBE_NUM enTubeNum;
		double dblMMinePosLat;
		double dblMMinePosLon;
		float fMineCourse;
		float fMineDistance;
		float fMineBearing;
		float fMineSpeed;
		float fMineRunDist;
		unsigned long ulLaunchTime;
		unsigned long ulElapseTime;
		unsigned long ulLayingTime;
		unsigned long ulEstimatedDrivingTime;
		long lBatteryAvailableTime;
		float fBatteryRemainPer;
		bool abWpnInfo[30];
		unsigned long ulRemainingTime;
		double dbLPLat;
		double dbLPLong;
		float fLPDepth;
		float fLPRcmdCourse;
		float fLPDist;
		float fLPRadius;
		unsigned long ulTimeOnWpt;
	};
}// namespace AIEP
