#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Strings/HashedString.h>

/// Defines categories and metadata for spatial data used by spatial systems.
///
/// Provides a category system for organizing spatial objects (like render objects, collision objects)
/// that can be used by spatial systems for efficient queries and updates. Categories are registered
/// globally and can have flags to indicate update frequency hints.
struct WSpatialData
{
  struct Flags
  {
    using StorageType = WUInt8;

    enum Enum
    {
      None = 0,
      FrequentChanges = W_BIT(0), ///< Indicates that objects in this category change their bounds frequently. Spatial System implementations can use that as hint for internal optimizations.

      Default = None
    };

    struct Bits
    {
      StorageType FrequentUpdates : 1;
    };
  };

  /// Represents a spatial data category for organizing objects in spatial systems.
  struct Category
  {
    W_ALWAYS_INLINE Category()
      : m_uiValue(WSmallInvalidIndex)
    {
    }

    W_ALWAYS_INLINE explicit Category(WUInt16 uiValue)
      : m_uiValue(uiValue)
    {
    }

    W_ALWAYS_INLINE bool operator==(const Category& other) const { return m_uiValue == other.m_uiValue; }
    W_ALWAYS_INLINE bool operator!=(const Category& other) const { return m_uiValue != other.m_uiValue; }

    WUInt16 m_uiValue;

    /// Returns the bitmask representation of this category for use in queries.
    W_ALWAYS_INLINE WUInt32 GetBitmask() const { return m_uiValue != WSmallInvalidIndex ? static_cast<WUInt32>(W_BIT(m_uiValue)) : 0; }
  };

  /// Registers a spatial data category under the given name.
  ///
  /// If the same category was already registered before, it returns that instead.
  /// Asserts that there are no more than 32 unique categories.
  W_CORE_DLL static Category RegisterCategory(WStringView sCategoryName, const WBitflags<Flags>& flags);

  /// Returns either an existing category with the given name or WInvalidSpatialDataCategory.
  W_CORE_DLL static Category FindCategory(WStringView sCategoryName);

  /// Returns the name of the given category.
  W_CORE_DLL static const WHashedString& GetCategoryName(Category category);

  /// Returns the flags for the given category.
  W_CORE_DLL static const WBitflags<Flags>& GetCategoryFlags(Category category);

private:
  struct CategoryData
  {
    WHashedString m_sName;
    WBitflags<Flags> m_Flags;
  };

  static WHybridArray<WSpatialData::CategoryData, 32>& GetCategoryData();
};

/// Predefined spatial data categories commonly used throughout the engine.
struct W_CORE_DLL WDefaultSpatialDataCategories
{
  static WSpatialData::Category RenderStatic;     ///< Static render objects that don't change position frequently
  static WSpatialData::Category RenderDynamic;    ///< Dynamic render objects that may change position frequently
  static WSpatialData::Category OcclusionStatic;  ///< Static objects used for occlusion culling
  static WSpatialData::Category OcclusionDynamic; ///< Dynamic objects used for occlusion culling
};

/// When an object is 'seen' by a view and thus tagged as 'visible', this enum describes what kind of observer triggered this.
///
/// This is used to determine how important certain updates, such as animations, are to execute.
/// E.g. when a 'shadow view' or 'reflection view' is the only thing that observes an object, animations / particle effects and so on,
/// can be updated less frequently.
struct WVisibilityState
{
  using StorageType = WUInt8;

  enum Enum : StorageType
  {
    Invisible = 0, ///< The object isn't visible to any view.
    Indirect = 1,  ///< The object is seen by a view that only indirectly makes the object visible (shadow / reflection / render target).
    Direct = 2,    ///< The object is seen directly by a main view and therefore it needs to be updated at maximum frequency.

    Default = Invisible
  };
};

#define WInvalidSpatialDataCategory WSpatialData::Category()
