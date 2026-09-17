#pragma once

#include <Mcp/McpTool.h>

/// Creates the LOD mesh assets that sit next to a mesh asset, which is otherwise only reachable
/// through the asset browser's context menu and its dialog.
///
/// Split the same way as the prefab tools: 'mesh_lod_info' reports what a mesh would get and what it
/// already has, 'mesh_lod_create' performs it. Creating with the defaults needs no info call.
///
/// The LODs are what makes 'mesh_prefab_create' build an WLodMeshComponent, so this runs before it.
class WMcpMeshLodTool : public WMcpToolProvider
{
  W_ADD_DYNAMIC_REFLECTION(WMcpMeshLodTool, WMcpToolProvider);

public:
  virtual void GetSupportedTools(WDynamicArray<WMcpToolDesc>& out_tools) const override;
  virtual void Execute(WStringView sToolName, const WVariantDictionary& arguments, WMcpToolResult& out_result) override;

private:
  void ExecuteInfo(const WVariantDictionary& arguments, WMcpToolResult& out_result);
  void ExecuteCreate(const WVariantDictionary& arguments, WMcpToolResult& out_result);
};
