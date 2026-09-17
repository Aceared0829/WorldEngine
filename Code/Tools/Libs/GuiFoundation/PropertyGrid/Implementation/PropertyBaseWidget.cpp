#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <GuiFoundation/PropertyGrid/DefaultState.h>
#include <GuiFoundation/PropertyGrid/Implementation/AddSubElementButton.moc.h>
#include <GuiFoundation/PropertyGrid/Implementation/ElementGroupButton.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/CollapsibleGroupBox.moc.h>
#include <GuiFoundation/Widgets/InlinedGroupBox.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

#include <GuiFoundation/GuiFoundationDLL.h>
#include <QClipboard>
#include <QDragEnterEvent>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QPainter>
#include <QScrollArea>
#include <QStringBuilder>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WPropertyClipboard, WNoBase, 1, WRTTIDefaultAllocator<WPropertyClipboard>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("m_Type", m_Type),
    W_MEMBER_PROPERTY("m_Value", m_Value),
    W_MEMBER_PROPERTY("m_ObjectGraph", m_ObjectGraph),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

/// *** BASE ***
WQtPropertyWidget::WQtPropertyWidget()
  : QWidget(nullptr)

{
  m_bUndead = false;
  m_bIsDefault = true;
  setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
}

WQtPropertyWidget::~WQtPropertyWidget() = default;

void WQtPropertyWidget::Init(WQtPropertyGridWidget* pGrid, WObjectAccessorBase* pObjectAccessor, const WRTTI* pType, const WAbstractProperty* pProp)
{
  m_pGrid = pGrid;
  m_pObjectAccessor = pObjectAccessor;
  m_pType = pType;
  m_pProp = pProp;
  W_ASSERT_DEBUG(m_pGrid && m_pObjectAccessor && m_pType && m_pProp, "");

  if (pProp->GetAttributeByType<WReadOnlyAttribute>() != nullptr || pProp->GetFlags().IsSet(WPropertyFlags::ReadOnly))
  {
    SetReadOnly();
  }

  OnInit();
}

void WQtPropertyWidget::SetSelection(const WArrayPtr<WPropertySelection>& items)
{
  m_Items = items;
}

const char* WQtPropertyWidget::GetLabel(WStringBuilder& ref_sTmp) const
{
  ref_sTmp.Set(m_pType->GetTypeName(), "::", m_pProp->GetPropertyName());
  return ref_sTmp;
}

