#include "../TestFramework.as"

void ExecuteTests()
{
    // These tests only test the general AS/C++ binding.
    // The are meant to find cases where the binding wasn't set up correctly, 
    // which mostly happens for operators, constructors and other special functions.
    // They do not test all the functionality, because that is already covered by the C++ tests.

    // WAngle
    {
        WAngle a, b;
        a = WAngle::MakeFromDegree(90);

        W_TEST_FLOAT(a.GetDegree(), 90.0f);
        b = -a;

        W_TEST_FLOAT(b.GetDegree(), -90.0f);

        WAngle c = a + b;
        c += a;
        c -= b;

        W_TEST_FLOAT(c.GetDegree(), c.GetDegree());
        W_TEST_BOOL(c == c);
        W_TEST_FLOAT(c.GetDegree(), (a * 2.0).GetDegree());
        W_TEST_BOOL(c == 2.0f * a);
        W_TEST_BOOL(c / 2.0f == a);
        W_TEST_BOOL(c / a == 2.0f);

        W_TEST_BOOL(a < c);
        W_TEST_BOOL(c > a);

        WAngle d = WAngle::AngleBetween(a, b);
        W_TEST_FLOAT(d.GetDegree(), 180);
    }

    // WTime
    {
        WTime t1 = WTime::MakeZero();
        W_TEST_FLOAT(t1.AsFloatInSeconds(), 0.0f);

        t1 = WTime::MakeFromSeconds(1.23f);
        W_TEST_FLOAT(t1.AsFloatInSeconds(), 1.23f);

        t1 += WTime::Seconds(2.0f);
        t1 -= WTime::Seconds(0.1f);
        W_TEST_FLOAT(t1.AsFloatInSeconds(), 3.13f);

        t1 *= 4.0f;
        t1 /= 2.0f;
        t1 = -t1;
        W_TEST_FLOAT(t1.AsFloatInSeconds(), -6.26f);

        WTime t2 = WTime::Seconds(6.26f);
        W_TEST_FLOAT(t1.AsFloatInSeconds(), -t2.AsFloatInSeconds());

        WTime t3 = t1;
        W_TEST_BOOL(t1 == t3);
        W_TEST_BOOL(t1 < t2);
        W_TEST_BOOL(t2 > t1);

        t3 *= 2.0f;
        W_TEST_BOOL(t3 < t1);
        
        t3 /= 2.0f;
        W_TEST_FLOAT(t1.AsFloatInSeconds(), t3.AsFloatInSeconds());

        t1 = 4 * (WTime::Seconds(2) * WTime::Seconds(3));
        t2 = WTime::Seconds(4) * 1.5f;
        W_TEST_FLOAT(t1.AsFloatInSeconds(), 24);
        W_TEST_FLOAT(t2.AsFloatInSeconds(), 6);
        
        t3 = t1 + t2;
        W_TEST_FLOAT(t3.AsFloatInSeconds(), 30);
        
        t3 = t3 - t1;
        W_TEST_FLOAT(t3.AsFloatInSeconds(), 6);

        t3 = t1 / t2;
        W_TEST_FLOAT(t3.AsFloatInSeconds(), 4);

        t3 = t1 / 3.0f;
        W_TEST_FLOAT(t3.AsFloatInSeconds(), 8);

        t3 = 48 / t1;
        W_TEST_FLOAT(t3.AsFloatInSeconds(), 2);
    }

    // WColor / WColorGammaUB
    {
        WColor c1(0, 0, 0);
        W_TEST_COLOR(c1, WColor::Black);

        WColor c2(WColorGammaUB(255, 255, 255));
        W_TEST_COLOR(c2, WColor::White);

        c2 = c1;
        W_TEST_COLOR(c2, WColor::Black);

        c1 = WColorGammaUB(255, 255, 255);
        W_TEST_COLOR(c1, WColor::White);

        c2 += WColor::White;
        W_TEST_COLOR(c2, WColor(1, 1, 1, 2));

        c2 -= WColor::White;
        W_TEST_COLOR(c2, WColor::Black);

        c1 *= 2;
        W_TEST_COLOR(c1, WColor::White * 2);

        c1 /= 2;
        W_TEST_COLOR(c1, (2 * WColor::White) / 2);

        c1 *= c1 + c1;
        W_TEST_COLOR(c1, WColor::White * 2);

        c1 = c1 - c1;
        W_TEST_COLOR(c1, WColor::MakeZero());

        W_TEST_BOOL(c1 == WColor::MakeZero());

        W_TEST_VEC4(WColor::White.WithAlpha(0.5).GetAsVec4(), WVec4(1, 1, 1, 0.5));

        W_TEST_COLOR(WColor::Red * WColor::Blue, WColor::Black);

        W_TEST_VEC4((WColor::White / 2.0f).GetAsVec4(), WVec4(0.5f));

        WColorGammaUB cg = WColor::Lime;
        W_TEST_INT(cg.r, 0);
        W_TEST_INT(cg.g, 255);
        W_TEST_INT(cg.b, 0);
        W_TEST_INT(cg.a, 255);

        cg = WColor::Red;
        W_TEST_INT(cg.r, 255);
        W_TEST_INT(cg.g, 0);
        W_TEST_INT(cg.b, 0);
        W_TEST_INT(cg.a, 255);
    }

    // WVec2
    {
        const WVec2 c0 = WVec2::MakeZero();
        const WVec2 c1(2, 4);
        const WVec2 c2(4, 2);
        const WVec2 c3(6);

        W_TEST_FLOAT(c0.x, 0);
        W_TEST_FLOAT(c0.y, 0);

        W_TEST_FLOAT(c1.x, 2);
        W_TEST_FLOAT(c1.y, 4);

        W_TEST_FLOAT(c3.x, 6);
        W_TEST_FLOAT(c3.y, 6);

        W_TEST_BOOL(c1 != c2);
        W_TEST_BOOL(c1 == c1);
        W_TEST_BOOL(c1 < c3);
        W_TEST_BOOL(c3 > c1);

        WVec2 v;
        v = c0;
        W_TEST_VEC2(v, c0);
        
        v += 2 * c1;
        W_TEST_VEC2(v, c1 * 2);
        
        v -= c1 * 2;
        W_TEST_VEC2(v, c0);

        v = c3;
        v *= 4;
        v /= 8;
        W_TEST_VEC2(v, c3 / 2);
        
        v = -1 * (c1 + c2);
        W_TEST_VEC2(v, -c3);
        
        v = c3 - c1;
        W_TEST_VEC2(v, c2);
    }

    // WVec3
    {
        const WVec3 c0 = WVec3::MakeZero();
        const WVec3 c1(2, 4, 3);
        const WVec3 c2(4, 2, 3);
        const WVec3 c3(6);

        W_TEST_FLOAT(c0.x, 0);
        W_TEST_FLOAT(c0.y, 0);
        W_TEST_FLOAT(c0.z, 0);

        W_TEST_FLOAT(c1.x, 2);
        W_TEST_FLOAT(c1.y, 4);
        W_TEST_FLOAT(c1.z, 3);

        W_TEST_FLOAT(c3.x, 6);
        W_TEST_FLOAT(c3.y, 6);
        W_TEST_FLOAT(c3.z, 6);

        W_TEST_BOOL(c1 != c2);
        W_TEST_BOOL(c1 == c1);
        W_TEST_BOOL(c1 < c3);
        W_TEST_BOOL(c3 > c1);

        WVec3 v;
        v = c0;
        W_TEST_VEC3(v, c0);
        
        v += 2 * c1;
        W_TEST_VEC3(v, c1 * 2);
        
        v -= c1 * 2;
        W_TEST_VEC3(v, c0);

        v = c3;
        v *= 4;
        v /= 8;
        W_TEST_VEC3(v, c3 / 2);
        
        v = -1 * (c1 + c2);
        W_TEST_VEC3(v, -c3);
        
        v = c3 - c1;
        W_TEST_VEC3(v, c2);
    }

    // WVec4
    {
        const WVec4 c0 = WVec4::MakeZero();
        const WVec4 c1(2, 4, 3, 1);
        const WVec4 c2(4, 2, 3, 5);
        const WVec4 c3(6);

        W_TEST_FLOAT(c0.x, 0);
        W_TEST_FLOAT(c0.y, 0);
        W_TEST_FLOAT(c0.z, 0);
        W_TEST_FLOAT(c0.w, 0);

        W_TEST_FLOAT(c1.x, 2);
        W_TEST_FLOAT(c1.y, 4);
        W_TEST_FLOAT(c1.z, 3);
        W_TEST_FLOAT(c1.w, 1);

        W_TEST_FLOAT(c3.x, 6);
        W_TEST_FLOAT(c3.y, 6);
        W_TEST_FLOAT(c3.y, 6);
        W_TEST_FLOAT(c3.y, 6);

        W_TEST_BOOL(c1 != c2);
        W_TEST_BOOL(c1 == c1);
        W_TEST_BOOL(c1 < c3);
        W_TEST_BOOL(c3 > c1);

        WVec4 v;
        v = c0;
        W_TEST_VEC4(v, c0);
        
        v += 2 * c1;
        W_TEST_VEC4(v, c1 * 2);
        
        v -= c1 * 2;
        W_TEST_VEC4(v, c0);

        v = c3;
        v *= 4;
        v /= 8;
        W_TEST_VEC4(v, c3 / 2);
        
        v = -1 * (c1 + c2);
        W_TEST_VEC4(v, -c3);
        
        v = c3 - c1;
        W_TEST_VEC4(v, c2);
    }

    // WQuat
    {
        W_TEST_QUAT(WQuat::MakeIdentity(), WQuat::MakeFromElements(0, 0, 0, 1));

        WQuat q = WQuat::MakeFromElements(1, 2, 3, 4);
        W_TEST_FLOAT(q.x, 1);
        W_TEST_FLOAT(q.y, 2);
        W_TEST_FLOAT(q.z, 3);
        W_TEST_FLOAT(q.w, 4);

        W_TEST_BOOL(q == q);
        W_TEST_BOOL(q != q.GetNegated());

        WQuat q2 = q;
        W_TEST_BOOL(q2 == q);

        q2 = WQuat::MakeFromAxisAndAngle(WVec3(1, 0, 0), WAngle::MakeFromDegree(90));
        WVec3 v = q2 * WVec3(1, 2, 3);

        W_TEST_VEC3(v, WVec3(1, -3, 2));

        q = q2 * q2;

        WVec3 axis;
        WAngle angle;
        q.GetRotationAxisAndAngle(axis, angle);

        W_TEST_VEC3(axis, WVec3(1, 0, 0));
        W_TEST_FLOAT(angle.GetDegree(), 180);
    }

    // WTransform
    {
        WTransform t1 = WTransform::MakeIdentity();
        W_TEST_VEC3(t1.m_vPosition, WVec3::MakeZero());
        W_TEST_QUAT(t1.m_qRotation, WQuat::MakeIdentity());
        W_TEST_VEC3(t1.m_vScale, WVec3(1));

        W_TEST_BOOL(t1 == t1.GetInverse());
        
        WTransform t2;
        t2 = t1;

        W_TEST_BOOL(t1 == t2);
        
        t1 += WVec3(1);
        W_TEST_BOOL(t1 != t2);
        W_TEST_BOOL(t1 == t2 + WVec3(1));
        W_TEST_BOOL(t1 - WVec3(1) == t2);

        t1 -= WVec3(1);
        W_TEST_BOOL(t1 == t2);

        W_TEST_BOOL(WTransform::Make(WVec3(1), WQuat::MakeIdentity(), WVec3(2)) == WTransform(WVec3(1), WQuat::MakeIdentity(), WVec3(2)));

        t1 = WTransform::Make(WVec3(1, 2, 3));
        WVec3 pos(2, 3, 4);
        pos = t1 * pos;
        W_TEST_VEC3(pos, WVec3(3, 5, 7));

        WQuat rot = WQuat::MakeFromAxisAndAngle(WVec3(1, 0, 0), WAngle::MakeFromDegree(90));
        t1 = t1 * rot;
        t1 = rot * t1;

        WTransform t3 = t1 * t2;
        WTransform t4 = WTransform::MakeGlobalTransform(t1, t2);
        W_TEST_BOOL(t3 == t4);
    }

    // WMath
    {
        // nothing that needs testing
    }
}