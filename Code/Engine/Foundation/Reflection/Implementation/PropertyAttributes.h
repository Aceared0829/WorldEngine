#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Math/ColorScheme.h>
#include <Foundation/Reflection/Reflection.h>

/// Base class of all attributes can be used to decorate a RTTI property.
class W_FOUNDATION_DLL WPropertyAttribute : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WPropertyAttribute, WReflectedClass);
};

/// A property attribute that indicates that the property may not be modified through the UI
class W_FOUNDATION_DLL WReadOnlyAttribute : public WPropertyAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WReadOnlyAttribute, WPropertyAttribute);
};

/// A property attribute that indicates that the property is not to be shown in the UI
class W_FOUNDATION_DLL WHiddenAttribute : public WPropertyAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WHiddenAttribute, WPropertyAttribute);
};

/// Marks a property as required. Asset check rules flag the property as an error if it is left empty
/// (e.g. an empty string, or an empty/invalid game object or component reference).
class W_FOUNDATION_DLL WRequiredAttribute : public WPropertyAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WRequiredAttribute, WPropertyAttribute);
};

/// A property attribute that indicates that the property is not to be serialized
/// and whatever it points to only exists temporarily while running or in editor.
class W_FOUNDATION_DLL WTemporaryAttribute : public WPropertyAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WTemporaryAttribute, WPropertyAttribute);
};

/// When placed on a component type, its editor shape icon will be rendered through geometry (always visible).
///
/// Useful for components whose icons tend to end up inside geometry, such as spline nodes.
class W_FOUNDATION_DLL WShapeIconAlwaysVisibleAttribute : public WPropertyAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WShapeIconAlwaysVisibleAttribute, WPropertyAttribute);
};

/// Used to categorize types (e.g. add component menu)
class W_FOUNDATION_DLL WCategoryAttribute : public WPropertyAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WCategoryAttribute, WPropertyAttribute);

public:
  WCategoryAttribute() = default;
  WCategoryAttribute(const char* szCategory)
    : m_sCategory(szCategory)
  {
  }

  const char* GetCategory() const { return m_sCategory; }

private:
  WUntrackedString m_sCategory;
};

/// A property attribute that indicates that this feature is still in development and should not be shown to all users.
class W_FOUNDATION_DLL WInDevelopmentAttribute : public WPropertyAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WInDevelopmentAttribute, WPropertyAttribute);

public:
  enum Phase
  {
    Alpha,
    Beta
  };

  WInDevelopmentAttribute() = default;
  WInDevelopmentAttribute(WInt32 iPhase) { m_Phase = iPhase; }

  const char* GetString() const;

  WInt32 m_Phase = Phase::Beta;
};


/// Used for dynamic titles of visual script nodes.
/// E.g. "Set Bool Property '{Name}'" will allow the title to by dynamic
/// by reading the current value of the 'Name' property.
class W_FOUNDATION_DLL WTitleAttribute : public WPropertyAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WTitleAttribute, WPropertyAttribute);

public:
  WTitleAttribute() = default;
  WTitleAttribute(const char* szTitle)
    : m_sTitle(szTitle)
  {
  }

  const char* GetTitle() const { return m_sTitle; }

private:
  WUntrackedString m_sTitle;
};

/// Used to colorize types
class W_FOUNDATION_DLL WColorAttribute : public WPropertyAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WColorAttribute, WPropertyAttribute);

public:
  WColorAttribute() = default;
  WColorAttribute(const WColor& color)
    : m_Color(color)
  {
  }
  const WColor& GetColor() const { return m_Color; }

private:
  WColor m_Color;
};

/// A property attribute that indicates that the alpha channel of an WColorGammaUB or WColor should be exposed in the UI.
class W_FOUNDATION_DLL WExposeColorAlphaAttribute : public WPropertyAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WExposeColorAlphaAttribute, WPropertyAttribute);
};

/// Used for any property shown as a line edit (int, float, vector etc).
class W_FOUNDATION_DLL WSuffixAttribute : public WPropertyAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WSuffixAttribute, WPropertyAttribute);

public:
  WSuffixAttribute() = default;
  WSuffixAttribute(const char* szSuffix)
    : m_sSuffix(szSuffix)
  {
  }

  const char* GetSuffix() const { return m_sSuffix; }

private:
  WUntrackedString m_sSuffix;
};

/// Used to show a text instead of the minimum value of a property.
class W_FOUNDATION_DLL WMinValueTextAttribute : public WPropertyAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WMinValueTextAttribute, WPropertyAttribute);

public:
  WMinValueTextAttribute() = default;
  WMinValueTextAttribute(const char* szText)
    : m_sText(szText)
  {
  }

  const char* GetText() const { return m_sText; }

private:
  WUntrackedString m_sText;
};

/// Sets the default value of the property.
class W_FOUNDATION_DLL WDefaultValueAttribute : public WPropertyAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WDefaultValueAttribute, WPropertyAttribute);

public:
  WDefaultValueAttribute() = default;

  WDefaultValueAttribute(const WVariant& value)
    : m_Value(value)
  {
  }

  WDefaultValueAttribute(WInt32 value)
    : m_Value(value)
  {
  }

  WDefaultValueAttribute(float value)
    : m_Value(value)
  {
  }

  WDefaultValueAttribute(double value)
    : m_Value(value)
  {
  }

  WDefaultValueAttribute(WStringView value)
    : m_Value(WVariant(value, false))
  {
  }

  WDefaultValueAttribute(const char* value)
    : m_Value(WVariant(WStringView(value), false))
  {
  }

  const WVariant& GetValue() const { return m_Value; }

private:
  WVariant m_Value;
};

/// A property attribute that allows to define min and max values for the UI. Min or max may be set to an invalid variant to indicate
/// unbounded values in one direction.
class W_FOUNDATION_DLL WClampValueAttribute : public WPropertyAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WClampValueAttribute, WPropertyAttribute);

public:
  WClampValueAttribute() = default;
  WClampValueAttribute(const WVariant& min, const WVariant& max)
    : m_MinValue(min)
    , m_MaxValue(max)
  {
  }

  const WVariant& GetMinValue() const { return m_MinValue; }
  const WVariant& GetMaxValue() const { return m_MaxValue; }

