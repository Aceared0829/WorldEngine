#pragma once

#include <RendererCore/Pipeline/Renderer.h>

/// Registry to get a renderer for a specific render data type. Instances of all renderers are automatically created and registered.
class W_RENDERERCORE_DLL WRendererRegistry
{
public:
  W_FORCE_INLINE static const WRenderer* GetRenderer(const WRTTI* pRenderDataType)
  {
    if (s_bRendererInstancesDirty)
    {
      CreateRendererInstances();
    }

    WUInt32 uiIndex = 0;
    if (s_RenderDataTypeToRendererIndex.TryGetValue(pRenderDataType, uiIndex))
    {
      return s_RendererInstances[uiIndex].Borrow();
    }

    return nullptr;
  }

private:
  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, RendererRegistry);

  static void PluginEventHandler(const WPluginEvent& e);
  static void UpdateRendererTypes();

  static void CreateRendererInstances();
  static void ClearRendererInstances();

  static WHybridArray<const WRTTI*, 16> s_RendererTypes;
  static WDynamicArray<WUniquePtr<WRenderer>> s_RendererInstances;
  static WHashTable<const WRTTI*, WUInt32> s_RenderDataTypeToRendererIndex;
  static bool s_bRendererInstancesDirty;
};
