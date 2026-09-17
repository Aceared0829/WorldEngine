class ScriptObject :  WAngelScriptClass
{
    WHashedString ButtonName;

    void OnMsgGenericEvent(WMsgGenericEvent@ msg)
    {
        if (msg.Message != "Use")
            return;
         
        WGameObject@ button = GetOwner().FindChildByName("Button");

        WTransformComponent@ slider;
        if (!button.TryGetComponentOfBaseType(@slider))
            return;

        if (slider.Running)
            return;

        slider.SetDirectionForwards(true);
        slider.Running = true;

        WMsgGenericEvent newMsg;
        newMsg.Message = ButtonName;

        GetOwnerComponent().BroadcastEventMsg(newMsg);
    }
}

