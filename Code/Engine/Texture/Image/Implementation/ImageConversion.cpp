#include <Texture/TexturePCH.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageConversion.h>

W_ENUMERABLE_CLASS_IMPLEMENTATION(WImageConversionStep);

namespace
{
  struct TableEntry
  {
    TableEntry() = default;

    TableEntry(const WImageConversionStep* pStep, const WImageConversionEntry& entry)
    {
      m_step = pStep;
      m_sourceFormat = entry.m_sourceFormat;
      m_targetFormat = entry.m_targetFormat;
      m_numChannels = WMath::Min(WImageFormat::GetNumChannels(entry.m_sourceFormat), WImageFormat::GetNumChannels(entry.m_targetFormat));

      float sourceBpp = WImageFormat::GetExactBitsPerPixel(m_sourceFormat);
      float targetBpp = WImageFormat::GetExactBitsPerPixel(m_targetFormat);

      m_flags = entry.m_flags;

      // Base cost is amount of bits processed
      m_cost = sourceBpp + targetBpp;

      // Penalty for non-inplace conversion
      if ((m_flags & WImageConversionFlags::InPlace) == 0)
      {
        m_cost *= 2;
      }

      // Penalize formats that aren't aligned to powers of two
      if (!WImageFormat::IsCompressed(m_sourceFormat) && !WImageFormat::IsCompressed(m_targetFormat))
      {
        auto sourceBppInt = static_cast<WUInt32>(sourceBpp);
        auto targetBppInt = static_cast<WUInt32>(targetBpp);
        if (!WMath::IsPowerOf2(sourceBppInt) || !WMath::IsPowerOf2(targetBppInt))
        {
          m_cost *= 2;
        }
      }

      m_cost += entry.m_fAdditionalPenalty;
    }

    const WImageConversionStep* m_step = nullptr;
    WImageFormat::Enum m_sourceFormat = WImageFormat::UNKNOWN;
    WImageFormat::Enum m_targetFormat = WImageFormat::UNKNOWN;
    WBitflags<WImageConversionFlags> m_flags;
    float m_cost = WMath::MaxValue<float>();
    WUInt32 m_numChannels = 0;

    static TableEntry chain(const TableEntry& a, const TableEntry& b)
    {
      if (WImageFormat::GetExactBitsPerPixel(a.m_sourceFormat) > WImageFormat::GetExactBitsPerPixel(a.m_targetFormat) &&
          WImageFormat::GetExactBitsPerPixel(b.m_sourceFormat) < WImageFormat::GetExactBitsPerPixel(b.m_targetFormat))
      {
        // Disallow chaining conversions which first reduce to a smaller intermediate and then go back to a larger one, since
        // we end up throwing away information.
        return {};
      }

      TableEntry entry;
      entry.m_step = a.m_step;
      entry.m_cost = a.m_cost + b.m_cost;
      entry.m_sourceFormat = a.m_sourceFormat;
      entry.m_targetFormat = a.m_targetFormat;
      entry.m_flags = a.m_flags;
      entry.m_numChannels = WMath::Min(a.m_numChannels, b.m_numChannels);
      return entry;
    }

    bool operator<(const TableEntry& other) const
    {
      if (m_numChannels > other.m_numChannels)
        return true;

      if (m_numChannels < other.m_numChannels)
        return false;

      return m_cost < other.m_cost;
    }

    bool isAdmissible() const
    {
      if (m_numChannels == 0)
        return false;

      return m_cost < WMath::MaxValue<float>();
    }
  };

  WMutex s_conversionTableLock;
  WHashTable<WUInt32, TableEntry> s_conversionTable;
  bool s_conversionTableValid = false;

  constexpr WUInt32 MakeKey(WImageFormat::Enum a, WImageFormat::Enum b)
  {
    return a * WImageFormat::NUM_FORMATS + b;
  }
  constexpr WUInt32 MakeTypeKey(WImageFormatType::Enum a, WImageFormatType::Enum b)
  {
    return (a << 16) + b;
  }

