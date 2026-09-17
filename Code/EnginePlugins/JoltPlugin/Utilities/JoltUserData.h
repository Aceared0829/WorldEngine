#pragma once

#include <JoltPlugin/Declarations.h>

#include <Foundation/Types/Bitflags.h>

class WComponent;
class WJoltDynamicActorComponent;
class WJoltStaticActorComponent;
class WJoltTriggerComponent;
class WJoltCharacterControllerComponent;
class WJoltShapeComponent;
class WJoltQueryShapeActorComponent;
class WJoltRagdollComponent;
class WJoltRopeComponent;
class WJoltActorComponent;
class WJoltClothSheetComponent;
class WJoltBreakableSlabComponent;
class WJoltHeightfieldColliderComponent;

class WJoltUserData
{
public:
  W_DECLARE_POD_TYPE();

  enum class Type
  {
    Invalid,
    DynamicActorComponent,
    StaticActorComponent,
    TriggerComponent,
    CharacterComponent,
    ShapeComponent,
    BreakableSlabComponent,
    QueryShapeActorComponent,
    RagdollComponent,
    RopeComponent,
    ClothSheetComponent,
    HeightfieldColliderComponent,
  };

  WJoltUserData() = default;
  ~WJoltUserData() = default;

  W_ALWAYS_INLINE void Init(WJoltDynamicActorComponent* pObject, WBitflags<WOnJoltContact> contactFlags)
  {
    m_Type = Type::DynamicActorComponent;
    m_pObject = pObject;
    m_OnContact = contactFlags;
  }

  W_ALWAYS_INLINE void Init(WJoltStaticActorComponent* pObject)
  {
    m_Type = Type::StaticActorComponent;
    m_pObject = pObject;
  }

  W_ALWAYS_INLINE void Init(WJoltTriggerComponent* pObject)
  {
    m_Type = Type::TriggerComponent;
    m_pObject = pObject;
  }

  W_ALWAYS_INLINE void Init(WJoltCharacterControllerComponent* pObject)
  {
    m_Type = Type::CharacterComponent;
    m_pObject = pObject;
  }

  W_ALWAYS_INLINE void Init(WJoltShapeComponent* pObject)
  {
    m_Type = Type::ShapeComponent;
    m_pObject = pObject;
  }

  W_ALWAYS_INLINE void Init(WJoltQueryShapeActorComponent* pObject)
  {
    m_Type = Type::QueryShapeActorComponent;
    m_pObject = pObject;
  }

  W_ALWAYS_INLINE void Init(WJoltRagdollComponent* pObject)
  {
    m_Type = Type::RagdollComponent;
    m_pObject = pObject;
  }

  W_ALWAYS_INLINE void Init(WJoltRopeComponent* pObject)
  {
    m_Type = Type::RopeComponent;
    m_pObject = pObject;
  }

  W_ALWAYS_INLINE void Init(WJoltClothSheetComponent* pObject)
  {
    m_Type = Type::ClothSheetComponent;
    m_pObject = pObject;
  }

  W_ALWAYS_INLINE void Init(WJoltBreakableSlabComponent* pObject, WBitflags<WOnJoltContact> contactFlags)
  {
    m_Type = Type::BreakableSlabComponent;
    m_pObject = pObject;
    m_OnContact = contactFlags;
  }

  W_ALWAYS_INLINE void Init(WJoltHeightfieldColliderComponent* pObject)
  {
    m_Type = Type::HeightfieldColliderComponent;
    m_pObject = pObject;
  }

  W_FORCE_INLINE void Invalidate()
  {
    m_Type = Type::Invalid;
    m_pObject = nullptr;
  }

  W_FORCE_INLINE static Type GetType(const void* pUserData)
  {
    const WJoltUserData* pJoltUserData = static_cast<const WJoltUserData*>(pUserData);
    if (pJoltUserData == nullptr)
      return Type::Invalid;

    return pJoltUserData->m_Type;
  }

  W_FORCE_INLINE void* GetObject() const
  {
    return m_pObject;
  }

  W_FORCE_INLINE static WComponent* GetComponent(const void* pUserData)
  {
    const WJoltUserData* pJoltUserData = static_cast<const WJoltUserData*>(pUserData);
    if (pJoltUserData == nullptr || pJoltUserData->m_Type == Type::Invalid)
    {
      return nullptr;
    }

    return static_cast<WComponent*>(pJoltUserData->m_pObject);
  }

  W_FORCE_INLINE static WJoltDynamicActorComponent* GetDynamicActorComponent(const void* pUserData)
  {
    const WJoltUserData* pJoltUserData = static_cast<const WJoltUserData*>(pUserData);
    if (pJoltUserData != nullptr && pJoltUserData->m_Type == Type::DynamicActorComponent)
    {
      return static_cast<WJoltDynamicActorComponent*>(pJoltUserData->m_pObject);
    }

    return nullptr;
  }

  W_FORCE_INLINE static WJoltTriggerComponent* GetTriggerComponent(const void* pUserData)
  {
    const WJoltUserData* pJoltUserData = static_cast<const WJoltUserData*>(pUserData);
    if (pJoltUserData != nullptr && pJoltUserData->m_Type == Type::TriggerComponent)
    {
      return static_cast<WJoltTriggerComponent*>(pJoltUserData->m_pObject);
    }

    return nullptr;
  }

  W_FORCE_INLINE static WBitflags<WOnJoltContact> GetContactFlags(const void* pUserData)
  {
    if (const WJoltUserData* pJoltUserData = static_cast<const WJoltUserData*>(pUserData))
    {
      return pJoltUserData->m_OnContact;
    }

    return WOnJoltContact::None;
  }

  W_FORCE_INLINE WBitflags<WOnJoltContact> GetContactFlags() const
  {
    return m_OnContact;
  }

private:
  Type m_Type = Type::Invalid;
  void* m_pObject = nullptr;
  WBitflags<WOnJoltContact> m_OnContact;
};
