#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdTransformd.h>

W_CREATE_SIMPLE_TEST(SimdMath, SimdMat4d)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "MakeFromColumnMajorArray / MakeFromRowMajorArray")
  {
    const double data[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

    {
      WSimdMat4d m = WSimdMat4d::MakeFromColumnMajorArray(data);

      W_TEST_BOOL((m.m_col0 == WSimdVec4d(1, 2, 3, 4)).AllSet());
      W_TEST_BOOL((m.m_col1 == WSimdVec4d(5, 6, 7, 8)).AllSet());
      W_TEST_BOOL((m.m_col2 == WSimdVec4d(9, 10, 11, 12)).AllSet());
      W_TEST_BOOL((m.m_col3 == WSimdVec4d(13, 14, 15, 16)).AllSet());
    }

    {
      WSimdMat4d m = WSimdMat4d::MakeFromRowMajorArray(data);

      W_TEST_BOOL((m.m_col0 == WSimdVec4d(1, 5, 9, 13)).AllSet());
      W_TEST_BOOL((m.m_col1 == WSimdVec4d(2, 6, 10, 14)).AllSet());
      W_TEST_BOOL((m.m_col2 == WSimdVec4d(3, 7, 11, 15)).AllSet());
      W_TEST_BOOL((m.m_col3 == WSimdVec4d(4, 8, 12, 16)).AllSet());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeFromColumns")
  {
    WSimdVec4d c0(1, 2, 3, 4);
    WSimdVec4d c1(5, 6, 7, 8);
    WSimdVec4d c2(9, 10, 11, 12);
    WSimdVec4d c3(13, 14, 15, 16);

    WSimdMat4d m = WSimdMat4d::MakeFromColumns(c0, c1, c2, c3);

    W_TEST_BOOL((m.m_col0 == WSimdVec4d(1, 2, 3, 4)).AllSet());
    W_TEST_BOOL((m.m_col1 == WSimdVec4d(5, 6, 7, 8)).AllSet());
    W_TEST_BOOL((m.m_col2 == WSimdVec4d(9, 10, 11, 12)).AllSet());
    W_TEST_BOOL((m.m_col3 == WSimdVec4d(13, 14, 15, 16)).AllSet());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetFromArray")
  {
    const double data[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

    {
      WSimdMat4d m = WSimdMat4d::MakeFromColumnMajorArray(data);

      W_TEST_BOOL((m.m_col0 == WSimdVec4d(1, 2, 3, 4)).AllSet());
      W_TEST_BOOL((m.m_col1 == WSimdVec4d(5, 6, 7, 8)).AllSet());
      W_TEST_BOOL((m.m_col2 == WSimdVec4d(9, 10, 11, 12)).AllSet());
      W_TEST_BOOL((m.m_col3 == WSimdVec4d(13, 14, 15, 16)).AllSet());
    }

    {
      WSimdMat4d m = WSimdMat4d::MakeFromRowMajorArray(data);

      W_TEST_BOOL((m.m_col0 == WSimdVec4d(1, 5, 9, 13)).AllSet());
      W_TEST_BOOL((m.m_col1 == WSimdVec4d(2, 6, 10, 14)).AllSet());
      W_TEST_BOOL((m.m_col2 == WSimdVec4d(3, 7, 11, 15)).AllSet());
      W_TEST_BOOL((m.m_col3 == WSimdVec4d(4, 8, 12, 16)).AllSet());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetAsArray")
  {
    WSimdMat4d m = WSimdMat4d::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    double data[16];

    m.GetAsArray(data, WMatrixLayout::ColumnMajor);
    W_TEST_DOUBLE(data[0], 1, 0.0001f);
    W_TEST_DOUBLE(data[1], 5, 0.0001f);
    W_TEST_DOUBLE(data[2], 9, 0.0001f);
    W_TEST_DOUBLE(data[3], 13, 0.0001f);
    W_TEST_DOUBLE(data[4], 2, 0.0001f);
    W_TEST_DOUBLE(data[5], 6, 0.0001f);
    W_TEST_DOUBLE(data[6], 10, 0.0001f);
    W_TEST_DOUBLE(data[7], 14, 0.0001f);
    W_TEST_DOUBLE(data[8], 3, 0.0001f);
    W_TEST_DOUBLE(data[9], 7, 0.0001f);
    W_TEST_DOUBLE(data[10], 11, 0.0001f);
    W_TEST_DOUBLE(data[11], 15, 0.0001f);
    W_TEST_DOUBLE(data[12], 4, 0.0001f);
    W_TEST_DOUBLE(data[13], 8, 0.0001f);
    W_TEST_DOUBLE(data[14], 12, 0.0001f);
    W_TEST_DOUBLE(data[15], 16, 0.0001f);

    m.GetAsArray(data, WMatrixLayout::RowMajor);
    W_TEST_DOUBLE(data[0], 1, 0.0001f);
    W_TEST_DOUBLE(data[1], 2, 0.0001f);
    W_TEST_DOUBLE(data[2], 3, 0.0001f);
    W_TEST_DOUBLE(data[3], 4, 0.0001f);
    W_TEST_DOUBLE(data[4], 5, 0.0001f);
    W_TEST_DOUBLE(data[5], 6, 0.0001f);
    W_TEST_DOUBLE(data[6], 7, 0.0001f);
    W_TEST_DOUBLE(data[7], 8, 0.0001f);
    W_TEST_DOUBLE(data[8], 9, 0.0001f);
    W_TEST_DOUBLE(data[9], 10, 0.0001f);
    W_TEST_DOUBLE(data[10], 11, 0.0001f);
    W_TEST_DOUBLE(data[11], 12, 0.0001f);
    W_TEST_DOUBLE(data[12], 13, 0.0001f);
    W_TEST_DOUBLE(data[13], 14, 0.0001f);
    W_TEST_DOUBLE(data[14], 15, 0.0001f);
    W_TEST_DOUBLE(data[15], 16, 0.0001f);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeIdentity")
  {
    WSimdMat4d m = WSimdMat4d::MakeIdentity();

    W_TEST_BOOL((m.m_col0 == WSimdVec4d(1, 0, 0, 0)).AllSet());
    W_TEST_BOOL((m.m_col1 == WSimdVec4d(0, 1, 0, 0)).AllSet());
    W_TEST_BOOL((m.m_col2 == WSimdVec4d(0, 0, 1, 0)).AllSet());
    W_TEST_BOOL((m.m_col3 == WSimdVec4d(0, 0, 0, 1)).AllSet());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeZero")
  {
    WSimdMat4d m = WSimdMat4d::MakeZero();

    W_TEST_BOOL((m.m_col0 == WSimdVec4d(0, 0, 0, 0)).AllSet());
    W_TEST_BOOL((m.m_col1 == WSimdVec4d(0, 0, 0, 0)).AllSet());
    W_TEST_BOOL((m.m_col2 == WSimdVec4d(0, 0, 0, 0)).AllSet());
    W_TEST_BOOL((m.m_col3 == WSimdVec4d(0, 0, 0, 0)).AllSet());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Transpose")
  {
    WSimdMat4d m = WSimdMat4d::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    m.Transpose();

    W_TEST_BOOL((m.m_col0 == WSimdVec4d(1, 2, 3, 4)).AllSet());
    W_TEST_BOOL((m.m_col1 == WSimdVec4d(5, 6, 7, 8)).AllSet());
    W_TEST_BOOL((m.m_col2 == WSimdVec4d(9, 10, 11, 12)).AllSet());
    W_TEST_BOOL((m.m_col3 == WSimdVec4d(13, 14, 15, 16)).AllSet());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetTranspose")
  {
    WSimdMat4d m0 = WSimdMat4d::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    WSimdMat4d m = m0.GetTranspose();

    W_TEST_BOOL((m.m_col0 == WSimdVec4d(1, 2, 3, 4)).AllSet());
    W_TEST_BOOL((m.m_col1 == WSimdVec4d(5, 6, 7, 8)).AllSet());
    W_TEST_BOOL((m.m_col2 == WSimdVec4d(9, 10, 11, 12)).AllSet());
    W_TEST_BOOL((m.m_col3 == WSimdVec4d(13, 14, 15, 16)).AllSet());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Invert")
  {
    for (double x = 1.0f; x < 360.0f; x += 20.0f)
    {
      for (double y = 2.0f; y < 360.0f; y += 27.0f)
      {
        for (double z = 3.0f; z < 360.0f; z += 33.0f)
        {
          WSimdQuatd q = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(x, y, z).GetNormalized<3>(), WAngle::MakeFromDegree(19.0f));

          WSimdTransformd t(q);

          WSimdMat4d m, inv;
          m = t.GetAsMat4();
          inv = m;
          W_TEST_BOOL(inv.Invert() == W_SUCCESS);

          WSimdVec4d v = m.TransformDirection(WSimdVec4d(1, 3, -10));
          WSimdVec4d vinv = inv.TransformDirection(v);

          W_TEST_BOOL(vinv.IsEqual(WSimdVec4d(1, 3, -10), WMath::DefaultEpsilon<double>()).AllSet<3>());
        }
      }
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetInverse")
  {
    for (double x = 1.0f; x < 360.0f; x += 19.0f)
    {
      for (double y = 2.0f; y < 360.0f; y += 29.0f)
      {
        for (double z = 3.0f; z < 360.0f; z += 31.0f)
        {
          WSimdQuatd q = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(x, y, z).GetNormalized<3>(), WAngle::MakeFromDegree(83.0f));

          WSimdTransformd t(q);

          WSimdMat4d m, inv;
          m = t.GetAsMat4();
          inv = m.GetInverse();

          WSimdVec4d v = m.TransformDirection(WSimdVec4d(1, 3, -10));
          WSimdVec4d vinv = inv.TransformDirection(v);

          W_TEST_BOOL(vinv.IsEqual(WSimdVec4d(1, 3, -10), WMath::DefaultEpsilon<double>()).AllSet<3>());
        }
      }
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsEqual")
  {
    WSimdMat4d m = WSimdMat4d::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    WSimdMat4d m2 = m;

    W_TEST_BOOL(m.IsEqual(m2, 0.0001f));

    m2.m_col0 += WSimdVec4d(0.00001f);
    W_TEST_BOOL(m.IsEqual(m2, 0.0001f));
    W_TEST_BOOL(!m.IsEqual(m2, 0.000001f));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsIdentity")
  {
    WSimdMat4d m = WSimdMat4d::MakeIdentity();

    W_TEST_BOOL(m.IsIdentity());

    m.m_col0.SetZero();
    W_TEST_BOOL(!m.IsIdentity());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsValid")
  {
    WSimdMat4d m = WSimdMat4d::MakeIdentity();

    W_TEST_BOOL(m.IsValid());

    m.m_col0.SetX(WMath::NaN<double>());
    W_TEST_BOOL(!m.IsValid());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsNaN")
  {
    WSimdMat4d m = WSimdMat4d::MakeIdentity();

    W_TEST_BOOL(!m.IsNaN());

    double data[16];

    for (WUInt32 i = 0; i < 16; ++i)
    {
      m = WSimdMat4d::MakeIdentity();
      m.GetAsArray(data, WMatrixLayout::ColumnMajor);
      data[i] = WMath::NaN<double>();
      m = WSimdMat4d::MakeFromColumnMajorArray(data);

      W_TEST_BOOL(m.IsNaN());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetRows")
  {
    WSimdVec4d r0(1, 2, 3, 4);
    WSimdVec4d r1(5, 6, 7, 8);
    WSimdVec4d r2(9, 10, 11, 12);
    WSimdVec4d r3(13, 14, 15, 16);

    WSimdMat4d m;
    m.SetRows(r0, r1, r2, r3);

    W_TEST_BOOL((m.m_col0 == WSimdVec4d(1, 5, 9, 13)).AllSet());
    W_TEST_BOOL((m.m_col1 == WSimdVec4d(2, 6, 10, 14)).AllSet());
    W_TEST_BOOL((m.m_col2 == WSimdVec4d(3, 7, 11, 15)).AllSet());
    W_TEST_BOOL((m.m_col3 == WSimdVec4d(4, 8, 12, 16)).AllSet());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetRows")
  {
    WSimdMat4d m = WSimdMat4d::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    WSimdVec4d r0, r1, r2, r3;
    m.GetRows(r0, r1, r2, r3);

    W_TEST_BOOL((r0 == WSimdVec4d(1, 2, 3, 4)).AllSet());
    W_TEST_BOOL((r1 == WSimdVec4d(5, 6, 7, 8)).AllSet());
    W_TEST_BOOL((r2 == WSimdVec4d(9, 10, 11, 12)).AllSet());
    W_TEST_BOOL((r3 == WSimdVec4d(13, 14, 15, 16)).AllSet());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "TransformPosition")
  {
    WSimdMat4d m = WSimdMat4d::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    const WSimdVec4d r = m.TransformPosition(WSimdVec4d(1, 2, 3));

    W_TEST_BOOL(r.IsEqual(WSimdVec4d(1 * 1 + 2 * 2 + 3 * 3 + 4, 1 * 5 + 2 * 6 + 3 * 7 + 8, 1 * 9 + 2 * 10 + 3 * 11 + 12), 0.0001f).AllSet<3>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "TransformDirection")
  {
    WSimdMat4d m = WSimdMat4d::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    const WSimdVec4d r = m.TransformDirection(WSimdVec4d(1, 2, 3));

    W_TEST_BOOL(r.IsEqual(WSimdVec4d(1 * 1 + 2 * 2 + 3 * 3, 1 * 5 + 2 * 6 + 3 * 7, 1 * 9 + 2 * 10 + 3 * 11), 0.0001f).AllSet<3>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator*(mat, mat)")
  {
    WSimdMat4d m1 = WSimdMat4d::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    WSimdMat4d m2 = WSimdMat4d::MakeFromValues(-1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -14, -15, -16);

    WSimdMat4d r = m1 * m2;

    W_TEST_BOOL((r.m_col0 == WSimdVec4d(-1 * 1 + -5 * 2 + -9 * 3 + -13 * 4, -1 * 5 + -5 * 6 + -9 * 7 + -13 * 8,
                                -1 * 9 + -5 * 10 + -9 * 11 + -13 * 12, -1 * 13 + -5 * 14 + -9 * 15 + -13 * 16))
                   .AllSet());
    W_TEST_BOOL((r.m_col1 == WSimdVec4d(-2 * 1 + -6 * 2 + -10 * 3 + -14 * 4, -2 * 5 + -6 * 6 + -10 * 7 + -14 * 8,
                                -2 * 9 + -6 * 10 + -10 * 11 + -14 * 12, -2 * 13 + -6 * 14 + -10 * 15 + -14 * 16))
                   .AllSet());
    W_TEST_BOOL((r.m_col2 == WSimdVec4d(-3 * 1 + -7 * 2 + -11 * 3 + -15 * 4, -3 * 5 + -7 * 6 + -11 * 7 + -15 * 8,
                                -3 * 9 + -7 * 10 + -11 * 11 + -15 * 12, -3 * 13 + -7 * 14 + -11 * 15 + -15 * 16))
                   .AllSet());
    W_TEST_BOOL((r.m_col3 == WSimdVec4d(-4 * 1 + -8 * 2 + -12 * 3 + -16 * 4, -4 * 5 + -8 * 6 + -12 * 7 + -16 * 8,
                                -4 * 9 + -8 * 10 + -12 * 11 + -16 * 12, -4 * 13 + -8 * 14 + -12 * 15 + -16 * 16))
                   .AllSet());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator== (mat, mat) | operator!= (mat, mat)")
  {
    WSimdMat4d m = WSimdMat4d::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    WSimdMat4d m2 = m;

    W_TEST_BOOL(m == m2);

    m2.m_col0 += WSimdVec4d(0.00001f);

    W_TEST_BOOL(m != m2);
  }
}