protected:
  WVariant m_MinValue;
  WVariant m_MaxValue;
};

/// Used to categorize properties into groups
class W_FOUNDATION_DLL WGroupAttribute : public WPropertyAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WGroupAttribute, WPropertyAttribute);

public:
  WGroupAttribute();
  WGroupAttribute(const char* szGroup, float fOrder = -1.0f);
  WGroupAttribute(const char* szGroup, const char* szIconName, float fOrder = -1.0f);

  const char* GetGroup() const { return m_sGroup; }
  const char* GetIconName() const { return m_sIconName; }
  float GetOrder() const { return m_fOrder; }

private:
  WUntrackedString m_sGroup;
  WUntrackedString m_sIconName;
  float m_fOrder = -1.0f;
};

/// Derive from this class if you want to define an attribute that replaces the property type widget.
///
/// Using this attribute affects both member properties as well as elements in a container but not the container widget.
/// When creating a property widget, the property grid will look for an attribute of this type and use
/// its type to look for a factory creator in WRttiMappedObjectFactory<WQtPropertyWidget>.
/// E.g. WRttiMappedObjectFactory<WQtPropertyWidget>::RegisterCreator(WGetStaticRTTI<WFileBrowserAttribute>(), FileBrowserCreator);
/// will replace the property widget for all properties that use WFileBrowserAttribute.
class W_FOUNDATION_DLL WTypeWidgetAttribute : public WPropertyAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WTypeWidgetAttribute, WPropertyAttribute);
};

/// Derive from this class if you want to define an attribute that replaces the property widget of containers.
///
/// Using this attribute affects the container widget but not container elements.
/// Only derive from this class if you want to replace the container widget itself, in every other case
/// prefer to use WTypeWidgetAttribute.
class W_FOUNDATION_DLL WContainerWidgetAttribute : public WPropertyAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WContainerWidgetAttribute, WPropertyAttribute);
};

/// Add this attribute to a tag set member property to make it use the tag set editor
/// and define the categories it will use as a ; separated list of category names.
///
/// Usage: W_SET_MEMBER_PROPERTY("Tags", m_Tags)->AddAttributes(new WTagSetWidgetAttribute("Category1;Category2")),
class W_FOUNDATION_DLL WTagSetWidgetAttribute : public WContainerWidgetAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WTagSetWidgetAttribute, WContainerWidgetAttribute);

public:
  WTagSetWidgetAttribute() = default;
  WTagSetWidgetAttribute(const char* szTagFilter)
    : m_sTagFilter(szTagFilter)
  {
  }

  const char* GetTagFilter() const { return m_sTagFilter; }

private:
  WUntrackedString m_sTagFilter;
};

/// This attribute indicates that a widget should not use temporary transactions when changing the value.
class W_FOUNDATION_DLL WNoTemporaryTransactionsAttribute : public WPropertyAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WNoTemporaryTransactionsAttribute, WPropertyAttribute);
};

/// Add this attribute to a variant map property to make it map to the exposed parameters
/// of an asset. For this, the member property name of the asset reference needs to be passed in.
/// The exposed parameters of the currently set asset on that property will be used as the source.
///
/// Usage:
/// W_ACCESSOR_PROPERTY("Effect", GetParticleEffectFile, SetParticleEffectFile)->AddAttributes(new WAssetBrowserAttribute("Particle
/// Effect")), W_MAP_ACCESSOR_PROPERTY("Parameters",...)->AddAttributes(new WExposedParametersAttribute("Effect")),
class W_FOUNDATION_DLL WExposedParametersAttribute : public WContainerWidgetAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WExposedParametersAttribute, WContainerWidgetAttribute);

public:
  WExposedParametersAttribute() = default;
  WExposedParametersAttribute(const char* szParametersSource)
    : m_sParametersSource(szParametersSource)
  {
  }

  const char* GetParametersSource() const { return m_sParametersSource; }

private:
  WUntrackedString m_sParametersSource;
};

/// Add this attribute to an embedded class or container property to make it retrieve its default values from a dynamic meta info object on an asset.
///
/// The default values are retrieved from the asset meta data of the currently set asset on that property.
///
/// Usage:
/// W_ACCESSOR_PROPERTY("Skeleton", GetSkeletonFile, SetSkeletonFile)->AddAttributes(new WAssetBrowserAttribute("Skeleton")),
///
/// // Use this if the embedded class m_SkeletonMetaData is of type WSkeletonMetaData.
/// W_MEMBER_PROPERTY("SkeletonMetaData", m_SkeletonMetaData)->AddAttributes(new WDynamicDefaultValueAttribute("Skeleton", "WSkeletonMetaData")),
///
/// // Use this if you don't want embed the entire meta object but just some container of it. In this case the LocalBones container must match in type to the property 'BonesArrayNameInMetaData' in the meta data type 'WSkeletonMetaData'.
/// W_MAP_MEMBER_PROPERTY("LocalBones", m_Bones)->AddAttributes(new WDynamicDefaultValueAttribute("Skeleton", "WSkeletonMetaData", "BonesArrayNameInMetaData")),
class W_FOUNDATION_DLL WDynamicDefaultValueAttribute : public WTypeWidgetAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WDynamicDefaultValueAttribute, WTypeWidgetAttribute);

public:
  WDynamicDefaultValueAttribute() = default;
  WDynamicDefaultValueAttribute(const char* szClassSource,
    const char* szClassType, const char* szClassProperty = nullptr)
    : m_sClassSource(szClassSource)
    , m_sClassType(szClassType)
    , m_sClassProperty(szClassProperty)
  {
  }

  const char* GetClassSource() const { return m_sClassSource; }
  const char* GetClassType() const { return m_sClassType; }
  const char* GetClassProperty() const { return m_sClassProperty; }

private:
  WUntrackedString m_sClassSource;
  WUntrackedString m_sClassType;
  WUntrackedString m_sClassProperty;
};


/// Sets the allowed actions on a container.
class W_FOUNDATION_DLL WContainerAttribute : public WPropertyAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WContainerAttribute, WPropertyAttribute);

