#include "Foundation/Serialization/AbstractObjectGraph.h"

#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Utils/BlackboardTemplateResource.h>

struct BCFlags
{
  enum Enum
  {
    ShowDebugInfo = 0,
    SendEntryChangedMessage,
    InitializedFromTemplate,
    IsRuntimeSerialized,
  };
};

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WBlackboardEntry, WNoBase, 1, WRTTIDefaultAllocator<WBlackboardEntry>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new WDynamicStringEnumAttribute("BlackboardKeysEnum")),
    W_MEMBER_PROPERTY("InitialValue", m_InitialValue)->AddAttributes(new WDefaultValueAttribute(0)),
    W_BITFLAGS_MEMBER_PROPERTY("Flags", WBlackboardEntryFlags, m_Flags)
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

WResult WBlackboardEntry::Serialize(WStreamWriter& inout_stream) const
{
  inout_stream << m_sName;
  inout_stream << m_InitialValue;
  inout_stream << m_Flags;

  return W_SUCCESS;
}

WResult WBlackboardEntry::Deserialize(WStreamReader& inout_stream)
{
  inout_stream >> m_sName;
  inout_stream >> m_InitialValue;
  inout_stream >> m_Flags;

  return W_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_IMPLEMENT_MESSAGE_TYPE(WMsgBlackboardEntryChanged);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgBlackboardEntryChanged, 1, WRTTIDefaultAllocator<WMsgBlackboardEntryChanged>)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Name", GetName, SetName),
    W_MEMBER_PROPERTY("OldValue", m_OldValue),
    W_MEMBER_PROPERTY("NewValue", m_NewValue),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_ABSTRACT_COMPONENT_TYPE(WBlackboardComponent, 3)
{
  W_BEGIN_PROPERTIES
  {
    W_RESOURCE_MEMBER_PROPERTY("Template", m_hTemplate)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_BlackboardTemplate")),
    W_ACCESSOR_PROPERTY("ShowDebugInfo", GetShowDebugInfo, SetShowDebugInfo),
  }
  W_END_PROPERTIES;

  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgUpdateLocalBounds, OnUpdateLocalBounds),
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnExtractRenderData),
  }
  W_END_MESSAGEHANDLERS;

  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(Reflection_FindBlackboard, In, "SearchObject", In, "BlackboardName")->AddFlags(WPropertyFlags::PureFunction)->AddAttributes(new WFunctionArgumentAttributes(1, new WDynamicStringEnumAttribute("BlackboardNamesEnum"))),
    W_SCRIPT_FUNCTION_PROPERTY(SetEntryValue, In, "Name", In, "Value")->AddAttributes(new WFunctionArgumentAttributes(0, new WDynamicStringEnumAttribute("BlackboardKeysEnum"))),
    W_SCRIPT_FUNCTION_PROPERTY(GetEntryValue, In, "Name")->AddAttributes(new WFunctionArgumentAttributes(0, new WDynamicStringEnumAttribute("BlackboardKeysEnum"))),
  }
  W_END_FUNCTIONS;

  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Logic"),
  }
  W_END_ATTRIBUTES;
}
W_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

WBlackboardComponent::WBlackboardComponent() = default;
WBlackboardComponent::~WBlackboardComponent() = default;

// static
WSharedPtr<WBlackboard> WBlackboardComponent::FindBlackboard(WGameObject& ref_searchObject, WStringView sBlackboardName /*= WStringView()*/)
{
  const WTempHashedString sBlackboardNameHashed(sBlackboardName);

  WBlackboardComponent* pBlackboardComponent = nullptr;
  WGameObject* pObject = &ref_searchObject;
  while (pObject != nullptr)
  {
    if (pObject->TryGetComponentOfBaseType(pBlackboardComponent))
    {
      if (sBlackboardName.IsEmpty() || (pBlackboardComponent->GetBoard() && pBlackboardComponent->GetBoard()->GetNameHashed() == sBlackboardNameHashed))
      {
        return pBlackboardComponent->GetBoard();
      }
    }

    pObject = pObject->GetParent();
  }

  if (sBlackboardName.IsEmpty())
  {
    return ref_searchObject.GetWorld()->GetBlackboard();
  }
  else
  {
    WHashedString sHashedBlackboardName;
    sHashedBlackboardName.Assign(sBlackboardName);
    return WBlackboard::GetOrCreateGlobal(sHashedBlackboardName);
  }

  return nullptr;
}

void WBlackboardComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  WStreamWriter& s = inout_stream.GetStream();

  s << m_hTemplate;
}

void WBlackboardComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  if (uiVersion < 3)
    return;

  WStreamReader& s = inout_stream.GetStream();

  s >> m_hTemplate;
}

void WBlackboardComponent::OnActivated()
{
  SUPER::OnActivated();

  if (GetShowDebugInfo())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void WBlackboardComponent::OnDeactivated()
{
  if (GetShowDebugInfo())
  {
    GetOwner()->UpdateLocalBounds();
  }

  SUPER::OnDeactivated();
}

const WSharedPtr<WBlackboard>& WBlackboardComponent::GetBoard()
{
  return m_pBoard;
}

WSharedPtr<const WBlackboard> WBlackboardComponent::GetBoard() const
{
  return m_pBoard;
}

void WBlackboardComponent::SetShowDebugInfo(bool bShow)
{
  SetUserFlag(BCFlags::ShowDebugInfo, bShow);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

bool WBlackboardComponent::GetShowDebugInfo() const
{
  return GetUserFlag(BCFlags::ShowDebugInfo);
}

void WBlackboardComponent::SetEntryValue(const char* szName, const WVariant& value)
{
  if (m_pBoard)
  {
    m_pBoard->SetEntryValue(szName, value);
  }
}

WVariant WBlackboardComponent::GetEntryValue(const char* szName) const
{
  if (m_pBoard)
  {
    return m_pBoard->GetEntryValue(WTempHashedString(szName));
  }

  return {};
}

// static
WBlackboard* WBlackboardComponent::Reflection_FindBlackboard(WGameObject* pSearchObject, WStringView sBlackboardName)
{
  if (pSearchObject != nullptr)
  {
    return FindBlackboard(*pSearchObject, sBlackboardName).Borrow();
  }

  return nullptr;
}

void WBlackboardComponent::OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg) const
{
  if (GetShowDebugInfo())
  {
    msg.AddBounds(WBoundingSphere::MakeFromCenterAndRadius(WVec3::MakeZero(), 2.0f), WDefaultSpatialDataCategories::RenderDynamic);
  }
}

void WBlackboardComponent::OnExtractRenderData(WMsgExtractRenderData& msg) const
{
  if (!GetShowDebugInfo() || m_pBoard == nullptr)
    return;

  if (msg.m_pView->GetCameraUsageHint() != WCameraUsageHint::MainView &&
      msg.m_pView->GetCameraUsageHint() != WCameraUsageHint::EditorView)
    return;

  // Don't extract render data for selection.
  if (msg.m_OverrideCategory != WInvalidRenderDataCategory)
    return;

  auto& entries = m_pBoard->GetAllEntries();
  if (entries.IsEmpty())
    return;

  WStringBuilder sb;
  sb.Append(m_pBoard->GetName(), "\n");

  for (auto it = entries.GetIterator(); it.IsValid(); ++it)
  {
    sb.AppendFormat("{}: {}\n", it.Key(), it.Value().m_Value);
  }

  WDebugRenderer::Draw3DText(msg.m_pView->GetHandle(), sb, GetOwner()->GetGlobalPosition(), WColor::Orange);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WLocalBlackboardComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("BlackboardName", GetBlackboardName, SetBlackboardName)->AddAttributes(new WDynamicStringEnumAttribute("BlackboardNamesEnum")),
    W_ACCESSOR_PROPERTY("SendEntryChangedMessage", GetSendEntryChangedMessage, SetSendEntryChangedMessage),
    W_ARRAY_ACCESSOR_PROPERTY("Entries", Entries_GetCount, Entries_GetValue, Entries_SetValue, Entries_Insert, Entries_Remove),
  }
  W_END_PROPERTIES;

  W_BEGIN_MESSAGESENDERS
  {
    W_MESSAGE_SENDER(m_EntryChangedSender)
  }
  W_END_MESSAGESENDERS;
}
W_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

