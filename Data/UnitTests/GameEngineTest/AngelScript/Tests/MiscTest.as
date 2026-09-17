#include "TestFramework.as"

enum Phase
{
    CVar,
    Array,
    Prefabs,
    Done
}

class ScriptObject : WAngelScriptTestClass
{
    private Phase m_Phase = Phase::CVar;

    ScriptObject()
    {
        super("MiscTest");
    }

    bool ExecuteTests()
    {
        if (m_Phase == Phase::CVar)
        {
            bool bOrgValue = WCVar::GetBoolValue("App.ShowFPS");

            bool bShowFPS1 = WCVar::GetBoolValue("App.ShowFPS");
            bool bShowFPS2 = WCVar::GetValue_asBool("App.ShowFPS");
            W_TEST_BOOL(bShowFPS1 == bShowFPS2);

            WCVar::SetBoolValue("App.ShowFPS", !bOrgValue);
            
            bShowFPS1 = WCVar::GetBoolValue("App.ShowFPS");
            bShowFPS2 = WCVar::GetValue_asBool("App.ShowFPS");
            W_TEST_BOOL(bShowFPS1 == bShowFPS2);

            WCVar::SetBoolValue("App.ShowFPS", false);

            m_Phase = Phase::Array;
        }
        else if (m_Phase == Phase::Array)
        {
            array<WInt32> a;
            W_TEST_BOOL(a.IsEmpty());
            a.SetCount(32);
            W_TEST_INT(a.GetCount(), 32);
            W_TEST_BOOL(!a.IsEmpty());
            
            for (WUInt32 i = 0; i < 32; ++i)
            {
                W_TEST_INT(a[i], 0);
                a[i] = i + 1;
                W_TEST_INT(a[i], i + 1);
            }

            W_TEST_INT(a.PeekBack(), 32);

            W_TEST_BOOL(a.Contains(11));
            W_TEST_BOOL(!a.Contains(0));
            W_TEST_BOOL(!a.Contains(33));

            array<WInt32> b = a;
            W_TEST_BOOL(a == b);

            a.ExpandAndGetRef() = 33;
            W_TEST_INT(a.GetCount(), 33);
            W_TEST_INT(a[32], 33);

            W_TEST_INT(a.IndexOf(13), 12);

            W_TEST_BOOL(a != b);
            b.ExpandAndGetRef() = 33;
            W_TEST_BOOL(a == b);

            a.PopBack();
            W_TEST_BOOL(a != b);
            b.PopBack();
            W_TEST_BOOL(a == b);

            a.Reverse();

            for (WUInt32 i = 0; i < 32; ++i)
            {
                W_TEST_INT(a[i], 32 - i);
            }

            W_TEST_BOOL(a != b);
            a.Sort();
            W_TEST_BOOL(a == b);

            a.Clear();
            W_TEST_BOOL(a.IsEmpty());
            
            b.SetCount(0);
            W_TEST_BOOL(b.IsEmpty());
            W_TEST_BOOL(a == b);

            m_Phase = Phase::Prefabs;
        }
        else if (m_Phase == Phase::Prefabs)
        {
            WTransform localTransform = WTransform::Make(WVec3(1, 2, 3));
            WPrefabs::SpawnPrefab("{ a3ce5d3d-be5e-4bda-8820-b1ce3b3d33fd }", WTransform::MakeGlobalTransform(GetOwner().GetGlobalTransform(), localTransform));

            localTransform.m_vPosition = WVec3(2, 3, 4);
            WPrefabs::SpawnPrefabAsChild("{ 42e938fb-5523-4606-8e64-6fee83dd0c7b }", GetOwner(), localTransform);
            
            m_Phase = Phase::Done;
        }

        return m_Phase != Phase::Done;
    }
}
