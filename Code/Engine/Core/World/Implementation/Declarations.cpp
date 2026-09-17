#include <Core/CorePCH.h>

#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/World/World.h>
#include <Foundation/Types/VariantTypeRegistry.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WGameObjectHandle, WNoBase, 1, WRTTIDefaultAllocator<WGameObjectHandle>)
W_END_STATIC_REFLECTED_TYPE;
W_DEFINE_CUSTOM_VARIANT_TYPE(WGameObjectHandle);

W_BEGIN_STATIC_REFLECTED_TYPE(WComponentHandle, WNoBase, 1, WRTTIDefaultAllocator<WComponentHandle>)
W_END_STATIC_REFLECTED_TYPE;
W_DEFINE_CUSTOM_VARIANT_TYPE(WComponentHandle);

W_BEGIN_STATIC_REFLECTED_ENUM(WObjectMode, 1)
  W_ENUM_CONSTANTS(WObjectMode::Automatic, WObjectMode::ForceDynamic)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WComponentMode, 1)
  W_ENUM_CONSTANTS(WComponentMode::Static, WComponentMode::Dynamic)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WObjectMsgQueueType, 1)
  W_ENUM_CONSTANTS(WObjectMsgQueueType::PostAsync, WObjectMsgQueueType::PostTransform, WObjectMsgQueueType::NextFrame, WObjectMsgQueueType::AfterInitialized)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WOnComponentFinishedAction, 1)
  W_ENUM_CONSTANTS(WOnComponentFinishedAction::None, WOnComponentFinishedAction::DeleteComponent, WOnComponentFinishedAction::DeleteGameObject)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WOnComponentFinishedAction2, 1)
  W_ENUM_CONSTANTS(WOnComponentFinishedAction2::None, WOnComponentFinishedAction2::DeleteComponent, WOnComponentFinishedAction2::DeleteGameObject, WOnComponentFinishedAction2::Restart)
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

//////////////////////////////////////////////////////////////////////////

void operator<<(WStreamWriter& inout_stream, const WGameObjectHandle& hValue)
{
  W_IGNORE_UNUSED(inout_stream);
  W_IGNORE_UNUSED(hValue);
  W_ASSERT_DEV(false, "This function should not be called. Use WWorldWriter::WriteGameObjectHandle instead.");
}

void operator>>(WStreamReader& inout_stream, WGameObjectHandle& ref_hValue)
{
  W_IGNORE_UNUSED(inout_stream);
  W_IGNORE_UNUSED(ref_hValue);
  W_ASSERT_DEV(false, "This function should not be called. Use WWorldReader::ReadGameObjectHandle instead.");
}

void operator<<(WStreamWriter& inout_stream, const WComponentHandle& hValue)
{
  W_IGNORE_UNUSED(inout_stream);
  W_IGNORE_UNUSED(hValue);
  W_ASSERT_DEV(false, "This function should not be called. Use WWorldWriter::WriteComponentHandle instead.");
}

void operator>>(WStreamReader& inout_stream, WComponentHandle& ref_hValue)
{
  W_IGNORE_UNUSED(inout_stream);
  W_IGNORE_UNUSED(ref_hValue);
  W_ASSERT_DEV(false, "This function should not be called. Use WWorldReader::ReadComponentHandle instead.");
}

//////////////////////////////////////////////////////////////////////////

namespace
{
  template <typename T>
  void HandleFinishedActionImpl(WComponent* pComponent, typename T::Enum action)
  {
    if (action == T::DeleteGameObject)
    {
      // Send a message to the owner object to check whether another component wants to delete this object later.
      // Can't use WGameObject::SendMessage because the object would immediately delete itself and furthermore the sender component needs to be
      // filtered out here.
      WMsgDeleteGameObject msg;

      for (WComponent* pComp : pComponent->GetOwner()->GetComponents())
      {
        if (pComp == pComponent)
          continue;

        pComp->SendMessage(msg);
        if (msg.m_bCancel)
        {
          action = T::DeleteComponent;
          break;
        }
      }

      if (action == T::DeleteGameObject)
      {
        pComponent->GetWorld()->DeleteObjectDelayed(pComponent->GetOwner()->GetHandle());
        return;
      }
    }

    if (action == T::DeleteComponent)
    {
      pComponent->DeleteComponent();
    }
  }

  template <typename T>
  void HandleDeleteObjectMsgImpl(WMsgDeleteGameObject& ref_msg, WEnum<T>& ref_action)
  {
    if (ref_action == T::DeleteComponent)
    {
      ref_msg.m_bCancel = true;
      ref_action = T::DeleteGameObject;
    }
    else if (ref_action == T::DeleteGameObject)
    {
      ref_msg.m_bCancel = true;
    }
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

void WOnComponentFinishedAction::HandleFinishedAction(WComponent* pComponent, WOnComponentFinishedAction::Enum action)
{
  HandleFinishedActionImpl<WOnComponentFinishedAction>(pComponent, action);
}

void WOnComponentFinishedAction::HandleDeleteObjectMsg(WMsgDeleteGameObject& ref_msg, WEnum<WOnComponentFinishedAction>& ref_action)
{
  HandleDeleteObjectMsgImpl(ref_msg, ref_action);
}

//////////////////////////////////////////////////////////////////////////

void WOnComponentFinishedAction2::HandleFinishedAction(WComponent* pComponent, WOnComponentFinishedAction2::Enum action)
{
  HandleFinishedActionImpl<WOnComponentFinishedAction2>(pComponent, action);
}

void WOnComponentFinishedAction2::HandleDeleteObjectMsg(WMsgDeleteGameObject& ref_msg, WEnum<WOnComponentFinishedAction2>& ref_action)
{
  HandleDeleteObjectMsgImpl(ref_msg, ref_action);
}

W_STATICLINK_FILE(Core, Core_World_Implementation_Declarations);