void WQtPropertyWidget::ExtendContextMenu(QMenu& m)
{
  m.setToolTipsVisible(true);
  // revert
  {
    QAction* pRevert = m.addAction("Revert to Default");
    pRevert->setEnabled(!m_bIsDefault);
    connect(pRevert, &QAction::triggered, this, [this]()
      {
      m_pObjectAccessor->StartTransaction("Revert to Default");

      switch (m_pProp->GetCategory())
      {
        case WPropertyCategory::Enum::Array:
        case WPropertyCategory::Enum::Set:
        case WPropertyCategory::Enum::Map:
        {

          WStatus res = WStatus(W_SUCCESS);
          if (!m_Items[0].m_Index.IsValid())
          {
            // Revert container
            WDefaultContainerState defaultState(m_pType, m_pObjectAccessor, m_Items, m_pProp->GetPropertyName());
            res = defaultState.RevertContainer();
          }
          else
          {
            const bool bIsValueType = WReflectionUtils::IsValueType(m_pProp) || m_pProp->GetFlags().IsAnySet(WPropertyFlags::IsEnum | WPropertyFlags::Bitflags);
            if (bIsValueType)
            {
              // Revert container value type element
              WDefaultContainerState defaultState(m_pType, m_pObjectAccessor, m_Items, m_pProp->GetPropertyName());
              res = defaultState.RevertElement({});
            }
            else
            {
              // Revert objects pointed to by the object type element
              WTempHybridArray<WPropertySelection, 8> ResolvedObjects;
              for (const auto& item : m_Items)
              {
                WUuid ObjectGuid = m_pObjectAccessor->Get<WUuid>(item.m_pObject, m_pProp, item.m_Index);
                if (ObjectGuid.IsValid())
                {
                  ResolvedObjects.PushBack({m_pObjectAccessor->GetObject(ObjectGuid), WVariant()});
                }
              }
              WDefaultObjectState defaultState(m_pType, m_pObjectAccessor, ResolvedObjects);
              res = defaultState.RevertObject();
            }
          }
          if (res.Failed())
          {
            res.LogFailure();
            m_pObjectAccessor->CancelTransaction();
            return;
          }
        }
        break;
        default:
        {
          // Revert object member property
          WDefaultObjectState defaultState(m_pType, m_pObjectAccessor, m_Items);
          WStatus res = defaultState.RevertProperty(m_pProp);
          if (res.Failed())
          {
            res.LogFailure();
            m_pObjectAccessor->CancelTransaction();
            return;
          }
        }
        break;
      }
      m_pObjectAccessor->FinishTransaction(); });
  }

  const char* szMimeType = "application/WEditor.Property";
  const char* szObjectMimeType = "application/WEditor.PropertyObject";
  const bool bValueType = WReflectionUtils::IsValueType(m_pProp) || m_pProp->GetFlags().IsAnySet(WPropertyFlags::Bitflags | WPropertyFlags::IsEnum);

  // Pasting onto a single element of a container (array/set/map) is a scalar SetValue,
  // not a whole-container replacement.
  const bool bIsContainerProp = m_pProp->GetCategory() == WPropertyCategory::Array || m_pProp->GetCategory() == WPropertyCategory::Set || m_pProp->GetCategory() == WPropertyCategory::Map;
  const bool bIsSingleElement = bIsContainerProp && !m_Items.IsEmpty() && m_Items[0].m_Index.IsValid();

  // Try to resolve every selection item to an underlying WDocumentObject. This is the
  // case for component group widgets (m_pProp is the Components array, m_Index is valid)
  // and for embedded class / pointer member properties.
  WHybridArray<const WDocumentObject*, 8> resolvedTargets;
  bool bAllResolved = !bValueType && !m_Items.IsEmpty();
  if (bAllResolved)
  {
    for (const WPropertySelection& sel : m_Items)
    {
      WVariant v;
      if (m_pObjectAccessor->GetValue(sel.m_pObject, m_pProp, v, sel.m_Index).Failed() || !v.IsA<WUuid>())
      {
        bAllResolved = false;
        break;
      }
      const WDocumentObject* pTarget = m_pObjectAccessor->GetObject(v.Get<WUuid>());
      if (pTarget == nullptr)
      {
        bAllResolved = false;
        break;
      }
      resolvedTargets.PushBack(pTarget);
    }
  }
  const bool bObjectType = bAllResolved && !resolvedTargets.IsEmpty();

  // Copy
  {
    WVariant commonValue = bValueType ? GetCommonValue(m_Items, m_pProp) : WVariant();
    QAction* pCopy = m.addAction("Copy Value");

    if (bValueType)
    {
      if (!commonValue.IsValid())
      {
        pCopy->setEnabled(false);
        pCopy->setToolTip("No common value in selection");
      }

      connect(pCopy, &QAction::triggered, this, [this, szMimeType, commonValue]()
        {
        WPropertyClipboard content;
        content.m_Type = m_pProp->GetSpecificType()->GetTypeName();
        content.m_Value = commonValue;

        // Serialize
        WContiguousMemoryStreamStorage streamStorage;
        WMemoryStreamWriter memoryWriter(&streamStorage);
        WReflectionSerializer::WriteObjectToDDL(memoryWriter, WGetStaticRTTI<WPropertyClipboard>(), &content);
        memoryWriter.WriteBytes("\0", 1).IgnoreResult(); // null terminate

        // Write to clipboard
        QClipboard* clipboard = QApplication::clipboard();
        QMimeData* mimeData = new QMimeData();
        QByteArray encodedData((const char*)streamStorage.GetData(), streamStorage.GetStorageSize32());

        mimeData->setData(szMimeType, encodedData);
        mimeData->setText(QString::fromUtf8((const char*)streamStorage.GetData()));
        clipboard->setMimeData(mimeData); });
    }
    else if (bObjectType)
    {
      // Multi-select object copy is ambiguous — only enable for a single source.
      if (resolvedTargets.GetCount() != 1)
      {
        pCopy->setEnabled(false);
        pCopy->setToolTip("Copy of object values requires a single selection");
      }

      const WDocumentObject* pSource = resolvedTargets[0];
      connect(pCopy, &QAction::triggered, this, [this, szObjectMimeType, pSource]()
        {
        // Serialize the object subgraph to DDL.
        WAbstractObjectGraph graph;
        WDocumentObjectConverterWriter writer(&graph, pSource->GetDocumentObjectManager());
        writer.AddObjectToGraph(pSource, "root");

        WContiguousMemoryStreamStorage graphStorage;
        WMemoryStreamWriter graphWriter(&graphStorage);
        WAbstractGraphDdlSerializer::Write(graphWriter, &graph);
        graphWriter.WriteBytes("\0", 1).IgnoreResult();

        WPropertyClipboard content;
        content.m_Type = pSource->GetType()->GetTypeName();
        content.m_ObjectGraph = (const char*)graphStorage.GetData();

        // Serialize the clipboard wrapper itself.
        WContiguousMemoryStreamStorage streamStorage;
        WMemoryStreamWriter memoryWriter(&streamStorage);
        WReflectionSerializer::WriteObjectToDDL(memoryWriter, WGetStaticRTTI<WPropertyClipboard>(), &content);
        memoryWriter.WriteBytes("\0", 1).IgnoreResult();

        QClipboard* clipboard = QApplication::clipboard();
        QMimeData* mimeData = new QMimeData();
        QByteArray encodedData((const char*)streamStorage.GetData(), streamStorage.GetStorageSize32());
        mimeData->setData(szObjectMimeType, encodedData);
        mimeData->setText(QString::fromUtf8((const char*)streamStorage.GetData()));
        clipboard->setMimeData(mimeData); });
    }
    else
    {
      pCopy->setEnabled(false);
      pCopy->setToolTip("Not a value type");
    }
  }

  // Paste
  {
    QAction* pPaste = m.addAction("Paste Value");

    QClipboard* clipboard = QApplication::clipboard();
    auto mimedata = clipboard->mimeData();

    if (!isEnabled())
    {
      pPaste->setEnabled(false);
      pPaste->setToolTip("Property is read only");
    }
    else if (bValueType && mimedata->hasFormat(szMimeType))
    {
      QByteArray ba = mimedata->data(szMimeType);
      WRawMemoryStreamReader memoryReader(ba.data(), ba.size());

      WPropertyClipboard content;
      WReflectionSerializer::ReadObjectPropertiesFromDDL(memoryReader, *WGetStaticRTTI<WPropertyClipboard>(), &content);

      // Pasting onto a single container element is a scalar SetValue, not a whole-container replacement.
      const bool bIsArrayPaste = (m_pProp->GetCategory() == WPropertyCategory::Array || m_pProp->GetCategory() == WPropertyCategory::Set) && !bIsSingleElement;
      const bool bIsMapPaste = m_pProp->GetCategory() == WPropertyCategory::Map && !bIsSingleElement;
      const WRTTI* pClipboardType = WRTTI::FindTypeByName(content.m_Type);
      const bool bIsEnumeration = pClipboardType && (pClipboardType->IsDerivedFrom<WEnumBase>() || pClipboardType->IsDerivedFrom<WBitflagsBase>() || m_pProp->GetSpecificType()->IsDerivedFrom<WEnumBase>() || m_pProp->GetSpecificType()->IsDerivedFrom<WBitflagsBase>());
      const bool bEnumerationMissmatch = bIsEnumeration ? pClipboardType != m_pProp->GetSpecificType() : false;
      const WResult clamped = WReflectionUtils::ClampValue(content.m_Value, m_pProp->GetAttributeByType<WClampValueAttribute>());

      if (content.m_Value.IsA<WVariantArray>() != bIsArrayPaste || content.m_Value.IsA<WVariantDictionary>() != bIsMapPaste)
      {
        pPaste->setEnabled(false);
        WStringBuilder sTemp;
        sTemp.SetFormat("Cannot convert clipboard and property content between containers and members.");
        pPaste->setToolTip(sTemp.GetData());
      }
      else if (bIsMapPaste)
      {
        // No further checks; per-entry conversion is attempted on apply.
      }
      else if (bEnumerationMissmatch || (!content.m_Value.CanConvertTo(m_pProp->GetSpecificType()->GetVariantType()) && content.m_Type != m_pProp->GetSpecificType()->GetTypeName()))
      {
        pPaste->setEnabled(false);
        WStringBuilder sTemp;
        sTemp.SetFormat("Cannot convert clipboard of type '{}' to property of type '{}'", content.m_Type, m_pProp->GetSpecificType()->GetTypeName());
        pPaste->setToolTip(sTemp.GetData());
      }
      else if (clamped.Failed())
      {
        pPaste->setEnabled(false);
        WStringBuilder sTemp;
        sTemp.SetFormat("The member property '{}' has an WClampValueAttribute but WReflectionUtils::ClampValue failed.", m_pProp->GetPropertyName());
      }

      connect(pPaste, &QAction::triggered, this, [this, content, bIsArrayPaste, bIsMapPaste]()
        {
        m_pObjectAccessor->StartTransaction("Paste Value");
        if (bIsArrayPaste)
        {
          const WVariantArray& values = content.m_Value.Get<WVariantArray>();
          for (const WPropertySelection& sel : m_Items)
          {
            if (m_pObjectAccessor->ClearByName(sel.m_pObject, m_pProp->GetPropertyName()).Failed())
            {
              m_pObjectAccessor->CancelTransaction();
              return;
            }
            for (const WVariant& val : values)
            {
              if (m_pObjectAccessor->InsertValue(sel.m_pObject, m_pProp, val, -1).Failed())
              {
                m_pObjectAccessor->CancelTransaction();
                return;
              }
            }
          }
        }
        else if (bIsMapPaste)
        {
          const WVariantDictionary& values = content.m_Value.Get<WVariantDictionary>();
          for (const WPropertySelection& sel : m_Items)
          {
            if (m_pObjectAccessor->ClearByName(sel.m_pObject, m_pProp->GetPropertyName()).Failed())
            {
              m_pObjectAccessor->CancelTransaction();
              return;
            }
            for (auto it = values.GetIterator(); it.IsValid(); ++it)
            {
              if (m_pObjectAccessor->InsertValue(sel.m_pObject, m_pProp, it.Value(), WVariant(it.Key())).Failed())
              {
                m_pObjectAccessor->CancelTransaction();
                return;
              }
            }
          }
        }
        else
        {
          for (const WPropertySelection& sel : m_Items)
          {
            if (m_pObjectAccessor->SetValue(sel.m_pObject, m_pProp, content.m_Value, sel.m_Index).Failed())
            {
              m_pObjectAccessor->CancelTransaction();
              return;
            }
          }
        }

        m_pObjectAccessor->FinishTransaction(); });
    }
    else if (bObjectType && mimedata->hasFormat(szObjectMimeType))
    {
      QByteArray ba = mimedata->data(szObjectMimeType);
      WRawMemoryStreamReader memoryReader(ba.data(), ba.size());

      WPropertyClipboard content;
      WReflectionSerializer::ReadObjectPropertiesFromDDL(memoryReader, *WGetStaticRTTI<WPropertyClipboard>(), &content);

      if (content.m_ObjectGraph.IsEmpty())
      {
        pPaste->setEnabled(false);
        pPaste->setToolTip("Clipboard does not contain object data");
      }
      else
      {
        // Capture targets and content for the lambda.
        WDynamicArray<const WDocumentObject*> targets;
        targets = resolvedTargets;

        connect(pPaste, &QAction::triggered, this, [this, content, targets]()
          {
          // Deserialize the clipboard graph.
          WRawMemoryStreamReader graphReader(content.m_ObjectGraph.GetData(), content.m_ObjectGraph.GetElementCount());
          WAbstractObjectGraph graph;
          if (WAbstractGraphDdlSerializer::Read(graphReader, &graph).Failed())
            return;

          const WAbstractObjectNode* pNode = graph.GetNodeByName("root");
          if (pNode == nullptr)
            return;

          m_pObjectAccessor->StartTransaction("Paste Component Values");

          for (const WDocumentObject* pTarget : targets)
          {
            const WRTTI* pTargetType = pTarget->GetType();
            for (const auto& nodeProp : pNode->GetProperties())
            {
              const WAbstractProperty* pTargetProp = pTargetType->FindPropertyByName(nodeProp.m_sPropertyName);
              if (pTargetProp == nullptr)
                continue;
              if (pTargetProp->GetFlags().IsSet(WPropertyFlags::ReadOnly))
                continue;
              if (pTargetProp->GetAttributeByType<WReadOnlyAttribute>() != nullptr)
                continue;
              if (pTargetProp->GetAttributeByType<WHiddenAttribute>() != nullptr)
                continue;
              // Skip nested object references; only by-value properties are pasted in v1.
              const bool bTargetValue = WReflectionUtils::IsValueType(pTargetProp) || pTargetProp->GetFlags().IsAnySet(WPropertyFlags::Bitflags | WPropertyFlags::IsEnum);
              if (!bTargetValue)
                continue;

              const WPropertyCategory::Enum cat = pTargetProp->GetCategory();
              if (cat == WPropertyCategory::Member)
              {
                if (!nodeProp.m_Value.IsValid() || nodeProp.m_Value.IsA<WVariantArray>() || nodeProp.m_Value.IsA<WVariantDictionary>())
                  continue;
                WVariant val = nodeProp.m_Value;
                // Enum/bitflags values are serialized as strings in the object graph; convert back to integer before assignment.
                if (pTargetProp->GetFlags().IsAnySet(WPropertyFlags::IsEnum | WPropertyFlags::Bitflags) && val.IsA<WString>())
                {
                  WInt64 iEnumValue = 0;
                  if (!WReflectionUtils::StringToEnumeration(pTargetProp->GetSpecificType(), val.Get<WString>(), iEnumValue))
                    continue;
                  val = iEnumValue;
                }
                else
                {
                  const WVariantType::Enum ttype = pTargetProp->GetSpecificType()->GetVariantType();
                  if (!val.CanConvertTo(ttype) && pTargetProp->GetSpecificType()->GetTypeName() != WRTTI::FindTypeByName(content.m_Type)->GetTypeName())
                    continue;
                }

                WReflectionUtils::ClampValue(val, pTargetProp->GetAttributeByType<WClampValueAttribute>()).IgnoreResult();
                m_pObjectAccessor->SetValue(pTarget, pTargetProp, val).LogFailure();
              }
              else if (cat == WPropertyCategory::Array || cat == WPropertyCategory::Set)
              {
                if (!nodeProp.m_Value.IsA<WVariantArray>())
                  continue;
                if (m_pObjectAccessor->ClearByName(pTarget, pTargetProp->GetPropertyName()).Failed())
                  continue;
                const WVariantArray& values = nodeProp.m_Value.Get<WVariantArray>();
                for (const WVariant& val : values)
                {
                  m_pObjectAccessor->InsertValue(pTarget, pTargetProp, val, -1).LogFailure();
                }
              }
              else if (cat == WPropertyCategory::Map)
              {
                if (!nodeProp.m_Value.IsA<WVariantDictionary>())
                  continue;
                if (m_pObjectAccessor->ClearByName(pTarget, pTargetProp->GetPropertyName()).Failed())
                  continue;
                const WVariantDictionary& values = nodeProp.m_Value.Get<WVariantDictionary>();
                for (auto it = values.GetIterator(); it.IsValid(); ++it)
                {
                  m_pObjectAccessor->InsertValue(pTarget, pTargetProp, it.Value(), it.Key()).LogFailure();
                }
              }
            }
          }

          m_pObjectAccessor->FinishTransaction(); });
      }
    }
    else if (!bValueType && !bObjectType)
    {
      pPaste->setEnabled(false);
      pPaste->setToolTip("Not a value type");
    }
    else
    {
      pPaste->setEnabled(false);
      pPaste->setToolTip("No matching property in clipboard");
    }
  }

  // copy internal name
  {
    auto lambda = [this]()
    {
      QClipboard* clipboard = QApplication::clipboard();
      QMimeData* mimeData = new QMimeData();
      mimeData->setText(m_pProp->GetPropertyName());
      clipboard->setMimeData(mimeData);

      WQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(
        WFmt("Copied Property Name: {}", m_pProp->GetPropertyName()), WTime::MakeFromSeconds(5));
    };

    QAction* pAction = m.addAction("Copy Internal Property Name:");
    connect(pAction, &QAction::triggered, this, lambda);

    QAction* pAction2 = m.addAction(m_pProp->GetPropertyName());
    connect(pAction2, &QAction::triggered, this, lambda);
  }
}

