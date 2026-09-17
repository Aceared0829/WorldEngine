#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Configuration/SubSystem.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <GuiFoundation/PropertyGrid/AttributeDefaultStateProvider.h>
#include <GuiFoundation/PropertyGrid/DefaultState.h>
#include <GuiFoundation/PropertyGrid/PrefabDefaultStateProvider.h>
#include <GuiFoundation/PropertyGrid/VariantSubDefaultStateProvider.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, DefaultState)
  ON_CORESYSTEMS_STARTUP
  {
    WDefaultState::RegisterDefaultStateProvider(WAttributeDefaultStateProvider::CreateProvider);
    WDefaultState::RegisterDefaultStateProvider(WPrefabDefaultStateProvider::CreateProvider);
    WDefaultState::RegisterDefaultStateProvider(WVariantSubDefaultStateProvider::CreateProvider);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WDefaultState::UnregisterDefaultStateProvider(WAttributeDefaultStateProvider::CreateProvider);
    WDefaultState::UnregisterDefaultStateProvider(WPrefabDefaultStateProvider::CreateProvider);
    WDefaultState::UnregisterDefaultStateProvider(WVariantSubDefaultStateProvider::CreateProvider);
  }
W_END_SUBSYSTEM_DECLARATION;
// clang-format on


WDynamicArray<WDefaultState::CreateStateProviderFunc> WDefaultState::s_Factories;

void WDefaultState::RegisterDefaultStateProvider(CreateStateProviderFunc func)
{
  s_Factories.PushBack(func);
}

void WDefaultState::UnregisterDefaultStateProvider(CreateStateProviderFunc func)
{
  s_Factories.RemoveAndCopy(func);
}

//////////////////////////////////////////////////////////////////////////

WDefaultObjectState::WDefaultObjectState(const WRTTI* pType, WObjectAccessorBase* pAccessor, const WArrayPtr<WPropertySelection> selection)
{
  m_pType = pType;
  m_pAccessor = pAccessor;
  m_Selection = selection;
  m_Providers.Reserve(m_Selection.GetCount());
  for (const WPropertySelection& sel : m_Selection)
  {
    auto& pProviders = m_Providers.ExpandAndGetRef();
    for (auto& func : WDefaultState::s_Factories)
    {
      WSharedPtr<WDefaultStateProvider> pProvider = func(pAccessor, sel.m_pObject, nullptr);
      if (pProvider != nullptr)
      {
        pProviders.PushBack(std::move(pProvider));
      }
      pProviders.Sort([](const WSharedPtr<WDefaultStateProvider>& pA, const WSharedPtr<WDefaultStateProvider>& pB) -> bool
        { return pA->GetRootDepth() > pB->GetRootDepth(); });
    }
  }
}

WColorGammaUB WDefaultObjectState::GetBackgroundColor() const
{
  return m_Providers[0][0]->GetBackgroundColor();
}

WString WDefaultObjectState::GetStateProviderName() const
{
  return m_Providers[0][0]->GetStateProviderName();
}

bool WDefaultObjectState::IsDefaultValue(const char* szProperty) const
{
  const WAbstractProperty* pProp = m_pType->FindPropertyByName(szProperty);
  return IsDefaultValue(pProp);
}

bool WDefaultObjectState::IsDefaultValue(const WAbstractProperty* pProp) const
{
  const WUInt32 uiObjects = m_Providers.GetCount();
  for (WUInt32 i = 0; i < uiObjects; i++)
  {
    WDefaultStateProvider::SuperArray super = m_Providers[i].GetArrayPtr().GetSubArray(1);
    const bool bNewDefault = m_Providers[i][0]->IsDefaultValue(super, m_pAccessor, m_Selection[i].m_pObject, pProp);
    if (!bNewDefault)
      return false;
  }
  return true;
}

WStatus WDefaultObjectState::RevertProperty(const char* szProperty)
{
  const WAbstractProperty* pProp = m_pType->FindPropertyByName(szProperty);
  return RevertProperty(pProp);
}

WStatus WDefaultObjectState::RevertProperty(const WAbstractProperty* pProp)
{
  const WUInt32 uiObjects = m_Providers.GetCount();
  for (WUInt32 i = 0; i < uiObjects; i++)
  {
    WDefaultStateProvider::SuperArray super = m_Providers[i].GetArrayPtr().GetSubArray(1);
    WStatus res = m_Providers[i][0]->RevertProperty(super, m_pAccessor, m_Selection[i].m_pObject, pProp);
    if (res.Failed())
      return res;
  }
  return WStatus(W_SUCCESS);
}

