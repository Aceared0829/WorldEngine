#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>
#include <Foundation/Utilities/EnumerableClass.h>

class WStreamWriter;
class WStreamReader;
class WVariantTypeInfo;

/// Variant type registry allows for custom variant type infos to be accessed.
///
/// Custom variant types are defined via the W_DECLARE_CUSTOM_VARIANT_TYPE and W_DEFINE_CUSTOM_VARIANT_TYPE macros.
/// \sa W_DECLARE_CUSTOM_VARIANT_TYPE, W_DEFINE_CUSTOM_VARIANT_TYPE
class W_FOUNDATION_DLL WVariantTypeRegistry
{
  W_DECLARE_SINGLETON(WVariantTypeRegistry);

public:
  /// Find the variant type info for the given WRTTI type.
  /// \return WVariantTypeInfo if one exits for the given type, otherwise nullptr.
  const WVariantTypeInfo* FindVariantTypeInfo(const WRTTI* pType) const;
  ~WVariantTypeRegistry();

private:
  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, VariantTypeRegistry);
  WVariantTypeRegistry();

  void PluginEventHandler(const WPluginEvent& EventData);
  void UpdateTypes();

  WHashTable<const WRTTI*, const WVariantTypeInfo*> m_TypeInfos;
};

/// Defines functions to allow the full feature set of WVariant to be used.
/// \sa W_DEFINE_CUSTOM_VARIANT_TYPE, WVariantTypeRegistry
class W_FOUNDATION_DLL WVariantTypeInfo : public WEnumerable<WVariantTypeInfo>
{
public:
  WVariantTypeInfo();
  virtual const WRTTI* GetType() const = 0;
  virtual WUInt32 Hash(const void* pObject) const = 0;
  virtual bool Equal(const void* pObjectA, const void* pObjectB) const = 0;
  virtual void Serialize(WStreamWriter& ref_writer, const void* pObject) const = 0;
  virtual void Deserialize(WStreamReader& ref_reader, void* pObject) const = 0;

  W_DECLARE_ENUMERABLE_CLASS(WVariantTypeInfo);
};

/// Helper template used by W_DEFINE_CUSTOM_VARIANT_TYPE.
/// \sa W_DEFINE_CUSTOM_VARIANT_TYPE
template <typename T>
class WVariantTypeInfoT : public WVariantTypeInfo
{
  const WRTTI* GetType() const override
  {
    return WGetStaticRTTI<T>();
  }
  WUInt32 Hash(const void* pObject) const override
  {
    return WHashHelper<T>::Hash(*static_cast<const T*>(pObject));
  }
  bool Equal(const void* pObjectA, const void* pObjectB) const override
  {
    return WHashHelper<T>::Equal(*static_cast<const T*>(pObjectA), *static_cast<const T*>(pObjectB));
  }
  void Serialize(WStreamWriter& writer, const void* pObject) const override
  {
    writer << *static_cast<const T*>(pObject);
  }
  void Deserialize(WStreamReader& reader, void* pObject) const override
  {
    reader >> *static_cast<T*>(pObject);
  }
};

/// Defines a custom variant type, allowing it to be serialized and compared. The type needs to be declared first before using this macro.
///
/// The given type must implement WHashHelper and WStreamWriter / WStreamReader operators.
/// Macros should be placed in any cpp. Note that once a custom type is defined, it is considered a value type and will be passed by value. It must be linked into every editor and engine dll to allow serialization. Thus it should only be used for common types in base libraries.
/// Limitations: Currently only member variables are supported on custom types, no arrays, set, maps etc. For best performance, any custom type smaller than 16 bytes should be POD so it can be inlined into the WVariant.
/// \sa W_DECLARE_CUSTOM_VARIANT_TYPE, WVariantTypeRegistry, WVariant
#define W_DEFINE_CUSTOM_VARIANT_TYPE(TYPE)                                                                                                                         \
  static_assert(WVariantTypeDeduction<TYPE>::value == WVariantType::TypedObject, "W_DECLARE_CUSTOM_VARIANT_TYPE needs to be added to the header defining TYPE"); \
  WVariantTypeInfoT<TYPE> g_WVariantTypeInfoT_##TYPE;
