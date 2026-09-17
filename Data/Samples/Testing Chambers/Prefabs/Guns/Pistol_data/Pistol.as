#include "../Weapon.as"
#include "../../../Scripts/GameDecls.as"

class Pistol : WeaponBaseClass
{
    private WTime nextAmmoPlus1Time;

    void OnSimulationStarted()
    {
        WeaponBaseClass::OnSimulationStarted();
        
        singleShotPerTrigger = true;
    }

    void FireWeapon(MsgWeaponInteraction@ msg) override
    {
        WSpawnComponent@ spawn;
        if (!GetOwner().FindChildByName("Spawn").TryGetComponentOfBaseType(@spawn))
            return;

        if (!spawn.CanTriggerManualSpawn())
            return;

        WClock@ clk = GetWorld().GetClock();
        nextAmmoPlus1Time = clk.GetAccumulatedTime() + WTime::Seconds(0.75);
        msg.weaponInfo.iAmmoInClip -= 1;

        spawn.TriggerManualSpawn(false, WVec3::MakeZero());

        PlayShootSound();
    }

    void UpdateWeapon(MsgWeaponInteraction@ msg) override
    {
        WClock@ clk = GetWorld().GetClock();
        if (nextAmmoPlus1Time <= clk.GetAccumulatedTime())
        {
            nextAmmoPlus1Time = clk.GetAccumulatedTime() + WTime::Seconds(0.75);

            msg.weaponInfo.iAmmoInClip = WMath::Min(msg.weaponInfo.iAmmoInClip + 1, msg.weaponInfo.iClipSize);
        }
    }
}