const WRTTI* WQtPropertyWidget::GetCommonBaseType(const WArrayPtr<WPropertySelection>& items)
{
  const WRTTI* pSubtype = nullptr;

  for (const auto& item : items)
  {
    const auto& accessor = item.m_pObject->GetTypeAccessor();

    if (pSubtype == nullptr)
      pSubtype = accessor.GetType();
    else
    {
      pSubtype = WReflectionUtils::GetCommonBaseType(pSubtype, accessor.GetType());
    }
  }

  return pSubtype;
}

QColor WQtPropertyWidget::SetPaletteBackgroundColor(WColorGammaUB inputColor, QPalette& ref_palette)
{
  QColor qColor = qApp->palette().color(QPalette::Window);
  if (inputColor.a != 0)
  {
    const WColor paletteColorLinear = qtToEzColor(qColor);
    const WColor inputColorLinear = inputColor;

    WColor blendedColor = WMath::Lerp(paletteColorLinear, inputColorLinear, inputColorLinear.a);
    blendedColor.a = 1.0f;
    qColor = WToQtColor(blendedColor);
  }

  ref_palette.setBrush(QPalette::Window, QBrush(qColor, Qt::SolidPattern));
  return qColor;
}

bool WQtPropertyWidget::GetCommonVariantSubType(const WArrayPtr<WPropertySelection>& items, const WAbstractProperty* pProperty, WVariantType::Enum& out_type)
{
  bool bFirst = true;
  // check if we have multiple values
  for (const auto& item : items)
  {
    if (bFirst)
    {
      bFirst = false;
      WVariant value;
      m_pObjectAccessor->GetValue(item.m_pObject, pProperty, value, item.m_Index).AssertSuccess();
      out_type = value.GetType();
    }
    else
    {
      WVariant valueNext;
      m_pObjectAccessor->GetValue(item.m_pObject, pProperty, valueNext, item.m_Index).AssertSuccess();
      if (valueNext.GetType() != out_type)
      {
        out_type = WVariantType::Invalid;
        return false;
      }
    }
  }
  return true;
}

WVariant WQtPropertyWidget::GetCommonValue(const WArrayPtr<WPropertySelection>& items, const WAbstractProperty* pProperty)
{
  if (!items[0].m_Index.IsValid() && (m_pProp->GetCategory() == WPropertyCategory::Array || m_pProp->GetCategory() == WPropertyCategory::Set))
  {
    WVariantArray values;
    // check if we have multiple values
    for (WUInt32 i = 0; i < items.GetCount(); i++)
    {
      const auto& item = items[i];
      if (i == 0)
      {
        m_pObjectAccessor->GetValues(item.m_pObject, pProperty, values).AssertSuccess();
      }
      else
      {
        WVariantArray valuesNext;
        m_pObjectAccessor->GetValues(item.m_pObject, pProperty, valuesNext).AssertSuccess();
        if (values != valuesNext)
        {
          return WVariant();
        }
      }
    }
    return values;
  }
  else if (!items[0].m_Index.IsValid() && m_pProp->GetCategory() == WPropertyCategory::Map)
  {
    WVariantDictionary first;
    for (WUInt32 i = 0; i < items.GetCount(); i++)
    {
      const auto& item = items[i];
      WDynamicArray<WVariant> keys;
      if (m_pObjectAccessor->GetKeys(item.m_pObject, pProperty, keys).Failed())
        return WVariant();

      WVariantDictionary current;
      for (const WVariant& key : keys)
      {
        WVariant val;
        if (m_pObjectAccessor->GetValue(item.m_pObject, pProperty, val, key).Failed())
          return WVariant();
        current.Insert(key.ConvertTo<WString>(), val);
      }

      if (i == 0)
      {
        first = std::move(current);
      }
      else
      {
        if (first.GetCount() != current.GetCount())
          return WVariant();
        for (auto it = first.GetIterator(); it.IsValid(); ++it)
        {
          WVariant* pOther = nullptr;
          if (!current.TryGetValue(it.Key(), pOther) || *pOther != it.Value())
            return WVariant();
        }
      }
    }
    return first;
  }
  else
  {
    WVariant value;
    // check if we have multiple values
    for (const auto& item : items)
    {
      if (!value.IsValid())
      {
        m_pObjectAccessor->GetValue(item.m_pObject, pProperty, value, item.m_Index).IgnoreResult();
      }
      else
      {
        WVariant valueNext;
        m_pObjectAccessor->GetValue(item.m_pObject, pProperty, valueNext, item.m_Index).AssertSuccess();
        if (value != valueNext)
        {
          value = WVariant();
          break;
        }
      }
    }
    return value;
  }
}

void WQtPropertyWidget::PrepareToDie()
{
  W_ASSERT_DEBUG(!m_bUndead, "Object has already been marked for cleanup");

  m_bUndead = true;

  DoPrepareToDie();
}

void WQtPropertyWidget::SetReadOnly(bool bReadOnly /*= true*/)
{
  setDisabled(bReadOnly);
}

void WQtPropertyWidget::OnCustomContextMenu(const QPoint& pt)
{
  QMenu m;
  m.setToolTipsVisible(true);

  ExtendContextMenu(m);
  m_pGrid->ExtendContextMenu(m, this);

  m.exec(pt); // pt is already in global space, because we fixed that
}

void WQtPropertyWidget::Broadcast(WPropertyEvent::Type type)
{
  WPropertyEvent ed;
  ed.m_Type = type;
  ed.m_pProperty = m_pProp;
  PropertyChangedHandler(ed);
}

void WQtPropertyWidget::PropertyChangedHandler(const WPropertyEvent& ed)
{
  if (m_bUndead)
    return;


  switch (ed.m_Type)
  {
    case WPropertyEvent::Type::SingleValueChanged:
    {
      WStringBuilder sTemp;
      sTemp.SetFormat("Change Property '{0}'", WTranslate(ed.m_pProperty->GetPropertyName()));
      m_pObjectAccessor->StartTransaction(sTemp);

      WStatus res(W_SUCCESS);

      for (const auto& sel : *ed.m_pItems)
      {
        res = m_pObjectAccessor->SetValue(sel.m_pObject, ed.m_pProperty, ed.m_Value, sel.m_Index);
        if (res.Failed())
          break;
      }

      if (res.Failed())
        m_pObjectAccessor->CancelTransaction();
      else
        m_pObjectAccessor->FinishTransaction();

      WQtUiServices::GetSingleton()->MessageBoxStatus(res, "Changing the property failed.");
    }
    break;

    case WPropertyEvent::Type::BeginTemporary:
    {
      WStringBuilder sTemp;
      sTemp.SetFormat("Change Property '{0}'", WTranslate(ed.m_pProperty->GetPropertyName()));
      m_pObjectAccessor->BeginTemporaryCommands(sTemp);
    }
    break;

    case WPropertyEvent::Type::EndTemporary:
    {
      m_pObjectAccessor->FinishTemporaryCommands();
    }
    break;

    case WPropertyEvent::Type::CancelTemporary:
    {
      m_pObjectAccessor->CancelTemporaryCommands();
    }
    break;
  }
}

bool WQtPropertyWidget::eventFilter(QObject* pWatched, QEvent* pEvent)
{
  if (pEvent->type() == QEvent::Wheel)
  {
    if (pWatched->parent())
    {
      pWatched->parent()->event(pEvent);
    }

    return true;
  }

  return false;
}

/// *** WQtUnsupportedPropertyWidget ***

WQtUnsupportedPropertyWidget::WQtUnsupportedPropertyWidget(const char* szMessage)
  : WQtPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pWidget = new QLabel(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pLayout->addWidget(m_pWidget);
  m_sMessage = szMessage;
}

void WQtUnsupportedPropertyWidget::OnInit()
{
  WQtScopedBlockSignals bs(m_pWidget);

  QString sMessage;
  if (!m_sMessage.IsEmpty())
  {
    sMessage = m_sMessage;
  }
  else
  {
    WStringBuilder tmp;
    sMessage = QStringLiteral("Unsupported Type: ") % QString::fromUtf8(m_pProp->GetSpecificType()->GetTypeName().GetData(tmp));
  }

  m_pWidget->setText(sMessage);
  m_pWidget->setToolTip(sMessage);
}


/// *** WQtStandardPropertyWidget ***

WQtStandardPropertyWidget::WQtStandardPropertyWidget()
  : WQtPropertyWidget()
{
}

void WQtStandardPropertyWidget::SetSelection(const WArrayPtr<WPropertySelection>& items)
{
  WQtPropertyWidget::SetSelection(items);

  m_OldValue = GetCommonValue(items, m_pProp);
  InternalSetValue(m_OldValue);
}

void WQtStandardPropertyWidget::BroadcastValueChanged(const WVariant& NewValue)
{
  if (NewValue == m_OldValue)
    return;

  m_OldValue = NewValue;

  WPropertyEvent ed;
  ed.m_Type = WPropertyEvent::Type::SingleValueChanged;
  ed.m_pProperty = m_pProp;
  ed.m_Value = NewValue;
  ed.m_pItems = &m_Items;
  PropertyChangedHandler(ed);
}


/// *** WQtPropertyPointerWidget ***

WQtPropertyPointerWidget::WQtPropertyPointerWidget()
  : WQtPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pGroup = new WQtCollapsibleGroupBox(this);
  m_pGroupLayout = new QHBoxLayout(nullptr);
  m_pGroupLayout->setSpacing(1);
  m_pGroupLayout->setContentsMargins(5, 0, 0, 0);
  m_pGroup->GetContent()->setLayout(m_pGroupLayout);

  m_pLayout->addWidget(m_pGroup);

  m_pAddButton = new WQtAddSubElementButton(WPropertyCategory::Member, WTranslate("POINTERWIDGET_Create"));
  m_pGroup->GetHeader()->layout()->addWidget(m_pAddButton);

  m_pDeleteButton = new WQtElementGroupButton(m_pGroup->GetHeader(), WQtElementGroupButton::ElementAction::DeleteElement, this);
  m_pGroup->GetHeader()->layout()->addWidget(m_pDeleteButton);
  connect(m_pDeleteButton, &QToolButton::clicked, this, &WQtPropertyPointerWidget::OnDeleteButtonClicked);

  m_pTypeWidget = nullptr;
}

