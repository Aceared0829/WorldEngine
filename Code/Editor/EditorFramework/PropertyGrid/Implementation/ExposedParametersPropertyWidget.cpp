#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/GUI/ExposedParametersTypeRegistry.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/GroupBoxBase.moc.h>
#include <ToolsFoundation/Reflection/VariantStorageAccessor.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WExposedParameterCommandAccessor, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WExposedParametersAsTypeCommandAccessor, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

bool WQtExposedParametersPropertyWidget::s_bRawMode = false;

WExposedParameterCommandAccessor::WExposedParameterCommandAccessor(
  WObjectAccessorBase* pSource, const WAbstractProperty* pParameterProp, const WAbstractProperty* pParameterSourceProp)
  : WObjectProxyAccessor(pSource)
  , m_pParameterProp(pParameterProp)
  , m_pParameterSourceProp(pParameterSourceProp)
{
}

WStatus WExposedParameterCommandAccessor::GetValue(
  const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant& out_value, WVariant index /*= WVariant()*/)
{
  if (IsExposedProperty(pObject, pProp))
    pProp = m_pParameterProp;

  WStatus res = WObjectProxyAccessor::GetValue(pObject, pProp, out_value, index);
  if (res.Succeeded() && !index.IsValid() && m_pParameterProp == pProp)
  {
    WVariantDictionary defaultDict;
    if (const WExposedParameters* pParams = GetExposedParams(pObject))
    {
      for (WExposedParameter* pParam : pParams->m_Parameters)
      {
        defaultDict.Insert(pParam->m_sName, pParam->m_DefaultValue);
      }
    }
    const WVariantDictionary& overwrittenDict = out_value.Get<WVariantDictionary>();
    for (auto it : overwrittenDict)
    {
      defaultDict[it.Key()] = it.Value();
    }
    out_value = defaultDict;
  }
  else if (res.Failed() && m_pParameterProp == pProp && index.IsA<WString>())
  {
    // If the actual GetValue fails but the key is an exposed param, return its default value instead.
    if (const WExposedParameter* pParam = GetExposedParam(pObject, index.Get<WString>()))
    {
      out_value = pParam->m_DefaultValue;
      return WStatus(W_SUCCESS);
    }
  }
  return res;
}

WStatus WExposedParameterCommandAccessor::SetValue(
  const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& newValue, WVariant index /*= WVariant()*/)
{
  if (IsExposedProperty(pObject, pProp))
    pProp = m_pParameterProp;

  WStatus res = WObjectProxyAccessor::SetValue(pObject, pProp, newValue, index);
  // As we pretend the exposed params always exist the actual SetValue will fail if this is not actually true,
  // so we redirect to insert to make it true.
  if (res.Failed() && m_pParameterProp == pProp && index.IsA<WString>())
  {
    return WExposedParameterCommandAccessor::InsertValue(pObject, pProp, newValue, index);
  }
  return res;
}

WStatus WExposedParameterCommandAccessor::RemoveValue(
  const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index /*= WVariant()*/)
{
  WStatus res = WObjectProxyAccessor::RemoveValue(pObject, pProp, index);
  if (res.Failed() && m_pParameterProp == pProp && index.IsA<WString>())
  {
    // It this is one of the exposed params, pretend we removed it successfully to suppress error messages.
    if (const WExposedParameter* pParam = GetExposedParam(pObject, index.Get<WString>()))
    {
      return WStatus(W_SUCCESS);
    }
  }
  return res;
}

WStatus WExposedParameterCommandAccessor::GetCount(const WDocumentObject* pObject, const WAbstractProperty* pProp, WInt32& out_iCount)
{
  if (m_pParameterProp == pProp)
  {
    WTempHybridArray<WVariant, 16> keys;
    GetKeys(pObject, pProp, keys).AssertSuccess();
    out_iCount = keys.GetCount();
    return WStatus(W_SUCCESS);
  }
  return WObjectProxyAccessor::GetCount(pObject, pProp, out_iCount);
}

