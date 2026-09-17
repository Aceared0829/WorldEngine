#include <EditorPluginVisualScript/EditorPluginVisualScriptPCH.h>

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptVariable.moc.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WVisualScriptVariableType, 1)
  W_ENUM_CONSTANT(WVisualScriptVariableType::Bool),
  W_ENUM_CONSTANT(WVisualScriptVariableType::Byte),
  W_ENUM_CONSTANT(WVisualScriptVariableType::Int),
  W_ENUM_CONSTANT(WVisualScriptVariableType::Int64),
  W_ENUM_CONSTANT(WVisualScriptVariableType::Float),
  W_ENUM_CONSTANT(WVisualScriptVariableType::Double),
  W_ENUM_CONSTANT(WVisualScriptVariableType::Color),
  W_ENUM_CONSTANT(WVisualScriptVariableType::Vector2),
  W_ENUM_CONSTANT(WVisualScriptVariableType::Vector3),
  W_ENUM_CONSTANT(WVisualScriptVariableType::Vector4),
  W_ENUM_CONSTANT(WVisualScriptVariableType::Quaternion),
  W_ENUM_CONSTANT(WVisualScriptVariableType::Transform),
  W_ENUM_CONSTANT(WVisualScriptVariableType::Time),
  W_ENUM_CONSTANT(WVisualScriptVariableType::Angle),
  W_ENUM_CONSTANT(WVisualScriptVariableType::String),
  W_ENUM_CONSTANT(WVisualScriptVariableType::HashedString),
  W_ENUM_CONSTANT(WVisualScriptVariableType::GameObject),
  W_ENUM_CONSTANT(WVisualScriptVariableType::Component),
  W_ENUM_CONSTANT(WVisualScriptVariableType::TypedPointer),
  W_ENUM_CONSTANT(WVisualScriptVariableType::Variant),
  // W_ENUM_CONSTANT(WVisualScriptVariableType::Resource), // Not yet supported in the editor
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

static_assert(static_cast<int>(WVisualScriptVariableType::Variant) == static_cast<int>(WVisualScriptDataType::Variant));
static_assert(static_cast<int>(WVisualScriptVariableType::Resource) == static_cast<int>(WVisualScriptDataType::Resource));

///////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WVisualScriptVariableCategory, 1)
  W_ENUM_CONSTANT(WVisualScriptVariableCategory::Member),
  W_ENUM_CONSTANT(WVisualScriptVariableCategory::Array),
  // W_ENUM_CONSTANT(WVisualScriptVariableCategory::Map), // Maps are not supported yet
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

// static
WPropertyCategory::Enum WVisualScriptVariableCategory::GetPropertyCategory(Enum category)
{
  switch (category)
  {
    case Member:
      return WPropertyCategory::Member;
    case Array:
      return WPropertyCategory::Array;
    case Map:
      return WPropertyCategory::Map;
    default:
      W_ASSERT_NOT_IMPLEMENTED;
      return WPropertyCategory::Member;
  }
}

///////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WVisualScriptVariableTypeDeclaration, WNoBase, 1, WRTTIDefaultAllocator<WVisualScriptVariableTypeDeclaration>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("Type", WVisualScriptVariableType, m_Type),
    W_ENUM_MEMBER_PROPERTY("Category", WVisualScriptVariableCategory, m_Category),
    W_MEMBER_PROPERTY("Public", m_bPublic),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
W_DEFINE_CUSTOM_VARIANT_TYPE(WVisualScriptVariableTypeDeclaration);
// clang-format on

WVisualScriptDataType::Enum WVisualScriptVariableTypeDeclaration::GetDataType() const
{
  if (m_Category == WVisualScriptVariableCategory::Array)
  {
    return WVisualScriptDataType::Array;
  }
  else if (m_Category == WVisualScriptVariableCategory::Map)
  {
    return WVisualScriptDataType::Map;
  }

  return static_cast<WVisualScriptDataType::Enum>(m_Type.GetValue());
}

void operator<<(WStreamWriter& inout_stream, const WVisualScriptVariableTypeDeclaration& value)
{
  inout_stream << value.m_Type;
  inout_stream << value.m_Category;
  inout_stream << value.m_bPublic;
}

