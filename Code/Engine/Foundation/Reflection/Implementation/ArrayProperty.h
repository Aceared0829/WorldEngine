#pragma once

/// \file

#include <Foundation/Reflection/Implementation/AbstractProperty.h>

class WRTTI;

/// Do not cast into this class or any of its derived classes, use WTypedArrayProperty instead.
template <typename Type>
class WTypedArrayProperty : public WAbstractArrayProperty
{
public:
  WTypedArrayProperty(const char* szPropertyName)
    : WAbstractArrayProperty(szPropertyName)
  {
    m_Flags = WPropertyFlags::GetParameterFlags<Type>();
    static_assert(!std::is_pointer<Type>::value ||
                    WVariantTypeDeduction<typename WTypeTraits<Type>::NonConstReferencePointerType>::value ==
                      WVariantType::Invalid,
      "Pointer to standard types are not supported.");
  }

  virtual const WRTTI* GetSpecificType() const override { return WGetStaticRTTI<typename WTypeTraits<Type>::NonConstReferencePointerType>(); }
};

/// Specialization of WTypedArrayProperty to retain the pointer in const char*.
template <>
class WTypedArrayProperty<const char*> : public WAbstractArrayProperty
{
public:
  WTypedArrayProperty(const char* szPropertyName)
    : WAbstractArrayProperty(szPropertyName)
  {
    m_Flags = WPropertyFlags::GetParameterFlags<const char*>();
  }

  virtual const WRTTI* GetSpecificType() const override { return WGetStaticRTTI<const char*>(); }
};


template <typename Class, typename Type>
class WAccessorArrayProperty : public WTypedArrayProperty<Type>
{
public:
  using RealType = typename WTypeTraits<Type>::NonConstReferenceType;
  using GetCountFunc = WUInt32 (Class::*)() const;
  using GetValueFunc = Type (Class::*)(WUInt32 uiIndex) const;
  using SetValueFunc = void (Class::*)(WUInt32 uiIndex, Type value);
  using InsertFunc = void (Class::*)(WUInt32 uiIndex, Type value);
  using RemoveFunc = void (Class::*)(WUInt32 uiIndex);


  WAccessorArrayProperty(
    const char* szPropertyName, GetCountFunc getCount, GetValueFunc getter, SetValueFunc setter, InsertFunc insert, RemoveFunc remove)
    : WTypedArrayProperty<Type>(szPropertyName)
  {
    W_ASSERT_DEBUG(getCount != nullptr, "The get count function of an array property cannot be nullptr.");
    W_ASSERT_DEBUG(getter != nullptr, "The get value function of an array property cannot be nullptr.");

    m_GetCount = getCount;
    m_Getter = getter;
    m_Setter = setter;
    m_Insert = insert;
    m_Remove = remove;

    if (m_Setter == nullptr)
      WAbstractArrayProperty::m_Flags.Add(WPropertyFlags::ReadOnly);
  }


  virtual WUInt32 GetCount(const void* pInstance) const override { return (static_cast<const Class*>(pInstance)->*m_GetCount)(); }

  virtual void GetValue(const void* pInstance, WUInt32 uiIndex, void* pObject) const override
  {
    W_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "GetValue: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    *static_cast<RealType*>(pObject) = (static_cast<const Class*>(pInstance)->*m_Getter)(uiIndex);
  }

  virtual void SetValue(void* pInstance, WUInt32 uiIndex, const void* pObject) const override
  {
    W_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "SetValue: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    W_ASSERT_DEBUG(m_Setter != nullptr, "The property '{0}' has no setter function, thus it is read-only.", WAbstractProperty::GetPropertyName());
    (static_cast<Class*>(pInstance)->*m_Setter)(uiIndex, *static_cast<const RealType*>(pObject));
  }

