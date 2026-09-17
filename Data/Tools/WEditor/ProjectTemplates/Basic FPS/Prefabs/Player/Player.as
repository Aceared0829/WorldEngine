#include "../../Scripts/Shared.as"

class Player : WAngelScriptClass
{
    int m_iHealth = 100;

    void Update(WTime deltaTime)
    {
        if (m_iHealth <= 0)
        {
            WDebug::DrawInfoText("YOU ARE DEAD", WDebugTextPlacement::TopCenter, "Player", WColor::OrangeRed);
            return;
        }
        
        WStringBuilder text;
        text.SetFormat("Health: {}", m_iHealth);
        WDebug::DrawInfoText(text, WDebugTextPlacement::TopLeft, "Player", WColor::White);

        WInputComponent@ inputComp;
        if (!GetOwner().TryGetComponentOfBaseType(@inputComp))
            return;
        
        UpdateCharacterMovement(inputComp);
        UpdateInteractions(inputComp);
    }

    void UpdateCharacterMovement(WInputComponent@ inputComp)
    {
        WMsgMoveCharacterController moveMsg;

        moveMsg.MoveForwards = inputComp.GetCurrentInputState("move_forwards", false);
        moveMsg.MoveBackwards = inputComp.GetCurrentInputState("move_backwards", false);
        moveMsg.StrafeLeft = inputComp.GetCurrentInputState("move_left", false);
        moveMsg.StrafeRight = inputComp.GetCurrentInputState("move_right", false);
        moveMsg.RotateLeft = inputComp.GetCurrentInputState("turn_left", false);
        moveMsg.RotateRight = inputComp.GetCurrentInputState("turn_right", false);
        moveMsg.Jump = inputComp.GetCurrentInputState("jump", false) > 0.5f;
        moveMsg.Run = inputComp.GetCurrentInputState("run", false) > 0.5f;
        moveMsg.Crouch = inputComp.GetCurrentInputState("crouch", false) > 0.5f;

        GetOwner().SendMessage(moveMsg);

        WGameObject@ headBoneObj = GetOwner().FindChildByName("HeadBone", true);
        if (@headBoneObj != null)
        {
            WHeadBoneComponent@ headBoneComp;
            if (headBoneObj.TryGetComponentOfBaseType(@headBoneComp))
            {
                float turnUp = inputComp.GetCurrentInputState("turn_up", false);
                float turnDown = inputComp.GetCurrentInputState("turn_down", false);

                headBoneComp.ChangeVerticalRotation(turnDown - turnUp);
            }
        }
    }

    WGameObject@ GetCamera()
    {
        return GetOwner().FindChildByName("Camera", true);
    }

    WJoltGrabObjectComponent@ GetGrabber()
    {
        WGameObject@ cameraObj = GetCamera();
        if (@cameraObj != null)
        {
            WJoltGrabObjectComponent@ grabComp;
            if (cameraObj.TryGetComponentOfBaseType(@grabComp))
            {
                return @grabComp;
            }
        }

        return null;
    }

    void UpdateInteractions(WInputComponent@ inputComp)
    {
        if (inputComp.GetCurrentInputState("shoot", true) != 0.0f)
        {
            WJoltGrabObjectComponent@ grabComp = GetGrabber();
            if (@grabComp != null)
            {
                if (grabComp.HasObjectGrabbed())
                {
                    grabComp.ThrowGrabbedObject(WVec3(200, 0, 0));
                    return;
                }
            }

            WGameObject@ gunObj = GetOwner().FindChildByName("Gun", true);
            if (@gunObj != null)
            {
                WSpawnComponent@ gunComp;
                if (gunObj.TryGetComponentOfBaseType(@gunComp))
                {
                    if (gunComp.TriggerManualSpawn(false, WVec3::MakeZero()))
                    {
                        // gun fired once
                        // repeat for "shotgun effect"
                        for (int i = 0; i < 20; ++i)
                        {
                            gunComp.TriggerManualSpawn(true, WVec3::MakeZero());
                        }

                        // asset GUID of the shotgun sound
                        WSound::PlaySound("{ 9db69eaf-518a-4832-97b0-dd8233df1b74 }", gunObj.GetGlobalPosition(), WQuat::MakeIdentity(), 1, 1, true);
                    }
                }
            }
        }

        if (inputComp.GetCurrentInputState("use", true) != 0.0f)
        {
            WJoltGrabObjectComponent@ grabComp = GetGrabber();
            if (@grabComp != null)
            {
                if (grabComp.HasObjectGrabbed())
                {
                    grabComp.DropGrabbedObject();
                    return;
                }
                else
                {
                    if (grabComp.GrabNearbyObject())
                        return;
                }
            }

            // otherwise try to 'use' the closest object

            WGameObject@ cameraObj = GetCamera();
            if (@cameraObj != null)
            {            
                WVec3 vHitPosition;
                WVec3 vHitNormal;
                WGameObjectHandle hHitObject;

                if (WPhysics::Raycast(vHitPosition, vHitNormal, hHitObject, cameraObj.GetGlobalPosition(), cameraObj.GetGlobalDirForwards() * 1.5f, WPhysics::GetCollisionLayerByName("Interaction Raycast"), WPhysicsShapeType(WPhysicsShapeType::Static | WPhysicsShapeType::Dynamic)))
                {
                    WMsgGenericEvent msgUse;
                    msgUse.Message = "use";

                    WGameObject@ hitObj;
                    if (GetWorld().TryGetObject(hHitObject, @hitObj))
                    {
                        hitObj.SendEventMessage(msgUse, GetOwnerComponent());
                    }
                }
            }
        }
    }

    void OnMsgDamage(WMsgDamage@ msg)
    {
        m_iHealth -= int(msg.Damage);
    }

    void OnMsgPickup(MsgPickup@ msg)
    {
        if (msg.m_iType == 0) // health
        {
            if (m_iHealth < 100)
            {
                m_iHealth = WMath::Min(m_iHealth + msg.m_iAmount, 100);
                msg.m_bConsumed = true;
            }
        }
    }
}