void operator>>(WStreamReader& inout_stream, WVisualScriptVariableTypeDeclaration& value)
{
  inout_stream >> value.m_Type;
  inout_stream >> value.m_Category;
  inout_stream >> value.m_bPublic;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WVisualScriptVariableAttribute, 1, WRTTIDefaultAllocator<WVisualScriptVariableAttribute>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

WQtVisualScriptVariableWidget::WQtVisualScriptVariableWidget() = default;
WQtVisualScriptVariableWidget::~WQtVisualScriptVariableWidget() = default;

void WQtVisualScriptVariableWidget::InternalSetValue(const WVariant& value)
{
  WQtVariantPropertyWidget::InternalSetValue(value);

  bool bEnableTypeSelection = true;
  for (const auto& item : m_Items)
  {
    WVariant typeDeclVar;
    if (m_pObjectAccessor->GetValueByName(item.m_pObject, "Type", typeDeclVar, item.m_Index).Failed())
      break;

    if (typeDeclVar.GetReflectedType() != WGetStaticRTTI<WVisualScriptVariableTypeDeclaration>())
      break;

    auto typeDecl = typeDeclVar.Get<WVisualScriptVariableTypeDeclaration>();
    if (typeDecl.m_Type != WVisualScriptVariableType::Variant || typeDecl.m_Category != WVisualScriptVariableCategory::Member)
    {
      bEnableTypeSelection = false;
      break;
    }
  }

  EnableTypeSelection(bEnableTypeSelection);
}

WResult WQtVisualScriptVariableWidget::GetVariantTypeDisplayName(WVariantType::Enum type, WStringBuilder& out_sName) const
{
  if (type == WVariantType::Int8 ||
      type == WVariantType::Int16 ||
      type == WVariantType::UInt16 ||
      type == WVariantType::UInt32 ||
      type == WVariantType::UInt64 ||
      type == WVariantType::Vector2I ||
      type == WVariantType::Vector3I ||
      type == WVariantType::Vector4I ||
      type == WVariantType::Vector2U ||
      type == WVariantType::Vector3U ||
      type == WVariantType::Vector4U ||
      type == WVariantType::StringView ||
      type == WVariantType::TempHashedString)
    return W_FAILURE;

  WVisualScriptDataType::Enum dataType = WVisualScriptDataType::FromVariantType(type);
  if (type != WVariantType::Invalid && dataType == WVisualScriptDataType::Invalid)
    return W_FAILURE;

  const WRTTI* pVisualScriptDataType = WGetStaticRTTI<WVisualScriptDataType>();
  if (WReflectionUtils::EnumerationToString(pVisualScriptDataType, dataType, out_sName) == false)
    return W_FAILURE;

  return W_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

WQtVisualScriptVariableTypeDeclarationWidget::WQtVisualScriptVariableTypeDeclarationWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 4);
  m_pLayout->setSpacing(1);
  setLayout(m_pLayout);

  m_pTypeList = new QComboBox(this);
  m_pTypeList->installEventFilter(this);
  m_pLayout->addWidget(m_pTypeList);

  m_pCategoryList = new QMenu(this);

  m_pCategoryButton = new QPushButton(this);
  m_pCategoryButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
  m_pCategoryButton->installEventFilter(this);
  m_pCategoryButton->setMenu(m_pCategoryList);
  m_pLayout->addWidget(m_pCategoryButton);

  m_pVisibilityButton = new QPushButton(this);
  m_pVisibilityButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
  QIcon icon;
  icon.addFile(QString::fromUtf8(":/EditorFramework/Icons/ObjectsHidden.svg"), QSize(), QIcon::Normal, QIcon::Off);
  icon.addFile(QString::fromUtf8(":/EditorFramework/Icons/ObjectsVisible.svg"), QSize(), QIcon::Normal, QIcon::On);
  m_pVisibilityButton->setIcon(icon);
  m_pVisibilityButton->setCheckable(true);
  m_pVisibilityButton->setToolTip(QCoreApplication::translate("VisualScriptVariable", "Make variable public or private", nullptr));
  m_pLayout->addWidget(m_pVisibilityButton);
}

WQtVisualScriptVariableTypeDeclarationWidget::~WQtVisualScriptVariableTypeDeclarationWidget() = default;

