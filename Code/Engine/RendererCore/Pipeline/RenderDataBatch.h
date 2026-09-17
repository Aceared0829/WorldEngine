#pragma once

#include <RendererCore/Pipeline/Declarations.h>

/// Represents a batch of render data that can be rendered together.
///
/// Render data is grouped into batches to minimize state changes during rendering.
/// Each batch contains render data of the same type, sorted by a sorting key.
/// Provides iterator access to iterate through the typed render data.
class WRenderDataBatch
{
private:
  struct SortableRenderData
  {
    W_DECLARE_POD_TYPE();

    const WRenderData* m_pRenderData;
    WUInt64 m_uiSortingKey;
  };

public:
  W_DECLARE_POD_TYPE();

  /// Iterator for traversing typed render data within a batch.
  template <typename T>
  class Iterator
  {
  public:
    const T& operator*() const;
    const T* operator->() const;

    operator const T*() const;

    /// Advances to the next element.
    void Next();

    /// Returns true if the iterator points to a valid element.
    bool IsValid() const;

    void operator++();

  private:
    friend class WRenderDataBatch;

    Iterator(const SortableRenderData* pStart, const SortableRenderData* pEnd);

    const SortableRenderData* m_pCurrent;
    const SortableRenderData* m_pEnd;
  };

  WUInt32 GetDataCount() const;

  template <typename T>
  const T* GetFirstData() const;

  template <typename T>
  Iterator<T> GetIterator(WUInt32 uiStartIndex = 0, WUInt32 uiCount = WInvalidIndex) const;

  WGALBufferHandle GetDataOffsetsBuffer() const;
  WUInt32 GetFirstDataOffsetIndex() const;
  WUInt32 GetInstanceCount() const;

private:
  friend class WExtractedRenderData;
  friend class WRenderDataBatchList;

  WArrayPtr<SortableRenderData> m_Data;

  WGALBufferHandle m_hDataOffsetsBuffer;
  WUInt32 m_uiFirstDataOffsetIndex = 0;
  WUInt32 m_uiInstanceCount = 0;
};

/// Contains a list of render data batches for a specific render category.
///
/// Used to access all batches that need to be rendered for a particular category.
class WRenderDataBatchList
{
public:
  WUInt32 GetBatchCount() const;

  const WRenderDataBatch& GetBatch(WUInt32 uiIndex) const;

private:
  friend class WExtractedRenderData;

  WArrayPtr<const WRenderDataBatch> m_Batches;
};

#include <RendererCore/Pipeline/Implementation/RenderDataBatch_inl.h>