WStatus WExposedParameterCommandAccessor::GetKeys(
  const WDocumentObject* pObject, const WAbstractProperty* pProp, WDynamicArray<WVariant>& out_keys)
{
  if (m_pParameterProp == pProp)
  {
    if (const WExposedParameters* pParams = GetExposedParams(pObject))
    {
      for (const auto& pParam : pParams->m_Parameters)
      {
        out_keys.PushBack(WVariant(pParam->m_sName));
      }

      WTempHybridArray<WVariant, 16> realKeys;
      WStatus res = WObjectProxyAccessor::GetKeys(pObject, pProp, realKeys);
      for (const auto& key : realKeys)
      {
        if (!out_keys.Contains(key))
        {
          out_keys.PushBack(key);
        }
      }
      return WStatus(W_SUCCESS);
    }
  }
  return WObjectProxyAccessor::GetKeys(pObject, pProp, out_keys);
}

WStatus WExposedParameterCommandAccessor::GetValues(
  const WDocumentObject* pObject, const WAbstractProperty* pProp, WDynamicArray<WVariant>& out_values)
{
  if (m_pParameterProp == pProp)
  {
    WTempHybridArray<WVariant, 16> keys;
    GetKeys(pObject, pProp, keys).AssertSuccess();
    for (const auto& key : keys)
    {
      auto& var = out_values.ExpandAndGetRef();
      W_VERIFY(GetValue(pObject, pProp, var, key).Succeeded(), "GetValue to valid a key should be not fail.");
    }
    return WStatus(W_SUCCESS);
  }
  return WObjectProxyAccessor::GetValues(pObject, pProp, out_values);
}


const WExposedParameters* WExposedParameterCommandAccessor::GetExposedParams(const WDocumentObject* pObject)
{
  WVariant value;
  if (WObjectProxyAccessor::GetValue(pObject, m_pParameterSourceProp, value).Succeeded())
  {
    if (value.IsA<WString>())
    {
      const auto& sValue = value.Get<WString>();
      if (const auto asset = WAssetCurator::GetSingleton()->FindSubAsset(sValue.GetData()))
      {
        return asset->m_pAssetInfo->m_Info->GetMetaInfo<WExposedParameters>();
      }
    }
  }
  return nullptr;
}


const WExposedParameter* WExposedParameterCommandAccessor::GetExposedParam(const WDocumentObject* pObject, const char* szParamName)
{
  if (const WExposedParameters* pParams = GetExposedParams(pObject))
  {
    return pParams->Find(szParamName);
  }
  return nullptr;
}


const WRTTI* WExposedParameterCommandAccessor::GetExposedParamsType(const WDocumentObject* pObject)
{
  WVariant value;
  if (WObjectProxyAccessor::GetValue(pObject, m_pParameterSourceProp, value).Succeeded())
  {
    if (value.IsA<WString>())
    {
      const auto& sValue = value.Get<WString>();
      if (const auto asset = WAssetCurator::GetSingleton()->FindSubAsset(sValue.GetData()))
      {
        return WExposedParametersTypeRegistry::GetSingleton()->GetExposedParametersType(sValue);
      }
    }
  }
  return nullptr;
}

const WRTTI* WExposedParameterCommandAccessor::GetCommonExposedParamsType(const WArrayPtr<WPropertySelection>& items)
{
  const WRTTI* type = nullptr;
  bool bFirst = true;
  // check if we have multiple values
  for (const auto& item : items)
  {
    if (bFirst)
    {
      type = GetExposedParamsType(item.m_pObject);
      bFirst = false;
    }
    else
    {
      auto type2 = GetExposedParamsType(item.m_pObject);
      if (type != type2)
      {
        return nullptr;
      }
    }
  }
  return type;
}

bool WExposedParameterCommandAccessor::IsExposedProperty(const WDocumentObject* pObject, const WAbstractProperty* pProp)
{
  if (auto type = GetExposedParamsType(pObject))
  {
    auto props = type->GetProperties();
    return std::any_of(cbegin(props), cend(props), [&](const WAbstractProperty* pOtherProp)
      { return pOtherProp == pProp; });
  }
  return false;
}

//////////////////////////////////////////////////////////////////////////

WExposedParametersAsTypeCommandAccessor::WExposedParametersAsTypeCommandAccessor(WExposedParameterCommandAccessor* pSource)
  : WObjectProxyAccessor(pSource)
{
}

WStatus WExposedParametersAsTypeCommandAccessor::GetValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant& out_value, WVariant index)
{
  W_SUCCEED_OR_RETURN(GetSubValue(pObject, pProp, out_value));

  WStatus result(W_SUCCESS);
  out_value = WVariantStorageAccessor(pProp->GetPropertyName(), out_value).GetValue(index, &result);
  return result;
}

