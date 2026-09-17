class ScriptObject :  WAngelScriptClass
{
    void OnMsgTriggerTriggered(WMsgTriggerTriggered@ msg)
    {
        if (msg.Message == "ActivatePaddleWheel")
        {
            if (msg.TriggerState == WTriggerState::Activated) {

                WGameObject@ spawn;
                if (GetWorld().TryGetObjectWithGlobalKey("PaddleWheelSpawn1", @spawn))
                {
                    spawn.SetActiveFlag(true);
                }

            }
            else if (msg.TriggerState == WTriggerState::Deactivated) {

                WGameObject@ spawn;
                if (GetWorld().TryGetObjectWithGlobalKey("PaddleWheelSpawn1", @spawn))
                {
                    spawn.SetActiveFlag(false);
                }

            }
        }

        if (msg.Message == "ActivateSwing") {

            if (msg.TriggerState == WTriggerState::Activated) 
            {
                WGameObject@ spawn;
                if (GetWorld().TryGetObjectWithGlobalKey("SwingSpawn1", @spawn))
                {
                    spawn.SetActiveFlag(true);
                }

            }
            else if (msg.TriggerState == WTriggerState::Deactivated) 
            {
                WGameObject@ spawn;
                if (GetWorld().TryGetObjectWithGlobalKey("SwingSpawn1", @spawn))
                {
                    spawn.SetActiveFlag(false);
                }
            }
        }
    }
}

