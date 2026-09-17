#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/TagSet.h>
#include <Foundation/Types/Uuid.h>

/// Describes the initial state of a game object.
struct W_CORE_DLL WGameObjectDesc
{
  W_DECLARE_POD_TYPE();

  bool m_bActiveFlag = true;                       ///< Whether the object should have the 'active flag' set. See WGameObject::SetActiveFlag().
  bool m_bDynamic = false;                         ///< Whether the object should start out as 'dynamic'. See WGameObject::MakeDynamic().
  WUInt16 m_uiTeamID = 0;                         ///< See WGameObject::GetTeamID().

  WHashedString m_sName;                          ///< See WGameObject::SetName().
  WGameObjectHandle m_hParent;                    ///< An optional parent object to attach this object to as a child.

  WVec3 m_LocalPosition = WVec3::MakeZero();     ///< The local position relative to the parent (or the world)
  WQuat m_LocalRotation = WQuat::MakeIdentity(); ///< The local rotation relative to the parent (or the world)
  WVec3 m_LocalScaling = WVec3(1);               ///< The local scaling relative to the parent (or the world)
  float m_LocalUniformScaling = 1.0f;              ///< An additional local uniform scaling relative to the parent (or the world)
  WTagSet m_Tags;                                 ///< See WGameObject::GetTags()
  WUInt32 m_uiStableRandomSeed = 0xFFFFFFFF;      ///< 0 means the game object gets a random value assigned, 0xFFFFFFFF means that if the object has a parent, the value will be derived deterministically from that one's seed, otherwise it gets a random value, any other value will be used directly
};