  virtual void Insert(void* pInstance, WUInt32 uiIndex, const void* pObject) const override
  {
    W_ASSERT_DEBUG(uiIndex <= GetCount(pInstance), "Insert: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    W_ASSERT_DEBUG(m_Insert != nullptr, "The property '{0}' has no insert function, thus it is read-only.", WAbstractProperty::GetPropertyName());
    (static_cast<Class*>(pInstance)->*m_Insert)(uiIndex, *static_cast<const RealType*>(pObject));
  }

  virtual void Remove(void* pInstance, WUInt32 uiIndex) const override
  {
    W_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "Remove: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    W_ASSERT_DEBUG(m_Remove != nullptr, "The property '{0}' has no setter function, thus it is read-only.", WAbstractProperty::GetPropertyName());
    (static_cast<Class*>(pInstance)->*m_Remove)(uiIndex);
  }

  virtual void Clear(void* pInstance) const override { SetCount(pInstance, 0); }

  virtual void SetCount(void* pInstance, WUInt32 uiCount) const override
  {
    W_ASSERT_DEBUG(m_Insert != nullptr && m_Remove != nullptr, "The property '{0}' has no remove and insert function, thus it is fixed-size.",
      WAbstractProperty::GetPropertyName());
    while (uiCount < GetCount(pInstance))
    {
      Remove(pInstance, GetCount(pInstance) - 1);
    }
    while (uiCount > GetCount(pInstance))
    {
      RealType elem = RealType();
      Insert(pInstance, GetCount(pInstance), &elem);
    }
  }

private:
  GetCountFunc m_GetCount;
  GetValueFunc m_Getter;
  SetValueFunc m_Setter;
  InsertFunc m_Insert;
  RemoveFunc m_Remove;
};



template <typename Class, typename Container, Container Class::*Member>
struct WArrayPropertyAccessor
{
  using ContainerType = typename WTypeTraits<Container>::NonConstReferenceType;
  using Type = typename WTypeTraits<typename WContainerSubTypeResolver<ContainerType>::Type>::NonConstReferenceType;

  static const ContainerType& GetConstContainer(const Class* pInstance) { return (*pInstance).*Member; }

  static ContainerType& GetContainer(Class* pInstance) { return (*pInstance).*Member; }
};


template <typename Class, typename Container, typename Type>
class WMemberArrayProperty : public WTypedArrayProperty<typename WTypeTraits<Type>::NonConstReferenceType>
{
public:
  using RealType = typename WTypeTraits<Type>::NonConstReferenceType;
  using GetConstContainerFunc = const Container& (*)(const Class* pInstance);
  using GetContainerFunc = Container& (*)(Class* pInstance);

  WMemberArrayProperty(const char* szPropertyName, GetConstContainerFunc constGetter, GetContainerFunc getter)
    : WTypedArrayProperty<RealType>(szPropertyName)
  {
    W_ASSERT_DEBUG(constGetter != nullptr, "The const get count function of an array property cannot be nullptr.");

    m_ConstGetter = constGetter;
    m_Getter = getter;

    if (m_Getter == nullptr)
      WAbstractArrayProperty::m_Flags.Add(WPropertyFlags::ReadOnly);
  }

  virtual WUInt32 GetCount(const void* pInstance) const override { return m_ConstGetter(static_cast<const Class*>(pInstance)).GetCount(); }

  virtual void GetValue(const void* pInstance, WUInt32 uiIndex, void* pObject) const override
  {
    W_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "GetValue: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    *static_cast<RealType*>(pObject) = m_ConstGetter(static_cast<const Class*>(pInstance))[uiIndex];
  }

