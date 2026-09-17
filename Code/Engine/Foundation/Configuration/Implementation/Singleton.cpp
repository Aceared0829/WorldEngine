#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Singleton.h>

WMap<size_t, WSingletonRegistry::SingletonEntry> WSingletonRegistry::s_Singletons;

const WMap<size_t, WSingletonRegistry::SingletonEntry>& WSingletonRegistry::GetAllRegisteredSingletons()
{
  return s_Singletons;
}
