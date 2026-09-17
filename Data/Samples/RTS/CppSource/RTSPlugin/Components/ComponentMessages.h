#pragma once

#include <RTSPlugin/RTSPluginDLL.h>

struct RtsMsgNavigateTo : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(RtsMsgNavigateTo, WMessage);

  WVec2 m_vTargetPosition;
};

/// Tell the unit to stop and stay where it currently is
struct RtsMsgStopNavigation : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(RtsMsgStopNavigation, WMessage);
};

struct RtsMsgArrivedAtLocation : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(RtsMsgArrivedAtLocation, WMessage);
};

struct RtsMsgAssignPosition : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(RtsMsgAssignPosition, WMessage);

  WVec2 m_vTargetPosition;
};

struct RtsMsgSetTarget : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(RtsMsgSetTarget, WMessage);

  WGameObjectHandle m_hObject;
};

struct RtsMsgApplyDamage : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(RtsMsgApplyDamage, WMessage);

  WInt16 m_iDamage;
};

/// Used to inform sub-systems (child nodes etc.) when health changes (taking damage etc.)
/// m_uiCurHealth == 0 means the unit is destroyed now
struct RtsMsgUnitHealthStatus : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(RtsMsgUnitHealthStatus, WMessage);

  WUInt16 m_uiCurHealth;
  WUInt16 m_uiMaxHealth;
  WInt16 m_iDifference;
};

/// Used to query the health/shields status for display
struct RtsMsgGatherUnitStats : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(RtsMsgGatherUnitStats, WMessage);

  WUInt16 m_uiCurHealth = 0;
  WUInt16 m_uiMaxHealth = 0;
  WUInt16 m_uiCurShields = 0;
  WUInt16 m_uiMaxShields = 0;
};
