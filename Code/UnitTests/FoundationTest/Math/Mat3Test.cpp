#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Implementation/AllClasses_inl.h>
#include <Foundation/Math/Mat3.h>

template <typename Type>
void TestMat3()
{
  using WMat3Type = WMat3Template<Type>;
  using WVec3Type = WVec3Template<Type>;

  W_TEST_BLOCK(WTestBlock::Enabled, "Default Constructor")
  {
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
    if (WMath::SupportsNaN<Type>())
    {
      // In debug the default constructor initializes everything with NaN.
      WMat3Type m;
      W_TEST_BOOL(WMath::IsNaN(m.m_fElementsCM[0]) && WMath::IsNaN(m.m_fElementsCM[1]) && WMath::IsNaN(m.m_fElementsCM[2]) &&
                   WMath::IsNaN(m.m_fElementsCM[3]) && WMath::IsNaN(m.m_fElementsCM[4]) && WMath::IsNaN(m.m_fElementsCM[5]) &&
                   WMath::IsNaN(m.m_fElementsCM[6]) && WMath::IsNaN(m.m_fElementsCM[7]) && WMath::IsNaN(m.m_fElementsCM[8]));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    Type testBlock[9] = {(Type)1, (Type)2, (Type)3, (Type)4, (Type)5, (Type)6, (Type)7, (Type)8, (Type)9};

    WMat3Type* m = ::new ((void*)&testBlock[0]) WMat3Type;

    W_TEST_BOOL(m->m_fElementsCM[0] == (Type)1 && m->m_fElementsCM[1] == (Type)2 && m->m_fElementsCM[2] == (Type)3 &&
                 m->m_fElementsCM[3] == (Type)4 && m->m_fElementsCM[4] == (Type)5 && m->m_fElementsCM[5] == (Type)6 &&
                 m->m_fElementsCM[6] == (Type)7 && m->m_fElementsCM[7] == (Type)8 && m->m_fElementsCM[8] == (Type)9);
#endif
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor (Array Data)")
  {
    const Type data[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    {
      WMat3Type m = WMat3Type::MakeFromColumnMajorArray(data);

      W_TEST_BOOL(m.m_fElementsCM[0] == (Type)1 && m.m_fElementsCM[1] == (Type)2 && m.m_fElementsCM[2] == (Type)3 &&
                   m.m_fElementsCM[3] == (Type)4 && m.m_fElementsCM[4] == (Type)5 && m.m_fElementsCM[5] == (Type)6 &&
                   m.m_fElementsCM[6] == (Type)7 && m.m_fElementsCM[7] == (Type)8 && m.m_fElementsCM[8] == (Type)9);
    }

    {
      WMat3Type m = WMat3Type::MakeFromRowMajorArray(data);

      W_TEST_BOOL(m.m_fElementsCM[0] == (Type)1 && m.m_fElementsCM[1] == (Type)4 && m.m_fElementsCM[2] == (Type)7 &&
                   m.m_fElementsCM[3] == (Type)2 && m.m_fElementsCM[4] == (Type)5 && m.m_fElementsCM[5] == (Type)8 &&
                   m.m_fElementsCM[6] == (Type)3 && m.m_fElementsCM[7] == (Type)6 && m.m_fElementsCM[8] == (Type)9);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor (Elements)")
  {
    WMat3Type m = WMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    
        W_TEST_FLOAT(m.Element(0, 0), 1, WMath::DefaultEpsilon<Type>());
        W_TEST_FLOAT(m.Element(1, 0), 2, WMath::DefaultEpsilon<Type>());
        W_TEST_FLOAT(m.Element(2, 0), 3, WMath::DefaultEpsilon<Type>());
        W_TEST_FLOAT(m.Element(0, 1), 4, WMath::DefaultEpsilon<Type>());
        W_TEST_FLOAT(m.Element(1, 1), 5, WMath::DefaultEpsilon<Type>());
        W_TEST_FLOAT(m.Element(2, 1), 6, WMath::DefaultEpsilon<Type>());
        W_TEST_FLOAT(m.Element(0, 2), 7, WMath::DefaultEpsilon<Type>());
        W_TEST_FLOAT(m.Element(1, 2), 8, WMath::DefaultEpsilon<Type>());
        W_TEST_FLOAT(m.Element(2, 2), 9, WMath::DefaultEpsilon<Type>());
      }
  W_TEST_BLOCK(WTestBlock::Enabled, "SetFromArray")
  {
    const Type data[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    {
      WMat3Type m = WMat3Type::MakeFromColumnMajorArray(data);

      W_TEST_BOOL(m.m_fElementsCM[0] == (Type)1 && m.m_fElementsCM[1] == (Type)2 && m.m_fElementsCM[2] == (Type)3 &&
                   m.m_fElementsCM[3] == (Type)4 && m.m_fElementsCM[4] == (Type)5 && m.m_fElementsCM[5] == (Type)6 &&
                   m.m_fElementsCM[6] == (Type)7 && m.m_fElementsCM[7] == (Type)8 && m.m_fElementsCM[8] == (Type)9);
    }

    {
      WMat3Type m = WMat3Type::MakeFromRowMajorArray(data);

      W_TEST_BOOL(m.m_fElementsCM[0] == (Type)1 && m.m_fElementsCM[1] == (Type)4 && m.m_fElementsCM[2] == (Type)7 &&
                   m.m_fElementsCM[3] == (Type)2 && m.m_fElementsCM[4] == (Type)5 && m.m_fElementsCM[5] == (Type)8 &&
                   m.m_fElementsCM[6] == (Type)3 && m.m_fElementsCM[7] == (Type)6 && m.m_fElementsCM[8] == (Type)9);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetElements")
  {
    WMat3Type m = WMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    
        W_TEST_FLOAT(m.Element(0, 0), 1, WMath::DefaultEpsilon<Type>());
        W_TEST_FLOAT(m.Element(1, 0), 2, WMath::DefaultEpsilon<Type>());
        W_TEST_FLOAT(m.Element(2, 0), 3, WMath::DefaultEpsilon<Type>());
        W_TEST_FLOAT(m.Element(0, 1), 4, WMath::DefaultEpsilon<Type>());
        W_TEST_FLOAT(m.Element(1, 1), 5, WMath::DefaultEpsilon<Type>());
        W_TEST_FLOAT(m.Element(2, 1), 6, WMath::DefaultEpsilon<Type>());
        W_TEST_FLOAT(m.Element(0, 2), 7, WMath::DefaultEpsilon<Type>());
        W_TEST_FLOAT(m.Element(1, 2), 8, WMath::DefaultEpsilon<Type>());
        W_TEST_FLOAT(m.Element(2, 2), 9, WMath::DefaultEpsilon<Type>());
      }
  W_TEST_BLOCK(WTestBlock::Enabled, "GetAsArray")
  {
    WMat3Type m = WMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    Type data[9];

    m.GetAsArray(data, WMatrixLayout::ColumnMajor);
    W_TEST_FLOAT(data[0], 1, WMath::LargeEpsilon<Type>());
    W_TEST_FLOAT(data[1], 4, WMath::LargeEpsilon<Type>());
    W_TEST_FLOAT(data[2], 7, WMath::LargeEpsilon<Type>());
    W_TEST_FLOAT(data[3], 2, WMath::LargeEpsilon<Type>());
    W_TEST_FLOAT(data[4], 5, WMath::LargeEpsilon<Type>());
    W_TEST_FLOAT(data[5], 8, WMath::LargeEpsilon<Type>());
    W_TEST_FLOAT(data[6], 3, WMath::LargeEpsilon<Type>());
    W_TEST_FLOAT(data[7], 6, WMath::LargeEpsilon<Type>());
    W_TEST_FLOAT(data[8], 9, WMath::LargeEpsilon<Type>());

    m.GetAsArray(data, WMatrixLayout::RowMajor);
    W_TEST_FLOAT(data[0], 1, WMath::LargeEpsilon<Type>());
    W_TEST_FLOAT(data[1], 2, WMath::LargeEpsilon<Type>());
    W_TEST_FLOAT(data[2], 3, WMath::LargeEpsilon<Type>());
    W_TEST_FLOAT(data[3], 4, WMath::LargeEpsilon<Type>());
    W_TEST_FLOAT(data[4], 5, WMath::LargeEpsilon<Type>());
    W_TEST_FLOAT(data[5], 6, WMath::LargeEpsilon<Type>());
    W_TEST_FLOAT(data[6], 7, WMath::LargeEpsilon<Type>());
    W_TEST_FLOAT(data[7], 8, WMath::LargeEpsilon<Type>());
    W_TEST_FLOAT(data[8], 9, WMath::LargeEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetZero")
  {
    WMat3Type m;
    m.SetZero();

    for (WUInt32 i = 0; i < 9; ++i)
      W_TEST_FLOAT(m.m_fElementsCM[i], (Type)0, (Type)0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetIdentity")
  {
    WMat3Type m;
    m.SetIdentity();

    W_TEST_FLOAT(m.Element(0, 0), 1, 0);
    W_TEST_FLOAT(m.Element(1, 0), 0, 0);
    W_TEST_FLOAT(m.Element(2, 0), 0, 0);
    W_TEST_FLOAT(m.Element(0, 1), 0, 0);
    W_TEST_FLOAT(m.Element(1, 1), 1, 0);
    W_TEST_FLOAT(m.Element(2, 1), 0, 0);
    W_TEST_FLOAT(m.Element(0, 2), 0, 0);
    W_TEST_FLOAT(m.Element(1, 2), 0, 0);
    W_TEST_FLOAT(m.Element(2, 2), 1, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetScalingMatrix")
  {
    WMat3Type m = WMat3Type::MakeScaling(WVec3Type(2, 3, 4));

    W_TEST_FLOAT(m.Element(0, 0), 2, 0);
    W_TEST_FLOAT(m.Element(1, 0), 0, 0);
    W_TEST_FLOAT(m.Element(2, 0), 0, 0);
    W_TEST_FLOAT(m.Element(0, 1), 0, 0);
    W_TEST_FLOAT(m.Element(1, 1), 3, 0);
    W_TEST_FLOAT(m.Element(2, 1), 0, 0);
    W_TEST_FLOAT(m.Element(0, 2), 0, 0);
    W_TEST_FLOAT(m.Element(1, 2), 0, 0);
    W_TEST_FLOAT(m.Element(2, 2), 4, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetRotationMatrixX")
  {
    WMat3Type m;

    m = WMat3Type::MakeRotationX(WAngleTemplate<Type>::MakeFromDegree(90));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(1, -3, 2), WMath::LargeEpsilon<Type>()));

    m = WMat3Type::MakeRotationX(WAngleTemplate<Type>::MakeFromDegree(180));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(1, -2, -3), WMath::LargeEpsilon<Type>()));

    m = WMat3Type::MakeRotationX(WAngleTemplate<Type>::MakeFromDegree(270));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(1, 3, -2), WMath::LargeEpsilon<Type>()));

    m = WMat3Type::MakeRotationX(WAngleTemplate<Type>::MakeFromDegree(360));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(1, 2, 3), WMath::LargeEpsilon<Type>()));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetRotationMatrixY")
  {
    WMat3Type m;

    m = WMat3Type::MakeRotationY(WAngleTemplate<Type>::MakeFromDegree(90));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(3, 2, -1), WMath::LargeEpsilon<Type>()));

    m = WMat3Type::MakeRotationY(WAngleTemplate<Type>::MakeFromDegree(180));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(-1, 2, -3), WMath::LargeEpsilon<Type>()));

    m = WMat3Type::MakeRotationY(WAngleTemplate<Type>::MakeFromDegree(270));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(-3, 2, 1), WMath::LargeEpsilon<Type>()));

    m = WMat3Type::MakeRotationY(WAngleTemplate<Type>::MakeFromDegree(360));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(1, 2, 3), WMath::LargeEpsilon<Type>()));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetRotationMatrixZ")
  {
    WMat3Type m;

    m = WMat3Type::MakeRotationZ(WAngleTemplate<Type>::MakeFromDegree(90));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(-2, 1, 3), WMath::LargeEpsilon<Type>()));

    m = WMat3Type::MakeRotationZ(WAngleTemplate<Type>::MakeFromDegree(180));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(-1, -2, 3), WMath::LargeEpsilon<Type>()));

    m = WMat3Type::MakeRotationZ(WAngleTemplate<Type>::MakeFromDegree(270));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(2, -1, 3), WMath::LargeEpsilon<Type>()));

    m = WMat3Type::MakeRotationZ(WAngleTemplate<Type>::MakeFromDegree(360));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(1, 2, 3), WMath::LargeEpsilon<Type>()));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetRotationMatrix")
  {
    WMat3Type m;

    m = WMat3Type::MakeAxisRotation(WVec3Type(1, 0, 0), WAngleTemplate<Type>::MakeFromDegree(90));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(1, -3, 2), WMath::DefaultEpsilon<Type>()));

    m = WMat3Type::MakeAxisRotation(WVec3Type(1, 0, 0), WAngleTemplate<Type>::MakeFromDegree(180));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(1, -2, -3), WMath::DefaultEpsilon<Type>()));

    m = WMat3Type::MakeAxisRotation(WVec3Type(1, 0, 0), WAngleTemplate<Type>::MakeFromDegree(270));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(1, 3, -2), WMath::DefaultEpsilon<Type>()));

    m = WMat3Type::MakeAxisRotation(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromDegree(90));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(3, 2, -1), WMath::DefaultEpsilon<Type>()));

    m = WMat3Type::MakeAxisRotation(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromDegree(180));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(-1, 2, -3), WMath::DefaultEpsilon<Type>()));

    m = WMat3Type::MakeAxisRotation(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromDegree(270));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(-3, 2, 1), WMath::DefaultEpsilon<Type>()));

    m = WMat3Type::MakeAxisRotation(WVec3Type(0, 0, 1), WAngleTemplate<Type>::MakeFromDegree(90));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(-2, 1, 3), WMath::DefaultEpsilon<Type>()));

    m = WMat3Type::MakeAxisRotation(WVec3Type(0, 0, 1), WAngleTemplate<Type>::MakeFromDegree(180));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(-1, -2, 3), WMath::DefaultEpsilon<Type>()));

    m = WMat3Type::MakeAxisRotation(WVec3Type(0, 0, 1), WAngleTemplate<Type>::MakeFromDegree(270));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(2, -1, 3), WMath::DefaultEpsilon<Type>()));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeIdentity")
  {
    WMat3Type m = WMat3Type::MakeIdentity();

    W_TEST_FLOAT(m.Element(0, 0), 1, 0);
    W_TEST_FLOAT(m.Element(1, 0), 0, 0);
    W_TEST_FLOAT(m.Element(2, 0), 0, 0);
    W_TEST_FLOAT(m.Element(0, 1), 0, 0);
    W_TEST_FLOAT(m.Element(1, 1), 1, 0);
    W_TEST_FLOAT(m.Element(2, 1), 0, 0);
    W_TEST_FLOAT(m.Element(0, 2), 0, 0);
    W_TEST_FLOAT(m.Element(1, 2), 0, 0);
    W_TEST_FLOAT(m.Element(2, 2), 1, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeZero")
  {
    WMat3Type m = WMat3Type::MakeZero();

    W_TEST_FLOAT(m.Element(0, 0), 0, 0);
    W_TEST_FLOAT(m.Element(1, 0), 0, 0);
    W_TEST_FLOAT(m.Element(2, 0), 0, 0);
    W_TEST_FLOAT(m.Element(0, 1), 0, 0);
    W_TEST_FLOAT(m.Element(1, 1), 0, 0);
    W_TEST_FLOAT(m.Element(2, 1), 0, 0);
    W_TEST_FLOAT(m.Element(0, 2), 0, 0);
    W_TEST_FLOAT(m.Element(1, 2), 0, 0);
    W_TEST_FLOAT(m.Element(2, 2), 0, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Transpose")
  {
    WMat3Type m = WMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    m.Transpose();

    
        W_TEST_FLOAT(m.Element(0, 0), 1, WMath::DefaultEpsilon<Type>());
        W_TEST_FLOAT(m.Element(1, 0), 4, WMath::DefaultEpsilon<Type>());
        W_TEST_FLOAT(m.Element(2, 0), 7, WMath::DefaultEpsilon<Type>());
        W_TEST_FLOAT(m.Element(0, 1), 2, WMath::DefaultEpsilon<Type>());
        W_TEST_FLOAT(m.Element(1, 1), 5, WMath::DefaultEpsilon<Type>());
        W_TEST_FLOAT(m.Element(2, 1), 8, WMath::DefaultEpsilon<Type>());
        W_TEST_FLOAT(m.Element(0, 2), 3, WMath::DefaultEpsilon<Type>());
        W_TEST_FLOAT(m.Element(1, 2), 6, WMath::DefaultEpsilon<Type>());
        W_TEST_FLOAT(m.Element(2, 2), 9, WMath::DefaultEpsilon<Type>());
      }
  W_TEST_BLOCK(WTestBlock::Enabled, "GetTranspose")
  {
    WMat3Type m0 = WMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    WMat3Type m = m0.GetTranspose();

    
        W_TEST_FLOAT(m.Element(0, 0), 1, WMath::DefaultEpsilon<Type>());
        W_TEST_FLOAT(m.Element(1, 0), 4, WMath::DefaultEpsilon<Type>());
        W_TEST_FLOAT(m.Element(2, 0), 7, WMath::DefaultEpsilon<Type>());
        W_TEST_FLOAT(m.Element(0, 1), 2, WMath::DefaultEpsilon<Type>());
        W_TEST_FLOAT(m.Element(1, 1), 5, WMath::DefaultEpsilon<Type>());
        W_TEST_FLOAT(m.Element(2, 1), 8, WMath::DefaultEpsilon<Type>());
        W_TEST_FLOAT(m.Element(0, 2), 3, WMath::DefaultEpsilon<Type>());
        W_TEST_FLOAT(m.Element(1, 2), 6, WMath::DefaultEpsilon<Type>());
        W_TEST_FLOAT(m.Element(2, 2), 9, WMath::DefaultEpsilon<Type>());
      }
  W_TEST_BLOCK(WTestBlock::Enabled, "Invert")
  {
    for (Type x = (Type)1.0; x < (Type)360.0; x += (Type)10.0)
    {
      for (Type y = (Type)2.0; y < (Type)360.0; y += (Type)17.0)
      {
        for (Type z = (Type)3.0; z < (Type)360.0; z += (Type)23.0)
        {
          WMat3Type m, inv;
          m = WMat3Type::MakeAxisRotation(WVec3Type(x, y, z).GetNormalized(), WAngleTemplate<Type>::MakeFromDegree((Type)19.0));
          inv = m;
          W_TEST_BOOL(inv.Invert() == W_SUCCESS);

          WVec3Type v = m * WVec3Type(1, 1, 1);
          WVec3Type vinv = inv * v;

          W_TEST_VEC3(vinv, WVec3Type(1, 1, 1), WMath::DefaultEpsilon<Type>());
        }
      }
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetInverse")
  {
    for (Type x = (Type)1.0; x < (Type)360.0; x += (Type)9.0)
    {
      for (Type y = (Type)2.0; y < (Type)360.0; y += (Type)19.0)
      {
        for (Type z = (Type)3.0; z < (Type)360.0; z += (Type)21.0)
        {
          WMat3Type m, inv;
          m = WMat3Type::MakeAxisRotation(WVec3Type(x, y, z).GetNormalized(), WAngleTemplate<Type>::MakeFromDegree((Type)83.0));
          inv = m.GetInverse();

          WVec3Type v = m * WVec3Type(1, 1, 1);
          WVec3Type vinv = inv * v;

          W_TEST_VEC3(vinv, WVec3Type(1, 1, 1), WMath::DefaultEpsilon<Type>());
        }
      }
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsZero")
  {
    WMat3Type m;

    m.SetIdentity();
    W_TEST_BOOL(!m.IsZero());

    m.SetZero();
    W_TEST_BOOL(m.IsZero());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsIdentity")
  {
    WMat3Type m;

    m.SetIdentity();
    W_TEST_BOOL(m.IsIdentity());

    m.SetZero();
    W_TEST_BOOL(!m.IsIdentity());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsValid")
  {
    if (WMath::SupportsNaN<Type>())
    {
      WMat3Type m;

      m.SetZero();
      W_TEST_BOOL(m.IsValid());

      m.m_fElementsCM[0] = WMath::NaN<Type>();
      W_TEST_BOOL(!m.IsValid());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetRow")
  {
    WMat3Type m = WMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    W_TEST_VEC3(m.GetRow(0), WVec3Type(1, 2, 3), (Type)0);
    W_TEST_VEC3(m.GetRow(1), WVec3Type(4, 5, 6), (Type)0);
    W_TEST_VEC3(m.GetRow(2), WVec3Type(7, 8, 9), (Type)0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetRow")
  {
    WMat3Type m;
    m.SetZero();

    m.SetRow(0, WVec3Type(1, 2, 3));
    W_TEST_VEC3(m.GetRow(0), WVec3Type(1, 2, 3), (Type)0);

    m.SetRow(1, WVec3Type(4, 5, 6));
    W_TEST_VEC3(m.GetRow(1), WVec3Type(4, 5, 6), (Type)0);

    m.SetRow(2, WVec3Type(7, 8, 9));
    W_TEST_VEC3(m.GetRow(2), WVec3Type(7, 8, 9), (Type)0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetColumn")
  {
    WMat3Type m = WMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    W_TEST_VEC3(m.GetColumn(0), WVec3Type(1, 4, 7), (Type)0);
    W_TEST_VEC3(m.GetColumn(1), WVec3Type(2, 5, 8), (Type)0);
    W_TEST_VEC3(m.GetColumn(2), WVec3Type(3, 6, 9), (Type)0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetColumn")
  {
    WMat3Type m;
    m.SetZero();

    m.SetColumn(0, WVec3Type(1, 2, 3));
    W_TEST_VEC3(m.GetColumn(0), WVec3Type(1, 2, 3), (Type)0);

    m.SetColumn(1, WVec3Type(4, 5, 6));
    W_TEST_VEC3(m.GetColumn(1), WVec3Type(4, 5, 6), (Type)0);

    m.SetColumn(2, WVec3Type(7, 8, 9));
    W_TEST_VEC3(m.GetColumn(2), WVec3Type(7, 8, 9), (Type)0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetDiagonal")
  {
    WMat3Type m = WMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    W_TEST_VEC3(m.GetDiagonal(), WVec3Type(1, 5, 9), (Type)0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetDiagonal")
  {
    WMat3Type m;
    m.SetZero();

    m.SetDiagonal(WVec3Type(1, 2, 3));
    W_TEST_VEC3(m.GetColumn(0), WVec3Type(1, 0, 0), (Type)0);
    W_TEST_VEC3(m.GetColumn(1), WVec3Type(0, 2, 0), (Type)0);
    W_TEST_VEC3(m.GetColumn(2), WVec3Type(0, 0, 3), (Type)0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetScalingFactors")
  {
    WMat3Type m = WMat3Type::MakeFromValues(1, 2, 3, 5, 6, 7, 9, 10, 11);

    WVec3Type s = m.GetScalingFactors();
    W_TEST_VEC3(s,
      WVec3Type(WMath::Sqrt((Type)(1 * 1 + 5 * 5 + 9 * 9)), WMath::Sqrt((Type)(2 * 2 + 6 * 6 + 10 * 10)),
        WMath::Sqrt((Type)(3 * 3 + 7 * 7 + 11 * 11))),
      WMath::LargeEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetScalingFactors")
  {
    WMat3Type m = WMat3Type::MakeFromValues(1, 2, 3, 5, 6, 7, 9, 10, 11);

    W_TEST_BOOL(m.SetScalingFactors(WVec3Type(1, 2, 3)) == W_SUCCESS);

    WVec3Type s = m.GetScalingFactors();
    W_TEST_VEC3(s, WVec3Type(1, 2, 3), WMath::LargeEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "TransformDirection")
  {
    WMat3Type m = WMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    const WVec3Type r = m.TransformDirection(WVec3Type(1, 2, 3));

    W_TEST_VEC3(r, WVec3Type(1 * 1 + 2 * 2 + 3 * 3, 1 * 4 + 2 * 5 + 3 * 6, 1 * 7 + 2 * 8 + 3 * 9), WMath::LargeEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator*=")
  {
    WMat3Type m = WMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    m *= (Type)2;

    W_TEST_VEC3(m.GetRow(0), WVec3Type(2, 4, 6), WMath::LargeEpsilon<Type>());
    W_TEST_VEC3(m.GetRow(1), WVec3Type(8, 10, 12), WMath::LargeEpsilon<Type>());
    W_TEST_VEC3(m.GetRow(2), WVec3Type(14, 16, 18), WMath::LargeEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator/=")
  {
    WMat3Type m = WMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    m *= (Type)4;
    m /= (Type)2;

    W_TEST_VEC3(m.GetRow(0), WVec3Type(2, 4, 6), WMath::LargeEpsilon<Type>());
    W_TEST_VEC3(m.GetRow(1), WVec3Type(8, 10, 12), WMath::LargeEpsilon<Type>());
    W_TEST_VEC3(m.GetRow(2), WVec3Type(14, 16, 18), WMath::LargeEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsIdentical")
  {
    WMat3Type m = WMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    WMat3Type m2 = m;

    W_TEST_BOOL(m.IsIdentical(m2));

    m2.m_fElementsCM[0] += WMath::DefaultEpsilon<Type>();
    W_TEST_BOOL(!m.IsIdentical(m2));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsEqual")
  {
    WMat3Type m = WMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    WMat3Type m2 = m;

    W_TEST_BOOL(m.IsEqual(m2, WMath::LargeEpsilon<Type>()));

    m2.m_fElementsCM[0] += WMath::DefaultEpsilon<Type>();
    W_TEST_BOOL(m.IsEqual(m2, WMath::LargeEpsilon<Type>()));
    W_TEST_BOOL(!m.IsEqual(m2, WMath::SmallEpsilon<Type>()));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator*(mat, mat)")
  {
    WMat3Type m1 = WMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    WMat3Type m2 = WMat3Type::MakeFromValues(-1, -2, -3, -4, -5, -6, -7, -8, -9);

    WMat3Type r = m1 * m2;

    W_TEST_VEC3(r.GetColumn(0), WVec3Type(-1 * 1 + -4 * 2 + -7 * 3, -1 * 4 + -4 * 5 + -7 * 6, -1 * 7 + -4 * 8 + -7 * 9), WMath::LargeEpsilon<Type>());
    W_TEST_VEC3(r.GetColumn(1), WVec3Type(-2 * 1 + -5 * 2 + -8 * 3, -2 * 4 + -5 * 5 + -8 * 6, -2 * 7 + -5 * 8 + -8 * 9), WMath::LargeEpsilon<Type>());
    W_TEST_VEC3(r.GetColumn(2), WVec3Type(-3 * 1 + -6 * 2 + -9 * 3, -3 * 4 + -6 * 5 + -9 * 6, -3 * 7 + -6 * 8 + -9 * 9), WMath::LargeEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator*(mat, vec)")
  {
    WMat3Type m = WMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    const WVec3Type r = m * (WVec3Type(1, 2, 3));

    W_TEST_VEC3(r, WVec3Type(1 * 1 + 2 * 2 + 3 * 3, 1 * 4 + 2 * 5 + 3 * 6, 1 * 7 + 2 * 8 + 3 * 9), WMath::LargeEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator*(mat, float) | operator*(float, mat)")
  {
    WMat3Type m0 = WMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    WMat3Type m = m0 * (Type)2;
    WMat3Type m2 = (Type)2 * m0;

    W_TEST_VEC3(m.GetRow(0), WVec3Type(2, 4, 6), WMath::LargeEpsilon<Type>());
    W_TEST_VEC3(m.GetRow(1), WVec3Type(8, 10, 12), WMath::LargeEpsilon<Type>());
    W_TEST_VEC3(m.GetRow(2), WVec3Type(14, 16, 18), WMath::LargeEpsilon<Type>());

    W_TEST_VEC3(m2.GetRow(0), WVec3Type(2, 4, 6), WMath::LargeEpsilon<Type>());
    W_TEST_VEC3(m2.GetRow(1), WVec3Type(8, 10, 12), WMath::LargeEpsilon<Type>());
    W_TEST_VEC3(m2.GetRow(2), WVec3Type(14, 16, 18), WMath::LargeEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator/(mat, float)")
  {
    WMat3Type m0 = WMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    m0 *= (Type)4;

    WMat3Type m = m0 / (Type)2;

    W_TEST_VEC3(m.GetRow(0), WVec3Type(2, 4, 6), WMath::LargeEpsilon<Type>());
    W_TEST_VEC3(m.GetRow(1), WVec3Type(8, 10, 12), WMath::LargeEpsilon<Type>());
    W_TEST_VEC3(m.GetRow(2), WVec3Type(14, 16, 18), WMath::LargeEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator+(mat, mat) | operator-(mat, mat)")
  {
    WMat3Type m0 = WMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    WMat3Type m1 = WMat3Type::MakeFromValues(-1, -2, -3, -4, -5, -6, -7, -8, -9);

    W_TEST_BOOL((m0 + m1).IsZero());
    W_TEST_BOOL((m0 - m1).IsEqual(m0 * (Type)2, WMath::LargeEpsilon<Type>()));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator== (mat, mat) | operator!= (mat, mat)")
  {
    WMat3Type m = WMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    WMat3Type m2 = m;

    W_TEST_BOOL(m == m2);

    m2.m_fElementsCM[0] += WMath::DefaultEpsilon<Type>();

    W_TEST_BOOL(m != m2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsNaN")
  {
    if (WMath::SupportsNaN<Type>())
    {
      WMat3Type m;

      m.SetIdentity();
      W_TEST_BOOL(!m.IsNaN());

      for (WUInt32 i = 0; i < 9; ++i)
      {
        m.SetIdentity();
        m.m_fElementsCM[i] = WMath::NaN<Type>();

        W_TEST_BOOL(m.IsNaN());
      }
    }
  }
}


W_CREATE_SIMPLE_TEST(Math, Mat3f)
{
  TestMat3<float>();
}
W_CREATE_SIMPLE_TEST(Math, Mat3d)
{
  TestMat3<double>();
}