public:
  WContainerAttribute() = default;
  WContainerAttribute(bool bCanAdd, bool bCanDelete, bool bCanMove)
  {
    m_bCanAdd = bCanAdd;
    m_bCanDelete = bCanDelete;
    m_bCanMove = bCanMove;
  }

  bool CanAdd() const { return m_bCanAdd; }
  bool CanDelete() const { return m_bCanDelete; }
  bool CanMove() const { return m_bCanMove; }

private:
  bool m_bCanAdd = false;
  bool m_bCanDelete = false;
  bool m_bCanMove = false;
};

/// Defines how a reference set by WFileBrowserAttribute and WAssetBrowserAttribute is treated.
///
/// A few examples to explain the flags:
/// ## Input for a mesh: **Transform | Thumbnail**
/// * The input (e.g. fbx) is obviously needed for transforming the asset.
/// * We also can't generate a thumbnail without it.
/// * But we don't need to package it with the final game as it is not used by the runtime.
///
/// ## Material on a mesh: **Thumbnail | Package**
/// * The default material on a mesh asset is not needed to transform the mesh. As only the material reference is stored in the mesh asset, any changes to the material do not affect the transform output of the mesh.
/// * It is obviously needed for the thumbnail as that is what is displayed in it.
/// * We also need to package this reference as otherwise the runtime would fail to instantiate the mesh without errors.
///
/// ## Surface on hit prefab: **Package**
/// * Transforming a surface is not affected if the prefab it spawns on impact changes. Only the reference is stored.
/// * The set prefab does not show up in the thumbnail so it is not needed.
/// * We do, however, need to package it or otherwise the runtime would fail to spawn the prefab on impact.
///
/// As a rule of thumb (also the default for each):
/// * WFileBrowserAttribute are mostly Transform and Thumbnail.
/// * WAssetBrowserAttribute are mostly Thumbnail and Package.
struct WDependencyFlags
{
  using StorageType = WUInt8;

  enum Enum
  {
    None = 0,              ///< The reference is not needed for anything in production. An example of this is editor references that are only used at edit time, e.g. a default animation clip for a skeleton.
    Thumbnail = W_BIT(0), ///< This reference is a dependency to generating a thumbnail. The material references of a mesh for example.
    Transform = W_BIT(1), ///< This reference is a dependency to transforming this asset. The input model of a mesh for example.
    Package = W_BIT(2),   ///< This reference needs to be packaged as it is used at runtime by this asset. All sounds or debris generated on impact of a surface are common examples of this.
    Default = 0
  };

  struct Bits
  {
    StorageType Thumbnail : 1;
    StorageType Transform : 1;
    StorageType Package : 1;
  };
};

W_DECLARE_FLAGS_OPERATORS(WDependencyFlags);
W_DECLARE_REFLECTABLE_TYPE(W_FOUNDATION_DLL, WDependencyFlags);

/// A property attribute that indicates that the string property should display a file browsing button.
///
/// Allows to specify the title for the browse dialog and the allowed file types.
/// Usage: W_MEMBER_PROPERTY("File", m_sFilePath)->AddAttributes(new WFileBrowserAttribute("Choose a File", "*.txt")),
class W_FOUNDATION_DLL WFileBrowserAttribute : public WTypeWidgetAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WFileBrowserAttribute, WTypeWidgetAttribute);

public:
  // Predefined common type filters
  static constexpr WStringView Meshes = "*.obj;*.fbx;*.gltf;*.glb"_wsv;
  static constexpr WStringView MeshesWithAnimations = "*.fbx;*.gltf;*.glb"_wsv;
  static constexpr WStringView ImagesLdrOnly = "*.dds;*.tga;*.png;*.jpg;*.jpeg"_wsv;
  static constexpr WStringView ImagesHdrOnly = "*.hdr;*.exr"_wsv;
  static constexpr WStringView ImagesLdrAndHdr = "*.dds;*.tga;*.png;*.jpg;*.jpeg;*.hdr;*.exr"_wsv;
  static constexpr WStringView CubemapsLdrAndHdr = "*.dds;*.hdr"_wsv;

  WFileBrowserAttribute() = default;
  WFileBrowserAttribute(WStringView sDialogTitle, WStringView sTypeFilter, WStringView sCustomAction = {}, WStringView sCreateTitle = {}, WBitflags<WDependencyFlags> depencyFlags = WDependencyFlags::Transform | WDependencyFlags::Thumbnail)
    : m_sDialogTitle(sDialogTitle)
    , m_sTypeFilter(sTypeFilter)
    , m_sCustomAction(sCustomAction)
    , m_sCreateTitle(sCreateTitle)
    , m_DependencyFlags(depencyFlags)
  {
  }

  WStringView GetDialogTitle() const { return m_sDialogTitle; }
  WStringView GetTypeFilter() const { return m_sTypeFilter; }
  WStringView GetCustomAction() const { return m_sCustomAction; }
  WStringView GetCreateTitle() const { return m_sCreateTitle; }
  WBitflags<WDependencyFlags> GetDependencyFlags() const { return m_DependencyFlags; }

private:
  WUntrackedString m_sDialogTitle;
  WUntrackedString m_sTypeFilter;
  WUntrackedString m_sCustomAction;
  WUntrackedString m_sCreateTitle;
  WBitflags<WDependencyFlags> m_DependencyFlags;
};

/// Indicates that the string property should allow to browse for an file (or programs) outside the project directories.
///
/// Allows to specify the title for the browse dialog and the allowed file types.
/// Usage: W_MEMBER_PROPERTY("File", m_sFilePath)->AddAttributes(new WFileBrowserAttribute("Choose a File", "*.exe")),
class W_FOUNDATION_DLL WExternalFileBrowserAttribute : public WTypeWidgetAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WExternalFileBrowserAttribute, WTypeWidgetAttribute);

public:
  WExternalFileBrowserAttribute() = default;
  WExternalFileBrowserAttribute(WStringView sDialogTitle, WStringView sTypeFilter)
    : m_sDialogTitle(sDialogTitle)
    , m_sTypeFilter(sTypeFilter)
  {
  }

  WStringView GetDialogTitle() const { return m_sDialogTitle; }
  WStringView GetTypeFilter() const { return m_sTypeFilter; }