WStatus WDefaultObjectState::RevertObject()
{
  const WUInt32 uiObjects = m_Providers.GetCount();
  for (WUInt32 i = 0; i < uiObjects; i++)
  {
    WDefaultStateProvider::SuperArray super = m_Providers[i].GetArrayPtr().GetSubArray(1);

    WTempHybridArray<const WAbstractProperty*, 32> properties;
    m_Selection[i].m_pObject->GetType()->GetAllProperties(properties);
    for (auto pProp : properties)
    {
      if (pProp->GetFlags().IsAnySet(WPropertyFlags::Hidden | WPropertyFlags::ReadOnly))
        continue;
      WStatus res = m_Providers[i][0]->RevertProperty(super, m_pAccessor, m_Selection[i].m_pObject, pProp);
      if (res.Failed())
        return res;
    }
  }
  return WStatus(W_SUCCESS);
}

WVariant WDefaultObjectState::GetDefaultValue(const char* szProperty, WUInt32 uiSelectionIndex) const
{
  const WAbstractProperty* pProp = m_pType->FindPropertyByName(szProperty);
  return GetDefaultValue(pProp, uiSelectionIndex);
}

WVariant WDefaultObjectState::GetDefaultValue(const WAbstractProperty* pProp, WUInt32 uiSelectionIndex) const
{
  W_ASSERT_DEBUG(uiSelectionIndex < m_Selection.GetCount(), "Selection index is out of bounds.");
  WDefaultStateProvider::SuperArray super = m_Providers[uiSelectionIndex].GetArrayPtr().GetSubArray(1);
  return m_Providers[uiSelectionIndex][0]->GetDefaultValue(super, m_pAccessor, m_Selection[uiSelectionIndex].m_pObject, pProp);
}

//////////////////////////////////////////////////////////////////////////

WDefaultContainerState::WDefaultContainerState(const WRTTI* pType, WObjectAccessorBase* pAccessor, const WArrayPtr<WPropertySelection> selection, const char* szProperty)
{
  m_pType = pType;
  m_pAccessor = pAccessor;
  m_Selection = selection;
  // We assume selections can only contain objects of the same (base) type.
  m_pProp = szProperty ? m_pType->FindPropertyByName(szProperty) : nullptr;
  m_Providers.Reserve(m_Selection.GetCount());
  for (const WPropertySelection& sel : m_Selection)
  {
    auto& pProviders = m_Providers.ExpandAndGetRef();
    for (auto& func : WDefaultState::s_Factories)
    {
      WSharedPtr<WDefaultStateProvider> pProvider = func(pAccessor, sel.m_pObject, m_pProp);
      if (pProvider != nullptr)
      {
        pProviders.PushBack(std::move(pProvider));
      }
      pProviders.Sort([](const WSharedPtr<WDefaultStateProvider>& pA, const WSharedPtr<WDefaultStateProvider>& pB) -> bool
        { return pA->GetRootDepth() > pB->GetRootDepth(); });
    }
  }
}

WColorGammaUB WDefaultContainerState::GetBackgroundColor() const
{
  return m_Providers[0][0]->GetBackgroundColor();
}

WString WDefaultContainerState::GetStateProviderName() const
{
  return m_Providers[0][0]->GetStateProviderName();
}

bool WDefaultContainerState::IsDefaultElement(WVariant index) const
{
  const WUInt32 uiObjects = m_Providers.GetCount();
  for (WUInt32 i = 0; i < uiObjects; i++)
  {
    WDefaultStateProvider::SuperArray super = m_Providers[i].GetArrayPtr().GetSubArray(1);
    W_ASSERT_DEBUG(index.IsValid() || m_Selection[i].m_Index.IsValid(), "If WDefaultContainerState is constructed without giving an indices in the selection, one must be provided on the IsDefaultElement call.");
    const bool bNewDefault = m_Providers[i][0]->IsDefaultValue(super, m_pAccessor, m_Selection[i].m_pObject, m_pProp, index.IsValid() ? index : m_Selection[i].m_Index);
    if (!bNewDefault)
      return false;
  }
  return true;
}