WQtPropertyPointerWidget::~WQtPropertyPointerWidget()
{
  m_pGrid->GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(
    WMakeDelegate(&WQtPropertyPointerWidget::StructureEventHandler, this));
}

void WQtPropertyPointerWidget::OnInit()
{
  UpdateTitle();
  m_pGrid->SetCollapseState(m_pGroup);
  connect(m_pGroup, &WQtGroupBoxBase::CollapseStateChanged, m_pGrid, &WQtPropertyGridWidget::OnCollapseStateChanged);

  // Add Buttons
  auto pAttr = m_pProp->GetAttributeByType<WContainerAttribute>();
  m_pAddButton->setVisible(!pAttr || pAttr->CanAdd());
  m_pDeleteButton->setVisible(!pAttr || pAttr->CanDelete());

  m_pAddButton->Init(m_pGrid, m_pObjectAccessor, m_pType, m_pProp);
  m_pGrid->GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(
    WMakeDelegate(&WQtPropertyPointerWidget::StructureEventHandler, this));
}

void WQtPropertyPointerWidget::SetSelection(const WArrayPtr<WPropertySelection>& items)
{
  WQtScopedUpdatesDisabled _(this);

  WQtPropertyWidget::SetSelection(items);

  if (m_pTypeWidget)
  {
    m_pGroupLayout->removeWidget(m_pTypeWidget);
    delete m_pTypeWidget;
    m_pTypeWidget = nullptr;
  }


  WTempHybridArray<WPropertySelection, 8> emptyItems;
  WTempHybridArray<WPropertySelection, 8> subItems;
  for (const auto& item : m_Items)
  {
    WUuid ObjectGuid = m_pObjectAccessor->Get<WUuid>(item.m_pObject, m_pProp, item.m_Index);
    if (!ObjectGuid.IsValid())
    {
      emptyItems.PushBack(item);
    }
    else
    {
      WPropertySelection sel;
      sel.m_pObject = m_pObjectAccessor->GetObject(ObjectGuid);

      subItems.PushBack(sel);
    }
  }

  auto pAttr = m_pProp->GetAttributeByType<WContainerAttribute>();
  if (!pAttr || pAttr->CanAdd())
    m_pAddButton->setVisible(!emptyItems.IsEmpty());
  if (!pAttr || pAttr->CanDelete())
    m_pDeleteButton->setVisible(!subItems.IsEmpty());

  if (!emptyItems.IsEmpty())
  {
    m_pAddButton->SetSelection(emptyItems);
  }

  const WRTTI* pCommonType = nullptr;
  if (!subItems.IsEmpty())
  {
    pCommonType = WQtPropertyWidget::GetCommonBaseType(subItems);

    m_pTypeWidget = new WQtTypeWidget(m_pGroup->GetContent(), m_pGrid, m_pObjectAccessor, pCommonType, nullptr, nullptr);
    m_pTypeWidget->SetSelection(subItems);

    m_pGroupLayout->addWidget(m_pTypeWidget);
  }

  UpdateTitle(pCommonType);
}


void WQtPropertyPointerWidget::DoPrepareToDie()
{
  if (m_pTypeWidget)
  {
    m_pTypeWidget->PrepareToDie();
  }
}

void WQtPropertyPointerWidget::UpdateTitle(const WRTTI* pType /*= nullptr*/)
{
  WStringBuilder sb = WTranslate(m_pProp->GetPropertyName());
  if (pType != nullptr)
  {
    WStringBuilder tmp;
    sb.Append(": ", WTranslate(pType->GetTypeName().GetData(tmp)));
  }
  m_pGroup->SetTitle(sb);
}

void WQtPropertyPointerWidget::OnDeleteButtonClicked()
{
  m_pObjectAccessor->StartTransaction("Delete Object");

  WStatus res(W_SUCCESS);
  const WTempHybridArray<WPropertySelection, 8> selection = m_pTypeWidget->GetSelection();
  for (auto& item : selection)
  {
    res = m_pObjectAccessor->RemoveObject(item.m_pObject);
    if (res.Failed())
      break;
  }

  if (res.Failed())
    m_pObjectAccessor->CancelTransaction();
  else
    m_pObjectAccessor->FinishTransaction();

  WQtUiServices::GetSingleton()->MessageBoxStatus(res, "Removing sub-element from the property failed.");
}

void WQtPropertyPointerWidget::StructureEventHandler(const WDocumentObjectStructureEvent& e)
{
  if (IsUndead())
    return;

  switch (e.m_EventType)
  {
    case WDocumentObjectStructureEvent::Type::AfterObjectAdded:
    case WDocumentObjectStructureEvent::Type::AfterObjectMoved:
    case WDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    {
      if (!e.m_sParentProperty.IsEqual(m_pProp->GetPropertyName()))
        return;

      if (std::none_of(cbegin(m_Items), cend(m_Items),
            [&](const WPropertySelection& sel)
            { return e.m_pNewParent == sel.m_pObject || e.m_pPreviousParent == sel.m_pObject; }))
        return;

      SetSelection(m_Items);
    }
    break;
    default:
      break;
  }
}

/// *** WQtEmbeddedClassPropertyWidget ***

WQtEmbeddedClassPropertyWidget::WQtEmbeddedClassPropertyWidget()
  : WQtPropertyWidget()

{
}


WQtEmbeddedClassPropertyWidget::~WQtEmbeddedClassPropertyWidget()
{
  m_pGrid->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(WMakeDelegate(&WQtEmbeddedClassPropertyWidget::PropertyEventHandler, this));
  m_pGrid->GetCommandHistory()->m_Events.RemoveEventHandler(WMakeDelegate(&WQtEmbeddedClassPropertyWidget::CommandHistoryEventHandler, this));
}

void WQtEmbeddedClassPropertyWidget::SetSelection(const WArrayPtr<WPropertySelection>& items)
{
  WQtScopedUpdatesDisabled _(this);

  WQtPropertyWidget::SetSelection(items);

  // Retrieve the objects the property points to. This could be an embedded class or
  // an element of an array, be it pointer or embedded class.
  m_ResolvedObjects.Clear();
  for (const auto& item : m_Items)
  {
    WUuid ObjectGuid = m_pObjectAccessor->Get<WUuid>(item.m_pObject, m_pProp, item.m_Index);
    WPropertySelection sel;
    sel.m_pObject = m_pObjectAccessor->GetObject(ObjectGuid);
    // sel.m_Index; intentionally invalid as we just retrieved the value so it is a pointer to an object

    m_ResolvedObjects.PushBack(sel);
  }

  m_pResolvedType = m_pProp->GetSpecificType();
  if (m_pProp->GetFlags().IsSet(WPropertyFlags::Pointer))
  {
    m_pResolvedType = WQtPropertyWidget::GetCommonBaseType(m_ResolvedObjects);
  }
}

void WQtEmbeddedClassPropertyWidget::SetPropertyValue(const WAbstractProperty* pProperty, const WVariant& NewValue)
{
  WStatus res(W_SUCCESS);
  for (const auto& sel : m_ResolvedObjects)
  {
    res = m_pObjectAccessor->SetValue(sel.m_pObject, pProperty, NewValue, sel.m_Index);
    if (res.Failed())
      break;
  }
  // WPropertyEvent ed;
  // ed.m_Type = WPropertyEvent::Type::SingleValueChanged;
  // ed.m_pProperty = pProperty;
  // ed.m_Value = NewValue;
  // ed.m_pItems = &m_ResolvedObjects;

  // m_Events.Broadcast(ed);
}

void WQtEmbeddedClassPropertyWidget::OnInit()
{
  m_pGrid->GetObjectManager()->m_PropertyEvents.AddEventHandler(WMakeDelegate(&WQtEmbeddedClassPropertyWidget::PropertyEventHandler, this));
  m_pGrid->GetCommandHistory()->m_Events.AddEventHandler(WMakeDelegate(&WQtEmbeddedClassPropertyWidget::CommandHistoryEventHandler, this));
}


void WQtEmbeddedClassPropertyWidget::DoPrepareToDie() {}

void WQtEmbeddedClassPropertyWidget::PropertyEventHandler(const WDocumentObjectPropertyEvent& e)
{
  if (IsUndead())
    return;

  if (std::none_of(cbegin(m_ResolvedObjects), cend(m_ResolvedObjects), [=](const WPropertySelection& sel)
        { return e.m_pObject == sel.m_pObject; }))
    return;

  if (!m_QueuedChanges.Contains(e.m_sProperty))
  {
    m_QueuedChanges.PushBack(e.m_sProperty);
  }
}


void WQtEmbeddedClassPropertyWidget::CommandHistoryEventHandler(const WCommandHistoryEvent& e)
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
      FlushQueuedChanges();
    }
    break;

    default:
      break;
  }
}

void WQtEmbeddedClassPropertyWidget::FlushQueuedChanges()
{
  for (const WString& sProperty : m_QueuedChanges)
  {
    OnPropertyChanged(sProperty);
  }

  m_QueuedChanges.Clear();
}

/// *** WQtPropertyTypeWidget ***

WQtPropertyTypeWidget::WQtPropertyTypeWidget(bool bAddCollapsibleGroup)
  : WQtPropertyWidget()
{
  m_pLayout = new QVBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);
  m_pGroup = nullptr;
  m_pGroupLayout = nullptr;

  if (bAddCollapsibleGroup)
  {
    m_pGroup = new WQtCollapsibleGroupBox(this);
    m_pGroupLayout = new QVBoxLayout(nullptr);
    m_pGroupLayout->setSpacing(1);
    m_pGroupLayout->setContentsMargins(5, 0, 0, 0);
    m_pGroup->GetContent()->setLayout(m_pGroupLayout);

    m_pLayout->addWidget(m_pGroup);
  }
  m_pTypeWidget = nullptr;
}

WQtPropertyTypeWidget::~WQtPropertyTypeWidget() = default;