private:
  WUntrackedString m_sDialogTitle;
  WUntrackedString m_sTypeFilter;
};

/// A property attribute that indicates that the string property is actually an asset reference.
///
/// Allows to specify the allowed asset types, separated with ;
/// Usage: W_MEMBER_PROPERTY("Texture", m_sTexture)->AddAttributes(new WAssetBrowserAttribute("Texture 2D;Texture 3D")),
class W_FOUNDATION_DLL WAssetBrowserAttribute : public WTypeWidgetAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WAssetBrowserAttribute, WTypeWidgetAttribute);

public:
  WAssetBrowserAttribute() = default;
  WAssetBrowserAttribute(const char* szTypeFilter, WBitflags<WDependencyFlags> depencyFlags = WDependencyFlags::Thumbnail | WDependencyFlags::Package)
    : m_DependencyFlags(depencyFlags)
  {
    SetTypeFilter(szTypeFilter);
  }

  WAssetBrowserAttribute(const char* szTypeFilter, const char* szRequiredTag, WBitflags<WDependencyFlags> depencyFlags = WDependencyFlags::Thumbnail | WDependencyFlags::Package)
    : m_DependencyFlags(depencyFlags)
  {
    SetTypeFilter(szTypeFilter);
    m_sRequiredTag = szRequiredTag;
  }

  void SetTypeFilter(const char* szTypeFilter)
  {
    WStringBuilder sTemp(";", szTypeFilter, ";");
    m_sTypeFilter = sTemp;
  }

  const char* GetTypeFilter() const { return m_sTypeFilter; }
  WBitflags<WDependencyFlags> GetDependencyFlags() const { return m_DependencyFlags; }

  const char* GetRequiredTag() const { return m_sRequiredTag; }

private:
  WUntrackedString m_sTypeFilter;
  WUntrackedString m_sRequiredTag;
  WBitflags<WDependencyFlags> m_DependencyFlags;
};

/// Can be used on integer properties to display them as enums. The valid enum values and their names may change at runtime.
///
/// See WDynamicEnum for details.
class W_FOUNDATION_DLL WDynamicEnumAttribute : public WTypeWidgetAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WDynamicEnumAttribute, WTypeWidgetAttribute);

public:
  WDynamicEnumAttribute() = default;
  WDynamicEnumAttribute(const char* szDynamicEnumName)
    : m_sDynamicEnumName(szDynamicEnumName)
  {
  }

  const char* GetDynamicEnumName() const { return m_sDynamicEnumName; }

private:
  WUntrackedString m_sDynamicEnumName;
};

/// Can be used on string properties to display them as enums. The valid enum values and their names may change at runtime.
///
/// See WDynamicStringEnum for details.
class W_FOUNDATION_DLL WDynamicStringEnumAttribute : public WTypeWidgetAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WDynamicStringEnumAttribute, WTypeWidgetAttribute);

public:
  WDynamicStringEnumAttribute() = default;
  WDynamicStringEnumAttribute(const char* szDynamicEnumName)
    : m_sDynamicEnumName(szDynamicEnumName)
  {
  }

  const char* GetDynamicEnumName() const { return m_sDynamicEnumName; }

private:
  WUntrackedString m_sDynamicEnumName;
};

/// Can be used on integer properties to display them as bitflags. The valid bitflags and their names may change at runtime.
class W_FOUNDATION_DLL WDynamicBitflagsAttribute : public WTypeWidgetAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WDynamicBitflagsAttribute, WTypeWidgetAttribute);

public:
  WDynamicBitflagsAttribute() = default;
  WDynamicBitflagsAttribute(WStringView sDynamicName)
    : m_sDynamicBitflagsName(sDynamicName)
  {
  }

  WStringView GetDynamicBitflagsName() const { return m_sDynamicBitflagsName; }

private:
  WUntrackedString m_sDynamicBitflagsName;
};

//////////////////////////////////////////////////////////////////////////

/// Base class for property attributes that activate an in-viewport manipulator gizmo.
///
/// Attach a subclass of this attribute to a reflected property or type to signal that an
/// interactive gizmo should appear in the viewport when the object is selected. The attribute
/// names up to six properties that the manipulator reads and writes. Which properties are used
/// depends on the concrete subclass.
///
/// The editor discovers this attribute through the reflection system. The ManipulatorManager
/// tracks which manipulator is active per document and drives the ManipulatorAdapterRegistry,
/// which instantiates the corresponding WManipulatorAdapter to handle the actual gizmo logic.
class W_FOUNDATION_DLL WManipulatorAttribute : public WPropertyAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WManipulatorAttribute, WPropertyAttribute);

public:
  WManipulatorAttribute(const char* szProperty1, const char* szProperty2 = nullptr, const char* szProperty3 = nullptr,
    const char* szProperty4 = nullptr, const char* szProperty5 = nullptr, const char* szProperty6 = nullptr);

  WUntrackedString m_sProperty1;
  WUntrackedString m_sProperty2;
  WUntrackedString m_sProperty3;
  WUntrackedString m_sProperty4;
  WUntrackedString m_sProperty5;
  WUntrackedString m_sProperty6;
};

//////////////////////////////////////////////////////////////////////////

class W_FOUNDATION_DLL WSphereManipulatorAttribute : public WManipulatorAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WSphereManipulatorAttribute, WManipulatorAttribute);

public:
  WSphereManipulatorAttribute();
  WSphereManipulatorAttribute(const char* szOuterRadiusProperty, const char* szInnerRadiusProperty = nullptr);

  const WUntrackedString& GetOuterRadiusProperty() const { return m_sProperty1; }
  const WUntrackedString& GetInnerRadiusProperty() const { return m_sProperty2; }
};


//////////////////////////////////////////////////////////////////////////

class W_FOUNDATION_DLL WCapsuleManipulatorAttribute : public WManipulatorAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WCapsuleManipulatorAttribute, WManipulatorAttribute);

public:
  WCapsuleManipulatorAttribute();
  WCapsuleManipulatorAttribute(const char* szHeightProperty, const char* szRadiusProperty);

  const WUntrackedString& GetLengthProperty() const { return m_sProperty1; }
  const WUntrackedString& GetRadiusProperty() const { return m_sProperty2; }
};


