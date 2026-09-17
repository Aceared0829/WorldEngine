void TestFailed(WStringView sMsg)
{
    WLog::Error(sMsg);
    throw(sMsg);
}

void W_TEST_BOOL(bool condition)
{
    if (!condition)
    {
        TestFailed("BOOL condition is false");
    }
}

void W_TEST_INT(int i1, int i2)
{
    if (i1 != i2)
    {
        WStringBuilder s;
        s.SetFormat("{} does not equal {}", i1, i2);
        TestFailed(s);
    }
}

void W_TEST_FLOAT(float f1, float f2, float epsilon = 0.001)
{
    if (!WMath::IsEqual(f1, f2, epsilon))
    {
        WStringBuilder s;
        s.SetFormat("{} does not equal {}", f1, f2);
        TestFailed(s);
    }
}

void W_TEST_VEC2(WVec2 v1, WVec2 v2, float epsilon = 0.001)
{
    if (!v1.IsEqual(v2, epsilon))
    {
        WStringBuilder s;
        s.SetFormat("({}, {}) does not equal ({}, {})", v1.x, v1.y, v2.x, v2.y);
        TestFailed(s);
    }
}

void W_TEST_VEC3(WVec3 v1, WVec3 v2, float epsilon = 0.001)
{
    if (!v1.IsEqual(v2, epsilon))
    {
        WStringBuilder s;
        s.SetFormat("({}, {}, {}) does not equal ({}, {}, {})", v1.x, v1.y, v1.z, v2.x, v2.y, v2.z);
        TestFailed(s);

    }
}

void W_TEST_VEC4(WVec4 v1, WVec4 v2, float epsilon = 0.001)
{
    if (!v1.IsEqual(v2, epsilon))
    {
        WStringBuilder s;
        s.SetFormat("({}, {}, {}, {}) does not equal ({}, {}, {}, {})", v1.x, v1.y, v1.z, v1.w, v2.x, v2.y, v2.z, v2.w);
        TestFailed(s);

    }
}

void W_TEST_QUAT(WQuat q1, WQuat q2, float epsilon = 0.001)
{
    if (!q1.IsEqualRotation(q2, epsilon))
    {
        WStringBuilder s;
        s.SetFormat("({}, {}, {}, {}) does not equal ({}, {}, {}, {})", q1.x, q1.y, q1.z, q1.w, q2.x, q2.y, q2.z, q2.w);
        TestFailed(s);
    }
}

void W_TEST_COLOR(WColor c1, WColor c2, float epsilon = 0.001)
{
    if (!c1.IsEqualRGBA(c2, epsilon))
    {
        WStringBuilder s;
        s.SetFormat("({}, {}, {}, {}) does not equal ({}, {}, {}, {})", c1.r, c1.g, c1.b, c1.a, c2.r, c2.g, c2.b, c2.a);
        TestFailed(s);
    }
}

void W_TEST_STRING(WStringView s1, WStringView s2)
{
    if (s1 != s2)
    {
        WStringBuilder s;
        s.SetFormat("'{}' does not equal '{}'", s1, s2);
        TestFailed(s);
    }
}

class WAngelScriptTestClass : WAngelScriptClass
{
    private WString m_sTestName;

    WAngelScriptTestClass(WStringView sName)
    {
        m_sTestName = sName;
    }

    void OnMsgGenericEvent(WMsgGenericEvent@ msg)
    {
        if (msg.Message == m_sTestName)
        {
            if (ExecuteTests())
                msg.Message = "repeat";
            else
                msg.Message = "done";
        }
    }    

    bool ExecuteTests()
    {
        throw("Function not implemented");
        return false;
    }
}