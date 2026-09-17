#pragma once

#include <Foundation/Containers/Set.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>

class WReflectedTypeStorageAccessor;
class WDocumentObject;

/// Manages all WReflectedTypeStorageAccessor instances.
///
/// This class takes care of patching all WReflectedTypeStorageAccessor instances when their
/// WRTTI is modified. It also provides the mapping from property name to the data
/// storage index of the corresponding WVariant in the WReflectedTypeStorageAccessor.
class W_TOOLSFOUNDATION_DLL WReflectedTypeStorageManager
{
public:
  WReflectedTypeStorageManager();

private:
  struct ReflectedTypeStorageMapping
  {
    struct StorageInfo
    {
      StorageInfo()
        : m_uiIndex(0)
        , m_Type(WVariant::Type::Invalid)
      {
      }
      StorageInfo(WUInt16 uiIndex, WVariant::Type::Enum type, const WVariant& defaultValue)
        : m_uiIndex(uiIndex)
        , m_Type(type)
        , m_DefaultValue(defaultValue)
      {
      }

      WUInt16 m_uiIndex;
      WEnum<WVariant::Type> m_Type;
      WVariant m_DefaultValue;
    };

    /// Flattens all POD type properties of the given WRTTI into m_PathToStorageInfoTable.
    ///
    /// The functions first adds all parent class properties and then adds its own properties.
    /// POD type properties are added under the current path.
    void AddProperties(const WRTTI* pType);
    void AddPropertiesRecursive(const WRTTI* pType, WSet<const WDocumentObject*>& ref_requiresPatchingEmbeddedClass);

    void UpdateInstances(WUInt32 uiIndex, const WAbstractProperty* pProperty, WSet<const WDocumentObject*>& ref_requiresPatchingEmbeddedClass);
    void AddPropertyToInstances(WUInt32 uiIndex, const WAbstractProperty* pProperty, WSet<const WDocumentObject*>& ref_requiresPatchingEmbeddedClass);

    WSet<WReflectedTypeStorageAccessor*> m_Instances;
    WHashTable<WString, StorageInfo> m_PathToStorageInfoTable;
  };

  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(ToolsFoundation, ReflectedTypeStorageManager);
  friend class WReflectedTypeStorageAccessor;

  static void Startup();
  static void Shutdown();

  static const ReflectedTypeStorageMapping* AddStorageAccessor(WReflectedTypeStorageAccessor* pInstance);
  static void RemoveStorageAccessor(WReflectedTypeStorageAccessor* pInstance);

  static ReflectedTypeStorageMapping* GetTypeStorageMapping(const WRTTI* pType);
  static void TypeEventHandler(const WPhantomRttiManagerEvent& e);
  static void PluginEventHandler(const WPluginEvent& EventData);

private:
  static WMap<const WRTTI*, ReflectedTypeStorageMapping*> s_ReflectedTypeToStorageMapping;
};