//////////////////////////////////////////////////////////////////////////

class W_FOUNDATION_DLL WBoxManipulatorAttribute : public WManipulatorAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WBoxManipulatorAttribute, WManipulatorAttribute);

public:
  WBoxManipulatorAttribute();
  WBoxManipulatorAttribute(const char* szSizeProperty, float fSizeScale, bool bRecenterParent, const char* szOffsetProperty = nullptr, const char* szRotationProperty = nullptr);

  bool m_bRecenterParent = false;
  float m_fSizeScale = 1.0f;

  const WUntrackedString& GetSizeProperty() const { return m_sProperty1; }
  const WUntrackedString& GetOffsetProperty() const { return m_sProperty2; }
  const WUntrackedString& GetRotationProperty() const { return m_sProperty3; }
};

//////////////////////////////////////////////////////////////////////////

class W_FOUNDATION_DLL WNonUniformBoxManipulatorAttribute : public WManipulatorAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WNonUniformBoxManipulatorAttribute, WManipulatorAttribute);

public:
  WNonUniformBoxManipulatorAttribute();
  WNonUniformBoxManipulatorAttribute(
    const char* szNegXProp, const char* szPosXProp, const char* szNegYProp, const char* szPosYProp, const char* szNegZProp, const char* szPosZProp);
  WNonUniformBoxManipulatorAttribute(const char* szSizeX, const char* szSizeY, const char* szSizeZ);

  bool HasSixAxis() const { return !m_sProperty4.IsEmpty(); }

  const WUntrackedString& GetNegXProperty() const { return m_sProperty1; }
  const WUntrackedString& GetPosXProperty() const { return m_sProperty2; }
  const WUntrackedString& GetNegYProperty() const { return m_sProperty3; }
  const WUntrackedString& GetPosYProperty() const { return m_sProperty4; }
  const WUntrackedString& GetNegZProperty() const { return m_sProperty5; }
  const WUntrackedString& GetPosZProperty() const { return m_sProperty6; }

  const WUntrackedString& GetSizeXProperty() const { return m_sProperty1; }
  const WUntrackedString& GetSizeYProperty() const { return m_sProperty2; }
  const WUntrackedString& GetSizeZProperty() const { return m_sProperty3; }
};

//////////////////////////////////////////////////////////////////////////

class W_FOUNDATION_DLL WConeLengthManipulatorAttribute : public WManipulatorAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WConeLengthManipulatorAttribute, WManipulatorAttribute);

public:
  WConeLengthManipulatorAttribute();
  WConeLengthManipulatorAttribute(const char* szRadiusProperty);

  const WUntrackedString& GetRadiusProperty() const { return m_sProperty1; }
};

//////////////////////////////////////////////////////////////////////////

class W_FOUNDATION_DLL WConeAngleManipulatorAttribute : public WManipulatorAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WConeAngleManipulatorAttribute, WManipulatorAttribute);

public:
  WConeAngleManipulatorAttribute();
  WConeAngleManipulatorAttribute(const char* szAngleProperty, float fScale = 1.0f, const char* szRadiusProperty = nullptr);

  const WUntrackedString& GetAngleProperty() const { return m_sProperty1; }
  const WUntrackedString& GetRadiusProperty() const { return m_sProperty2; }

  float m_fScale;
};

//////////////////////////////////////////////////////////////////////////

class W_FOUNDATION_DLL WTransformManipulatorAttribute : public WManipulatorAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WTransformManipulatorAttribute, WManipulatorAttribute);

public:
  WTransformManipulatorAttribute();
  WTransformManipulatorAttribute(const char* szTranslateProperty, const char* szRotateProperty = nullptr, const char* szScaleProperty = nullptr, const char* szOffsetTranslation = nullptr, const char* szOffsetRotation = nullptr);

  const WUntrackedString& GetTranslateProperty() const { return m_sProperty1; }
  const WUntrackedString& GetRotateProperty() const { return m_sProperty2; }
  const WUntrackedString& GetScaleProperty() const { return m_sProperty3; }
  const WUntrackedString& GetGetOffsetTranslationProperty() const { return m_sProperty4; }
  const WUntrackedString& GetGetOffsetRotationProperty() const { return m_sProperty5; }
};

//////////////////////////////////////////////////////////////////////////

class W_FOUNDATION_DLL WBoneManipulatorAttribute : public WManipulatorAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WBoneManipulatorAttribute, WManipulatorAttribute);

public:
  WBoneManipulatorAttribute();
  WBoneManipulatorAttribute(const char* szTransformProperty, const char* szBindTo);

  const WUntrackedString& GetTransformProperty() const { return m_sProperty1; }
};

//////////////////////////////////////////////////////////////////////////

class W_FOUNDATION_DLL WSplineManipulatorAttribute : public WManipulatorAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WSplineManipulatorAttribute, WManipulatorAttribute);

public:
  WSplineManipulatorAttribute();
  WSplineManipulatorAttribute(const char* szBindTo, const char* szClosedProperty);

  const WUntrackedString& GetBindTo() const { return m_sProperty1; }
  const WUntrackedString& GetClosedProperty() const { return m_sProperty2; }
};

//////////////////////////////////////////////////////////////////////////

class W_FOUNDATION_DLL WSplineTangentManipulatorAttribute : public WManipulatorAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WSplineTangentManipulatorAttribute, WManipulatorAttribute);

public:
  WSplineTangentManipulatorAttribute();
  WSplineTangentManipulatorAttribute(const char* szTangentMode, const char* szCustomTangent);

  const WUntrackedString& GetTangentModeProperty() const { return m_sProperty1; }
  const WUntrackedString& GetCustomTangentProperty() const { return m_sProperty2; }
};

//////////////////////////////////////////////////////////////////////////

struct WVisualizerAnchor
{
  using StorageType = WUInt8;

  enum Enum
  {
    Center = 0,
    PosX = W_BIT(0),
    NegX = W_BIT(1),
    PosY = W_BIT(2),
    NegY = W_BIT(3),
    PosZ = W_BIT(4),
    NegZ = W_BIT(5),

    Default = Center
  };

