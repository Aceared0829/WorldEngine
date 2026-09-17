#pragma once

#include <Core/Utils/Blackboard.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/TagSet.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelineNode.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <RendererCore/Pipeline/ViewData.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

class WBlackboard;
class WFrustum;
class WWorld;
class WRenderPipeline;

/// Encapsulates a view on the given world through the given camera
/// and rendered with the specified RenderPipeline into the given render target setup.
class W_RENDERERCORE_DLL WView : WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WView, WReflectedClass);

private:
  /// Use WRenderLoop::CreateView to create a view.
  WView();
  ~WView();

  W_DISALLOW_COPY_AND_ASSIGN(WView);

public:
  WViewHandle GetHandle() const;

  void SetName(WStringView sName);
  WStringView GetName() const;

  void SetWorld(WWorld* pWorld);
  WWorld* GetWorld();
  const WWorld* GetWorld() const;

  /// Sets the swapchain that this view will be rendering into. Can be invalid in case the render target is an off-screen buffer in which case SetRenderTargets needs to be called.
  /// Setting the swap-chain is necessary in order to acquire and present the image to the window.
  /// SetSwapChain and SetRenderTargets are mutually exclusive. Calling this function will reset the render targets.
  void SetSwapChain(WGALSwapChainHandle hSwapChain);
  WGALSwapChainHandle GetSwapChain() const;

  /// Sets the off-screen render targets. Use SetSwapChain if rendering to a window.
  /// SetSwapChain and SetRenderTargets are mutually exclusive. Calling this function will reset the swap chain.
  void SetRenderTargets(const WGALRenderTargets& renderTargets);
  const WGALRenderTargets& GetRenderTargets() const;

  /// Returns the render targets that were either set via the swapchain or via the manually set render targets.
  const WGALRenderTargets& GetActiveRenderTargets() const;

  void SetRenderPipelineResource(WRenderPipelineResourceHandle hPipeline);
  WRenderPipelineResourceHandle GetRenderPipelineResource() const;

  void SetCamera(WCamera* pCamera);
  WCamera* GetCamera();
  const WCamera* GetCamera() const;

  void SetCullingCamera(const WCamera* pCamera);
  const WCamera* GetCullingCamera() const;

  void SetLodCamera(const WCamera* pCamera);
  const WCamera* GetLodCamera() const;

  /// Returns the camera usage hint for the view.
  WEnum<WCameraUsageHint> GetCameraUsageHint() const;
  /// Sets the camera usage hint for the view. If not 'None', the camera component of the same usage will be auto-connected
  ///   to this view.
  void SetCameraUsageHint(WEnum<WCameraUsageHint> val);

  void SetViewRenderMode(WEnum<WViewRenderMode> value);
  WEnum<WViewRenderMode> GetViewRenderMode() const;

  void SetViewport(const WRectFloat& viewport);
  const WRectFloat& GetViewport() const;

  /// Forces the render pipeline to be rebuilt.
  void ForceUpdate();

  const WViewData& GetData() const;

  bool IsValid() const;

  /// Extracts all relevant data from the world to render the view.
  void ExtractData();

  /// Returns a task implementation that calls ExtractData on this view.
  const WSharedPtr<WTask>& GetExtractTask();


  /// Calculates the start position and direction (in world space) of the picking ray through the screen position in this view.
  ///
  /// fNormalizedScreenPosX and fNormalizedScreenPosY are expected to be in [0; 1] range (normalized screen coordinates).
  /// If no ray can be computed, W_FAILURE is returned.
  WResult ComputePickingRay(float fNormalizedScreenPosX, float fNormalizedScreenPosY, WVec3& out_vRayStartPos, WVec3& out_vRayDir) const;

  /// Calculates the normalized screen-space coordinate ([0; 1] range) that the given world-space point projects to.
  ///
  /// Returns W_FAILURE, if the point could not be projected into screen-space.
  WResult ComputeScreenSpacePos(const WVec3& vWorldPos, WVec3& out_vScreenPosNormalized) const;

  /// Calculates the world-space position that the given normalized screen-space coordinate maps to
  WResult ComputeWorldSpacePos(float fNormalizedScreenPosX, float fNormalizedScreenPosY, WVec3& out_vWorldPos) const;

  /// Converts a screen-space position from pixel coordinates to normalized coordinates.
  void ConvertScreenPixelPosToNormalizedPos(WVec3& inout_vPixelPos);

  /// Converts a screen-space position from normalized coordinates to pixel coordinates.
  void ConvertScreenNormalizedPosToPixelPos(WVec3& inout_vNormalizedPos);


  /// Returns the current projection matrix.
  const WMat4& GetProjectionMatrix(WCameraEye eye = WCameraEye::Left) const;

  /// Returns the current inverse projection matrix.
  const WMat4& GetInverseProjectionMatrix(WCameraEye eye = WCameraEye::Left) const;

  /// Returns the current view matrix (camera orientation).
  const WMat4& GetViewMatrix(WCameraEye eye = WCameraEye::Left) const;

  /// Returns the current inverse view matrix (inverse camera orientation).
  const WMat4& GetInverseViewMatrix(WCameraEye eye = WCameraEye::Left) const;

  /// Returns the current view-projection matrix.
  const WMat4& GetViewProjectionMatrix(WCameraEye eye = WCameraEye::Left) const;

  /// Returns the current inverse view-projection matrix.
  const WMat4& GetInverseViewProjectionMatrix(WCameraEye eye = WCameraEye::Left) const;

  /// Returns the frustum that should be used for determine visible objects for this view.
  void ComputeCullingFrustum(WFrustum& out_frustum) const;

  void SetShaderPermutationVariable(const char* szName, const char* szValue);

  /// This blackboard can be used to set view specific render pipeline pass or extractor properties,
  /// or to overwrite properties that are already set on the world blackboard.
  ///
  /// To set properties the blackboard entry name must be in the form of "PassName.PropertyName" or "ExtractorName.PropertyName".
  void SetBlackboard(const WSharedPtr<WBlackboard>& pBlackboard);
  const WSharedPtr<WBlackboard>& GetBlackboard() const;

  /// Pushes the view and camera data into the extracted data of the pipeline.
  ///
  /// Use WRenderWorld::GetDataIndexForExtraction() to update the data from the extraction thread. Can't be used if this view is currently extracted.
  /// Use WRenderWorld::GetDataIndexForRendering() to update the data from the render thread.
  void UpdateViewData(WUInt32 uiDataIndex);

  WTagSet m_IncludeTags;
  WTagSet m_ExcludeTags;

