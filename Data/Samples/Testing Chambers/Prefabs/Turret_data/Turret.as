class ScriptObject : WAngelScriptClass
{
    float Health = 50;
    private uint8 CollisionLayer = 0;

    WGameObjectHandle target;
    WComponentHandle gunSpawn;
    WComponentHandle gunSound;

    void OnMsgDamage(WMsgDamage@ msg)
    {
        if (Health <= 0)
            return;

        Health -= msg.Damage;

        if (Health > 0)
            return;

        SetUpdateInterval(WTime::Seconds(60)); // basically deactivate future updates

        auto expObj = GetOwner().FindChildByName("Explosion", true);
        if (@expObj == null)
            return;

        WSpawnComponent@ spawnComp;
        if (expObj.TryGetComponentOfBaseType(@spawnComp))
        {
            spawnComp.TriggerManualSpawn(true, WVec3::MakeZero());
        }
    }

    void OnSimulationStarted()
    {
        SetUpdateInterval(WTime::Milliseconds(500));

        CollisionLayer = WPhysics::GetCollisionLayerByName("Visibility Raycast");

        auto gunObj = GetOwner().FindChildByName("Gun", true);

        WSpawnComponent@ gunSpawnComp;
        if (gunObj.TryGetComponentOfBaseType(@gunSpawnComp))
            gunSpawn = gunSpawnComp.GetHandle();

        WFmodEventComponent@ gunSoundComp;
        if (gunObj.TryGetComponentOfBaseType(@gunSoundComp))
            gunSound = gunSoundComp.GetHandle();
    }

    bool FoundObjectCallback(WGameObject@ go)
    {
        target = go.GetHandle();
        return false;
    }

    void Update(WTime deltaTime)
    {
        if (Health <= 0)
            return;

        if (gunSpawn.IsInvalidated())
            return;

        WGameObject@ owner = GetOwner();

        target.Invalidate();
        WSpatial::FindObjectsInSphere("Player", owner.GetGlobalPosition(), 15, ReportObjectCB(FoundObjectCallback));

        WGameObject@ targetObj;
        if (!GetWorld().TryGetObject(target, @targetObj))
        {
            SetUpdateInterval(WTime::Milliseconds(500));
            return;
        }

        WVec3 dirToTarget = targetObj.GetGlobalPosition() - owner.GetGlobalPosition();

        const float distance = dirToTarget.GetLengthAndNormalize();

        WVec3 vHitPosition;
        WVec3 vHitNormal;
        WGameObjectHandle HitObject;

        if (WPhysics::Raycast(vHitPosition, vHitNormal, HitObject, owner.GetGlobalPosition(), dirToTarget * distance, CollisionLayer, WPhysicsShapeType::Static))
        {
            // obstacle in the way
            return;
        }

        SetUpdateInterval(WTime::Milliseconds(50));

        WQuat targetRotation = WQuat::MakeShortestRotation(WVec3::MakeAxisX(), dirToTarget);

        WQuat newRotation = WQuat::MakeSlerp(owner.GetGlobalRotation(), targetRotation, 0.1);

        owner.SetGlobalRotation(newRotation);

        dirToTarget.Normalize();

        if (dirToTarget.Dot(owner.GetGlobalDirForwards()) > WMath::Cos(WAngle::MakeFromDegree(15)))
        {
            WSpawnComponent@ gunSpawnComp;
            if (GetWorld().TryGetComponent(gunSpawn, @gunSpawnComp))
            {
                auto spawned = gunSpawnComp.TriggerManualSpawn(false, WVec3::MakeZero());;
                if (spawned)
                {
                    WFmodEventComponent@ gunSoundComp;
                    if (GetWorld().TryGetComponent(gunSound, @gunSoundComp))
                    {
                        gunSoundComp.StartOneShot();
                    }
                }
            }
        }
    }
}

