#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenNodePins.h>
#include <Foundation/Reflection/Reflection.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WProcGenNodePin, WNoBase, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WProcGenNodeInputPin, WProcGenNodePin, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WProcGenNodeOutputPin, WProcGenNodePin, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;
// clang-format on
