#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/Messages/DamageMessage.h>

// clang-format off
W_IMPLEMENT_MESSAGE_TYPE(WMsgDamage);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgDamage, 1, WRTTIDefaultAllocator<WMsgDamage>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Damage", m_fDamage),
    W_MEMBER_PROPERTY("HitObjectName", m_sHitObjectName),
    W_MEMBER_PROPERTY("GlobalPosition", m_vGlobalPosition),
    W_MEMBER_PROPERTY("ImpactDirection", m_vImpactDirection),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

W_STATICLINK_FILE(GameEngine, GameEngine_Messages_Implementation_DamageMessage);
