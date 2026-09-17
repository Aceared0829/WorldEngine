#pragma once

#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <VisualScriptPlugin/Runtime/VisualScriptDataType.h>

struct WVisualScriptVariableType
{
  using StorageType = WUInt8;

  enum Enum
  {
    Invalid = WVisualScriptDataType::Invalid,
    Bool = WVisualScriptDataType::Bool,
    Byte,
    Int,
    Int64,
    Float,
    Double,
    Color,
    Vector2,
    Vector3,
    Vector4,
    Quaternion,
    Transform,
    Time,
    Angle,
    String,
    HashedString,
    GameObject,
    Component,
    TypedPointer,
    Variant,

    Resource = WVisualScriptDataType::Resource,

    Default = Int,
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_EDITORPLUGINVISUALSCRIPT_DLL, WVisualScriptVariableType);

//////////////////////////////////////////////////////////////////////////////

struct WVisualScriptVariableCategory
{
  using StorageType = WUInt8;

  enum Enum
  {
    Member,
    Array,
    Map,
    Default = Member,
  };

  static WPropertyCategory::Enum GetPropertyCategory(Enum category);
};

W_DECLARE_REFLECTABLE_TYPE(W_EDITORPLUGINVISUALSCRIPT_DLL, WVisualScriptVariableCategory);

//////////////////////////////////////////////////////////////////////////////

struct WVisualScriptVariableTypeDeclaration
{
  W_DECLARE_POD_TYPE();

  WEnum<WVisualScriptVariableType> m_Type;
  WEnum<WVisualScriptVariableCategory> m_Category;
  bool m_bPublic = false;
  WUInt8 m_uiPadding = 0;

  bool operator==(const WVisualScriptVariableTypeDeclaration& other) const
  {
    return m_Type == other.m_Type && m_Category == other.m_Category && m_bPublic == other.m_bPublic;
  }

  WVisualScriptDataType::Enum GetDataType() const;
};

W_DECLARE_REFLECTABLE_TYPE(W_EDITORPLUGINVISUALSCRIPT_DLL, WVisualScriptVariableTypeDeclaration);
W_DECLARE_CUSTOM_VARIANT_TYPE(WVisualScriptVariableTypeDeclaration);

class WStreamWriter;
class WStreamReader;

void operator<<(WStreamWriter& inout_stream, const WVisualScriptVariableTypeDeclaration& value);
void operator>>(WStreamReader& inout_stream, WVisualScriptVariableTypeDeclaration& value);

template <>
struct WHashHelper<WVisualScriptVariableTypeDeclaration>
{
  W_ALWAYS_INLINE static WUInt32 Hash(const WVisualScriptVariableTypeDeclaration& value) { return WHashHelper<WUInt32>::Hash(*(const WUInt32*)&value); }

  W_ALWAYS_INLINE static bool Equal(const WVisualScriptVariableTypeDeclaration& a, const WVisualScriptVariableTypeDeclaration& b) { return a == b; }
};

//////////////////////////////////////////////////////////////////////////

class WVisualScriptVariableAttribute : public WTypeWidgetAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WVisualScriptVariableAttribute, WTypeWidgetAttribute);
};

//////////////////////////////////////////////////////////////////////////

class WQtVisualScriptVariableWidget : public WQtVariantPropertyWidget
{
  Q_OBJECT;

public:
  WQtVisualScriptVariableWidget();
  virtual ~WQtVisualScriptVariableWidget();

  virtual void InternalSetValue(const WVariant& value) override;

  virtual WResult GetVariantTypeDisplayName(WVariantType::Enum type, WStringBuilder& out_sName) const override;
};

//////////////////////////////////////////////////////////////////////////

class WQtVisualScriptVariableTypeDeclarationWidget : public WQtStandardPropertyWidget
{
  Q_OBJECT;

public:
  WQtVisualScriptVariableTypeDeclarationWidget();
  virtual ~WQtVisualScriptVariableTypeDeclarationWidget();

  virtual void OnInit() override;
  virtual void InternalSetValue(const WVariant& value) override;

private:
  void ChangeType();

  QHBoxLayout* m_pLayout = nullptr;
  QComboBox* m_pTypeList = nullptr;
  QMenu* m_pCategoryList = nullptr;
  QPushButton* m_pCategoryButton = nullptr;
  QPushButton* m_pVisibilityButton = nullptr;
};

//////////////////////////////////////////////////////////////////////////////

struct WVisualScriptVariable
{
  WHashedString m_sName;
  WVisualScriptVariableTypeDeclaration m_TypeDecl;
  WVariant m_DefaultValue;

  bool m_bClampRange = false;
  double m_fMinValue = 0.0;
  double m_fMaxValue = 1.0;

  static void ConvertDefaultValue(WVariant& inout_defaultValue, WVisualScriptVariableTypeDeclaration targetTypeDecl);
};

W_DECLARE_REFLECTABLE_TYPE(W_EDITORPLUGINVISUALSCRIPT_DLL, WVisualScriptVariable);

//////////////////////////////////////////////////////////////////////////

struct WVisualScriptExpressionDataType
{
  using StorageType = WUInt8;

  enum Enum
  {
    Int,
    Float,
    Vector2,
    Vector3,
    Vector4,
    Color,

    Default = Float
  };

  static WVisualScriptDataType::Enum GetVisualScriptDataType(Enum dataType);
};

W_DECLARE_REFLECTABLE_TYPE(W_EDITORPLUGINVISUALSCRIPT_DLL, WVisualScriptExpressionDataType);

struct WVisualScriptExpressionVariable
{
  WHashedString m_sName;
  WEnum<WVisualScriptExpressionDataType> m_Type;
};

W_DECLARE_REFLECTABLE_TYPE(W_EDITORPLUGINVISUALSCRIPT_DLL, WVisualScriptExpressionVariable);
