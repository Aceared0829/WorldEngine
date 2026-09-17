#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Reflection/PropertyPath.h>

using WPrefabResourceHandle = WTypedResourceHandle<class WPrefabResource>;

struct W_CORE_DLL WPrefabResourceDescriptor
{
};

struct W_CORE_DLL WExposedPrefabParameterDesc
{
  WHashedString m_sExposeName;
  WUInt32 m_uiWorldReaderChildObject : 1; // 0 -> use root object array, 1 -> use child object array
  WUInt32 m_uiWorldReaderObjectIndex : 31;
  WHashedString m_sComponentType;         // WRTTI type name to identify which component is meant, empty string -> affects game object
  WHashedString m_sProperty;              // which property to override
  WPropertyPath m_CachedPropertyPath;     // cached WPropertyPath to apply a value to the specified property

  void Save(WStreamWriter& inout_stream) const;
  void Load(WStreamReader& inout_stream);
};

class W_CORE_DLL WPrefabResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WPrefabResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WPrefabResource);
  W_RESOURCE_DECLARE_CREATEABLE(WPrefabResource, WPrefabResourceDescriptor);

public:
  WPrefabResource();

  enum class InstantiateResult : WUInt8
  {
    Success,
    NotYetLoaded,
    Error,
  };

  /// Helper function to instantiate a prefab without having to deal with resource acquisition.
  static WPrefabResource::InstantiateResult InstantiatePrefab(const WPrefabResourceHandle& hPrefab, bool bBlockTillLoaded, WWorld& ref_world, const WTransform& rootTransform, WPrefabInstantiationOptions options = {}, const WArrayMap<WHashedString, WVariant>* pExposedParamValues = nullptr);

  /// Creates an instance of this prefab in the given world.
  void InstantiatePrefab(WWorld& ref_world, const WTransform& rootTransform, WPrefabInstantiationOptions options, const WArrayMap<WHashedString, WVariant>* pExposedParamValues = nullptr);

  void ApplyExposedParameterValues(const WArrayMap<WHashedString, WVariant>* pExposedParamValues, const WDynamicArray<WGameObject*>& createdChildObjects, const WDynamicArray<WGameObject*>& createdRootObjects) const;

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  WUInt32 FindFirstParamWithName(WUInt64 uiNameHash) const;

  WWorldReader m_WorldReader;
  WDynamicArray<WExposedPrefabParameterDesc> m_PrefabParamDescs;
};
