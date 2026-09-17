#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Memory/InstanceDataAllocator.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

class WAnimGraphInstance;
class WAnimGraphNode;

//////////////////////////////////////////////////////////////////////////

using WAnimGraphResourceHandle = WTypedResourceHandle<class WAnimGraphResource>;

/// Maps a logical animation clip name to an actual animation clip resource.
///
/// Animation graphs reference clips by name (e.g., "Walk", "Jump") rather than directly referencing
/// resources. This allows the same graph to be used with different animation sets by remapping the clip
/// names to different resources.
///
/// This indirection allows:
/// - Same graph with different animation sets (e.g., male/female characters)
/// - Runtime clip swapping for character customization
/// - Graph reuse across different character types
struct W_RENDERERCORE_DLL WAnimationClipMapping : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WAnimationClipMapping, WReflectedClass);

  WHashedString m_sClipName;
  WAnimationClipResourceHandle m_hClip;

  const char* GetClipName() const { return m_sClipName.GetData(); }
  void SetClipName(const char* szName) { m_sClipName.Assign(szName); }
};

/// Resource containing an animation graph definition (WAnimGraph) and its animation clip mappings (WAnimationClipMapping).
///
/// ## Content
///
/// - **Animation Graph**: The node graph structure (WAnimGraph)
/// - **Clip Mappings**: Maps logical names like "Walk" to actual animation clip resources
/// - **Include Graphs**: References to other graph resources to compose larger graphs
///
/// ## Usage Pattern
///
/// Resources are loaded via the resource manager and shared across multiple characters:
///
/// ```cpp
/// WAnimGraphResourceHandle hGraph = WResourceManager::LoadResource<WAnimGraphResource>("Character.WAnimGraph");
/// controller.AddAnimGraph(hGraph);
/// ```
class W_RENDERERCORE_DLL WAnimGraphResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WAnimGraphResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WAnimGraphResource);

public:
  WAnimGraphResource();
  ~WAnimGraphResource();

  const WAnimGraph& GetAnimationGraph() const { return m_AnimGraph; }

  /// References to other animation graph resources that should be included.
  ///
  /// Used to compose complex graphs from smaller reusable pieces.
  WArrayPtr<const WString> GetIncludeGraphs() const { return m_IncludeGraphs; }

  /// Default mappings from clip names to animation resources.
  ///
  /// These can be overridden at runtime via WAnimController::SetAnimationClipInfo().
  const WDynamicArray<WAnimationClipMapping>& GetAnimationClipMapping() const { return m_AnimationClipMapping; }

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  WDynamicArray<WString> m_IncludeGraphs;
  WDynamicArray<WAnimationClipMapping> m_AnimationClipMapping;
  WAnimGraph m_AnimGraph;
};
