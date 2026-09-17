#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <RmlUi/Include/RmlUi/Core.h>

#include <Foundation/Basics.h>

namespace Rml
{
  class Context;
}

class W_RMLUIPLUGIN_DLL WRmlUiDataBinding
{
public:
  virtual ~WRmlUiDataBinding() = default;

  virtual WResult Initialize(Rml::Context& ref_context) = 0;
  virtual void Deinitialize(Rml::Context& ref_context) = 0;

  /// Returns true if anything was updated
  virtual bool Update() = 0;
};