bool WDefaultContainerState::IsDefaultContainer() const
{
  const WUInt32 uiObjects = m_Providers.GetCount();
  for (WUInt32 i = 0; i < uiObjects; i++)
  {
    WDefaultStateProvider::SuperArray super = m_Providers[i].GetArrayPtr().GetSubArray(1);
    const bool bNewDefault = m_Providers[i][0]->IsDefaultValue(super, m_pAccessor, m_Selection[i].m_pObject, m_pProp);
    if (!bNewDefault)
      return false;
  }
  return true;
}

WStatus WDefaultContainerState::RevertElement(WVariant index)
{
  const WUInt32 uiObjects = m_Providers.GetCount();
  for (WUInt32 i = 0; i < uiObjects; i++)
  {
    WDefaultStateProvider::SuperArray super = m_Providers[i].GetArrayPtr().GetSubArray(1);
    W_ASSERT_DEBUG(index.IsValid() || m_Selection[i].m_Index.IsValid(), "If WDefaultContainerState is constructed without giving an indices in the selection, one must be provided on the RevertElement call.");
    WStatus res = m_Providers[i][0]->RevertProperty(super, m_pAccessor, m_Selection[i].m_pObject, m_pProp, index.IsValid() ? index : m_Selection[i].m_Index);
    if (res.Failed())
      return res;
  }
  return WStatus(W_SUCCESS);
}

WStatus WDefaultContainerState::RevertContainer()
{
  const WUInt32 uiObjects = m_Providers.GetCount();
  for (WUInt32 i = 0; i < uiObjects; i++)
  {
    WDefaultStateProvider::SuperArray super = m_Providers[i].GetArrayPtr().GetSubArray(1);
    WStatus res = m_Providers[i][0]->RevertProperty(super, m_pAccessor, m_Selection[i].m_pObject, m_pProp);
    if (res.Failed())
      return res;
  }
  return WStatus(W_SUCCESS);
}

WVariant WDefaultContainerState::GetDefaultElement(WVariant index, WUInt32 uiSelectionIndex) const
{
  W_ASSERT_DEBUG(uiSelectionIndex < m_Selection.GetCount(), "Selection index is out of bounds.");
  WDefaultStateProvider::SuperArray super = m_Providers[uiSelectionIndex].GetArrayPtr().GetSubArray(1);
  return m_Providers[uiSelectionIndex][0]->GetDefaultValue(super, m_pAccessor, m_Selection[uiSelectionIndex].m_pObject, m_pProp, index);
}

WVariant WDefaultContainerState::GetDefaultContainer(WUInt32 uiSelectionIndex) const
{
  W_ASSERT_DEBUG(uiSelectionIndex < m_Selection.GetCount(), "Selection index is out of bounds.");
  WDefaultStateProvider::SuperArray super = m_Providers[uiSelectionIndex].GetArrayPtr().GetSubArray(1);
  return m_Providers[uiSelectionIndex][0]->GetDefaultValue(super, m_pAccessor, m_Selection[uiSelectionIndex].m_pObject, m_pProp);
}

//////////////////////////////////////////////////////////////////////////


bool WDefaultStateProvider::IsDefaultValue(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index)
{
  const WVariant def = GetDefaultValue(superPtr, pAccessor, pObject, pProp, index);
  WVariant value;
  pAccessor->GetValue(pObject, pProp, value, index).LogFailure();

  const bool bIsValueType = WReflectionUtils::IsValueType(pProp) || pProp->GetFlags().IsAnySet(WPropertyFlags::IsEnum | WPropertyFlags::Bitflags);
  if (index.IsValid() && !bIsValueType)
  {
    // #TODO we do not support reverting entire objects just yet.
    return true;
  }

  return def == value;
}