WLocalBlackboardComponent::WLocalBlackboardComponent()
{
  m_pBoard = WBlackboard::Create("");
}

WLocalBlackboardComponent::WLocalBlackboardComponent(WLocalBlackboardComponent&& other) = default;
WLocalBlackboardComponent::~WLocalBlackboardComponent() = default;
WLocalBlackboardComponent& WLocalBlackboardComponent::operator=(WLocalBlackboardComponent&& other) = default;

void WLocalBlackboardComponent::Initialize()
{
  SUPER::Initialize();

  if (IsActive())
  {
    // we already do this here, so that the BB is initialized even if OnSimulationStarted() hasn't been called yet
    InitializeFromTemplate();
    SetUserFlag(BCFlags::InitializedFromTemplate, true);
  }
}

void WLocalBlackboardComponent::OnActivated()
{
  SUPER::OnActivated();

  if (GetUserFlag(BCFlags::InitializedFromTemplate) == false)
  {
    InitializeFromTemplate();
  }
}

void WLocalBlackboardComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  SetUserFlag(BCFlags::InitializedFromTemplate, false);
}

void WLocalBlackboardComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // we repeat this here, mainly for the editor case, when the asset has been modified (new entries added)
  // and we then press play, to have the new entries in the BB
  // this would NOT update the initial values, though, if they changed
  InitializeFromTemplate();

  // override with the component specific initial entries
  for (auto& entry : m_InitialEntries)
  {
    m_pBoard->SetEntryValue(entry.m_sName, entry.m_InitialValue);
    m_pBoard->SetEntryFlags(entry.m_sName, entry.m_Flags).AssertSuccess();
  }
}

void WLocalBlackboardComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  WStreamWriter& s = inout_stream.GetStream();

  s << m_pBoard->GetName();
  s.WriteArray(m_InitialEntries).IgnoreResult();
}

void WLocalBlackboardComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SetUserFlag(BCFlags::IsRuntimeSerialized, true);

  const WUInt32 uiBaseVersion = inout_stream.GetComponentTypeVersion(WBlackboardComponent::GetStaticRTTI());
  if (uiBaseVersion < 3)
    return;

  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  WStreamReader& s = inout_stream.GetStream();

  WStringBuilder sb;
  s >> sb;
  m_pBoard->SetName(sb);
  m_pBoard->RemoveAllEntries();

  // we don't write the data to m_InitialEntries, because that is never needed anymore at runtime
  WDynamicArray<WBlackboardEntry> initialEntries;
  if (s.ReadArray(initialEntries).Succeeded())
  {
    for (WUInt32 i = 0; i < initialEntries.GetCount(); ++i)
    {
      auto& entry = initialEntries[i];

      m_pBoard->SetEntryValue(entry.m_sName, entry.m_InitialValue);
      m_pBoard->SetEntryFlags(entry.m_sName, entry.m_Flags).AssertSuccess();
      m_pBoard->SetEditorIndex(entry.m_sName, static_cast<WUInt8>(i)).AssertSuccess(); // allows us to map exposed parameters to the proper value
    }
  }
}

void WLocalBlackboardComponent::SetSendEntryChangedMessage(bool bSend)
{
  if (GetSendEntryChangedMessage() == bSend)
    return;

  SetUserFlag(BCFlags::SendEntryChangedMessage, bSend);

  if (bSend)
  {
    m_pBoard->OnEntryEvent().AddEventHandler(WMakeDelegate(&WLocalBlackboardComponent::OnEntryChanged, this));
  }
  else
  {
    m_pBoard->OnEntryEvent().RemoveEventHandler(WMakeDelegate(&WLocalBlackboardComponent::OnEntryChanged, this));
  }
}

bool WLocalBlackboardComponent::GetSendEntryChangedMessage() const
{
  return GetUserFlag(BCFlags::SendEntryChangedMessage);
}

void WLocalBlackboardComponent::SetBlackboardName(const char* szName)
{
  m_pBoard->SetName(szName);
}