void WQtPropertyTypeWidget::OnInit()
{
  if (m_pGroup)
  {
    m_pGroup->SetTitle(WTranslate(m_pProp->GetPropertyName()));
    m_pGrid->SetCollapseState(m_pGroup);
    connect(m_pGroup, &WQtGroupBoxBase::CollapseStateChanged, m_pGrid, &WQtPropertyGridWidget::OnCollapseStateChanged);
  }
}

void WQtPropertyTypeWidget::SetSelection(const WArrayPtr<WPropertySelection>& items)
{
  WQtScopedUpdatesDisabled _(this);

  WQtPropertyWidget::SetSelection(items);

  QVBoxLayout* pLayout = m_pGroup != nullptr ? m_pGroupLayout : m_pLayout;
  QWidget* pOwner = m_pGroup != nullptr ? m_pGroup->GetContent() : this;
  if (m_pTypeWidget)
  {
    pLayout->removeWidget(m_pTypeWidget);
    delete m_pTypeWidget;
    m_pTypeWidget = nullptr;
  }

  // Retrieve the objects the property points to. This could be an embedded class or
  // an element of an array, be it pointer or embedded class.
  WTempHybridArray<WPropertySelection, 8> ResolvedObjects;
  for (const auto& item : m_Items)
  {
    WUuid ObjectGuid = m_pObjectAccessor->Get<WUuid>(item.m_pObject, m_pProp, item.m_Index);
    WPropertySelection sel;
    sel.m_pObject = m_pObjectAccessor->GetObject(ObjectGuid);
    // sel.m_Index; intentionally invalid as we just retrieved the value so it is a pointer to an object

    ResolvedObjects.PushBack(sel);
  }

  const WRTTI* pCommonType = nullptr;
  if (m_pProp->GetFlags().IsSet(WPropertyFlags::Pointer))
  {
    pCommonType = WQtPropertyWidget::GetCommonBaseType(ResolvedObjects);
  }
  else
  {
    // If we create a widget for a member class we already determined the common base type at the parent type widget.
    // As we are not dealing with a pointer in this case the type must match the property exactly.
    pCommonType = m_pProp->GetSpecificType();
  }
  m_pTypeWidget = new WQtTypeWidget(pOwner, m_pGrid, m_pObjectAccessor, pCommonType, nullptr, nullptr);
  pLayout->addWidget(m_pTypeWidget);
  m_pTypeWidget->SetSelection(ResolvedObjects);
}


void WQtPropertyTypeWidget::SetIsDefault(bool bIsDefault)
{
  // The default state set by the parent object / container only refers to the element's correct position in the container but the entire state of the object. As recursively checking an entire object if is has any non-default values is quite costly, we just pretend the object is never in its default state the the user can click revert to default on any object at any time.
  m_bIsDefault = false;
}

void WQtPropertyTypeWidget::DoPrepareToDie()
{
  if (m_pTypeWidget)
  {
    m_pTypeWidget->PrepareToDie();
  }
}

/// *** WQtPropertyContainerWidget ***

WQtPropertyContainerWidget::WQtPropertyContainerWidget()
  : WQtPropertyWidget()

{
  m_Pal = palette();
  setAutoFillBackground(true);

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  m_pLayout->setSpacing(0);
  setLayout(m_pLayout);

  m_pGroup = new WQtCollapsibleGroupBox(this);
  m_pGroupLayout = new QVBoxLayout(nullptr);
  m_pGroupLayout->setSpacing(1);
  m_pGroupLayout->setContentsMargins(5, 0, 0, 0);
  m_pGroup->GetContent()->setLayout(m_pGroupLayout);
  m_pGroup->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  connect(m_pGroup, &QWidget::customContextMenuRequested, this, &WQtPropertyContainerWidget::OnContainerContextMenu);

  setAcceptDrops(true);
  m_pLayout->addWidget(m_pGroup);
}

WQtPropertyContainerWidget::~WQtPropertyContainerWidget()
{
  Clear();
}

void WQtPropertyContainerWidget::SetSelection(const WArrayPtr<WPropertySelection>& items)
{
  WQtPropertyWidget::SetSelection(items);

  UpdateElements();

  if (m_pAddButton)
  {
    m_pAddButton->SetSelection(m_Items);
  }
}

void WQtPropertyContainerWidget::SetIsDefault(bool bIsDefault)
{
  // This is called from the type widget which we ignore as we have a tighter scoped default value provider for containers.
}

void WQtPropertyContainerWidget::DoPrepareToDie()
{
  for (const auto& e : m_Elements)
  {
    e.m_pWidget->PrepareToDie();
  }
}

void WQtPropertyContainerWidget::dragEnterEvent(QDragEnterEvent* event)
{
  updateDropIndex(event);
}

void WQtPropertyContainerWidget::dragMoveEvent(QDragMoveEvent* event)
{
  updateDropIndex(event);
}

void WQtPropertyContainerWidget::dragLeaveEvent(QDragLeaveEvent* event)
{
  m_iDropSource = -1;
  m_iDropTarget = -1;
  update();
}

void WQtPropertyContainerWidget::dropEvent(QDropEvent* event)
{
  if (updateDropIndex(event))
  {
    WQtGroupBoxBase* pGroup = qobject_cast<WQtGroupBoxBase*>(event->source());
    Element* pDragElement =
      std::find_if(begin(m_Elements), end(m_Elements), [pGroup](const Element& elem) -> bool
        { return elem.m_pSubGroup == pGroup; });
    if (pDragElement)
    {
      const WAbstractProperty* pProp = pDragElement->m_pWidget->GetProperty();
      WTempHybridArray<WPropertySelection, 8> items = pDragElement->m_pWidget->GetSelection();
      if (m_iDropSource != m_iDropTarget && (m_iDropSource + 1) != m_iDropTarget)
      {
        MoveItems(items, m_iDropTarget - m_iDropSource);
      }
    }
  }
  m_iDropSource = -1;
  m_iDropTarget = -1;
  update();
}

void WQtPropertyContainerWidget::paintEvent(QPaintEvent* event)
{
  WQtPropertyWidget::paintEvent(event);
  if (m_iDropSource != -1 && m_iDropTarget != -1)
  {
    WInt32 iYPos = 0;
    if (m_iDropTarget < (WInt32)m_Elements.GetCount())
    {
      const QPoint globalPos = m_Elements[m_iDropTarget].m_pSubGroup->mapToGlobal(QPoint(0, 0));
      iYPos = mapFromGlobal(globalPos).y();
    }
    else
    {
      const QPoint globalPos = m_Elements[m_Elements.GetCount() - 1].m_pSubGroup->mapToGlobal(QPoint(0, 0));
      iYPos = mapFromGlobal(globalPos).y() + m_Elements[m_Elements.GetCount() - 1].m_pSubGroup->height();
    }

    QPainter painter(this);
    painter.setPen(QPen(Qt::PenStyle::NoPen));
    painter.setBrush(palette().brush(QPalette::Highlight));
    painter.drawRect(0, iYPos - 3, width(), 4);
  }
}

void WQtPropertyContainerWidget::showEvent(QShowEvent* event)
{
  // Use of style sheets (ADS) breaks previously set palette.
  setPalette(m_Pal);
  WQtPropertyWidget::showEvent(event);
}

bool WQtPropertyContainerWidget::updateDropIndex(QDropEvent* pEvent)
{
  if (pEvent->source() && pEvent->mimeData()->hasFormat("application/x-groupBoxDragProperty"))
  {
    // Is the drop source part of this widget?
    for (WUInt32 i = 0; i < m_Elements.GetCount(); i++)
    {
      if (m_Elements[i].m_pSubGroup == pEvent->source())
      {
        pEvent->setDropAction(Qt::MoveAction);
        pEvent->accept();
        WInt32 iNewDropTarget = -1;
        // Find closest drop target.
        const WInt32 iGlobalYPos = mapToGlobal(pEvent->position().toPoint()).y();
        for (WUInt32 j = 0; j < m_Elements.GetCount(); j++)
        {
          const QRect rect(m_Elements[j].m_pSubGroup->mapToGlobal(QPoint(0, 0)), m_Elements[j].m_pSubGroup->size());
          if (iGlobalYPos > rect.center().y())
          {
            iNewDropTarget = (WInt32)j + 1;
          }
          else if (iGlobalYPos < rect.center().y())
          {
            iNewDropTarget = (WInt32)j;
            break;
          }
        }
        if (m_iDropSource != (WInt32)i || m_iDropTarget != iNewDropTarget)
        {
          m_iDropSource = (WInt32)i;
          m_iDropTarget = iNewDropTarget;
          update();
        }
        return true;
      }
    }
  }

  if (m_iDropSource != -1 || m_iDropTarget != -1)
  {
    m_iDropSource = -1;
    m_iDropTarget = -1;
    update();
  }
  pEvent->ignore();
  return false;
}

void WQtPropertyContainerWidget::OnElementButtonClicked()
{
  WQtElementGroupButton* pButton = qobject_cast<WQtElementGroupButton*>(sender());
  const WAbstractProperty* pProp = pButton->GetGroupWidget()->GetProperty();
  WTempHybridArray<WPropertySelection, 8> items = pButton->GetGroupWidget()->GetSelection();

  switch (pButton->GetAction())
  {
    case WQtElementGroupButton::ElementAction::MoveElementUp:
    {
      MoveItems(items, -1);
    }
    break;
    case WQtElementGroupButton::ElementAction::MoveElementDown:
    {
      MoveItems(items, 2);
    }
    break;
    case WQtElementGroupButton::ElementAction::DeleteElement:
    {
      DeleteItems(items);
    }
    break;

    case WQtElementGroupButton::ElementAction::Help:
      // handled by custom lambda
      break;
  }
}

void WQtPropertyContainerWidget::OnDragStarted(QMimeData& ref_mimeData)
{
  WQtGroupBoxBase* pGroup = qobject_cast<WQtGroupBoxBase*>(sender());
  Element* pDragElement =
    std::find_if(begin(m_Elements), end(m_Elements), [pGroup](const Element& elem) -> bool
      { return elem.m_pSubGroup == pGroup; });
  if (pDragElement)
  {
    ref_mimeData.setData("application/x-groupBoxDragProperty", QByteArray());
  }
}

