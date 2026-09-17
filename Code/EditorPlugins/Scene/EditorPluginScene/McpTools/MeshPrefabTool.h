#pragma once

#include <Mcp/McpTool.h>

/// Creates prefabs from mesh assets, which is otherwise only reachable through the asset browser's
/// context menu and its dialog.
///
/// Split in two because the options that make sense depend on the mesh: a box collider needs bounds,
/// a triangle mesh collider cannot be used for a dynamic actor, and without the Jolt plugin none of
/// them exist. 'mesh_prefab_info' reports that for one mesh, 'mesh_prefab_create' then acts on it.
/// Calling create directly is fine when the defaults are wanted.
class WMcpMeshPrefabTool : public WMcpToolProvider
{
  W_ADD_DYNAMIC_REFLECTION(WMcpMeshPrefabTool, WMcpToolProvider);

public:
  virtual void GetSupportedTools(WDynamicArray<WMcpToolDesc>& out_tools) const override;
  virtual void Execute(WStringView sToolName, const WVariantDictionary& arguments, WMcpToolResult& out_result) override;

private:
  void ExecuteInfo(const WVariantDictionary& arguments, WMcpToolResult& out_result);
  void ExecuteCreate(const WVariantDictionary& arguments, WMcpToolResult& out_result);
};