void WQtVisualScriptVariableTypeDeclarationWidget::OnInit()
{
  WTempHybridArray<WReflectionUtils::EnumKeyValuePair, 16> enumValues;

  {
    WReflectionUtils::GetEnumKeysAndValues(WGetStaticRTTI<WVisualScriptVariableType>(), enumValues);
    for (auto& val : enumValues)
    {
      m_pTypeList->addItem(WMakeQString(WTranslate(val.m_sKey)), val.m_iValue);
    }

    connect(m_pTypeList, &QComboBox::currentIndexChanged, [this](int iIndex)
      { ChangeType(); });
  }

  {
    QActionGroup* pActionGroup = new QActionGroup(this);
    WReflectionUtils::GetEnumKeysAndValues(WGetStaticRTTI<WVisualScriptVariableCategory>(), enumValues);

    QString sIcons[] = {
      ":/EditorPluginVisualScript/Icons/VariableCategoryMember.svg",
      ":/EditorPluginVisualScript/Icons/VariableCategoryArray.svg",
      ":/EditorPluginVisualScript/Icons/VariableCategoryMap.svg"};

    W_ASSERT_DEV(W_ARRAY_SIZE(sIcons) >= enumValues.GetCount(), "Need exactly one icon per category");

    for (WUInt32 i = 0; i < enumValues.GetCount(); ++i)
    {
      QIcon icon;
      icon.addFile(sIcons[i]);

      QAction* pAction = new QAction(icon, WMakeQString(WTranslate(enumValues[i].m_sKey)), this);
      pAction->setCheckable(true);

      connect(pAction, &QAction::triggered, [this]()
        { ChangeType(); });

      pActionGroup->addAction(pAction);
      m_pCategoryList->addAction(pAction);
    }
  }

  connect(m_pVisibilityButton, &QPushButton::toggled, [this](bool)
    { ChangeType(); });
}

void WQtVisualScriptVariableTypeDeclarationWidget::InternalSetValue(const WVariant& value)
{
  auto typeDecl = value.Get<WVisualScriptVariableTypeDeclaration>();

  {
    WQtScopedBlockSignals bs(m_pTypeList);
    for (int i = 0; i < m_pTypeList->count(); ++i)
    {
      if (m_pTypeList->itemData(i).toInt() == typeDecl.m_Type)
      {
        m_pTypeList->setCurrentIndex(i);
        break;
      }
    }
  }

  {
    WQtScopedBlockSignals bs(m_pCategoryList);
    QAction* pAction = m_pCategoryList->actions()[typeDecl.m_Category];
    pAction->setChecked(true);

    m_pCategoryButton->setIcon(pAction->icon());
  }

  {
    WQtScopedBlockSignals bs(m_pVisibilityButton);
    m_pVisibilityButton->setChecked(typeDecl.m_bPublic);
  }
}

void WQtVisualScriptVariableTypeDeclarationWidget::ChangeType()
{
  QAction* pAction = nullptr;
  WUInt32 uiCategory = 0;
  for (auto action : m_pCategoryList->actions())
  {
    if (action->isChecked())
    {
      pAction = action;
      break;
    }
    ++uiCategory;
  }
  W_ASSERT_DEV(pAction != nullptr, "No category selected");

  m_pObjectAccessor->StartTransaction("Change variable type");

  for (const auto& item : m_Items)
  {
    WVisualScriptVariableTypeDeclaration typeDecl;

    typeDecl.m_Type = static_cast<WVisualScriptVariableType::Enum>(m_pTypeList->currentData().toInt());
    typeDecl.m_Category = static_cast<WVisualScriptVariableCategory::Enum>(uiCategory);
    typeDecl.m_bPublic = m_pVisibilityButton->isChecked();

    m_pObjectAccessor->SetValue(item.m_pObject, m_pProp, typeDecl, item.m_Index).AssertSuccess();
  }

  m_pObjectAccessor->FinishTransaction();

  m_pCategoryButton->setIcon(pAction->icon());
}

///////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WVisualScriptVariable, WNoBase, 3, WRTTIDefaultAllocator<WVisualScriptVariable>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Name", m_sName),
    W_MEMBER_PROPERTY("Type", m_TypeDecl),
    W_MEMBER_PROPERTY("DefaultValue", m_DefaultValue)->AddAttributes(new WDefaultValueAttribute(0), new WVisualScriptVariableAttribute()),
    W_MEMBER_PROPERTY("ClampRange", m_bClampRange),
    W_MEMBER_PROPERTY("MinValue", m_fMinValue),
    W_MEMBER_PROPERTY("MaxValue", m_fMaxValue)->AddAttributes(new WDefaultValueAttribute(1)),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

