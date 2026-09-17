#include <Foundation/FoundationPCH.h>

#include <Foundation/Reflection/Implementation/DynamicRTTI.h>
#include <Foundation/Reflection/Implementation/RTTI.h>

bool WReflectedClass::IsInstanceOf(const WRTTI* pType) const
{
  return GetDynamicRTTI()->IsDerivedFrom(pType);
}
