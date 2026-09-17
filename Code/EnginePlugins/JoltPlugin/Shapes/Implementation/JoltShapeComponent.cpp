#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

// clang-format off
W_BEGIN_ABSTRACT_COMPONENT_TYPE(WJoltShapeComponent, 1)
{
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Physics/Jolt/Shapes"),
  }
  W_END_ATTRIBUTES;
}
W_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

WJoltShapeComponent::WJoltShapeComponent() = default;
WJoltShapeComponent::~WJoltShapeComponent() = default;

void WJoltShapeComponent::Initialize()
{
  if (IsActive())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void WJoltShapeComponent::OnDeactivated()
{
  if (m_uiUserDataIndex != WInvalidIndex)
  {
    WJoltWorldModule* pModule = GetWorld()->GetModule<WJoltWorldModule>();
    pModule->DeallocateUserData(m_uiUserDataIndex);
  }

  SUPER::OnDeactivated();
}

const WJoltUserData* WJoltShapeComponent::GetUserData()
{
  WJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<WJoltWorldModule>();

  if (m_uiUserDataIndex != WInvalidIndex)
  {
    return &pModule->GetUserData(m_uiUserDataIndex);
  }
  else
  {
    WJoltUserData* pUserData = nullptr;
    m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
    pUserData->Init(this);

    return pUserData;
  }
}

WUInt32 WJoltShapeComponent::GetUserDataIndex()
{
  GetUserData();
  return m_uiUserDataIndex;
}


W_STATICLINK_FILE(JoltPlugin, JoltPlugin_Shapes_Implementation_JoltShapeComponent);
