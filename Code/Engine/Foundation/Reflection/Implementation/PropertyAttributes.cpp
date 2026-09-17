#include <Foundation/FoundationPCH.h>

#include <Foundation/Reflection/Reflection.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WPropertyAttribute, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WReadOnlyAttribute, 1, WRTTIDefaultAllocator<WReadOnlyAttribute>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WHiddenAttribute, 1, WRTTIDefaultAllocator<WHiddenAttribute>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRequiredAttribute, 1, WRTTIDefaultAllocator<WRequiredAttribute>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTemporaryAttribute, 1, WRTTIDefaultAllocator<WTemporaryAttribute>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_BITFLAGS(WDependencyFlags, 1)
W_BITFLAGS_CONSTANTS(WDependencyFlags::Package, WDependencyFlags::Thumbnail, WDependencyFlags::Transform)
W_END_STATIC_REFLECTED_BITFLAGS;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WShapeIconAlwaysVisibleAttribute, 1, WRTTIDefaultAllocator<WShapeIconAlwaysVisibleAttribute>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCategoryAttribute, 1, WRTTIDefaultAllocator<WCategoryAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Category", m_sCategory),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const char*),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WInDevelopmentAttribute, 1, WRTTIDefaultAllocator<WInDevelopmentAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Phase", m_Phase),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(WInt32),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;


const char* WInDevelopmentAttribute::GetString() const
{
  switch (m_Phase)
  {
  case Phase::Alpha:
    return "ALPHA";

  case Phase::Beta:
    return "BETA";

    W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return "";
}

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTitleAttribute, 1, WRTTIDefaultAllocator<WTitleAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Title", m_sTitle),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const char*),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WColorAttribute, 1, WRTTIDefaultAllocator<WColorAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Color", m_Color),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(WColor),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WExposeColorAlphaAttribute, 1, WRTTIDefaultAllocator<WExposeColorAlphaAttribute>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSuffixAttribute, 1, WRTTIDefaultAllocator<WSuffixAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Suffix", m_sSuffix),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const char*),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMinValueTextAttribute, 1, WRTTIDefaultAllocator<WMinValueTextAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Text", m_sText),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const char*),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDefaultValueAttribute, 1, WRTTIDefaultAllocator<WDefaultValueAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Value", m_Value),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const WVariant&),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WImageSliderUiAttribute, 1, WRTTIDefaultAllocator<WImageSliderUiAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ImageGenerator", m_sImageGenerator),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const char*),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WClampValueAttribute, 1, WRTTIDefaultAllocator<WClampValueAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Min", m_MinValue),
    W_MEMBER_PROPERTY("Max", m_MaxValue),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const WVariant&, const WVariant&),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGroupAttribute, 1, WRTTIDefaultAllocator<WGroupAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Group", m_sGroup),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const char*, float),
    W_CONSTRUCTOR_PROPERTY(const char*, const char*, float),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;

WGroupAttribute::WGroupAttribute()
= default;

WGroupAttribute::WGroupAttribute(const char* szGroup, float fOrder)
  : m_sGroup(szGroup)
  , m_fOrder(fOrder)
{
}