const char* WLocalBlackboardComponent::GetBlackboardName() const
{
  return m_pBoard->GetName();
}

WUInt32 WLocalBlackboardComponent::Entries_GetCount() const
{
  if (IsEditor())
  {
    return m_InitialEntries.GetCount();
  }
  return m_pBoard->GetAllEntries().GetCount();
}

WBlackboardEntry WLocalBlackboardComponent::Entries_GetValue(WUInt32 uiIndex) const
{
  if (IsEditor())
  {
    return m_InitialEntries[uiIndex];
  }

  WHashedString sName = m_pBoard->FindNameForEditorIndex(static_cast<WUInt8>(uiIndex));
  WBlackboardEntry tempEntry;
  if (!sName.IsEmpty())
  {
    tempEntry.m_sName = sName;
    tempEntry.m_InitialValue = m_pBoard->GetEntryValue(sName);
    tempEntry.m_Flags = m_pBoard->GetEntryFlags(sName);
  }
  return tempEntry;
}

void WLocalBlackboardComponent::Entries_SetValue(WUInt32 uiIndex, WBlackboardEntry entry)
{
  if (IsEditor())
  {
    m_InitialEntries.EnsureCount(uiIndex + 1);

    // Remove old name under this index
    if (const WBlackboard::Entry* pEntry = m_pBoard->GetEntry(m_InitialEntries[uiIndex].m_sName))
    {
      if (m_InitialEntries[uiIndex].m_sName != entry.m_sName)
      {
        m_pBoard->RemoveEntry(m_InitialEntries[uiIndex].m_sName);
      }
    }
    m_InitialEntries[uiIndex] = entry;
  }

  m_pBoard->SetEntryValue(entry.m_sName, entry.m_InitialValue);
  m_pBoard->SetEntryFlags(entry.m_sName, entry.m_Flags).AssertSuccess();
}

void WLocalBlackboardComponent::Entries_Insert(WUInt32 uiIndex, WBlackboardEntry entry)
{
  if (IsEditor())
  {
    m_InitialEntries.InsertAt(uiIndex, entry);
  }

  m_pBoard->SetEntryValue(entry.m_sName, entry.m_InitialValue);
  m_pBoard->SetEntryFlags(entry.m_sName, entry.m_Flags).AssertSuccess();
  m_pBoard->SetEditorIndex(entry.m_sName, static_cast<WUInt8>(uiIndex)).AssertSuccess();
}

void WLocalBlackboardComponent::Entries_Remove(WUInt32 uiIndex)
{
  if (IsEditor())
  {
    auto& entry = m_InitialEntries[uiIndex];
    m_pBoard->RemoveEntry(entry.m_sName);

    m_InitialEntries.RemoveAtAndCopy(uiIndex);
  }
}

void WLocalBlackboardComponent::OnEntryChanged(const WBlackboard::EntryEvent& e)
{
  if (!IsActiveAndInitialized())
    return;

  WMsgBlackboardEntryChanged msg;
  msg.m_sName = e.m_sName;
  msg.m_OldValue = e.m_OldValue;
  msg.m_NewValue = e.m_pEntry->m_Value;

  m_EntryChangedSender.SendEventMessage(msg, this, GetOwner());
}

void WLocalBlackboardComponent::InitializeFromTemplate()
{
  if (!m_hTemplate.IsValid())
    return;

  WResourceLock<WBlackboardTemplateResource> pTemplate(m_hTemplate, WResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pTemplate.GetAcquireResult() != WResourceAcquireResult::Final)
    return;

  for (const auto& entry : pTemplate->GetDescriptor().m_Entries)
  {
    m_pBoard->SetEntryValue(entry.m_sName, entry.m_InitialValue);
    m_pBoard->SetEntryFlags(entry.m_sName, entry.m_Flags).AssertSuccess();
  }
}

