#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/Frustum.h>
#include <Foundation/Math/Vec3.h>
#include <Utilities/UtilitiesDLL.h>

struct WDynamicTree
{
  struct WObjectData
  {
    WInt32 m_iObjectType;
    WInt32 m_iObjectInstance;
  };

  struct WMultiMapKey
  {
    WUInt32 m_uiKey;
    WUInt32 m_uiCounter;

    WMultiMapKey()
    {
      m_uiKey = 0;
      m_uiCounter = 0;
    }

    inline bool operator<(const WMultiMapKey& rhs) const
    {
      if (m_uiKey == rhs.m_uiKey)
        return m_uiCounter < rhs.m_uiCounter;

      return m_uiKey < rhs.m_uiKey;
    }

    inline bool operator==(const WMultiMapKey& rhs) const { return (m_uiCounter == rhs.m_uiCounter && m_uiKey == rhs.m_uiKey); }
  };
};

using WDynamicTreeObject = WMap<WDynamicTree::WMultiMapKey, WDynamicTree::WObjectData>::Iterator;
using WDynamicTreeObjectConst = WMap<WDynamicTree::WMultiMapKey, WDynamicTree::WObjectData>::ConstIterator;

/// Callback type for object queries. Return "false" to abort a search (e.g. when the desired element has been found).
using W_VISIBLE_OBJ_CALLBACK = bool (*)(void*, WDynamicTreeObjectConst);

class WDynamicOctree;
class WDynamicQuadtree;
