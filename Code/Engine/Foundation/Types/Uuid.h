
#pragma once

#include <Foundation/Algorithm/HashingUtils.h>

class WStreamReader;
class WStreamWriter;

/// 128-bit Universally Unique Identifier (UUID/GUID) for object identification and referencing.
///
/// WUuid provides a robust way to uniquely identify objects, assets, or entities across systems,
/// time, and network boundaries. It's essential for serialization, asset management, networking,
/// and any scenario where objects need stable, globally unique identities.
class W_FOUNDATION_DLL WUuid
{
public:
  W_DECLARE_POD_TYPE();

  /// Default constructor. Constructed Uuid will be invalid.
  W_ALWAYS_INLINE WUuid() = default; // [tested]

  /// Constructs the Uuid from existing values
  W_ALWAYS_INLINE constexpr WUuid(WUInt64 uiLow, WUInt64 uiHigh)
    : m_uiHigh(uiHigh)
    , m_uiLow(uiLow)
  {
  }

  /// Comparison operator. [tested]
  W_ALWAYS_INLINE bool operator==(const WUuid& other) const;

  /// Comparison operator. [tested]
  W_ALWAYS_INLINE bool operator!=(const WUuid& other) const;

  /// Comparison operator.
  W_ALWAYS_INLINE bool operator<(const WUuid& other) const;

  /// Returns true if this is a valid Uuid.
  W_ALWAYS_INLINE bool IsValid() const;

  /// Returns an invalid UUID.
  [[nodiscard]] W_ALWAYS_INLINE static WUuid MakeInvalid() { return WUuid(0, 0); }

  /// Returns a new Uuid.
  [[nodiscard]] static WUuid MakeUuid();

  /// Returns the internal 128 Bit of data
  void GetValues(WUInt64& ref_uiLow, WUInt64& ref_uiHigh) const
  {
    ref_uiHigh = m_uiHigh;
    ref_uiLow = m_uiLow;
  }

  /// Creates a uuid from a string. The result is always the same for the same string.
  [[nodiscard]] static WUuid MakeStableUuidFromString(WStringView sString);

  /// Creates a uuid from an integer. The result is always the same for the same input.
  [[nodiscard]] static WUuid MakeStableUuidFromInt(WInt64 iInt);

  /// Adds the given seed value to this guid, creating a new guid. The process is reversible.
  W_ALWAYS_INLINE void CombineWithSeed(const WUuid& seed);

  /// Subtracts the given seed from this guid, restoring the original guid.
  W_ALWAYS_INLINE void RevertCombinationWithSeed(const WUuid& seed);

  /// Combines two guids using hashing, irreversible and order dependent.
  W_ALWAYS_INLINE void HashCombine(const WUuid& hash);

private:
  friend W_FOUNDATION_DLL_FRIEND void operator>>(WStreamReader& inout_stream, WUuid& ref_value);
  friend W_FOUNDATION_DLL_FRIEND void operator<<(WStreamWriter& inout_stream, const WUuid& value);

  WUInt64 m_uiHigh = 0;
  WUInt64 m_uiLow = 0;
};

#include <Foundation/Types/Implementation/Uuid_inl.h>
