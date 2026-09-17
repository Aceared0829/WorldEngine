class ScriptObject :  WAngelScriptClass
{
    private bool ragdollFinished = false;

    void OnMsgDamage(WMsgDamage@ msg)
    {
        if (ragdollFinished)
            return;
            
        WJoltHitboxComponent@ col;
        if (GetOwner().TryGetComponentOfBaseType(@col))
        {
            // if present, deactivate the bone collider component, it isn't needed anymore
            col.Active = false;
        }
        
        WJoltDynamicActorComponent@ da;
        if (GetOwner().TryGetComponentOfBaseType(@da))
        {
            // if present, deactivate the dynamic actor component, it isn't needed anymore
            da.Active = false;
        }            
        
        WJoltRagdollComponent@ rdc;
        if (GetOwner().TryGetComponentOfBaseType(@rdc))
        {
            if (rdc.IsActiveAndSimulating())
            {
                ragdollFinished = true;
                return;
            }

            rdc.StartMode = WJoltRagdollStartMode::WithCurrentMeshPose;
            rdc.Active = true;

            // we want the ragdoll to get a kick, so send an impulse message
            WMsgPhysicsAddImpulse imp;
            imp.Impulse = msg.ImpactDirection;
            imp.Impulse *= WMath::Min(msg.Damage, 5) * 10;
            imp.GlobalPosition = msg.GlobalPosition;
            rdc.SendMessage(imp);
        }
    }
}

