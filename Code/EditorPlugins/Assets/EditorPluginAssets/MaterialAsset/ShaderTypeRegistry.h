#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Timestamp.h>

struct WPhantomRttiManagerEvent;
struct WPluginEvent;

class WShaderTypeRegistry
{
  W_DECLARE_SINGLETON(WShaderTypeRegistry);

public:
  WShaderTypeRegistry();
  ~WShaderTypeRegistry();

  const WRTTI* GetShaderType(WStringView sShaderPath);
  const WRTTI* GetShaderBaseType() const { return m_pBaseType; }

private:
  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(EditorFramework, ShaderTypeRegistry);

  struct ShaderData
  {
    ShaderData() = default;

    WString m_sShaderPath;
    WString m_sAbsShaderPath;
    WTimestamp m_fileModifiedTime;
    const WRTTI* m_pType = nullptr;
  };
  void UpdateShaderType(ShaderData& data);

  void RegisterBaseType();
  void PhantomTypeRegistryEventHandler(const WPhantomRttiManagerEvent& e);

  void PluginEventHandler(const WPluginEvent& e);

  WMap<WString, ShaderData> m_ShaderTypes;
  const WRTTI* m_pBaseType = nullptr;
};
