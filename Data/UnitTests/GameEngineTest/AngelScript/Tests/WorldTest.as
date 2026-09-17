#include "TestFramework.as"

enum Phase
{
    CreateObj,
    CheckExists,
    CheckDeleted,
    Done,
}

class ScriptObject : WAngelScriptTestClass
{
    private Phase m_Phase = Phase::CreateObj;
    private WGameObjectHandle m_hCreated;
    private WTime m_LastUpdate;

    ScriptObject()
    {
        super("WorldTest");
    }
    
    bool ExecuteTests()
    {
        WGameObject@ owner = GetOwner();
        WWorld@ world = GetWorld();

        WGameObject@ allTestsObj;
        W_TEST_BOOL(world.TryGetObjectWithGlobalKey("Tests", @allTestsObj));
        W_TEST_STRING(allTestsObj.GetName(), "All Tests");

        WClock@ clock = world.GetClock();
        WTime tNow = clock.GetAccumulatedTime();
        W_TEST_BOOL(tNow > m_LastUpdate);
        m_LastUpdate = tNow;

        if (m_Phase == Phase::CreateObj)
        {
            WGameObjectDesc gd;
            gd.m_LocalPosition.Set(1, 2, 3);
            gd.m_sName = "TestObj";
            gd.m_hParent = owner.GetHandle();
            
            WGameObject@ go;
            m_hCreated = GetWorld().CreateObject(gd, go);
            W_TEST_BOOL(world.IsValidObject(m_hCreated));

            m_Phase = Phase::CheckExists;
            return true;
        }

        if (m_Phase == Phase::CheckExists)
        {
            WGameObject@ obj1 = owner.FindChildByName("TestObj");
            W_TEST_BOOL(@obj1 != null);
            W_TEST_BOOL(world.IsValidObject(m_hCreated));
            W_TEST_BOOL(obj1.GetHandle() == m_hCreated);

            WGameObject@ obj2;
            W_TEST_BOOL(world.TryGetObject(m_hCreated, @obj2));

            W_TEST_BOOL(@obj1 == @obj2);

            world.DeleteObjectDelayed(obj1.GetHandle());

            W_TEST_STRING(obj2.GetName(), "TestObj");

            m_Phase = Phase::CheckDeleted;
            return true;            
        }

        if (m_Phase == Phase::CheckDeleted)
        {
            WGameObject@ obj = owner.FindChildByName("TestObj");
            W_TEST_BOOL(@obj == null);

            W_TEST_BOOL(!world.IsValidObject(m_hCreated));

            WGameObject@ obj2;
            W_TEST_BOOL(!world.TryGetObject(m_hCreated, @obj2));

            m_Phase = Phase::Done;
            return false;
        }

        return false;
    }
}

