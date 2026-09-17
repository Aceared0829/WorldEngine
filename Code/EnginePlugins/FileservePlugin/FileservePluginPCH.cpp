#include <FileservePlugin/FileservePluginPCH.h>

W_STATICLINK_LIBRARY(FileServePlugin)
{
  if (bReturn)
    return;

  W_STATICLINK_REFERENCE(FileServePlugin_Client_FileserveClient);
  W_STATICLINK_REFERENCE(FileServePlugin_FileservePlugin);
}
