enum State
{
    Idle,
    Active,
    Searching,
}

class NPC : WAngelScriptClass
{
    int Health = 100;

    private State m_State = State::Idle;
    private WVec3 m_vKnownPlayerLoc;
    private WVec3 m_vLastFootstep;

    void OnSimulationStarted()
    {
        SetUpdateInterval(WTime::Milliseconds(400));
        m_vLastFootstep = GetOwner().GetGlobalPosition();
    }

    void Shoot()
    {
        WGameObject@ gunObj = GetOwner().FindChildByName("Gun");

        if (@gunObj == null)
            return;

        WSpawnComponent@ spawnComp;
        if (!gunObj.TryGetComponentOfBaseType(@spawnComp))
            return;

        if (spawnComp.TriggerManualSpawn(false, WVec3::MakeZero()))
        {
            // shot fired

            WSound::PlaySound("{ d4e58c28-450f-449a-a920-7e0459066700 }", gunObj.GetGlobalPosition(), WQuat::MakeIdentity(), 1, 1, true);
        }
    }

    void Update(WTime deltaTime)
    {
        WAiNavigationComponent@ navComp;
        if (!GetOwner().TryGetComponentOfBaseType(@navComp))
            return;

        const float fPlayDistance = 20.0f;

        // we could check this once (with a huge radius) and just store the result
        WGameObject@ playerMarkerObj = WSpatial::FindClosestObjectInSphere("Player", GetOwner().GetGlobalPosition(), fPlayDistance);
        if (@playerMarkerObj == null)
            return;

        WVec3 vHitPosition, vitNormal;
        WGameObjectHandle hHitObject;
        WVec3 vStart = GetOwner().GetGlobalPosition() + WVec3(0, 0, 1.5);
        WVec3 vEnd = playerMarkerObj.GetGlobalPosition();

        if ((vEnd - vStart).GetLengthSquared() < (fPlayDistance * fPlayDistance))
        {
            if (!WPhysics::OverlapTestLine(vStart, vEnd, 0, WPhysicsShapeType(WPhysicsShapeType::Static | WPhysicsShapeType::Dynamic)))
            {
                m_vKnownPlayerLoc = playerMarkerObj.GetGlobalPosition();
                m_State = State::Active;
                SetUpdateInterval(WTime::Milliseconds(40));

                // WDebug::DrawLine(vStart, vEnd, WColor::IndianRed, WColor::IndianRed);

                const float fDist = (vEnd - vStart).GetLength();
                navComp.StopWalking(WMath::Min(1.0f, fDist * 0.3f));
                
                if (navComp.GetState() == WAiNavigationComponentState::Idle)
                {
                    Shoot();
                }

                navComp.TurnTowards(vEnd.GetAsVec2());
            }
            else
            {
                // WDebug::DrawLine(vStart, vEnd, WColor::Yellow, WColor::Yellow);

                if (m_State == State::Active)
                {
                    m_State = State::Searching;
                }
                else
                {
                    switch (navComp.GetState())
                    {
                    case WAiNavigationComponentState::Idle:
                        m_State = State::Idle;
                        break;
                    }
                }
            }
        }

        if (m_State == State::Idle)
        {
            SetUpdateInterval(WTime::Milliseconds(400));
            return;
        }

        if (m_State == State::Searching)
        {
            navComp.SetDestination(m_vKnownPlayerLoc, false);
        }

        if ((GetOwner().GetGlobalPosition() - m_vLastFootstep).GetLengthSquared() > 1.5f)
        {
            // walked more than one meter? spawn a footstep sound
            m_vLastFootstep = GetOwner().GetGlobalPosition();

            WPhysics::RaycastSurfaceInteraction(m_vLastFootstep + WVec3(0, 0, 0.1f), WVec3(0, 0, -0.5f), 0, WPhysicsShapeType::Static, "{ 0d634746-1154-4917-af6a-2d740fc752f5 }", "Footstep");
        }
    }

    void OnMsgDamage(WMsgDamage@ msg)
    {
        if (Health <= 0)
            return;

        Health -= int(msg.Damage);

        if (Health <= 0)
        {
            WTransform pos = GetOwner().GetGlobalTransform();
            pos.m_vPosition += WVec3(0, 0, 0.9f);
            WPrefabs::SpawnPrefab("{ 3d758985-7126-92f0-1cc1-36dcd5419ee1 }", pos);

            WMsgDeleteGameObject del;
            GetOwner().PostMessage(del, WTime::Milliseconds(100));
        }
    }
}