private:
  friend class WRenderWorld;
  friend class WMemoryUtils;
  friend class WGpuPipelineTest;

  WViewId m_InternalId;

  WSharedPtr<WTask> m_pExtractTask;

  WWorld* m_pWorld = nullptr;

  WRenderPipelineResourceHandle m_hRenderPipeline;
  WUInt32 m_uiRenderPipelineResourceDescriptionCounter = 0;
  WSharedPtr<WRenderPipeline> m_pRenderPipeline;
  WCamera* m_pCamera = nullptr;
  const WCamera* m_pCullingCamera = nullptr;
  const WCamera* m_pLodCamera = nullptr;


private:
  WRenderPipelineNodeInputPin m_PinRenderTarget0;
  WRenderPipelineNodeInputPin m_PinRenderTarget1;
  WRenderPipelineNodeInputPin m_PinRenderTarget2;
  WRenderPipelineNodeInputPin m_PinRenderTarget3;
  WRenderPipelineNodeInputPin m_PinDepthStencil;

private:
  void UpdateCachedMatrices() const;

  /// Rebuilds pipeline if necessary and pushes double-buffered settings into the pipeline.
  void EnsureUpToDate();

  mutable WUInt32 m_uiLastCameraSettingsModification = 0;
  mutable WUInt32 m_uiLastCameraOrientationModification = 0;
  mutable float m_fLastViewportAspectRatio = 1.0f;

  mutable WViewData m_Data;

  WInternal::RenderDataCache* m_pRenderDataCache = nullptr;

  WDynamicArray<WPermutationVar> m_PermutationVars;
  bool m_bPermutationVarsDirty = false;

  void ReadBackPassProperties();

  void ApplyPermutationVars();
  void ApplyPropertiesFromBlackboard();
  void RebuildPropertyMappings(const WBlackboard* const* pBlackboards);
  void UpdatePropertyMappings(const bool* pBlackboardValuesChanged);
  bool RebuildSwitchMappings(const WBlackboard* const* pBlackboards);
  bool UpdateSwitchValues(const bool* pBlackboardValuesChanged);

  WSharedPtr<WBlackboard> m_pWorldBlackboard;
  WSharedPtr<WBlackboard> m_pViewBlackboard;

  enum SourceBlackboard : WUInt8
  {
    World,
    View,

    COUNT
  };

  struct ChangeCounter
  {
    WUInt32 m_uiStructure = 0;
    WUInt32 m_uiValue = 0;
  };

  ChangeCounter m_BlackboardChangeCounter[SourceBlackboard::COUNT] = {};

  struct PropertyMapping
  {
    WReflectedClass* m_pObject = nullptr;
    const WAbstractMemberProperty* m_pProperty = nullptr;
    // Only valid as long as the blackboard's structure does not change, see SwitchMapping::m_pEntry.
    const WBlackboard::Entry* m_pEntry = nullptr;
    WUInt32 m_uiEntryChangeCounter = 0;
    SourceBlackboard m_SourceIndex = SourceBlackboard::World;

    WVariant m_DefaultValue;
  };

  WHashTable<WHashedString, PropertyMapping> m_PropertyMappings;

  struct SwitchMapping
  {
    // Entries are stored in a hash table, so this pointer is only valid as long as the blackboard's structure does not change. ApplyPropertiesFromBlackboard detects such a change in the same frame it happens and rebuilds the mappings before they are read again.
    const WBlackboard::Entry* m_pEntry = nullptr;
    WUInt32 m_uiEntryChangeCounter = 0;
    SourceBlackboard m_SourceIndex = SourceBlackboard::World;
  };

  WDynamicArray<SwitchMapping> m_SwitchMappings;

  // Forces both property and switch mappings to be resolved again, even if no blackboard reported a change. Necessary when a blackboard is attached or detached, because a detached blackboard cannot report the removal of its entries.
  bool m_bBlackboardMappingsDirty = true;
};

#include <RendererCore/Pipeline/Implementation/View_inl.h>
