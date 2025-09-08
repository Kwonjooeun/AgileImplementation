#pragma once
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>
#include <array>

//#include <corecrt_math_defines.h>
typedef float		float32_t;
typedef double		float64_t;

#define DEG2RAD 1.745329251994329547e-2
#define RAD2DEG (180.0/M_PI)
#define M_PI	3.14159265358979323846   // pi
#define MS2KN			1.943844				// m/s -> knot
#define KN2MS			0.514444				// knot -> m/s

#define C_TRAJECTORY_SIZE 128

class CCalcMethod
{
public:
	CCalcMethod(void);
	~CCalcMethod(void);

	//---------------------------------------------------------------------------------------------------------
	//역삼각함수 Argument 설정 ex)if arg >1, then asin(1) or else if arg < -1, then asin(-1)
			// input : Argument
			// output: 1 or -1
	static double	Set_Reversed_Trigonometrical_Argument(double dArg);

	// X, Y 좌표와 각도를 이용하여 사분면에 따른 각도 보정
			// input : 위치 - (Px, Py, Angle)
			// output : 각도 - (Result)
	static double	Compensate_Qudrant_Angle(double dPx, double dPy, double dAngle);

	static void GetRangeBearing(double i_dPx, double i_dPy, double& i_dRange, double &i_Bearing);


	// 1: R Turn, -1: L Turn, 0: Same Angle
	static int GetTurnDirection(double i_fCurCourse, double i_fDestCourse);
	static double GetMinAngleDiff(double i_fCurCourse, double i_fDestCourse);

	static double GetTurnAngle(int i_iTurnDir, double i_fCurCourse, double i_fDestCourse);

	static double toStdAngle(double i_fDeg);
	static double toBFAngle(double i_fDeg);

	static bool isAngleXInsideSector(double i_fAngleX, double i_fAngleStart, double i_fAngleEnd);

	//        static double GetTurnAngle(double i_fCurDir, double i_fDstDir);

	static double GetAvrOfAngles(double i_fAngle1, double i_fAngle2);
	static double GetDistance(double i_dPx, double i_dPy, double i_dPx2, double i_dPy2);
	static int findClosestIndex(const std::array<float, C_TRAJECTORY_SIZE>& vec, float target);
};

