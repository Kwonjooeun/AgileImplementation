#include "WeaponStateManagerBase.h"
#include "../../Common/Utils/ConfigManager.h"
#include <iostream>
#include <algorithm>

namespace WeaponControl {

// =============================================================================
// 기본 상태 전이 맵
// =============================================================================
const std::map<EN_WPN_CTRL_STATE, std::set<EN_WPN_CTRL_STATE>> WeaponStateManagerBase::s_defaultTransitionMap = {
    {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF, {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_ON}},
    {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_ON, {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF}},
    {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_RTL, {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_LAUNCH, EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF}},
    {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_LAUNCH, {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_ABORT}},
    {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_ABORT, {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF}},
    {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_POST_LAUNCH, {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF}}
};


} // namespace WeaponControl
