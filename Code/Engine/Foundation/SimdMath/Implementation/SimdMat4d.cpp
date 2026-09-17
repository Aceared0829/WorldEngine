#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Mat4.h>
#include <Foundation/SimdMath/SimdMat4d.h>

///\todo optimize

WResult WSimdMat4d::Invert(const WSimdDouble& fEpsilon)
{
  WMat4d tmp;
  GetAsArray(tmp.m_fElementsCM, WMatrixLayout::ColumnMajor);

  if (tmp.Invert(fEpsilon).Failed())
    return W_FAILURE;

  *this = WSimdMat4d::MakeFromColumnMajorArray(tmp.m_fElementsCM);

  return W_SUCCESS;
}
