
W_ALWAYS_INLINE WWorld* WWorldModule::GetWorld()
{
  return m_pWorld;
}

W_ALWAYS_INLINE const WWorld* WWorldModule::GetWorld() const
{
  return m_pWorld;
}

//////////////////////////////////////////////////////////////////////////

template <typename ModuleType, typename RTTIType>
WWorldModuleTypeId WWorldModuleFactory::RegisterWorldModule()
{
  struct Helper
  {
    static WWorldModule* Create(WAllocator* pAllocator, WWorld* pWorld) { return W_NEW(pAllocator, ModuleType, pWorld); }
  };

  const WRTTI* pRtti = WGetStaticRTTI<RTTIType>();
  return RegisterWorldModule(pRtti, &Helper::Create);
}