  struct IntermediateBuffer
  {
    IntermediateBuffer(WUInt32 uiBitsPerBlock)
      : m_bitsPerBlock(uiBitsPerBlock)
    {
    }
    WUInt32 m_bitsPerBlock;
  };

  WUInt32 allocateScratchBufferIndex(WDynamicArray<IntermediateBuffer>& ref_scratchBuffers, WUInt32 uiBitsPerBlock, WUInt32 uiExcludedIndex)
  {
    int foundIndex = -1;

    for (WUInt32 bufferIndex = 0; bufferIndex < WUInt32(ref_scratchBuffers.GetCount()); ++bufferIndex)
    {
      if (bufferIndex == uiExcludedIndex)
      {
        continue;
      }

      if (ref_scratchBuffers[bufferIndex].m_bitsPerBlock == uiBitsPerBlock)
      {
        foundIndex = bufferIndex;
        break;
      }
    }

    if (foundIndex >= 0)
    {
      // Reuse existing scratch buffer
      return foundIndex;
    }
    else
    {
      // Allocate new scratch buffer
      ref_scratchBuffers.PushBack(IntermediateBuffer(uiBitsPerBlock));
      return ref_scratchBuffers.GetCount() - 1;
    }
  }
} // namespace

WImageConversionStep::WImageConversionStep()
{
  s_conversionTableValid = false;
}

WImageConversionStep::~WImageConversionStep()
{
  s_conversionTableValid = false;
}

WResult WImageConversion::BuildPath(WImageFormat::Enum sourceFormat, WImageFormat::Enum targetFormat, bool bSourceEqualsTarget, WDynamicArray<WImageConversion::ConversionPathNode>& out_path, WUInt32& out_uiNumScratchBuffers)
{
  W_LOCK(s_conversionTableLock);

  out_path.Clear();
  out_uiNumScratchBuffers = 0;

  if (sourceFormat == targetFormat)
  {
    ConversionPathNode node;
    node.m_sourceFormat = sourceFormat;
    node.m_targetFormat = targetFormat;
    node.m_inPlace = bSourceEqualsTarget;
    node.m_sourceBufferIndex = 0;
    node.m_targetBufferIndex = 0;
    node.m_step = nullptr;
    out_path.PushBack(node);
    return W_SUCCESS;
  }

  if (!s_conversionTableValid)
  {
    RebuildConversionTable();
  }

  for (WImageFormat::Enum current = sourceFormat; current != targetFormat;)
  {
    WUInt32 currentTableIndex = MakeKey(current, targetFormat);

    TableEntry entry;

    if (!s_conversionTable.TryGetValue(currentTableIndex, entry))
    {
      return W_FAILURE;
    }

    WImageConversion::ConversionPathNode step;
    step.m_sourceFormat = entry.m_sourceFormat;
    step.m_targetFormat = entry.m_targetFormat;
    step.m_inPlace = entry.m_flags.IsAnySet(WImageConversionFlags::InPlace);
    step.m_step = entry.m_step;

    current = entry.m_targetFormat;

    out_path.PushBack(step);
  }

  WTempHybridArray<IntermediateBuffer, 16> scratchBuffers;
  scratchBuffers.PushBack(IntermediateBuffer(WImageFormat::GetBitsPerBlock(targetFormat)));

  const int iLastPathIndex = out_path.GetCount() - 1;
  for (int i = iLastPathIndex; i >= 0; --i)
  {
    if (i == iLastPathIndex)
      out_path[i].m_targetBufferIndex = 0;
    else
      out_path[i].m_targetBufferIndex = out_path[i + 1].m_sourceBufferIndex;

    if (i > 0)
    {
      if (out_path[i].m_inPlace)
      {
        out_path[i].m_sourceBufferIndex = out_path[i].m_targetBufferIndex;
      }
      else
      {
        WUInt32 bitsPerBlock = WImageFormat::GetBitsPerBlock(out_path[i].m_sourceFormat);

        out_path[i].m_sourceBufferIndex = allocateScratchBufferIndex(scratchBuffers, bitsPerBlock, out_path[i].m_targetBufferIndex);
      }
    }
  }

  if (bSourceEqualsTarget)
  {
    // Enforce constraint that source == target
    out_path[0].m_sourceBufferIndex = 0;

    // Did we accidentally break the in-place invariant?
    if (out_path[0].m_sourceBufferIndex == out_path[0].m_targetBufferIndex && !out_path[0].m_inPlace)
    {
      if (out_path.GetCount() == 1)
      {
        // Only a single step, so we need to add a copy step
        WImageConversion::ConversionPathNode copy;
        copy.m_inPlace = false;
        copy.m_sourceFormat = sourceFormat;
        copy.m_targetFormat = sourceFormat;
        copy.m_sourceBufferIndex = out_path[0].m_sourceBufferIndex;
        copy.m_targetBufferIndex =
          allocateScratchBufferIndex(scratchBuffers, WImageFormat::GetBitsPerBlock(out_path[0].m_sourceFormat), out_path[0].m_sourceBufferIndex);
        out_path[0].m_sourceBufferIndex = copy.m_targetBufferIndex;
        copy.m_step = nullptr;
        out_path.InsertAt(0, copy);
      }
      else
      {
        // Turn second step to non-inplace
        out_path[1].m_inPlace = false;
        out_path[1].m_sourceBufferIndex =
          allocateScratchBufferIndex(scratchBuffers, WImageFormat::GetBitsPerBlock(out_path[1].m_sourceFormat), out_path[0].m_sourceBufferIndex);
        out_path[0].m_targetBufferIndex = out_path[1].m_sourceBufferIndex;
      }
    }
  }
  else
  {
    out_path[0].m_sourceBufferIndex = scratchBuffers.GetCount();
  }

  out_uiNumScratchBuffers = scratchBuffers.GetCount() - 1;

  return W_SUCCESS;
}