  struct Bits
  {
    StorageType PosX : 1;
    StorageType NegX : 1;
    StorageType PosY : 1;
    StorageType NegY : 1;
    StorageType PosZ : 1;
    StorageType NegZ : 1;
  };
};

W_DECLARE_FLAGS_OPERATORS(WVisualizerAnchor);
W_DECLARE_REFLECTABLE_TYPE(W_FOUNDATION_DLL, WVisualizerAnchor);

//////////////////////////////////////////////////////////////////////////

class W_FOUNDATION_DLL WVisualizerAttribute : public WPropertyAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WVisualizerAttribute, WPropertyAttribute);

public:
  WVisualizerAttribute(const char* szProperty1, const char* szProperty2 = nullptr, const char* szProperty3 = nullptr,
    const char* szProperty4 = nullptr, const char* szProperty5 = nullptr, const char* szProperty6 = nullptr);

  WUntrackedString m_sProperty1;
  WUntrackedString m_sProperty2;
  WUntrackedString m_sProperty3;
  WUntrackedString m_sProperty4;
  WUntrackedString m_sProperty5;
  WUntrackedString m_sProperty6;
  WBitflags<WVisualizerAnchor> m_Anchor;
};

//////////////////////////////////////////////////////////////////////////

class W_FOUNDATION_DLL WBoxVisualizerAttribute : public WVisualizerAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WBoxVisualizerAttribute, WVisualizerAttribute);

public:
  WBoxVisualizerAttribute();
  WBoxVisualizerAttribute(const char* szSizeProperty, float fSizeScale = 1.0f, const WColor& fixedColor = WColorScheme::LightUI(WColorScheme::Grape), const char* szColorProperty = nullptr, WBitflags<WVisualizerAnchor> anchor = WVisualizerAnchor::Center, WVec3 vOffsetOrScale = WVec3::MakeZero(), const char* szOffsetProperty = nullptr, const char* szRotationProperty = nullptr);

  const WUntrackedString& GetSizeProperty() const { return m_sProperty1; }
  const WUntrackedString& GetColorProperty() const { return m_sProperty2; }
  const WUntrackedString& GetOffsetProperty() const { return m_sProperty3; }
  const WUntrackedString& GetRotationProperty() const { return m_sProperty4; }

  float m_fSizeScale = 1.0f;
  WColor m_Color;
  WVec3 m_vOffsetOrScale;
};

//////////////////////////////////////////////////////////////////////////

class W_FOUNDATION_DLL WSphereVisualizerAttribute : public WVisualizerAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WSphereVisualizerAttribute, WVisualizerAttribute);

public:
  WSphereVisualizerAttribute();
  WSphereVisualizerAttribute(const char* szRadiusProperty, const WColor& fixedColor = WColorScheme::LightUI(WColorScheme::Grape), const char* szColorProperty = nullptr, WBitflags<WVisualizerAnchor> anchor = WVisualizerAnchor::Center, WVec3 vOffsetOrScale = WVec3::MakeZero(), const char* szOffsetProperty = nullptr);

  const WUntrackedString& GetRadiusProperty() const { return m_sProperty1; }
  const WUntrackedString& GetColorProperty() const { return m_sProperty2; }
  const WUntrackedString& GetOffsetProperty() const { return m_sProperty3; }

  WColor m_Color;
  WVec3 m_vOffsetOrScale;
};


//////////////////////////////////////////////////////////////////////////

class W_FOUNDATION_DLL WCapsuleVisualizerAttribute : public WVisualizerAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WCapsuleVisualizerAttribute, WVisualizerAttribute);

public:
  WCapsuleVisualizerAttribute();
  WCapsuleVisualizerAttribute(const char* szHeightProperty, const char* szRadiusProperty, const WColor& fixedColor = WColorScheme::LightUI(WColorScheme::Grape), const char* szColorProperty = nullptr, WBitflags<WVisualizerAnchor> anchor = WVisualizerAnchor::Center);

  const WUntrackedString& GetHeightProperty() const { return m_sProperty1; }
  const WUntrackedString& GetRadiusProperty() const { return m_sProperty2; }
  const WUntrackedString& GetColorProperty() const { return m_sProperty3; }

  WColor m_Color;
};

//////////////////////////////////////////////////////////////////////////

class W_FOUNDATION_DLL WCylinderVisualizerAttribute : public WVisualizerAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WCylinderVisualizerAttribute, WVisualizerAttribute);

public:
  WCylinderVisualizerAttribute();
  WCylinderVisualizerAttribute(WEnum<WBasisAxis> axis, const char* szHeightProperty, const char* szRadiusProperty, const WColor& fixedColor = WColorScheme::LightUI(WColorScheme::Grape), const char* szColorProperty = nullptr, WBitflags<WVisualizerAnchor> anchor = WVisualizerAnchor::Center, WVec3 vOffsetOrScale = WVec3::MakeZero(), const char* szOffsetProperty = nullptr);
  WCylinderVisualizerAttribute(const char* szAxisProperty, const char* szHeightProperty, const char* szRadiusProperty, const WColor& fixedColor = WColorScheme::LightUI(WColorScheme::Grape), const char* szColorProperty = nullptr, WBitflags<WVisualizerAnchor> anchor = WVisualizerAnchor::Center, WVec3 vOffsetOrScale = WVec3::MakeZero(), const char* szOffsetProperty = nullptr);

  const WUntrackedString& GetAxisProperty() const { return m_sProperty5; }
  const WUntrackedString& GetHeightProperty() const { return m_sProperty1; }
  const WUntrackedString& GetRadiusProperty() const { return m_sProperty2; }
  const WUntrackedString& GetColorProperty() const { return m_sProperty3; }
  const WUntrackedString& GetOffsetProperty() const { return m_sProperty4; }

  WColor m_Color;
  WVec3 m_vOffsetOrScale;
  WEnum<WBasisAxis> m_Axis;
};

//////////////////////////////////////////////////////////////////////////

class W_FOUNDATION_DLL WDirectionVisualizerAttribute : public WVisualizerAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WDirectionVisualizerAttribute, WVisualizerAttribute);

