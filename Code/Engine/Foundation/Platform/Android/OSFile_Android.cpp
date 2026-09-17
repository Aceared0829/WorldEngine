#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_ANDROID)
#  include <Foundation/Platform/Android/Utils/AndroidJni.h>
#  include <Foundation/Platform/Android/Utils/AndroidUtils.h>
#  include <android_native_app_glue.h>

#  define W_POSIX_FILE_NOGETAPPLICATIONPATH
#  define W_POSIX_FILE_NOGETUSERDATAFOLDER
#  define W_POSIX_FILE_NOGETTEMPDATAFOLDER
#  define W_POSIX_FILE_NOGETUSERDOCUMENTSFOLDER

#  include <Foundation/Platform/Posix/OSFile_Posix.inl>

WStringView WOSFile::GetApplicationPath()
{
  if (s_sApplicationPath.IsEmpty())
  {
    WJniAttachment attachment;

    WJniString packagePath = attachment.GetActivity().Call<WJniString>("getPackageCodePath");
    // By convention, android requires assets to be placed in the 'Assets' folder
    // inside the apk thus we use that as our SDK root.
    WStringBuilder sTemp = packagePath.GetData();
    sTemp.AppendPath("Assets/WDummyBin");
    s_sApplicationPath = sTemp;
  }

  return s_sApplicationPath;
}

WString WOSFile::GetUserDataFolder(WStringView sSubFolder)
{
  if (s_sUserDataPath.IsEmpty())
  {
    android_app* app = WAndroidUtils::GetAndroidApp();
    s_sUserDataPath = app->activity->internalDataPath;
  }

  WStringBuilder s = s_sUserDataPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

WString WOSFile::GetTempDataFolder(WStringView sSubFolder)
{
  if (s_sTempDataPath.IsEmpty())
  {
    WJniAttachment attachment;

    WJniObject cacheDir = attachment.GetActivity().Call<WJniObject>("getCacheDir");
    WJniString path = cacheDir.Call<WJniString>("getPath");
    s_sTempDataPath = path.GetData();
  }

  WStringBuilder s = s_sTempDataPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

WString WOSFile::GetUserDocumentsFolder(WStringView sSubFolder)
{
  if (s_sUserDocumentsPath.IsEmpty())
  {
    W_ASSERT_NOT_IMPLEMENTED;
  }

  WStringBuilder s = s_sUserDocumentsPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

#endif
