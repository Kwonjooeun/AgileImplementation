#pragma once

#include "IWeapon.h"
#include <atomic>
#include <mutex>
#include <chrono>
#include <functional>

namespace WeaponControl {

/**
 * @brief 무장 기본 클래스
 * 
 * IWeapon 인터페이스의 공통 기능을 구현하는 기반 클래스입니다.
 * 상태 관리 기능은 제거되고 순수한 무장 속성과 기본 동작만 제공합니다.
 */
class WeaponBase : public IWeapon {
};

} // namespace WeaponControl