void WImageConversion::RebuildConversionTable()
{
  W_LOCK(s_conversionTableLock);

  s_conversionTable.Clear();

  // Prime conversion table with known conversions
  for (WImageConversionStep* conversion = WImageConversionStep::GetFirstInstance(); conversion; conversion = conversion->GetNextInstance())
  {
    WArrayPtr<const WImageConversionEntry> entries = conversion->GetSupportedConversions();

    for (WUInt32 subIndex = 0; subIndex < (WUInt32)entries.GetCount(); subIndex++)
    {
      const WImageConversionEntry& subConversion = entries[subIndex];

      if (subConversion.m_flags.IsAnySet(WImageConversionFlags::InPlace))
      {
        W_ASSERT_DEV(WImageFormat::IsCompressed(subConversion.m_sourceFormat) == WImageFormat::IsCompressed(subConversion.m_targetFormat) &&
                        WImageFormat::GetBitsPerBlock(subConversion.m_sourceFormat) == WImageFormat::GetBitsPerBlock(subConversion.m_targetFormat),
          "In-place conversions are only allowed between formats of the same number of bits per pixel and compressedness");
      }

      if (WImageFormat::GetType(subConversion.m_sourceFormat) == WImageFormatType::PLANAR)
      {
        W_ASSERT_DEV(WImageFormat::GetType(subConversion.m_targetFormat) == WImageFormatType::LINEAR, "Conversions from planar formats must target linear formats");
      }
      else if (WImageFormat::GetType(subConversion.m_targetFormat) == WImageFormatType::PLANAR)
      {
        W_ASSERT_DEV(WImageFormat::GetType(subConversion.m_sourceFormat) == WImageFormatType::LINEAR, "Conversions to planar formats must sourced from linear formats");
      }

      WUInt32 tableIndex = MakeKey(subConversion.m_sourceFormat, subConversion.m_targetFormat);

      // Use the cheapest known conversion for each combination in case there are multiple ones
      TableEntry candidate(conversion, subConversion);

      TableEntry existing;

      if (!s_conversionTable.TryGetValue(tableIndex, existing) || candidate < existing)
      {
        s_conversionTable.Insert(tableIndex, candidate);
      }
    }
  }

  for (WUInt32 i = 0; i < WImageFormat::NUM_FORMATS; i++)
  {
    const WImageFormat::Enum format = static_cast<WImageFormat::Enum>(i);
    // Add copy-conversion (from and to same format)
    s_conversionTable.Insert(
      MakeKey(format, format), TableEntry(nullptr, WImageConversionEntry(WImageConversionEntry(format, format, WImageConversionFlags::InPlace))));
  }

  // Straight from http://en.wikipedia.org/wiki/Floyd-Warshall_algorithm
  for (WUInt32 k = 1; k < WImageFormat::NUM_FORMATS; k++)
  {
    for (WUInt32 i = 1; i < WImageFormat::NUM_FORMATS; i++)
    {
      if (k == i)
      {
        continue;
      }

      WUInt32 tableIndexIK = MakeKey(static_cast<WImageFormat::Enum>(i), static_cast<WImageFormat::Enum>(k));

      TableEntry entryIK;
      if (!s_conversionTable.TryGetValue(tableIndexIK, entryIK))
      {
        continue;
      }

      for (WUInt32 j = 1; j < WImageFormat::NUM_FORMATS; j++)
      {
        if (j == i || j == k)
        {
          continue;
        }

        WUInt32 tableIndexIJ = MakeKey(static_cast<WImageFormat::Enum>(i), static_cast<WImageFormat::Enum>(j));
        WUInt32 tableIndexKJ = MakeKey(static_cast<WImageFormat::Enum>(k), static_cast<WImageFormat::Enum>(j));

        TableEntry entryKJ;
        if (!s_conversionTable.TryGetValue(tableIndexKJ, entryKJ))
        {
          continue;
        }

        TableEntry candidate = TableEntry::chain(entryIK, entryKJ);

        TableEntry existing;
        if (candidate.isAdmissible() && candidate < s_conversionTable[tableIndexIJ])
        {
          // To Convert from format I to format J, first Convert from I to K
          s_conversionTable[tableIndexIJ] = candidate;
        }
      }
    }
  }

  s_conversionTableValid = true;
}