public:
  WDirectionVisualizerAttribute();
  WDirectionVisualizerAttribute(WEnum<WBasisAxis> axis, float fScale, const WColor& fixedColor = WColorScheme::LightUI(WColorScheme::Grape), const char* szColorProperty = nullptr, const char* szLengthProperty = nullptr);
  WDirectionVisualizerAttribute(const char* szAxisProperty, float fScale, const WColor& fixedColor = WColorScheme::LightUI(WColorScheme::Grape), const char* szColorProperty = nullptr, const char* szLengthProperty = nullptr);

  const WUntrackedString& GetColorProperty() const { return m_sProperty1; }
  const WUntrackedString& GetLengthProperty() const { return m_sProperty2; }
  const WUntrackedString& GetAxisProperty() const { return m_sProperty3; }

  WEnum<WBasisAxis> m_Axis;
  WColor m_Color;
  float m_fScale;
};

//////////////////////////////////////////////////////////////////////////

class W_FOUNDATION_DLL WConeVisualizerAttribute : public WVisualizerAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WConeVisualizerAttribute, WVisualizerAttribute);

public:
  WConeVisualizerAttribute();

  /// Attribute to add on an RTTI type to add a cone visualizer for specific properties.
  ///
  /// szRadiusProperty may be nullptr, in which case it is assumed to be 1
  /// fScale will be multiplied with value of szRadiusProperty to determine the size of the cone
  /// szColorProperty may be nullptr. In this case it is ignored and fixedColor is used instead.
  /// fixedColor is ignored if szColorProperty is valid.
  WConeVisualizerAttribute(WEnum<WBasisAxis> axis, const char* szAngleProperty, float fScale, const char* szRadiusProperty, const WColor& fixedColor = WColorScheme::LightUI(WColorScheme::Grape), const char* szColorProperty = nullptr);

  const WUntrackedString& GetAngleProperty() const { return m_sProperty1; }
  const WUntrackedString& GetRadiusProperty() const { return m_sProperty2; }
  const WUntrackedString& GetColorProperty() const { return m_sProperty3; }

  WEnum<WBasisAxis> m_Axis;
  WColor m_Color;
  float m_fScale;
};

//////////////////////////////////////////////////////////////////////////

class W_FOUNDATION_DLL WCameraVisualizerAttribute : public WVisualizerAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WCameraVisualizerAttribute, WVisualizerAttribute);

public:
  WCameraVisualizerAttribute();

  /// Attribute to add on an RTTI type to add a camera cone visualizer.
  WCameraVisualizerAttribute(const char* szModeProperty, const char* szFovProperty, const char* szOrthoDimProperty, const char* szNearPlaneProperty, const char* szFarPlaneProperty);

  const WUntrackedString& GetModeProperty() const { return m_sProperty1; }
  const WUntrackedString& GetFovProperty() const { return m_sProperty2; }
  const WUntrackedString& GetOrthoDimProperty() const { return m_sProperty3; }
  const WUntrackedString& GetNearPlaneProperty() const { return m_sProperty4; }
  const WUntrackedString& GetFarPlaneProperty() const { return m_sProperty5; }
};

//////////////////////////////////////////////////////////////////////////

/// Visualizes a Vec3 property as a 3D cross marker at the specified position in the viewport.
///
/// The position is interpreted as a local-space offset from the object's origin.
/// \c szColorProperty may be nullptr, in which case \c fixedColor is used.
class W_FOUNDATION_DLL WPositionVisualizerAttribute : public WVisualizerAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WPositionVisualizerAttribute, WVisualizerAttribute);

public:
  WPositionVisualizerAttribute();
  WPositionVisualizerAttribute(const char* szPositionProperty, float fSizeScale = 0.1f, const WColor& fixedColor = WColorScheme::LightUI(WColorScheme::Grape), const char* szColorProperty = nullptr);

  const WUntrackedString& GetPositionProperty() const { return m_sProperty1; }
  const WUntrackedString& GetColorProperty() const { return m_sProperty2; }

  float m_fSizeScale = 0.1f;
  WColor m_Color;
};

//////////////////////////////////////////////////////////////////////////

// Implementation moved here as it requires WPropertyAttribute to be fully defined.
template <typename Type>
const Type* WRTTI::GetAttributeByType() const
{
  for (const auto* pAttr : m_Attributes)
  {
    if (pAttr->GetDynamicRTTI()->IsDerivedFrom<Type>())
      return static_cast<const Type*>(pAttr);
  }
  if (GetParentType() != nullptr)
    return GetParentType()->GetAttributeByType<Type>();
  else
    return nullptr;
}

template <typename Type>
const Type* WAbstractProperty::GetAttributeByType() const
{
  for (const auto* pAttr : m_Attributes)
  {
    if (pAttr->GetDynamicRTTI()->IsDerivedFrom<Type>())
      return static_cast<const Type*>(pAttr);
  }
  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

/// A property attribute that specifies the max size of an array. If it is reached, no further elemets are allowed to be added.
class W_FOUNDATION_DLL WMaxArraySizeAttribute : public WPropertyAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WMaxArraySizeAttribute, WPropertyAttribute);

public:
  WMaxArraySizeAttribute() = default;
  WMaxArraySizeAttribute(WUInt32 uiMaxSize) { m_uiMaxSize = uiMaxSize; }

  const WUInt32& GetMaxSize() const { return m_uiMaxSize; }

private:
  WUInt32 m_uiMaxSize = 0;
};

//////////////////////////////////////////////////////////////////////////

/// If this attribute is set, the UI is encouraged to prevent the user from creating duplicates of the same thing.
///
/// For arrays of objects this means that multiple objects of the same type are not allowed.
class W_FOUNDATION_DLL WPreventDuplicatesAttribute : public WPropertyAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WPreventDuplicatesAttribute, WPropertyAttribute);

public:
  WPreventDuplicatesAttribute() = default;
};

//////////////////////////////////////////////////////////////////////////

/// Attribute for types that should not be exposed to the scripting framework
class W_FOUNDATION_DLL WExcludeFromScript : public WPropertyAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WExcludeFromScript, WPropertyAttribute);
};

