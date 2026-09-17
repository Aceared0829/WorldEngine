#include <CppProjectPlugin/CppProjectPluginPCH.h>

#include <Core/Input/InputManager.h>
#include <Core/System/Window.h>
#include <Core/World/World.h>
#include <CppProjectPlugin/GameState/CppProjectGameState.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Logging/Log.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/MeshComponent.h>

WCVarBool cvar_DebugDisplay("CppProject.DebugDisplay", false, WCVarFlags::Default, "Whether the game should display debug geometry.");

W_BEGIN_DYNAMIC_REFLECTED_TYPE(CppProjectGameState, 1, WRTTIDefaultAllocator<CppProjectGameState>)
W_END_DYNAMIC_REFLECTED_TYPE;

CppProjectGameState::CppProjectGameState() = default;
CppProjectGameState::~CppProjectGameState() = default;

void CppProjectGameState::GetStartupOptions(WString& out_sScene, WString& out_sPreloadCollection)
{
  // replace this to load a certain scene at startup
  // the default implementation looks at the command line "-scene" argument

  // if we have a "-scene" command line argument, it was launched from the editor and we should load that
  if (WCommandLineUtils::GetGlobalInstance()->HasOption("-scene"))
  {
    out_sScene = WCommandLineUtils::GetGlobalInstance()->GetStringOption("-scene");
  }
  else
  {
    // otherwise, we use the hardcoded 'Main.WScene'
    // if that doesn't exist, this function has to be adjusted
    // note that you can return an asset GUID here, instead of a path
    out_sScene = "AssetCache/Common/Scenes/Main.WBinScene";
  }

  WStringBuilder sPreloadCollection = out_sScene;
  sPreloadCollection.ChangeFileExtension("WBinCollection");
  if (WFileSystem::ExistsFile(sPreloadCollection))
  {
    out_sPreloadCollection = sPreloadCollection;
  }
}

void CppProjectGameState::OnActivation(WWorld* pWorld, WStringView sStartPosition, const WTransform& startPositionOffset)
{
  W_LOG_BLOCK("GameState::Activate");

  SUPER::OnActivation(pWorld, sStartPosition, startPositionOffset);

  // the main entry point when the game starts
  // could do some setup here, but in a lot of cases it is better to leave this as is
  // and instead override the various other virtual functions that the game state provides
  // see below and see WGameState for additional details
}

void CppProjectGameState::AfterWorldUpdate()
{
  SUPER::AfterWorldUpdate();

  if (cvar_DebugDisplay)
  {
    WDebugRenderer::DrawLineSphere(m_pMainWorld, WBoundingSphere::MakeFromCenterAndRadius(WVec3::MakeZero(), 1.0f), WColor::Orange);
  }

  WDebugRenderer::Draw2DText(m_pMainWorld, "Press 'O' to spawn objects", WVec2I32(10, 10), WColor::White);
  WDebugRenderer::Draw2DText(m_pMainWorld, "Press 'P' to remove objects", WVec2I32(10, 30), WColor::White);
}

void CppProjectGameState::BeforeWorldUpdate()
{
  SUPER::BeforeWorldUpdate();

  W_LOCK(m_pMainWorld->GetWriteMarker());

  // if you need to modify the world, this is a good place to do it
}

WResult CppProjectGameState::SpawnPlayer(WStringView sStartPosition, const WTransform& startPositionOffset)
{
  // replace this to create a custom player object or load a prefab
  return SUPER::SpawnPlayer(sStartPosition, startPositionOffset);
}

void CppProjectGameState::OnChangedMainWorld(WWorld* pPrevWorld, WWorld* pNewWorld, WStringView sStartPosition, const WTransform& startPositionOffset)
{
  SUPER::OnChangedMainWorld(pPrevWorld, pNewWorld, sStartPosition, startPositionOffset);

  // called whenever the main world is changed, ie when transitioning between levels
  // may need to update references to the world here or reset some state
}

static void RegisterInputAction(const char* szInputSet, const char* szInputAction, const char* szKey1, const char* szKey2 = nullptr, const char* szKey3 = nullptr)
{
  WInputActionConfig cfg;
  cfg.m_bApplyTimeScaling = true;
  cfg.m_sInputSlotTrigger[0] = szKey1;
  cfg.m_sInputSlotTrigger[1] = szKey2;
  cfg.m_sInputSlotTrigger[2] = szKey3;

  WInputManager::SetInputActionConfig(szInputSet, szInputAction, cfg, true);
}

void CppProjectGameState::ConfigureInputActions()
{
  SUPER::ConfigureInputActions();

  RegisterInputAction("CppProjectPlugin", "SpawnObject", WInputSlot_KeyO, WInputSlot_Controller0_ButtonA, WInputSlot_MouseButton2);
  RegisterInputAction("CppProjectPlugin", "DeleteObject", WInputSlot_KeyP, WInputSlot_Controller0_ButtonB);
}

void CppProjectGameState::ProcessInput()
{
  SUPER::ProcessInput();

  WWorld* pWorld = m_pMainWorld;

  if (WInputManager::GetInputActionState("CppProjectPlugin", "SpawnObject") == WKeyState::Pressed)
  {
    const WVec3 pos = GetMainCamera()->GetCenterPosition() + GetMainCamera()->GetCenterDirForwards();

    // make sure we are allowed to modify the world
    W_LOCK(pWorld->GetWriteMarker());

    // create a game object at the desired position
    WGameObjectDesc desc;
    desc.m_LocalPosition = pos;

    WGameObject* pObject = nullptr;
    WGameObjectHandle hObject = pWorld->CreateObject(desc, pObject);

    m_SpawnedObjects.PushBack(hObject);

    // attach a mesh component to the object
    WMeshComponent* pMesh;
    pWorld->GetOrCreateComponentManager<WMeshComponentManager>()->CreateComponent(pObject, pMesh);

    // Set the mesh to use.
    // Here we use a path relative to the project directory.
    // We have to reference the 'transformed' file, not the source file.
    // This would break if the source asset is moved or renamed.
    pMesh->SetMeshFile("AssetCache/Common/Meshes/Sphere.WBinMesh");

    // here we use the asset GUID to reference the transformed asset
    // we can copy the GUID from the asset browser
    // the GUID is stable even if the source asset gets moved or renamed
    // using asset collections we could also give a nice name like 'Blue Material' to this asset
    WMaterialResourceHandle hMaterial = WResourceManager::LoadResource<WMaterialResource>("{ aa1c5601-bc43-fbf8-4e07-6a3df3af51e7 }");

    // override the mesh material in the first slot with something different
    pMesh->SetMaterial(0, hMaterial);
  }

  if (WInputManager::GetInputActionState("CppProjectPlugin", "DeleteObject") == WKeyState::Pressed)
  {
    if (!m_SpawnedObjects.IsEmpty())
    {
      // make sure we are allowed to modify the world
      W_LOCK(pWorld->GetWriteMarker());

      WGameObjectHandle hObject = m_SpawnedObjects.PeekBack();
      m_SpawnedObjects.PopBack();

      // this is only for demonstration purposes, removing the object will delete all attached components as well
      WGameObject* pObject = nullptr;
      if (pWorld->TryGetObject(hObject, pObject))
      {
        WMeshComponent* pMesh = nullptr;
        if (pObject->TryGetComponentOfBaseType(pMesh))
        {
          pMesh->DeleteComponent();
        }
      }

      // delete the object, all its children and attached components
      pWorld->DeleteObjectDelayed(hObject);
    }
  }
}

void CppProjectGameState::ConfigureMainCamera()
{
  SUPER::ConfigureMainCamera();

  // do custom camera setup here
}