  virtual void SetValue(void* pInstance, WUInt32 uiIndex, const void* pObject) const override
  {
    W_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "SetValue: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    W_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const array accessor function, thus it is read-only.",
      WAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance))[uiIndex] = *static_cast<const RealType*>(pObject);
  }

  virtual void Insert(void* pInstance, WUInt32 uiIndex, const void* pObject) const override
  {
    W_ASSERT_DEBUG(uiIndex <= GetCount(pInstance), "Insert: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    W_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const array accessor function, thus it is read-only.",
      WAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).InsertAt(uiIndex, *static_cast<const RealType*>(pObject));
  }

  virtual void Remove(void* pInstance, WUInt32 uiIndex) const override
  {
    W_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "Remove: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    W_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const array accessor function, thus it is read-only.",
      WAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).RemoveAtAndCopy(uiIndex);
  }

  virtual void Clear(void* pInstance) const override
  {
    W_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const array accessor function, thus it is read-only.",
      WAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).Clear();
  }

// Suppress conversion warning from WUInt32 to WUInt16 for SetCount when using WSmallArray as container
#ifdef _MSC_VER /* Visual Studio warning fix */
#  pragma warning(push)
#  pragma warning(disable : 4244)
#endif
  virtual void SetCount(void* pInstance, WUInt32 uiCount) const override
  {
    W_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const array accessor function, thus it is read-only.",
      WAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).SetCount(uiCount);
  }
#ifdef _MSC_VER /* Visual Studio warning fix */
#  pragma warning(pop)
#endif
  virtual void* GetValuePointer(void* pInstance, WUInt32 uiIndex) const override
  {
    W_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "GetValue: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    return &(m_Getter(static_cast<Class*>(pInstance))[uiIndex]);
  }

private:
  GetConstContainerFunc m_ConstGetter;
  GetContainerFunc m_Getter;
};

/// Read only version of WMemberArrayProperty that does not call any functions that modify the array. This is needed to reflect WArrayPtr members.
template <typename Class, typename Container, typename Type>
class WMemberArrayReadOnlyProperty : public WTypedArrayProperty<typename WTypeTraits<Type>::NonConstReferenceType>
{
public:
  using RealType = typename WTypeTraits<Type>::NonConstReferenceType;
  using GetConstContainerFunc = const Container& (*)(const Class* pInstance);

  WMemberArrayReadOnlyProperty(const char* szPropertyName, GetConstContainerFunc constGetter)
    : WTypedArrayProperty<RealType>(szPropertyName)
  {
    W_ASSERT_DEBUG(constGetter != nullptr, "The const get count function of an array property cannot be nullptr.");

    m_ConstGetter = constGetter;
    WAbstractArrayProperty::m_Flags.Add(WPropertyFlags::ReadOnly);
  }

  virtual WUInt32 GetCount(const void* pInstance) const override { return m_ConstGetter(static_cast<const Class*>(pInstance)).GetCount(); }

  virtual void GetValue(const void* pInstance, WUInt32 uiIndex, void* pObject) const override
  {
    W_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "GetValue: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    *static_cast<RealType*>(pObject) = m_ConstGetter(static_cast<const Class*>(pInstance))[uiIndex];
  }

  virtual void SetValue(void* pInstance, WUInt32 uiIndex, const void* pObject) const override
  {
    W_REPORT_FAILURE("The property '{0}' is read-only.", WAbstractProperty::GetPropertyName());
  }

  virtual void Insert(void* pInstance, WUInt32 uiIndex, const void* pObject) const override
  {
    W_REPORT_FAILURE("The property '{0}' is read-only.", WAbstractProperty::GetPropertyName());
  }

  virtual void Remove(void* pInstance, WUInt32 uiIndex) const override
  {
    W_REPORT_FAILURE("The property '{0}' is read-only.", WAbstractProperty::GetPropertyName());
  }

  virtual void Clear(void* pInstance) const override
  {
    W_REPORT_FAILURE("The property '{0}' is read-only.", WAbstractProperty::GetPropertyName());
  }

  virtual void SetCount(void* pInstance, WUInt32 uiCount) const override
  {
    W_REPORT_FAILURE("The property '{0}' is read-only.", WAbstractProperty::GetPropertyName());
  }

private:
  GetConstContainerFunc m_ConstGetter;
};
