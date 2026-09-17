#pragma once

#include <Core/World/GameObject.h>
#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <VisualScriptPlugin/VisualScriptPluginDLL.h>

/// Data types that are available in visual script. These are a subset of WVariantType.
///
/// Like with WVariantType, the order of these types is important as they are used to determine
/// if a type is "bigger" during type deduction. Also the enum values are serialized in visual script files.
struct W_VISUALSCRIPTPLUGIN_DLL WVisualScriptDataType
{
  using StorageType = WUInt8;

  enum Enum : WUInt8
  {
    Invalid = 0,

    Bool,
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
    Array,
    Map,
    Coroutine,

    Count,

    EnumValue,
    BitflagValue,
    Resource,

    ExtendedCount,

    AnyPointer = 0xFE,
    Any = 0xFF,

    Default = Invalid,
  };

  W_ALWAYS_INLINE static bool IsNumber(Enum dataType) { return dataType >= Byte && dataType <= Double; }
  W_ALWAYS_INLINE static bool IsNumberOrBool(Enum dataType) { return dataType == Bool || IsNumber(dataType); }
  W_ALWAYS_INLINE static bool IsVector(Enum dataType) { return dataType >= Vector2 && dataType <= Vector4; }
  W_ALWAYS_INLINE static bool IsPointer(Enum dataType) { return (dataType >= GameObject && dataType <= TypedPointer) || dataType == Coroutine; }

  static WVariantType::Enum GetVariantType(Enum dataType);
  static Enum FromVariantType(WVariantType::Enum variantType);

  static WProcessingStream::DataType GetStreamDataType(Enum dataType);

  static const WRTTI* GetRtti(Enum dataType);
  static Enum FromRtti(const WRTTI* pRtti);

  static WUInt32 GetStorageSize(Enum dataType);
  static WUInt32 GetStorageAlignment(Enum dataType);

  static const char* GetName(Enum dataType);

  static bool CanConvertTo(Enum sourceDataType, Enum targetDataType);
};

W_DECLARE_REFLECTABLE_TYPE(W_VISUALSCRIPTPLUGIN_DLL, WVisualScriptDataType);

struct W_VISUALSCRIPTPLUGIN_DLL WVisualScriptGameObjectHandle
{
  WGameObjectHandle m_Handle;
  mutable WGameObject* m_Ptr;
  mutable WUInt32 m_uiExecutionCounter;

  void AssignHandle(const WGameObjectHandle& hObject)
  {
    m_Handle = hObject;
    m_Ptr = nullptr;
    m_uiExecutionCounter = 0;
  }

  void AssignPtr(WGameObject* pObject, WUInt32 uiExecutionCounter)
  {
    m_Handle = pObject != nullptr ? pObject->GetHandle() : WGameObjectHandle();
    m_Ptr = pObject;
    m_uiExecutionCounter = uiExecutionCounter;
  }

  WGameObject* GetPtr(WUInt32 uiExecutionCounter) const;
};

struct W_VISUALSCRIPTPLUGIN_DLL WVisualScriptComponentHandle
{
  WComponentHandle m_Handle;
  mutable WComponent* m_Ptr;
  mutable WUInt32 m_uiExecutionCounter;

  void AssignHandle(const WComponentHandle& hComponent)
  {
    m_Handle = hComponent;
    m_Ptr = nullptr;
    m_uiExecutionCounter = 0;
  }

  void AssignPtr(WComponent* pComponent, WUInt32 uiExecutionCounter)
  {
    m_Handle = pComponent != nullptr ? pComponent->GetHandle() : WComponentHandle();
    m_Ptr = pComponent;
    m_uiExecutionCounter = uiExecutionCounter;
  }

  WComponent* GetPtr(WUInt32 uiExecutionCounter) const;
};
