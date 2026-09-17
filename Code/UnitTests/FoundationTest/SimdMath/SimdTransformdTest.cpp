#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Transform.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdTransformd.h>

W_CREATE_SIMPLE_TEST(SimdMath, SimdTransformd)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor")
  {
    WSimdTransformd t0;

    {
      WSimdQuatd qRot = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(1, 2, 3).GetNormalized<3>(), WAngled::MakeFromDegree(42.0).GetRadian());

      WSimdVec4d pos(4, 5, 6);
      WSimdVec4d scale(7, 8, 9);

      WSimdTransformd t(pos);
      W_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      W_TEST_BOOL(t.m_Rotation == WSimdQuatd::MakeIdentity());
      W_TEST_BOOL((t.m_Scale == WSimdVec4d(1.0)).AllSet<3>());

      t = WSimdTransformd(pos, qRot);
      W_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      W_TEST_BOOL(t.m_Rotation == qRot);
      W_TEST_BOOL((t.m_Scale == WSimdVec4d(1.0)).AllSet<3>());

      t = WSimdTransformd(pos, qRot, scale);
      W_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      W_TEST_BOOL(t.m_Rotation == qRot);
      W_TEST_BOOL((t.m_Scale == scale).AllSet<3>());

      t = WSimdTransformd(qRot);
      W_TEST_BOOL(t.m_Position.IsZero<3>());
      W_TEST_BOOL(t.m_Rotation == qRot);
      W_TEST_BOOL((t.m_Scale == WSimdVec4d(1.0)).AllSet<3>());
    }

    {
      WSimdQuatd qRot;
      qRot = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(1, 2, 3).GetNormalized<3>(), WAngled::MakeFromDegree(42.0).GetRadian());

      WSimdVec4d pos(4, 5, 6);
      WSimdVec4d scale(7, 8, 9);

      WSimdTransformd t = WSimdTransformd::Make(pos);
      W_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      W_TEST_BOOL(t.m_Rotation == WSimdQuatd::MakeIdentity());
      W_TEST_BOOL((t.m_Scale == WSimdVec4d(1.0)).AllSet<3>());

      t = WSimdTransformd::Make(pos, qRot);
      W_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      W_TEST_BOOL(t.m_Rotation == qRot);
      W_TEST_BOOL((t.m_Scale == WSimdVec4d(1.0)).AllSet<3>());

      t = WSimdTransformd::Make(pos, qRot, scale);
      W_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      W_TEST_BOOL(t.m_Rotation == qRot);
      W_TEST_BOOL((t.m_Scale == scale).AllSet<3>());
    }

    {
      WSimdTransformd t = WSimdTransformd::MakeIdentity();

      W_TEST_BOOL(t.m_Position.IsZero<3>());
      W_TEST_BOOL(t.m_Rotation == WSimdQuatd::MakeIdentity());
      W_TEST_BOOL((t.m_Scale == WSimdVec4d(1.0)).AllSet<3>());

      W_TEST_BOOL(t == WSimdTransformd::MakeIdentity());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Inverse")
  {
    WSimdTransformd tParent(WSimdVec4d(1, 2, 3));
    tParent.m_Rotation = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 1, 0), WAngled::MakeFromDegree(90).GetRadian());
    tParent.m_Scale = WSimdVec4d(2.0);

    WSimdTransformd tToChild(WSimdVec4d(4, 5, 6));
    tToChild.m_Rotation = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 0, 1), WAngled::MakeFromDegree(90).GetRadian());
    tToChild.m_Scale = WSimdVec4d(4.0);

    WSimdTransformd tChild;
    tChild = tParent * tToChild;

    // invert twice -> get back original
    WSimdTransformd t2 = tToChild;
    t2.Invert();
    t2.Invert();
    W_TEST_BOOL(t2.IsEqual(tToChild, 0.0001f));

    WSimdTransformd tInvToChild = tToChild.GetInverse();

    WSimdTransformd tParentFromChild;
    tParentFromChild = tChild * tInvToChild;

    W_TEST_BOOL(tParent.IsEqual(tParentFromChild, 0.0001f));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetLocalTransform")
  {
    WSimdQuatd q;
    q = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 0, 1), WAngled::MakeFromDegree(90).GetRadian());

    WSimdTransformd tParent(WSimdVec4d(1, 2, 3));
    tParent.m_Rotation = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 1, 0), WAngled::MakeFromDegree(90).GetRadian());
    tParent.m_Scale = WSimdVec4d(2.0);

    WSimdTransformd tChild;
    tChild.m_Position = WSimdVec4d(13, 12, -5);
    tChild.m_Rotation = tParent.m_Rotation * q;
    tChild.m_Scale = WSimdVec4d(8.0);

    WSimdTransformd tToChild = WSimdTransformd::MakeLocalTransform(tParent, tChild);

    W_TEST_BOOL(tToChild.m_Position.IsEqual(WSimdVec4d(4, 5, 6), 0.0001f).AllSet<3>());
    W_TEST_BOOL(tToChild.m_Rotation.IsEqualRotation(q, 0.0001f));
    W_TEST_BOOL((tToChild.m_Scale == WSimdVec4d(4.0)).AllSet<3>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetGlobalTransform")
  {
    WSimdTransformd tParent(WSimdVec4d(1, 2, 3));
    tParent.m_Rotation = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 1, 0), WAngled::MakeFromDegree(90).GetRadian());
    tParent.m_Scale = WSimdVec4d(2.0);

    WSimdTransformd tToChild(WSimdVec4d(4, 5, 6));
    tToChild.m_Rotation = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 0, 1), WAngled::MakeFromDegree(90).GetRadian());
    tToChild.m_Scale = WSimdVec4d(4.0);

    WSimdTransformd tChild = WSimdTransformd::MakeGlobalTransform(tParent, tToChild);

    W_TEST_BOOL(tChild.m_Position.IsEqual(WSimdVec4d(13, 12, -5), 0.0001f).AllSet<3>());
    W_TEST_BOOL(tChild.m_Rotation.IsEqualRotation(tParent.m_Rotation * tToChild.m_Rotation, 0.0001f));
    W_TEST_BOOL((tChild.m_Scale == WSimdVec4d(8.0)).AllSet<3>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetAsMat4")
  {
    WSimdTransformd t(WSimdVec4d(1, 2, 3));
    t.m_Rotation = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 1, 0), WAngled::MakeFromDegree(34).GetRadian());
    t.m_Scale = WSimdVec4d(2, -1, 5);

    WSimdMat4d m = t.GetAsMat4();

    // reference
    WSimdMat4d refM;
    {
      WQuatd q = WQuatd::MakeFromAxisAndAngle(WVec3d(0, 1, 0), WAngled::MakeFromDegree(34));

      WTransformd referenceTransform(WVec3d(1, 2, 3), q, WVec3d(2, -1, 5));
      WMat4d tmp = referenceTransform.GetAsMat4();
      refM = WSimdMat4d::MakeFromColumnMajorArray(tmp.m_fElementsCM);
    }
    W_TEST_BOOL(m.IsEqual(refM, 0.00001f));

    WSimdVec4d p[8] = {WSimdVec4d(-4, 0, 0), WSimdVec4d(5, 0, 0), WSimdVec4d(0, -6, 0), WSimdVec4d(0, 7, 0), WSimdVec4d(0, 0, -8),
      WSimdVec4d(0, 0, 9), WSimdVec4d(1, -2, 3), WSimdVec4d(-4, 5, 7)};

    for (WUInt32 i = 0; i < W_ARRAY_SIZE(p); ++i)
    {
      WSimdVec4d pt = t.TransformPosition(p[i]);
      WSimdVec4d pm = m.TransformPosition(p[i]);

      W_TEST_BOOL(pt.IsEqual(pm, 0.00001f).AllSet<3>());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "TransformPos / Dir / operator*")
  {
    WSimdQuatd qRotX, qRotY;
    qRotX = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(1, 0, 0), WAngled::MakeFromDegree(90.0).GetRadian());
    qRotY = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 1, 0), WAngled::MakeFromDegree(90.0).GetRadian());

    WSimdTransformd t(WSimdVec4d(1, 2, 3, 10), qRotY * qRotX, WSimdVec4d(2, -2, 4, 11));

    WSimdVec4d v;
    v = t.TransformPosition(WSimdVec4d(4, 5, 6, 12));
    W_TEST_BOOL(v.IsEqual(WSimdVec4d((5 * -2) + 1, (-6 * 4) + 2, (-4 * 2) + 3), 0.0001f).AllSet<3>());

    v = t.TransformDirection(WSimdVec4d(4, 5, 6, 13));
    W_TEST_BOOL(v.IsEqual(WSimdVec4d((5 * -2), (-6 * 4), (-4 * 2)), 0.0001f).AllSet<3>());

    v = t * WSimdVec4d(4, 5, 6, 12);
    W_TEST_BOOL(v.IsEqual(WSimdVec4d((5 * -2) + 1, (-6 * 4) + 2, (-4 * 2) + 3), 0.0001f).AllSet<3>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Operators")
  {
    {
      WSimdTransformd tParent(WSimdVec4d(1, 2, 3));
      tParent.m_Rotation = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 1, 0), WAngled::MakeFromDegree(90).GetRadian());
      tParent.m_Scale = WSimdVec4d(2.0);

      WSimdTransformd tToChild(WSimdVec4d(4, 5, 6));
      tToChild.m_Rotation = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 0, 1), WAngled::MakeFromDegree(90).GetRadian());
      tToChild.m_Scale = WSimdVec4d(4.0);

      // this is exactly the same as SetGlobalTransform
      WSimdTransformd tChild;
      tChild = tParent * tToChild;

      W_TEST_BOOL(tChild.m_Position.IsEqual(WSimdVec4d(13, 12, -5), 0.0001f).AllSet<3>());
      W_TEST_BOOL(tChild.m_Rotation.IsEqualRotation(tParent.m_Rotation * tToChild.m_Rotation, 0.0001f));
      W_TEST_BOOL((tChild.m_Scale == WSimdVec4d(8.0)).AllSet<3>());

      tChild = tParent;
      tChild *= tToChild;

      W_TEST_BOOL(tChild.m_Position.IsEqual(WSimdVec4d(13, 12, -5), 0.0001f).AllSet<3>());
      W_TEST_BOOL(tChild.m_Rotation.IsEqualRotation(tParent.m_Rotation * tToChild.m_Rotation, 0.0001f));
      W_TEST_BOOL((tChild.m_Scale == WSimdVec4d(8.0)).AllSet<3>());

      WSimdVec4d a(7, 8, 9);
      WSimdVec4d b;
      b = tToChild.TransformPosition(a);
      b = tParent.TransformPosition(b);

      WSimdVec4d c;
      c = tChild.TransformPosition(a);

      W_TEST_BOOL(b.IsEqual(c, 0.0001f).AllSet());

      // verify that it works exactly like a 4x4 matrix
      /*const WMat4 mParent = tParent.GetAsMat4();
      const WMat4 mToChild = tToChild.GetAsMat4();
      const WMat4 mChild = mParent * mToChild;

      W_TEST_BOOL(mChild.IsEqual(tChild.GetAsMat4(), 0.0001f));*/
    }

    {
      WSimdTransformd t(WSimdVec4d(1, 2, 3));
      t.m_Rotation = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 1, 0), WAngled::MakeFromDegree(90).GetRadian());
      t.m_Scale = WSimdVec4d(2.0);

      WSimdQuatd q = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 0, 1), WAngled::MakeFromDegree(90).GetRadian());

      WSimdTransformd t2 = t * q;
      WSimdTransformd t4 = q * t;

      WSimdTransformd t3 = t;
      t3 *= q;
      W_TEST_BOOL(t2 == t3);
      W_TEST_BOOL(t3 != t4);

      WSimdVec4d a(7, 8, 9);
      WSimdVec4d b;
      b = t2.TransformPosition(a);

      WSimdVec4d c = q * a;
      c = t.TransformPosition(c);

      W_TEST_BOOL(b.IsEqual(c, 0.0001f).AllSet());
    }

    {
      WSimdTransformd t(WSimdVec4d(1, 2, 3));
      t.m_Rotation = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 1, 0), WAngle::MakeFromDegree(90).GetRadian());
      t.m_Scale = WSimdVec4d(2.0);

      WSimdVec4d p(4, 5, 6);

      WSimdTransformd t2 = t + p;
      WSimdTransformd t3 = t;
      t3 += p;
      W_TEST_BOOL(t2 == t3);

      WSimdVec4d a(7, 8, 9);
      WSimdVec4d b;
      b = t2.TransformPosition(a);

      WSimdVec4d c = t.TransformPosition(a) + p;

      W_TEST_BOOL(b.IsEqual(c, 0.0001f).AllSet());
    }

    {
      WSimdTransformd t(WSimdVec4d(1, 2, 3));
      t.m_Rotation = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 1, 0), WAngle::MakeFromDegree(90).GetRadian());
      t.m_Scale = WSimdVec4d(2.0);

      WSimdVec4d p(4, 5, 6);

      WSimdTransformd t2 = t - p;
      WSimdTransformd t3 = t;
      t3 -= p;
      W_TEST_BOOL(t2 == t3);

      WSimdVec4d a(7, 8, 9);
      WSimdVec4d b;
      b = t2.TransformPosition(a);

      WSimdVec4d c = t.TransformPosition(a) - p;

      W_TEST_BOOL(b.IsEqual(c, 0.0001f).AllSet());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Comparison")
  {
    WSimdTransformd t(WSimdVec4d(1, 2, 3));
    t.m_Rotation = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 1, 0), WAngled::MakeFromDegree(90).GetRadian());

    W_TEST_BOOL(t == t);

    WSimdTransformd t2(WSimdVec4d(1, 2, 4));
    t2.m_Rotation = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 1, 0), WAngled::MakeFromDegree(90).GetRadian());

    W_TEST_BOOL(t != t2);

    WSimdTransformd t3(WSimdVec4d(1, 2, 3));
    t3.m_Rotation = WSimdQuatd::MakeFromAxisAndAngle(WSimdVec4d(0, 1, 0), WAngled::MakeFromDegree(91).GetRadian());

    W_TEST_BOOL(t != t3);
  }
}
