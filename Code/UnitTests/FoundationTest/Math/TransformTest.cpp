#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Transform.h>

template <typename Type>
void TestTransform()
{
  using WTransformType = WTransformTemplate<Type>;
  using WVec3Type = WVec3Template<Type>;
  using WMat3Type = WMat3Template<Type>;
  using WMat4Type = WMat4Template<Type>;
  using WQuatType = WQuatTemplate<Type>;

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructors")
  {
    WTransformType t0;

    {
      WTransformType t(WVec3Type(1, 2, 3));
      W_TEST_VEC3(t.m_vPosition, WVec3Type(1, 2, 3), 0);
    }

    {
      WQuatType qRot;
      qRot = WQuatType::MakeFromAxisAndAngle(WVec3Type(1, 2, 3).GetNormalized(), WAngleTemplate<Type>::MakeFromDegree(42.0f));

      WTransformType t(WVec3Type(4, 5, 6), qRot);
      W_TEST_VEC3(t.m_vPosition, WVec3Type(4, 5, 6), 0);
      W_TEST_BOOL(t.m_qRotation == qRot);
    }

    {
      WMat3Type mRot = WMat3Type::MakeAxisRotation(WVec3Type(1, 2, 3).GetNormalized(), WAngleTemplate<Type>::MakeFromDegree(42.0f));

      WQuatType q;
      q = WQuatType::MakeFromMat3(mRot);

      WTransformType t(WVec3Type(4, 5, 6), q);
      W_TEST_VEC3(t.m_vPosition, WVec3Type(4, 5, 6), 0);
      W_TEST_BOOL(t.m_qRotation.GetAsMat3().IsEqual(mRot, (Type)0.0001));
    }

    {
      WQuatType qRot;
      qRot.SetIdentity();

      WTransformType t(WVec3Type(4, 5, 6), qRot, WVec3Type(2, 3, 4));
      W_TEST_VEC3(t.m_vPosition, WVec3Type(4, 5, 6), 0);
      W_TEST_BOOL(t.m_qRotation.GetAsMat3().IsEqual(WMat3Type::MakeFromValues(1, 0, 0, 0, 1, 0, 0, 0, 1), (Type)0.001));
      W_TEST_VEC3(t.m_vScale, WVec3Type(2, 3, 4), 0);
    }

    {
      WMat3Type mRot = WMat3Type::MakeAxisRotation(WVec3Type(1, 2, 3).GetNormalized(), WAngleTemplate<Type>::MakeFromDegree(42.0f));
      WMat4Type mTrans;
      mTrans.SetTransformationMatrix(mRot, WVec3Type(1, 2, 3));

      WTransformType t = WTransformType::MakeFromMat4(mTrans);
      W_TEST_VEC3(t.m_vPosition, WVec3Type(1, 2, 3), 0);
      W_TEST_BOOL(t.m_qRotation.GetAsMat3().IsEqual(mRot, (Type)0.001));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetIdentity")
  {
    WTransformType t;
    t.SetIdentity();

    W_TEST_VEC3(t.m_vPosition, WVec3Type(0), 0);
    W_TEST_BOOL(t.m_qRotation == WQuatType::MakeIdentity());
    W_TEST_BOOL(t.m_vScale == WVec3Type((Type)1.0));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetAsMat4")
  {
    WQuatType qRot;
    qRot.SetIdentity();

    WTransformType t(WVec3Type(4, 5, 6), qRot, WVec3Type(2, 3, 4));
    W_TEST_BOOL(t.GetAsMat4() == WMat4Type::MakeFromValues(2, 0, 0, 4, 0, 3, 0, 5, 0, 0, 4, 6, 0, 0, 0, 1));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator + / -")
  {
    WTransformType t0, t1;
    t0.SetIdentity();
    t1.SetIdentity();

    t1 = t0 + WVec3Type(2, 3, 4);
    W_TEST_VEC3(t1.m_vPosition, WVec3Type(2, 3, 4), (Type)0.0001);

    t1 = t1 - WVec3Type(4, 2, 1);
    W_TEST_VEC3(t1.m_vPosition, WVec3Type(-2, 1, 3), (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator * (quat)")
  {
    WQuatType qRotX, qRotY;
    qRotX = WQuatType::MakeFromAxisAndAngle(WVec3Type(1, 0, 0), WAngleTemplate<Type>::MakeFromRadian(1.57079637f));
    qRotY = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromRadian(1.57079637f));

    WTransformType t0, t1;
    t0.SetIdentity();
    t1.SetIdentity();

    t1 = qRotX * t0;
    W_TEST_VEC3(t1.m_vPosition, WVec3Type(0, 0, 0), (Type)0.0001);

    WQuatType q;
    q = WQuatType::MakeFromMat3(WMat3Type::MakeFromValues(1, 0, 0, 0, 0, -1, 0, 1, 0));
    W_TEST_BOOL(t1.m_qRotation.IsEqualRotation(q, (Type)0.0001));

    t1 = qRotY * t1;
    W_TEST_VEC3(t1.m_vPosition, WVec3Type(0, 0, 0), (Type)0.0001);
    q = WQuatType::MakeFromMat3(WMat3Type::MakeFromValues(0, 1, 0, 0, 0, -1, -1, 0, 0));
    W_TEST_BOOL(t1.m_qRotation.IsEqualRotation(q, (Type)0.0001));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator * (vec3)")
  {
    WQuatType qRotX, qRotY;
    qRotX = WQuatType::MakeFromAxisAndAngle(WVec3Type(1, 0, 0), WAngleTemplate<Type>::MakeFromRadian(1.57079637f));
    qRotY = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromRadian(1.57079637f));

    WTransformType t;
    t.SetIdentity();

    t = qRotX * t;

    W_TEST_BOOL(t.m_qRotation.IsEqualRotation(qRotX, (Type)0.0001));
    W_TEST_VEC3(t.m_vPosition, WVec3Type(0, 0, 0), (Type)0.0001);
    W_TEST_VEC3(t.m_vScale, WVec3Type(1, 1, 1), (Type)0.0001);

    t = t + WVec3Type(1, 2, 3);

    W_TEST_BOOL(t.m_qRotation.IsEqualRotation(qRotX, (Type)0.0001));
    W_TEST_VEC3(t.m_vPosition, WVec3Type(1, 2, 3), (Type)0.0001);
    W_TEST_VEC3(t.m_vScale, WVec3Type(1, 1, 1), (Type)0.0001);

    t = qRotY * t;

    W_TEST_BOOL(t.m_qRotation.IsEqualRotation(qRotY * qRotX, (Type)0.0001));
    W_TEST_VEC3(t.m_vPosition, WVec3Type(1, 2, 3), (Type)0.0001);
    W_TEST_VEC3(t.m_vScale, WVec3Type(1, 1, 1), (Type)0.0001);

    WQuatType q;
    q = WQuatType::MakeFromMat3(WMat3Type::MakeFromValues(0, 1, 0, 0, 0, -1, -1, 0, 0));
    W_TEST_BOOL(t.m_qRotation.IsEqualRotation(q, (Type)0.0001));

    WVec3Type v;
    v = t * WVec3Type(4, 5, 6);

    W_TEST_VEC3(v, WVec3Type(5 + 1, -6 + 2, -4 + 3), (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsIdentical")
  {
    WTransformType t(WVec3Type(1, 2, 3));
    t.m_qRotation = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromDegree(90));

    W_TEST_BOOL(t.IsIdentical(t));

    WTransformType t2(WVec3Type(1, 2, 4));
    t2.m_qRotation = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromDegree(90));

    W_TEST_BOOL(!t.IsIdentical(t2));

    WTransformType t3(WVec3Type(1, 2, 3));
    t3.m_qRotation = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromDegree(91));

    W_TEST_BOOL(!t.IsIdentical(t3));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator == / !=")
  {
    WTransformType t(WVec3Type(1, 2, 3));
    t.m_qRotation = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromDegree(90));

    W_TEST_BOOL(t == t);

    WTransformType t2(WVec3Type(1, 2, 4));
    t2.m_qRotation = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromDegree(90));

    W_TEST_BOOL(t != t2);

    WTransformType t3(WVec3Type(1, 2, 3));
    t3.m_qRotation = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromDegree(91));

    W_TEST_BOOL(t != t3);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsEqual")
  {
    WTransformType t(WVec3Type(1, 2, 3));
    t.m_qRotation = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromDegree(90));

    W_TEST_BOOL(t.IsEqual(t, WMath::DefaultEpsilon<Type>()));

    WTransformType t2(WVec3Type(1, 2, (Type)3 + 2*WMath::DefaultEpsilon<Type>()));
    t2.m_qRotation = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromDegree(90));

    W_TEST_BOOL(t.IsEqual(t2, WMath::LargeEpsilon<Type>()));
    W_TEST_BOOL(!t.IsEqual(t2, WMath::DefaultEpsilon<Type>()));

    WTransformType t3(WVec3Type(1, 2, 3));
    t3.m_qRotation = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromDegree(90 + WMath::VeryHugeEpsilon<Type>()));

    W_TEST_BOOL(t.IsEqual(t3, WMath::VeryHugeEpsilon<Type>()));
    W_TEST_BOOL(!t.IsEqual(t3, WMath::DefaultEpsilon<Type>()));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator*(WTransformType, WTransformType)")
  {
    WTransformType tParent(WVec3Type(1, 2, 3));
    tParent.m_qRotation = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromRadian(1.57079637f));
    tParent.m_vScale.Set(2);

    WTransformType tToChild(WVec3Type(4, 5, 6));
    tToChild.m_qRotation = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 0, 1), WAngleTemplate<Type>::MakeFromRadian(1.57079637f));
    tToChild.m_vScale.Set(4);

    // this is exactly the same as SetGlobalTransform
    WTransformType tChild;
    tChild = tParent * tToChild;

    W_TEST_VEC3(tChild.m_vPosition, WVec3Type(13, 12, -5), (Type)0.003);
    W_TEST_BOOL(tChild.m_qRotation.GetAsMat3().IsEqual(WMat3Type::MakeFromValues(0, 0, 1, 1, 0, 0, 0, 1, 0), (Type)0.0001));
    W_TEST_VEC3(tChild.m_vScale, WVec3Type(8, 8, 8), (Type)0.0001);

    // verify that it works exactly like a 4x4 matrix
    const WMat4Type mParent = tParent.GetAsMat4();
    const WMat4Type mToChild = tToChild.GetAsMat4();
    const WMat4Type mChild = mParent * mToChild;

    W_TEST_BOOL(mChild.IsEqual(tChild.GetAsMat4(), (Type)0.0001));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator*(WTransformType, WMat4)")
  {
    WTransformType tParent(WVec3Type(1, 2, 3));
    tParent.m_qRotation = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromDegree(90));
    tParent.m_vScale.Set(2);

    WTransformType tToChild(WVec3Type(4, 5, 6));
    tToChild.m_qRotation = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 0, 1), WAngleTemplate<Type>::MakeFromDegree(90));
    tToChild.m_vScale.Set(4);

    // this is exactly the same as SetGlobalTransform
    WTransformType tChild;
    tChild = tParent * tToChild;

    W_TEST_VEC3(tChild.m_vPosition, WVec3Type(13, 12, -5), (Type)0.0001);
    W_TEST_BOOL(tChild.m_qRotation.GetAsMat3().IsEqual(WMat3Type::MakeFromValues(0, 0, 1, 1, 0, 0, 0, 1, 0), (Type)0.0001));
    W_TEST_VEC3(tChild.m_vScale, WVec3Type(8, 8, 8), (Type)0.0001);

    // verify that it works exactly like a 4x4 matrix
    const WMat4Type mParent = tParent.GetAsMat4();
    const WMat4Type mToChild = tToChild.GetAsMat4();
    const WMat4Type mChild = mParent * mToChild;

    W_TEST_BOOL(mChild.IsEqual(tChild.GetAsMat4(), (Type)0.0001));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator*(WMat4, WTransformType)")
  {
    WTransformType tParent(WVec3Type(1, 2, 3));
    tParent.m_qRotation = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromDegree(90));
    tParent.m_vScale.Set(2);

    WTransformType tToChild(WVec3Type(4, 5, 6));
    tToChild.m_qRotation = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 0, 1), WAngleTemplate<Type>::MakeFromDegree(90));
    tToChild.m_vScale.Set(4);

    // this is exactly the same as SetGlobalTransform
    WTransformType tChild;
    tChild = tParent * tToChild;

    W_TEST_VEC3(tChild.m_vPosition, WVec3Type(13, 12, -5), (Type)0.0001);
    W_TEST_BOOL(tChild.m_qRotation.GetAsMat3().IsEqual(WMat3Type::MakeFromValues(0, 0, 1, 1, 0, 0, 0, 1, 0), (Type)0.0001));
    W_TEST_VEC3(tChild.m_vScale, WVec3Type(8, 8, 8), (Type)0.0001);

    // verify that it works exactly like a 4x4 matrix
    const WMat4Type mParent = tParent.GetAsMat4();
    const WMat4Type mToChild = tToChild.GetAsMat4();
    const WMat4Type mChild = mParent * mToChild;

    W_TEST_BOOL(mChild.IsEqual(tChild.GetAsMat4(), (Type)0.0001));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Invert / GetInverse")
  {
    WTransformType tParent(WVec3Type(1, 2, 3));
    tParent.m_qRotation = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromDegree(90));
    tParent.m_vScale.Set(2);

    WTransformType tToChild(WVec3Type(4, 5, 6));
    tToChild.m_qRotation = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 0, 1), WAngleTemplate<Type>::MakeFromDegree(90));
    tToChild.m_vScale.Set(4);

    WTransformType tChild;
    tChild = WTransformType::MakeGlobalTransform(tParent, tToChild);

    // negate twice -> get back original
    tToChild.Invert();
    tToChild.Invert();

    WTransformType tInvToChild = tToChild.GetInverse();

    WTransformType tParentFromChild;
    tParentFromChild = WTransformType::MakeGlobalTransform(tChild, tInvToChild);

    W_TEST_BOOL(tParent.IsEqual(tParentFromChild, (Type)0.0001));
  }

  //////////////////////////////////////////////////////////////////////////
  // Tests copied and ported over from WSimdTransform
  //////////////////////////////////////////////////////////////////////////

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor")
  {
    WTransformType t0;

    {
      WQuatType qRot;
      qRot = WQuatType::MakeFromAxisAndAngle(WVec3Type(1, 2, 3).GetNormalized(), WAngleTemplate<Type>::MakeFromDegree(42.0f));

      WVec3Type pos(4, 5, 6);
      WVec3Type scale(7, 8, 9);

      WTransformType t(pos);
      W_TEST_BOOL((t.m_vPosition == pos));
      W_TEST_BOOL(t.m_qRotation == WQuatType::MakeIdentity());
      W_TEST_BOOL((t.m_vScale == WVec3Type(1)));

      t = WTransformType(pos, qRot);
      W_TEST_BOOL((t.m_vPosition == pos));
      W_TEST_BOOL(t.m_qRotation == qRot);
      W_TEST_BOOL((t.m_vScale == WVec3Type(1)));

      t = WTransformType(pos, qRot, scale);
      W_TEST_BOOL((t.m_vPosition == pos));
      W_TEST_BOOL(t.m_qRotation == qRot);
      W_TEST_BOOL((t.m_vScale == scale));

      t = WTransformType(WVec3Type::MakeZero(), qRot);
      W_TEST_BOOL(t.m_vPosition.IsZero());
      W_TEST_BOOL(t.m_qRotation == qRot);
      W_TEST_BOOL((t.m_vScale == WVec3Type(1)));
    }

    {
      WTransformType t;
      t.SetIdentity();

      W_TEST_BOOL(t.m_vPosition.IsZero());
      W_TEST_BOOL(t.m_qRotation == WQuatType::MakeIdentity());
      W_TEST_BOOL((t.m_vScale == WVec3Type(1)));

      W_TEST_BOOL(t == WTransformType::MakeIdentity());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Inverse")
  {
    WTransformType tParent(WVec3Type(1, 2, 3));
    tParent.m_qRotation = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromDegree(90));
    tParent.m_vScale = WVec3Type(2);

    WTransformType tToChild(WVec3Type(4, 5, 6));
    tToChild.m_qRotation = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 0, 1), WAngleTemplate<Type>::MakeFromDegree(90));
    tToChild.m_vScale = WVec3Type(4);

    WTransformType tChild;
    tChild = tParent * tToChild;

    // invert twice -> get back original
    WTransformType t2 = tToChild;
    t2.Invert();
    t2.Invert();
    W_TEST_BOOL(t2.IsEqual(tToChild, (Type)0.0001));

    WTransformType tInvToChild = tToChild.GetInverse();

    WTransformType tParentFromChild;
    tParentFromChild = tChild * tInvToChild;

    W_TEST_BOOL(tParent.IsEqual(tParentFromChild, (Type)0.0001));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetLocalTransform")
  {
    WQuatType q;
    q = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 0, 1), WAngleTemplate<Type>::MakeFromDegree(90));

    WTransformType tParent(WVec3Type(1, 2, 3));
    tParent.m_qRotation = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromDegree(90));
    tParent.m_vScale = WVec3Type(2);

    WTransformType tChild;
    tChild.m_vPosition = WVec3Type(13, 12, -5);
    tChild.m_qRotation = tParent.m_qRotation * q;
    tChild.m_vScale = WVec3Type(8);

    WTransformType tToChild;
    tToChild = WTransformType::MakeLocalTransform(tParent, tChild);

    W_TEST_BOOL(tToChild.m_vPosition.IsEqual(WVec3Type(4, 5, 6), (Type)0.0001));
    W_TEST_BOOL(tToChild.m_qRotation.IsEqualRotation(q, (Type)0.0001));
    W_TEST_BOOL((tToChild.m_vScale == WVec3Type(4)));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetGlobalTransform")
  {
    WTransformType tParent(WVec3Type(1, 2, 3));
    tParent.m_qRotation = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromDegree(90));
    tParent.m_vScale = WVec3Type(2);

    WTransformType tToChild(WVec3Type(4, 5, 6));
    tToChild.m_qRotation = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 0, 1), WAngleTemplate<Type>::MakeFromDegree(90));
    tToChild.m_vScale = WVec3Type(4);

    WTransformType tChild;
    tChild = WTransformType::MakeGlobalTransform(tParent, tToChild);

    W_TEST_BOOL(tChild.m_vPosition.IsEqual(WVec3Type(13, 12, -5), (Type)0.0001));
    W_TEST_BOOL(tChild.m_qRotation.IsEqualRotation(tParent.m_qRotation * tToChild.m_qRotation, (Type)0.0001));
    W_TEST_BOOL((tChild.m_vScale == WVec3Type(8)));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetAsMat4")
  {
    WTransformType t(WVec3Type(1, 2, 3));
    t.m_qRotation = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromDegree(34));
    t.m_vScale = WVec3Type(2, -1, 5);

    WMat4Type m = t.GetAsMat4();

    WMat4Type refM;
    refM.SetZero();
    {
      WQuatType q;
      q = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromDegree(34));

      WTransformType referenceTransform(WVec3Type(1, 2, 3), q, WVec3Type(2, -1, 5));
      WMat4Type tmp = referenceTransform.GetAsMat4();
      refM = WMat4Type::MakeFromColumnMajorArray(tmp.m_fElementsCM);
    }
    W_TEST_BOOL(m.IsEqual(refM, WMath::DefaultEpsilon<Type>()));

    WVec3Type p[8] = {
      WVec3Type(-4, 0, 0), WVec3Type(5, 0, 0), WVec3Type(0, -6, 0), WVec3Type(0, 7, 0), WVec3Type(0, 0, -8), WVec3Type(0, 0, 9), WVec3Type(1, -2, 3), WVec3Type(-4, 5, 7)};

    for (WUInt32 i = 0; i < W_ARRAY_SIZE(p); ++i)
    {
      WVec3Type pt = t.TransformPosition(p[i]);
      WVec3Type pm = m.TransformPosition(p[i]);

      W_TEST_BOOL(pt.IsEqual(pm, WMath::DefaultEpsilon<Type>()));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "TransformPos / Dir / operator*")
  {
    WQuatType qRotX, qRotY;
    qRotX = WQuatType::MakeFromAxisAndAngle(WVec3Type(1, 0, 0), WAngleTemplate<Type>::MakeFromDegree(90.0f));
    qRotY = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromDegree(90.0f));

    WTransformType t(WVec3Type(1, 2, 3), qRotY * qRotX, WVec3Type(2, -2, 4));

    WVec3Type v;
    v = t.TransformPosition(WVec3Type(4, 5, 6));
    W_TEST_BOOL(v.IsEqual(WVec3Type((5 * -2) + 1, (-6 * 4) + 2, (-4 * 2) + 3), (Type)0.0001));

    v = t.TransformDirection(WVec3Type(4, 5, 6));
    W_TEST_BOOL(v.IsEqual(WVec3Type((5 * -2), (-6 * 4), (-4 * 2)), (Type)0.0001));

    v = t * WVec3Type(4, 5, 6);
    W_TEST_BOOL(v.IsEqual(WVec3Type((5 * -2) + 1, (-6 * 4) + 2, (-4 * 2) + 3), (Type)0.0001));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Operators")
  {
    {
      WTransformType tParent(WVec3Type(1, 2, 3));
      tParent.m_qRotation = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromDegree(90));
      tParent.m_vScale = WVec3Type(2);

      WTransformType tToChild(WVec3Type(4, 5, 6));
      tToChild.m_qRotation = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 0, 1), WAngleTemplate<Type>::MakeFromDegree(90));
      tToChild.m_vScale = WVec3Type(4);

      // this is exactly the same as SetGlobalTransform
      WTransformType tChild;
      tChild = tParent * tToChild;

      W_TEST_BOOL(tChild.m_vPosition.IsEqual(WVec3Type(13, 12, -5), (Type)0.0001));
      W_TEST_BOOL(tChild.m_qRotation.IsEqualRotation(tParent.m_qRotation * tToChild.m_qRotation, (Type)0.0001));
      W_TEST_BOOL((tChild.m_vScale == WVec3Type(8)));

      tChild = tParent;
      tChild = tChild * tToChild;

      W_TEST_BOOL(tChild.m_vPosition.IsEqual(WVec3Type(13, 12, -5), (Type)0.0001));
      W_TEST_BOOL(tChild.m_qRotation.IsEqualRotation(tParent.m_qRotation * tToChild.m_qRotation, (Type)0.0001));
      W_TEST_BOOL((tChild.m_vScale == WVec3Type(8)));

      WVec3Type a(7, 8, 9);
      WVec3Type b;
      b = tToChild.TransformPosition(a);
      b = tParent.TransformPosition(b);

      WVec3Type c;
      c = tChild.TransformPosition(a);

      W_TEST_BOOL(b.IsEqual(c, (Type)0.0001));

      // verify that it works exactly like a 4x4 matrix
      const WMat4Type mParent = tParent.GetAsMat4();
      const WMat4Type mToChild = tToChild.GetAsMat4();
      const WMat4Type mChild = mParent * mToChild;

      W_TEST_BOOL(mChild.IsEqual(tChild.GetAsMat4(), (Type)0.0001));
    }

    {
      WTransformType t(WVec3Type(1, 2, 3));
      t.m_qRotation = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromDegree(90));
      t.m_vScale = WVec3Type(2);

      WQuatType q;
      q = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 0, 1), WAngleTemplate<Type>::MakeFromDegree(90));

      WTransformType t2 = t * q;
      WTransformType t4 = q * t;

      WTransformType t3 = t;
      t3 = t3 * q;
      W_TEST_BOOL(t2 == t3);
      W_TEST_BOOL(t3 != t4);

      WVec3Type a(7, 8, 9);
      WVec3Type b;
      b = t2.TransformPosition(a);

      WVec3Type c = q * a;
      c = t.TransformPosition(c);

      W_TEST_BOOL(b.IsEqual(c, (Type)0.0001));
    }

    {
      WTransformType t(WVec3Type(1, 2, 3));
      t.m_qRotation = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromDegree(90));
      t.m_vScale = WVec3Type(2);

      WVec3Type p(4, 5, 6);

      WTransformType t2 = t + p;
      WTransformType t3 = t;
      t3 += p;
      W_TEST_BOOL(t2 == t3);

      WVec3Type a(7, 8, 9);
      WVec3Type b;
      b = t2.TransformPosition(a);

      WVec3Type c = t.TransformPosition(a) + p;

      W_TEST_BOOL(b.IsEqual(c, (Type)0.0001));
    }

    {
      WTransformType t(WVec3Type(1, 2, 3));
      t.m_qRotation = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromDegree(90));
      t.m_vScale = WVec3Type(2);

      WVec3Type p(4, 5, 6);

      WTransformType t2 = t - p;
      WTransformType t3 = t;
      t3 -= p;
      W_TEST_BOOL(t2 == t3);

      WVec3Type a(7, 8, 9);
      WVec3Type b;
      b = t2.TransformPosition(a);

      WVec3Type c = t.TransformPosition(a) - p;

      W_TEST_BOOL(b.IsEqual(c, (Type)0.0001));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Comparison")
  {
    WTransformType t(WVec3Type(1, 2, 3));
    t.m_qRotation = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromDegree(90));

    W_TEST_BOOL(t == t);

    WTransformType t2(WVec3Type(1, 2, 4));
    t2.m_qRotation = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromDegree(90));

    W_TEST_BOOL(t != t2);

    WTransformType t3(WVec3Type(1, 2, 3));
    t3.m_qRotation = WQuatType::MakeFromAxisAndAngle(WVec3Type(0, 1, 0), WAngleTemplate<Type>::MakeFromDegree(91));

    W_TEST_BOOL(t != t3);
  }
}


W_CREATE_SIMPLE_TEST(Math, Transformf)
{
  TestTransform<float>();
}
W_CREATE_SIMPLE_TEST(Math, Transformd)
{
  TestTransform<double>();
}
