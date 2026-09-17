
class ScriptObject : WAngelScriptClass
{
    float Health = 10;

    void OnMsgDamage(WMsgDamage@ msg)
    {
        if (Health <= 0) 
            return;

        Health -= msg.Damage;

        if (Health > 0)
            return;

        auto spawnNode = GetOwner().FindChildByName("OnBreakSpawn");
        if (@spawnNode != null)
        {
            WSpawnComponent@ spawnComp;
            if (spawnNode.TryGetComponentOfBaseType(@spawnComp))
            {
                auto offset = WVec3::MakeRandomPointInSphere(GetWorld().GetRandomNumberGenerator());
                offset *= 0.3;
                spawnComp.TriggerManualSpawn(true, offset);
            }
        }

        GetWorld().DeleteObjectDelayed(GetOwner().GetHandle());
    }
}

