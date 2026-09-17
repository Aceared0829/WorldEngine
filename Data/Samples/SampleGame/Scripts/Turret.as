class Turret : WAngelScriptClass
{
    float Range = 3;

    private array<WGameObjectHandle> allTargets;
    private WTime lastDamageTime;

    void OnSimulationStarted()
    {
        // update this component every frame
        SetUpdateInterval(WTime::MakeZero());
        lastDamageTime = GetWorld().GetClock().GetAccumulatedTime();
    }

    bool FoundTargetCallback(WGameObject@ go)
    {
        allTargets.PushBack(go.GetHandle());
        return true;
    }

    void Update()
    {
        auto owner = GetOwner();

        // find all objects with the 'TurretTarget' marker that are close by
        allTargets.Clear();
        WSpatial::FindObjectsInSphere("TurretTarget", owner.GetGlobalPosition(), Range, ReportObjectCB(FoundTargetCallback));

        DrawLinesToTargets();

        const WTime gameTime = GetWorld().GetClock().GetAccumulatedTime();

        if (gameTime - lastDamageTime > WTime::Milliseconds(40))
        {
            lastDamageTime = gameTime;
            DamageAllTargets(4);
        }
    }

    void DrawLinesToTargets()
    {
        WVec3 startPos = GetOwner().GetGlobalPosition();

        for (uint i = 0; i < allTargets.GetCount(); ++i)
        {
            WGameObject@ obj;
            if (GetWorld().TryGetObject(allTargets[i], obj))
            {
                WVec3 endPos = obj.GetGlobalPosition();
                WDebug::DrawLine(startPos, endPos, WColor::OrangeRed, WColor::OrangeRed);
            }
        }

    }

    void DamageAllTargets(float damage)
    {
        WMsgDamage dmgMsg;
        dmgMsg.Damage = damage;

        for (uint i = 0; i < allTargets.GetCount(); ++i)
        {
            GetWorld().SendMessage(allTargets[i], dmgMsg);
        }
    }
}

