#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Implementation/AllClasses_inl.h>
#include <Foundation/Math/Mat4.h>

template <typename Type>
void TestMat4()
{
  using WMat4Type = WMat4Template<Type>;
  using WMat3Type = WMat3Template<Type>;
  using WVec3Type = WVec3Template<Type>;
  using WVec4Type = WVec4Template<Type>;

  W_TEST_BLOCK(WTestBlock::Enabled, "Default Constructor")
  {
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
    if (WMath::SupportsNaN<Type>())
    {
      // In debug the default constructor initializes everything with NaN.
      WMat4Type m;
      W_TEST_BOOL(WMath::IsNaN(m.m_fElementsCM[0]) && WMath::IsNaN(m.m_fElementsCM[1]) && WMath::IsNaN(m.m_fElementsCM[2]) &&
                   WMath::IsNaN(m.m_fElementsCM[3]) && WMath::IsNaN(m.m_fElementsCM[4]) && WMath::IsNaN(m.m_fElementsCM[5]) &&
                   WMath::IsNaN(m.m_fElementsCM[6]) && WMath::IsNaN(m.m_fElementsCM[7]) && WMath::IsNaN(m.m_fElementsCM[8]) &&
                   WMath::IsNaN(m.m_fElementsCM[9]) && WMath::IsNaN(m.m_fElementsCM[10]) && WMath::IsNaN(m.m_fElementsCM[11]) &&
                   WMath::IsNaN(m.m_fElementsCM[12]) && WMath::IsNaN(m.m_fElementsCM[13]) && WMath::IsNaN(m.m_fElementsCM[14]) &&
                   WMath::IsNaN(m.m_fElementsCM[15]));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    Type testBlock[16] = {(Type)1, (Type)2, (Type)3, (Type)4, (Type)5, (Type)6, (Type)7, (Type)8,
                          (Type)9, (Type)10, (Type)11, (Type)12, (Type)13, (Type)14, (Type)15, (Type)16};
    WMat4Type* m = ::new ((void*)&testBlock[0]) WMat4Type;

    W_TEST_BOOL(m->m_fElementsCM[0] == (Type)1 && m->m_fElementsCM[1] == (Type)2 && m->m_fElementsCM[2] == (Type)3 && m->m_fElementsCM[3] == (Type)4 &&
                 m->m_fElementsCM[4] == (Type)5 && m->m_fElementsCM[5] == (Type)6 && m->m_fElementsCM[6] == (Type)7 && m->m_fElementsCM[7] == (Type)8 &&
                 m->m_fElementsCM[8] == (Type)9 && m->m_fElementsCM[9] == (Type)10 && m->m_fElementsCM[10] == (Type)11 && m->m_fElementsCM[11] == (Type)12 &&
                 m->m_fElementsCM[12] == (Type)13 && m->m_fElementsCM[13] == (Type)14 && m->m_fElementsCM[14] == (Type)15 && m->m_fElementsCM[15] == (Type)16);
#endif
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor (Array Data)")
  {
    const Type data[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

    {
      WMat4Type m = WMat4Type::MakeFromColumnMajorArray(data);

      W_TEST_BOOL(m.m_fElementsCM[0] == (Type)1 && m.m_fElementsCM[1] == (Type)2 && m.m_fElementsCM[2] == (Type)3 && m.m_fElementsCM[3] == (Type)4 &&
                   m.m_fElementsCM[4] == (Type)5 && m.m_fElementsCM[5] == (Type)6 && m.m_fElementsCM[6] == (Type)7 && m.m_fElementsCM[7] == (Type)8 &&
                   m.m_fElementsCM[8] == (Type)9 && m.m_fElementsCM[9] == (Type)10 && m.m_fElementsCM[10] == (Type)11 && m.m_fElementsCM[11] == (Type)12 &&
                   m.m_fElementsCM[12] == (Type)13 && m.m_fElementsCM[13] == (Type)14 && m.m_fElementsCM[14] == (Type)15 && m.m_fElementsCM[15] == (Type)16);
    }

    {
      WMat4Type m = WMat4Type::MakeFromRowMajorArray(data);

      W_TEST_BOOL(m.m_fElementsCM[0] == (Type)1 && m.m_fElementsCM[1] == (Type)5 && m.m_fElementsCM[2] == (Type)9 && m.m_fElementsCM[3] == (Type)13 &&
                   m.m_fElementsCM[4] == (Type)2 && m.m_fElementsCM[5] == (Type)6 && m.m_fElementsCM[6] == (Type)10 && m.m_fElementsCM[7] == (Type)14 &&
                   m.m_fElementsCM[8] == (Type)3 && m.m_fElementsCM[9] == (Type)7 && m.m_fElementsCM[10] == (Type)11 && m.m_fElementsCM[11] == (Type)15 &&
                   m.m_fElementsCM[12] == (Type)4 && m.m_fElementsCM[13] == (Type)8 && m.m_fElementsCM[14] == (Type)12 && m.m_fElementsCM[15] == (Type)16);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor (Elements)")
  {
    WMat4Type m = WMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    W_TEST_FLOAT(m.Element(0, 0), 1, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(m.Element(1, 0), 2, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(m.Element(2, 0), 3, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(m.Element(3, 0), 4, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(m.Element(0, 1), 5, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(m.Element(1, 1), 6, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(m.Element(2, 1), 7, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(m.Element(3, 1), 8, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(m.Element(0, 2), 9, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(m.Element(1, 2), 10, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(m.Element(2, 2), 11, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(m.Element(3, 2), 12, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(m.Element(0, 3), 13, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(m.Element(1, 3), 14, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(m.Element(2, 3), 15, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(m.Element(3, 3), 16, WMath::SmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor (composite)")
  {
    WMat3Type mr = WMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);
    WVec3Type vt(10, 11, 12);

    WMat4Type m(mr, vt);

    W_TEST_FLOAT(m.Element(0, 0), 1, 0);
    W_TEST_FLOAT(m.Element(1, 0), 2, 0);
    W_TEST_FLOAT(m.Element(2, 0), 3, 0);
    W_TEST_FLOAT(m.Element(3, 0), 10, 0);
    W_TEST_FLOAT(m.Element(0, 1), 4, 0);
    W_TEST_FLOAT(m.Element(1, 1), 5, 0);
    W_TEST_FLOAT(m.Element(2, 1), 6, 0);
    W_TEST_FLOAT(m.Element(3, 1), 11, 0);
    W_TEST_FLOAT(m.Element(0, 2), 7, 0);
    W_TEST_FLOAT(m.Element(1, 2), 8, 0);
    W_TEST_FLOAT(m.Element(2, 2), 9, 0);
    W_TEST_FLOAT(m.Element(3, 2), 12, 0);
    W_TEST_FLOAT(m.Element(0, 3), 0, 0);
    W_TEST_FLOAT(m.Element(1, 3), 0, 0);
    W_TEST_FLOAT(m.Element(2, 3), 0, 0);
    W_TEST_FLOAT(m.Element(3, 3), 1, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetFromArray")
  {
    const Type data[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

    {
      WMat4Type m = WMat4Type::MakeFromColumnMajorArray(data);

      W_TEST_BOOL(m.m_fElementsCM[0] == (Type)1 && m.m_fElementsCM[1] == (Type)2 && m.m_fElementsCM[2] == (Type)3 && m.m_fElementsCM[3] == (Type)4 &&
                   m.m_fElementsCM[4] == (Type)5 && m.m_fElementsCM[5] == (Type)6 && m.m_fElementsCM[6] == (Type)7 && m.m_fElementsCM[7] == (Type)8 &&
                   m.m_fElementsCM[8] == (Type)9 && m.m_fElementsCM[9] == (Type)10 && m.m_fElementsCM[10] == (Type)11 && m.m_fElementsCM[11] == (Type)12 &&
                   m.m_fElementsCM[12] == (Type)13 && m.m_fElementsCM[13] == (Type)14 && m.m_fElementsCM[14] == (Type)15 && m.m_fElementsCM[15] == (Type)16);
    }

    {
      WMat4Type m = WMat4Type::MakeFromRowMajorArray(data);

      W_TEST_BOOL(m.m_fElementsCM[0] == (Type)1 && m.m_fElementsCM[1] == (Type)5 && m.m_fElementsCM[2] == (Type)9 && m.m_fElementsCM[3] == (Type)13 &&
                   m.m_fElementsCM[4] == (Type)2 && m.m_fElementsCM[5] == (Type)6 && m.m_fElementsCM[6] == (Type)10 && m.m_fElementsCM[7] == (Type)14 &&
                   m.m_fElementsCM[8] == (Type)3 && m.m_fElementsCM[9] == (Type)7 && m.m_fElementsCM[10] == (Type)11 && m.m_fElementsCM[11] == (Type)15 &&
                   m.m_fElementsCM[12] == (Type)4 && m.m_fElementsCM[13] == (Type)8 && m.m_fElementsCM[14] == (Type)12 && m.m_fElementsCM[15] == (Type)16);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetElements")
  {
    WMat4Type m = WMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    W_TEST_FLOAT(m.Element(0, 0), 1, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(m.Element(1, 0), 2, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(m.Element(2, 0), 3, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(m.Element(3, 0), 4, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(m.Element(0, 1), 5, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(m.Element(1, 1), 6, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(m.Element(2, 1), 7, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(m.Element(3, 1), 8, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(m.Element(0, 2), 9, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(m.Element(1, 2), 10, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(m.Element(2, 2), 11, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(m.Element(3, 2), 12, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(m.Element(0, 3), 13, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(m.Element(1, 3), 14, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(m.Element(2, 3), 15, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(m.Element(3, 3), 16, WMath::SmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeTransformation")
  {
    WMat3Type mr = WMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);
    WVec3Type vt(10, 11, 12);

    WMat4Type m = WMat4Type::MakeTransformation(mr, vt);

    W_TEST_FLOAT(m.Element(0, 0), 1, 0);
    W_TEST_FLOAT(m.Element(1, 0), 2, 0);
    W_TEST_FLOAT(m.Element(2, 0), 3, 0);
    W_TEST_FLOAT(m.Element(3, 0), 10, 0);
    W_TEST_FLOAT(m.Element(0, 1), 4, 0);
    W_TEST_FLOAT(m.Element(1, 1), 5, 0);
    W_TEST_FLOAT(m.Element(2, 1), 6, 0);
    W_TEST_FLOAT(m.Element(3, 1), 11, 0);
    W_TEST_FLOAT(m.Element(0, 2), 7, 0);
    W_TEST_FLOAT(m.Element(1, 2), 8, 0);
    W_TEST_FLOAT(m.Element(2, 2), 9, 0);
    W_TEST_FLOAT(m.Element(3, 2), 12, 0);
    W_TEST_FLOAT(m.Element(0, 3), 0, 0);
    W_TEST_FLOAT(m.Element(1, 3), 0, 0);
    W_TEST_FLOAT(m.Element(2, 3), 0, 0);
    W_TEST_FLOAT(m.Element(3, 3), 1, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetAsArray")
  {
    WMat4Type m = WMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    Type data[16];

    m.GetAsArray(data, WMatrixLayout::ColumnMajor);
    W_TEST_FLOAT(data[0], 1, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(data[1], 5, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(data[2], 9, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(data[3], 13, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(data[4], 2, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(data[5], 6, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(data[6], 10, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(data[7], 14, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(data[8], 3, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(data[9], 7, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(data[10], 11, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(data[11], 15, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(data[12], 4, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(data[13], 8, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(data[14], 12, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(data[15], 16, WMath::SmallEpsilon<Type>());

    m.GetAsArray(data, WMatrixLayout::RowMajor);
    W_TEST_FLOAT(data[0], 1, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(data[1], 2, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(data[2], 3, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(data[3], 4, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(data[4], 5, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(data[5], 6, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(data[6], 7, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(data[7], 8, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(data[8], 9, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(data[9], 10, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(data[10], 11, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(data[11], 12, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(data[12], 13, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(data[13], 14, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(data[14], 15, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(data[15], 16, WMath::SmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetZero")
  {
    WMat4Type m;
    m.SetZero();

    for (WUInt32 i = 0; i < 16; ++i)
      W_TEST_FLOAT(m.m_fElementsCM[i], (Type)0, (Type)0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetIdentity")
  {
    WMat4Type m;
    m.SetIdentity();

    W_TEST_FLOAT(m.Element(0, 0), 1, 0);
    W_TEST_FLOAT(m.Element(1, 0), 0, 0);
    W_TEST_FLOAT(m.Element(2, 0), 0, 0);
    W_TEST_FLOAT(m.Element(3, 0), 0, 0);
    W_TEST_FLOAT(m.Element(0, 1), 0, 0);
    W_TEST_FLOAT(m.Element(1, 1), 1, 0);
    W_TEST_FLOAT(m.Element(2, 1), 0, 0);
    W_TEST_FLOAT(m.Element(3, 1), 0, 0);
    W_TEST_FLOAT(m.Element(0, 2), 0, 0);
    W_TEST_FLOAT(m.Element(1, 2), 0, 0);
    W_TEST_FLOAT(m.Element(2, 2), 1, 0);
    W_TEST_FLOAT(m.Element(3, 2), 0, 0);
    W_TEST_FLOAT(m.Element(0, 3), 0, 0);
    W_TEST_FLOAT(m.Element(1, 3), 0, 0);
    W_TEST_FLOAT(m.Element(2, 3), 0, 0);
    W_TEST_FLOAT(m.Element(3, 3), 1, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetTranslationMatrix")
  {
    WMat4Type m = WMat4Type::MakeTranslation(WVec3Type(2, 3, 4));

    W_TEST_FLOAT(m.Element(0, 0), 1, 0);
    W_TEST_FLOAT(m.Element(1, 0), 0, 0);
    W_TEST_FLOAT(m.Element(2, 0), 0, 0);
    W_TEST_FLOAT(m.Element(3, 0), 2, 0);
    W_TEST_FLOAT(m.Element(0, 1), 0, 0);
    W_TEST_FLOAT(m.Element(1, 1), 1, 0);
    W_TEST_FLOAT(m.Element(2, 1), 0, 0);
    W_TEST_FLOAT(m.Element(3, 1), 3, 0);
    W_TEST_FLOAT(m.Element(0, 2), 0, 0);
    W_TEST_FLOAT(m.Element(1, 2), 0, 0);
    W_TEST_FLOAT(m.Element(2, 2), 1, 0);
    W_TEST_FLOAT(m.Element(3, 2), 4, 0);
    W_TEST_FLOAT(m.Element(0, 3), 0, 0);
    W_TEST_FLOAT(m.Element(1, 3), 0, 0);
    W_TEST_FLOAT(m.Element(2, 3), 0, 0);
    W_TEST_FLOAT(m.Element(3, 3), 1, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetScalingMatrix")
  {
    WMat4Type m = WMat4Type::MakeScaling(WVec3Type(2, 3, 4));

    W_TEST_FLOAT(m.Element(0, 0), 2, 0);
    W_TEST_FLOAT(m.Element(1, 0), 0, 0);
    W_TEST_FLOAT(m.Element(2, 0), 0, 0);
    W_TEST_FLOAT(m.Element(3, 0), 0, 0);
    W_TEST_FLOAT(m.Element(0, 1), 0, 0);
    W_TEST_FLOAT(m.Element(1, 1), 3, 0);
    W_TEST_FLOAT(m.Element(2, 1), 0, 0);
    W_TEST_FLOAT(m.Element(3, 1), 0, 0);
    W_TEST_FLOAT(m.Element(0, 2), 0, 0);
    W_TEST_FLOAT(m.Element(1, 2), 0, 0);
    W_TEST_FLOAT(m.Element(2, 2), 4, 0);
    W_TEST_FLOAT(m.Element(3, 2), 0, 0);
    W_TEST_FLOAT(m.Element(0, 3), 0, 0);
    W_TEST_FLOAT(m.Element(1, 3), 0, 0);
    W_TEST_FLOAT(m.Element(2, 3), 0, 0);
    W_TEST_FLOAT(m.Element(3, 3), 1, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetRotationMatrixX")
  {
    WMat4Type m;

    m = WMat4Type::MakeRotationX(WAngleTemplate<Type>::MakeFromDegree(90));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(1, -3, 2), WMath::DefaultEpsilon<Type>()));

    m = WMat4Type::MakeRotationX(WAngleTemplate<Type>::MakeFromDegree(180));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(1, -2, -3), WMath::DefaultEpsilon<Type>()));

    m = WMat4Type::MakeRotationX(WAngleTemplate<Type>::MakeFromDegree(270));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(1, 3, -2), WMath::DefaultEpsilon<Type>()));

    m = WMat4Type::MakeRotationX(WAngleTemplate<Type>::MakeFromDegree(360));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(1, 2, 3), WMath::DefaultEpsilon<Type>()));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetRotationMatrixY")
  {
    WMat4Type m;

    m = WMat4Type::MakeRotationY(WAngleTemplate<Type>::MakeFromDegree(90));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(3, 2, -1), WMath::DefaultEpsilon<Type>()));

    m = WMat4Type::MakeRotationY(WAngleTemplate<Type>::MakeFromDegree(180));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(-1, 2, -3), WMath::DefaultEpsilon<Type>()));

    m = WMat4Type::MakeRotationY(WAngleTemplate<Type>::MakeFromDegree(270));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(-3, 2, 1), WMath::DefaultEpsilon<Type>()));

    m = WMat4Type::MakeRotationY(WAngleTemplate<Type>::MakeFromDegree(360));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(1, 2, 3), WMath::DefaultEpsilon<Type>()));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetRotationMatrixZ")
  {
    WMat4Type m;

    m = WMat4Type::MakeRotationZ(WAngleTemplate<Type>::MakeFromDegree(90));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(-2, 1, 3), WMath::DefaultEpsilon<Type>()));

    m = WMat4Type::MakeRotationZ(WAngleTemplate<Type>::MakeFromDegree(180));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(-1, -2, 3), WMath::DefaultEpsilon<Type>()));

    m = WMat4Type::MakeRotationZ(WAngleTemplate<Type>::MakeFromDegree(270));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(2, -1, 3), WMath::DefaultEpsilon<Type>()));

    m = WMat4Type::MakeRotationZ(WAngleTemplate<Type>::MakeFromDegree(360));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(1, 2, 3), WMath::DefaultEpsilon<Type>()));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetRotationMatrix")
  {
    WMat4Type m;

    m = WMat4Type::MakeAxisRotation(WVec3Type(1, 0, 0), WAngleTemplate<Type>::MakeFromDegree(90));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(1, -3, 2), WMath::DefaultEpsilon<Type>()));

    m = WMat4Type::MakeAxisRotation(WVec3Type(1, 0, 0), WAngleTemplate<Type>::MakeFromDegree(180));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(1, -2, -3), WMath::DefaultEpsilon<Type>()));

    m = WMat4Type::MakeAxisRotation(WVec3Type(1, 0, 0), WAngleTemplate<Type>::MakeFromDegree(270));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(1, 3, -2), WMath::DefaultEpsilon<Type>()));

    m = WMat4Type::MakeAxisRotation(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromDegree(90));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(3, 2, -1), WMath::DefaultEpsilon<Type>()));

    m = WMat4Type::MakeAxisRotation(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromDegree(180));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(-1, 2, -3), WMath::DefaultEpsilon<Type>()));

    m = WMat4Type::MakeAxisRotation(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromDegree(270));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(-3, 2, 1), WMath::DefaultEpsilon<Type>()));

    m = WMat4Type::MakeAxisRotation(WVec3Type(0, 0, 1), WAngleTemplate<Type>::MakeFromDegree(90));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(-2, 1, 3), WMath::DefaultEpsilon<Type>()));

    m = WMat4Type::MakeAxisRotation(WVec3Type(0, 0, 1), WAngleTemplate<Type>::MakeFromDegree(180));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(-1, -2, 3), WMath::DefaultEpsilon<Type>()));

    m = WMat4Type::MakeAxisRotation(WVec3Type(0, 0, 1), WAngleTemplate<Type>::MakeFromDegree(270));
    W_TEST_BOOL((m * WVec3Type(1, 2, 3)).IsEqual(WVec3Type(2, -1, 3), WMath::DefaultEpsilon<Type>()));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeIdentity")
  {
    WMat4Type m = WMat4Type::MakeIdentity();

    W_TEST_FLOAT(m.Element(0, 0), 1, 0);
    W_TEST_FLOAT(m.Element(1, 0), 0, 0);
    W_TEST_FLOAT(m.Element(2, 0), 0, 0);
    W_TEST_FLOAT(m.Element(3, 0), 0, 0);
    W_TEST_FLOAT(m.Element(0, 1), 0, 0);
    W_TEST_FLOAT(m.Element(1, 1), 1, 0);
    W_TEST_FLOAT(m.Element(2, 1), 0, 0);
    W_TEST_FLOAT(m.Element(3, 1), 0, 0);
    W_TEST_FLOAT(m.Element(0, 2), 0, 0);
    W_TEST_FLOAT(m.Element(1, 2), 0, 0);
    W_TEST_FLOAT(m.Element(2, 2), 1, 0);
    W_TEST_FLOAT(m.Element(3, 2), 0, 0);
    W_TEST_FLOAT(m.Element(0, 3), 0, 0);
    W_TEST_FLOAT(m.Element(1, 3), 0, 0);
    W_TEST_FLOAT(m.Element(2, 3), 0, 0);
    W_TEST_FLOAT(m.Element(3, 3), 1, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeZero")
  {
    WMat4Type m = WMat4Type::MakeZero();

    W_TEST_FLOAT(m.Element(0, 0), 0, 0);
    W_TEST_FLOAT(m.Element(1, 0), 0, 0);
    W_TEST_FLOAT(m.Element(2, 0), 0, 0);
    W_TEST_FLOAT(m.Element(3, 0), 0, 0);
    W_TEST_FLOAT(m.Element(0, 1), 0, 0);
    W_TEST_FLOAT(m.Element(1, 1), 0, 0);
    W_TEST_FLOAT(m.Element(2, 1), 0, 0);
    W_TEST_FLOAT(m.Element(3, 1), 0, 0);
    W_TEST_FLOAT(m.Element(0, 2), 0, 0);
    W_TEST_FLOAT(m.Element(1, 2), 0, 0);
    W_TEST_FLOAT(m.Element(2, 2), 0, 0);
    W_TEST_FLOAT(m.Element(3, 2), 0, 0);
    W_TEST_FLOAT(m.Element(0, 3), 0, 0);
    W_TEST_FLOAT(m.Element(1, 3), 0, 0);
    W_TEST_FLOAT(m.Element(2, 3), 0, 0);
    W_TEST_FLOAT(m.Element(3, 3), 0, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Transpose")
  {
    WMat4Type m = WMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    m.Transpose();

    W_TEST_FLOAT(m.Element(0, 0), 1, 0);
    W_TEST_FLOAT(m.Element(1, 0), 5, 0);
    W_TEST_FLOAT(m.Element(2, 0), 9, 0);
    W_TEST_FLOAT(m.Element(3, 0), 13, 0);
    W_TEST_FLOAT(m.Element(0, 1), 2, 0);
    W_TEST_FLOAT(m.Element(1, 1), 6, 0);
    W_TEST_FLOAT(m.Element(2, 1), 10, 0);
    W_TEST_FLOAT(m.Element(3, 1), 14, 0);
    W_TEST_FLOAT(m.Element(0, 2), 3, 0);
    W_TEST_FLOAT(m.Element(1, 2), 7, 0);
    W_TEST_FLOAT(m.Element(2, 2), 11, 0);
    W_TEST_FLOAT(m.Element(3, 2), 15, 0);
    W_TEST_FLOAT(m.Element(0, 3), 4, 0);
    W_TEST_FLOAT(m.Element(1, 3), 8, 0);
    W_TEST_FLOAT(m.Element(2, 3), 12, 0);
    W_TEST_FLOAT(m.Element(3, 3), 16, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetTranspose")
  {
    WMat4Type m0 = WMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    WMat4Type m = m0.GetTranspose();

    W_TEST_FLOAT(m.Element(0, 0), 1, 0);
    W_TEST_FLOAT(m.Element(1, 0), 5, 0);
    W_TEST_FLOAT(m.Element(2, 0), 9, 0);
    W_TEST_FLOAT(m.Element(3, 0), 13, 0);
    W_TEST_FLOAT(m.Element(0, 1), 2, 0);
    W_TEST_FLOAT(m.Element(1, 1), 6, 0);
    W_TEST_FLOAT(m.Element(2, 1), 10, 0);
    W_TEST_FLOAT(m.Element(3, 1), 14, 0);
    W_TEST_FLOAT(m.Element(0, 2), 3, 0);
    W_TEST_FLOAT(m.Element(1, 2), 7, 0);
    W_TEST_FLOAT(m.Element(2, 2), 11, 0);
    W_TEST_FLOAT(m.Element(3, 2), 15, 0);
    W_TEST_FLOAT(m.Element(0, 3), 4, 0);
    W_TEST_FLOAT(m.Element(1, 3), 8, 0);
    W_TEST_FLOAT(m.Element(2, 3), 12, 0);
    W_TEST_FLOAT(m.Element(3, 3), 16, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Invert")
  {
    for (Type x = (Type)1.0; x < (Type)360.0; x += (Type)10.0)
    {
      for (Type y = (Type)2.0; y < (Type)360.0; y += (Type)17.0)
      {
        for (Type z = (Type)3.0; z < (Type)360.0; z += (Type)23.0)
        {
          WMat4Type m, inv;
          m = WMat4Type::MakeAxisRotation(WVec3Type(x, y, z).GetNormalized(), WAngleTemplate<Type>::MakeFromDegree((Type)19.0));
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
          WMat4Type m, inv;
          m = WMat4Type::MakeAxisRotation(WVec3Type(x, y, z).GetNormalized(), WAngleTemplate<Type>::MakeFromDegree((Type)83.0));
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
    WMat4Type m;

    m.SetIdentity();
    W_TEST_BOOL(!m.IsZero());

    m.SetZero();
    W_TEST_BOOL(m.IsZero());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsIdentity")
  {
    WMat4Type m;

    m.SetIdentity();
    W_TEST_BOOL(m.IsIdentity());

    m.SetZero();
    W_TEST_BOOL(!m.IsIdentity());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsValid")
  {
    if (WMath::SupportsNaN<Type>())
    {
      WMat4Type m;

      m.SetZero();
      W_TEST_BOOL(m.IsValid());

      m.m_fElementsCM[0] = WMath::NaN<Type>();
      W_TEST_BOOL(!m.IsValid());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetRow")
  {
    WMat4Type m = WMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    W_TEST_VEC4(m.GetRow(0), WVec4Type(1, 2, 3, 4), (Type)0);
    W_TEST_VEC4(m.GetRow(1), WVec4Type(5, 6, 7, 8), (Type)0);
    W_TEST_VEC4(m.GetRow(2), WVec4Type(9, 10, 11, 12), (Type)0);
    W_TEST_VEC4(m.GetRow(3), WVec4Type(13, 14, 15, 16), (Type)0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetRow")
  {
    WMat4Type m;
    m.SetZero();

    m.SetRow(0, WVec4Type(1, 2, 3, 4));
    W_TEST_VEC4(m.GetRow(0), WVec4Type(1, 2, 3, 4), (Type)0);

    m.SetRow(1, WVec4Type(5, 6, 7, 8));
    W_TEST_VEC4(m.GetRow(1), WVec4Type(5, 6, 7, 8), (Type)0);

    m.SetRow(2, WVec4Type(9, 10, 11, 12));
    W_TEST_VEC4(m.GetRow(2), WVec4Type(9, 10, 11, 12), (Type)0);

    m.SetRow(3, WVec4Type(13, 14, 15, 16));
    W_TEST_VEC4(m.GetRow(3), WVec4Type(13, 14, 15, 16), (Type)0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetColumn")
  {
    WMat4Type m = WMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    W_TEST_VEC4(m.GetColumn(0), WVec4Type(1, 5, 9, 13), (Type)0);
    W_TEST_VEC4(m.GetColumn(1), WVec4Type(2, 6, 10, 14), (Type)0);
    W_TEST_VEC4(m.GetColumn(2), WVec4Type(3, 7, 11, 15), (Type)0);
    W_TEST_VEC4(m.GetColumn(3), WVec4Type(4, 8, 12, 16), (Type)0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetColumn")
  {
    WMat4Type m;
    m.SetZero();

    m.SetColumn(0, WVec4Type(1, 2, 3, 4));
    W_TEST_VEC4(m.GetColumn(0), WVec4Type(1, 2, 3, 4), (Type)0);

    m.SetColumn(1, WVec4Type(5, 6, 7, 8));
    W_TEST_VEC4(m.GetColumn(1), WVec4Type(5, 6, 7, 8), (Type)0);

    m.SetColumn(2, WVec4Type(9, 10, 11, 12));
    W_TEST_VEC4(m.GetColumn(2), WVec4Type(9, 10, 11, 12), (Type)0);

    m.SetColumn(3, WVec4Type(13, 14, 15, 16));
    W_TEST_VEC4(m.GetColumn(3), WVec4Type(13, 14, 15, 16), (Type)0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetDiagonal")
  {
    WMat4Type m = WMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    W_TEST_VEC4(m.GetDiagonal(), WVec4Type(1, 6, 11, 16), (Type)0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetDiagonal")
  {
    WMat4Type m;
    m.SetZero();

    m.SetDiagonal(WVec4Type(1, 2, 3, 4));
    W_TEST_VEC4(m.GetColumn(0), WVec4Type(1, 0, 0, 0), (Type)0);
    W_TEST_VEC4(m.GetColumn(1), WVec4Type(0, 2, 0, 0), (Type)0);
    W_TEST_VEC4(m.GetColumn(2), WVec4Type(0, 0, 3, 0), (Type)0);
    W_TEST_VEC4(m.GetColumn(3), WVec4Type(0, 0, 0, 4), (Type)0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetTranslationVector")
  {
    WMat4Type m = WMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    W_TEST_VEC3(m.GetTranslationVector(), WVec3Type(4, 8, 12), (Type)0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetTranslationVector")
  {
    WMat4Type m = WMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    m.SetTranslationVector(WVec3Type(17, 18, 19));
    W_TEST_VEC4(m.GetRow(0), WVec4Type(1, 2, 3, 17), (Type)0);
    W_TEST_VEC4(m.GetRow(1), WVec4Type(5, 6, 7, 18), (Type)0);
    W_TEST_VEC4(m.GetRow(2), WVec4Type(9, 10, 11, 19), (Type)0);
    W_TEST_VEC4(m.GetRow(3), WVec4Type(13, 14, 15, 16), (Type)0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetRotationalPart")
  {
    WMat4Type m = WMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    WMat3Type r = WMat3Type::MakeFromValues(17, 18, 19, 20, 21, 22, 23, 24, 25);

    m.SetRotationalPart(r);
    W_TEST_VEC4(m.GetRow(0), WVec4Type(17, 18, 19, 4), (Type)0);
    W_TEST_VEC4(m.GetRow(1), WVec4Type(20, 21, 22, 8), (Type)0);
    W_TEST_VEC4(m.GetRow(2), WVec4Type(23, 24, 25, 12), (Type)0);
    W_TEST_VEC4(m.GetRow(3), WVec4Type(13, 14, 15, 16), (Type)0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetRotationalPart")
  {
    WMat4Type m = WMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    WMat3Type r = m.GetRotationalPart();
    W_TEST_VEC3(r.GetRow(0), WVec3Type(1, 2, 3), (Type)0);
    W_TEST_VEC3(r.GetRow(1), WVec3Type(5, 6, 7), (Type)0);
    W_TEST_VEC3(r.GetRow(2), WVec3Type(9, 10, 11), (Type)0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetScalingFactors")
  {
    WMat4Type m = WMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    WVec3Type s = m.GetScalingFactors();
    W_TEST_VEC3(s,
      WVec3Type(WMath::Sqrt((Type)(1 * 1 + 5 * 5 + 9 * 9)), WMath::Sqrt((Type)(2 * 2 + 6 * 6 + 10 * 10)),
        WMath::Sqrt((Type)(3 * 3 + 7 * 7 + 11 * 11))),
      WMath::SmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetScalingFactors")
  {
    WMat4Type m = WMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    W_TEST_BOOL(m.SetScalingFactors(WVec3Type(1, 2, 3)) == W_SUCCESS);

    WVec3Type s = m.GetScalingFactors();
    W_TEST_VEC3(s, WVec3Type(1, 2, 3), WMath::SmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "TransformDirection")
  {
    WMat4Type m = WMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    const WVec3Type r = m.TransformDirection(WVec3Type(1, 2, 3));

    W_TEST_VEC3(r, WVec3Type(1 * 1 + 2 * 2 + 3 * 3, 1 * 5 + 2 * 6 + 3 * 7, 1 * 9 + 2 * 10 + 3 * 11), WMath::SmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "TransformDirection(array)")
  {
    WMat4Type m = WMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    WVec3Type data[3] = {WVec3Type(1, 2, 3), WVec3Type(4, 5, 6), WVec3Type(7, 8, 9)};

    m.TransformDirection(data, 2);

    W_TEST_VEC3(data[0], WVec3Type(1 * 1 + 2 * 2 + 3 * 3, 1 * 5 + 2 * 6 + 3 * 7, 1 * 9 + 2 * 10 + 3 * 11), WMath::SmallEpsilon<Type>());
    W_TEST_VEC3(data[1], WVec3Type(4 * 1 + 5 * 2 + 6 * 3, 4 * 5 + 5 * 6 + 6 * 7, 4 * 9 + 5 * 10 + 6 * 11), WMath::SmallEpsilon<Type>());
    W_TEST_VEC3(data[2], WVec3Type(7, 8, 9), 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "TransformPosition")
  {
    WMat4Type m = WMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    const WVec3Type r = m.TransformPosition(WVec3Type(1, 2, 3));

    W_TEST_VEC3(r, WVec3Type(1 * 1 + 2 * 2 + 3 * 3 + 4, 1 * 5 + 2 * 6 + 3 * 7 + 8, 1 * 9 + 2 * 10 + 3 * 11 + 12), WMath::SmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "TransformPosition(array)")
  {
    WMat4Type m = WMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    WVec3Type data[3] = {WVec3Type(1, 2, 3), WVec3Type(4, 5, 6), WVec3Type(7, 8, 9)};

    m.TransformPosition(data, 2);

    W_TEST_VEC3(data[0], WVec3Type(1 * 1 + 2 * 2 + 3 * 3 + 4, 1 * 5 + 2 * 6 + 3 * 7 + 8, 1 * 9 + 2 * 10 + 3 * 11 + 12), WMath::SmallEpsilon<Type>());
    W_TEST_VEC3(data[1], WVec3Type(4 * 1 + 5 * 2 + 6 * 3 + 4, 4 * 5 + 5 * 6 + 6 * 7 + 8, 4 * 9 + 5 * 10 + 6 * 11 + 12), WMath::SmallEpsilon<Type>());
    W_TEST_VEC3(data[2], WVec3Type(7, 8, 9), 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Transform")
  {
    WMat4Type m = WMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    const WVec4Type r = m.Transform(WVec4Type(1, 2, 3, 4));

    W_TEST_VEC4(r,
      WVec4Type(1 * 1 + 2 * 2 + 3 * 3 + 4 * 4, 1 * 5 + 2 * 6 + 3 * 7 + 8 * 4, 1 * 9 + 2 * 10 + 3 * 11 + 12 * 4, 1 * 13 + 2 * 14 + 3 * 15 + 4 * 16),
      WMath::SmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Transform(array)")
  {
    WMat4Type m = WMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    WVec4Type data[3] = {WVec4Type(1, 2, 3, 4), WVec4Type(5, 6, 7, 8), WVec4Type(9, 10, 11, 12)};

    m.Transform(data, 2);

    W_TEST_VEC4(data[0],
      WVec4Type(1 * 1 + 2 * 2 + 3 * 3 + 4 * 4, 1 * 5 + 2 * 6 + 3 * 7 + 8 * 4, 1 * 9 + 2 * 10 + 3 * 11 + 12 * 4, 1 * 13 + 2 * 14 + 3 * 15 + 4 * 16),
      WMath::SmallEpsilon<Type>());
    W_TEST_VEC4(data[1],
      WVec4Type(5 * 1 + 6 * 2 + 7 * 3 + 8 * 4, 5 * 5 + 6 * 6 + 7 * 7 + 8 * 8, 5 * 9 + 6 * 10 + 7 * 11 + 12 * 8, 5 * 13 + 6 * 14 + 7 * 15 + 8 * 16),
      WMath::SmallEpsilon<Type>());
    W_TEST_VEC4(data[2], WVec4Type(9, 10, 11, 12), 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator*=")
  {
    WMat4Type m = WMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    m *= (Type)2;

    W_TEST_VEC4(m.GetRow(0), WVec4Type(2, 4, 6, 8), WMath::SmallEpsilon<Type>());
    W_TEST_VEC4(m.GetRow(1), WVec4Type(10, 12, 14, 16), WMath::SmallEpsilon<Type>());
    W_TEST_VEC4(m.GetRow(2), WVec4Type(18, 20, 22, 24), WMath::SmallEpsilon<Type>());
    W_TEST_VEC4(m.GetRow(3), WVec4Type(26, 28, 30, 32), WMath::SmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator/=")
  {
    WMat4Type m = WMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    m *= (Type)4;
    m /= (Type)2;

    W_TEST_VEC4(m.GetRow(0), WVec4Type(2, 4, 6, 8), WMath::SmallEpsilon<Type>());
    W_TEST_VEC4(m.GetRow(1), WVec4Type(10, 12, 14, 16), WMath::SmallEpsilon<Type>());
    W_TEST_VEC4(m.GetRow(2), WVec4Type(18, 20, 22, 24), WMath::SmallEpsilon<Type>());
    W_TEST_VEC4(m.GetRow(3), WVec4Type(26, 28, 30, 32), WMath::SmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsIdentical")
  {
    WMat4Type m = WMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    WMat4Type m2 = m;

    W_TEST_BOOL(m.IsIdentical(m2));

    m2.m_fElementsCM[0] += WMath::SmallEpsilon<Type>();
    W_TEST_BOOL(!m.IsIdentical(m2));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsEqual")
  {
    WMat4Type m = WMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    WMat4Type m2 = m;

    W_TEST_BOOL(m.IsEqual(m2, WMath::SmallEpsilon<Type>()));

    m2.m_fElementsCM[0] += WMath::DefaultEpsilon<Type>();
    W_TEST_BOOL(m.IsEqual(m2, WMath::DefaultEpsilon<Type>()));
    W_TEST_BOOL(!m.IsEqual(m2, WMath::SmallEpsilon<Type>()));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator*(mat, mat)")
  {
    WMat4Type m1 = WMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    WMat4Type m2 = WMat4Type::MakeFromValues(-1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -14, -15, -16);

    WMat4Type r = m1 * m2;

    W_TEST_VEC4(r.GetColumn(0),
      WVec4Type(-1 * 1 + -5 * 2 + -9 * 3 + -13 * 4, -1 * 5 + -5 * 6 + -9 * 7 + -13 * 8, -1 * 9 + -5 * 10 + -9 * 11 + -13 * 12,
        -1 * 13 + -5 * 14 + -9 * 15 + -13 * 16),
      WMath::LargeEpsilon<Type>());
    W_TEST_VEC4(r.GetColumn(1),
      WVec4Type(-2 * 1 + -6 * 2 + -10 * 3 + -14 * 4, -2 * 5 + -6 * 6 + -10 * 7 + -14 * 8, -2 * 9 + -6 * 10 + -10 * 11 + -14 * 12,
        -2 * 13 + -6 * 14 + -10 * 15 + -14 * 16),
      WMath::LargeEpsilon<Type>());
    W_TEST_VEC4(r.GetColumn(2),
      WVec4Type(-3 * 1 + -7 * 2 + -11 * 3 + -15 * 4, -3 * 5 + -7 * 6 + -11 * 7 + -15 * 8, -3 * 9 + -7 * 10 + -11 * 11 + -15 * 12,
        -3 * 13 + -7 * 14 + -11 * 15 + -15 * 16),
      WMath::LargeEpsilon<Type>());
    W_TEST_VEC4(r.GetColumn(3),
      WVec4Type(-4 * 1 + -8 * 2 + -12 * 3 + -16 * 4, -4 * 5 + -8 * 6 + -12 * 7 + -16 * 8, -4 * 9 + -8 * 10 + -12 * 11 + -16 * 12,
        -4 * 13 + -8 * 14 + -12 * 15 + -16 * 16),
      WMath::LargeEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator*(mat, vec3)")
  {
    WMat4Type m = WMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    const WVec3Type r = m * WVec3Type(1, 2, 3);

    W_TEST_VEC3(r, WVec3Type(1 * 1 + 2 * 2 + 3 * 3 + 4, 1 * 5 + 2 * 6 + 3 * 7 + 8, 1 * 9 + 2 * 10 + 3 * 11 + 12), WMath::SmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator*(mat, vec4)")
  {
    WMat4Type m = WMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    const WVec4Type r = m * WVec4Type(1, 2, 3, 4);

    W_TEST_VEC4(r,
      WVec4Type(1 * 1 + 2 * 2 + 3 * 3 + 4 * 4, 1 * 5 + 2 * 6 + 3 * 7 + 4 * 8, 1 * 9 + 2 * 10 + 3 * 11 + 4 * 12, 1 * 13 + 2 * 14 + 3 * 15 + 4 * 16),
      WMath::SmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator*(mat, float) | operator*(float, mat)")
  {
    WMat4Type m0 = WMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    WMat4Type m = m0 * (Type)2;
    WMat4Type m2 = (Type)2 * m0;

    W_TEST_VEC4(m.GetRow(0), WVec4Type(2, 4, 6, 8), WMath::SmallEpsilon<Type>());
    W_TEST_VEC4(m.GetRow(1), WVec4Type(10, 12, 14, 16), WMath::SmallEpsilon<Type>());
    W_TEST_VEC4(m.GetRow(2), WVec4Type(18, 20, 22, 24), WMath::SmallEpsilon<Type>());
    W_TEST_VEC4(m.GetRow(3), WVec4Type(26, 28, 30, 32), WMath::SmallEpsilon<Type>());

    W_TEST_VEC4(m2.GetRow(0), WVec4Type(2, 4, 6, 8), WMath::SmallEpsilon<Type>());
    W_TEST_VEC4(m2.GetRow(1), WVec4Type(10, 12, 14, 16), WMath::SmallEpsilon<Type>());
    W_TEST_VEC4(m2.GetRow(2), WVec4Type(18, 20, 22, 24), WMath::SmallEpsilon<Type>());
    W_TEST_VEC4(m2.GetRow(3), WVec4Type(26, 28, 30, 32), WMath::SmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator/(mat, float)")
  {
    WMat4Type m0 = WMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    m0 *= (Type)4;

    WMat4Type m = m0 / (Type)2;

    W_TEST_VEC4(m.GetRow(0), WVec4Type(2, 4, 6, 8), WMath::SmallEpsilon<Type>());
    W_TEST_VEC4(m.GetRow(1), WVec4Type(10, 12, 14, 16), WMath::SmallEpsilon<Type>());
    W_TEST_VEC4(m.GetRow(2), WVec4Type(18, 20, 22, 24), WMath::SmallEpsilon<Type>());
    W_TEST_VEC4(m.GetRow(3), WVec4Type(26, 28, 30, 32), WMath::SmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator+(mat, mat) | operator-(mat, mat)")
  {
    WMat4Type m0 = WMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    WMat4Type m1 = WMat4Type::MakeFromValues(-1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -14, -15, -16);

    W_TEST_BOOL((m0 + m1).IsZero());
    W_TEST_BOOL((m0 - m1).IsEqual(m0 * (Type)2, WMath::SmallEpsilon<Type>()));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator== (mat, mat) | operator!= (mat, mat)")
  {
    WMat4Type m = WMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    WMat4Type m2 = m;

    W_TEST_BOOL(m == m2);

    m2.m_fElementsCM[0] += WMath::SmallEpsilon<Type>();

    W_TEST_BOOL(m != m2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsNaN")
  {
    if (WMath::SupportsNaN<Type>())
    {
      WMat4Type m;

      m.SetIdentity();
      W_TEST_BOOL(!m.IsNaN());

      for (WUInt32 i = 0; i < 16; ++i)
      {
        m.SetIdentity();
        m.m_fElementsCM[i] = WMath::NaN<Type>();

        W_TEST_BOOL(m.IsNaN());
      }
    }
  }
}


W_CREATE_SIMPLE_TEST(Math, Mat4f)
{
  TestMat4<float>();
}
W_CREATE_SIMPLE_TEST(Math, Mat4d)
{
  TestMat4<double>();
}