void WQtPropertyContainerWidget::OnContainerContextMenu(const QPoint& pt)
{
  WQtGroupBoxBase* pGroup = qobject_cast<WQtGroupBoxBase*>(sender());

  QMenu m;
  m.setToolTipsVisible(true);
  ExtendContextMenu(m);

  if (!m.isEmpty())
  {
    m.exec(pGroup->mapToGlobal(pt));
  }
}

void WQtPropertyContainerWidget::OnCustomElementContextMenu(const QPoint& pt)
{
  WQtGroupBoxBase* pGroup = qobject_cast<WQtGroupBoxBase*>(sender());
  Element* pElement = std::find_if(begin(m_Elements), end(m_Elements), [pGroup](const Element& elem) -> bool
    { return elem.m_pSubGroup == pGroup; });

  if (pElement)
  {
    QMenu m;
    m.setToolTipsVisible(true);
    pElement->m_pWidget->ExtendContextMenu(m);

    m_pGrid->ExtendContextMenu(m, pElement->m_pWidget);

    if (!m.isEmpty())
    {
      m.exec(pGroup->mapToGlobal(pt));
    }
  }
}

WQtGroupBoxBase* WQtPropertyContainerWidget::CreateElement(QWidget* pParent)
{
  auto pBox = new WQtCollapsibleGroupBox(pParent);
  return pBox;
}

WQtPropertyWidget* WQtPropertyContainerWidget::CreateWidget(WUInt32 index)
{
  return new WQtPropertyTypeWidget();
}

WQtPropertyContainerWidget::Element& WQtPropertyContainerWidget::AddElement(WUInt32 index)
{
  WQtGroupBoxBase* pSubGroup = CreateElement(m_pGroup);
  pSubGroup->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  connect(pSubGroup, &WQtGroupBoxBase::CollapseStateChanged, m_pGrid, &WQtPropertyGridWidget::OnCollapseStateChanged);
  connect(pSubGroup, &QWidget::customContextMenuRequested, this, &WQtPropertyContainerWidget::OnCustomElementContextMenu);

  QVBoxLayout* pSubLayout = new QVBoxLayout(nullptr);
  pSubLayout->setContentsMargins(5, 0, 5, 0);
  pSubLayout->setSpacing(1);
  pSubGroup->GetContent()->setLayout(pSubLayout);

  m_pGroupLayout->insertWidget((int)index, pSubGroup);

  WQtPropertyWidget* pNewWidget = CreateWidget(index);

  pNewWidget->setParent(pSubGroup);
  pSubLayout->addWidget(pNewWidget);

  pNewWidget->Init(m_pGrid, m_pObjectAccessor, m_pType, m_pProp);

  // Add Buttons
  auto pAttr = m_pProp->GetAttributeByType<WContainerAttribute>();
  if ((!pAttr || pAttr->CanMove()) && GetContainerCategory() != WPropertyCategory::Map)
  {
    pSubGroup->SetDraggable(true);
    connect(pSubGroup, &WQtGroupBoxBase::DragStarted, this, &WQtPropertyContainerWidget::OnDragStarted);
  }

  WQtElementGroupButton* pHelpButton = new WQtElementGroupButton(pSubGroup->GetHeader(), WQtElementGroupButton::ElementAction::Help, pNewWidget);
  pSubGroup->GetHeader()->layout()->addWidget(pHelpButton);
  pHelpButton->setVisible(false); // added now, and shown later when we know the URL

  if (!pAttr || pAttr->CanDelete())
  {
    WQtElementGroupButton* pDeleteButton =
      new WQtElementGroupButton(pSubGroup->GetHeader(), WQtElementGroupButton::ElementAction::DeleteElement, pNewWidget);
    pSubGroup->GetHeader()->layout()->addWidget(pDeleteButton);
    connect(pDeleteButton, &QToolButton::clicked, this, &WQtPropertyContainerWidget::OnElementButtonClicked);
  }

  m_Elements.InsertAt(index, Element(pSubGroup, pNewWidget, pHelpButton));
  return m_Elements[index];
}

void WQtPropertyContainerWidget::RemoveElement(WUInt32 index)
{
  Element& elem = m_Elements[index];

  m_pGroupLayout->removeWidget(elem.m_pSubGroup);
  delete elem.m_pSubGroup;
  m_Elements.RemoveAtAndCopy(index);
}

void WQtPropertyContainerWidget::UpdateElements()
{
  WQtScopedUpdatesDisabled _(this);

  GetRequiredElements(m_Keys);
  const WUInt32 iElements = m_Keys.GetCount();

  while (m_Elements.GetCount() > iElements)
  {
    RemoveElement(m_Elements.GetCount() - 1);
  }
  while (m_Elements.GetCount() < iElements)
  {
    AddElement(m_Elements.GetCount());
  }

  for (WUInt32 i = 0; i < iElements; ++i)
  {
    UpdateElement(i);
  }

  UpdatePropertyMetaState();

  // Force re-layout of parent hierarchy to prevent flicker.
  QWidget* pCur = m_pGroup;
  while (pCur != nullptr && qobject_cast<QScrollArea*>(pCur) == nullptr)
  {
    pCur->updateGeometry();
    pCur = pCur->parentWidget();
  }
}

void WQtPropertyContainerWidget::GetRequiredElements(WDynamicArray<WVariant>& out_keys) const
{
  out_keys.Clear();
  if (GetContainerCategory() == WPropertyCategory::Map)
  {
    W_VERIFY(m_pObjectAccessor->GetKeys(m_Items[0].m_pObject, m_pProp, out_keys).Succeeded(), "GetKeys should always succeed.");
    WTempHybridArray<WVariant, 16> keys;
    for (WUInt32 i = 1; i < m_Items.GetCount(); i++)
    {
      keys.Clear();
      W_VERIFY(m_pObjectAccessor->GetKeys(m_Items[i].m_pObject, m_pProp, keys).Succeeded(), "GetKeys should always succeed.");
      for (WInt32 k = (WInt32)m_Keys.GetCount() - 1; k >= 0; --k)
      {
        if (!keys.Contains(m_Keys[k]))
        {
          out_keys.RemoveAtAndSwap(k);
        }
      }
    }
    out_keys.Sort([](const WVariant& a, const WVariant& b)
      { return a.Get<WString>().Compare(b.Get<WString>()) < 0; });
    return;
  }
  else
  {
    WInt32 iElements = 0x7FFFFFFF;
    for (const auto& item : m_Items)
    {
      WInt32 iCount = 0;
      W_VERIFY(m_pObjectAccessor->GetCount(item.m_pObject, m_pProp, iCount).Succeeded(), "GetCount should always succeed.");
      iElements = WMath::Min(iElements, iCount);
    }
    W_ASSERT_DEV(iElements >= 0, "Mismatch between storage and RTTI ({0})", iElements);
    for (WUInt32 i = 0; i < (WUInt32)iElements; i++)
    {
      out_keys.PushBack(i);
    }

    return;
  }
}

void WQtPropertyContainerWidget::UpdatePropertyMetaState()
{
  WPropertyMetaState* pMeta = WPropertyMetaState::GetSingleton();
  WHashTable<WVariant, WPropertyUiState> ElementStates;
  pMeta->GetContainerElementsState(m_Items, m_pProp->GetPropertyName(), ElementStates);

  WDefaultContainerState defaultState(m_pType, m_pObjectAccessor, m_Items, m_pProp->GetPropertyName());
  m_bIsDefault = defaultState.IsDefaultContainer();
  m_pGroup->SetBoldTitle(!m_bIsDefault);

  QColor qColor = WQtPropertyWidget::SetPaletteBackgroundColor(defaultState.GetBackgroundColor(), m_Pal);
  setPalette(m_Pal);

  const bool bReadOnly = m_pProp->GetFlags().IsSet(WPropertyFlags::ReadOnly) ||
                         (m_pProp->GetAttributeByType<WReadOnlyAttribute>() != nullptr);
  for (WUInt32 i = 0; i < m_Elements.GetCount(); i++)
  {
    Element& element = m_Elements[i];
    WVariant& key = m_Keys[i];
    const bool bIsDefault = defaultState.IsDefaultElement(key);
    auto itData = ElementStates.Find(key);
    WPropertyUiState::Visibility state = WPropertyUiState::Default;
    if (itData.IsValid())
    {
      state = itData.Value().m_Visibility;
    }

    if (element.m_pSubGroup)
    {
      element.m_pSubGroup->setVisible(state != WPropertyUiState::Invisible);
      element.m_pSubGroup->setEnabled(!bReadOnly && state != WPropertyUiState::Disabled);
      element.m_pSubGroup->SetBoldTitle(!bIsDefault);

      // If the fill color is invalid that means no border is drawn and we don't want to change the color then.
      if (!element.m_pSubGroup->GetFillColor().isValid())
      {
        element.m_pSubGroup->SetFillColor(qColor);
      }
    }
    if (element.m_pWidget)
    {
      element.m_pWidget->setVisible(state != WPropertyUiState::Invisible);
      element.m_pWidget->SetReadOnly(bReadOnly || state == WPropertyUiState::Disabled);
      element.m_pWidget->SetIsDefault(bIsDefault);
    }
  }
}

WPropertyCategory::Enum WQtPropertyContainerWidget::GetContainerCategory() const
{
  return m_pProp->GetCategory();
}

void WQtPropertyContainerWidget::Clear()
{
  while (m_Elements.GetCount() > 0)
  {
    RemoveElement(m_Elements.GetCount() - 1);
  }

  m_Elements.Clear();
}

void WQtPropertyContainerWidget::OnInit()
{
  WStringBuilder fullname(m_pType->GetTypeName(), "::", m_pProp->GetPropertyName());

  m_pGroup->SetTitle(WTranslate(fullname));

  const WContainerAttribute* pArrayAttr = m_pProp->GetAttributeByType<WContainerAttribute>();
  if (!pArrayAttr || pArrayAttr->CanAdd())
  {
    WStringBuilder sTmp, tmp2;
    sTmp.SetFormat(WTranslate("CONTAINER_AddEntry").GetData(tmp2), m_pProp->GetPropertyName());

    m_pAddButton = new WQtAddSubElementButton(GetContainerCategory(), sTmp);
    m_pAddButton->Init(m_pGrid, m_pObjectAccessor, m_pType, m_pProp);

    QWidget* pTmp = new QWidget();
    pTmp->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    pTmp->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout* pLayout = new QHBoxLayout();
    pLayout->setContentsMargins(0, 2, 2, 5);
    pTmp->setLayout(pLayout);
    pLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
    pLayout->addWidget(m_pAddButton);
    pLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

    m_pGroupLayout->addWidget(pTmp);
  }

  m_pGrid->SetCollapseState(m_pGroup);
  connect(m_pGroup, &WQtGroupBoxBase::CollapseStateChanged, m_pGrid, &WQtPropertyGridWidget::OnCollapseStateChanged);
}

