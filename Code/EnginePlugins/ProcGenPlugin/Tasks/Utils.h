#pragma once

#include <ProcGenPlugin/ProcGenPluginDLL.h>

#include <Foundation/CodeUtils/Expression/ExpressionDeclarations.h>

class WVolumeCollection;
class WWorld;

namespace WProcGenInternal
{
  struct Output;
}

struct W_PROCGENPLUGIN_DLL WProcGenExpressionFunctions
{
  static WExpressionFunction s_ApplyVolumesFunc;
  static WExpressionFunction s_GetInstanceSeedFunc;
};

class W_PROCGENPLUGIN_DLL WProcGenGlobalData
{
public:
  static void ExtractVolumeCollections(const WWorld& world, const WBoundingBox& box, const WProcGenInternal::Output& output, WDeque<WVolumeCollection>& ref_volumeCollections, WExpression::GlobalData& ref_globalData);

  static void SetInstanceSeed(WUInt32 uiSeed, WExpression::GlobalData& ref_globalData);

  static void SetCurves(const WProcGenInternal::Output& output, WExpression::GlobalData& ref_globalData);
};
