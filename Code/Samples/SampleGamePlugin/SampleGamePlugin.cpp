#include <SampleGamePlugin/SampleGamePluginPCH.h>

#include <SampleGamePlugin/SampleGamePluginDLL.h>

// BEGIN-DOCS-CODE-SNIPPET: plugin-setup
W_PLUGIN_ON_LOADED()
{
  // you could do something here, though this is rare
}

W_PLUGIN_ON_UNLOADED()
{
  // you could do something here, though this is rare
}
// END-DOCS-CODE-SNIPPET
