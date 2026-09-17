#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/LUTAsset/LUTAssetObjects.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLUTAssetProperties, 1, WRTTIDefaultAllocator<WLUTAssetProperties>)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Input", GetInputFile, SetInputFile)->AddAttributes(new WFileBrowserAttribute("Select CUBE file", "*.cube"), new WRequiredAttribute()),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void WLUTAssetProperties::PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == WGetStaticRTTI<WLUTAssetProperties>())
  {
    auto& props = *e.m_pPropertyStates;

    props["Input"].m_Visibility = WPropertyUiState::Default;
    props["Input"].m_sNewLabelText = "WLUTAssetProperties::CUBEfile";
  }
}

WString WLUTAssetProperties::GetAbsoluteInputFilePath() const
{
  WStringBuilder sPath = m_sInput;
  sPath.MakeCleanPath();

  if (!sPath.IsAbsolutePath())
  {
    WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath);
  }

  return sPath;
}
