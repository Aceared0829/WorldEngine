#pragma once

#include <EditorFramework/CodeGen/CppSettings.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/VariantType.h>

/// Specifies which IDE to use for opening C++ project solution files.
/// Only saved in editor preferences, does not have to work cross-platform.
struct W_EDITORFRAMEWORK_DLL WIDE
{
  using StorageType = WUInt8;

  enum Enum
  {
    DefaultProgram,   ///< Uses the system default way of opening an sln file
#if W_ENABLED(W_PLATFORM_WINDOWS)
    VisualStudio,     ///< Opens with Visual Studio
#endif
    VisualStudioCode, ///< Opens with VS Code
    Rider,            ///< Opens with JetBrains Rider
    Default = DefaultProgram
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_EDITORFRAMEWORK_DLL, WIDE);

/// Specifies which compiler to use for building C++ plugin code.
/// Only saved in editor preferences, does not have to work cross-platform.
struct W_EDITORFRAMEWORK_DLL WCompiler
{
  using StorageType = WUInt8;

  enum Enum
  {
    Clang,
#if W_ENABLED(W_PLATFORM_LINUX)
    Gcc,
#elif W_ENABLED(W_PLATFORM_WINDOWS)
    Vs2022,
    Vs2026,
#endif

#if W_ENABLED(W_PLATFORM_WINDOWS)
    Default = Vs2022
#else
    Default = Gcc
#endif
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_EDITORFRAMEWORK_DLL, WCompiler);

/// Stores compiler configuration including custom compiler paths.
struct W_EDITORFRAMEWORK_DLL WCompilerPreferences
{
  WEnum<WCompiler> m_Compiler;
  bool m_bCustomCompiler = false; ///< If true, use custom compiler paths instead of predefined compiler
  WString m_sCppCompiler;        ///< Path to C++ compiler executable
  WString m_sCCompiler;          ///< Path to C compiler executable
  WString m_sRcCompiler;         ///< Path to resource compiler executable
};

W_DECLARE_REFLECTABLE_TYPE(W_EDITORFRAMEWORK_DLL, WCompilerPreferences);

/// Configuration for launching an external code editor.
struct W_EDITORFRAMEWORK_DLL WCodeEditorPreferences
{
  bool m_bIsVisualStudio = false; ///< If true, uses Visual Studio specific command line format
  WString m_sEditorPath;         ///< Full path to the code editor executable
  WString m_sEditorArgs;         ///< Command line arguments passed to the editor
};

W_DECLARE_REFLECTABLE_TYPE(W_EDITORFRAMEWORK_DLL, WCodeEditorPreferences);

/// Manages C++ plugin project generation, compilation, and IDE integration.
///
/// Provides functionality to generate CMake projects for C++ game plugins,
/// configure compilers, build solutions, and open code in external editors.
struct W_EDITORFRAMEWORK_DLL WCppProject : public WPreferences
{
  W_ADD_DYNAMIC_REFLECTION(WCppProject, WPreferences);

  /// Describes a compiler configuration detected or configured on this machine.
  struct MachineSpecificCompilerPaths
  {
    WString m_sNiceName;    ///< Display name for this compiler configuration
    WEnum<WCompiler> m_Compiler;
    WString m_sCCompiler;   ///< Path to C compiler
    WString m_sCppCompiler; ///< Path to C++ compiler
    bool m_bIsCustom;        ///< If true, this is a user-configured compiler, not auto-detected
  };

  /// Result type for operations that may modify files.
  enum class ModifyResult
  {
    FAILURE,      ///< Operation failed
    NOT_MODIFIED, ///< Files already up-to-date, no changes made
    MODIFIED      ///< Files were successfully modified
  };


  WCppProject();
  ~WCppProject();

  /// Returns the directory where C++ plugin template files are copied to.
  /// If sProjectDirectory is empty, uses the current project directory.
  static WString GetTargetSourceDir(WStringView sProjectDirectory = {});

  /// Returns the CMake generator-specific folder name (e.g., "vs2022x64").
  static WString GetGeneratorFolderName(const WCppSettings& cfg);

  /// Returns the CMake generator name string for the -G parameter.
  static WString GetCMakeGeneratorName(const WCppSettings& cfg);

  /// Returns the source directory for the C++ plugin project.
  /// If sProjectDirectory is empty, uses the current project directory.
  static WString GetPluginSourceDir(const WCppSettings& cfg, WStringView sProjectDirectory = {});

  /// Returns the CMake build directory path.
  static WString GetBuildDir(const WCppSettings& cfg);

  /// Returns the full path to the generated solution file.
  static WString GetSolutionPath(const WCppSettings& cfg);

