#include "../../Scripts/GameDecls.as"

class ConsumablePickup : WAngelScriptClass
{
    int ObjectType = 0;
    int Amount = 0;

    void OnMsgTriggerTriggered(WMsgTriggerTriggered@ msg)
    {
        if (msg.TriggerState == WTriggerState::Activated && msg.Message == "Pickup")
        {
            MsgAddConsumable hm;
            hm.consumableType = ConsumableType(ObjectType);
            hm.amount = Amount;

            GetWorld().SendMessageRecursive(msg.GameObject, hm);

            if (hm.return_consumed == false)
                return;

            WFmodEventComponent@ sound;
            if (GetOwner().TryGetComponentOfBaseType(@sound))
            {
                sound.StartOneShot();
            }

            WMsgDeleteGameObject del;
            GetOwner().PostMessage(del, WTime::Seconds(0.1));
        }
    }
}