WGroupAttribute::WGroupAttribute(const char* szGroup, const char* szIconName, float fOrder)
  : m_sGroup(szGroup)
  , m_sIconName(szIconName)
  , m_fOrder(fOrder)
{
}

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTypeWidgetAttribute, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WContainerWidgetAttribute, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTagSetWidgetAttribute, 1, WRTTIDefaultAllocator<WTagSetWidgetAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Filter", m_sTagFilter),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const char*),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WNoTemporaryTransactionsAttribute, 1, WRTTIDefaultAllocator<WNoTemporaryTransactionsAttribute>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WExposedParametersAttribute, 1, WRTTIDefaultAllocator<WExposedParametersAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ParametersSource", m_sParametersSource),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const char*),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDynamicDefaultValueAttribute, 1, WRTTIDefaultAllocator<WDynamicDefaultValueAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ClassSource", m_sClassSource),
    W_MEMBER_PROPERTY("ClassType", m_sClassType),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const char*, const char*),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WContainerAttribute, 1, WRTTIDefaultAllocator<WContainerAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("CanAdd", m_bCanAdd),
    W_MEMBER_PROPERTY("CanDelete", m_bCanDelete),
    W_MEMBER_PROPERTY("CanMove", m_bCanMove),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(bool, bool, bool),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WFileBrowserAttribute, 1, WRTTIDefaultAllocator<WFileBrowserAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Title", m_sDialogTitle),
    W_MEMBER_PROPERTY("Filter", m_sTypeFilter),
    W_MEMBER_PROPERTY("CustomAction", m_sCustomAction),
    W_BITFLAGS_MEMBER_PROPERTY("DependencyFlags", WDependencyFlags, m_DependencyFlags),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(WStringView, WStringView),
    W_CONSTRUCTOR_PROPERTY(WStringView, WStringView, WStringView),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WExternalFileBrowserAttribute, 1, WRTTIDefaultAllocator<WExternalFileBrowserAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Title", m_sDialogTitle),
    W_MEMBER_PROPERTY("Filter", m_sTypeFilter),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(WStringView, WStringView),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAssetBrowserAttribute, 1, WRTTIDefaultAllocator<WAssetBrowserAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Filter", m_sTypeFilter),
    W_MEMBER_PROPERTY("RequiredTag", m_sRequiredTag),
    W_BITFLAGS_MEMBER_PROPERTY("DependencyFlags", WDependencyFlags, m_DependencyFlags),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const char*),
    W_CONSTRUCTOR_PROPERTY(const char*, WBitflags<WDependencyFlags>),
    W_CONSTRUCTOR_PROPERTY(const char*, const char*),
    W_CONSTRUCTOR_PROPERTY(const char*, const char*, WBitflags<WDependencyFlags>),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDynamicEnumAttribute, 1, WRTTIDefaultAllocator<WDynamicEnumAttribute>)
{
  W_BEGIN_PROPERTIES
  {
   W_MEMBER_PROPERTY("DynamicEnum", m_sDynamicEnumName),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
   W_CONSTRUCTOR_PROPERTY(const char*),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDynamicStringEnumAttribute, 1, WRTTIDefaultAllocator<WDynamicStringEnumAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("DynamicEnum", m_sDynamicEnumName),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const char*),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDynamicBitflagsAttribute, 1, WRTTIDefaultAllocator<WDynamicBitflagsAttribute>)
{
  W_BEGIN_PROPERTIES
  {
   W_MEMBER_PROPERTY("DynamicBitflags", m_sDynamicBitflagsName),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
   W_CONSTRUCTOR_PROPERTY(WStringView),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WManipulatorAttribute, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Property1", m_sProperty1),
    W_MEMBER_PROPERTY("Property2", m_sProperty2),
    W_MEMBER_PROPERTY("Property3", m_sProperty3),
    W_MEMBER_PROPERTY("Property4", m_sProperty4),
    W_MEMBER_PROPERTY("Property5", m_sProperty5),
    W_MEMBER_PROPERTY("Property6", m_sProperty6),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WManipulatorAttribute::WManipulatorAttribute(const char* szProperty1, const char* szProperty2 /*= nullptr*/, const char* szProperty3 /*= nullptr*/,
  const char* szProperty4 /*= nullptr*/, const char* szProperty5 /*= nullptr*/, const char* szProperty6 /*= nullptr*/)
  : m_sProperty1(szProperty1)
  , m_sProperty2(szProperty2)
  , m_sProperty3(szProperty3)
  , m_sProperty4(szProperty4)
  , m_sProperty5(szProperty5)
  , m_sProperty6(szProperty6)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSphereManipulatorAttribute, 1, WRTTIDefaultAllocator<WSphereManipulatorAttribute>)
{
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const char*),
    W_CONSTRUCTOR_PROPERTY(const char*, const char*),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WSphereManipulatorAttribute::WSphereManipulatorAttribute()
  : WManipulatorAttribute(nullptr)
{
}

WSphereManipulatorAttribute::WSphereManipulatorAttribute(const char* szOuterRadius, const char* szInnerRadius)
  : WManipulatorAttribute(szOuterRadius, szInnerRadius)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCapsuleManipulatorAttribute, 1, WRTTIDefaultAllocator<WCapsuleManipulatorAttribute>)
{
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const char*, const char*),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WCapsuleManipulatorAttribute::WCapsuleManipulatorAttribute()
  : WManipulatorAttribute(nullptr)
{
}

WCapsuleManipulatorAttribute::WCapsuleManipulatorAttribute(const char* szLength, const char* szRadius)
  : WManipulatorAttribute(szLength, szRadius)
{
}


//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WBoxManipulatorAttribute, 1, WRTTIDefaultAllocator<WBoxManipulatorAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("scale", m_fSizeScale),
    W_MEMBER_PROPERTY("recenter", m_bRecenterParent),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const char*, float, bool),
    W_CONSTRUCTOR_PROPERTY(const char*, float, bool, const char*),
    W_CONSTRUCTOR_PROPERTY(const char*, float, bool, const char*, const char*),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WBoxManipulatorAttribute::WBoxManipulatorAttribute()
  : WManipulatorAttribute(nullptr)
{
}

