#include "AiShipBase.as"

enum PreyState
{
    Wandering, // no threat nearby
    Fleeing,   // hunter detected, actively evading
    Cornered,  // hunter got within panic range, emergency escape
}

class Prey : AiShipBase
{
    WString HunterMarker = "Hunter";

    private PreyState m_State = PreyState::Wandering;
    private PreyState m_PrevState = PreyState::Wandering;

    private float m_fAwarenessRange = 30.0f;
    private float m_fFleeDistance = 100.0f;   // how far to run when fleeing
    private float m_fPanicRange = 8.0f;      // emergency flee if hunter this close

    void OnSimulationStarted()
    {
        m_vHomePos = GetOwner().GetGlobalPosition();
        m_fWanderRadius = 200.0f;
        m_fLife = 30.0f;
        SetUpdateInterval(WTime::Milliseconds(80));
    }

    void Update(WTime deltaTime)
    {
        WAiVoxelNavigationComponent@ navComp;
        if (!GetOwner().TryGetComponentOfBaseType(@navComp))
            return;

        WVec3 vOwnPos    = GetOwner().GetGlobalPosition();
        WVec3 vStatusPos = vOwnPos + WVec3(0, 0, 2.2f);

        if (TryRecoverFromFailedState(navComp, vOwnPos))
            return;

        // Detect nearest hunter
        WGameObject@ hunterObj = WSpatial::FindClosestObjectInSphere(HunterMarker, vOwnPos, m_fAwarenessRange);

        if (@hunterObj != null)
        {
            WVec3 vHunterPos = hunterObj.GetGlobalPosition();
            float fDistToHunter = vOwnPos.GetDistanceTo(vHunterPos);

            m_State = (fDistToHunter < m_fPanicRange) ? PreyState::Cornered : PreyState::Fleeing;

            bool bNeedsNewDestination = false;

            if (m_PrevState == PreyState::Wandering)
            {
                navComp.CancelNavigation();
                m_bHasDestination = false;
                bNeedsNewDestination = true;
            }
            else if (!navComp.IsNavigating() || navComp.IsApproachingDestination())
            {
                bNeedsNewDestination = true;
            }

            if (bNeedsNewDestination)
            {
                if (fDistToHunter < m_fPanicRange)
                {
                    PickFleeDestinationRandom(navComp, vOwnPos);
                }
                else
                {
                    WVec3 vAwayDir = vOwnPos - vHunterPos;
                    float fLen = vAwayDir.GetLength();

                    if (fLen > 0.01f)
                    {
                        vAwayDir = vAwayDir * (1.0f / fLen); // normalize manually

                        // pull back towards home, stronger the further away we already are
                        WVec3 vHomeDir = m_vHomePos - vOwnPos;
                        float fHomeDist = vHomeDir.GetLength();
                        float fHomeWeight = WMath::Clamp(fHomeDist / m_fWanderRadius, 0.0f, 1.5f);
                        if (fHomeDist > 0.01f)
                            vHomeDir = vHomeDir * (1.0f / fHomeDist);

                        // always add some randomness so multiple prey don't all flee the same way
                        WRandom@ rng = GetWorld().GetRandomNumberGenerator();
                        float lateralBias = rng.FloatMinMax(-0.6f, 0.6f);
                        WVec3 vSide = vAwayDir.GetOrthogonalVector();

                        WVec3 vFleeDir = vAwayDir + vSide * lateralBias + vHomeDir * fHomeWeight;

                        if (vFleeDir.GetLength() > 0.01f)
                            vAwayDir = vFleeDir.GetNormalized();

                        SetNavDestination(navComp, vOwnPos + vAwayDir * m_fFleeDistance);
                    }
                    else
                    {
                        PickFleeDestinationRandom(navComp, vOwnPos);
                    }
                }
            }

            if (ShowDebugInfo)
                WDebug::DrawLine(vOwnPos, vHunterPos, WColor::Yellow, WColor::OrangeRed);
        }
        else
        {
            m_State = PreyState::Wandering;

            if (!navComp.IsNavigating() || navComp.IsApproachingDestination())
            {
                PickWanderDestination(navComp);
            }
        }

        m_PrevState = m_State;

        if (ShowDebugInfo)
        {
            DrawDebugState(vStatusPos, vOwnPos);

            if (m_bHasDestination)
            {
                WDebug::DrawLine(vOwnPos, m_vDestination, WColor::Cyan, WColor::Cyan);
            }
        }
    }

    void PickFleeDestinationRandom(WAiVoxelNavigationComponent@ navComp, WVec3 vOwnPos)
    {
        WVec3 vPoint;

        if (navComp.FindRandomPointAroundSphere(vOwnPos, m_fFleeDistance, 32, vPoint))
        {
            SetNavDestination(navComp, vPoint);
        }
    }

    void DrawDebugState(WVec3 vTextPos, WVec3 vOwnPos)
    {
        if (m_State == PreyState::Wandering)
        {
            WDebug::Draw3DText("Prey: Wandering", vTextPos, WColor::Green, 24);
        }
        else if (m_State == PreyState::Fleeing)
        {
            WDebug::Draw3DText("Prey: FLEEING!", vTextPos, WColor::Yellow, 28);
        }
        else
        {
            WDebug::Draw3DText("Prey: Cornered!", vTextPos, WColor::Orange, 28);
        }

        // Visualize awareness sphere
        WDebug::DrawLineSphere(vOwnPos, m_fAwarenessRange, WColor(0, 1, 0, 0.25f));
    }
}
