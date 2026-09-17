#pragma once

#include <Foundation/Basics.h>

/// Space-efficient pointer storage that embeds flags in unused low-order bits
///
/// Exploits the fact that aligned pointers have zero bits in their lower positions to store
/// additional flag data without increasing memory usage. The number of available bits depends
/// on alignment: 8-byte aligned pointers provide 3 bits, 4-byte aligned provide 2 bits, etc.
///
/// When accessing the pointer, the flag bits are automatically masked off to yield the original
/// pointer value. Flag operations are independent of pointer operations, allowing separate
/// manipulation of the embedded data.
///
/// Common use cases include storing object state flags alongside pointers in data structures,
/// reducing memory overhead in pointer-heavy applications like trees or linked lists.
template <typename PtrType, WUInt8 NumFlagBits = 2>
class WPointerWithFlags
{
private:
  enum : size_t
  {
    AllOnes = (std::size_t)(-1),
    PtrBits = sizeof(void*) * 8,
    FlagsMask = (AllOnes >> (PtrBits - NumFlagBits)),
    PtrMask = ~FlagsMask,
  };

  void* m_pPtr = nullptr;

public:
  /// Initializes the pointer and flags with zero
  WPointerWithFlags() = default;

  /// Initializes the pointer and flags
  explicit WPointerWithFlags(PtrType* pPtr, WUInt8 uiFlags = 0) { SetPtrAndFlags(pPtr, uiFlags); }

  /// Changes the pointer and flags
  void SetPtrAndFlags(PtrType* pPtr, WUInt8 uiFlags)
  {
    const std::uintptr_t isrc = *reinterpret_cast<std::uintptr_t*>(&pPtr);
    std::uintptr_t& iptr = *reinterpret_cast<std::uintptr_t*>(&m_pPtr);

    iptr = (isrc & PtrMask) | (uiFlags & FlagsMask);
  }

  /// Returns the masked off pointer value
  const PtrType* GetPtr() const
  {
    const std::uintptr_t& iptr = *reinterpret_cast<const std::uintptr_t*>(&m_pPtr);
    return reinterpret_cast<const PtrType*>(iptr & PtrMask); // mask off lower N bits
  }

  /// Returns the masked off pointer value
  PtrType* GetPtr()
  {
    std::uintptr_t& iptr = *reinterpret_cast<std::uintptr_t*>(&m_pPtr);
    return reinterpret_cast<PtrType*>(iptr & PtrMask); // mask off lower N bits
  }

  /// Changes the pointer value only. Flags stay unchanged.
  void SetPtr(PtrType* pPtr)
  {
    const std::uintptr_t isrc = *reinterpret_cast<std::uintptr_t*>(&pPtr);
    W_ASSERT_DEBUG(
      (isrc & FlagsMask) == 0, "The given pointer does not have an {} byte alignment and thus cannot be stored lossless.", 1u << NumFlagBits);

    std::uintptr_t& iptr = *reinterpret_cast<std::uintptr_t*>(&m_pPtr);

    iptr = (isrc & PtrMask) | (iptr & FlagsMask);
  }
  /// Returns the flags value only
  WUInt8 GetFlags() const
  {
    const std::uintptr_t& iptr = *reinterpret_cast<const std::uintptr_t*>(&m_pPtr);
    return static_cast<WUInt8>(iptr & FlagsMask);
  }

  /// Changes only the flags value. The given value must fit into the reserved bits.
  void SetFlags(WUInt8 uiFlags)
  {
    W_ASSERT_DEBUG(uiFlags <= FlagsMask, "The flag value {} requires more than {} bits", uiFlags, NumFlagBits);

    std::uintptr_t& iptr = *reinterpret_cast<std::uintptr_t*>(&m_pPtr);

    iptr = (iptr & PtrMask) | (uiFlags & FlagsMask);
  }

  /// Returns the masked off pointer value
  operator PtrType*() { return GetPtr(); }

  /// Returns the masked off pointer value
  operator const PtrType*() const { return GetPtr(); }

  /// Changes the pointer value only. Flags stay unchanged.
  void operator=(PtrType* pPtr) { SetPtr(pPtr); }

  /// Compares the pointer part for equality (flags are ignored)
  template <typename = typename std::enable_if<std::is_const<PtrType>::value == false>>
  bool operator==(const PtrType* pPtr) const
  {
    return GetPtr() == pPtr;
  }

#if W_DISABLED(W_USE_CPP20_OPERATORS)
  /// Compares the pointer part for inequality (flags are ignored)
  template <typename = typename std::enable_if<std::is_const<PtrType>::value == false>>
  bool operator!=(const PtrType* pPtr) const
  {
    return !(*this == pPtr);
  }
#endif

  bool operator==(const WPointerWithFlags<PtrType, NumFlagBits>& rhs) const
  {
    return GetPtr() == rhs.GetPtr();
  }

  W_ADD_DEFAULT_OPERATOR_NOTEQUAL(const WPointerWithFlags<PtrType, NumFlagBits>&);

  /// Compares the pointer part for equality (flags are ignored)
  bool operator==(PtrType* pPtr) const { return GetPtr() == pPtr; }
  W_ADD_DEFAULT_OPERATOR_NOTEQUAL(PtrType*);

  /// Compares the pointer part for equality (flags are ignored)
  bool operator==(std::nullptr_t) const { return GetPtr() == nullptr; }
  W_ADD_DEFAULT_OPERATOR_NOTEQUAL(std::nullptr_t);

  /// Checks whether the pointer part is not nullptr (flags are ignored)
  explicit operator bool() const { return GetPtr() != nullptr; }

  /// Dereferences the pointer
  const PtrType* operator->() const { return GetPtr(); }

  /// Dereferences the pointer
  PtrType* operator->() { return GetPtr(); }

  /// Dereferences the pointer
  const PtrType& operator*() const { return *GetPtr(); }

  /// Dereferences the pointer
  PtrType& operator*() { return *GetPtr(); }
};