void WQtPropertyContainerWidget::DeleteItems(WHybridArray<WPropertySelection, 8>& items)
{
  m_pObjectAccessor->StartTransaction("Delete Object");

  WStatus res(W_SUCCESS);
  const bool bIsValueType = WReflectionUtils::IsValueType(m_pProp);

  if (bIsValueType)
  {
    for (auto& item : items)
    {
      res = m_pObjectAccessor->RemoveValue(item.m_pObject, m_pProp, item.m_Index);
      if (res.Failed())
        break;
    }
  }
  else
  {
    WRemoveObjectCommand cmd;

    for (auto& item : items)
    {
      WUuid value = m_pObjectAccessor->Get<WUuid>(item.m_pObject, m_pProp, item.m_Index);
      const WDocumentObject* pObject = m_pObjectAccessor->GetObject(value);
      res = m_pObjectAccessor->RemoveObject(pObject);
      if (res.Failed())
        break;
    }
  }

  if (res.Failed())
    m_pObjectAccessor->CancelTransaction();
  else
    m_pObjectAccessor->FinishTransaction();

  WQtUiServices::GetSingleton()->MessageBoxStatus(res, "Removing sub-element from the property failed.");
}

void WQtPropertyContainerWidget::MoveItems(WHybridArray<WPropertySelection, 8>& items, WInt32 iMove)
{
  W_ASSERT_DEV(GetContainerCategory() != WPropertyCategory::Map, "Map entries can't be moved.");

  m_pObjectAccessor->StartTransaction("Reparent Object");

  WStatus res(W_SUCCESS);
  const bool bIsValueType = WReflectionUtils::IsValueType(m_pProp);
  if (bIsValueType)
  {
    for (auto& item : items)
    {
      WInt32 iCurIndex = item.m_Index.ConvertTo<WInt32>() + iMove;
      if (iCurIndex < 0 || iCurIndex > m_pObjectAccessor->GetCount(item.m_pObject, m_pProp))
        continue;

      res = m_pObjectAccessor->MoveValue(item.m_pObject, m_pProp, item.m_Index, iCurIndex);
      if (res.Failed())
        break;
    }
  }
  else
  {
    WMoveObjectCommand cmd;

    for (auto& item : items)
    {
      WInt32 iCurIndex = item.m_Index.ConvertTo<WInt32>() + iMove;
      if (iCurIndex < 0 || iCurIndex > m_pObjectAccessor->GetCount(item.m_pObject, m_pProp))
        continue;

      WUuid value = m_pObjectAccessor->Get<WUuid>(item.m_pObject, m_pProp, item.m_Index);
      const WDocumentObject* pObject = m_pObjectAccessor->GetObject(value);

      res = m_pObjectAccessor->MoveObject(pObject, item.m_pObject, m_pProp, iCurIndex);
      if (res.Failed())
        break;
    }
  }

  if (res.Failed())
    m_pObjectAccessor->CancelTransaction();
  else
    m_pObjectAccessor->FinishTransaction();

  WQtUiServices::GetSingleton()->MessageBoxStatus(res, "Moving sub-element failed.");
}


/// *** WQtPropertyStandardTypeContainerWidget ***

WQtPropertyStandardTypeContainerWidget::WQtPropertyStandardTypeContainerWidget()
  : WQtPropertyContainerWidget()
{
}

WQtPropertyStandardTypeContainerWidget::~WQtPropertyStandardTypeContainerWidget() = default;

WQtGroupBoxBase* WQtPropertyStandardTypeContainerWidget::CreateElement(QWidget* pParent)
{
  auto* pBox = new WQtInlinedGroupBox(pParent);
  pBox->SetFillColor(QColor::Invalid);
  return pBox;
}


WQtPropertyWidget* WQtPropertyStandardTypeContainerWidget::CreateWidget(WUInt32 index)
{
  return WQtPropertyGridWidget::CreateMemberPropertyWidget(m_pProp);
}

WQtPropertyContainerWidget::Element& WQtPropertyStandardTypeContainerWidget::AddElement(WUInt32 index)
{
  WQtPropertyContainerWidget::Element& elem = WQtPropertyContainerWidget::AddElement(index);
  return elem;
}

void WQtPropertyStandardTypeContainerWidget::RemoveElement(WUInt32 index)
{
  WQtPropertyContainerWidget::RemoveElement(index);
}

void WQtPropertyStandardTypeContainerWidget::UpdateElement(WUInt32 index)
{
  Element& elem = m_Elements[index];

  WTempHybridArray<WPropertySelection, 8> SubItems;

  for (const auto& item : m_Items)
  {
    WPropertySelection sel;
    sel.m_pObject = item.m_pObject;
    sel.m_Index = m_Keys[index];

    SubItems.PushBack(sel);
  }

  WStringBuilder sTitle;
  if (GetContainerCategory() == WPropertyCategory::Map)
    sTitle.SetFormat("{0}", m_Keys[index].ConvertTo<WString>());
  else
    sTitle.SetFormat("[{0}]", m_Keys[index].ConvertTo<WString>());

  elem.m_pSubGroup->SetTitle(sTitle);
  m_pGrid->SetCollapseState(elem.m_pSubGroup);
  elem.m_pWidget->SetSelection(SubItems);
}

/// *** WQtPropertyTypeContainerWidget ***

WQtPropertyTypeContainerWidget::WQtPropertyTypeContainerWidget() = default;

WQtPropertyTypeContainerWidget::~WQtPropertyTypeContainerWidget()
{
  m_pGrid->GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(
    WMakeDelegate(&WQtPropertyTypeContainerWidget::StructureEventHandler, this));
  m_pGrid->GetCommandHistory()->m_Events.RemoveEventHandler(WMakeDelegate(&WQtPropertyTypeContainerWidget::CommandHistoryEventHandler, this));
}

void WQtPropertyTypeContainerWidget::OnInit()
{
  WQtPropertyContainerWidget::OnInit();
  m_pGrid->GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(
    WMakeDelegate(&WQtPropertyTypeContainerWidget::StructureEventHandler, this));
  m_pGrid->GetCommandHistory()->m_Events.AddEventHandler(WMakeDelegate(&WQtPropertyTypeContainerWidget::CommandHistoryEventHandler, this));
}

void WQtPropertyTypeContainerWidget::UpdateElement(WUInt32 index)
{
  Element& elem = m_Elements[index];
  WTempHybridArray<WPropertySelection, 8> SubItems;

  // To be in line with all other WQtPropertyWidget the container element will
  // be given a selection in the form of this is the parent object, this is the property and in this
  // specific case this is the index you are working on. So SubItems only decorates the items with the correct index.
  for (const auto& item : m_Items)
  {
    WPropertySelection sel;
    sel.m_pObject = item.m_pObject;
    sel.m_Index = m_Keys[index];

    SubItems.PushBack(sel);
  }

  {
    // To get the correct name we actually need to resolve the selection to the actual objects
    // they are pointing to.
    WTempHybridArray<WPropertySelection, 8> ResolvedObjects;
    for (const auto& item : SubItems)
    {
      WUuid ObjectGuid = m_pObjectAccessor->Get<WUuid>(item.m_pObject, m_pProp, item.m_Index);
      WPropertySelection sel;
      sel.m_pObject = m_pObjectAccessor->GetObject(ObjectGuid);
      ResolvedObjects.PushBack(sel);
    }

    const WRTTI* pCommonType = WQtPropertyWidget::GetCommonBaseType(ResolvedObjects);

    // Label
    {
      WStringBuilder sTitle, tmp;
      sTitle.SetFormat("[{0}] - {1}", m_Keys[index].ConvertTo<WString>(), WTranslate(pCommonType->GetTypeName().GetData(tmp)));

      if (auto pInDev = pCommonType->GetAttributeByType<WInDevelopmentAttribute>())
      {
        sTitle.AppendFormat(" [ {} ]", pInDev->GetString());
      }

      elem.m_pSubGroup->SetTitle(sTitle);
    }

    WColor borderIconColor = WColor::MakeZero();

    if (const WColorAttribute* pColorAttrib = pCommonType->GetAttributeByType<WColorAttribute>())
    {
      borderIconColor = pColorAttrib->GetColor();
      elem.m_pSubGroup->SetFillColor(WToQtColor(pColorAttrib->GetColor()));
    }
    else if (const WCategoryAttribute* pCatAttrib = pCommonType->GetAttributeByType<WCategoryAttribute>())
    {
      borderIconColor = WColorScheme::GetCategoryColor(pCatAttrib->GetCategory(), WColorScheme::CategoryColorUsage::BorderIconColor);
      elem.m_pSubGroup->SetFillColor(WToQtColor(WColorScheme::GetCategoryColor(pCatAttrib->GetCategory(), WColorScheme::CategoryColorUsage::BorderColor)));
    }
    else
    {
      const QPalette& pal = palette();
      elem.m_pSubGroup->SetFillColor(pal.mid().color());
    }

    // Icon
    {
      WStringBuilder sIconName;
      sIconName.Set(":/TypeIcons/", pCommonType->GetTypeName(), ".svg");
      elem.m_pSubGroup->SetIcon(WQtUiServices::GetCachedIconResource(sIconName.GetData(), borderIconColor));
    }

    // help URL
    {
      WStringBuilder tmp;
      QString url = WMakeQString(WTranslateHelpURL(pCommonType->GetTypeName().GetData(tmp)));

      if (!url.isEmpty())
      {
        elem.m_pHelpButton->setVisible(true);
        connect(elem.m_pHelpButton, &QToolButton::clicked, this, [=]()
          { QDesktopServices::openUrl(QUrl(url)); });
      }
      else
      {
        elem.m_pHelpButton->setVisible(false);
      }
    }
  }


  m_pGrid->SetCollapseState(elem.m_pSubGroup);
  elem.m_pWidget->SetSelection(SubItems);
}