WResult WImageConversion::Convert(const WImageView& source, WImage& ref_target, WImageFormat::Enum targetFormat)
{
  W_PROFILE_SCOPE("WImageConversion::Convert");

  WImageFormat::Enum sourceFormat = source.GetImageFormat();

  // Trivial copy
  if (sourceFormat == targetFormat)
  {
    if (&source != &ref_target)
    {
      // copy if not already the same
      ref_target.ResetAndCopy(source);
    }
    return W_SUCCESS;
  }

  WTempHybridArray<ConversionPathNode, 16> path;
  WUInt32 numScratchBuffers = 0;
  if (BuildPath(sourceFormat, targetFormat, &source == &ref_target, path, numScratchBuffers).Failed())
  {
    return W_FAILURE;
  }

  return Convert(source, ref_target, path, numScratchBuffers);
}

WResult WImageConversion::Convert(const WImageView& source, WImage& ref_target, WArrayPtr<ConversionPathNode> path, WUInt32 uiNumScratchBuffers)
{
  W_ASSERT_DEV(path.GetCount() > 0, "Invalid conversion path");
  W_ASSERT_DEV(path[0].m_sourceFormat == source.GetImageFormat(), "Invalid conversion path");

  WTempHybridArray<WImage, 16> intermediates;
  intermediates.SetCount(uiNumScratchBuffers);

  const WImageView* pSource = &source;

  for (WUInt32 i = 0; i < path.GetCount(); ++i)
  {
    WUInt32 targetIndex = path[i].m_targetBufferIndex;

    WImage* pTarget = targetIndex == 0 ? &ref_target : &intermediates[targetIndex - 1];

    if (ConvertSingleStep(path[i].m_step, *pSource, *pTarget, path[i].m_targetFormat).Failed())
    {
      return W_FAILURE;
    }

    pSource = pTarget;
  }

  return W_SUCCESS;
}

