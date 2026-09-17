#pragma once

#include <Core/CoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/World/Declarations.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>


/// A base class for user-defined data assets.
///
/// Allows users to define their own asset types that can be created, edited and referenced in the editor without writing an editor plugin.
///
/// In order to do that, subclass WCustomData,
/// and put the macro W_DECLARE_CUSTOM_DATA_RESOURCE(YourCustomData) into the header next to your custom type.
/// Also put the macro W_DEFINE_CUSTOM_DATA_RESOURCE(YourCustomData) into the implementation file.
///
/// Those will also define resource and resource handle types, such as YourCustomDataResource and YourCustomDataResourceHandle.
///
/// For a full example see SampleCustomData in the SampleGamePlugin.
class W_CORE_DLL WCustomData : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WCustomData, WReflectedClass);

public:
  /// Loads the serialized custom data using a robust serialization-based method.
  ///
  /// This function does not need to be overridden. It will work, even if the properties change.
  /// It is only virtual in case you want to hook into the deserialization process.
  virtual void Load(class WAbstractObjectGraph& ref_graph, class WRttiConverterContext& ref_context, const class WAbstractObjectNode* pRootNode);
};

/// Base class for resources that represent different implementations of WCustomData
///
/// These resources are automatically generated using these macros:
///   W_DECLARE_CUSTOM_DATA_RESOURCE(YourCustomData)
///   W_DEFINE_CUSTOM_DATA_RESOURCE(YourCustomData)
///
/// Put the former into a header next to YourCustomData and the latter into a cpp file.
///
/// This builds these types:
///   YourCustomDataResource
///   YourCustomDataResourceHandle
///
/// You can then use these to reference this resource type for example in components.
/// For a full example search the SampleGamePlugin for SampleCustomDataResource and SampleCustomDataResourceHandle and see how they are used.
class W_CORE_DLL WCustomDataResourceBase : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WCustomDataResourceBase, WResource);

public:
  WCustomDataResourceBase();
  ~WCustomDataResourceBase();

protected:
  virtual void CreateAndLoadData(WAbstractObjectGraph& ref_graph, WRttiConverterContext& ref_context, const WAbstractObjectNode* pRootNode) = 0;
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  WResourceLoadDesc UpdateContent_Internal(WStreamReader* Stream, const WRTTI& rtti);
};

/// Template resource type for sub-classed WCustomData types.
///
/// See WCustomDataResourceBase for details.
template <typename T>
class WCustomDataResource : public WCustomDataResourceBase
{
public:
  WCustomDataResource();
  ~WCustomDataResource();

  /// Provides read access to the custom data type.
  ///
  /// Returns nullptr, if the resource wasn't loaded successfully.
  const T* GetData() const { return GetLoadingState() == WResourceState::Loaded ? reinterpret_cast<const T*>(m_Data) : nullptr; }

protected:
  virtual void CreateAndLoadData(WAbstractObjectGraph& graph, WRttiConverterContext& context, const WAbstractObjectNode* pRootNode) override;

  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;

  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;

  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  struct alignas(alignof(T))
  {
    WUInt8 m_Data[sizeof(T)];
  };
};

/// Helper macro to declare a WCustomDataResource<T> and a matching resource handle
///
/// See WCustomDataResourceBase for details.
#define W_DECLARE_CUSTOM_DATA_RESOURCE(SELF)                              \
  class SELF##Resource : public WCustomDataResource<SELF>                 \
  {                                                                        \
    W_ADD_DYNAMIC_REFLECTION(SELF##Resource, WCustomDataResource<SELF>); \
    W_RESOURCE_DECLARE_COMMON_CODE(SELF##Resource);                       \
  };                                                                       \
                                                                           \
  using SELF##ResourceHandle = WTypedResourceHandle<SELF##Resource>

/// Helper macro to define a WCustomDataResource<T>
///
/// See WCustomDataResourceBase for details.
#define W_DEFINE_CUSTOM_DATA_RESOURCE(SELF)                                                 \
  W_BEGIN_DYNAMIC_REFLECTED_TYPE(SELF##Resource, 1, WRTTIDefaultAllocator<SELF##Resource>) \
  W_END_DYNAMIC_REFLECTED_TYPE;                                                             \
                                                                                             \
  W_RESOURCE_IMPLEMENT_COMMON_CODE(SELF##Resource)


#include <Core/Utils/Implementation/CustomData_inl.h>
