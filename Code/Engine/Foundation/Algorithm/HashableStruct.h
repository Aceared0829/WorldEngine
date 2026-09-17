
#pragma once

#include <Foundation/Basics.h>


/// This class provides a base class for hashable structs (e.g. descriptor objects).
///
/// To help with this there are two parts:
///   1) memclear on initialization.
///   2) a CalculateHash() function calculating the 32 bit hash of the object.
///
/// You can make your own struct hashable by deriving from WHashableStruct providing the type of
/// your class / struct as the template parameter.
template <typename DERIVED>
class WHashableStruct
{
public:
  WHashableStruct();                                       // [tested]
  WHashableStruct(const WHashableStruct<DERIVED>& other); // [tested]

  void operator=(const WHashableStruct<DERIVED>& other);   // [tested]
  bool operator==(const WHashableStruct<DERIVED>& other) const;
  bool operator!=(const WHashableStruct<DERIVED>& other) const;
  bool operator<(const WHashableStruct<DERIVED>& other) const;

  /// Calculates the 32 bit hash of the struct and returns it
  WUInt32 CalculateHash() const; // [tested]
};

#include <Foundation/Algorithm/Implementation/HashableStruct_inl.h>