WBoxManipulatorAttribute::WBoxManipulatorAttribute(const char* szSizeProperty, float fSizeScale, bool bRecenterParent, const char* szOffsetProperty, const char* szRotationProperty)
  : WManipulatorAttribute(szSizeProperty, szOffsetProperty, szRotationProperty)
{
  m_bRecenterParent = bRecenterParent;
  m_fSizeScale = fSizeScale;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WNonUniformBoxManipulatorAttribute, 1, WRTTIDefaultAllocator<WNonUniformBoxManipulatorAttribute>)
{
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*),
    W_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const char*, const char*, const char*),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WNonUniformBoxManipulatorAttribute::WNonUniformBoxManipulatorAttribute()
  : WManipulatorAttribute(nullptr)
{
}

WNonUniformBoxManipulatorAttribute::WNonUniformBoxManipulatorAttribute(
  const char* szNegXProp, const char* szPosXProp, const char* szNegYProp, const char* szPosYProp, const char* szNegZProp, const char* szPosZProp)
  : WManipulatorAttribute(szNegXProp, szPosXProp, szNegYProp, szPosYProp, szNegZProp, szPosZProp)
{
}

WNonUniformBoxManipulatorAttribute::WNonUniformBoxManipulatorAttribute(const char* szSizeX, const char* szSizeY, const char* szSizeZ)
  : WManipulatorAttribute(szSizeX, szSizeY, szSizeZ)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WConeLengthManipulatorAttribute, 1, WRTTIDefaultAllocator<WConeLengthManipulatorAttribute>)
{
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const char*),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WConeLengthManipulatorAttribute::WConeLengthManipulatorAttribute()
  : WManipulatorAttribute(nullptr)
{
}

WConeLengthManipulatorAttribute::WConeLengthManipulatorAttribute(const char* szRadiusProperty)
  : WManipulatorAttribute(szRadiusProperty)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WConeAngleManipulatorAttribute, 1, WRTTIDefaultAllocator<WConeAngleManipulatorAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("scale", m_fScale),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const char*),
    W_CONSTRUCTOR_PROPERTY(const char*, float),
    W_CONSTRUCTOR_PROPERTY(const char*, float, const char*),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WConeAngleManipulatorAttribute::WConeAngleManipulatorAttribute()
  : WManipulatorAttribute(nullptr)
{
  m_fScale = 1.0f;
}

WConeAngleManipulatorAttribute::WConeAngleManipulatorAttribute(const char* szAngleProperty, float fScale, const char* szRadiusProperty)
  : WManipulatorAttribute(szAngleProperty, szRadiusProperty)
{
  m_fScale = fScale;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTransformManipulatorAttribute, 1, WRTTIDefaultAllocator<WTransformManipulatorAttribute>)
{
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const char*, const char*),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WTransformManipulatorAttribute::WTransformManipulatorAttribute()
  : WManipulatorAttribute(nullptr)
{
}

WTransformManipulatorAttribute::WTransformManipulatorAttribute(
  const char* szTranslateProperty, const char* szRotateProperty, const char* szScaleProperty, const char* szOffsetTranslation, const char* szOffsetRotation)
  : WManipulatorAttribute(szTranslateProperty, szRotateProperty, szScaleProperty, szOffsetTranslation, szOffsetRotation)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WBoneManipulatorAttribute, 1, WRTTIDefaultAllocator<WBoneManipulatorAttribute>)
{
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const char*, const char*),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WBoneManipulatorAttribute::WBoneManipulatorAttribute()
  : WManipulatorAttribute(nullptr)
{
}

WBoneManipulatorAttribute::WBoneManipulatorAttribute(const char* szTransformProperty, const char* szBindTo)
  : WManipulatorAttribute(szTransformProperty, szBindTo)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSplineManipulatorAttribute, 1, WRTTIDefaultAllocator<WSplineManipulatorAttribute>)
{
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const char*, const char*),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WSplineManipulatorAttribute::WSplineManipulatorAttribute()
  : WManipulatorAttribute(nullptr)
{
}

WSplineManipulatorAttribute::WSplineManipulatorAttribute(const char* szBindTo, const char* szClosedProperty)
  : WManipulatorAttribute(szBindTo, szClosedProperty)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSplineTangentManipulatorAttribute, 1, WRTTIDefaultAllocator<WSplineTangentManipulatorAttribute>)
{
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const char*, const char*),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WSplineTangentManipulatorAttribute::WSplineTangentManipulatorAttribute()
  : WManipulatorAttribute(nullptr)
{
}