bool WLocalBlackboardComponent::IsEditor() const
{
  return !GetUserFlag(BCFlags::IsRuntimeSerialized);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WGlobalBlackboardInitMode, 1)
  W_ENUM_CONSTANTS(WGlobalBlackboardInitMode::EnsureEntriesExist, WGlobalBlackboardInitMode::ResetEntryValues, WGlobalBlackboardInitMode::ClearEntireBlackboard)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_COMPONENT_TYPE(WGlobalBlackboardComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("BlackboardName", GetBlackboardName, SetBlackboardName)->AddAttributes(new WDynamicStringEnumAttribute("BlackboardNamesEnum")),
    W_ENUM_MEMBER_PROPERTY("InitMode", WGlobalBlackboardInitMode, m_InitMode),
  }
  W_END_PROPERTIES;

  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Logic"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

WGlobalBlackboardComponent::WGlobalBlackboardComponent() = default;
WGlobalBlackboardComponent::WGlobalBlackboardComponent(WGlobalBlackboardComponent&& other) = default;
WGlobalBlackboardComponent::~WGlobalBlackboardComponent() = default;
WGlobalBlackboardComponent& WGlobalBlackboardComponent::operator=(WGlobalBlackboardComponent&& other) = default;

void WGlobalBlackboardComponent::Initialize()
{
  SUPER::Initialize();

  if (IsActive())
  {
    // we already do this here, so that the BB is initialized even if OnSimulationStarted() hasn't been called yet
    InitializeFromTemplate();
    SetUserFlag(BCFlags::InitializedFromTemplate, true);
  }
}

void WGlobalBlackboardComponent::OnActivated()
{
  SUPER::OnActivated();

  if (GetUserFlag(BCFlags::InitializedFromTemplate) == false)
  {
    InitializeFromTemplate();
  }
}

void WGlobalBlackboardComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  SetUserFlag(BCFlags::InitializedFromTemplate, false);
}

void WGlobalBlackboardComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // we repeat this here, mainly for the editor case, when the asset has been modified (new entries added)
  // and we then press play, to have the new entries in the BB
  // this would NOT update the initial values, though, if they changed
  InitializeFromTemplate();
}

void WGlobalBlackboardComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  WStreamWriter& s = inout_stream.GetStream();

  s << m_sName;
  s << m_InitMode;
}

void WGlobalBlackboardComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  const WUInt32 uiBaseVersion = inout_stream.GetComponentTypeVersion(WBlackboardComponent::GetStaticRTTI());
  if (uiBaseVersion < 3)
    return;

  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  WStreamReader& s = inout_stream.GetStream();

  s >> m_sName;
  s >> m_InitMode;
}

void WGlobalBlackboardComponent::SetBlackboardName(const char* szName)
{
  m_sName.Assign(szName);
}

const char* WGlobalBlackboardComponent::GetBlackboardName() const
{
  return m_sName;
}

void WGlobalBlackboardComponent::InitializeFromTemplate()
{
  m_pBoard = WBlackboard::GetOrCreateGlobal(m_sName);

  if (m_InitMode == WGlobalBlackboardInitMode::ClearEntireBlackboard)
  {
    m_pBoard->RemoveAllEntries();
  }

  if (!m_hTemplate.IsValid())
    return;

  WResourceLock<WBlackboardTemplateResource> pTemplate(m_hTemplate, WResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pTemplate.GetAcquireResult() != WResourceAcquireResult::Final)
    return;

  for (const auto& entry : pTemplate->GetDescriptor().m_Entries)
  {
    if (!m_pBoard->HasEntry(entry.m_sName) || m_InitMode != WGlobalBlackboardInitMode::EnsureEntriesExist)
    {
      // make sure the entry exists and enforce that it has this value
      m_pBoard->SetEntryValue(entry.m_sName, entry.m_InitialValue);
      // also overwrite the flags
      m_pBoard->SetEntryFlags(entry.m_sName, entry.m_Flags).AssertSuccess();
    }
  }
}


class WBlackboardComponent_2_3 : public WGraphPatch
{
public:
  WBlackboardComponent_2_3()
    : WGraphPatch("WBlackboardComponent", 3)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    ref_context.RenameClass("WLocalBlackboardComponent");
  }
};

WBlackboardComponent_2_3 g_WBlackboardComponent_2_3;


W_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_BlackboardComponent);
