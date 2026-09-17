#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Quat.h>

template <typename Type>
void TestQuat()
{
  using WQuatType = WQuatTemplate<Type>;
  using WVec3Type = WVec3Template<Type>;
  using WMat3Type = WMat3Template<Type>;
  using WMat4Type = WMat4Template<Type>;

  W_TEST_BLOCK(WTestBlock::Enabled, "Default Constructor")
  {
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
    if (WMath::SupportsNaN<Type>())
    {
      // In debug the default constructor initializes everything with NaN.
      WQuatType p;
      W_TEST_BOOL(WMath::IsNaN(p.x) && WMath::IsNaN(p.y) && WMath::IsNaN(p.z) && WMath::IsNaN(p.w));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    Type testBlock[4] = {(Type)1, (Type)2, (Type)3, (Type)4};
    WQuatType* p = ::new ((void*)&testBlock[0]) WQuatType;
    W_TEST_BOOL(p->x == (Type)1 && p->y == (Type)2 && p->z == (Type)3 && p->w == (Type)4);
#endif
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor(x,y,z,w)")
  {
    WQuatType q((Type)1, (Type)2, (Type)3, (Type)4);

    W_TEST_VEC3(q.GetVectorPart(), WVec3Type((Type)1, (Type)2, (Type)3), (Type)0.0001);
    W_TEST_FLOAT(q.w, (Type)4, (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeIdentity")
  {
    WQuatType q = WQuatType::MakeIdentity();

    W_TEST_VEC3(q.GetVectorPart(), WVec3Type((Type)0, (Type)0, (Type)0), (Type)0.0001);
    W_TEST_FLOAT(q.w, (Type)1, (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetIdentity")
  {
    WQuatType q((Type)1, (Type)2, (Type)3, (Type)4);

    q.SetIdentity();

    W_TEST_VEC3(q.GetVectorPart(), WVec3Type((Type)0, (Type)0, (Type)0), (Type)0.0001);
    W_TEST_FLOAT(q.w, (Type)1, (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetElements")
  {
    WQuatType q((Type)5, (Type)6, (Type)7, (Type)8);

    q = WQuatType((Type)1, (Type)2, (Type)3, (Type)4);

    W_TEST_VEC3(q.GetVectorPart(), WVec3Type((Type)1, (Type)2, (Type)3), (Type)0.0001);
    W_TEST_FLOAT(q.w, (Type)4, (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetFromAxisAndAngle / operator* (quat, vec)")
  {
    {
      WQuatType q;
      q = WQuatType::MakeFromAxisAndAngle(WVec3Type((Type)1, (Type)0, (Type)0), WAngleTemplate<Type>::MakeFromDegree((Type)90));

      W_TEST_VEC3(q * WVec3Type((Type)0, (Type)1, (Type)0), WVec3Type((Type)0, (Type)0, (Type)1), (Type)0.0001);
    }

    {
      WQuatType q;
      q = WQuatType::MakeFromAxisAndAngle(WVec3Type((Type)0, (Type)1, (Type)0), WAngleTemplate<Type>::MakeFromDegree((Type)90));

      W_TEST_VEC3(q * WVec3Type((Type)1, (Type)0, (Type)0), WVec3Type((Type)0, (Type)0, (Type)-1), (Type)0.0001);
    }

    {
      WQuatType q;
      q = WQuatType::MakeFromAxisAndAngle(WVec3Type((Type)0, (Type)0, (Type)1), WAngleTemplate<Type>::MakeFromDegree((Type)90));

      W_TEST_VEC3(q * WVec3Type((Type)0, (Type)1, (Type)0), WVec3Type((Type)-1, (Type)0, (Type)0), (Type)0.0001);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetShortestRotation / IsEqualRotation")
  {
    WQuatType q1, q2, q3;
    q1 = WQuatType::MakeShortestRotation(WVec3Type((Type)0, (Type)1, (Type)0), WVec3Type((Type)1, (Type)0, (Type)0));
    q2 = WQuatType::MakeFromAxisAndAngle(WVec3Type((Type)0, (Type)0, (Type)-1), WAngleTemplate<Type>::MakeFromDegree((Type)90));
    q3 = WQuatType::MakeFromAxisAndAngle(WVec3Type((Type)0, (Type)0, (Type)1), WAngleTemplate<Type>::MakeFromDegree((Type)-90));

    W_TEST_BOOL(q1.IsEqualRotation(q2, WMath::LargeEpsilon<Type>()));
    W_TEST_BOOL(q1.IsEqualRotation(q3, WMath::LargeEpsilon<Type>()));

    W_TEST_BOOL(WQuatType::MakeIdentity().IsEqualRotation(WQuatType::MakeIdentity(), WMath::LargeEpsilon<Type>()));
    W_TEST_BOOL(WQuatType::MakeIdentity().IsEqualRotation(WQuatType((Type)0, (Type)0, (Type)0, (Type)-1), WMath::LargeEpsilon<Type>()));

    WQuatType q4((Type)0, (Type)0, (Type)0, (Type)1 + WMath::VerySmallEpsilon<Type>()*1.2);
    WQuatType q5((Type)0, (Type)0, (Type)0, (Type)1 + WMath::VerySmallEpsilon<Type>()*2.3);
    W_TEST_BOOL(q4.IsEqualRotation(q5, WMath::LargeEpsilon<Type>()));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetFromMat3")
  {
    WMat3Type m;
    m = WMat3Type::MakeRotationZ(WAngleTemplate<Type>::MakeFromDegree((Type)-90));

    WQuatType q1, q2, q3;
    q1 = WQuatType::MakeFromMat3(m);
    q2 = WQuatType::MakeFromAxisAndAngle(WVec3Type((Type)0, (Type)0, (Type)-1), WAngleTemplate<Type>::MakeFromDegree((Type)90));
    q3 = WQuatType::MakeFromAxisAndAngle(WVec3Type((Type)0, (Type)0, (Type)1), WAngleTemplate<Type>::MakeFromDegree((Type)-90));

    W_TEST_BOOL(q1.IsEqualRotation(q2, WMath::LargeEpsilon<Type>()));
    W_TEST_BOOL(q1.IsEqualRotation(q3, WMath::LargeEpsilon<Type>()));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetSlerp")
  {
    WQuatType q1, q2, q3, qr;
    q1 = WQuatType::MakeFromAxisAndAngle(WVec3Type((Type)0, (Type)0, (Type)1), WAngleTemplate<Type>::MakeFromDegree((Type)45));
    q2 = WQuatType::MakeFromAxisAndAngle(WVec3Type((Type)0, (Type)0, (Type)1), WAngleTemplate<Type>::MakeFromDegree((Type)0));
    q3 = WQuatType::MakeFromAxisAndAngle(WVec3Type((Type)0, (Type)0, (Type)1), WAngleTemplate<Type>::MakeFromDegree((Type)90));

    qr = WQuatType::MakeSlerp(q2, q3, (Type)0.5);

    W_TEST_BOOL(q1.IsEqualRotation(qr, (Type)0.0001));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetRotationAxisAndAngle")
  {
    WQuatType q1, q2, q3;
    q1 = WQuatType::MakeShortestRotation(WVec3Type((Type)0, (Type)1, (Type)0), WVec3Type((Type)1, (Type)0, (Type)0));
    q2 = WQuatType::MakeFromAxisAndAngle(WVec3Type((Type)0, (Type)0, (Type)-1), WAngleTemplate<Type>::MakeFromDegree((Type)90));
    q3 = WQuatType::MakeFromAxisAndAngle(WVec3Type((Type)0, (Type)0, (Type)1), WAngleTemplate<Type>::MakeFromDegree((Type)-90));

    WVec3Type axis;
    WAngleTemplate<Type> angle;

    q1.GetRotationAxisAndAngle(axis, angle);
    W_TEST_VEC3(axis, WVec3Type((Type)0, (Type)0, (Type)-1), (Type)0.001);
    W_TEST_FLOAT(angle.GetDegree(), (Type)90, WMath::LargeEpsilon<Type>());

    q2.GetRotationAxisAndAngle(axis, angle);
    W_TEST_VEC3(axis, WVec3Type((Type)0, (Type)0, (Type)-1), (Type)0.001);
    W_TEST_FLOAT(angle.GetDegree(), (Type)90, WMath::LargeEpsilon<Type>());

    q3.GetRotationAxisAndAngle(axis, angle);
    W_TEST_VEC3(axis, WVec3Type((Type)0, (Type)0, (Type)-1), (Type)0.001);
    W_TEST_FLOAT(angle.GetDegree(), (Type)90, WMath::LargeEpsilon<Type>());

    WQuatType::MakeIdentity().GetRotationAxisAndAngle(axis, angle);
    W_TEST_VEC3(axis, WVec3Type((Type)1, (Type)0, (Type)0), (Type)0.001);
    W_TEST_FLOAT(angle.GetDegree(), (Type)0, WMath::LargeEpsilon<Type>());

    WQuatType otherIdentity((Type)0, (Type)0, (Type)0, (Type)-1);
    otherIdentity.GetRotationAxisAndAngle(axis, angle);
    W_TEST_VEC3(axis, WVec3Type((Type)1, (Type)0, (Type)0), (Type)0.001);
    W_TEST_FLOAT(angle.GetDegree(), (Type)360, WMath::LargeEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetAsMat3")
  {
    WQuatType q;
    q = WQuatType::MakeFromAxisAndAngle(WVec3Type((Type)0, (Type)0, (Type)1), WAngleTemplate<Type>::MakeFromDegree((Type)90));

    WMat3Type mr;
    mr = WMat3Type::MakeRotationZ(WAngleTemplate<Type>::MakeFromDegree((Type)90));

    WMat3Type m = q.GetAsMat3();

    W_TEST_BOOL(mr.IsEqual(m, WMath::DefaultEpsilon<Type>()));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetAsMat4")
  {
    WQuatType q;
    q = WQuatType::MakeFromAxisAndAngle(WVec3Type((Type)0, (Type)0, (Type)1), WAngleTemplate<Type>::MakeFromDegree((Type)90));

    WMat4Type mr;
    mr = WMat4Type::MakeRotationZ(WAngleTemplate<Type>::MakeFromDegree((Type)90));

    WMat4Type m = q.GetAsMat4();

    W_TEST_BOOL(mr.IsEqual(m, WMath::DefaultEpsilon<Type>()));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsValid / Normalize")
  {
    WQuatType q((Type)1, (Type)2, (Type)3, (Type)4);
    W_TEST_BOOL(!q.IsValid((Type)0.001));

    q.Normalize();
    W_TEST_BOOL(q.IsValid((Type)0.001));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetInverse / Invert")
  {
    WQuatType q, q1;
    q = WQuatType::MakeFromAxisAndAngle(WVec3Type((Type)0, (Type)0, (Type)1), WAngleTemplate<Type>::MakeFromDegree((Type)90));
    q1 = WQuatType::MakeFromAxisAndAngle(WVec3Type((Type)0, (Type)0, (Type)1), WAngleTemplate<Type>::MakeFromDegree((Type)-90));

    WQuatType q2 = q.GetInverse();
    W_TEST_BOOL(q1.IsEqualRotation(q2, (Type)0.0001));

    WQuatType q3 = q;
    q3.Invert();
    W_TEST_BOOL(q1.IsEqualRotation(q3, (Type)0.0001));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Dot")
  {
    WQuatType q, q1, q2;
    q = WQuatType::MakeFromAxisAndAngle(WVec3Type((Type)0, (Type)0, (Type)1), WAngleTemplate<Type>::MakeFromDegree((Type)90));
    q1 = WQuatType::MakeFromAxisAndAngle(WVec3Type((Type)0, (Type)0, (Type)1), WAngleTemplate<Type>::MakeFromDegree((Type)-90));
    q2 = WQuatType::MakeFromAxisAndAngle(WVec3Type((Type)0, (Type)1, (Type)0), WAngleTemplate<Type>::MakeFromDegree((Type)45));

    W_TEST_FLOAT(q.Dot(q), (Type)1.0, (Type)0.0001);
    W_TEST_FLOAT(q.Dot(WQuatType::MakeIdentity()), WMath::Cos(WAngleTemplate<Type>::MakeFromRadian(WAngleTemplate<Type>::DegToRad((Type)90.0 / (Type)2.0))), (Type)0.0001);
    W_TEST_FLOAT(q.Dot(q1), (Type)0.0, (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator*(quat, quat)")
  {
    WQuatType q1, q2, qr, q3;
    q1 = WQuatType::MakeFromAxisAndAngle(WVec3Type((Type)0, (Type)0, (Type)1), WAngleTemplate<Type>::MakeFromDegree((Type)60));
    q2 = WQuatType::MakeFromAxisAndAngle(WVec3Type((Type)0, (Type)0, (Type)1), WAngleTemplate<Type>::MakeFromDegree((Type)30));
    q3 = WQuatType::MakeFromAxisAndAngle(WVec3Type((Type)0, (Type)0, (Type)1), WAngleTemplate<Type>::MakeFromDegree((Type)90));

    qr = q1 * q2;

    W_TEST_BOOL(qr.IsEqualRotation(q3, (Type)0.0001));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator==/!=")
  {
    WQuatType q1, q2;
    q1 = WQuatType::MakeFromAxisAndAngle(WVec3Type((Type)0, (Type)0, (Type)1), WAngleTemplate<Type>::MakeFromDegree((Type)60));
    q2 = WQuatType::MakeFromAxisAndAngle(WVec3Type((Type)0, (Type)0, (Type)1), WAngleTemplate<Type>::MakeFromDegree((Type)30));
    W_TEST_BOOL(q1 != q2);

    q2 = WQuatType::MakeFromAxisAndAngle(WVec3Type((Type)1, (Type)0, (Type)0), WAngleTemplate<Type>::MakeFromDegree((Type)60));
    W_TEST_BOOL(q1 != q2);

    q2 = WQuatType::MakeFromAxisAndAngle(WVec3Type((Type)0, (Type)0, (Type)1), WAngleTemplate<Type>::MakeFromDegree((Type)60));
    W_TEST_BOOL(q1 == q2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsNaN")
  {
    if (WMath::SupportsNaN<Type>())
    {
      WQuatType q;

      q.SetIdentity();
      W_TEST_BOOL(!q.IsNaN());

      q.SetIdentity();
      q.w = WMath::NaN<Type>();
      W_TEST_BOOL(q.IsNaN());

      q.SetIdentity();
      q.x = WMath::NaN<Type>();
      W_TEST_BOOL(q.IsNaN());

      q.SetIdentity();
      q.y = WMath::NaN<Type>();
      W_TEST_BOOL(q.IsNaN());

      q.SetIdentity();
      q.z = WMath::NaN<Type>();
      W_TEST_BOOL(q.IsNaN());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "rotation direction")
  {
    WMat3Type m;
    m = WMat3Type::MakeRotationZ(WAngleTemplate<Type>::MakeFromDegree((Type)90.0));

    WQuatType q;
    q = WQuatType::MakeFromAxisAndAngle(WVec3Type((Type)0, (Type)0, (Type)1), WAngleTemplate<Type>::MakeFromDegree((Type)90.0));

    WVec3Type xAxis((Type)1, (Type)0, (Type)0);

    WVec3Type temp1 = m.TransformDirection(xAxis);
    WVec3Type temp2 = q.GetAsMat3().TransformDirection(xAxis);

    W_TEST_BOOL(temp1.IsEqual(temp2, (Type)0.01));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetAsEulerAngles / SetFromEulerAngles")
  {
    WAngleTemplate<Type> ax, ay, az;

    for (WUInt32 x = 0; x < 360; x += 15)
    {
      WQuatType q = WQuatType::MakeFromEulerAngles(WAngleTemplate<Type>::MakeFromDegree((Type)x), {}, {});

      WMat3Type m;
      m = WMat3Type::MakeRotationX(WAngleTemplate<Type>::MakeFromDegree((Type)x));
      WQuatType qm;
      qm = WQuatType::MakeFromMat3(m);
      W_TEST_BOOL(q.IsEqualRotation(qm, (Type)0.01));

      WVec3Type axis;
      WAngleTemplate<Type> angle;
      q.GetRotationAxisAndAngle(axis, angle, (Type)0.01);

      W_TEST_VEC3(axis, WVec3Type::MakeAxisX(), (Type)0.001);
      W_TEST_FLOAT(angle.GetDegree(), (Type)x, (Type)0.1);

      q.GetAsEulerAngles(ax, ay, az);
      W_TEST_BOOL(ax.IsEqualNormalized(WAngleTemplate<Type>::MakeFromDegree((Type)x), WAngleTemplate<Type>::MakeFromDegree((Type)0.1)));
    }

    for (WInt32 y = -90; y < 360; y += 15)
    {
      WQuatType q = WQuatType::MakeFromEulerAngles({}, WAngleTemplate<Type>::MakeFromDegree((Type)y), {});

      WMat3Type m;
      m = WMat3Type::MakeRotationY(WAngleTemplate<Type>::MakeFromDegree((Type)y));
      WQuatType qm;
      qm = WQuatType::MakeFromMat3(m);
      W_TEST_BOOL(q.IsEqualRotation(qm, (Type)0.01));

      WVec3Type axis;
      WAngleTemplate<Type> angle;
      q.GetRotationAxisAndAngle(axis, angle, (Type)0.01);

      if (y < 0)
      {
        W_TEST_VEC3(axis, -WVec3Type::MakeAxisY(), (Type)0.001);
        W_TEST_FLOAT(angle.GetDegree(), (Type)-y, (Type)0.1);
      }
      else if (y > 0)
      {
        W_TEST_VEC3(axis, WVec3Type::MakeAxisY(), (Type)0.001);
        W_TEST_FLOAT(angle.GetDegree(), (Type)y, (Type)0.1);
      }

      // pitch is only defined in -90..90 range
      if (y >= -90 && y <= 90)
      {
        q.GetAsEulerAngles(ax, ay, az);
        W_TEST_FLOAT(ay.GetDegree(), (Type)y, (Type)0.1);
      }
    }

    for (WUInt32 z = 15; z < 360; z += 15)
    {
      WQuatType q = WQuatType::MakeFromEulerAngles({}, {}, WAngleTemplate<Type>::MakeFromDegree((Type)z));

      WMat3Type m;
      m = WMat3Type::MakeRotationZ(WAngleTemplate<Type>::MakeFromDegree((Type)z));
      WQuatType qm;
      qm = WQuatType::MakeFromMat3(m);
      W_TEST_BOOL(q.IsEqualRotation(qm, (Type)0.01));

      WVec3Type axis;
      WAngleTemplate<Type> angle;
      q.GetRotationAxisAndAngle(axis, angle, (Type)0.01);

      W_TEST_VEC3(axis, WVec3Type::MakeAxisZ(), (Type)0.001);
      W_TEST_FLOAT(angle.GetDegree(), (Type)z, (Type)0.1);

      q.GetAsEulerAngles(ax, ay, az);
      W_TEST_BOOL(az.IsEqualNormalized(WAngleTemplate<Type>::MakeFromDegree((Type)z), WAngleTemplate<Type>::MakeFromDegree((Type)0.1)));
    }

    for (WUInt32 x = 0; x < 360; x += 15)
    {
      for (WUInt32 y = 0; y < 360; y += 15)
      {
        for (WUInt32 z = 0; z < 360; z += 30)
        {
          WQuatType q1 = WQuatType::MakeFromEulerAngles(WAngleTemplate<Type>::MakeFromDegree((Type)x), WAngleTemplate<Type>::MakeFromDegree((Type)y), WAngleTemplate<Type>::MakeFromDegree((Type)z));

          q1.GetAsEulerAngles(ax, ay, az);

          WQuatType q2 = WQuatType::MakeFromEulerAngles(ax, ay, az);

          W_TEST_BOOL(q1.IsEqualRotation(q2, (Type)0.1));

          // Check that euler order is ZYX aka 3-2-1
          WQuatType q3;
          {
            WQuatType xRot, yRot, zRot;
            xRot = WQuatType::MakeFromAxisAndAngle(WVec3Type::MakeAxisX(), WAngleTemplate<Type>::MakeFromDegree((Type)x));
            yRot = WQuatType::MakeFromAxisAndAngle(WVec3Type::MakeAxisY(), WAngleTemplate<Type>::MakeFromDegree((Type)y));
            zRot = WQuatType::MakeFromAxisAndAngle(WVec3Type::MakeAxisZ(), WAngleTemplate<Type>::MakeFromDegree((Type)z));

            q3 = zRot * yRot * xRot;
          }
          W_TEST_BOOL(q1.IsEqualRotation(q3, (Type)0.01));
        }
      }
    }
  }
}

W_CREATE_SIMPLE_TEST(Math, Quaternionf)
{
  TestQuat<float>();
}

W_CREATE_SIMPLE_TEST(Math, Quaterniond)
{
  TestQuat<double>();
}
