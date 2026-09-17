#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Mat4.h>
#include <Foundation/SimdMath/SimdMat4f.h>

///\todo optimize

WResult WSimdMat4f::Invert(const WSimdFloat& fEpsilon)
{
  WMat4 tmp;
  GetAsArray(tmp.m_fElementsCM, WMatrixLayout::ColumnMajor);

  if (tmp.Invert(fEpsilon).Failed())
    return W_FAILURE;

  *this = WSimdMat4f::MakeFromColumnMajorArray(tmp.m_fElementsCM);

  return W_SUCCESS;
}