WStatus WExposedParametersAsTypeCommandAccessor::SetValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& newValue, WVariant index)
{
  return SetSubValue(pObject, pProp, [&](WVariant& subValue) -> WStatus
    { return WVariantStorageAccessor(pProp->GetPropertyName(), subValue).SetValue(newValue, index); });
}

WStatus WExposedParametersAsTypeCommandAccessor::InsertValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& newValue, WVariant index)
{
  return SetSubValue(pObject, pProp, [&](WVariant& subValue) -> WStatus
    { return WVariantStorageAccessor(pProp->GetPropertyName(), subValue).InsertValue(index, newValue); });
}

WStatus WExposedParametersAsTypeCommandAccessor::RemoveValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index)
{
  return SetSubValue(pObject, pProp, [&](WVariant& subValue) -> WStatus
    { return WVariantStorageAccessor(pProp->GetPropertyName(), subValue).RemoveValue(index); });
}

WStatus WExposedParametersAsTypeCommandAccessor::MoveValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& oldIndex, const WVariant& newIndex)
{
  return SetSubValue(pObject, pProp, [&](WVariant& subValue) -> WStatus
    { return WVariantStorageAccessor(pProp->GetPropertyName(), subValue).MoveValue(oldIndex, newIndex); });
}

WStatus WExposedParametersAsTypeCommandAccessor::GetCount(const WDocumentObject* pObject, const WAbstractProperty* pProp, WInt32& out_iCount)
{
  WVariant subValue;
  W_SUCCEED_OR_RETURN(GetSubValue(pObject, pProp, subValue));
  out_iCount = WVariantStorageAccessor(pProp->GetPropertyName(), subValue).GetCount();
  return W_SUCCESS;
}

WStatus WExposedParametersAsTypeCommandAccessor::GetKeys(const WDocumentObject* pObject, const WAbstractProperty* pProp, WDynamicArray<WVariant>& out_keys)
{
  WVariant subValue;
  W_SUCCEED_OR_RETURN(GetSubValue(pObject, pProp, subValue));
  return WVariantStorageAccessor(pProp->GetPropertyName(), subValue).GetKeys(out_keys);
}

WStatus WExposedParametersAsTypeCommandAccessor::GetValues(const WDocumentObject* pObject, const WAbstractProperty* pProp, WDynamicArray<WVariant>& out_values)
{
  WVariant subValue;
  W_SUCCEED_OR_RETURN(GetSubValue(pObject, pProp, subValue));
  WTempHybridArray<WVariant, 16> keys;
  WVariantStorageAccessor accessor(pProp->GetPropertyName(), subValue);
  W_SUCCEED_OR_RETURN(accessor.GetKeys(keys));
  out_values.Clear();
  out_values.Reserve(keys.GetCount());
  for (const WVariant& key : keys)
  {
    out_values.PushBack(accessor.GetValue(key));
  }
  return W_SUCCESS;
}

WObjectAccessorBase* WExposedParametersAsTypeCommandAccessor::ResolveProxy(const WDocumentObject*& ref_pObject, const WRTTI*& ref_pType, const WAbstractProperty*& ref_pProp, WDynamicArray<WVariant>& ref_indices)
{
  const WRTTI* pType = GetSourceAccessor()->GetExposedParamsType(ref_pObject);
  if (pType == ref_pType)
  {
    W_ASSERT_DEBUG(pType && pType->FindPropertyByName(ref_pProp->GetPropertyName()) == ref_pProp, "");
    ref_indices.InsertAt(0, ref_pProp->GetPropertyName());
    ref_pType = ref_pObject->GetType();
    ref_pProp = GetSourceAccessor()->m_pParameterProp;
  }
  return m_pSource->ResolveProxy(ref_pObject, ref_pType, ref_pProp, ref_indices);
}

WStatus WExposedParametersAsTypeCommandAccessor::GetSubValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant& out_value)
{
  const WRTTI* pType = GetSourceAccessor()->GetExposedParamsType(pObject);
  W_ASSERT_DEBUG(pType && pType->FindPropertyByName(pProp->GetPropertyName()) == pProp, "");

  WStatus result = GetSourceAccessor()->GetValue(pObject, GetSourceAccessor()->m_pParameterProp, out_value, pProp->GetPropertyName());
  if (result.Failed())
    return result;

  PatchPropertyType(out_value, pProp);

  return result;
}

