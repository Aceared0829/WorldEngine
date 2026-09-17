#include "TestFramework.as"

enum Phase
{
    SendAS,
    SendCPP,
    PostAS,
    WaitPostAS,
    PostCPP,
    WaitPostCPP,
    CheckInvalid,
    SendEventCPP,
    PostEventCPP,
    WaitPostEventCPP,

    Done
}

class AsTestMsg : WAngelScriptMessage
{
    int iFromParticipant;
    ScriptObject@ FromComponent = null;
    WString sMsg;
    int iHandledBy0 = 0;
    int iHandledBy1 = 0;
    int iHandledBy2 = 0;
}

class AsTestMsg2 : WAngelScriptMessage
{
}

class ScriptObject : WAngelScriptTestClass
{
    int Participant = -1;

    private Phase m_Phase = Phase::SendAS;
    private int m_iResponses = 0;
    private int m_iInternalTriggers = 0;
    private WTime m_waitStartTime;
    private bool m_bReceivedDamage = false;
    private WString m_sTriggerMsg;

    ScriptObject()
    {
        super("MessagingTest");
    }

    bool ExecuteTests()
    {
        W_TEST_BOOL(Participant != -1);

        if (m_Phase == Phase::SendAS)
        {
            // recursive
            {
                AsTestMsg msg;
                msg.iFromParticipant = Participant;
                @msg.FromComponent = @this;
    
                m_iResponses = 0;
                W_TEST_BOOL(GetOwner().GetParent().SendMessageRecursive(msg));

                W_TEST_INT(msg.iHandledBy0, 1);
                W_TEST_INT(msg.iHandledBy1, 1);
                W_TEST_INT(msg.iHandledBy2, 1);
                W_TEST_INT(m_iResponses, 3);
            }

            // non-recursive
            {
                AsTestMsg msg;
                msg.iFromParticipant = Participant;
                @msg.FromComponent = @this;
    
                m_iResponses = 0;
                W_TEST_BOOL(GetOwner().SendMessage(msg));
    
                W_TEST_INT(msg.iHandledBy0, (Participant == 0) ? 1 : 0);
                W_TEST_INT(msg.iHandledBy1, (Participant == 1) ? 1 : 0);
                W_TEST_INT(msg.iHandledBy2, (Participant == 2) ? 1 : 0);
                W_TEST_INT(m_iResponses, 1);
            }

            // directly to component
            {
                AsTestMsg msg;
                msg.iFromParticipant = Participant;
                @msg.FromComponent = @this;
    
                m_iResponses = 0;
                // W_TEST_BOOL(GetOwnerComponent().SendMessage(msg)); // not yet implemented
                // W_TEST_BOOL(GetWorld().SendMessage(GetOwnerComponent().GetHandle(), msg)); // not yet implemented
    
                // W_TEST_INT(msg.iHandledBy0, (Participant == 0) ? 1 : 0);
                // W_TEST_INT(msg.iHandledBy1, (Participant == 1) ? 1 : 0);
                // W_TEST_INT(msg.iHandledBy2, (Participant == 2) ? 1 : 0);
                // W_TEST_INT(m_iResponses, 1);
            }

            m_Phase = Phase::SendCPP;
        }
        else if (m_Phase == Phase::SendCPP)
        {
            // recursive
            {
                WMsgComponentInternalTrigger msg;
                msg.Message = "Hello";
                msg.Payload = 0;
        
                m_iInternalTriggers = 0;
                W_TEST_BOOL(GetOwner().GetParent().SendMessageRecursive(msg));

                W_TEST_INT(m_iInternalTriggers, 1); // this component gets it once
                W_TEST_INT(msg.Payload, 3);
            }

            // non-recursive
            {
                WMsgComponentInternalTrigger msg;
                msg.Message = "Hello";
                msg.Payload = 0;
        
                m_iInternalTriggers = 0;
                W_TEST_BOOL(GetOwner().SendMessage(msg));
    
                W_TEST_INT(m_iInternalTriggers, 1);
                W_TEST_INT(msg.Payload, 1);
            }

            // directly to component
            {
                WMsgComponentInternalTrigger msg;
                msg.Message = "Hello";
                msg.Payload = 0;
    
                m_iInternalTriggers = 0;
                W_TEST_BOOL(GetOwnerComponent().SendMessage(msg));
                W_TEST_INT(m_iInternalTriggers, 1);
                W_TEST_INT(msg.Payload, 1);

                GetWorld().SendMessage(GetOwnerComponent().GetHandle(), msg);
                W_TEST_INT(m_iInternalTriggers, 2);
                W_TEST_INT(msg.Payload, 2);
            }

            m_Phase = Phase::PostAS;
        }
        else if (m_Phase == Phase::PostAS)
        {
            // recursive
            {
                AsTestMsg msg;
                msg.iFromParticipant = Participant;
                @msg.FromComponent = @this;
    
                m_iResponses = 0;
                GetOwner().GetParent().PostMessageRecursive(msg, WTime::Milliseconds(200));

                W_TEST_INT(msg.iHandledBy0, 0);
                W_TEST_INT(msg.iHandledBy1, 0);
                W_TEST_INT(msg.iHandledBy2, 0);                
            }

            // non-recursive
            {
                AsTestMsg msg;
                msg.iFromParticipant = Participant;
                @msg.FromComponent = @this;
    
                m_iResponses = 0;
                GetOwner().PostMessage(msg, WTime::Milliseconds(200));

                W_TEST_INT(msg.iHandledBy0, 0);
                W_TEST_INT(msg.iHandledBy1, 0);
                W_TEST_INT(msg.iHandledBy2, 0);                
            }

            // directly to component
            {
                AsTestMsg msg;
                msg.iFromParticipant = Participant;
                @msg.FromComponent = @this;
    
                m_iResponses = 0;
                // GetOwnerComponent().PostMessage(msg); // not yet implemented
                // GetWorld().PostMessage(GetOwnerComponent().GetHandle(), msg); // not yet implemented
    
                W_TEST_INT(msg.iHandledBy0, 0);
                W_TEST_INT(msg.iHandledBy1, 0);
                W_TEST_INT(msg.iHandledBy2, 0);
            }            

            W_TEST_INT(m_iResponses, 0);

            m_waitStartTime = GetWorld().GetClock().GetAccumulatedTime();
            m_Phase = Phase::WaitPostAS;
        }
        else if (m_Phase == Phase::WaitPostAS)
        {
            // the test uses fixed time-stepping, so using WTime::Now() here doesn't work
            const WTime tNow = GetWorld().GetClock().GetAccumulatedTime();

            if (tNow - m_waitStartTime < WTime::Milliseconds(200))
            {
                W_TEST_INT(m_iResponses, 0);
            }
            else if (tNow - m_waitStartTime > WTime::Milliseconds(220))
            {
                W_TEST_INT(m_iResponses, 4);

                m_Phase = Phase::PostCPP;
            }
        }
        else if (m_Phase == Phase::PostCPP)
        {
            m_iInternalTriggers = 0;

            // recursive
            {
                WMsgComponentInternalTrigger msg;
                msg.Message = "Hello";
                msg.Payload = 0;
        
                GetOwner().GetParent().PostMessageRecursive(msg, WTime::Milliseconds(200));

                W_TEST_INT(msg.Payload, 0);
            }

            // non-recursive
            {
                WMsgComponentInternalTrigger msg;
                msg.Message = "Hello";
                msg.Payload = 0;
        
                GetOwner().PostMessage(msg, WTime::Milliseconds(200));
    
                W_TEST_INT(msg.Payload, 0);
            }

            // directly to component
            {
                WMsgComponentInternalTrigger msg;
                msg.Message = "Hello";
                msg.Payload = 0;
    
                GetOwnerComponent().PostMessage(msg, WTime::Milliseconds(200));
                GetWorld().PostMessage(GetOwnerComponent().GetHandle(), msg, WTime::Milliseconds(200));

                W_TEST_INT(msg.Payload, 0);
            }

            W_TEST_INT(m_iInternalTriggers, 0);

            m_waitStartTime = GetWorld().GetClock().GetAccumulatedTime();
            m_Phase = Phase::WaitPostCPP;
        }
        else if (m_Phase == Phase::WaitPostCPP)
        {
            // the test uses fixed time-stepping, so using WTime::Now() here doesn't work
            const WTime tNow = GetWorld().GetClock().GetAccumulatedTime();

            if (tNow - m_waitStartTime < WTime::Milliseconds(200))
            {
                W_TEST_INT(m_iInternalTriggers, 0);
            }
            else if (tNow - m_waitStartTime > WTime::Milliseconds(220))
            {
                W_TEST_INT(m_iInternalTriggers, 6);

                m_Phase = Phase::PostCPP;
            }

            m_Phase = Phase::CheckInvalid;
        }
        else if (m_Phase == Phase::CheckInvalid)
        {
            WMsgDamage msg;
            W_TEST_BOOL(GetOwner().GetParent().SendMessageRecursive(msg) == false);
            W_TEST_BOOL(!m_bReceivedDamage);
            
            m_Phase = Phase::SendEventCPP;
        }
        else if (m_Phase == Phase::SendEventCPP)
        {
            m_sTriggerMsg.Clear();
            WGameObject@ child = GetOwner().FindChildByName("Child");

            WMsgTriggerTriggered msg;
            msg.Message = "This just happened.";
            child.SendEventMessage(msg, GetOwnerComponent());
            W_TEST_STRING(m_sTriggerMsg, "This just happened.");

            m_Phase = Phase::PostEventCPP;
        }
        else if (m_Phase == Phase::PostEventCPP)
        {
            m_sTriggerMsg.Clear();
            WGameObject@ child = GetOwner().FindChildByName("Child");

            WMsgTriggerTriggered msg;
            msg.Message = "This just happened.";
            child.PostEventMessage(msg, GetOwnerComponent(), WTime::Milliseconds(200));
            W_TEST_STRING(m_sTriggerMsg, "");

            m_waitStartTime = GetWorld().GetClock().GetAccumulatedTime();
            m_Phase = Phase::WaitPostEventCPP;
        }
        else if (m_Phase == Phase::WaitPostEventCPP)
        {
            // the test uses fixed time-stepping, so using WTime::Now() here doesn't work
            const WTime tNow = GetWorld().GetClock().GetAccumulatedTime();

            if (tNow - m_waitStartTime < WTime::Milliseconds(200))
            {
                W_TEST_STRING(m_sTriggerMsg, "");
            }
            else if (tNow - m_waitStartTime > WTime::Milliseconds(220))
            {
                W_TEST_STRING(m_sTriggerMsg, "This just happened.");

                m_Phase = Phase::Done;
            }
        }

        return m_Phase != Phase::Done;
    }

    void OnMsgAsTestMsg(AsTestMsg@ msg)
    {
        switch(Participant)
        {
            case 0:
            ++msg.iHandledBy0;
            break;
            case 1:
            ++msg.iHandledBy1;
            break;
            case 2:
            ++msg.iHandledBy2;
            break;
        }

        AsTestMsg2 response;
        msg.FromComponent.GetOwner().SendMessage(response);
    }

    void OnMsgAsTestMsg2(AsTestMsg2@ msg)
    {
        ++m_iResponses;
    }    
    
    void OnMsgAsTestMsg3(AsTestMsg2@ msg)
    {
        // double message handlers are ignored

        m_iResponses = 333;
    }

    void OnMsgComponentInternalTrigger(WMsgComponentInternalTrigger@ msg)
    {
        if (msg.Message == "Hello")
        {
            ++m_iInternalTriggers;
        }

        msg.Payload = msg.Payload + 1;
    }

    void InvalidMsgHandlerName(WMsgDamage@ msg)
    {
        m_bReceivedDamage = true;
    }

    void OnMsgTriggerTriggered(WMsgTriggerTriggered@ msg)
    {
        m_sTriggerMsg = msg.Message;
    }
}