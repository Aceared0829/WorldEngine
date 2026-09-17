#include "GameDecls.as"

class UnlockWeapon : WAngelScriptClass
{
    int weaponType = 0;

    void OnMsgTriggerTriggered(WMsgTriggerTriggered@ msg)
    {
        if (msg.TriggerState == WTriggerState::Activated && msg.Message == "Pickup")
        {
            MsgUnlockWeapon hm;
            hm.weaponType = WeaponType(weaponType);

            GetWorld().SendMessageRecursive(msg.GameObject, hm);

            if (!hm.return_consumed)
                return;

            WFmodEventComponent@ sound;
            if (GetOwner().TryGetComponentOfBaseType(@sound))
                sound.StartOneShot();

            // delete yourself
            WMsgDeleteGameObject del;
            GetOwner().PostMessage(del, WTime::Seconds(0.1));
        }
    }
}