WStatus WExposedParametersAsTypeCommandAccessor::SetSubValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, const WDelegate<WStatus(WVariant&)>& func)
{
  WVariant currentValue;
  W_SUCCEED_OR_RETURN(GetSubValue(pObject, pProp, currentValue));
  W_SUCCEED_OR_RETURN(func(currentValue));
  return GetSourceAccessor()->SetValue(pObject, pProp, currentValue, pProp->GetPropertyName());
}

void WExposedParametersAsTypeCommandAccessor::PatchPropertyType(WVariant& ref_value, const WAbstractProperty* pProp)
{
  if (pProp->GetSpecificType() == WGetStaticRTTI<WVariant>() && pProp->GetCategory() == WPropertyCategory::Member)
    return;

  const WVariantType::Enum propType = WToolsReflectionUtils::GetStorageType(pProp);
  const WVariantType::Enum valueType = ref_value.GetType();
  if (propType != valueType)
  {
    if (ref_value.CanConvertTo(propType))
    {
      ref_value = ref_value.ConvertTo(propType);
    }
    else
    {
      ref_value = WToolsReflectionUtils::GetStorageDefault(pProp);
    }
  }

  if (pProp->GetSpecificType() == WGetStaticRTTI<WVariant>())
    return;

  switch (pProp->GetCategory())
  {
    case WPropertyCategory::Array:
      if (const WVariantType::Enum propElementType = pProp->GetSpecificType()->GetVariantType(); propElementType != WVariantType::Invalid)
      {
        const WVariantArray& array = ref_value.Get<WVariantArray>();
        for (WUInt32 i = 0; i < array.GetCount(); ++i)
        {
          const WVariant& element = array[i];
          const WVariantType::Enum valueElementType = element.GetType();
          if (propElementType != valueElementType)
          {
            WVariantArray& arrayWritable = ref_value.GetWritable<WVariantArray>();
            WVariant& elementWritable = arrayWritable[i];
            if (elementWritable.CanConvertTo(propElementType))
            {
              elementWritable = elementWritable.ConvertTo(propType);
            }
            else
            {
              elementWritable = WReflectionUtils::GetDefaultVariantFromType(propElementType);
            }
          }
        }
      }
      break;
    case WPropertyCategory::Map:
      if (const WVariantType::Enum propElementType = pProp->GetSpecificType()->GetVariantType(); propElementType != WVariantType::Invalid)
      {
        const WVariantDictionary& map = ref_value.Get<WVariantDictionary>();
        for (auto it : map)
        {
          const WVariant& element = it.Value();
          const WVariantType::Enum valueElementType = element.GetType();
          if (propElementType != valueElementType)
          {
            WVariantDictionary& mapWritable = ref_value.GetWritable<WVariantDictionary>();
            WVariant* pElementWritable = nullptr;
            if (mapWritable.TryGetValue(it.Key(), pElementWritable))
            {
              if (pElementWritable->CanConvertTo(propElementType))
              {
                *pElementWritable = pElementWritable->ConvertTo(propType);
              }
              else
              {
                *pElementWritable = WReflectionUtils::GetDefaultVariantFromType(propElementType);
              }
            }
          }
        }
      }
      break;

    default:
      break;
  }
}

//////////////////////////////////////////////////////////////////////////

WQtExposedParametersPropertyWidget::WQtExposedParametersPropertyWidget()
  : WQtPropertyStandardTypeContainerWidget()
{
  // Replace the container layout so we can prepend the type widget before all container elements
  delete m_pGroupLayout;
  m_pGroupLayout = new QVBoxLayout(nullptr);
  m_pGroupLayout->setSpacing(1);
  m_pGroupLayout->setContentsMargins(5, 0, 0, 0);

  m_pTypeViewLayout = new QVBoxLayout(nullptr);
  m_pTypeViewLayout->addLayout(m_pGroupLayout);
  m_pTypeViewLayout->setSpacing(0);
  m_pTypeViewLayout->setContentsMargins(0, 0, 0, 0);

  m_pGroup->GetContent()->setLayout(m_pTypeViewLayout);
}

