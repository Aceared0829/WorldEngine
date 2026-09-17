#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Strings/String.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class WDocument;

class W_TOOLSFOUNDATION_DLL WApplicationServices
{
  W_DECLARE_SINGLETON(WApplicationServices);

public:
  WApplicationServices();

  /// A writable folder in which application specific user data may be stored
  WString GetApplicationUserDataFolder() const;

  /// A read-only folder in which application specific data may be located
  WString GetApplicationDataFolder() const;

  /// The writable location where the application should store preferences (user specific settings)
  WString GetApplicationPreferencesFolder() const;

  /// The writable location where preferences for the current WToolsProject should be stored (user specific settings)
  WString GetProjectPreferencesFolder() const;

  WString GetProjectPreferencesFolder(WStringView sProjectFilePath) const;

  /// The writable location where preferences for the given WDocument should be stored (user specific settings)
  WString GetDocumentPreferencesFolder(const WDocument* pDocument) const;

  /// The read-only folder where pre-compiled binaries for external tools can be found
  WString GetPrecompiledToolsFolder(bool bUsePrecompiledTools) const;

  /// The folder under which the sample projects are stored
  WString GetSampleProjectsFolder() const;
};
