#include "TestFramework.as"

enum Phase
{
    Init,
    Update1,
    Update2,
    NoUpdate,

    Done
}

class ScriptObject : WAngelScriptTestClass
{
    int NumUpdates = 0;
    private int m_iNumUpdatesDone = 0;

    private int m_iActivated = 0;
    private int m_iDeactivated = 0;
    private int m_iSimStarted = 0;
    private int m_iUpdated = 0;
    private Phase m_Phase = Phase::Init;

    ScriptObject()
    {
        super("EntryPointsTest");
    }

    void OnActivated()
    {
        m_iActivated++;

        // make sure there is no immediate update
        SetUpdateInterval(WTime::MakeFromHours(1));
    }

    void OnDeactivated()
    {
        m_iDeactivated++;
    }

    void OnSimulationStarted()
    {
        m_iSimStarted++;
    }

    void Update()
    {
        m_iUpdated++;
    }

    bool ExecuteTests()
    {
        if (m_Phase == Phase::Init)
        {
            W_TEST_BOOL(NumUpdates > 0);
            W_TEST_INT(m_iActivated, 1);
            W_TEST_INT(m_iDeactivated, 0);
            W_TEST_INT(m_iSimStarted, 1);
            W_TEST_INT(m_iUpdated, 0);

            SetUpdateInterval(WTime::MakeZero()); // enable updates
            m_Phase = Phase::Update1;
        }
        else if (m_Phase == Phase::Update1)
        {
            ++m_iNumUpdatesDone;

            W_TEST_INT(m_iActivated, 1);
            W_TEST_INT(m_iDeactivated, 0);
            W_TEST_INT(m_iSimStarted, 1);
            W_TEST_INT(m_iUpdated, m_iNumUpdatesDone);

            if (m_iNumUpdatesDone == NumUpdates)
                m_Phase = Phase::Update2;
            else
                m_Phase = Phase::Update1; // repeat this
        }
        else if (m_Phase == Phase::Update2)
        {
            ++m_iNumUpdatesDone;

            W_TEST_INT(m_iActivated, 1);
            W_TEST_INT(m_iDeactivated, 0);
            W_TEST_INT(m_iSimStarted, 1);
            W_TEST_INT(m_iUpdated, m_iNumUpdatesDone);

            SetUpdateInterval(WTime::MakeFromHours(1)); // disable updates
            m_Phase = Phase::NoUpdate;
        }
        else if (m_Phase == Phase::NoUpdate)
        {
            W_TEST_INT(m_iActivated, 1);
            W_TEST_INT(m_iDeactivated, 0);
            W_TEST_INT(m_iSimStarted, 1);
            W_TEST_INT(m_iUpdated, m_iNumUpdatesDone); // no change

            m_Phase = Phase::Done;
        }

        return m_Phase != Phase::Done;
    }
}