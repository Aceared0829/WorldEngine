#pragma once

#include <Foundation/Math/Vec2.h>
#include <Foundation/Reflection/Reflection.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

/// A comment box that can be placed in any visual graph to annotate groups of nodes.
///
/// Comments are treated as special nodes by WVisualGraphObjectManager: they participate in
/// position tracking and serialization but have no pins and cannot be connected.
/// Their size and color are document properties editable through the property panel.
class W_TOOLSFOUNDATION_DLL WVisualGraphComment : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WVisualGraphComment, WReflectedClass);

public:
  WString m_sComment = "Comment";
  WVec2 m_vSize = WVec2(300, 200);
  WColorGammaUB m_Color = WColorGammaUB(70, 70, 70, 200);
};
