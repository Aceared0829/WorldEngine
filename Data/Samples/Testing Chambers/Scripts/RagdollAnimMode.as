class ScriptObject : WAngelScriptClass
{
    void OnMsgGenericEvent(WMsgGenericEvent@ msg)
    {
        if (msg.Message == "AnimMode-Powered")
        {
            WJoltRagdollComponent@ ragdoll;
            if (GetOwner().TryGetComponentOfBaseType(@ragdoll))
            {
                ragdoll.AnimMode = WJoltRagdollAnimMode::Powered;
                ragdoll.FadeJointMotorStrength(0.0f, WTime::MakeFromMilliseconds(1000));
            }
        }
        else if (msg.Message == "AnimMode-Limp")
        {
            WJoltRagdollComponent@ ragdoll;
            if (GetOwner().TryGetComponentOfBaseType(@ragdoll))
            {
                // ragdoll.AnimMode = WJoltRagdollAnimMode::Limp;
                // ragdoll.FadeJointMotorStrength(0.0f, WTime::MakeFromMilliseconds(500));
            }
        }
    }

    void OnSimulationStarted()
    {
            // WJoltRagdollComponent@ ragdoll;
            // if (GetOwner().TryGetComponentOfBaseType(@ragdoll))
            // {
            //     ragdoll.SetJointMotorStrength(100);
            //     ragdoll.AnimMode = WJoltRagdollAnimMode::Powered;
            //     ragdoll.FadeJointMotorStrength(2.0f, WTime::MakeFromMilliseconds(5000));
            // }
    }
}