void WQtPropertyTypeContainerWidget::StructureEventHandler(const WDocumentObjectStructureEvent& e)
{
  if (IsUndead())
    return;

  switch (e.m_EventType)
  {
    case WDocumentObjectStructureEvent::Type::AfterObjectAdded:
    case WDocumentObjectStructureEvent::Type::AfterObjectMoved:
    case WDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    {
      if (!e.m_sParentProperty.IsEqual(m_pProp->GetPropertyName()))
        return;

      if (std::none_of(cbegin(m_Items), cend(m_Items),
            [&](const WPropertySelection& sel)
            { return e.m_pNewParent == sel.m_pObject || e.m_pPreviousParent == sel.m_pObject; }))
        return;

      m_bNeedsUpdate = true;
    }
    break;
    default:
      break;
  }
}

void WQtPropertyTypeContainerWidget::CommandHistoryEventHandler(const WCommandHistoryEvent& e)
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
      if (m_bNeedsUpdate)
      {
        m_bNeedsUpdate = false;
        UpdateElements();
      }
    }
    break;

    default:
      break;
  }
}

/// *** WQtVariantPropertyWidget ***

WQtVariantPropertyWidget::WQtVariantPropertyWidget()
{
  m_pLayout = new QVBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 4);
  m_pLayout->setSpacing(1);
  setLayout(m_pLayout);

  m_pTypeList = new QComboBox(this);
  m_pTypeList->installEventFilter(this);
  m_pTypeList->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pLayout->addWidget(m_pTypeList);
}

WQtVariantPropertyWidget::~WQtVariantPropertyWidget() = default;

void WQtVariantPropertyWidget::OnInit()
{
  WVariantType::Enum order[] = {
    WVariantType::Invalid,
    WVariantType::Bool,
    WVariantType::Int8,
    WVariantType::UInt8,
    WVariantType::Int16,
    WVariantType::UInt16,
    WVariantType::Int32,
    WVariantType::UInt32,
    WVariantType::Int64,
    WVariantType::UInt64,
    WVariantType::Float,
    WVariantType::Double,
    WVariantType::Angle,
    WVariantType::Time,
    WVariantType::Color,
    WVariantType::ColorGamma,
    WVariantType::String,
    WVariantType::StringView,
    WVariantType::HashedString,
    WVariantType::TempHashedString,
    WVariantType::Vector2,
    WVariantType::Vector3,
    WVariantType::Vector4,
    WVariantType::Vector2I,
    WVariantType::Vector3I,
    WVariantType::Vector4I,
    WVariantType::Vector2U,
    WVariantType::Vector3U,
    WVariantType::Vector4U,
    WVariantType::Quaternion,
    WVariantType::Transform,
    WVariantType::Matrix3,
    WVariantType::Matrix4,
    WVariantType::Uuid,
    WVariantType::DataBuffer,
    WVariantType::VariantArray,
    WVariantType::VariantDictionary,
    WVariantType::TypedPointer,
    WVariantType::TypedObject,
  };

  WStringBuilder sName;
  for (int i = 0; i < W_ARRAY_SIZE(order); ++i)
  {
    if (GetVariantTypeDisplayName(order[i], sName).Succeeded())
    {
      m_pTypeList->addItem(WMakeQString(WTranslate(sName)), order[i]);
    }
  }

  connect(m_pTypeList, &QComboBox::currentIndexChanged,
    [this](int iIndex)
    {
      ChangeVariantType(static_cast<WVariantType::Enum>(m_pTypeList->itemData(iIndex).toInt()));
    });
}

void WQtVariantPropertyWidget::InternalSetValue(const WVariant& value)
{
  WVariantType::Enum commonType = WVariantType::Invalid;
  const bool sameType = GetCommonVariantSubType(m_Items, m_pProp, commonType);
  const WRTTI* pNewtSubType = commonType != WVariantType::Invalid ? WReflectionUtils::GetTypeFromVariant(commonType) : nullptr;
  if (pNewtSubType != m_pCurrentSubType || m_pWidget == nullptr)
  {
    if (m_pWidget)
    {
      m_pWidget->PrepareToDie();
      m_pWidget->deleteLater();
      m_pWidget = nullptr;
    }
    m_pCurrentSubType = pNewtSubType;
    if (pNewtSubType)
    {
      if (commonType == WVariantType::VariantArray || commonType == WVariantType::VariantDictionary)
      {
        m_pWidget = new WQtVariantContainerWidget(commonType);
      }
      else
        m_pWidget = WQtPropertyGridWidget::GetFactory().CreateObject(pNewtSubType);

      if (!m_pWidget)
      {
        m_pWidget = new WQtUnsupportedPropertyWidget("<Unsupported Type>");
      }
    }
    else if (!sameType)
    {
      m_pWidget = new WQtUnsupportedPropertyWidget("Multi-selection has varying types");
    }
    else
    {
      m_pWidget = new WQtUnsupportedPropertyWidget("<Invalid Type>");
    }

    m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_pWidget->setParent(this);
    m_pLayout->addWidget(m_pWidget);
    m_pWidget->Init(m_pGrid, m_pObjectAccessor, m_pType, m_pProp);

    UpdateTypeListSelection(commonType);
  }
  m_pWidget->SetSelection(m_Items);
}

void WQtVariantPropertyWidget::DoPrepareToDie()
{
  if (m_pWidget)
    m_pWidget->PrepareToDie();
}

void WQtVariantPropertyWidget::UpdateTypeListSelection(WVariantType::Enum type)
{
  WQtScopedBlockSignals bs(m_pTypeList);
  for (int i = 0; i < m_pTypeList->count(); ++i)
  {
    if (m_pTypeList->itemData(i).toInt() == type)
    {
      m_pTypeList->setCurrentIndex(i);
      return;
    }
  }

  const WRTTI* pVariantEnum = WGetStaticRTTI<WVariantType>();
  WStringBuilder sName;
  if (WReflectionUtils::EnumerationToString(pVariantEnum, type, sName))
  {
    m_pTypeList->setPlaceholderText(WMakeQString(WTranslate(sName)));
  }

  m_pTypeList->setCurrentIndex(-1);
}

void WQtVariantPropertyWidget::ChangeVariantType(WVariantType::Enum type)
{
  m_pObjectAccessor->StartTransaction("Change variant type");
  // check if we have multiple values
  for (const auto& item : m_Items)
  {
    WVariant value;
    W_VERIFY(m_pObjectAccessor->GetValue(item.m_pObject, m_pProp, value, item.m_Index).Succeeded(), "");
    if (value.CanConvertTo(type))
    {
      W_VERIFY(m_pObjectAccessor->SetValue(item.m_pObject, m_pProp, value.ConvertTo(type), item.m_Index).Succeeded(), "");
    }
    else
    {
      W_VERIFY(m_pObjectAccessor->SetValue(item.m_pObject, m_pProp, WReflectionUtils::GetDefaultVariantFromType(type), item.m_Index).Succeeded(), "");
    }
  }
  m_pObjectAccessor->FinishTransaction();
}

void WQtVariantPropertyWidget::EnableTypeSelection(bool bEnable)
{
  m_pTypeList->setVisible(bEnable);
}

WResult WQtVariantPropertyWidget::GetVariantTypeDisplayName(WVariantType::Enum type, WStringBuilder& out_sName) const
{
  switch (type)
  {
    case WVariantType::FirstStandardType:
    case WVariantType::StringView:
    case WVariantType::DataBuffer:
    case WVariantType::TempHashedString:
    case WVariantType::Matrix3:
    case WVariantType::Matrix4:
    case WVariantType::Int8:
    case WVariantType::UInt8:
    case WVariantType::Int16:
    case WVariantType::UInt16:
    case WVariantType::UInt32:
    case WVariantType::Int64:
    case WVariantType::UInt64:
    case WVariantType::Double:
    case WVariantType::HashedString:
    case WVariantType::Vector2U:
    case WVariantType::Vector3U:
    case WVariantType::Vector4U:
    case WVariantType::Uuid:
    case WVariantType::ColorGamma:
      return W_FAILURE;

    case WVariantType::VariantArray:
    case WVariantType::VariantDictionary:
      break;

    default:
      if (type >= WVariantType::LastStandardType)
        return W_FAILURE;
      break;
  }

  const WRTTI* pVariantEnum = WGetStaticRTTI<WVariantType>();
  if (WReflectionUtils::EnumerationToString(pVariantEnum, type, out_sName) == false)
  {
    return W_FAILURE;
  }

  return W_SUCCESS;
}

/// *** WQtVariantContainerWidget ***

WQtVariantContainerWidget::WQtVariantContainerWidget(WVariantType::Enum variantType)
{
  switch (variantType)
  {
    case WVariantType::VariantArray:
      m_ContainerCategory = WPropertyCategory::Array;
      break;
    case WVariantType::VariantDictionary:
      m_ContainerCategory = WPropertyCategory::Map;
      break;
    default:
      W_REPORT_FAILURE("Only VariantArray and VariantDictionary are supported by WQtVariantContainerWidget.");
  }
}

void WQtVariantContainerWidget::OnInit()
{
  // Init is only called once at creation time so it is safe to replace the object accessor here.
  // As each WVariantSubAccessor manages only one depth level into the WVariant we need to wrap the object accessor for each level again which requires creating a unique accessor for each container and maintaining ownership to it.
  m_pVariantSubAccessor = W_DEFAULT_NEW(WVariantSubAccessor, m_pObjectAccessor, m_pProp);
  m_pObjectAccessor = m_pVariantSubAccessor.Borrow();
  WQtPropertyContainerWidget::OnInit();
}

void WQtVariantContainerWidget::SetSelection(const WArrayPtr<WPropertySelection>& items)
{
  WMap<const WDocumentObject*, WVariant> subItems;
  for (auto it : items)
  {
    subItems.Insert(it.m_pObject, it.m_Index);
  }
  m_pVariantSubAccessor->SetSubItems(subItems);
  WQtPropertyContainerWidget::SetSelection(items);
}

WPropertyCategory::Enum WQtVariantContainerWidget::GetContainerCategory() const
{
  return m_ContainerCategory;
}
