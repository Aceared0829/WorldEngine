#include "../TestFramework.as"

void ExecuteTests()
{
    WString m_sString = "String";
    WHashedString m_sHashedString = "HashedString";
    WTempHashedString m_sTempHashedString = "TempHashedString";
    WStringView svLong = "Nett hier. Aber waren sie schon mal in Baden-Württemberg?";

    // WStringView
    {
        WStringView sv0;
        W_TEST_BOOL(sv0.IsEmpty());

        WStringView sv1("Test1");
        W_TEST_BOOL(!sv1.IsEmpty());
        W_TEST_STRING(sv1, "Test1");

        WStringView sv2(sv1);
        W_TEST_BOOL(sv1 == sv2);
        W_TEST_BOOL(sv1 == "Test1");
        W_TEST_STRING(sv1, sv2);

        WStringView sv3(m_sString);
        W_TEST_STRING(sv3, m_sString);

        WStringBuilder sb1;
        sb1 = "Test1";
        W_TEST_STRING(sv1, sb1);
        W_TEST_BOOL(sv1 == sb1);

        WStringBuilder sb2("Test2");

        WStringView sv4(sb1);
        W_TEST_STRING(sv4, sb1);
        sv4 = sb2;
        W_TEST_STRING(sv4, sb2);
        W_TEST_BOOL(sv4 == sb2);
        W_TEST_BOOL(sv4 != sb1);

        sv1 = m_sHashedString;
        W_TEST_STRING(sv1, m_sHashedString.GetView());
        W_TEST_STRING(sv1, m_sHashedString);

        sv1 = m_sString;
        W_TEST_STRING(sv1, m_sString);

        sv1 = sb2;
        W_TEST_STRING(sv1, sb2);

        W_TEST_BOOL("Abcd" < "aBCD");

        W_TEST_BOOL("abc" > "ABC");
    }

    // WString
    {
        WString s0;
        W_TEST_BOOL(s0.IsEmpty());

        WString s1("abc");
        W_TEST_STRING(s1, "abc");

        s1 = svLong;
        W_TEST_BOOL(s1 == svLong);
        W_TEST_STRING(s1, svLong);
        W_TEST_INT(s1.GetElementCount(), 58);

        WString s2(s1);
        W_TEST_STRING(s2, svLong);

        WStringBuilder sb1(svLong);
        W_TEST_INT(sb1.GetCharacterCount(), 57);

        W_TEST_BOOL(!s2.IsEmpty());
        s2.Clear();
        W_TEST_BOOL(s2.IsEmpty());

        s2 = sb1;
        W_TEST_STRING(s2, svLong);

        WString s3(sb1);
        W_TEST_STRING(s3, svLong);
        s3 = m_sHashedString;
        W_TEST_STRING(s3, m_sHashedString);

        WString s4(m_sHashedString);
        W_TEST_STRING(s4, m_sHashedString);

        s4 = "TEST";
        W_TEST_STRING(s4, "TEST");

        s4 = s3;
        W_TEST_STRING(s4, m_sHashedString);

        s3 = "ABC";
        s4 = "abc";
        W_TEST_BOOL(s3 < s4);
    }

    // WStringBuilder
    {
        WStringBuilder sb1;
        WStringBuilder sb2b("T", "e");
        WStringBuilder sb2("T", "e", "s", "t");
        WStringBuilder sb3(m_sString);
        WStringBuilder sb4(m_sHashedString);
        WStringBuilder sb5(sb2);

        W_TEST_BOOL(sb1.IsEmpty());
        W_TEST_STRING(sb2b, "Te");
        W_TEST_STRING(sb2, "Test");
        W_TEST_STRING(sb3, m_sString);
        W_TEST_STRING(sb4, m_sHashedString);
        W_TEST_STRING(sb5, sb2);

        sb1.SetFormat("a {} {} {} {}", 1, "b", 2.3f, WTime::Seconds(7));
        W_TEST_STRING(sb1, "a 1 b 2.3 7sec");
    }

    // WHashedString
    {
        WHashedString hs1();
        W_TEST_BOOL(hs1.IsEmpty());

        WHashedString hs2("Hallo");
        W_TEST_STRING(hs2, "Hallo");
        W_TEST_BOOL(!hs2.IsEmpty());

        WHashedString hs3(hs2);
        W_TEST_STRING(hs2, hs3);
        W_TEST_BOOL(hs2 == hs3);

        hs1.Assign("Text");
        W_TEST_STRING(hs1, "Text");
        W_TEST_BOOL(hs1 != hs3);

        hs1 = "Bla";
        W_TEST_STRING(hs1, "Bla");
        W_TEST_BOOL(hs1 == "Bla");

        hs1 = m_sString;
        W_TEST_STRING(hs1,  m_sString);
        W_TEST_BOOL(!hs1.IsEmpty());
        hs1.Clear();
        W_TEST_BOOL(hs1.IsEmpty());

        WTempHashedString ths = "Hallo";
        W_TEST_BOOL(hs2 == ths);
    }

    // WTempHashedString
    {
        WTempHashedString ths1;
        W_TEST_BOOL(ths1.IsEmpty());

        WTempHashedString ths2(svLong);
        WHashedString hs1(svLong);
        W_TEST_BOOL(hs1 == ths2);

        WTempHashedString ths3(ths2);
        W_TEST_BOOL(ths2 == ths3);

        WTempHashedString ths4(hs1);
        W_TEST_BOOL(ths4 == hs1);

        ths4 = "Guck guck";
        W_TEST_BOOL(ths4 != hs1);
        W_TEST_BOOL(!ths4.IsEmpty());

        ths4 = hs1;
        W_TEST_BOOL(ths4 == hs1);
        ths4.Clear();
        W_TEST_BOOL(ths4.IsEmpty());
    }
}