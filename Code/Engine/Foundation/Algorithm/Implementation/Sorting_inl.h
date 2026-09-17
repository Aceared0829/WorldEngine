
template <typename Container, typename Comparer>
void WSorting::QuickSort(Container& inout_container, const Comparer& comparer)
{
  if (inout_container.IsEmpty())
    return;

  QuickSort(inout_container, 0, inout_container.GetCount() - 1, comparer);
}

template <typename T, typename Comparer>
void WSorting::QuickSort(WArrayPtr<T>& inout_arrayPtr, const Comparer& comparer)
{
  if (inout_arrayPtr.IsEmpty())
    return;

  QuickSort(inout_arrayPtr, 0, inout_arrayPtr.GetCount() - 1, comparer);
}

template <typename Container, typename Comparer>
void WSorting::InsertionSort(Container& inout_container, const Comparer& comparer)
{
  if (inout_container.IsEmpty())
    return;

  InsertionSort(inout_container, 0, inout_container.GetCount() - 1, comparer);
}

template <typename T, typename Comparer>
void WSorting::InsertionSort(WArrayPtr<T>& inout_arrayPtr, const Comparer& comparer)
{
  if (inout_arrayPtr.IsEmpty())
    return;

  InsertionSort(inout_arrayPtr, 0, inout_arrayPtr.GetCount() - 1, comparer);
}

template <typename Container, typename Comparer>
void WSorting::QuickSort(Container& inout_container, WUInt32 uiStartIndex, WUInt32 uiEndIndex, const Comparer& in_comparer)
{
  if (uiStartIndex < uiEndIndex)
  {
    if (uiEndIndex - uiStartIndex <= INSERTION_THRESHOLD)
    {
      InsertionSort(inout_container, uiStartIndex, uiEndIndex, in_comparer);
    }
    else
    {
      const WUInt32 uiPivotIndex = Partition(inout_container, uiStartIndex, uiEndIndex, in_comparer);

      WUInt32 uiFirstHalfEndIndex = uiPivotIndex > 0 ? uiPivotIndex - 1 : 0;
      WUInt32 uiSecondHalfStartIndex = uiPivotIndex + 1;

      while (uiFirstHalfEndIndex > uiStartIndex && !DoCompare(in_comparer, inout_container[uiFirstHalfEndIndex], inout_container[uiPivotIndex]))
      {
        uiFirstHalfEndIndex--;
      }

      while (uiSecondHalfStartIndex <= uiEndIndex && !DoCompare(in_comparer, inout_container[uiPivotIndex], inout_container[uiSecondHalfStartIndex]))
      {
        uiSecondHalfStartIndex++;
      }

      if (uiStartIndex < uiFirstHalfEndIndex)
        QuickSort(inout_container, uiStartIndex, uiFirstHalfEndIndex, in_comparer);

      if (uiSecondHalfStartIndex < uiEndIndex)
        QuickSort(inout_container, uiSecondHalfStartIndex, uiEndIndex, in_comparer);
    }
  }
}

template <typename Container, typename Comparer>
WUInt32 WSorting::Partition(Container& inout_container, WUInt32 uiLeft, WUInt32 uiRight, const Comparer& comparer)
{
  WUInt32 uiPivotIndex = (uiLeft + uiRight) / 2;

  if (DoCompare(comparer, inout_container[uiLeft], inout_container[uiRight]))
  {
    // left < right

    if (DoCompare(comparer, inout_container[uiRight], inout_container[uiPivotIndex]))
    {
      // left < right < pivot
      uiPivotIndex = uiRight;
    }
    else if (DoCompare(comparer, inout_container[uiLeft], inout_container[uiPivotIndex]))
    {
      // left < pivot < right
    }
    else
    {
      // pivot < left < right
      uiPivotIndex = uiLeft;
    }
  }
  else
  {
    // right < left

    if (DoCompare(comparer, inout_container[uiLeft], inout_container[uiPivotIndex]))
    {
      uiPivotIndex = uiLeft; // right < left < pivot
    }
    else if (DoCompare(comparer, inout_container[uiRight], inout_container[uiPivotIndex]))
    {
      // right < pivot < left
    }
    else
    {
      // pivot < right < left
      uiPivotIndex = uiRight;
    }
  }

  WMath::Swap(inout_container[uiPivotIndex], inout_container[uiRight]); // move pivot to right

  WUInt32 uiIndex = uiLeft;
  for (WUInt32 i = uiLeft; i < uiRight; ++i)
  {
    if (DoCompare(comparer, inout_container[i], inout_container[uiRight]))
    {
      WMath::Swap(inout_container[i], inout_container[uiIndex]);
      ++uiIndex;
    }
  }

  WMath::Swap(inout_container[uiIndex], inout_container[uiRight]); // move pivot back in place

  return uiIndex;
}