WResult WImageConversion::ConvertRaw(
  WConstByteBlobPtr source, WByteBlobPtr target, WUInt32 uiNumElements, WImageFormat::Enum sourceFormat, WImageFormat::Enum targetFormat)
{
  if (uiNumElements == 0)
  {
    return W_SUCCESS;
  }

  // Trivial copy
  if (sourceFormat == targetFormat)
  {
    if (target.GetPtr() != source.GetPtr())
      memcpy(target.GetPtr(), source.GetPtr(), uiNumElements * WUInt64(WImageFormat::GetBitsPerPixel(sourceFormat)) / 8);
    return W_SUCCESS;
  }

  if (WImageFormat::IsCompressed(sourceFormat) || WImageFormat::IsCompressed(targetFormat))
  {
    return W_FAILURE;
  }

  WTempHybridArray<ConversionPathNode, 16> path;
  WUInt32 numScratchBuffers;
  if (BuildPath(sourceFormat, targetFormat, source.GetPtr() == target.GetPtr(), path, numScratchBuffers).Failed())
  {
    return W_FAILURE;
  }

  return ConvertRaw(source, target, uiNumElements, path, numScratchBuffers);
}

WResult WImageConversion::ConvertRaw(
  WConstByteBlobPtr source, WByteBlobPtr target, WUInt32 uiNumElements, WArrayPtr<ConversionPathNode> path, WUInt32 uiNumScratchBuffers)
{
  W_ASSERT_DEV(path.GetCount() > 0, "Path of length 0 is invalid.");

  if (uiNumElements == 0)
  {
    return W_SUCCESS;
  }

  if (WImageFormat::IsCompressed(path.GetPtr()->m_sourceFormat) || WImageFormat::IsCompressed((path.GetEndPtr() - 1)->m_targetFormat))
  {
    return W_FAILURE;
  }

  WTempHybridArray<WBlob, 16> intermediates;
  intermediates.SetCount(uiNumScratchBuffers);

  for (WUInt32 i = 0; i < path.GetCount(); ++i)
  {
    WUInt32 targetIndex = path[i].m_targetBufferIndex;
    WUInt32 targetBpp = WImageFormat::GetBitsPerPixel(path[i].m_targetFormat);

    WByteBlobPtr stepTarget;
    if (targetIndex == 0)
    {
      stepTarget = target;
    }
    else
    {
      WUInt32 expectedSize = static_cast<WUInt32>(targetBpp * uiNumElements / 8);
      intermediates[targetIndex - 1].SetCountUninitialized(expectedSize);
      stepTarget = intermediates[targetIndex - 1].GetByteBlobPtr();
    }

    if (path[i].m_step == nullptr)
    {
      memcpy(stepTarget.GetPtr(), source.GetPtr(), uiNumElements * targetBpp / 8);
    }
    else
    {
      if (static_cast<const WImageConversionStepLinear*>(path[i].m_step)
            ->ConvertPixels(source, stepTarget, uiNumElements, path[i].m_sourceFormat, path[i].m_targetFormat)
            .Failed())
      {
        return W_FAILURE;
      }
    }

    source = stepTarget;
  }

  return W_SUCCESS;
}

