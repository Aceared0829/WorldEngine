
#pragma once

#include <Foundation/Communication/Message.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

/// Implementation of a message queue on top of a deque.
///
/// Enqueue and TryDequeue/TryPeek methods are thread safe all the others are not. To ensure
/// thread safety for all methods the queue can be locked using WLock like a mutex.
/// Every entry consists of a pointer to a message and some meta data.
/// Lifetime of the enqueued messages needs to be managed by the user.
/// \see WMessage
template <typename MetaDataType>
class WMessageQueueBase
{
public:
  struct Entry
  {
    W_DECLARE_POD_TYPE();

    WMessage* m_pMessage;
    MetaDataType m_MetaData;
    mutable WUInt64 m_uiMessageHash = 0;
  };

protected:
  /// No memory is allocated during construction.
  WMessageQueueBase(WAllocator* pAllocator); // [tested]

  /// No memory is allocated during construction.
  WMessageQueueBase(const WMessageQueueBase& rhs, WAllocator* pAllocator);

  /// Destructor.
  ~WMessageQueueBase(); // [tested]

  /// Assignment operator.
  void operator=(const WMessageQueueBase& rhs);

public:
  /// Returns the element at the given index. Not thread safe.
  Entry& operator[](WUInt32 uiIndex); // [tested]

  /// Returns the element at the given index. Not thread safe.
  const Entry& operator[](WUInt32 uiIndex) const; // [tested]

  /// Returns the number of active elements in the queue.
  WUInt32 GetCount() const;

  /// Returns true, if the queue does not contain any elements.
  bool IsEmpty() const;

  /// Destructs all elements and sets the count to zero. Does not deallocate any data.
  void Clear();

  /// Expands the queue so it can at least store the given capacity.
  void Reserve(WUInt32 uiCount);

  /// Tries to compact the array to avoid wasting memory.The resulting capacity is at least 'GetCount' (no elements get removed).
  void Compact();

  /// Enqueues the given message and meta-data. This method is thread safe.
  void Enqueue(WMessage* pMessage, const MetaDataType& metaData); // [tested]

  /// Dequeues the first element if the queue is not empty and returns true. Returns false if the queue is empty. This method is thread safe.
  bool TryDequeue(WMessage*& out_pMessage, MetaDataType& out_metaData); // [tested]

  /// Gives the first element if the queue is not empty and returns true. Returns false if the queue is empty. This method is thread safe.
  bool TryPeek(WMessage*& out_pMessage, MetaDataType& out_metaData); // [tested]

  /// Returns the first element in the queue. Not thread safe.
  Entry& Peek();

  /// Removes the first element from the queue. Not thread safe.
  void Dequeue();

  /// Sort with explicit comparer. Not thread safe.
  template <typename Comparer>
  void Sort(const Comparer& comparer); // [tested]

  /// Acquires an exclusive lock on the queue. Do not use this method directly but use WLock instead.
  void Lock(); // [tested]

  /// Releases a lock that has been previously acquired. Do not use this method directly but use WLock instead.
  void Unlock(); // [tested]

private:
  WDeque<Entry, WNullAllocatorWrapper> m_Queue;
  WMutex m_Mutex;
};

/// \see WMessageQueueBase
template <typename MetaDataType, typename AllocatorWrapper = WDefaultAllocatorWrapper>
class WMessageQueue : public WMessageQueueBase<MetaDataType>
{
public:
  WMessageQueue();
  WMessageQueue(WAllocator* pAllocator);

  WMessageQueue(const WMessageQueue<MetaDataType, AllocatorWrapper>& rhs);
  WMessageQueue(const WMessageQueueBase<MetaDataType>& rhs);

  void operator=(const WMessageQueue<MetaDataType, AllocatorWrapper>& rhs);
  void operator=(const WMessageQueueBase<MetaDataType>& rhs);
};

#include <Foundation/Communication/Implementation/MessageQueue_inl.h>