WSplineTangentManipulatorAttribute::WSplineTangentManipulatorAttribute(const char* szTangentMode, const char* szCustomTangent)
  : WManipulatorAttribute(szTangentMode, szCustomTangent)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_BITFLAGS(WVisualizerAnchor, 1)
W_BITFLAGS_CONSTANTS(WVisualizerAnchor::Center, WVisualizerAnchor::PosX, WVisualizerAnchor::NegX, WVisualizerAnchor::PosY, WVisualizerAnchor::NegY, WVisualizerAnchor::PosZ, WVisualizerAnchor::NegZ)
W_END_STATIC_REFLECTED_BITFLAGS;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WVisualizerAttribute, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Property1", m_sProperty1),
    W_MEMBER_PROPERTY("Property2", m_sProperty2),
    W_MEMBER_PROPERTY("Property3", m_sProperty3),
    W_MEMBER_PROPERTY("Property4", m_sProperty4),
    W_MEMBER_PROPERTY("Property5", m_sProperty5),
    W_MEMBER_PROPERTY("Property6", m_sProperty6),
    W_BITFLAGS_MEMBER_PROPERTY("Anchor", WVisualizerAnchor, m_Anchor),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WVisualizerAttribute::WVisualizerAttribute(const char* szProperty1, const char* szProperty2 /*= nullptr*/, const char* szProperty3 /*= nullptr*/,
  const char* szProperty4 /*= nullptr*/, const char* szProperty5 /*= nullptr*/, const char* szProperty6 /*= nullptr*/)
  : m_sProperty1(szProperty1)
  , m_sProperty2(szProperty2)
  , m_sProperty3(szProperty3)
  , m_sProperty4(szProperty4)
  , m_sProperty5(szProperty5)
  , m_sProperty6(szProperty6)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WBoxVisualizerAttribute, 1, WRTTIDefaultAllocator<WBoxVisualizerAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Color", m_Color),
    W_MEMBER_PROPERTY("OffsetOrScale", m_vOffsetOrScale),
    W_MEMBER_PROPERTY("SizeScale", m_fSizeScale),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const char*, float, const WColor&, const char*, WBitflags<WVisualizerAnchor>, WVec3, const char*, const char*),
    W_CONSTRUCTOR_PROPERTY(const char*, float, const WColor&, const char*, WBitflags<WVisualizerAnchor>, WVec3, const char*),
    W_CONSTRUCTOR_PROPERTY(const char*, float, const WColor&, const char*, WBitflags<WVisualizerAnchor>, WVec3),
    W_CONSTRUCTOR_PROPERTY(const char*, float, const WColor&, const char*, WBitflags<WVisualizerAnchor>),
    W_CONSTRUCTOR_PROPERTY(const char*, float, const WColor&, const char*),
    W_CONSTRUCTOR_PROPERTY(const char*, float, const WColor&),
    W_CONSTRUCTOR_PROPERTY(const char*, float),
    W_CONSTRUCTOR_PROPERTY(const char*),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WBoxVisualizerAttribute::WBoxVisualizerAttribute()
  : WVisualizerAttribute(nullptr)
{
}

WBoxVisualizerAttribute::WBoxVisualizerAttribute(const char* szSizeProperty, float fSizeScale, const WColor& fixedColor /*= WColorScheme::LightUI(WColorScheme::Grape)*/, const char* szColorProperty /*= nullptr*/, WBitflags<WVisualizerAnchor> anchor /*= WVisualizerAnchor::Center*/, WVec3 vOffsetOrScale /*= WVec3::MakeZero*/, const char* szOffsetProperty /*= nullptr*/, const char* szRotationProperty /*= nullptr*/)
  : WVisualizerAttribute(szSizeProperty, szColorProperty, szOffsetProperty, szRotationProperty)
  , m_Color(fixedColor)
  , m_vOffsetOrScale(vOffsetOrScale)
{
  m_Anchor = anchor;
  m_fSizeScale = fSizeScale;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSphereVisualizerAttribute, 1, WRTTIDefaultAllocator<WSphereVisualizerAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Color", m_Color),
    W_MEMBER_PROPERTY("OffsetOrScale", m_vOffsetOrScale),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const char*, const WColor&, const char*, WBitflags<WVisualizerAnchor>, WVec3, const char*),
    W_CONSTRUCTOR_PROPERTY(const char*, const WColor&, const char*, WBitflags<WVisualizerAnchor>, WVec3),
    W_CONSTRUCTOR_PROPERTY(const char*, const WColor&, const char*, WBitflags<WVisualizerAnchor>),
    W_CONSTRUCTOR_PROPERTY(const char*, const WColor&, const char*),
    W_CONSTRUCTOR_PROPERTY(const char*, const WColor&),
    W_CONSTRUCTOR_PROPERTY(const char*),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WSphereVisualizerAttribute::WSphereVisualizerAttribute()
  : WVisualizerAttribute(nullptr)
{
}