WQtExposedParametersPropertyWidget::~WQtExposedParametersPropertyWidget()
{
  m_pGrid->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(WMakeDelegate(&WQtExposedParametersPropertyWidget::PropertyEventHandler, this));
  m_pGrid->GetCommandHistory()->m_Events.RemoveEventHandler(WMakeDelegate(&WQtExposedParametersPropertyWidget::CommandHistoryEventHandler, this));
  WPhantomRttiManager::s_Events.RemoveEventHandler(WMakeDelegate(&WQtExposedParametersPropertyWidget::PhantomTypeRegistryEventHandler, this));
}

void WQtExposedParametersPropertyWidget::SetSelection(const WArrayPtr<WPropertySelection>& items)
{
  const WRTTI* pCommonType = m_pProxy->GetCommonExposedParamsType(items);
  if (m_pTypeWidget && m_pTypeWidget->GetType() != pCommonType)
  {
    m_pTypeWidget->PrepareToDie();
    m_pTypeWidget->deleteLater();
    m_pTypeWidget = nullptr;
  }

  if (m_pTypeWidget == nullptr && pCommonType != nullptr)
  {
    m_pTypeWidget = new WQtTypeWidget(m_pGroup->GetContent(), m_pGrid, m_pTypeProxy.Borrow(), pCommonType, nullptr, nullptr);
    m_pTypeViewLayout->insertWidget(0, m_pTypeWidget);
  }

  if (m_pTypeWidget)
  {
    m_pTypeWidget->setVisible(!s_bRawMode);
    m_pTypeWidget->SetSelection(items);
  }


  WQtPropertyStandardTypeContainerWidget::SetSelection(items);
  UpdateActionState();
}

