#include "../Weapon.as"
#include "../../../Scripts/GameDecls.as"

class Shotgun : WeaponBaseClass
{
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

        msg.weaponInfo.iAmmoInClip -= 1;

        WRandom@ rng = GetWorld().GetRandomNumberGenerator();
        
        for (int i = 0; i < 16; ++i) 
        {
            spawn.TriggerManualSpawn(true, WVec3(rng.DoubleMinMax(-0.05, 0.05), 0, 0));
        }

        PlayShootSound();
    }
}