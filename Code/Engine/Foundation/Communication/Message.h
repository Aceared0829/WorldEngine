#pragma once

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/UniquePtr.h>

using WMessageId = WUInt16;
class WStreamWriter;
class WStreamReader;

/// Base class for all message types. Each message type has it's own id which is used to dispatch messages efficiently.
///
/// To implement a custom message type derive from WMessage and add W_DECLARE_MESSAGE_TYPE to the type declaration.
/// W_IMPLEMENT_MESSAGE_TYPE needs to be added to a cpp.
/// \see WRTTI
///
/// For the automatic cloning to work and for efficiency the messages must only contain simple data members.
/// For instance, everything that allocates internally (strings, arrays) should be avoided.
/// Instead, such objects should be located somewhere else and the message should only contain pointers to the data.
///
class W_FOUNDATION_DLL WMessage : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WMessage, WReflectedClass);

protected:
  explicit WMessage(size_t messageSize)
  {
    const auto sizeOffset = (reinterpret_cast<uintptr_t>(&m_Id) - reinterpret_cast<uintptr_t>(this)) + sizeof(m_Id);
    memset((void*)WMemoryUtils::AddByteOffset(this, sizeOffset), 0, messageSize - sizeOffset);
    m_uiSize = static_cast<WUInt16>(messageSize);
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
    m_uiDebugMessageRouting = 0;
#endif
  }

public:
  W_ALWAYS_INLINE WMessage()
    : WMessage(sizeof(WMessage))
  {
  }

  virtual ~WMessage() = default;

  /// Derived message types can override this method to influence sorting order. Smaller keys are processed first.
  virtual WInt32 GetSortingKey() const { return 0; }

  /// Returns the id for this message type.
  W_ALWAYS_INLINE WMessageId GetId() const { return m_Id; }

  /// Returns the size in byte of this message.
  W_ALWAYS_INLINE WUInt16 GetSize() const { return m_uiSize; }

  /// Calculates a hash of the message.
  W_ALWAYS_INLINE WUInt64 GetHash() const { return WHashingUtils::xxHash64(this, m_uiSize); }

  /// Implement this for efficient transmission across process boundaries (e.g. network transfer etc.)
  ///
  /// If the message is only ever sent within the same process between nodes of the same WWorld,
  /// this does not need to be implemented.
  ///
  /// Note that PackageForTransfer() will automatically include the WRTTI type version into the stream
  /// and ReplicatePackedMessage() will pass this into Deserialize(). Use this if the serialization changes.
  virtual void Serialize(WStreamWriter& inout_stream) const
  {
    W_IGNORE_UNUSED(inout_stream);
    W_ASSERT_NOT_IMPLEMENTED;
  }

  /// \see Serialize()
  virtual void Deserialize(WStreamReader& inout_stream, WUInt8 uiTypeVersion)
  {
    W_IGNORE_UNUSED(inout_stream);
    W_IGNORE_UNUSED(uiTypeVersion);
    W_ASSERT_NOT_IMPLEMENTED;
  }

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  /// set to true while debugging a message routing problem
  /// if the message is not delivered to any recipient at all, information about why that is will be written to WLog
  W_ALWAYS_INLINE void SetDebugMessageRouting(bool bDebug) { m_uiDebugMessageRouting = bDebug; }

  W_ALWAYS_INLINE bool GetDebugMessageRouting() const { return m_uiDebugMessageRouting; }
#endif

protected:
  W_ALWAYS_INLINE static WMessageId GetNextMsgId() { return s_NextMsgId++; }

  WMessageId m_Id;

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  WUInt16 m_uiSize : 15;
  WUInt16 m_uiDebugMessageRouting : 1;
#else
  WUInt16 m_uiSize;
#endif

  static WMessageId s_NextMsgId;


  //////////////////////////////////////////////////////////////////////////
  // Transferring and replicating messages
  //

public:
  /// Writes msg to stream in such a way that ReplicatePackedMessage() can restore it even in another process
  ///
  /// For this to work the message type has to have the Serialize and Deserialize functions implemented.
  ///
  /// \note This is NOT used by WWorld. Within the same process messages can be dispatched more efficiently.
  static void PackageForTransfer(const WMessage& msg, WStreamWriter& inout_stream);

  /// Restores a message that was written by PackageForTransfer()
  ///
  /// If the message type is unknown, nullptr is returned.
  /// \see PackageForTransfer()
  static WUniquePtr<WMessage> ReplicatePackedMessage(WStreamReader& inout_stream);

private:
};

/// Add this macro to the declaration of your custom message type.
#define W_DECLARE_MESSAGE_TYPE(messageType, baseType)      \
private:                                                    \
  W_ADD_DYNAMIC_REFLECTION(messageType, baseType);         \
  static WMessageId MSG_ID;                                \
                                                            \
protected:                                                  \
  W_ALWAYS_INLINE explicit messageType(size_t messageSize) \
    : baseType(messageSize)                                 \
  {                                                         \
    m_Id = messageType::MSG_ID;                             \
  }                                                         \
                                                            \
public:                                                     \
  static WMessageId GetTypeMsgId()                         \
  {                                                         \
    static WMessageId id = WMessage::GetNextMsgId();      \
    return id;                                              \
  }                                                         \
                                                            \
  W_ALWAYS_INLINE messageType()                            \
    : messageType(sizeof(messageType))                      \
  {                                                         \
  }

/// Implements the given message type. Add this macro to a cpp outside of the type declaration.
#define W_IMPLEMENT_MESSAGE_TYPE(messageType) WMessageId messageType::MSG_ID = messageType::GetTypeMsgId();


/// Base class for all message senders.
template <typename T>
struct WMessageSenderBase
{
  using MessageType = T;
};
