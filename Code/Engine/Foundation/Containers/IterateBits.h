#pragma once

#include <Foundation/Containers/Implementation/BitIterator.h>

/// Helper base class to iterate over the bit indices or bit values of an integer.
/// \tparam DataType The type of data that is being iterated over.
/// \tparam ReturnsIndex If set, returns the index of the bit. Otherwise returns the value of the bit, i.e. W_BIT(value).
/// \tparam ReturnType Returned value type of the iterator.
/// \sa WIterateBitValues, WIterateBitIndices
template <typename DataType, bool ReturnsIndex, typename ReturnType = DataType>
struct WIterateBits
{
  explicit WIterateBits(DataType data)
  {
    m_Data = data;
  }

  WBitIterator<DataType, ReturnsIndex, ReturnType> begin() const
  {
    return WBitIterator<DataType, ReturnsIndex, ReturnType>(m_Data);
  };

  WBitIterator<DataType, ReturnsIndex, ReturnType> end() const
  {
    return WBitIterator<DataType, ReturnsIndex, ReturnType>();
  };

  DataType m_Data = {};
};

/// Helper class to iterate over the bit values of an integer.
/// The class can iterate over the bits of any unsigned integer type that is equal to or smaller than WUInt64.
/// \code{.cpp}
///    WUInt64 bits = 0b1101;
///    for (auto bit : WIterateBitValues(bits))
///    {
///      WLog::Info("{}", bit); // Outputs 1, 4, 8
///    }
/// \endcode
/// \tparam DataType The type of data that is being iterated over.
/// \tparam ReturnType Returned value type of the iterator. Defaults to same as DataType.
template <typename DataType, typename ReturnType = DataType>
struct WIterateBitValues : public WIterateBits<DataType, false, ReturnType>
{
  explicit WIterateBitValues(DataType data)
    : WIterateBits<DataType, false, ReturnType>(data)
  {
  }
};

/// Helper class to iterate over the bit indices of an integer.
/// The class can iterate over the bits of any unsigned integer type that is equal to or smaller than WUInt64.
/// \code{.cpp}
///    WUInt64 bits = 0b1101;
///    for (auto bit : WIterateBitIndices(bits))
///    {
///      WLog::Info("{}", bit); // Outputs 0, 2, 3
///    }
/// \endcode
/// \tparam DataType The type of data that is being iterated over.
/// \tparam ReturnType Returned value type of the iterator. Defaults to same as DataType.
template <typename DataType, typename ReturnType = DataType>
struct WIterateBitIndices : public WIterateBits<DataType, true, ReturnType>
{
  explicit WIterateBitIndices(DataType data)
    : WIterateBits<DataType, true, ReturnType>(data)
  {
  }
};