/// Attribute to mark a function up to be exposed to the scripting system. Arguments specify the names of the function parameters.
class W_FOUNDATION_DLL WScriptableFunctionAttribute : public WPropertyAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WScriptableFunctionAttribute, WPropertyAttribute);

  enum ArgType : WUInt8
  {
    In,
    Out,
    Inout
  };

  WScriptableFunctionAttribute(ArgType argType1 = In, const char* szArg1 = nullptr, ArgType argType2 = In, const char* szArg2 = nullptr,
    ArgType argType3 = In, const char* szArg3 = nullptr, ArgType argType4 = In, const char* szArg4 = nullptr, ArgType argType5 = In,
    const char* szArg5 = nullptr, ArgType argType6 = In, const char* szArg6 = nullptr, ArgType argType7 = In, const char* szArg7 = nullptr, ArgType argType8 = In, const char* szArg8 = nullptr, ArgType argType9 = In, const char* szArg9 = nullptr, ArgType argType10 = In, const char* szArg10 = nullptr, ArgType argType11 = In, const char* szArg11 = nullptr, ArgType argType12 = In, const char* szArg12 = nullptr);

  WUInt32 GetArgumentCount() const { return m_ArgNames.GetCount(); }
  const char* GetArgumentName(WUInt32 uiIndex) const { return m_ArgNames[uiIndex]; }

  ArgType GetArgumentType(WUInt32 uiIndex) const { return static_cast<ArgType>(m_ArgTypes[uiIndex]); }

private:
  WHybridArray<WUntrackedString, 6> m_ArgNames;
  WHybridArray<WUInt8, 6> m_ArgTypes;
};

/// Wrapper Attribute to add an attribute to a function argument
class W_FOUNDATION_DLL WFunctionArgumentAttributes : public WPropertyAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WFunctionArgumentAttributes, WPropertyAttribute);

  WFunctionArgumentAttributes() = default;
  WFunctionArgumentAttributes(WUInt32 uiArgIndex, const WPropertyAttribute* pAttribute1, const WPropertyAttribute* pAttribute2 = nullptr, const WPropertyAttribute* pAttribute3 = nullptr, const WPropertyAttribute* pAttribute4 = nullptr);
  ~WFunctionArgumentAttributes();

  WUInt32 GetArgumentIndex() const { return m_uiArgIndex; }
  WArrayPtr<const WPropertyAttribute* const> GetArgumentAttributes() const { return m_ArgAttributes; }

private:
  WUInt32 m_uiArgIndex = 0;
  // Not pretty, but the values in the array are either created using 'new' when using this class as a reflection decoration, or created using 'W_DEFAULT_NEW' when serialized and sent to the editor so in the dtor we need to know where these came from.
  bool m_bUsesGlobalNew = false;
  WHybridArray<const WPropertyAttribute*, 4> m_ArgAttributes;
};

/// Used to mark an array or (unsigned)int property as source for dynamic pin generation on nodes
class W_FOUNDATION_DLL WDynamicPinAttribute : public WPropertyAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WDynamicPinAttribute, WPropertyAttribute);

public:
  WDynamicPinAttribute() = default;
  WDynamicPinAttribute(const char* szProperty);

  const WUntrackedString& GetProperty() const { return m_sProperty; }

private:
  WUntrackedString m_sProperty;
};

//////////////////////////////////////////////////////////////////////////

/// Used to mark that a component provides functionality that is executed with a long operation in the editor.
///
/// \a szOpTypeName must be the class name of a class derived from WLongOpProxy.
/// Once a component is added to a scene with this attribute, the named long op will appear in the UI and can be executed.
///
/// The automatic registration is done by WLongOpsAdapter
class W_FOUNDATION_DLL WLongOpAttribute : public WPropertyAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WLongOpAttribute, WPropertyAttribute);

public:
  WLongOpAttribute() = default;
  WLongOpAttribute(const char* szOpTypeName)
    : m_sOpTypeName(szOpTypeName)
  {
  }

  WUntrackedString m_sOpTypeName;
};

//////////////////////////////////////////////////////////////////////////

/// A property attribute that indicates that the string property is actually a game object reference.
class W_FOUNDATION_DLL WGameObjectReferenceAttribute : public WTypeWidgetAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WGameObjectReferenceAttribute, WTypeWidgetAttribute);

public:
  WGameObjectReferenceAttribute() = default;
};

//////////////////////////////////////////////////////////////////////////

/// Displays the value range as an image, allowing users to pick a value like on a slider.
///
/// This attribute always has to be combined with an WClampValueAttribute to define the min and max value range.
/// The constructor takes the name of an image generator. The generator is used to build the QImage used for the slider background.
///
/// Image generators are registered through WQtImageSliderWidget::s_ImageGenerators. Search the codebase for that variable
/// to determine which types of image generators are available.
/// You can register custom generators as well.
class W_FOUNDATION_DLL WImageSliderUiAttribute : public WTypeWidgetAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WImageSliderUiAttribute, WTypeWidgetAttribute);

public:
  WImageSliderUiAttribute() = default;
  WImageSliderUiAttribute(const char* szImageGenerator)
  {
    m_sImageGenerator = szImageGenerator;
  }

  WUntrackedString m_sImageGenerator;
};

//////////////////////////////////////////////////////////////////////////

/// Attribute that turns a string property into a selector for an RTTI type.
///
/// The base type defines what types to display.
/// For example if "WComponent" is passed in, only types derived from WComponent are listed.
class W_FOUNDATION_DLL WRttiTypeStringAttribute : public WTypeWidgetAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WRttiTypeStringAttribute, WTypeWidgetAttribute);

public:
  WRttiTypeStringAttribute() = default;
  WRttiTypeStringAttribute(const char* szBaseType)
    : m_sBaseType(szBaseType)
  {
  }

  const char* GetBaseType() const { return m_sBaseType; }

private:
  WUntrackedString m_sBaseType;
};

//////////////////////////////////////////////////////////////////////////

/// Marks a component type as requiring child-order synchronization from the editor.
/// The editor uses this attribute to identify components that need to receive an ordered
/// list of their parent game object's children via the reflected function SetChildOrder.
class W_FOUNDATION_DLL WSyncChildOrderAttribute : public WPropertyAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WSyncChildOrderAttribute, WPropertyAttribute);
};
