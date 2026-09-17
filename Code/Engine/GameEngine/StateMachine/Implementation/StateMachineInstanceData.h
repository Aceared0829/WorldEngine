#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Containers/Blob.h>
#include <Foundation/Containers/SmallArray.h>
#include <Foundation/Memory/InstanceDataAllocator.h>

namespace WStateMachineInternal
{
  /// Helper class to manage instance data for compound states or transitions
  struct W_GAMEENGINE_DLL Compound
  {
    W_ALWAYS_INLINE WUInt32 GetBaseOffset() const { return m_InstanceDataOffsets.GetUserData<WUInt32>(); }
    W_ALWAYS_INLINE WUInt32 GetDataSize() const { return m_InstanceDataAllocator.GetTotalDataSize(); }

    WSmallArray<WUInt32, 2> m_InstanceDataOffsets;
    WInstanceDataAllocator m_InstanceDataAllocator;

    struct InstanceData
    {
      const Compound* m_pOwner = nullptr;

      ~InstanceData()
      {
        if (m_pOwner != nullptr)
        {
          m_pOwner->m_InstanceDataAllocator.Destruct(GetBlobPtr());
        }
      }

      W_ALWAYS_INLINE WByteBlobPtr GetBlobPtr()
      {
        return WByteBlobPtr(WMemoryUtils::AddByteOffset(reinterpret_cast<WUInt8*>(this), m_pOwner->GetBaseOffset()), m_pOwner->GetDataSize());
      }
    };

    W_ALWAYS_INLINE void* GetSubInstanceData(InstanceData* pData, WUInt32 uiIndex) const
    {
      return pData != nullptr ? m_InstanceDataAllocator.GetInstanceData(pData->GetBlobPtr(), m_InstanceDataOffsets[uiIndex]) : nullptr;
    }

    W_FORCE_INLINE void Initialize(InstanceData* pData) const
    {
      if (pData != nullptr && pData->m_pOwner == nullptr)
      {
        pData->m_pOwner = this;
        m_InstanceDataAllocator.Construct(pData->GetBlobPtr());
      }
    }

    template <typename T>
    bool GetInstanceDataDesc(WArrayPtr<T*> subObjects, WInstanceDataDesc& out_desc)
    {
      m_InstanceDataOffsets.Clear();
      m_InstanceDataAllocator.ClearDescs();

      WUInt32 uiMaxAlignment = 0;

      WInstanceDataDesc instanceDataDesc;
      for (T* pSubObject : subObjects)
      {
        WUInt32 uiOffset = WInvalidIndex;
        if (pSubObject->GetInstanceDataDesc(instanceDataDesc))
        {
          uiOffset = m_InstanceDataAllocator.AddDesc(instanceDataDesc);
          uiMaxAlignment = WMath::Max(uiMaxAlignment, instanceDataDesc.m_uiTypeAlignment);
        }
        m_InstanceDataOffsets.PushBack(uiOffset);
      }

      if (uiMaxAlignment > 0)
      {
        out_desc.FillFromType<InstanceData>();
        out_desc.m_ConstructorFunction = nullptr; // not needed, instance data is constructed on first OnEnter

        WUInt32 uiBaseOffset = WMemoryUtils::AlignSize(out_desc.m_uiTypeSize, uiMaxAlignment);
        m_InstanceDataOffsets.GetUserData<WUInt32>() = uiBaseOffset;

        out_desc.m_uiTypeSize = uiBaseOffset + m_InstanceDataAllocator.GetTotalDataSize();
        out_desc.m_uiTypeAlignment = WMath::Max(out_desc.m_uiTypeAlignment, uiMaxAlignment);

        return true;
      }

      return false;
    }
  };
} // namespace WStateMachineInternal
