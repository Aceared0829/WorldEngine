#pragma once

#include <Foundation/Containers/Deque.h>

/// A List-class, similar to STL::list
///
/// This container class allows fast insertion and erasure of elements.
/// Access is limited to iteration from front-to-back or back-to-front, there is no random-access.
/// Define the type of object to store in the list via the template argument T.
template <typename T>
class WListBase
{
private:
  struct ListElement;

  struct ListElementBase
  {
    ListElementBase();

    ListElement* m_pPrev;
    ListElement* m_pNext;
  };

  /// A list-node, containing data and prev/next pointers
  struct ListElement : public ListElementBase
  {
    ListElement()
      : ListElementBase()
    {
    }
    explicit ListElement(const T& data);

    T m_Data = {};
  };

  /// base-class for all iterators
  struct ConstIterator
  {
    W_DECLARE_POD_TYPE();

    /// Constructor.
    ConstIterator()
      : m_pElement(nullptr)
    {
    } // [tested]

    /// Equality comparison operator.
    bool operator==(typename WListBase<T>::ConstIterator it2) const { return (m_pElement == it2.m_pElement); } // [tested]
    W_ADD_DEFAULT_OPERATOR_NOTEQUAL(typename WListBase<T>::ConstIterator);

    /// Grants access to the node-data.
    const T& operator*() const { return (m_pElement->m_Data); } // [tested]

    /// Grants access to the node-data.
    const T* operator->() const { return (&m_pElement->m_Data); } // [tested]

    /// Moves the iterator to the next node.
    void Next() { m_pElement = m_pElement->m_pNext; } // [tested]

    /// Moves the iterator to the previous node.
    void Prev() { m_pElement = m_pElement->m_pPrev; } // [tested]

    /// Checks whether this iterator points to a valid element (and not the start/end of the list)
    bool IsValid() const { return ((m_pElement != nullptr) && (m_pElement->m_pPrev != nullptr) && (m_pElement->m_pNext != nullptr)); } // [tested]

    /// Moves the iterator to the next element in the list.
    void operator++() { Next(); } // [tested]

    /// Moves the iterator to the previous element in the list.
    void operator--() { Prev(); } // [tested]

  private:
    friend class WListBase<T>;

    ConstIterator(ListElement* pInit)
      : m_pElement(pInit)
    {
    }

    ListElement* m_pElement;
  };

public:
  /// A forward-iterator. Allows sequential access from front-to-back.
  struct Iterator : public ConstIterator
  {
    // this is required to pull in the const version of this function
    using ConstIterator::operator*;
    using ConstIterator::operator->;

    W_DECLARE_POD_TYPE();

    /// Constructor.
    Iterator()
      : ConstIterator()
    {
    } // [tested]

    /// Accesses the element stored in the node.
    T& operator*() { return (this->m_pElement->m_Data); } // [tested]

    /// Accesses the element stored in the node.
    T* operator->() { return (&this->m_pElement->m_Data); } // [tested]

  private:
    friend class WListBase<T>;

    explicit Iterator(ListElement* pInit)
      : ConstIterator(pInit)
    {
    }
  };

protected:
  /// Initializes the list to be empty.
  explicit WListBase(WAllocator* pAllocator); // [tested]

  /// Initializes the list with a copy from another list.
  WListBase(const WListBase<T>& cc, WAllocator* pAllocator); // [tested]

  /// Destroys the list and all its content.
  ~WListBase(); // [tested]

  /// Copies the list cc into this list.
  void operator=(const WListBase<T>& cc); // [tested]

public:
  /// Clears the list, afterwards it is empty.
  void Clear(); // [tested]

  /// See WDeque::Compact()
  void Compact();

  /// Returns the number of elements in the list. O(1) operation.
  WUInt32 GetCount() const; // [tested]

  /// Returns whether size == 0. O(1) operation.
  bool IsEmpty() const; // [tested]

  /// Returns the very first element in the list.
  const T& PeekFront() const; // [tested]

  /// Returns the very last element in the list.
  const T& PeekBack() const; // [tested]

  /// Returns the very first element in the list.
  T& PeekFront(); // [tested]

  /// Returns the very last element in the list.
  T& PeekBack(); // [tested]

  /// Appends a default-constructed element to the list and returns a reference to it.
  T& PushBack(); // [tested]

  /// Appends a copy of the given element to the list.
  void PushBack(const T& element); // [tested]

  /// Removes the very last element from the list.
  void PopBack(); // [tested]

  /// Appends a default-constructed element to the front of the list and returns a reference to it.
  T& PushFront(); // [tested]

  /// Appends a copy of the given element to the front of the list.
  void PushFront(const T& element); // [tested]

  /// Removes the very first element from the list.
  void PopFront(); // [tested]

  /// Sets the number of elements that are in the list.
  void SetCount(WUInt32 uiNewSize); // [tested]

  /// Inserts one element before the position defined by the iterator.
  Iterator Insert(const Iterator& pos, const T& data); // [tested]

  /// Inserts the range defined by [first;last) after pos.
  void Insert(const Iterator& pos, ConstIterator first, const ConstIterator& last);

  /// Inserts a default constructed element before the position defined by the iterator.
  Iterator Insert(const Iterator& pos);

  /// Erases the element pointed to by the iterator.
  Iterator Remove(const Iterator& pos); // [tested]

  /// Erases range [first; last).
  Iterator Remove(Iterator first, const Iterator& last);

  /// Returns an iterator to the first list-element.
  Iterator GetIterator(); // [tested]

  /// Returns an iterator pointing behind the last element. Necessary if one wants to insert elements at the end of a list.
  Iterator GetEndIterator(); // [tested]

  /// Returns a const-iterator to the first list-element.
  ConstIterator GetIterator() const; // [tested]

  /// Returns a const-iterator pointing behind the last element. Necessary if one wants to insert elements at the end of a list.
  ConstIterator GetEndIterator() const; // [tested]

  /// Returns the allocator that is used by this instance.
  WAllocator* GetAllocator() const { return m_Elements.GetAllocator(); }

  /// Comparison operator
  bool operator==(const WListBase<T>& rhs) const; // [tested]
  W_ADD_DEFAULT_OPERATOR_NOTEQUAL(const WListBase<T>&);

  /// Returns the amount of bytes that are currently allocated on the heap.
  WUInt64 GetHeapMemoryUsage() const { return m_Elements.GetHeapMemoryUsage(); } // [tested]

private:
  /// Sentinel node before the first element.
  ListElementBase m_First;

  /// Sentinel node after the last element.
  ListElementBase m_Last;

  // Small hack to get around const problems.
  Iterator m_End;

  /// The number of active elements in the list.
  WUInt32 m_uiCount;

  /// Acquires and initializes one default constructed node.
  ListElement* AcquireNode();

  /// Destructs one node and puts it into the free-list.
  void ReleaseNode(ListElement* pNode);

  /// Data-Store. Contains all the elements.
  WDeque<ListElement, WNullAllocatorWrapper, false> m_Elements;

  /// Stack that holds recently freed nodes, that can be quickly reused.
  ListElement* m_pFreeElementStack;
};

/// \see WListBase
template <typename T, typename AllocatorWrapper = WDefaultAllocatorWrapper>
class WList : public WListBase<T>
{
public:
  WList();
  explicit WList(WAllocator* pAllocator);

  WList(const WList<T, AllocatorWrapper>& other);
  WList(const WListBase<T>& other);

  void operator=(const WList<T, AllocatorWrapper>& rhs);
  void operator=(const WListBase<T>& rhs);
};

#include <Foundation/Containers/Implementation/List_inl.h>