WSphereVisualizerAttribute::WSphereVisualizerAttribute(const char* szRadiusProperty, const WColor& fixedColor /*= WColorScheme::LightUI(WColorScheme::Grape)*/, const char* szColorProperty /*= nullptr*/, WBitflags<WVisualizerAnchor> anchor /*= WVisualizerAnchor::Center*/, WVec3 vOffsetOrScale /*= WVec3::MakeZero*/, const char* szOffsetProperty /*= nullptr*/)
  : WVisualizerAttribute(szRadiusProperty, szColorProperty, szOffsetProperty)
  , m_Color(fixedColor)
  , m_vOffsetOrScale(vOffsetOrScale)
{
  m_Anchor = anchor;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCapsuleVisualizerAttribute, 1, WRTTIDefaultAllocator<WCapsuleVisualizerAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Color", m_Color),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const char*, const char*, const WColor&, const char*, WBitflags<WVisualizerAnchor>),
    W_CONSTRUCTOR_PROPERTY(const char*, const char*, const WColor&, const char*),
    W_CONSTRUCTOR_PROPERTY(const char*, const char*, const WColor&),
    W_CONSTRUCTOR_PROPERTY(const char*, const char*),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WCapsuleVisualizerAttribute::WCapsuleVisualizerAttribute()
  : WVisualizerAttribute(nullptr)
{
}

WCapsuleVisualizerAttribute::WCapsuleVisualizerAttribute(const char* szHeightProperty, const char* szRadiusProperty, const WColor& fixedColor /*= WColorScheme::LightUI(WColorScheme::Grape)*/, const char* szColorProperty /*= nullptr*/, WBitflags<WVisualizerAnchor> anchor /*= WVisualizerAnchor::Center*/)
  : WVisualizerAttribute(szHeightProperty, szRadiusProperty, szColorProperty)
  , m_Color(fixedColor)
{
  m_Anchor = anchor;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCylinderVisualizerAttribute, 1, WRTTIDefaultAllocator<WCylinderVisualizerAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Color", m_Color),
    W_MEMBER_PROPERTY("OffsetOrScale", m_vOffsetOrScale),
    W_ENUM_MEMBER_PROPERTY("Axis", WBasisAxis, m_Axis),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(WEnum<WBasisAxis>, const char*, const char*, const WColor&, const char*, WBitflags<WVisualizerAnchor>, WVec3, const char*),
    W_CONSTRUCTOR_PROPERTY(WEnum<WBasisAxis>, const char*, const char*, const WColor&, const char*, WBitflags<WVisualizerAnchor>, WVec3),
    W_CONSTRUCTOR_PROPERTY(WEnum<WBasisAxis>, const char*, const char*, const WColor&, const char*, WBitflags<WVisualizerAnchor>),
    W_CONSTRUCTOR_PROPERTY(WEnum<WBasisAxis>, const char*, const char*, const WColor&, const char*),
    W_CONSTRUCTOR_PROPERTY(WEnum<WBasisAxis>, const char*, const char*, const WColor&),
    W_CONSTRUCTOR_PROPERTY(WEnum<WBasisAxis>, const char*, const char*),
    W_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const WColor&, const char*, WBitflags<WVisualizerAnchor>, WVec3, const char*),
    W_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const WColor&, const char*, WBitflags<WVisualizerAnchor>, WVec3),
    W_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const WColor&, const char*, WBitflags<WVisualizerAnchor>),
    W_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const WColor&, const char*),
    W_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const WColor&),
    W_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WCylinderVisualizerAttribute::WCylinderVisualizerAttribute()
  : WVisualizerAttribute(nullptr)
{
}

