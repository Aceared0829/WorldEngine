#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/VarianceTypes.h>
#include <Foundation/Types/VariantTypeRegistry.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WVarianceTypeBase, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Variance", m_fVariance)
  }
    W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WVarianceTypeFloat, WVarianceTypeBase, 1, WRTTIDefaultAllocator<WVarianceTypeFloat>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Value", m_Value)
  }
    W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WVarianceTypeTime, WVarianceTypeBase, 1, WRTTIDefaultAllocator<WVarianceTypeTime>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Value", m_Value)
  }
    W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WVarianceTypeAngle, WVarianceTypeBase, 1, WRTTIDefaultAllocator<WVarianceTypeAngle>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Value", m_Value)
  }
    W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

W_DEFINE_CUSTOM_VARIANT_TYPE(WVarianceTypeFloat);
W_DEFINE_CUSTOM_VARIANT_TYPE(WVarianceTypeTime);
W_DEFINE_CUSTOM_VARIANT_TYPE(WVarianceTypeAngle);

W_STATICLINK_FILE(Foundation, Foundation_Types_Implementation_VarianceTypes);
