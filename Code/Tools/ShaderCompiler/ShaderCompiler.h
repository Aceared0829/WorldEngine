#pragma once

#include <GameEngine/GameApplication/GameApplication.h>
#include <RendererCore/ShaderCompiler/PermutationGenerator.h>

class WShaderCompilerApplication : public WGameApplication
{
public:
  using SUPER = WGameApplication;

  WShaderCompilerApplication();

  virtual void Run() override;

private:
  void PrintConfig();
  WResult CompileShader(WStringView sShaderFile);
  WResult ExtractPermutationVarValues(WStringView sShaderFile);

  virtual WResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;
  virtual void Init_LoadProjectPlugins() override {}
  virtual void Init_SetupDefaultResources() override {}
  virtual void Init_ConfigureTags() override {}
  virtual bool Run_ProcessApplicationInput() override { return true; }

  WPermutationGenerator m_PermutationGenerator;
  WString m_sPlatforms;
  WString m_sShaderFiles;
  WMap<WString, WHybridArray<WString, 4>> m_FixedPermVars;
};
