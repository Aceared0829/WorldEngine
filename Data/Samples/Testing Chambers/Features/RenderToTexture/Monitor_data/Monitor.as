shared class MsgSwitchMonitor : WAngelScriptMessage
{
    WString renderTarget;
    WString screenMaterial;
}

class Monitor : WAngelScriptClass
{
    void OnMsgSwitchMonitor(MsgSwitchMonitor@ msg)
    {
        auto display = GetOwner().FindChildByName("Display");

        WMsgSetMeshMaterial mat;
        mat.MaterialSlot = 0;
        mat.Material = msg.screenMaterial;

        display.SendMessage(mat);

        WRenderTargetActivatorComponent@ activator;
        if (display.TryGetComponentOfBaseType(@activator))
        {
            activator.RenderTarget = msg.renderTarget;
        }
    }
}

