#pragma once

#include <Core/World/WorldModule.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererFoundation/Shader/Types.h>

// temporary until we moved all the render data caching into this module
#include <RendererCore/RenderWorld/RenderWorld.h>

struct WPerInstanceData;
struct WRenderWorldExtractionEvent;

/// Manager for render data and instance data buffers.
///
/// Render data is used to extract rendering information from components during the extraction phase that is then used for rendering.
/// If many objects should be rendered with one instanced draw call, instance data buffers are used to hold the per-instance information.
/// For that the render data should derive from WInstanceableRenderData. See WPerInstanceData what data is supported by default for each instance.
/// When more per instance data is needed it is possible to register a custom instance data buffer and attach that to the render data as well.
class W_RENDERERCORE_DLL WRenderDataManager : public WWorldModule
{
  W_DECLARE_WORLD_MODULE();
  W_ADD_DYNAMIC_REFLECTION(WRenderDataManager, WWorldModule);

public:
  WRenderDataManager(WWorld* pWorld);
  virtual ~WRenderDataManager();

  virtual void Initialize() override;

  /// Creates render data that is only valid for this frame. The data is automatically deleted after the frame has been rendered.
  template <typename T>
  T* CreateRenderDataForThisFrame(const WGameObject* pOwner) const;

  // TODO: move render data caching into this world module as well

  /// Gets or creates per-instance data for the given instance data offset.
  ///
  /// This function is thread-safe and is typically called in an WMsgExtractRenderData message handler.
  /// The render data manager holds two instance data buffers, one for static objects and one for dynamic objects.
  /// Typically one would pass GetOwner()->IsDynamic() as bDynamic. If the corresponding render data is not cached
  /// it is better to always pass true so that the static buffer does not need to be uploaded every frame.
  WArrayPtr<WPerInstanceData> GetOrCreateInstanceData(const WComponent* pOwnerComponent, bool bDynamic, WGALDynamicBufferHandle& out_hBuffer, WInstanceDataOffset& inout_instanceDataOffset, WUInt32 uiCount = 1) const;

  /// Deletes the instance data associated with the given instance data offset.
  ///
  /// This function is thread-safe but is typically called in the OnDeactivated function of a component.
  void DeleteInstanceData(WInstanceDataOffset& inout_instanceDataOffset) const;

  /// Helper function to fill WPerInstanceData.
  static void FillPerInstanceData(WPerInstanceData& out_perInstanceData, const WGameObject* pObject, const WTransform& globalTransform, WUInt32 uiUniqueID = 0, const WColor& color = WColor::White, const WVec4& vCustomData = WVec4(0, 1, 0, 1), float fBoundingSphereRadius = 1.0f, WUInt32 uiRandomSeed = 0);

  /// Helper function that combines GetOrCreateInstanceData and FillPerInstanceData.
  WGALDynamicBufferHandle GetOrCreateInstanceDataAndFill(const WComponent& ownerComponent, bool bDynamic, const WTransform& globalTransform, WInstanceDataOffset& inout_instanceDataOffset, WUInt32 uiUniqueID = 0, const WColor& color = WColor::White, const WVec4& vCustomData = WVec4(0, 1, 0, 1)) const;


  /// Registers a custom instance data buffer that can be used to store additional per-instance data.
  ///
  /// The beforeUploadCallback is called just before the buffer is uploaded each frame, so it can be used to e.g. wait for a task that generated the data.
  WUInt32 RegisterCustomInstanceData(const WGALBufferCreationDescription& desc, WStringView sDebugName, WDelegate<void()> beforeUploadCallback = {});

  /// Gets or creates custom per-instance data for the given instance data offset.
  ///
  /// This function is thread-safe and is typically called in an WMsgExtractRenderData message handler.
  template <typename T>
  WArrayPtr<T> GetOrCreateCustomInstanceData(WUInt32 uiCustomDataIndex, const WComponent* pOwnerComponent, WGALDynamicBufferHandle& out_hBuffer, WCustomInstanceDataOffset& inout_instanceDataOffset, WUInt32 uiCount = 1) const;

  /// Deletes the custom instance data associated with the given instance data offset.
  ///
  /// This function is thread-safe but is typically called in the OnDeactivated function of a component.
  void DeleteCustomInstanceData(WUInt32 uiCustomDataIndex, WCustomInstanceDataOffset& inout_instanceDataOffset) const;

  /// Helper function that combines GetOrCreateCustomInstanceData and fills it with the given data.
  template <typename T>
  WGALDynamicBufferHandle GetOrCreateCustomInstanceDataAndFill(WUInt32 uiCustomDataIndex, const WComponent& ownerComponent, WCustomInstanceDataOffset& inout_instanceDataOffset, const T& data) const;

  /// Returns the underlying dynamic buffer for the given custom instance data buffer index.
  WGALDynamicBufferHandle GetCustomInstanceDataBuffer(WUInt32 uiCustomDataIndex) const;

  /// Compacts the given custom instance data buffer to reduce fragmentation.
  ///
  /// This is only necessary if allocations with different counts were created and deleted over time.
  void CompactCustomInstanceDataBuffer(WUInt32 uiCustomDataIndex, WUInt32 uiMaxSteps = 16);


  /// Gets or creates skinning data for the given instance data offset.
  ///
  /// WSkinningState wraps around these functions to manage skinning data for skinned meshes
  /// and should be preferred instead of calling these functions directly.
  WArrayPtr<WShaderTransform> GetOrCreateSkinningData(const WComponent* pOwnerComponent, WCustomInstanceDataOffset& inout_instanceDataOffset, WUInt32 uiNumTransforms) const;

  /// Gets the skinning data for reading for the given instance data offset.
  WArrayPtr<const WShaderTransform> GetSkinningData(const WCustomInstanceDataOffset& instanceDataOffset) const;

  /// Deletes the skinning data associated with the given instance data offset.
  void DeleteSkinningData(WCustomInstanceDataOffset& inout_instanceDataOffset) const;

  /// Returns the underlying dynamic buffer that holds the skinning data.
  WGALDynamicBufferHandle GetSkinningDataBuffer() const;

private:
  WByteArrayPtr GetOrCreateCustomInstanceData(WUInt32 uiCustomDataIndex, WUInt32 uiStructByteSize, const WComponent* pOwnerComponent, WGALDynamicBufferHandle& out_hBuffer, WCustomInstanceDataOffset& inout_instanceDataOffset, WUInt32 uiCount) const;

  void CompactSkinningDataBuffer(const UpdateContext& context);
  void OnExtractionEvent(const WRenderWorldExtractionEvent& e);

  mutable WMutex m_Mutex;

  WHybridArray<WGALDynamicBufferHandle, 16> m_Buffers;
  WDynamicArray<WDelegate<void()>> m_BeforeUploadCallbacks;

  struct ExtractionData
  {
    WHybridArray<WGALDynamicBuffer*, 16> m_pBuffers;
  };

  ExtractionData m_ExtractionData;
};

#include <RendererCore/Pipeline/Implementation/RenderDataManager_inl.h>
