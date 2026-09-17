class ScriptObject :  WAngelScriptClass
{
    private float distance = 0;

    void OnMsgGenericEvent(WMsgGenericEvent@ msg)
    {
        if (msg.Message == "RaycastChanged")
        {
            WGameObject@ beamObj = GetOwner().FindChildByName("Beam");

            WRaycastComponent@ rayComp;
            if (beamObj.TryGetComponentOfBaseType(@rayComp))
            {
                const float newDist = rayComp.GetCurrentDistance();
                if (newDist < distance - 0.01)
                {
                    // allow some slack
                    Explode();
                }

                distance = newDist;
            }
        }
    }

    void Explode()
    {
        WGameObject@ exp = GetOwner().FindChildByName("Explosion");

        if (@exp != null)
        {
            WSpawnComponent@ spawnExpl;
            if (exp.TryGetComponentOfBaseType(@spawnExpl))
            {
                spawnExpl.TriggerManualSpawn(true, WVec3::MakeZero());
            }
        }

        GetWorld().DeleteObjectDelayed(GetOwner().GetHandle());
    }

    void OnMsgDamage(WMsgDamage@ msg)
    {
        // explode on any damage
        Explode();
    }
}