WStatus WDefaultStateProvider::RevertProperty(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index)
{
  const bool bIsValueType = WReflectionUtils::IsValueType(pProp) || pProp->GetFlags().IsAnySet(WPropertyFlags::IsEnum | WPropertyFlags::Bitflags);
  if (!bIsValueType)
  {
    W_ASSERT_DEBUG(!index.IsValid(), "Reverting non-value type container elements is not supported yet. IsDefaultValue should have returned true to prevent this call from being allowed.");

    return RevertObjectContainer(superPtr, pAccessor, pObject, pProp);
  }

  WDeque<WAbstractGraphDiffOperation> diff;
  auto& op = diff.ExpandAndGetRef();
  op.m_Node = pObject->GetGuid();
  op.m_Operation = WAbstractGraphDiffOperation::Op::PropertyChanged;
  op.m_sProperty = pProp->GetPropertyName();
  op.m_uiTypeVersion = 0;
  if (index.IsValid())
  {
    WVariant def = GetDefaultValue(superPtr, pAccessor, pObject, pProp, index);
    switch (pProp->GetCategory())
    {
      case WPropertyCategory::Array:
      case WPropertyCategory::Set:
      {
        W_ASSERT_DEBUG(index.CanConvertTo<WInt32>(), "Array / Set indices must be integers.");
        W_SUCCEED_OR_RETURN(pAccessor->GetValue(pObject, pProp, op.m_Value));
        W_ASSERT_DEBUG(op.m_Value.IsA<WVariantArray>(), "");

        WVariantArray& currentValue2 = op.m_Value.GetWritable<WVariantArray>();
        currentValue2[index.ConvertTo<WUInt32>()] = def;
      }
      break;
      case WPropertyCategory::Map:
      {
        W_ASSERT_DEBUG(index.IsString(), "Map indices must be strings.");
        W_SUCCEED_OR_RETURN(pAccessor->GetValue(pObject, pProp, op.m_Value));
        W_ASSERT_DEBUG(op.m_Value.IsA<WVariantDictionary>(), "");

        WVariantDictionary& currentValue2 = op.m_Value.GetWritable<WVariantDictionary>();
        currentValue2[index.ConvertTo<WString>()] = def;
      }
      break;
      default:
        break;
    }
  }
  else
  {
    WVariant def = GetDefaultValue(superPtr, pAccessor, pObject, pProp, index);
    op.m_Value = def;
  }

  WDocumentObjectConverterReader::ApplyDiffToObject(pAccessor, pObject, diff);
  return WStatus(W_SUCCESS);
}

WStatus WDefaultStateProvider::RevertObjectContainer(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp)
{
  WDeque<WAbstractGraphDiffOperation> diff;
  WStatus res = CreateRevertContainerDiff(superPtr, pAccessor, pObject, pProp, diff);
  if (res.Succeeded())
  {
    WDocumentObjectConverterReader::ApplyDiffToObject(pAccessor, pObject, diff);
  }
  return res;
}

bool WDefaultStateProvider::DoesVariantMatchProperty(const WVariant& value, const WAbstractProperty* pProp, WVariant index)
{
  const bool bIsValueType = WReflectionUtils::IsValueType(pProp) || pProp->GetFlags().IsAnySet(WPropertyFlags::IsEnum | WPropertyFlags::Bitflags);

  if (pProp->GetSpecificType() == WGetStaticRTTI<WVariant>())
    return true;

  auto MatchesElementType = [&](const WVariant& value2) -> bool
  {
    if (pProp->GetFlags().IsAnySet(WPropertyFlags::IsEnum | WPropertyFlags::Bitflags))
    {
      return value2.IsNumber() && !value2.IsFloatingPoint();
    }
    else if (pProp->GetFlags().IsAnySet(WPropertyFlags::StandardType))
    {
      return value2.CanConvertTo(pProp->GetSpecificType()->GetVariantType());
    }
    else if (bIsValueType)
    {
      return value2.GetReflectedType() == pProp->GetSpecificType();
    }
    else
    {
      return value2.IsA<WUuid>();
    }
  };

  switch (pProp->GetCategory())
  {
    case WPropertyCategory::Member:
    {
      return MatchesElementType(value);
    }
    break;
    case WPropertyCategory::Array:
    case WPropertyCategory::Set:
    {
      if (index.IsValid())
      {
        return MatchesElementType(value);
      }
      else
      {
        if (value.IsA<WVariantArray>())
        {
          const WVariantArray& valueArray = value.Get<WVariantArray>();
          return std::all_of(cbegin(valueArray), cend(valueArray), MatchesElementType);
        }
      }
    }
    break;
    case WPropertyCategory::Map:
    {
      if (index.IsValid())
      {
        return MatchesElementType(value);
      }
      else
      {
        if (value.IsA<WVariantDictionary>())
        {
          const WVariantDictionary& valueDict = value.Get<WVariantDictionary>();
          return std::all_of(cbegin(valueDict), cend(valueDict), [&](const auto& it)
            { return MatchesElementType(it.Value()); });
        }
      }
    }
    break;
    default:
      break;
  }
  return false;
}