WResult WImageConversion::ConvertSingleStep(
  const WImageConversionStep* pStep, const WImageView& source, WImage& target, WImageFormat::Enum targetFormat)
{
  if (!pStep)
  {
    target.ResetAndCopy(source);
    return W_SUCCESS;
  }

  WImageFormat::Enum sourceFormat = source.GetImageFormat();

  WImageHeader header = source.GetHeader();
  header.SetImageFormat(targetFormat);
  target.ResetAndAlloc(header);

  switch (MakeTypeKey(WImageFormat::GetType(sourceFormat), WImageFormat::GetType(targetFormat)))
  {
    case MakeTypeKey(WImageFormatType::LINEAR, WImageFormatType::LINEAR):
    {
      // we have to do the computation in 64-bit otherwise it might overflow for very large textures (8k x 4k or bigger).
      WUInt64 numElements = WUInt64(8) * target.GetByteBlobPtr().GetCount() / (WUInt64)WImageFormat::GetBitsPerPixel(targetFormat);
      return static_cast<const WImageConversionStepLinear*>(pStep)->ConvertPixels(
        source.GetByteBlobPtr(), target.GetByteBlobPtr(), (WUInt32)numElements, sourceFormat, targetFormat);
    }

    case MakeTypeKey(WImageFormatType::LINEAR, WImageFormatType::BLOCK_COMPRESSED):
      return ConvertSingleStepCompress(source, target, sourceFormat, targetFormat, pStep);

    case MakeTypeKey(WImageFormatType::LINEAR, WImageFormatType::PLANAR):
      return ConvertSingleStepPlanarize(source, target, sourceFormat, targetFormat, pStep);

    case MakeTypeKey(WImageFormatType::BLOCK_COMPRESSED, WImageFormatType::LINEAR):
      return ConvertSingleStepDecompress(source, target, sourceFormat, targetFormat, pStep);

    case MakeTypeKey(WImageFormatType::PLANAR, WImageFormatType::LINEAR):
      return ConvertSingleStepDeplanarize(source, target, sourceFormat, targetFormat, pStep);

    default:
      W_ASSERT_NOT_IMPLEMENTED;
      return W_FAILURE;
  }
}

WResult WImageConversion::ConvertSingleStepDecompress(
  const WImageView& source, WImage& target, WImageFormat::Enum sourceFormat, WImageFormat::Enum targetFormat, const WImageConversionStep* pStep)
{
  for (WUInt32 arrayIndex = 0; arrayIndex < source.GetNumArrayIndices(); arrayIndex++)
  {
    for (WUInt32 face = 0; face < source.GetNumFaces(); face++)
    {
      for (WUInt32 mipLevel = 0; mipLevel < source.GetNumMipLevels(); mipLevel++)
      {
        const WUInt32 width = target.GetWidth(mipLevel);
        const WUInt32 height = target.GetHeight(mipLevel);

        const WUInt32 blockSizeX = WImageFormat::GetBlockWidth(sourceFormat);
        const WUInt32 blockSizeY = WImageFormat::GetBlockHeight(sourceFormat);

        const WUInt32 numBlocksX = source.GetNumBlocksX(mipLevel);
        const WUInt32 numBlocksY = source.GetNumBlocksY(mipLevel);

        const WUInt64 targetRowPitch = target.GetRowPitch(mipLevel);
        const WUInt32 targetBytesPerPixel = WImageFormat::GetBitsPerPixel(targetFormat) / 8;

        // Decompress into a temp memory block so we don't have to explicitly handle the case where the image is not a multiple of the block
        // size
        WTempHybridArray<WUInt8, 256> tempBuffer;
        tempBuffer.SetCount(numBlocksX * blockSizeX * blockSizeY * targetBytesPerPixel);

        for (WUInt32 slice = 0; slice < source.GetDepth(mipLevel); slice++)
        {
          for (WUInt32 blockY = 0; blockY < numBlocksY; blockY++)
          {
            WImageView sourceRowView = source.GetRowView(mipLevel, face, arrayIndex, blockY, slice);

            if (static_cast<const WImageConversionStepDecompressBlocks*>(pStep)
                  ->DecompressBlocks(sourceRowView.GetByteBlobPtr(), WByteBlobPtr(tempBuffer.GetData(), tempBuffer.GetCount()), numBlocksX,
                    sourceFormat, targetFormat)
                  .Failed())
            {
              return W_FAILURE;
            }

            for (WUInt32 blockX = 0; blockX < numBlocksX; blockX++)
            {
              WUInt8* targetPointer = target.GetPixelPointer<WUInt8>(mipLevel, face, arrayIndex, blockX * blockSizeX, blockY * blockSizeY, slice);

              // Copy into actual target, clamping to image dimensions
              WUInt32 copyWidth = WMath::Min(blockSizeX, width - blockX * blockSizeX);
              WUInt32 copyHeight = WMath::Min(blockSizeY, height - blockY * blockSizeY);
              for (WUInt32 row = 0; row < copyHeight; row++)
              {
                memcpy(targetPointer, &tempBuffer[(blockX * blockSizeX + row) * blockSizeY * targetBytesPerPixel],
                  WMath::SafeMultiply32(copyWidth, targetBytesPerPixel));
                targetPointer += targetRowPitch;
              }
            }
          }
        }
      }
    }
  }

  return W_SUCCESS;
}

