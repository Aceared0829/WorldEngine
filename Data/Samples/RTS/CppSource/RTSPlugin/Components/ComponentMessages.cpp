#include <RTSPlugin/RTSPluginPCH.h>

#include <RTSPlugin/Components/ComponentMessages.h>

// clang-format off

W_IMPLEMENT_MESSAGE_TYPE(RtsMsgNavigateTo);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(RtsMsgNavigateTo, 1, WRTTIDefaultAllocator<RtsMsgNavigateTo>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_IMPLEMENT_MESSAGE_TYPE(RtsMsgStopNavigation);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(RtsMsgStopNavigation, 1, WRTTIDefaultAllocator<RtsMsgStopNavigation>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_IMPLEMENT_MESSAGE_TYPE(RtsMsgArrivedAtLocation);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(RtsMsgArrivedAtLocation, 1, WRTTIDefaultAllocator<RtsMsgArrivedAtLocation>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_IMPLEMENT_MESSAGE_TYPE(RtsMsgAssignPosition);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(RtsMsgAssignPosition, 1, WRTTIDefaultAllocator<RtsMsgAssignPosition>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_IMPLEMENT_MESSAGE_TYPE(RtsMsgSetTarget);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(RtsMsgSetTarget, 1, WRTTIDefaultAllocator<RtsMsgSetTarget>)
W_END_DYNAMIC_REFLECTED_TYPE;


W_IMPLEMENT_MESSAGE_TYPE(RtsMsgApplyDamage);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(RtsMsgApplyDamage, 1, WRTTIDefaultAllocator<RtsMsgApplyDamage>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_IMPLEMENT_MESSAGE_TYPE(RtsMsgUnitHealthStatus);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(RtsMsgUnitHealthStatus, 1, WRTTIDefaultAllocator<RtsMsgUnitHealthStatus>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_IMPLEMENT_MESSAGE_TYPE(RtsMsgGatherUnitStats);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(RtsMsgGatherUnitStats, 1, WRTTIDefaultAllocator<RtsMsgGatherUnitStats>)
W_END_DYNAMIC_REFLECTED_TYPE;

// clang-format on
