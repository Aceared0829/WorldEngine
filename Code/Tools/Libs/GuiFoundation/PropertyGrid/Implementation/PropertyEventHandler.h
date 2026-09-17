#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/Declarations.h>

class WQtPropertyGridWidget;
class WAbstractProperty;

struct WPropertyEvent
{
  enum class Type
  {
    SingleValueChanged,
    BeginTemporary,
    EndTemporary,
    CancelTemporary,
  };

  Type m_Type;
  const WAbstractProperty* m_pProperty;
  const WHybridArray<WPropertySelection, 8>* m_pItems;
  WVariant m_Value;
};
