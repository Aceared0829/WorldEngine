#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_IMPLEMENT_MESSAGE_TYPE(WMsgMoveCharacterController);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgMoveCharacterController, 1, WRTTIDefaultAllocator<WMsgMoveCharacterController>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("MoveForwards", m_fMoveForwards),
    W_MEMBER_PROPERTY("MoveBackwards", m_fMoveBackwards),
    W_MEMBER_PROPERTY("StrafeLeft", m_fStrafeLeft),
    W_MEMBER_PROPERTY("StrafeRight", m_fStrafeRight),
    W_MEMBER_PROPERTY("RotateLeft", m_fRotateLeft),
    W_MEMBER_PROPERTY("RotateRight", m_fRotateRight),
    W_MEMBER_PROPERTY("Run", m_bRun),
    W_MEMBER_PROPERTY("Jump", m_bJump),
    W_MEMBER_PROPERTY("Crouch", m_bCrouch),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_STATICLINK_FILE(GameEngine, GameEngine_Physics_Implementation_CharacterControllerComponent);
