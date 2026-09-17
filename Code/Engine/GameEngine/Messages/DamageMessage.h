#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Communication/Message.h>

struct W_GAMEENGINE_DLL WMsgDamage : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgDamage, WMessage);

  double m_fDamage = 0;
  WString m_sHitObjectName; ///< The actual game object that was hit (may be a child of the object to which the message is sent)

  WVec3 m_vGlobalPosition;  ///< The global position at which the damage was applied. Set to zero, if unused.
  WVec3 m_vImpactDirection; ///< The direction into which the damage was applied (e.g. direction of a projectile). May be zero.
};