void WQtExposedParametersPropertyWidget::OnInit()
{
  m_pGrid->GetObjectManager()->m_PropertyEvents.AddEventHandler(WMakeDelegate(&WQtExposedParametersPropertyWidget::PropertyEventHandler, this));
  m_pGrid->GetCommandHistory()->m_Events.AddEventHandler(WMakeDelegate(&WQtExposedParametersPropertyWidget::CommandHistoryEventHandler, this));
  WPhantomRttiManager::s_Events.AddEventHandler(WMakeDelegate(&WQtExposedParametersPropertyWidget::PhantomTypeRegistryEventHandler, this));

  const auto* pAttrib = m_pProp->GetAttributeByType<WExposedParametersAttribute>();
  W_ASSERT_DEV(pAttrib, "WQtExposedParametersPropertyWidget was created for a property that does not have the WExposedParametersAttribute.");
  m_sExposedParamProperty = pAttrib->GetParametersSource();
  const WAbstractProperty* pParameterSourceProp = m_pType->FindPropertyByName(m_sExposedParamProperty);
  W_ASSERT_DEV(
    pParameterSourceProp, "The exposed parameter source '{0}' does not exist on type '{1}'", m_sExposedParamProperty, m_pType->GetTypeName());
  m_pSourceObjectAccessor = m_pObjectAccessor;
  m_pProxy = W_DEFAULT_NEW(WExposedParameterCommandAccessor, m_pSourceObjectAccessor, m_pProp, pParameterSourceProp);
  m_pTypeProxy = W_DEFAULT_NEW(WExposedParametersAsTypeCommandAccessor, m_pProxy.Borrow());
  // Overwriting this will display the exposed parameter map as before, i.e. each property will be shown in the map even if not present. As this is now obsolete given the phantom type widget, this is probably no longer needed?
  // m_pObjectAccessor = m_pProxy.Borrow();

  WQtPropertyStandardTypeContainerWidget::OnInit();

  auto layout = qobject_cast<QHBoxLayout*>(m_pGroup->GetHeader()->layout());

  layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Minimum));

  layout->setStretch(0, 1);

  {
    // Help button to indicate exposed parameter mismatches.
    m_pFixMeButton = new QToolButton();
    m_pFixMeButton->setAutoRaise(true);
    m_pFixMeButton->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
    m_pFixMeButton->setIcon(WQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/Attention.svg"));
    auto sp = m_pFixMeButton->sizePolicy();
    sp.setVerticalPolicy(QSizePolicy::Ignored);
    m_pFixMeButton->setSizePolicy(sp);
    QMenu* pFixMeMenu = new QMenu(m_pFixMeButton);
    {
      m_pRemoveUnusedAction = pFixMeMenu->addAction(QStringLiteral("Remove unused keys"));
      m_pRemoveUnusedAction->setToolTip(
        QStringLiteral("The map contains keys that are no longer used by the asset's exposed parameters and thus can be removed."));
      connect(m_pRemoveUnusedAction, &QAction::triggered, this, [this](bool bChecked)
        { RemoveUnusedKeys(false); });
    }
    {
      m_pFixTypesAction = pFixMeMenu->addAction(QStringLiteral("Fix keys with wrong types"));
      connect(m_pFixTypesAction, &QAction::triggered, this, [this](bool bChecked)
        { FixKeyTypes(false); });
    }
    m_pFixMeButton->setMenu(pFixMeMenu);

    auto layout = qobject_cast<QHBoxLayout*>(m_pGroup->GetHeader()->layout());
    layout->insertWidget(layout->count() - 1, m_pFixMeButton);
  }

  {
    // Button to toggle between type-based view and the raw dictionary-based view of the exposed parameters.
    m_pToggleRawModeButton = new QToolButton();
    m_pToggleRawModeButton->setAutoRaise(true);
    m_pToggleRawModeButton->setCheckable(true);
    m_pToggleRawModeButton->setChecked(s_bRawMode);
    m_pToggleRawModeButton->setIcon(WQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/ExposedParameterViewToggle.svg"));
    auto sp = m_pToggleRawModeButton->sizePolicy();
    sp.setVerticalPolicy(QSizePolicy::Ignored);
    m_pToggleRawModeButton->setSizePolicy(sp);
    m_pToggleRawModeButton->setToolTip("Toggle between Type or RAW dictionary view");

    connect(m_pToggleRawModeButton, &QToolButton::toggled, this, [this](bool checked)
      {
        s_bRawMode = checked;
        WQtScopedUpdatesDisabled _(this);
        SetSelection(m_Items); });

    auto layout = qobject_cast<QHBoxLayout*>(m_pGroup->GetHeader()->layout());
    layout->insertWidget(layout->count() - 1, m_pToggleRawModeButton);
  }
}

void WQtExposedParametersPropertyWidget::UpdateElement(WUInt32 index)
{
  WQtPropertyStandardTypeContainerWidget::UpdateElement(index);
}

void WQtExposedParametersPropertyWidget::UpdatePropertyMetaState()
{
  WQtPropertyStandardTypeContainerWidget::UpdatePropertyMetaState();
}

void WQtExposedParametersPropertyWidget::GetRequiredElements(WDynamicArray<WVariant>& out_keys) const
{
  if (!s_bRawMode)
  {
    out_keys.Clear();
  }
  else
  {
    WQtPropertyContainerWidget::GetRequiredElements(out_keys);
  }
}

void WQtExposedParametersPropertyWidget::DoPrepareToDie()
{
  WQtPropertyStandardTypeContainerWidget::DoPrepareToDie();
  if (m_pTypeWidget)
  {
    m_pTypeWidget->PrepareToDie();
  }
}

void WQtExposedParametersPropertyWidget::PropertyEventHandler(const WDocumentObjectPropertyEvent& e)
{
  if (IsUndead())
    return;

  if (std::none_of(cbegin(m_Items), cend(m_Items), [=](const WPropertySelection& sel)
        { return e.m_pObject == sel.m_pObject; }))
    return;

  if (!m_bNeedsUpdate && m_sExposedParamProperty == e.m_sProperty)
  {
    FlushOrQueueChanges(true, false);
  }
  if (!m_bNeedsMetaDataUpdate && m_pProp->GetPropertyName() == e.m_sProperty)
  {
    FlushOrQueueChanges(false, true);
  }
}

void WQtExposedParametersPropertyWidget::CommandHistoryEventHandler(const WCommandHistoryEvent& e)
{
  if (IsUndead())
    return;

  switch (e.m_Type)
  {
    case WCommandHistoryEvent::Type::UndoEnded:
    case WCommandHistoryEvent::Type::RedoEnded:
    case WCommandHistoryEvent::Type::TransactionEnded:
    case WCommandHistoryEvent::Type::TransactionCanceled:
    {
      FlushOrQueueChanges(false, false);
    }
    break;

    default:
      break;
  }
}

void WQtExposedParametersPropertyWidget::PhantomTypeRegistryEventHandler(const WPhantomRttiManagerEvent& e)
{
  if (const WRTTI* pCommonType = m_pProxy->GetCommonExposedParamsType(m_Items))
  {
    if (e.m_pChangedType->IsDerivedFrom(pCommonType))
    {
      if (m_pTypeWidget)
      {
        // The type widget stores pointer to properties which have been destroyed by the phantom type update so we need to destroy it and recreate it.
        m_pTypeWidget->PrepareToDie();
        m_pTypeWidget->deleteLater();
        m_pTypeWidget = nullptr;
      }
      FlushOrQueueChanges(true, true);
    }
  }
}

void WQtExposedParametersPropertyWidget::FlushOrQueueChanges(bool bNeedsUpdate, bool bNeedsMetaDataUpdate)
{
  m_bNeedsUpdate |= bNeedsUpdate;
  m_bNeedsMetaDataUpdate |= bNeedsMetaDataUpdate;
  // Wait until the transaction is done and this function will be called again inside CommandHistoryEventHandler.
  if (m_pGrid->GetCommandHistory()->IsInTransaction() || m_pGrid->GetCommandHistory()->IsInUndoRedo())
    return;

  if (m_bNeedsUpdate)
  {
    m_bNeedsUpdate = false;
    SetSelection(m_Items);
  }
  if (m_bNeedsMetaDataUpdate)
  {
    // m_bNeedsMetaDataUpdate is reset inside UpdateActionState as it can be called from other places.
    UpdateActionState();
  }
}

bool WQtExposedParametersPropertyWidget::RemoveUnusedKeys(bool bTestOnly)
{
  bool bStuffDone = false;
  if (!bTestOnly)
    m_pSourceObjectAccessor->StartTransaction("Remove unused keys");
  for (const auto& item : m_Items)
  {
    if (const WExposedParameters* pParams = m_pProxy->GetExposedParams(item.m_pObject))
    {
      WTempHybridArray<WVariant, 16> keys;
      W_VERIFY(m_pSourceObjectAccessor->GetKeys(item.m_pObject, m_pProp, keys).Succeeded(), "");
      for (auto& key : keys)
      {
        if (!pParams->Find(key.Get<WString>()))
        {
          if (!bTestOnly)
          {
            bStuffDone = true;
            m_pSourceObjectAccessor->RemoveValue(item.m_pObject, m_pProp, key).LogFailure();
          }
          else
          {
            return true;
          }
        }
      }
    }
  }
  if (!bTestOnly)
    m_pSourceObjectAccessor->FinishTransaction();
  return bStuffDone;
}

bool WQtExposedParametersPropertyWidget::FixKeyTypes(bool bTestOnly)
{
  bool bStuffDone = false;
  if (!bTestOnly)
    m_pSourceObjectAccessor->StartTransaction("Remove unused keys");
  for (const auto& item : m_Items)
  {
    if (const WExposedParameters* pParams = m_pProxy->GetExposedParams(item.m_pObject))
    {
      WTempHybridArray<WVariant, 16> keys;
      W_VERIFY(m_pSourceObjectAccessor->GetKeys(item.m_pObject, m_pProp, keys).Succeeded(), "");
      for (auto& key : keys)
      {
        if (const auto* pParam = pParams->Find(key.Get<WString>()))
        {
          WVariant value;
          const WRTTI* pType = pParam->m_DefaultValue.GetReflectedType();
          W_VERIFY(m_pSourceObjectAccessor->GetValue(item.m_pObject, m_pProp, value, key).Succeeded(), "");
          if (value.GetReflectedType() != pType)
          {
            if (!bTestOnly)
            {
              bStuffDone = true;
              WVariantType::Enum type = pParam->m_DefaultValue.GetType();
              if (value.CanConvertTo(type))
              {
                m_pProxy->SetValue(item.m_pObject, m_pProp, value.ConvertTo(type), key).LogFailure();
              }
              else
              {
                m_pProxy->SetValue(item.m_pObject, m_pProp, pParam->m_DefaultValue, key).LogFailure();
              }
            }
            else
            {
              return true;
            }
          }
        }
      }
    }
  }
  if (!bTestOnly)
    m_pSourceObjectAccessor->FinishTransaction();
  return bStuffDone;
}

void WQtExposedParametersPropertyWidget::UpdateActionState()
{
  m_bNeedsMetaDataUpdate = false;
  m_pRemoveUnusedAction->setEnabled(RemoveUnusedKeys(true));
  m_pFixTypesAction->setEnabled(FixKeyTypes(true));
  m_pFixMeButton->setVisible(m_pRemoveUnusedAction->isEnabled() || m_pFixTypesAction->isEnabled());
}
