#pragma once

#include <Core/CoreDLL.h>
#include <Core/ResourceManager/ResourceManager.h>

/// Adds two member functions to a class, GetXyzFile() and SetXyzFile() with Xyz being equal to 'name', which allow to access the handle through strings.
///
/// This macro is just for convenience, so that one doesn't need to write this boilerplate code by hand for every resource handle that
/// should be exposed through the reflection system.
/// The accessors still need to be exposed to the reflection system like this:
///
/// W_ACCESSOR_PROPERTY("XyzResource", GetXyzFile, SetXyzFile)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Xyz")),
///
#define W_ADD_RESOURCEHANDLE_ACCESSORS(name, member)                                  \
  void Set##name##File(WStringView sFile)                                             \
  {                                                                                    \
    if (!sFile.IsEmpty())                                                              \
    {                                                                                  \
      member = WResourceManager::LoadResource<decltype(member)::ResourceType>(sFile); \
    }                                                                                  \
    else                                                                               \
    {                                                                                  \
      member = {};                                                                     \
    }                                                                                  \
  }                                                                                    \
                                                                                       \
  WStringView Get##name##File() const                                                 \
  {                                                                                    \
    return member.GetResourceID();                                                     \
  }

/// Same as W_ADD_RESOURCEHANDLE_ACCESSORS, but calls 'setterFunc' instead of assigning to 'member' directly.
///
/// This can be used, if the setter should do additional validation or bookkeeping.
#define W_ADD_RESOURCEHANDLE_ACCESSORS_WITH_SETTER(name, member, setterFunc)             \
  void Set##name##File(WStringView sFile)                                                \
  {                                                                                       \
    if (!sFile.IsEmpty())                                                                 \
    {                                                                                     \
      setterFunc(WResourceManager::LoadResource<decltype(member)::ResourceType>(sFile)); \
    }                                                                                     \
    else                                                                                  \
    {                                                                                     \
      setterFunc({});                                                                     \
    }                                                                                     \
  }                                                                                       \
                                                                                          \
  WStringView Get##name##File() const                                                    \
  {                                                                                       \
    return member.GetResourceID();                                                        \
  }


/// [internal] Helper class to generate accessor functions for (private) resource handle members
template <typename Class, typename Type, Type Class::*Member>
struct WResourceHandlePropertyAccessor
{
  static WStringView GetValue(const Class* pInstance) { return ((*pInstance).*Member).GetResourceID(); }

  static void SetValue(Class* pInstance, WStringView value)
  {
    if (!value.IsEmpty())
    {
      (*pInstance).*Member = WResourceManager::LoadResource<typename Type::ResourceType>(value);
    }
    else
    {
      (*pInstance).*Member = {};
    }
  }

  static void* GetPropertyPointer(const Class* pInstance)
  {
    W_IGNORE_UNUSED(pInstance);

    // No access to sub-properties
    return nullptr;
  }
};

/// Similar to W_MEMBER_PROPERTY, but makes it convenient to expose resource handle properties
#define W_RESOURCE_MEMBER_PROPERTY(PropertyName, MemberName)                                                        \
  (new WMemberProperty<OwnType, WStringView>(PropertyName,                                                         \
    &WResourceHandlePropertyAccessor<OwnType, W_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetValue, \
    &WResourceHandlePropertyAccessor<OwnType, W_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::SetValue, \
    &WResourceHandlePropertyAccessor<OwnType, W_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetPropertyPointer))



/// [internal] An implementation of WTypedMemberProperty that uses custom getter / setter functions to access a property.
template <typename Class, typename Type>
class WResourceAccessorProperty : public WTypedMemberProperty<WStringView>
{
public:
  using RealType = WStringView;
  using HandleType = typename WTypeTraits<Type>::NonConstReferenceType;
  using ResourceType = typename HandleType::ResourceType;
  using GetterFunc = Type (Class::*)() const;
  using SetterFunc = void (Class::*)(Type value);

  WResourceAccessorProperty(const char* szPropertyName, GetterFunc getter, SetterFunc setter)
    : WTypedMemberProperty<RealType>(szPropertyName)
  {
    W_ASSERT_DEBUG(getter != nullptr, "The getter of a property cannot be nullptr.");

    m_Getter = getter;
    m_Setter = setter;

    if (m_Setter == nullptr)
      WAbstractMemberProperty::m_Flags.Add(WPropertyFlags::ReadOnly);
  }

  virtual void* GetPropertyPointer(const void* pInstance) const override
  {
    W_IGNORE_UNUSED(pInstance);

    // No access to sub-properties, if we have accessors for this property
    return nullptr;
  }

  virtual RealType GetValue(const void* pInstance) const override // [tested]
  {
    return (static_cast<const Class*>(pInstance)->*m_Getter)().GetResourceID();
  }

  virtual void SetValue(void* pInstance, RealType value) const override // [tested]
  {
    W_ASSERT_DEV(m_Setter != nullptr, "The property '{0}' has no setter function, thus it is read-only.", WAbstractProperty::GetPropertyName());

    if (m_Setter)
    {
      if (!value.IsEmpty())
      {
        (static_cast<Class*>(pInstance)->*m_Setter)(WResourceManager::LoadResource<ResourceType>(value));
      }
      else
      {
        (static_cast<Class*>(pInstance)->*m_Setter)({});
      }
    }
  }

private:
  GetterFunc m_Getter;
  SetterFunc m_Setter;
};

/// Similar to W_RESOURCE_MEMBER_PROPERTY, but takes a getter and setter function that access the resource handle.
///
/// This can be used to control what other things should happen, if a handle gets modified.
#define W_RESOURCE_ACCESSOR_PROPERTY(PropertyName, Getter, Setter) \
  (new WResourceAccessorProperty<OwnType, W_GETTER_TYPE(OwnType, OwnType::Getter)>(PropertyName, &OwnType::Getter, &OwnType::Setter))