// static
void WVisualScriptVariable::ConvertDefaultValue(WVariant& inout_defaultValue, WVisualScriptVariableTypeDeclaration targetTypeDecl)
{
  auto ConvertOrSetToDefault = [](WVariant& v, WVisualScriptDataType::Enum targetType)
  {
    if (targetType == WVisualScriptDataType::Variant)
      return;

    auto variantTargetType = WVisualScriptDataType::GetVariantType(targetType);
    if (variantTargetType == WVariantType::Invalid || variantTargetType == WVariantType::TypedObject || variantTargetType == WVariantType::TypedPointer)
    {
      v = WVariant();
      return;
    }

    WResult res = W_SUCCESS;
    v = v.ConvertTo(variantTargetType, &res);
    if (res.Failed())
    {
      v = WReflectionUtils::GetDefaultVariantFromType(WVisualScriptDataType::GetRtti(targetType));
    }
  };

  auto targetType = static_cast<WVisualScriptDataType::Enum>(targetTypeDecl.m_Type.GetValue());
  if (targetTypeDecl.m_Category == WVisualScriptVariableCategory::Array)
  {
    if (inout_defaultValue.IsA<WVariantArray>())
    {
      WVariantArray a = inout_defaultValue.Get<WVariantArray>();
      for (auto& v : a)
      {
        ConvertOrSetToDefault(v, targetType);
      }
      inout_defaultValue = a;
    }
    else
    {
      WVariantArray a;
      ConvertOrSetToDefault(inout_defaultValue, targetType);
      a.PushBack(inout_defaultValue);
      inout_defaultValue = a;
    }
  }
  else if (targetTypeDecl.m_Category == WVisualScriptVariableCategory::Map)
  {
    if (inout_defaultValue.IsA<WVariantDictionary>())
    {
      WVariantDictionary d = inout_defaultValue.Get<WVariantDictionary>();
      for (auto it = d.GetIterator(); it.IsValid(); it.Next())
      {
        WVariant v = it.Value();
        ConvertOrSetToDefault(v, targetType);
        d[it.Key()] = v;
      }
      inout_defaultValue = d;
    }
    else
    {
      WVariantDictionary d;
      ConvertOrSetToDefault(inout_defaultValue, targetType);
      d["Key"] = inout_defaultValue;
      inout_defaultValue = d;
    }
  }
  else
  {
    ConvertOrSetToDefault(inout_defaultValue, targetType);
  }
}

/////////////////////////////////////////////////////////////////////////////

static WQtPropertyWidget* VisualScriptVariableTypeCreator(const WRTTI* pRtti)
{
  return new WQtVisualScriptVariableWidget();
}

static WQtPropertyWidget* VisualScriptVariableTypeDeclarationCreator(const WRTTI* pRtti)
{
  return new WQtVisualScriptVariableTypeDeclarationWidget();
}

void WVisualScriptVariable_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e)
{
  const WRTTI* pRtti = WGetStaticRTTI<WVisualScriptVariable>();

  auto& typeAccessor = e.m_pObject->GetTypeAccessor();

  if (typeAccessor.GetType() != pRtti)
    return;

  auto typeDecl = typeAccessor.GetValue("Type").Get<WVisualScriptVariableTypeDeclaration>();
  const bool bIsPublicNumberType = typeDecl.m_bPublic && WVisualScriptDataType::IsNumber(static_cast<WVisualScriptDataType::Enum>(typeDecl.m_Type.GetValue()));

  auto& props = *e.m_pPropertyStates;

  auto clampRangeVisibility = bIsPublicNumberType ? WPropertyUiState::Default : WPropertyUiState::Invisible;
  props["ClampRange"].m_Visibility = clampRangeVisibility;
  props["MinValue"].m_Visibility = clampRangeVisibility;
  props["MaxValue"].m_Visibility = clampRangeVisibility;
}

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginVisualScript, VisualScriptVariable)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "ToolsFoundation", "PropertyMetaState"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WVisualScriptVariableAttribute>(), VisualScriptVariableTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WVisualScriptVariableTypeDeclaration>(), VisualScriptVariableTypeDeclarationCreator);

    WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WVisualScriptVariable_PropertyMetaStateEventHandler);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WVisualScriptVariableAttribute>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WVisualScriptVariableTypeDeclaration>());

    WPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(WVisualScriptVariable_PropertyMetaStateEventHandler);
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

///////////////////////////////////////////////////////////////////////////

