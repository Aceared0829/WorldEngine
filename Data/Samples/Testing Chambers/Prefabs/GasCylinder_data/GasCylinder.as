class ScriptObject :  WAngelScriptClass
{
    private float capHealth = 5;
    private float bodyHealth = 50;
    private uint32 capForce = 0;

    void OnSimulationStarted()
    {
        // no update needed by default
        SetUpdateInterval(WTime::MakeFromSeconds(10));
    }

    void Update(WTime deltaTime)
    {
        if (capHealth <= 0) 
        {
            auto owner = GetOwner();
            auto cap = owner.FindChildByName("Cap");

            WJoltDynamicActorComponent@ actor;
            if (owner.TryGetComponentOfBaseType(@actor))
            {
                WVec3 force = cap.GetGlobalDirUp();

                auto randomDir = WVec3::MakeRandomDirection(GetWorld().GetRandomNumberGenerator());
                randomDir *= 0.4;
    
                force += randomDir;
                force *= -2;

                actor.AddOrUpdateForce(capForce, WTime::Seconds(0.5f), force);
            }
        }
    }

    void OnMsgDamage(WMsgDamage@ msg)
    {
        bodyHealth -= msg.Damage;

        if (bodyHealth <= 0)
        {
            Explode();
            return;
        }

        if (msg.HitObjectName == "Cap")
        {
            if (capHealth > 0) 
            {
                capHealth -= msg.Damage;

                if (capHealth <= 0) 
                {
                    // update every frame, to apply regularly the physics force
                    SetUpdateInterval(WTime::MakeZero());

                    auto leakObj = GetOwner().FindChildByName("LeakEffect");
                    if (@leakObj != null)
                    {
                        WParticleComponent@ leakFX;
                        if (leakObj.TryGetComponentOfBaseType(@leakFX))
                        {
                            leakFX.StartEffect();
                        }
                        else
                        {
                            WLog::Error("Failed to start particle effect!");
                        }

                        WFmodEventComponent@ leakSound;
                        if (leakObj.TryGetComponentOfBaseType(@leakSound))
                        {
                            leakSound.Play();
                        }
                    }

                    // trigger code path below
                    msg.HitObjectName = "Tick";
                }
            }
        }

        if (msg.HitObjectName == "Tick")
        {
            WMsgDamage tickDmg;
            tickDmg.Damage = 1;
            tickDmg.HitObjectName = "Tick";
            GetOwner().PostMessage(tickDmg, WTime::MakeFromMilliseconds(100));
        }
    }

    void Explode()
    {
        auto owner = GetOwner();
        auto exp = owner.FindChildByName("Explosion");

        if (@exp != null)
        {
            WSpawnComponent@ spawnExpl;
            if (exp.TryGetComponentOfBaseType(@spawnExpl))
            {
                spawnExpl.TriggerManualSpawn(false, WVec3::MakeZero());
            }
        }

        GetWorld().DeleteObjectDelayed(GetOwner().GetHandle());
    }
}

