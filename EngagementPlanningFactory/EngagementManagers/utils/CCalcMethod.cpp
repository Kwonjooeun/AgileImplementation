#include "CCalcMethod.h"

CCalcMethod::CCalcMethod(void)
{
}

CCalcMethod::~CCalcMethod(void)
{
}

int CCalcMethod::findClosestIndex(const std::array<float, C_TRAJECTORY_SIZE>& vec, float target) {
	auto it = std::lower_bound(vec.begin(), vec.end(), target);
	if (it == vec.begin()) return 0; // target이 최소값보다 작을 경우
	if (it == vec.end()) return vec.size() - 1; // target이 최대값보다 클 경우

	int idx = std::distance(vec.begin(), it); // lower_bound 위치의 인덱스
	int prevIdx = idx - 1; // 이전 인덱스

	// 더 가까운 값의 인덱스를 반환
	return (std::abs(vec[idx] - target) < std::abs(vec[prevIdx] - target)) ? idx : prevIdx;
}

double	CCalcMethod::Set_Reversed_Trigonometrical_Argument(double dArg)
{
	double dResult = dArg;
	if (dResult < -1)
	{
		dResult = -1;
	}
	if (dResult > 1)
	{
		dResult = 1;
	}
	return dResult;
}
double	CCalcMethod::Compensate_Qudrant_Angle(double dPx, double dPy, double dAngle)
{
	double dResult = 0;

	if ((dPx >= 0) && (dPy < 0))	// 2사분면
	{
		dResult = 180 - dAngle;
	}
	else if ((dPx < 0) && (dPy < 0))	// 3사분면
	{
		dResult = 180 + dAngle;
	}
	else if ((dPx < 0) && (dPy > 0))	// 4사분면
	{
		dResult = 360 - dAngle;
	}
	else
	{

	}
	return dResult;
}



double CCalcMethod::GetTurnAngle(int i_iTurnDir, double i_fCurCourse, double i_fDestCourse)
{
	double fAngle = 0.0f;

	if (i_iTurnDir == 1) // turn right
	{
		fAngle = toStdAngle(i_fDestCourse - i_fCurCourse);
	}
	if (i_iTurnDir == -1) // turn left
	{
		fAngle = toStdAngle(i_fCurCourse - i_fDestCourse);
	}
	return fAngle;
}

double CCalcMethod::GetMinAngleDiff(double i_fCurCourse, double i_fDestCourse)
{
	double fTurnAngle = 0.f;
	double fRAngleDiff = toStdAngle(i_fDestCourse - i_fCurCourse);
	double fLAngleDiff = toStdAngle(i_fCurCourse - i_fDestCourse);

	if (fLAngleDiff > fRAngleDiff)
	{
		fTurnAngle = fRAngleDiff;
	}
	else
	{
		fTurnAngle = fLAngleDiff;
	}

	return fTurnAngle;
}

//double CCalcMethod::GetTurnAngle(double i_fCurDir, double i_fDstDir)
//{
//    double fResult = 0;
//
//    if ( fmod(i_fDstDir - i_fCurDir + 360.0,360.0) < fmod (i_fCurDir -  i_fDstDir + 360.0,360.0 )  )
//    {
//        fResult = fmod(i_fDstDir - i_fCurDir + 360.0,360.0);
//    }
//
//    if( fmod(i_fDstDir - i_fCurDir + 360.0,360.0) > fmod (i_fCurDir -  i_fDstDir + 360.0,360.0 )  )
//    {
//        fResult = -1 * fmod (i_fCurDir -  i_fDstDir + 360.0,360.0 );
//    }
//
//    return fResult;
//}


bool CCalcMethod::isAngleXInsideSector(double i_fAngleX, double i_fAngleSectorStart, double i_fAngleSectorEnd)
{
	bool bReturnVal = false;
	if (toStdAngle(i_fAngleX - i_fAngleSectorStart) < 180 &&
		toStdAngle(i_fAngleSectorEnd - i_fAngleX) < 180
		)
	{
		bReturnVal = true;
	}
	return bReturnVal;
}


int CCalcMethod::GetTurnDirection(double i_fCurCourse, double i_fDestCourse)
{
	double fRAngleDiff = fmod((i_fDestCourse - i_fCurCourse) + 360.f, 360.f);
	double fLAngleDiff = fmod((i_fCurCourse - i_fDestCourse) + 360.f, 360.f);

	int iTurnR = 0;

	if (fRAngleDiff > FLT_EPSILON)
	{
		if (fLAngleDiff > fRAngleDiff)
		{
			iTurnR = 1;
		}
		else
		{
			iTurnR = -1;
		}
	}

	return iTurnR;
}

double CCalcMethod::toStdAngle(double i_fDeg)
{
	while (i_fDeg > (360.0f - FLT_EPSILON))
	{
		i_fDeg = i_fDeg - 360.0f;
	}

	while (i_fDeg < (0 - FLT_EPSILON))
	{
		i_fDeg = i_fDeg + 360.0f;
	}
	return i_fDeg;
}

double CCalcMethod::toBFAngle(double i_fDeg)
{
	double fAngle = toStdAngle(i_fDeg);

	if (fAngle > (180.f + FLT_EPSILON))
	{
		fAngle -= 360.f;
	}

	return fAngle;
}

double CCalcMethod::GetAvrOfAngles(double i_fAngle1, double i_fAngle2)
{
	double fAngleDiff = GetMinAngleDiff(i_fAngle1, i_fAngle2);
	double fAvrAngle = toStdAngle(i_fAngle1 + (fAngleDiff*0.5f));
	return fAvrAngle;
}

double CCalcMethod::GetDistance(double i_dE, double i_dN, double i_dE2, double i_dN2)
{
	double diff_x = i_dE2 - i_dE;
	double diff_y = i_dN2 - i_dN;
	double range = sqrt( (diff_x*diff_x) + (diff_y*diff_y) );
	return range;
}

void CCalcMethod::GetRangeBearing(double i_dE, double i_dN, double& o_dRange, double &o_Bearing)
{
	o_dRange = sqrt((i_dE*i_dE) + (i_dN*i_dN));

	double dblBearing = 0.0;
	if (fabs(o_dRange) < DBL_EPSILON)
	{
		dblBearing = 0.0;
	}
	else
	{
		dblBearing = asin(i_dE / o_dRange) * RAD2DEG;
	}

	if ((i_dE > 0) && (i_dN < 0))	// 2사분면
	{
		dblBearing = 180.0 - dblBearing;
	}
	else if ((i_dE < 0) && (i_dN < 0))	// 3사분면
	{
		dblBearing = 180.0 - dblBearing;
	}
	else
	{
	}

	o_Bearing = toStdAngle(dblBearing);
}

