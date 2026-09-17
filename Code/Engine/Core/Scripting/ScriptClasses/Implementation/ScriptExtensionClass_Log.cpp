#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClasses/ScriptExtensionClass_Log.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WScriptExtensionClass_Log, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(Info, In, "Text", In, "Params")->AddAttributes(new WDynamicPinAttribute("Params")),
    W_SCRIPT_FUNCTION_PROPERTY(Warning, In, "Text", In, "Params")->AddAttributes(new WDynamicPinAttribute("Params")),
    W_SCRIPT_FUNCTION_PROPERTY(Error, In, "Text", In, "Params")->AddAttributes(new WDynamicPinAttribute("Params")),
  }
  W_END_FUNCTIONS;
  W_BEGIN_ATTRIBUTES
  {
    new WScriptExtensionAttribute("Log"),
  }
  W_END_ATTRIBUTES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

static WStringView BuildFormattedText(WStringView sText, const WVariantArray& params, WStringBuilder& ref_sStorage)
{
  WTempHybridArray<WString, 12> stringStorage;
  stringStorage.Reserve(params.GetCount());
  for (auto& param : params)
  {
    stringStorage.PushBack(param.ConvertTo<WString>());
  }

  WTempHybridArray<WStringView, 12> stringViews;
  stringViews.Reserve(stringStorage.GetCount());
  for (auto& s : stringStorage)
  {
    stringViews.PushBack(s);
  }

  WFormatString fs(sText);
  return fs.BuildFormattedText(ref_sStorage, stringViews.GetData(), stringViews.GetCount());
}

// static
void WScriptExtensionClass_Log::Info(WStringView sText, const WVariantArray& params)
{
  WStringBuilder sStorage;
  WLog::Info(BuildFormattedText(sText, params, sStorage));
}

// static
void WScriptExtensionClass_Log::Warning(WStringView sText, const WVariantArray& params)
{
  WStringBuilder sStorage;
  WLog::Warning(BuildFormattedText(sText, params, sStorage));
}

// static
void WScriptExtensionClass_Log::Error(WStringView sText, const WVariantArray& params)
{
  WStringBuilder sStorage;
  WLog::Error(BuildFormattedText(sText, params, sStorage));
}


W_STATICLINK_FILE(Core, Core_Scripting_ScriptClasses_Implementation_ScriptExtensionClass_Log);
