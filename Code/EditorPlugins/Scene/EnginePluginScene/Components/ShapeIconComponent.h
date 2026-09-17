#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>

using WShapeIconComponentManager = WComponentManager<class WShapeIconComponent, WBlockStorageType::Compact>;

/// This is a dummy component that the editor creates on all 'empty' nodes for the sole purpose to render a shape icon and enable picking.
///
/// Though in the future one could potentially use them for other editor functionality, such as displaying the object name or some other useful text.
class W_ENGINEPLUGINSCENE_DLL WShapeIconComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WShapeIconComponent, WComponent, WShapeIconComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WShapeIconComponent

public:
  WShapeIconComponent();
  ~WShapeIconComponent();
};