WCylinderVisualizerAttribute::WCylinderVisualizerAttribute(WEnum<WBasisAxis> axis, const char* szHeightProperty, const char* szRadiusProperty, const WColor& fixedColor /*= WColorScheme::LightUI(WColorScheme::Grape)*/, const char* szColorProperty /*= nullptr*/, WBitflags<WVisualizerAnchor> anchor /*= WVisualizerAnchor::Center*/, WVec3 vOffsetOrScale /*= WVec3::MakeZero*/, const char* szOffsetProperty /*= nullptr*/)
  : WVisualizerAttribute(szHeightProperty, szRadiusProperty, szColorProperty, szOffsetProperty)
  , m_Color(fixedColor)
  , m_vOffsetOrScale(vOffsetOrScale)
  , m_Axis(axis)
{
  m_Anchor = anchor;
}

WCylinderVisualizerAttribute::WCylinderVisualizerAttribute(const char* szAxisProperty, const char* szHeightProperty, const char* szRadiusProperty, const WColor& fixedColor /*= WColorScheme::LightUI(WColorScheme::Grape)*/, const char* szColorProperty /*= nullptr*/, WBitflags<WVisualizerAnchor> anchor /*= WVisualizerAnchor::Center*/, WVec3 vOffsetOrScale /*= WVec3::MakeZero()*/, const char* szOffsetProperty /*= nullptr*/)
  : WVisualizerAttribute(szHeightProperty, szRadiusProperty, szColorProperty, szOffsetProperty, szAxisProperty)
  , m_Color(fixedColor)
  , m_vOffsetOrScale(vOffsetOrScale)
{
  m_Axis = WBasisAxis::Default;
  m_Anchor = anchor;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDirectionVisualizerAttribute, 1, WRTTIDefaultAllocator<WDirectionVisualizerAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("Axis", WBasisAxis, m_Axis),
    W_MEMBER_PROPERTY("Color", m_Color),
    W_MEMBER_PROPERTY("Scale", m_fScale)
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(WEnum<WBasisAxis>, float, const WColor&, const char*, const char*),
    W_CONSTRUCTOR_PROPERTY(WEnum<WBasisAxis>, float, const WColor&, const char*),
    W_CONSTRUCTOR_PROPERTY(WEnum<WBasisAxis>, float, const WColor&),
    W_CONSTRUCTOR_PROPERTY(WEnum<WBasisAxis>, float),
    W_CONSTRUCTOR_PROPERTY(const char*, float, const WColor&, const char*, const char*),
    W_CONSTRUCTOR_PROPERTY(const char*, float, const WColor&, const char*),
    W_CONSTRUCTOR_PROPERTY(const char*, float, const WColor&),
    W_CONSTRUCTOR_PROPERTY(const char*, float),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WDirectionVisualizerAttribute::WDirectionVisualizerAttribute()
  : WVisualizerAttribute(nullptr)
{
  m_Axis = WBasisAxis::PositiveX;
  m_fScale = 1.0f;
  m_Color = WColor::White;
}

WDirectionVisualizerAttribute::WDirectionVisualizerAttribute(WEnum<WBasisAxis> axis, float fScale, const WColor& fixedColor /*= WColorScheme::LightUI(WColorScheme::Grape)*/, const char* szColorProperty /*= nullptr*/, const char* szLengthProperty /*= nullptr*/)
  : WVisualizerAttribute(szColorProperty, szLengthProperty)
  , m_Axis(axis)
  , m_Color(fixedColor)
  , m_fScale(fScale)
{
}

WDirectionVisualizerAttribute::WDirectionVisualizerAttribute(const char* szAxisProperty, float fScale, const WColor& fixedColor /*= WColorScheme::LightUI(WColorScheme::Grape)*/, const char* szColorProperty /*= nullptr*/, const char* szLengthProperty /*= nullptr*/)
  : WVisualizerAttribute(szColorProperty, szLengthProperty, szAxisProperty)
  , m_Axis(WBasisAxis::PositiveX)
  , m_Color(fixedColor)
  , m_fScale(fScale)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WConeVisualizerAttribute, 1, WRTTIDefaultAllocator<WConeVisualizerAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("Axis", WBasisAxis, m_Axis),
    W_MEMBER_PROPERTY("Color", m_Color),
    W_MEMBER_PROPERTY("Scale", m_fScale),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(WEnum<WBasisAxis>, const char*, float, const char*, const WColor&, const char*),
    W_CONSTRUCTOR_PROPERTY(WEnum<WBasisAxis>, const char*, float, const char*, const WColor&),
    W_CONSTRUCTOR_PROPERTY(WEnum<WBasisAxis>, const char*, float, const char*),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WConeVisualizerAttribute::WConeVisualizerAttribute()
  : WVisualizerAttribute(nullptr)
  , m_Axis(WBasisAxis::PositiveX)
  , m_Color(WColor::Red)
  , m_fScale(1.0f)
{
}

WConeVisualizerAttribute::WConeVisualizerAttribute(WEnum<WBasisAxis> axis, const char* szAngleProperty, float fScale,
  const char* szRadiusProperty, const WColor& fixedColor /*= WColorScheme::LightUI(WColorScheme::Grape)*/, const char* szColorProperty)
  : WVisualizerAttribute(szAngleProperty, szRadiusProperty, szColorProperty)
  , m_Axis(axis)
  , m_Color(fixedColor)
  , m_fScale(fScale)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCameraVisualizerAttribute, 1, WRTTIDefaultAllocator<WCameraVisualizerAttribute>)
{
  //W_BEGIN_PROPERTIES
  //W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const char*, const char*),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WCameraVisualizerAttribute::WCameraVisualizerAttribute()
  : WVisualizerAttribute(nullptr)
{
}

WCameraVisualizerAttribute::WCameraVisualizerAttribute(const char* szModeProperty, const char* szFovProperty, const char* szOrthoDimProperty,
  const char* szNearPlaneProperty, const char* szFarPlaneProperty)
  : WVisualizerAttribute(szModeProperty, szFovProperty, szOrthoDimProperty, szNearPlaneProperty, szFarPlaneProperty)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WPositionVisualizerAttribute, 1, WRTTIDefaultAllocator<WPositionVisualizerAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("SizeScale", m_fSizeScale),
    W_MEMBER_PROPERTY("Color", m_Color),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const char*, float, const WColor&, const char*),
    W_CONSTRUCTOR_PROPERTY(const char*, float, const WColor&),
    W_CONSTRUCTOR_PROPERTY(const char*, float),
    W_CONSTRUCTOR_PROPERTY(const char*),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WPositionVisualizerAttribute::WPositionVisualizerAttribute()
  : WVisualizerAttribute(nullptr)
{
}

