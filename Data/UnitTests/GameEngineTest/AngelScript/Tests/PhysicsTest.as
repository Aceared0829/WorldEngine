#include "TestFramework.as"

enum Phase
{
    Raycast,
    Spatial,
    Done
}

class ScriptObject : WAngelScriptTestClass
{
    private Phase m_Phase = Phase::Raycast;
    private int m_iFoundInside = 0;
    private int m_iFoundOther = 0;
    private array<WGameObjectHandle> m_Found;

    ScriptObject()
    {
        super("PhysicsTest");
    }

    bool FoundObject(WGameObject@ obj)
    {
        if (obj.HasName("Inside"))
            ++m_iFoundInside;
        else
            ++m_iFoundOther;

        m_Found.PushBack(obj.GetHandle());

        return true;
    }

    bool ExecuteTests()
    {
        if (m_Phase == Phase::Raycast)
        {
            WVec3 vHitPosition, vHitNormal;
            WGameObjectHandle hHitObject;

            W_TEST_BOOL(!WPhysics::Raycast(vHitPosition, vHitNormal, hHitObject, WVec3(1, 2, 3), WVec3(0, 0, -1) * 10.0f, 0, WPhysicsShapeType::Dynamic));

            W_TEST_BOOL(WPhysics::Raycast(vHitPosition, vHitNormal, hHitObject, WVec3(1, 2, 3), WVec3(0, 0, -1) * 10.0f, 0, WPhysicsShapeType::Static));

            W_TEST_VEC3(vHitPosition, WVec3(1, 2, 0));
            W_TEST_VEC3(vHitNormal, WVec3(0, 0, 1));

            m_Phase = Phase::Spatial;
        }
        else if (m_Phase == Phase::Spatial)
        {
            WSpatial::FindObjectsInSphere("Marker", WVec3(5.5f, 5.5f, 5.0f), 1.0f, ReportObjectCB(FoundObject));

            W_TEST_INT(m_iFoundInside, 4);
            W_TEST_INT(m_iFoundOther, 0);
            W_TEST_INT(m_Found.GetCount(), 4);

            m_Phase = Phase::Done;
        }

        return m_Phase != Phase::Done;
    }
}