WResult WImageConversion::ConvertSingleStepCompress(
  const WImageView& source, WImage& target, WImageFormat::Enum sourceFormat, WImageFormat::Enum targetFormat, const WImageConversionStep* pStep)
{
  for (WUInt32 arrayIndex = 0; arrayIndex < source.GetNumArrayIndices(); arrayIndex++)
  {
    for (WUInt32 face = 0; face < source.GetNumFaces(); face++)
    {
      for (WUInt32 mipLevel = 0; mipLevel < source.GetNumMipLevels(); mipLevel++)
      {
        const WUInt32 sourceWidth = source.GetWidth(mipLevel);
        const WUInt32 sourceHeight = source.GetHeight(mipLevel);

        const WUInt32 numBlocksX = target.GetNumBlocksX(mipLevel);
        const WUInt32 numBlocksY = target.GetNumBlocksY(mipLevel);

        const WUInt32 targetWidth = numBlocksX * WImageFormat::GetBlockWidth(targetFormat);
        const WUInt32 targetHeight = numBlocksY * WImageFormat::GetBlockHeight(targetFormat);

        const WUInt64 sourceRowPitch = source.GetRowPitch(mipLevel);
        const WUInt32 sourceBytesPerPixel = WImageFormat::GetBitsPerPixel(sourceFormat) / 8;

        // Pad image to multiple of block size for compression
        WImageHeader paddedSliceHeader;
        paddedSliceHeader.SetWidth(targetWidth);
        paddedSliceHeader.SetHeight(targetHeight);
        paddedSliceHeader.SetImageFormat(sourceFormat);

        WImage paddedSlice;
        paddedSlice.ResetAndAlloc(paddedSliceHeader);

        for (WUInt32 slice = 0; slice < source.GetDepth(mipLevel); slice++)
        {
          for (WUInt32 y = 0; y < targetHeight; ++y)
          {
            WUInt32 sourceY = WMath::Min(y, sourceHeight - 1);

            memcpy(paddedSlice.GetPixelPointer<void>(0, 0, 0, 0, y), source.GetPixelPointer<void>(mipLevel, face, arrayIndex, 0, sourceY, slice),
              static_cast<size_t>(sourceRowPitch));

            for (WUInt32 x = sourceWidth; x < targetWidth; ++x)
            {
              memcpy(paddedSlice.GetPixelPointer<void>(0, 0, 0, x, y),
                source.GetPixelPointer<void>(mipLevel, face, arrayIndex, sourceWidth - 1, sourceY, slice), sourceBytesPerPixel);
            }
          }

          WResult result = static_cast<const WImageConversionStepCompressBlocks*>(pStep)->CompressBlocks(paddedSlice.GetByteBlobPtr(),
            target.GetSliceView(mipLevel, face, arrayIndex, slice).GetByteBlobPtr(), numBlocksX, numBlocksY, sourceFormat, targetFormat);

          if (result.Failed())
          {
            return W_FAILURE;
          }
        }
      }
    }
  }

  return W_SUCCESS;
}