WPositionVisualizerAttribute::WPositionVisualizerAttribute(const char* szPositionProperty, float fSizeScale, const WColor& fixedColor, const char* szColorProperty)
  : WVisualizerAttribute(szPositionProperty, szColorProperty)
  , m_fSizeScale(fSizeScale)
  , m_Color(fixedColor)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMaxArraySizeAttribute, 1, WRTTIDefaultAllocator<WMaxArraySizeAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("MaxSize", m_uiMaxSize),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(WUInt32),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WPreventDuplicatesAttribute, 1, WRTTIDefaultAllocator<WPreventDuplicatesAttribute>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WExcludeFromScript, 1, WRTTIDefaultAllocator<WExcludeFromScript>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WScriptableFunctionAttribute, 1, WRTTIDefaultAllocator<WScriptableFunctionAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_ARRAY_MEMBER_PROPERTY("ArgNames", m_ArgNames),
    W_ARRAY_MEMBER_PROPERTY("ArgTypes", m_ArgTypes),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WScriptableFunctionAttribute::WScriptableFunctionAttribute(ArgType argType1 /*= In*/, const char* szArg1 /*= nullptr*/,
  ArgType argType2 /*= In*/, const char* szArg2 /*= nullptr*/,
  ArgType argType3 /*= In*/, const char* szArg3 /*= nullptr*/,
  ArgType argType4 /*= In*/, const char* szArg4 /*= nullptr*/,
  ArgType argType5 /*= In*/, const char* szArg5 /*= nullptr*/,
  ArgType argType6 /*= In*/, const char* szArg6 /*= nullptr*/,
  ArgType argType7 /*= In*/, const char* szArg7 /*= nullptr*/,
  ArgType argType8 /*= In*/, const char* szArg8 /*= nullptr*/,
  ArgType argType9 /*= In*/, const char* szArg9 /*= nullptr*/,
  ArgType argType10 /*= In*/, const char* szArg10 /*= nullptr*/,
  ArgType argType11 /*= In*/, const char* szArg11 /*= nullptr*/,
  ArgType argType12 /*= In*/, const char* szArg12 /*= nullptr*/)
{
  {
    if (WStringUtils::IsNullOrEmpty(szArg1))
      return;

    m_ArgNames.PushBack(szArg1);
    m_ArgTypes.PushBack(argType1);
  }
  {
    if (WStringUtils::IsNullOrEmpty(szArg2))
      return;

    m_ArgNames.PushBack(szArg2);
    m_ArgTypes.PushBack(argType2);
  }
  {
    if (WStringUtils::IsNullOrEmpty(szArg3))
      return;

    m_ArgNames.PushBack(szArg3);
    m_ArgTypes.PushBack(argType3);
  }
  {
    if (WStringUtils::IsNullOrEmpty(szArg4))
      return;

    m_ArgNames.PushBack(szArg4);
    m_ArgTypes.PushBack(argType4);
  }
  {
    if (WStringUtils::IsNullOrEmpty(szArg5))
      return;

    m_ArgNames.PushBack(szArg5);
    m_ArgTypes.PushBack(argType5);
  }
  {
    if (WStringUtils::IsNullOrEmpty(szArg6))
      return;

    m_ArgNames.PushBack(szArg6);
    m_ArgTypes.PushBack(argType6);
  }
  {
    if (WStringUtils::IsNullOrEmpty(szArg7))
      return;

    m_ArgNames.PushBack(szArg7);
    m_ArgTypes.PushBack(argType7);
  }
  {
    if (WStringUtils::IsNullOrEmpty(szArg8))
      return;

    m_ArgNames.PushBack(szArg8);
    m_ArgTypes.PushBack(argType8);
  }
  {
    if (WStringUtils::IsNullOrEmpty(szArg9))
      return;

    m_ArgNames.PushBack(szArg9);
    m_ArgTypes.PushBack(argType9);
  }
  {
    if (WStringUtils::IsNullOrEmpty(szArg10))
      return;

    m_ArgNames.PushBack(szArg10);
    m_ArgTypes.PushBack(argType10);
  }
  {
    if (WStringUtils::IsNullOrEmpty(szArg11))
      return;

    m_ArgNames.PushBack(szArg11);
    m_ArgTypes.PushBack(argType11);
  }
  {
    if (WStringUtils::IsNullOrEmpty(szArg12))
      return;

    m_ArgNames.PushBack(szArg12);
    m_ArgTypes.PushBack(argType12);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WFunctionArgumentAttributes, 1, WRTTIDefaultAllocator<WFunctionArgumentAttributes>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ArgIndex", m_uiArgIndex),
    W_ARRAY_MEMBER_PROPERTY("ArgAttributes", m_ArgAttributes)->AddFlags(WPropertyFlags::PointerOwner),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WFunctionArgumentAttributes::WFunctionArgumentAttributes(WUInt32 uiArgIndex, const WPropertyAttribute* pAttribute1, const WPropertyAttribute* pAttribute2 /*= nullptr*/, const WPropertyAttribute* pAttribute3 /*= nullptr*/, const WPropertyAttribute* pAttribute4 /*= nullptr*/)
  : m_uiArgIndex(uiArgIndex)
{
  m_bUsesGlobalNew = true;
  {
    if (pAttribute1 == nullptr)
      return;

    m_ArgAttributes.PushBack(pAttribute1);
  }
  {
    if (pAttribute2 == nullptr)
      return;

    m_ArgAttributes.PushBack(pAttribute2);
  }
  {
    if (pAttribute3 == nullptr)
      return;

    m_ArgAttributes.PushBack(pAttribute3);
  }
  {
    if (pAttribute4 == nullptr)
      return;

    m_ArgAttributes.PushBack(pAttribute4);
  }
}

WFunctionArgumentAttributes::~WFunctionArgumentAttributes()
{
  for (auto pAttribute : m_ArgAttributes)
  {
    auto pAttributeNonConst = const_cast<WPropertyAttribute*>(pAttribute);
    if (m_bUsesGlobalNew)
    {
      delete pAttributeNonConst;
    }
    else
    {
      W_DEFAULT_DELETE(pAttributeNonConst);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDynamicPinAttribute, 1, WRTTIDefaultAllocator<WDynamicPinAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Property", m_sProperty)
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WDynamicPinAttribute::WDynamicPinAttribute(const char* szProperty)
  : m_sProperty(szProperty)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLongOpAttribute, 1, WRTTIDefaultAllocator<WLongOpAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Type", m_sOpTypeName),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(),
    W_CONSTRUCTOR_PROPERTY(const char*),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGameObjectReferenceAttribute, 1, WRTTIDefaultAllocator<WGameObjectReferenceAttribute>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRttiTypeStringAttribute, 1, WRTTIDefaultAllocator<WRttiTypeStringAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("BaseType", m_sBaseType),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const char*),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSyncChildOrderAttribute, 1, WRTTIDefaultAllocator<WSyncChildOrderAttribute>)
W_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

W_STATICLINK_FILE(Foundation, Foundation_Reflection_Implementation_PropertyAttributes);