template <typename T, typename Comparer>
void WSorting::QuickSort(WArrayPtr<T>& inout_arrayPtr, WUInt32 uiStartIndex, WUInt32 uiEndIndex, const Comparer& comparer)
{
  T* ptr = inout_arrayPtr.GetPtr();

  if (uiStartIndex < uiEndIndex)
  {
    if (uiEndIndex - uiStartIndex <= INSERTION_THRESHOLD)
    {
      InsertionSort(inout_arrayPtr, uiStartIndex, uiEndIndex, comparer);
    }
    else
    {
      const WUInt32 uiPivotIndex = Partition(ptr, uiStartIndex, uiEndIndex, comparer);

      WUInt32 uiFirstHalfEndIndex = uiPivotIndex > 0 ? uiPivotIndex - 1 : 0;
      WUInt32 uiSecondHalfStartIndex = uiPivotIndex + 1;

      while (uiFirstHalfEndIndex > uiStartIndex && !DoCompare(comparer, ptr[uiFirstHalfEndIndex], ptr[uiPivotIndex]))
      {
        uiFirstHalfEndIndex--;
      }

      while (uiSecondHalfStartIndex <= uiEndIndex && !DoCompare(comparer, ptr[uiPivotIndex], ptr[uiSecondHalfStartIndex]))
      {
        uiSecondHalfStartIndex++;
      }

      if (uiStartIndex < uiFirstHalfEndIndex)
        QuickSort(inout_arrayPtr, uiStartIndex, uiFirstHalfEndIndex, comparer);

      if (uiSecondHalfStartIndex < uiEndIndex)
        QuickSort(inout_arrayPtr, uiSecondHalfStartIndex, uiEndIndex, comparer);
    }
  }
}

template <typename T, typename Comparer>
WUInt32 WSorting::Partition(T* pPtr, WUInt32 uiLeft, WUInt32 uiRight, const Comparer& comparer)
{
  WUInt32 uiPivotIndex = (uiLeft + uiRight) / 2;

  if (DoCompare(comparer, pPtr[uiLeft], pPtr[uiRight]))
  {
    // left < right

    if (DoCompare(comparer, pPtr[uiRight], pPtr[uiPivotIndex]))
    {
      // left < right < pivot
      uiPivotIndex = uiRight;
    }
    else if (DoCompare(comparer, pPtr[uiLeft], pPtr[uiPivotIndex]))
    {
      // left < pivot < right
    }
    else
    {
      // pivot < left < right
      uiPivotIndex = uiLeft;
    }
  }
  else
  {
    // right < left

    if (DoCompare(comparer, pPtr[uiLeft], pPtr[uiPivotIndex]))
    {
      uiPivotIndex = uiLeft; // right < left < pivot
    }
    else if (DoCompare(comparer, pPtr[uiRight], pPtr[uiPivotIndex]))
    {
      // right < pivot < left
    }
    else
    {
      // pivot < right < left
      uiPivotIndex = uiRight;
    }
  }

  WMath::Swap(pPtr[uiPivotIndex], pPtr[uiRight]); // move pivot to right

  WUInt32 uiIndex = uiLeft;
  for (WUInt32 i = uiLeft; i < uiRight; ++i)
  {
    if (DoCompare(comparer, pPtr[i], pPtr[uiRight]))
    {
      WMath::Swap(pPtr[i], pPtr[uiIndex]);
      ++uiIndex;
    }
  }

  WMath::Swap(pPtr[uiIndex], pPtr[uiRight]); // move pivot back in place

  return uiIndex;
}


template <typename Container, typename Comparer>
void WSorting::InsertionSort(Container& inout_container, WUInt32 uiStartIndex, WUInt32 uiEndIndex, const Comparer& comparer)
{
  for (WUInt32 i = uiStartIndex + 1; i <= uiEndIndex; ++i)
  {
    WUInt32 uiHoleIndex = i;
    while (uiHoleIndex > uiStartIndex && DoCompare(comparer, inout_container[uiHoleIndex], inout_container[uiHoleIndex - 1]))
    {
      WMath::Swap(inout_container[uiHoleIndex], inout_container[uiHoleIndex - 1]);
      --uiHoleIndex;
    }
  }
}

template <typename T, typename Comparer>
void WSorting::InsertionSort(WArrayPtr<T>& inout_arrayPtr, WUInt32 uiStartIndex, WUInt32 uiEndIndex, const Comparer& comparer)
{
  T* ptr = inout_arrayPtr.GetPtr();

  for (WUInt32 i = uiStartIndex + 1; i <= uiEndIndex; ++i)
  {
    WUInt32 uiHoleIndex = i;
    T valueToInsert = std::move(ptr[uiHoleIndex]);

    while (uiHoleIndex > uiStartIndex && DoCompare(comparer, valueToInsert, ptr[uiHoleIndex - 1]))
    {
      --uiHoleIndex;
    }

    const WUInt32 uiMoveCount = i - uiHoleIndex;
    if (uiMoveCount > 0)
    {
      WMemoryUtils::RelocateOverlapped(ptr + uiHoleIndex + 1, ptr + uiHoleIndex, uiMoveCount);
      WMemoryUtils::MoveConstruct(ptr + uiHoleIndex, std::move(valueToInsert));
    }
    else
    {
      ptr[uiHoleIndex] = std::move(valueToInsert);
    }
  }
}