WResult WImageConversion::ConvertSingleStepDeplanarize(
  const WImageView& source, WImage& target, WImageFormat::Enum sourceFormat, WImageFormat::Enum targetFormat, const WImageConversionStep* pStep)
{
  for (WUInt32 arrayIndex = 0; arrayIndex < source.GetNumArrayIndices(); arrayIndex++)
  {
    for (WUInt32 face = 0; face < source.GetNumFaces(); face++)
    {
      for (WUInt32 mipLevel = 0; mipLevel < source.GetNumMipLevels(); mipLevel++)
      {
        const WUInt32 width = target.GetWidth(mipLevel);
        const WUInt32 height = target.GetHeight(mipLevel);

        WTempHybridArray<WImageView, 2> sourcePlanes;
        for (WUInt32 planeIndex = 0; planeIndex < source.GetPlaneCount(); ++planeIndex)
        {
          const WUInt32 blockSizeX = WImageFormat::GetBlockWidth(sourceFormat, planeIndex);
          const WUInt32 blockSizeY = WImageFormat::GetBlockHeight(sourceFormat, planeIndex);

          if (width % blockSizeX != 0 || height % blockSizeY != 0)
          {
            // Input image must be aligned to block dimensions already.
            return W_FAILURE;
          }

          sourcePlanes.PushBack(source.GetPlaneView(mipLevel, face, arrayIndex, planeIndex));
        }

        if (static_cast<const WImageConversionStepDeplanarize*>(pStep)
              ->ConvertPixels(sourcePlanes, target.GetSubImageView(mipLevel, face, arrayIndex), width, height, sourceFormat, targetFormat)
              .Failed())
        {
          return W_FAILURE;
        }
      }
    }
  }

  return W_SUCCESS;
}

WResult WImageConversion::ConvertSingleStepPlanarize(
  const WImageView& source, WImage& target, WImageFormat::Enum sourceFormat, WImageFormat::Enum targetFormat, const WImageConversionStep* pStep)
{
  for (WUInt32 arrayIndex = 0; arrayIndex < source.GetNumArrayIndices(); arrayIndex++)
  {
    for (WUInt32 face = 0; face < source.GetNumFaces(); face++)
    {
      for (WUInt32 mipLevel = 0; mipLevel < source.GetNumMipLevels(); mipLevel++)
      {
        const WUInt32 width = target.GetWidth(mipLevel);
        const WUInt32 height = target.GetHeight(mipLevel);

        WTempHybridArray<WImage, 2> targetPlanes;
        for (WUInt32 planeIndex = 0; planeIndex < target.GetPlaneCount(); ++planeIndex)
        {
          const WUInt32 blockSizeX = WImageFormat::GetBlockWidth(targetFormat, planeIndex);
          const WUInt32 blockSizeY = WImageFormat::GetBlockHeight(targetFormat, planeIndex);

          if (width % blockSizeX != 0 || height % blockSizeY != 0)
          {
            // Input image must be aligned to block dimensions already.
            return W_FAILURE;
          }

          targetPlanes.PushBack(target.GetPlaneView(mipLevel, face, arrayIndex, planeIndex));
        }

        if (static_cast<const WImageConversionStepPlanarize*>(pStep)
              ->ConvertPixels(source.GetSubImageView(mipLevel, face, arrayIndex), targetPlanes, width, height, sourceFormat, targetFormat)
              .Failed())
        {
          return W_FAILURE;
        }
      }
    }
  }

  return W_SUCCESS;
}

bool WImageConversion::IsConvertible(WImageFormat::Enum sourceFormat, WImageFormat::Enum targetFormat)
{
  W_LOCK(s_conversionTableLock);

  if (!s_conversionTableValid)
  {
    RebuildConversionTable();
  }

  WUInt32 tableIndex = MakeKey(sourceFormat, targetFormat);
  return s_conversionTable.Contains(tableIndex);
}

WImageFormat::Enum WImageConversion::FindClosestCompatibleFormat(
  WImageFormat::Enum format, WArrayPtr<const WImageFormat::Enum> compatibleFormats)
{
  W_LOCK(s_conversionTableLock);

  if (!s_conversionTableValid)
  {
    RebuildConversionTable();
  }

  TableEntry bestEntry;
  WImageFormat::Enum bestFormat = WImageFormat::UNKNOWN;

  for (WUInt32 targetIndex = 0; targetIndex < WUInt32(compatibleFormats.GetCount()); targetIndex++)
  {
    WUInt32 tableIndex = MakeKey(format, compatibleFormats[targetIndex]);
    TableEntry candidate;
    if (s_conversionTable.TryGetValue(tableIndex, candidate) && candidate < bestEntry)
    {
      bestEntry = candidate;
      bestFormat = compatibleFormats[targetIndex];
    }
  }

  return bestFormat;
}