class WVisualScriptVariablePatch_1_2 : public WGraphPatch
{
public:
  WVisualScriptVariablePatch_1_2()
    : WGraphPatch("WVisualScriptVariable", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    auto* pDefaultValue = pNode->FindProperty("DefaultValue");
    auto* pExpose = pNode->FindProperty("Expose");

    if (pDefaultValue && pExpose)
    {
      WVisualScriptVariableTypeDeclaration typeDecl;

      WVariantType::Enum variantType = pDefaultValue->m_Value.GetType();
      typeDecl.m_Type = static_cast<WVisualScriptVariableType::Enum>(WVisualScriptDataType::FromVariantType(variantType));
      typeDecl.m_Category = WVisualScriptVariableCategory::Member;

      if (variantType == WVariantType::VariantArray)
      {
        typeDecl.m_Type = WVisualScriptVariableType::Variant;
        typeDecl.m_Category = WVisualScriptVariableCategory::Array;
      }
      else if (variantType == WVariantType::VariantDictionary)
      {
        typeDecl.m_Type = WVisualScriptVariableType::Variant;
        typeDecl.m_Category = WVisualScriptVariableCategory::Map;
      }

      typeDecl.m_bPublic = pExpose->m_Value.ConvertTo<bool>();

      pNode->AddProperty("Type", typeDecl);
    }
  }
};

WVisualScriptVariablePatch_1_2 g_WVisualScriptVariablePatch_1_2;

class WVisualScriptVariable_2_3 : public WGraphPatch
{
public:
  WVisualScriptVariable_2_3()
    : WGraphPatch("WVisualScriptVariable", 3)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    // The patch from 1 to 2 will already set the correct values for types.
    if (pNode->GetTypeVersion() < 2)
      return;

    auto* pType = pNode->FindProperty("Type");
    if (pType && pType->m_Value.IsA<WVisualScriptVariableTypeDeclaration>())
    {
      auto typeDecl = pType->m_Value.Get<WVisualScriptVariableTypeDeclaration>();

      constexpr WUInt32 uiOldVector3TypeValue = 8;
      if (typeDecl.m_Type.GetValue() == uiOldVector3TypeValue)
      {
        typeDecl.m_Type = WVisualScriptVariableType::Vector3;
        pType->m_Value = typeDecl;
        return;
      }

      constexpr WUInt32 uiOldQuaternionTypeValue = 9;
      constexpr WUInt32 uiOffsetToQuaternion = static_cast<WUInt32>(WVisualScriptVariableType::Quaternion) - uiOldQuaternionTypeValue;
      if (typeDecl.m_Type.GetValue() >= uiOldQuaternionTypeValue)
      {
        typeDecl.m_Type = static_cast<WVisualScriptVariableType::Enum>(typeDecl.m_Type.GetValue() + uiOffsetToQuaternion);
        pType->m_Value = typeDecl;
      }
    }
  }
};

WVisualScriptVariable_2_3 g_WVisualScriptVariable_2_3;

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WVisualScriptExpressionDataType, 1)
  W_ENUM_CONSTANT(WVisualScriptExpressionDataType::Int),
  W_ENUM_CONSTANT(WVisualScriptExpressionDataType::Float),
  W_ENUM_CONSTANT(WVisualScriptExpressionDataType::Vector2),
  W_ENUM_CONSTANT(WVisualScriptExpressionDataType::Vector3),
  W_ENUM_CONSTANT(WVisualScriptExpressionDataType::Vector4),
  W_ENUM_CONSTANT(WVisualScriptExpressionDataType::Color),
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_TYPE(WVisualScriptExpressionVariable, WNoBase, 1, WRTTIDefaultAllocator<WVisualScriptExpressionVariable>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Name", m_sName),
    W_ENUM_MEMBER_PROPERTY("Type", WVisualScriptExpressionDataType, m_Type),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

WVisualScriptDataType::Enum WVisualScriptExpressionDataType::GetVisualScriptDataType(Enum dataType)
{
  switch (dataType)
  {
    case Int:
      return WVisualScriptDataType::Int;
    case Float:
      return WVisualScriptDataType::Float;
    case Vector2:
      return WVisualScriptDataType::Vector2;
    case Vector3:
      return WVisualScriptDataType::Vector3;
    case Vector4:
      return WVisualScriptDataType::Vector4;
    case Color:
      return WVisualScriptDataType::Color;
    default:
      W_ASSERT_NOT_IMPLEMENTED;
  }

  return WVisualScriptDataType::Invalid;
}
