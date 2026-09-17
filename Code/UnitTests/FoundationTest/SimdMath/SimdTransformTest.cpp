#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Transform.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdTransform.h>

W_CREATE_SIMPLE_TEST(SimdMath, SimdTransform)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor")
  {
    WSimdTransform t0;

    {
      WSimdQuat qRot = WSimdQuat::MakeFromAxisAndAngle(WSimdVec4f(1, 2, 3).GetNormalized<3>(), WAngle::MakeFromDegree(42.0f));

      WSimdVec4f pos(4, 5, 6);
      WSimdVec4f scale(7, 8, 9);

      WSimdTransform t(pos);
      W_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      W_TEST_BOOL(t.m_Rotation == WSimdQuat::MakeIdentity());
      W_TEST_BOOL((t.m_Scale == WSimdVec4f(1)).AllSet<3>());

      t = WSimdTransform(pos, qRot);
      W_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      W_TEST_BOOL(t.m_Rotation == qRot);
      W_TEST_BOOL((t.m_Scale == WSimdVec4f(1)).AllSet<3>());

      t = WSimdTransform(pos, qRot, scale);
      W_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      W_TEST_BOOL(t.m_Rotation == qRot);
      W_TEST_BOOL((t.m_Scale == scale).AllSet<3>());

      t = WSimdTransform(qRot);
      W_TEST_BOOL(t.m_Position.IsZero<3>());
      W_TEST_BOOL(t.m_Rotation == qRot);
      W_TEST_BOOL((t.m_Scale == WSimdVec4f(1)).AllSet<3>());
    }

    {
      WSimdQuat qRot;
      qRot = WSimdQuat::MakeFromAxisAndAngle(WSimdVec4f(1, 2, 3).GetNormalized<3>(), WAngle::MakeFromDegree(42.0f));

      WSimdVec4f pos(4, 5, 6);
      WSimdVec4f scale(7, 8, 9);

      WSimdTransform t = WSimdTransform::Make(pos);
      W_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      W_TEST_BOOL(t.m_Rotation == WSimdQuat::MakeIdentity());
      W_TEST_BOOL((t.m_Scale == WSimdVec4f(1)).AllSet<3>());

      t = WSimdTransform::Make(pos, qRot);
      W_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      W_TEST_BOOL(t.m_Rotation == qRot);
      W_TEST_BOOL((t.m_Scale == WSimdVec4f(1)).AllSet<3>());

      t = WSimdTransform::Make(pos, qRot, scale);
      W_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      W_TEST_BOOL(t.m_Rotation == qRot);
      W_TEST_BOOL((t.m_Scale == scale).AllSet<3>());
    }

    {
      WSimdTransform t = WSimdTransform::MakeIdentity();

      W_TEST_BOOL(t.m_Position.IsZero<3>());
      W_TEST_BOOL(t.m_Rotation == WSimdQuat::MakeIdentity());
      W_TEST_BOOL((t.m_Scale == WSimdVec4f(1)).AllSet<3>());

      W_TEST_BOOL(t == WSimdTransform::MakeIdentity());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Inverse")
  {
    WSimdTransform tParent(WSimdVec4f(1, 2, 3));
    tParent.m_Rotation = WSimdQuat::MakeFromAxisAndAngle(WSimdVec4f(0, 1, 0), WAngle::MakeFromDegree(90));
    tParent.m_Scale = WSimdVec4f(2);

    WSimdTransform tToChild(WSimdVec4f(4, 5, 6));
    tToChild.m_Rotation = WSimdQuat::MakeFromAxisAndAngle(WSimdVec4f(0, 0, 1), WAngle::MakeFromDegree(90));
    tToChild.m_Scale = WSimdVec4f(4);

    WSimdTransform tChild;
    tChild = tParent * tToChild;

    // invert twice -> get back original
    WSimdTransform t2 = tToChild;
    t2.Invert();
    t2.Invert();
    W_TEST_BOOL(t2.IsEqual(tToChild, 0.0001f));

    WSimdTransform tInvToChild = tToChild.GetInverse();

    WSimdTransform tParentFromChild;
    tParentFromChild = tChild * tInvToChild;

    W_TEST_BOOL(tParent.IsEqual(tParentFromChild, 0.0001f));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetLocalTransform")
  {
    WSimdQuat q;
    q = WSimdQuat::MakeFromAxisAndAngle(WSimdVec4f(0, 0, 1), WAngle::MakeFromDegree(90));

    WSimdTransform tParent(WSimdVec4f(1, 2, 3));
    tParent.m_Rotation = WSimdQuat::MakeFromAxisAndAngle(WSimdVec4f(0, 1, 0), WAngle::MakeFromDegree(90));
    tParent.m_Scale = WSimdVec4f(2);

    WSimdTransform tChild;
    tChild.m_Position = WSimdVec4f(13, 12, -5);
    tChild.m_Rotation = tParent.m_Rotation * q;
    tChild.m_Scale = WSimdVec4f(8);

    WSimdTransform tToChild = WSimdTransform::MakeLocalTransform(tParent, tChild);

    W_TEST_BOOL(tToChild.m_Position.IsEqual(WSimdVec4f(4, 5, 6), 0.0001f).AllSet<3>());
    W_TEST_BOOL(tToChild.m_Rotation.IsEqualRotation(q, 0.0001f));
    W_TEST_BOOL((tToChild.m_Scale == WSimdVec4f(4)).AllSet<3>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetGlobalTransform")
  {
    WSimdTransform tParent(WSimdVec4f(1, 2, 3));
    tParent.m_Rotation = WSimdQuat::MakeFromAxisAndAngle(WSimdVec4f(0, 1, 0), WAngle::MakeFromDegree(90));
    tParent.m_Scale = WSimdVec4f(2);

    WSimdTransform tToChild(WSimdVec4f(4, 5, 6));
    tToChild.m_Rotation = WSimdQuat::MakeFromAxisAndAngle(WSimdVec4f(0, 0, 1), WAngle::MakeFromDegree(90));
    tToChild.m_Scale = WSimdVec4f(4);

    WSimdTransform tChild = WSimdTransform::MakeGlobalTransform(tParent, tToChild);

    W_TEST_BOOL(tChild.m_Position.IsEqual(WSimdVec4f(13, 12, -5), 0.0001f).AllSet<3>());
    W_TEST_BOOL(tChild.m_Rotation.IsEqualRotation(tParent.m_Rotation * tToChild.m_Rotation, 0.0001f));
    W_TEST_BOOL((tChild.m_Scale == WSimdVec4f(8)).AllSet<3>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetAsMat4")
  {
    WSimdTransform t(WSimdVec4f(1, 2, 3));
    t.m_Rotation = WSimdQuat::MakeFromAxisAndAngle(WSimdVec4f(0, 1, 0), WAngle::MakeFromDegree(34));
    t.m_Scale = WSimdVec4f(2, -1, 5);

    WSimdMat4f m = t.GetAsMat4();

    // reference
    WSimdMat4f refM;
    {
      WQuat q = WQuat::MakeFromAxisAndAngle(WVec3(0, 1, 0), WAngle::MakeFromDegree(34));

      WTransform referenceTransform(WVec3(1, 2, 3), q, WVec3(2, -1, 5));
      WMat4 tmp = referenceTransform.GetAsMat4();
      refM = WSimdMat4f::MakeFromColumnMajorArray(tmp.m_fElementsCM);
    }
    W_TEST_BOOL(m.IsEqual(refM, 0.00001f));

    WSimdVec4f p[8] = {WSimdVec4f(-4, 0, 0), WSimdVec4f(5, 0, 0), WSimdVec4f(0, -6, 0), WSimdVec4f(0, 7, 0), WSimdVec4f(0, 0, -8),
      WSimdVec4f(0, 0, 9), WSimdVec4f(1, -2, 3), WSimdVec4f(-4, 5, 7)};

    for (WUInt32 i = 0; i < W_ARRAY_SIZE(p); ++i)
    {
      WSimdVec4f pt = t.TransformPosition(p[i]);
      WSimdVec4f pm = m.TransformPosition(p[i]);

      W_TEST_BOOL(pt.IsEqual(pm, 0.00001f).AllSet<3>());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "TransformPos / Dir / operator*")
  {
    WSimdQuat qRotX, qRotY;
    qRotX = WSimdQuat::MakeFromAxisAndAngle(WSimdVec4f(1, 0, 0), WAngle::MakeFromDegree(90.0f));
    qRotY = WSimdQuat::MakeFromAxisAndAngle(WSimdVec4f(0, 1, 0), WAngle::MakeFromDegree(90.0f));

    WSimdTransform t(WSimdVec4f(1, 2, 3, 10), qRotY * qRotX, WSimdVec4f(2, -2, 4, 11));

    WSimdVec4f v;
    v = t.TransformPosition(WSimdVec4f(4, 5, 6, 12));
    W_TEST_BOOL(v.IsEqual(WSimdVec4f((5 * -2) + 1, (-6 * 4) + 2, (-4 * 2) + 3), 0.0001f).AllSet<3>());

    v = t.TransformDirection(WSimdVec4f(4, 5, 6, 13));
    W_TEST_BOOL(v.IsEqual(WSimdVec4f((5 * -2), (-6 * 4), (-4 * 2)), 0.0001f).AllSet<3>());

    v = t * WSimdVec4f(4, 5, 6, 12);
    W_TEST_BOOL(v.IsEqual(WSimdVec4f((5 * -2) + 1, (-6 * 4) + 2, (-4 * 2) + 3), 0.0001f).AllSet<3>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Operators")
  {
    {
      WSimdTransform tParent(WSimdVec4f(1, 2, 3));
      tParent.m_Rotation = WSimdQuat::MakeFromAxisAndAngle(WSimdVec4f(0, 1, 0), WAngle::MakeFromDegree(90));
      tParent.m_Scale = WSimdVec4f(2);

      WSimdTransform tToChild(WSimdVec4f(4, 5, 6));
      tToChild.m_Rotation = WSimdQuat::MakeFromAxisAndAngle(WSimdVec4f(0, 0, 1), WAngle::MakeFromDegree(90));
      tToChild.m_Scale = WSimdVec4f(4);

      // this is exactly the same as SetGlobalTransform
      WSimdTransform tChild;
      tChild = tParent * tToChild;

      W_TEST_BOOL(tChild.m_Position.IsEqual(WSimdVec4f(13, 12, -5), 0.0001f).AllSet<3>());
      W_TEST_BOOL(tChild.m_Rotation.IsEqualRotation(tParent.m_Rotation * tToChild.m_Rotation, 0.0001f));
      W_TEST_BOOL((tChild.m_Scale == WSimdVec4f(8)).AllSet<3>());

      tChild = tParent;
      tChild *= tToChild;

      W_TEST_BOOL(tChild.m_Position.IsEqual(WSimdVec4f(13, 12, -5), 0.0001f).AllSet<3>());
      W_TEST_BOOL(tChild.m_Rotation.IsEqualRotation(tParent.m_Rotation * tToChild.m_Rotation, 0.0001f));
      W_TEST_BOOL((tChild.m_Scale == WSimdVec4f(8)).AllSet<3>());

      WSimdVec4f a(7, 8, 9);
      WSimdVec4f b;
      b = tToChild.TransformPosition(a);
      b = tParent.TransformPosition(b);

      WSimdVec4f c;
      c = tChild.TransformPosition(a);

      W_TEST_BOOL(b.IsEqual(c, 0.0001f).AllSet());

      // verify that it works exactly like a 4x4 matrix
      /*const WMat4 mParent = tParent.GetAsMat4();
      const WMat4 mToChild = tToChild.GetAsMat4();
      const WMat4 mChild = mParent * mToChild;

      W_TEST_BOOL(mChild.IsEqual(tChild.GetAsMat4(), 0.0001f));*/
    }

    {
      WSimdTransform t(WSimdVec4f(1, 2, 3));
      t.m_Rotation = WSimdQuat::MakeFromAxisAndAngle(WSimdVec4f(0, 1, 0), WAngle::MakeFromDegree(90));
      t.m_Scale = WSimdVec4f(2);

      WSimdQuat q = WSimdQuat::MakeFromAxisAndAngle(WSimdVec4f(0, 0, 1), WAngle::MakeFromDegree(90));

      WSimdTransform t2 = t * q;
      WSimdTransform t4 = q * t;

      WSimdTransform t3 = t;
      t3 *= q;
      W_TEST_BOOL(t2 == t3);
      W_TEST_BOOL(t3 != t4);

      WSimdVec4f a(7, 8, 9);
      WSimdVec4f b;
      b = t2.TransformPosition(a);

      WSimdVec4f c = q * a;
      c = t.TransformPosition(c);

      W_TEST_BOOL(b.IsEqual(c, 0.0001f).AllSet());
    }

    {
      WSimdTransform t(WSimdVec4f(1, 2, 3));
      t.m_Rotation = WSimdQuat::MakeFromAxisAndAngle(WSimdVec4f(0, 1, 0), WAngle::MakeFromDegree(90));
      t.m_Scale = WSimdVec4f(2);

      WSimdVec4f p(4, 5, 6);

      WSimdTransform t2 = t + p;
      WSimdTransform t3 = t;
      t3 += p;
      W_TEST_BOOL(t2 == t3);

      WSimdVec4f a(7, 8, 9);
      WSimdVec4f b;
      b = t2.TransformPosition(a);

      WSimdVec4f c = t.TransformPosition(a) + p;

      W_TEST_BOOL(b.IsEqual(c, 0.0001f).AllSet());
    }

    {
      WSimdTransform t(WSimdVec4f(1, 2, 3));
      t.m_Rotation = WSimdQuat::MakeFromAxisAndAngle(WSimdVec4f(0, 1, 0), WAngle::MakeFromDegree(90));
      t.m_Scale = WSimdVec4f(2);

      WSimdVec4f p(4, 5, 6);

      WSimdTransform t2 = t - p;
      WSimdTransform t3 = t;
      t3 -= p;
      W_TEST_BOOL(t2 == t3);

      WSimdVec4f a(7, 8, 9);
      WSimdVec4f b;
      b = t2.TransformPosition(a);

      WSimdVec4f c = t.TransformPosition(a) - p;

      W_TEST_BOOL(b.IsEqual(c, 0.0001f).AllSet());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Comparison")
  {
    WSimdTransform t(WSimdVec4f(1, 2, 3));
    t.m_Rotation = WSimdQuat::MakeFromAxisAndAngle(WSimdVec4f(0, 1, 0), WAngle::MakeFromDegree(90));

    W_TEST_BOOL(t == t);

    WSimdTransform t2(WSimdVec4f(1, 2, 4));
    t2.m_Rotation = WSimdQuat::MakeFromAxisAndAngle(WSimdVec4f(0, 1, 0), WAngle::MakeFromDegree(90));

    W_TEST_BOOL(t != t2);

    WSimdTransform t3(WSimdVec4f(1, 2, 3));
    t3.m_Rotation = WSimdQuat::MakeFromAxisAndAngle(WSimdVec4f(0, 1, 0), WAngle::MakeFromDegree(91));

    W_TEST_BOOL(t != t3);
  }
}
