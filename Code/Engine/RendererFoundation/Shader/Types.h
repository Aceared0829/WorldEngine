#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Math/Mat3.h>
#include <Foundation/Math/Transform.h>

/// A wrapper class that converts a WMat3 into the correct data layout for shaders.
class WShaderMat3
{
public:
  W_DECLARE_POD_TYPE();

  W_RENDERERFOUNDATION_DLL static bool TransposeShaderMatrices /*= false*/;

  W_ALWAYS_INLINE WShaderMat3() = default;

  W_ALWAYS_INLINE WShaderMat3(const WMat3& m) { *this = m; }

  W_FORCE_INLINE void operator=(const WMat3& m)
  {
    if (WShaderMat3::TransposeShaderMatrices)
    {
      m_Data[0] = m.m_fElementsCM[0];
      m_Data[1] = m.m_fElementsCM[3];
      m_Data[2] = m.m_fElementsCM[6];
      m_Data[3] = 0.0f;

      m_Data[4] = m.m_fElementsCM[1];
      m_Data[5] = m.m_fElementsCM[4];
      m_Data[6] = m.m_fElementsCM[7];
      m_Data[7] = 0.0f;

      m_Data[8] = m.m_fElementsCM[2];
      m_Data[9] = m.m_fElementsCM[5];
      m_Data[10] = m.m_fElementsCM[8];
      m_Data[11] = 0.0f;
    }
    else
    {
      WMemoryUtils::Copy(&m_Data[0], &m.m_fElementsCM[0], 3);
      m_Data[3] = 0.0f;

      WMemoryUtils::Copy(&m_Data[4], &m.m_fElementsCM[3], 3);
      m_Data[7] = 0.0f;

      WMemoryUtils::Copy(&m_Data[8], &m.m_fElementsCM[6], 3);
      m_Data[11] = 0.0f;
    }
  }

private:
  float m_Data[12];
};

/// A wrapper class that converts a WMat4 into the correct data layout for shaders.
class WShaderMat4
{
public:
  W_DECLARE_POD_TYPE();

  W_ALWAYS_INLINE WShaderMat4() = default;

  W_ALWAYS_INLINE WShaderMat4(const WMat4& m) { *this = m; }

  W_FORCE_INLINE void operator=(const WMat4& m)
  {
    if (WShaderMat3::TransposeShaderMatrices)
    {
      for (WUInt32 c = 0; c < 4; ++c)
      {
        m_Data[c * 4 + 0] = m.Element(0, c);
        m_Data[c * 4 + 1] = m.Element(1, c);
        m_Data[c * 4 + 2] = m.Element(2, c);
        m_Data[c * 4 + 3] = m.Element(3, c);
      }
    }
    else
    {
      WMemoryUtils::Copy(m_Data, m.m_fElementsCM, 16);
    }
  }

private:
  float m_Data[16];
};

/// A wrapper class that converts a WTransform into the correct data layout for shaders.
class WShaderTransform
{
public:
  W_DECLARE_POD_TYPE();

  W_ALWAYS_INLINE WShaderTransform() = default;

  inline void operator=(const WTransform& t) { *this = t.GetAsMat4(); }

  inline void operator=(const WMat4& t)
  {
    float data[16];

    if (WShaderMat3::TransposeShaderMatrices)
      t.GetAsArray(data, WMatrixLayout::ColumnMajor);
    else
      t.GetAsArray(data, WMatrixLayout::RowMajor);

    WMemoryUtils::Copy(&m_Data[0], &data[0], 12);
  }

  inline void operator=(const WMat3& t)
  {
    float data[9];

    if (WShaderMat3::TransposeShaderMatrices)
      t.GetAsArray(data, WMatrixLayout::ColumnMajor);
    else
      t.GetAsArray(data, WMatrixLayout::RowMajor);

    m_Data[0] = data[0];
    m_Data[1] = data[1];
    m_Data[2] = data[2];
    m_Data[3] = 0;

    m_Data[4] = data[3];
    m_Data[5] = data[4];
    m_Data[6] = data[5];
    m_Data[7] = 0;

    m_Data[8] = data[6];
    m_Data[9] = data[7];
    m_Data[10] = data[8];
    m_Data[11] = 0;
  }

  inline WMat4 GetAsMat4() const
  {
    WMat4 res;
    res.SetRow(0, reinterpret_cast<const WVec4&>(m_Data[0]));
    res.SetRow(1, reinterpret_cast<const WVec4&>(m_Data[4]));
    res.SetRow(2, reinterpret_cast<const WVec4&>(m_Data[8]));
    res.SetRow(3, WVec4(0, 0, 0, 1));

    return res;
  }

  inline WVec3 GetTranslationVector() const
  {
    return WVec3(m_Data[3], m_Data[7], m_Data[11]);
  }

private:
  float m_Data[12];
};

/// A wrapper class that converts a bool into the correct data layout for shaders.
class WShaderBool
{
public:
  W_DECLARE_POD_TYPE();

  W_ALWAYS_INLINE WShaderBool() = default;

  W_ALWAYS_INLINE WShaderBool(bool b) { m_uiData = b ? 0xFFFFFFFF : 0; }

  W_ALWAYS_INLINE void operator=(bool b) { m_uiData = b ? 0xFFFFFFFF : 0; }

private:
  WUInt32 m_uiData;
};
