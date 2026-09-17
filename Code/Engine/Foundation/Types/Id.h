#pragma once

/// \file

#include <Foundation/Basics.h>

/// Declares an id type, see generic id below how to use this
#define W_DECLARE_ID_TYPE(name, instanceIndexBits, generationBits)                                                     \
  static const StorageType MAX_INSTANCES = (1ULL << instanceIndexBits);                                                 \
  static const StorageType INVALID_INSTANCE_INDEX = MAX_INSTANCES - 1;                                                  \
  static const StorageType INDEX_AND_GENERATION_MASK = ((WUInt64) - 1) >> (64 - (instanceIndexBits + generationBits)); \
  W_DECLARE_POD_TYPE();                                                                                                \
  W_ALWAYS_INLINE name()                                                                                               \
  {                                                                                                                     \
    m_Data = INVALID_INSTANCE_INDEX;                                                                                    \
  }                                                                                                                     \
  W_ALWAYS_INLINE explicit name(StorageType internalData)                                                              \
  {                                                                                                                     \
    m_Data = internalData;                                                                                              \
  }                                                                                                                     \
  W_ALWAYS_INLINE bool operator==(const name other) const                                                              \
  {                                                                                                                     \
    return m_Data == other.m_Data;                                                                                      \
  }                                                                                                                     \
  W_ALWAYS_INLINE bool operator!=(const name other) const                                                              \
  {                                                                                                                     \
    return m_Data != other.m_Data;                                                                                      \
  }                                                                                                                     \
  W_ALWAYS_INLINE bool operator<(const name other) const                                                               \
  {                                                                                                                     \
    return m_Data < other.m_Data;                                                                                       \
  }                                                                                                                     \
  W_ALWAYS_INLINE void Invalidate()                                                                                    \
  {                                                                                                                     \
    m_Data = INVALID_INSTANCE_INDEX;                                                                                    \
  }                                                                                                                     \
  W_ALWAYS_INLINE bool IsInvalidated() const                                                                           \
  {                                                                                                                     \
    return m_Data == INVALID_INSTANCE_INDEX;                                                                            \
  }                                                                                                                     \
  W_ALWAYS_INLINE bool IsIndexAndGenerationEqual(const name other) const                                               \
  {                                                                                                                     \
    return (m_Data & INDEX_AND_GENERATION_MASK) == (other.m_Data & INDEX_AND_GENERATION_MASK);                          \
  }


/// Generic identifier template that combines instance indexing with generation counting for safe object references.
///
/// This ID system solves the "dangling pointer" problem for object management by using a two-part identifier:
/// - Instance Index: Points to a slot in an object array or similar data structure
/// - Generation Counter: Detects when a slot has been reused for a different object
///
/// When an object is destroyed, its generation counter is incremented. Any existing IDs with the old
/// generation value become automatically invalid, preventing access to the new object that might
/// occupy the same index.
///
/// Benefits:
/// - Safe object references that can detect stale access
/// - Efficient array-based object storage with O(1) access
/// - Automatic detection of use-after-free scenarios
/// - Compact representation (configurable bit allocation)
/// - Type safety when used with W_DECLARE_HANDLE_TYPE
///
/// Template parameters allow customization of the index space vs. generation granularity:
/// - More instance bits = larger object arrays possible
/// - More generation bits = longer time before wraparound reuse
///
/// Typical configurations:
/// - WGenericId<24, 8>: 16M objects, 256 generations (good for most uses)
/// - WGenericId<16, 16>: 64K objects, 65K generations (for high-churn scenarios)
template <WUInt32 InstanceIndexBits, WUInt32 GenerationBits>
struct WGenericId
{
  enum
  {
    STORAGE_SIZE = ((InstanceIndexBits + GenerationBits - 1) / 8) + 1
  };
  using StorageType = typename WSizeToType<STORAGE_SIZE>::Type;

  W_DECLARE_ID_TYPE(WGenericId, InstanceIndexBits, GenerationBits);

  W_ALWAYS_INLINE WGenericId(StorageType instanceIndex, StorageType generation)
  {
    m_Data = 0;
    m_InstanceIndex = instanceIndex;
    m_Generation = generation;
  }

  union
  {
    StorageType m_Data;
    struct
    {
      StorageType m_InstanceIndex : InstanceIndexBits;
      StorageType m_Generation : GenerationBits;
    };
  };
};

#define W_DECLARE_HANDLE_TYPE(name, idType)               \
public:                                                    \
  W_DECLARE_POD_TYPE();                                   \
  W_ALWAYS_INLINE name() {}                               \
  W_ALWAYS_INLINE explicit name(idType internalId)        \
    : m_InternalId(internalId)                             \
  {                                                        \
  }                                                        \
  W_ALWAYS_INLINE bool operator==(const name other) const \
  {                                                        \
    return m_InternalId == other.m_InternalId;             \
  }                                                        \
  W_ALWAYS_INLINE bool operator!=(const name other) const \
  {                                                        \
    return m_InternalId != other.m_InternalId;             \
  }                                                        \
  W_ALWAYS_INLINE bool operator<(const name other) const  \
  {                                                        \
    return m_InternalId < other.m_InternalId;              \
  }                                                        \
  W_ALWAYS_INLINE void Invalidate()                       \
  {                                                        \
    m_InternalId.Invalidate();                             \
  }                                                        \
  W_ALWAYS_INLINE bool IsInvalidated() const              \
  {                                                        \
    return m_InternalId.IsInvalidated();                   \
  }                                                        \
  W_ALWAYS_INLINE idType GetInternalID() const            \
  {                                                        \
    return m_InternalId;                                   \
  }                                                        \
  using IdType = idType;                                   \
                                                           \
protected:                                                 \
  idType m_InternalId;                                     \
  operator idType()                                        \
  {                                                        \
    return m_InternalId;                                   \
  }                                                        \
  operator const idType() const                            \
  {                                                        \
    return m_InternalId;                                   \
  }
