class TargetSphere : WAngelScriptClass
{
    private int curDamage = 0;

    void OnSimulationStarted()
    {
        SetUpdateInterval(WTime::Milliseconds(100));
    }

    void OnMsgDamage(WMsgDamage@ msg)
    {
        curDamage += int(msg.Damage);
    }

    void OnMsgInputActionTriggered(WMsgInputActionTriggered@ msg)
    {
        if (msg.TriggerState == WTriggerState::Activated)
        {
            if (msg.InputAction == "Heal")
            {
                curDamage = 0;
            }
        }
    }

    void Update()
    {
        curDamage = WMath::Clamp(curDamage - 1, 0, 1000.0f);
        float dmg = curDamage / 100.0f;

        WMsgSetColor msgCol;
        msgCol.Color = WColor::MakeRGBA(dmg, dmg * 0.05, dmg * 0.05, 1.0);

        GetOwner().SendMessageRecursive(msgCol);

        WParticleComponent@ fireFX;
        if (GetOwner().TryGetComponentOfBaseType(@fireFX))
        {
            if (dmg > 1.0)
            {
                if (!fireFX.IsEffectActive())
                {
                    fireFX.StartEffect();
                }
            }
            else if (dmg < 0.8)
            {
                fireFX.StopEffect();
            }
        }
    }
}

