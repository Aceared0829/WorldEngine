#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/UniquePtr.h>

/// What a translated string is used for.
enum class WTranslationUsage
{
  Default,
  Tooltip,
  HelpURL,

  ENUM_COUNT
};

/// Base class to translate one string into another
class W_FOUNDATION_DLL WTranslator
{
public:
  WTranslator();
  virtual ~WTranslator();

  /// The given string (with the given hash) shall be translated
  virtual WStringView Translate(WStringView sString, WUInt64 uiStringHash, WTranslationUsage usage) = 0;

  /// Called to reset internal state
  virtual void Reset();

  /// May reload the known translations
  virtual void Reload();

  /// Will call Reload() on all currently active translators
  static void ReloadAllTranslators();

  static void HighlightUntranslated(bool bHighlight);

  static bool GetHighlightUntranslated() { return s_bHighlightUntranslated; }

private:
  static bool s_bHighlightUntranslated;
  static WHybridArray<WTranslator*, 4> s_AllTranslators;
};

/// Just returns the same string that is passed into it. Can be used to display the actually untranslated strings
class W_FOUNDATION_DLL WTranslatorPassThrough : public WTranslator
{
public:
  virtual WStringView Translate(WStringView sString, WUInt64 uiStringHash, WTranslationUsage usage) override
  {
    W_IGNORE_UNUSED(uiStringHash);
    W_IGNORE_UNUSED(usage);
    return sString;
  }
};

/// Can store translated strings and all translation requests will come from that storage. Returns nullptr if the requested string is
/// not known
class W_FOUNDATION_DLL WTranslatorStorage : public WTranslator
{
public:
  /// Stores szString as the translation for the string with the given hash
  virtual void StoreTranslation(WStringView sString, WUInt64 uiStringHash, WTranslationUsage usage);

  /// Returns the translated string for uiStringHash, or nullptr, if not available
  virtual WStringView Translate(WStringView sString, WUInt64 uiStringHash, WTranslationUsage usage) override;

  /// Clears all stored translation strings
  virtual void Reset() override;

  /// Simply executes Reset() on this translator
  virtual void Reload() override;

protected:
  WMap<WUInt64, WString> m_Translations[(int)WTranslationUsage::ENUM_COUNT];
};

/// Outputs a 'Missing Translation' warning the first time a string translation is requested.
/// Otherwise always returns nullptr, allowing the next translator to take over.
class W_FOUNDATION_DLL WTranslatorLogMissing : public WTranslatorStorage
{
public:
  /// Can be used from external code to (temporarily) deactivate error logging (a bit hacky)
  static bool s_bActive;

  virtual WStringView Translate(WStringView sString, WUInt64 uiStringHash, WTranslationUsage usage) override;
};

/// Loads translations from files. Each translator can have different search paths, but the files to be loaded are the same for all of them.
class W_FOUNDATION_DLL WTranslatorFromFiles : public WTranslatorStorage
{
public:
  /// Loads all files recursively from the specified folder as translation files.
  ///
  /// The given path must be absolute or resolvable to an absolute path.
  /// On failure, the function does nothing.
  /// This function depends on WFileSystemIterator to be available.
  void AddTranslationFilesFromFolder(const char* szFolder);

  virtual WStringView Translate(WStringView sString, WUInt64 uiStringHash, WTranslationUsage usage) override;

  virtual void Reload() override;

private:
  void LoadTranslationFile(const char* szFullPath);

  WHybridArray<WString, 4> m_Folders;
};

/// Returns the same string that is passed into it, but strips off class names and separates the text at CamelCase boundaries.
class W_FOUNDATION_DLL WTranslatorMakeMoreReadable : public WTranslatorStorage
{
public:
  virtual WStringView Translate(WStringView sString, WUInt64 uiStringHash, WTranslationUsage usage) override;
};

/// Handles looking up translations for strings.
///
/// Multiple translators can be registered to get translations from different sources.
class W_FOUNDATION_DLL WTranslationLookup
{
public:
  /// Translators will be queried in the reverse order that they were added.
  static void AddTranslator(WUniquePtr<WTranslator> pTranslator);

  /// Prefer to use the WTranslate macro instead of calling this function directly. Will query all translators for a translation,
  /// until one is found.
  static WStringView Translate(WStringView sString, WUInt64 uiStringHash, WTranslationUsage usage);

  /// Deletes all translators.
  static void Clear();

private:
  static WHybridArray<WUniquePtr<WTranslator>, 16> s_Translators;
};

/// Use this macro to query a translation for a string from the WTranslationLookup system
#define WTranslate(string) WTranslationLookup::Translate(string, WHashingUtils::StringHash(string), WTranslationUsage::Default)

/// Use this macro to query a translation for a tooltip string from the WTranslationLookup system
#define WTranslateTooltip(string) WTranslationLookup::Translate(string, WHashingUtils::StringHash(string), WTranslationUsage::Tooltip)

/// Use this macro to query a translation for a help URL from the WTranslationLookup system
#define WTranslateHelpURL(string) WTranslationLookup::Translate(string, WHashingUtils::StringHash(string), WTranslationUsage::HelpURL)