  /// Checks that the build directory leaves enough room for the paths CMake and the compiler toolchain
  /// generate below it.
  ///
  /// This is about the external build tools, not about WorldEngine's own file access: WOSFile goes through
  /// WDosDevicePath and handles paths beyond MAX_PATH, but MSBuild fails on the intermediate files it
  /// writes itself (its .tlog files in particular). That failure otherwise surfaces only as an opaque
  /// exception deep in the CMake output. Only ever fails on Windows.
  static WStatus CheckBuildPathLength(const WCppSettings& cfg);

  /// Opens the solution file in the configured IDE.
  static WStatus OpenSolution(const WCppSettings& cfg);

  /// Attempts to launch the configured code editor with the specified file and line number
  static WStatus OpenInCodeEditor(const WStringView& sFileName, WInt32 iLineNumber);

  static WStringView CompilerToString(WCompiler::Enum compiler);

  /// Returns the compiler used to build the editor SDK itself.
  /// Plugins should typically use a compatible compiler to avoid ABI issues.
  static WCompiler::Enum GetSdkCompiler();

  /// Returns the major version number of the compiler used to build the SDK.
  static WString GetSdkCompilerMajorVersion();

  /// Tests if the currently configured compiler can be invoked successfully.
  static WStatus TestCompiler();

  /// Returns the path to the CMake executable bundled with the engine.
  static const char* GetCMakePath();

  /// Verifies that the CMake cache matches the current configuration.
  /// Returns failure if the cache is outdated or incompatible.
  static WResult CheckCMakeCache(const WCppSettings& cfg);

  /// Checks if CMakeUserPresets.json needs updating for the given configuration.
  /// If bWriteResult is true, writes the updated file when changes are needed.
  static ModifyResult CheckCMakeUserPresets(const WCppSettings& cfg, bool bWriteResult);

  static bool ExistsSolution(const WCppSettings& cfg);

  static bool ExistsProjectCMakeListsTxt();

  /// Returns true if sDst doesn't exist yet, or for some file types (e.g. CMakeLists.txt), when they got an update.
  static bool ShouldOverwriteExisting(WStringView sSrc, WStringView sDst);

  /// Copies default C++ plugin template files to the project's source directory.
  static WResult PopulateWithDefaultSources(const WCppSettings& cfg, WUInt32* pNumFilesCopied = nullptr);

  static WResult CleanBuildDir(const WCppSettings& cfg);

  /// Runs CMake to generate the build system files.
  static WResult RunCMake(const WCppSettings& cfg);

  /// Checks if CMake needs to run and executes it if required.
  /// Skips CMake if the build files are already up-to-date.
  static WResult RunCMakeIfNecessary(const WCppSettings& cfg);

  /// Compiles the C++ plugin solution using the configured compiler.
  static WResult CompileSolution(const WCppSettings& cfg);

  /// Checks if compilation is needed and builds the code if required.
  /// Skips the build if the plugin binary is already up-to-date.
  static WResult BuildCodeIfNecessary(const WCppSettings& cfg);

  /// Creates a minimal CMakeUserPresets.json structure for the given configuration.
  static WVariantDictionary CreateEmptyCMakeUserPresetsJson(const WCppSettings& cfg);

  /// Updates an existing CMakeUserPresets.json with current configuration settings.
  /// The inout_json parameter is both read and modified.
  static ModifyResult ModifyCMakeUserPresetsJson(const WCppSettings& cfg, WVariantDictionary& inout_json);

  /// Writes the plugin configuration header files based on current settings.
  static void UpdatePluginConfig(const WCppSettings& cfg);

  /// Writes CMakeEngineExtensions.txt with the selected engine plugins for linking.
  static WResult UpdateEnginePluginDependencies();

  /// Ensures the C++ plugin project is generated and compiled.
  /// Runs all necessary steps to make the plugin ready for use.
  static WResult EnsureCppPluginReady();

  /// Checks whether the C++ plugin needs to be rebuilt.
  /// Returns true if source files have changed since the last build.
  static bool IsBuildRequired();

  /// Fired when a notable change has been made.
  static WEvent<const WCppSettings&> s_ChangeEvents;

  static void LoadPreferences();

  static WArrayPtr<const MachineSpecificCompilerPaths> GetMachineSpecificCompilers() { return s_MachineSpecificCompilers.GetArrayPtr(); }

  /// Changes the current preferences to use an SDK-compatible compiler.
  /// This is necessary to avoid ABI compatibility issues between the plugin and engine.
  static WResult ForceSdkCompatibleCompiler();

private:
  WEnum<WIDE> m_Ide;
  WCompilerPreferences m_CompilerPreferences;
  WCodeEditorPreferences m_CodeEditorPreferences;

  static WDynamicArray<MachineSpecificCompilerPaths> s_MachineSpecificCompilers;
};
