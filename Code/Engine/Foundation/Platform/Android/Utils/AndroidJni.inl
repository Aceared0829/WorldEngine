struct WJniModifiers
{
  enum Enum
  {
    PUBLIC = 1,
    PRIVATE = 2,
    PROTECTED = 4,
    STATIC = 8,
    FINAL = 16,
    SYNCHRONIZED = 32,
    VOLATILE = 64,
    TRANSIENT = 128,
    NATIVE = 256,
    INTERFACE = 512,
    ABSTRACT = 1024,
    STRICT = 2048,
  };
};

WJniObject::WJniObject(jobject object, WJniOwnerShip ownerShip)
  : m_class(nullptr)
{
  switch (ownerShip)
  {
    case WJniOwnerShip::OWN:
      m_object = object;
      m_own = true;
      break;

    case WJniOwnerShip::COPY:
      m_object = WJniAttachment::GetEnv()->NewLocalRef(object);
      m_own = true;
      break;

    case WJniOwnerShip::BORROW:
      m_object = object;
      m_own = false;
      break;
  }
}

WJniObject::WJniObject(const WJniObject& other)
  : m_class(nullptr)
{
  m_object = WJniAttachment::GetEnv()->NewLocalRef(other.m_object);
  m_own = true;
}

WJniObject::WJniObject(WJniObject&& other)
{
  m_object = other.m_object;
  m_class = other.m_class;
  m_own = other.m_own;

  other.m_object = nullptr;
  other.m_class = nullptr;
  other.m_own = false;
}

WJniObject& WJniObject::operator=(const WJniObject& other)
{
  if (this == &other)
    return *this;

  Reset();
  m_object = WJniAttachment::GetEnv()->NewLocalRef(other.m_object);
  m_own = true;
  return *this;
}

WJniObject& WJniObject::operator=(WJniObject&& other)
{
  if (this == &other)
    return *this;

  Reset();

  m_object = other.m_object;
  m_class = other.m_class;
  m_own = other.m_own;

  other.m_object = nullptr;
  other.m_class = nullptr;
  other.m_own = false;

  return *this;
}

WJniObject::~WJniObject()
{
  Reset();
}

void WJniObject::Reset()
{
  if (m_object && m_own)
  {
    WJniAttachment::GetEnv()->DeleteLocalRef(m_object);
    m_object = nullptr;
    m_own = false;
  }
  if (m_class)
  {
    WJniAttachment::GetEnv()->DeleteLocalRef(m_class);
    m_class = nullptr;
  }
}

jobject WJniObject::GetJObject() const
{
  return m_object;
}

bool WJniObject::operator==(const WJniObject& other) const
{
  return WJniAttachment::GetEnv()->IsSameObject(m_object, other.m_object) == JNI_TRUE;
}

bool WJniObject::operator!=(const WJniObject& other) const
{
  return !operator==(other);
}

// Template specializations to dispatch to the correct JNI method for each C++ type.
template <typename T, bool unused = false>
struct WJniTraits
{
  static_assert(unused, "The passed C++ type is not supported by the JNI wrapper. Arguments and returns types must be one of bool, signed char/jbyte, unsigned short/jchar, short/jshort, int/jint, long long/jlong, float/jfloat, double/jdouble, WJniObject, WJniString or WJniClass.");

  // Places the argument inside a jvalue union.
  static jvalue ToValue(T);

  // Retrieves the Java class static type of the argument. For primitives, this is not the boxed type, but the primitive type.
  static WJniClass GetStaticType();

  // Retrieves the Java class dynamic type of the argument. For primitives, this is not the boxed type, but the primitive type.
  static WJniClass GetRuntimeType(T);

  // Creates an invalid/null object to return in case of errors.
  static T GetEmptyObject();

  // Call an instance method with the return type.
  template <typename... Args>
  static T CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  // Call a static method with the return type.
  template <typename... Args>
  static T CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  // Sets/gets a field of the type.
  static void SetField(jobject self, jfieldID field, T);
  static T GetField(jobject self, jfieldID field);

  // Sets/gets a static field of the type.
  static void SetStaticField(jclass clazz, jfieldID field, T);
  static T GetStaticField(jclass clazz, jfieldID field);

  // Appends the JNI type signature of this type to the string buf
  static bool AppendSignature(const T& obj, WStringBuilder& str);
  static const char* GetSignatureStatic();
};

template <>
struct WJniTraits<bool>
{
  static inline jvalue ToValue(bool value);

  static inline WJniClass GetStaticType();

  static inline WJniClass GetRuntimeType(bool);

  static inline bool GetEmptyObject();

  template <typename... Args>
  static bool CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static bool CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, bool arg);
  static inline bool GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, bool arg);
  static inline bool GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(bool, WStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct WJniTraits<jbyte>
{
  static inline jvalue ToValue(jbyte value);

  static inline WJniClass GetStaticType();

  static inline WJniClass GetRuntimeType(jbyte);

  static inline jbyte GetEmptyObject();

  template <typename... Args>
  static jbyte CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static jbyte CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, jbyte arg);
  static inline jbyte GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, jbyte arg);
  static inline jbyte GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(jbyte, WStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct WJniTraits<jchar>
{
  static inline jvalue ToValue(jchar value);

  static inline WJniClass GetStaticType();

  static inline WJniClass GetRuntimeType(jchar);

  static inline jchar GetEmptyObject();

  template <typename... Args>
  static jchar CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static jchar CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, jchar arg);
  static inline jchar GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, jchar arg);
  static inline jchar GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(jchar, WStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct WJniTraits<jshort>
{
  static inline jvalue ToValue(jshort value);

  static inline WJniClass GetStaticType();

  static inline WJniClass GetRuntimeType(jshort);

  static inline jshort GetEmptyObject();

  template <typename... Args>
  static jshort CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static jshort CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, jshort arg);
  static inline jshort GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, jshort arg);
  static inline jshort GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(jshort, WStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct WJniTraits<jint>
{
  static inline jvalue ToValue(jint value);

  static inline WJniClass GetStaticType();

  static inline WJniClass GetRuntimeType(jint);

  static inline jint GetEmptyObject();

  template <typename... Args>
  static jint CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static jint CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, jint arg);
  static inline jint GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, jint arg);
  static inline jint GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(jint, WStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct WJniTraits<jlong>
{
  static inline jvalue ToValue(jlong value);

  static inline WJniClass GetStaticType();

  static inline WJniClass GetRuntimeType(jlong);

  static inline jlong GetEmptyObject();

  template <typename... Args>
  static jlong CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static jlong CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, jlong arg);
  static inline jlong GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, jlong arg);
  static inline jlong GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(jlong, WStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct WJniTraits<jfloat>
{
  static inline jvalue ToValue(jfloat value);

  static inline WJniClass GetStaticType();

  static inline WJniClass GetRuntimeType(jfloat);

  static inline jfloat GetEmptyObject();

  template <typename... Args>
  static jfloat CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static jfloat CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, jfloat arg);
  static inline jfloat GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, jfloat arg);
  static inline jfloat GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(jfloat, WStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct WJniTraits<jdouble>
{
  static inline jvalue ToValue(jdouble value);

  static inline WJniClass GetStaticType();

  static inline WJniClass GetRuntimeType(jdouble);

  static inline jdouble GetEmptyObject();

  template <typename... Args>
  static jdouble CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static jdouble CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, jdouble arg);
  static inline jdouble GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, jdouble arg);
  static inline jdouble GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(jdouble, WStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct WJniTraits<WJniObject>
{
  static inline jvalue ToValue(const WJniObject& object);

  static inline WJniClass GetStaticType();

  static inline WJniClass GetRuntimeType(const WJniObject& object);

  static inline WJniObject GetEmptyObject();

  template <typename... Args>
  static WJniObject CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static WJniObject CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, const WJniObject& arg);
  static inline WJniObject GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, const WJniObject& arg);
  static inline WJniObject GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(const WJniObject& obj, WStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct WJniTraits<WJniClass>
{
  static inline jvalue ToValue(const WJniClass& object);

  static inline WJniClass GetStaticType();

  static inline WJniClass GetRuntimeType(const WJniClass& object);

  static inline WJniClass GetEmptyObject();

  template <typename... Args>
  static WJniClass CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static WJniClass CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, const WJniClass& arg);
  static inline WJniClass GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, const WJniClass& arg);
  static inline WJniClass GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(const WJniClass& obj, WStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct WJniTraits<WJniString>
{
  static inline jvalue ToValue(const WJniString& object);

  static inline WJniClass GetStaticType();

  static inline WJniClass GetRuntimeType(const WJniString& object);

  static inline WJniString GetEmptyObject();

  template <typename... Args>
  static WJniString CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static WJniString CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, const WJniString& arg);
  static inline WJniString GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, const WJniString& arg);
  static inline WJniString GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(const WJniString& obj, WStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct WJniTraits<void>
{
  static inline WJniClass GetStaticType();

  static inline void GetEmptyObject();

  template <typename... Args>
  static void CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static void CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline const char* GetSignatureStatic();
};

// Helpers to unpack variadic templates.
struct WJniImpl
{
  static void CollectArgumentTypes(WJniClass* target)
  {
  }

  template <typename T, typename... Tail>
  static void CollectArgumentTypes(WJniClass* target, const T& arg, const Tail&... tail)
  {
    *target = WJniTraits<T>::GetRuntimeType(arg);
    return WJniImpl::CollectArgumentTypes(target + 1, tail...);
  }

  static void UnpackArgs(jvalue* target)
  {
  }

  template <typename T, typename... Tail>
  static void UnpackArgs(jvalue* target, const T& arg, const Tail&... tail)
  {
    *target = WJniTraits<T>::ToValue(arg);
    return UnpackArgs(target + 1, tail...);
  }

  template <typename Ret, typename... Args>
  static bool BuildMethodSignature(WStringBuilder& signature, const Args&... args)
  {
    signature.Append("(");
    if (!WJniImpl::AppendSignature(signature, args...))
    {
      return false;
    }
    signature.Append(")");
    signature.Append(WJniTraits<Ret>::GetSignatureStatic());
    return true;
  }

  static bool AppendSignature(WStringBuilder& signature)
  {
    return true;
  }

  template <typename T, typename... Tail>
  static bool AppendSignature(WStringBuilder& str, const T& arg, const Tail&... tail)
  {
    return WJniTraits<T>::AppendSignature(arg, str) && AppendSignature(str, tail...);
  }
};

jvalue WJniTraits<bool>::ToValue(bool value)
{
  jvalue result;
  result.z = value ? JNI_TRUE : JNI_FALSE;
  return result;
}

WJniClass WJniTraits<bool>::GetStaticType()
{
  return WJniClass("java/lang/Boolean").UnsafeGetStaticField<WJniClass>("TYPE", "Ljava/lang/Class;");
}

WJniClass WJniTraits<bool>::GetRuntimeType(bool)
{
  return GetStaticType();
}

bool WJniTraits<bool>::GetEmptyObject()
{
  return false;
}

template <typename... Args>
bool WJniTraits<bool>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  WJniImpl::UnpackArgs(array, args...);
  return WJniAttachment::GetEnv()->CallBooleanMethodA(self, method, array) == JNI_TRUE;
}

template <typename... Args>
bool WJniTraits<bool>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  WJniImpl::UnpackArgs(array, args...);
  return WJniAttachment::GetEnv()->CallStaticBooleanMethodA(clazz, method, array) == JNI_TRUE;
}

void WJniTraits<bool>::SetField(jobject self, jfieldID field, bool arg)
{
  return WJniAttachment::GetEnv()->SetBooleanField(self, field, arg ? JNI_TRUE : JNI_FALSE);
}

bool WJniTraits<bool>::GetField(jobject self, jfieldID field)
{
  return WJniAttachment::GetEnv()->GetBooleanField(self, field) == JNI_TRUE;
}

void WJniTraits<bool>::SetStaticField(jclass clazz, jfieldID field, bool arg)
{
  return WJniAttachment::GetEnv()->SetStaticBooleanField(clazz, field, arg ? JNI_TRUE : JNI_FALSE);
}

bool WJniTraits<bool>::GetStaticField(jclass clazz, jfieldID field)
{
  return WJniAttachment::GetEnv()->GetStaticBooleanField(clazz, field) == JNI_TRUE;
}

bool WJniTraits<bool>::AppendSignature(bool, WStringBuilder& str)
{
  str.Append("Z");
  return true;
}

const char* WJniTraits<bool>::GetSignatureStatic()
{
  return "Z";
}

jvalue WJniTraits<jbyte>::ToValue(jbyte value)
{
  jvalue result;
  result.b = value;
  return result;
}

WJniClass WJniTraits<jbyte>::GetStaticType()
{
  return WJniClass("java/lang/Byte").UnsafeGetStaticField<WJniClass>("TYPE", "Ljava/lang/Class;");
}

WJniClass WJniTraits<jbyte>::GetRuntimeType(jbyte)
{
  return GetStaticType();
}

jbyte WJniTraits<jbyte>::GetEmptyObject()
{
  return 0;
}

template <typename... Args>
jbyte WJniTraits<jbyte>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  WJniImpl::UnpackArgs(array, args...);
  return WJniAttachment::GetEnv()->CallByteMethodA(self, method, array);
}

template <typename... Args>
jbyte WJniTraits<jbyte>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  WJniImpl::UnpackArgs(array, args...);
  return WJniAttachment::GetEnv()->CallStaticByteMethodA(clazz, method, array);
}

void WJniTraits<jbyte>::SetField(jobject self, jfieldID field, jbyte arg)
{
  return WJniAttachment::GetEnv()->SetByteField(self, field, arg);
}

jbyte WJniTraits<jbyte>::GetField(jobject self, jfieldID field)
{
  return WJniAttachment::GetEnv()->GetByteField(self, field);
}

void WJniTraits<jbyte>::SetStaticField(jclass clazz, jfieldID field, jbyte arg)
{
  return WJniAttachment::GetEnv()->SetStaticByteField(clazz, field, arg);
}

jbyte WJniTraits<jbyte>::GetStaticField(jclass clazz, jfieldID field)
{
  return WJniAttachment::GetEnv()->GetStaticByteField(clazz, field);
}

bool WJniTraits<jbyte>::AppendSignature(jbyte, WStringBuilder& str)
{
  str.Append("B");
  return true;
}

const char* WJniTraits<jbyte>::GetSignatureStatic()
{
  return "B";
}

jvalue WJniTraits<jchar>::ToValue(jchar value)
{
  jvalue result;
  result.c = value;
  return result;
}

WJniClass WJniTraits<jchar>::GetStaticType()
{
  return WJniClass("java/lang/Character").UnsafeGetStaticField<WJniClass>("TYPE", "Ljava/lang/Class;");
}

WJniClass WJniTraits<jchar>::GetRuntimeType(jchar)
{
  return GetStaticType();
}

jchar WJniTraits<jchar>::GetEmptyObject()
{
  return 0;
}

template <typename... Args>
jchar WJniTraits<jchar>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  WJniImpl::UnpackArgs(array, args...);
  return WJniAttachment::GetEnv()->CallCharMethodA(self, method, array);
}

template <typename... Args>
jchar WJniTraits<jchar>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  WJniImpl::UnpackArgs(array, args...);
  return WJniAttachment::GetEnv()->CallStaticCharMethodA(clazz, method, array);
}

void WJniTraits<jchar>::SetField(jobject self, jfieldID field, jchar arg)
{
  return WJniAttachment::GetEnv()->SetCharField(self, field, arg);
}

jchar WJniTraits<jchar>::GetField(jobject self, jfieldID field)
{
  return WJniAttachment::GetEnv()->GetCharField(self, field);
}

void WJniTraits<jchar>::SetStaticField(jclass clazz, jfieldID field, jchar arg)
{
  return WJniAttachment::GetEnv()->SetStaticCharField(clazz, field, arg);
}

jchar WJniTraits<jchar>::GetStaticField(jclass clazz, jfieldID field)
{
  return WJniAttachment::GetEnv()->GetStaticCharField(clazz, field);
}

bool WJniTraits<jchar>::AppendSignature(jchar, WStringBuilder& str)
{
  str.Append("C");
  return true;
}

const char* WJniTraits<jchar>::GetSignatureStatic()
{
  return "C";
}

jvalue WJniTraits<jshort>::ToValue(jshort value)
{
  jvalue result;
  result.s = value;
  return result;
}

WJniClass WJniTraits<jshort>::GetStaticType()
{
  return WJniClass("java/lang/Short").UnsafeGetStaticField<WJniClass>("TYPE", "Ljava/lang/Class;");
}

WJniClass WJniTraits<jshort>::GetRuntimeType(jshort)
{
  return GetStaticType();
}

jshort WJniTraits<jshort>::GetEmptyObject()
{
  return 0;
}

template <typename... Args>
jshort WJniTraits<jshort>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  WJniImpl::UnpackArgs(array, args...);
  return WJniAttachment::GetEnv()->CallShortMethodA(self, method, array);
}

template <typename... Args>
jshort WJniTraits<jshort>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  WJniImpl::UnpackArgs(array, args...);
  return WJniAttachment::GetEnv()->CallStaticShortMethodA(clazz, method, array);
}

void WJniTraits<jshort>::SetField(jobject self, jfieldID field, jshort arg)
{
  return WJniAttachment::GetEnv()->SetShortField(self, field, arg);
}

jshort WJniTraits<jshort>::GetField(jobject self, jfieldID field)
{
  return WJniAttachment::GetEnv()->GetShortField(self, field);
}

void WJniTraits<jshort>::SetStaticField(jclass clazz, jfieldID field, jshort arg)
{
  return WJniAttachment::GetEnv()->SetStaticShortField(clazz, field, arg);
}

jshort WJniTraits<jshort>::GetStaticField(jclass clazz, jfieldID field)
{
  return WJniAttachment::GetEnv()->GetStaticShortField(clazz, field);
}

bool WJniTraits<jshort>::AppendSignature(jshort, WStringBuilder& str)
{
  str.Append("S");
  return true;
}

const char* WJniTraits<jshort>::GetSignatureStatic()
{
  return "S";
}

jvalue WJniTraits<jint>::ToValue(jint value)
{
  jvalue result;
  result.i = value;
  return result;
}

WJniClass WJniTraits<jint>::GetStaticType()
{
  return WJniClass("java/lang/Integer").UnsafeGetStaticField<WJniClass>("TYPE", "Ljava/lang/Class;");
}

WJniClass WJniTraits<jint>::GetRuntimeType(jint)
{
  return GetStaticType();
}

jint WJniTraits<jint>::GetEmptyObject()
{
  return 0;
}

template <typename... Args>
jint WJniTraits<jint>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  WJniImpl::UnpackArgs(array, args...);
  return WJniAttachment::GetEnv()->CallIntMethodA(self, method, array);
}

template <typename... Args>
jint WJniTraits<jint>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  WJniImpl::UnpackArgs(array, args...);
  return WJniAttachment::GetEnv()->CallStaticIntMethodA(clazz, method, array);
}

void WJniTraits<jint>::SetField(jobject self, jfieldID field, jint arg)
{
  return WJniAttachment::GetEnv()->SetIntField(self, field, arg);
}

jint WJniTraits<jint>::GetField(jobject self, jfieldID field)
{
  return WJniAttachment::GetEnv()->GetIntField(self, field);
}

void WJniTraits<jint>::SetStaticField(jclass clazz, jfieldID field, jint arg)
{
  return WJniAttachment::GetEnv()->SetStaticIntField(clazz, field, arg);
}

jint WJniTraits<jint>::GetStaticField(jclass clazz, jfieldID field)
{
  return WJniAttachment::GetEnv()->GetStaticIntField(clazz, field);
}

bool WJniTraits<jint>::AppendSignature(jint, WStringBuilder& str)
{
  str.Append("I");
  return true;
}

const char* WJniTraits<jint>::GetSignatureStatic()
{
  return "I";
}

jvalue WJniTraits<jlong>::ToValue(jlong value)
{
  jvalue result;
  result.j = value;
  return result;
}

WJniClass WJniTraits<jlong>::GetStaticType()
{
  return WJniClass("java/lang/Long").UnsafeGetStaticField<WJniClass>("TYPE", "Ljava/lang/Class;");
}

WJniClass WJniTraits<jlong>::GetRuntimeType(jlong)
{
  return GetStaticType();
}

jlong WJniTraits<jlong>::GetEmptyObject()
{
  return 0;
}

template <typename... Args>
jlong WJniTraits<jlong>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  WJniImpl::UnpackArgs(array, args...);
  return WJniAttachment::GetEnv()->CallLongMethodA(self, method, array);
}

template <typename... Args>
jlong WJniTraits<jlong>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  WJniImpl::UnpackArgs(array, args...);
  return WJniAttachment::GetEnv()->CallStaticLongMethodA(clazz, method, array);
}

void WJniTraits<jlong>::SetField(jobject self, jfieldID field, jlong arg)
{
  return WJniAttachment::GetEnv()->SetLongField(self, field, arg);
}

jlong WJniTraits<jlong>::GetField(jobject self, jfieldID field)
{
  return WJniAttachment::GetEnv()->GetLongField(self, field);
}

void WJniTraits<jlong>::SetStaticField(jclass clazz, jfieldID field, jlong arg)
{
  return WJniAttachment::GetEnv()->SetStaticLongField(clazz, field, arg);
}

jlong WJniTraits<jlong>::GetStaticField(jclass clazz, jfieldID field)
{
  return WJniAttachment::GetEnv()->GetStaticLongField(clazz, field);
}

bool WJniTraits<jlong>::AppendSignature(jlong, WStringBuilder& str)
{
  str.Append("J");
  return true;
}

const char* WJniTraits<jlong>::GetSignatureStatic()
{
  return "J";
}

jvalue WJniTraits<jfloat>::ToValue(jfloat value)
{
  jvalue result;
  result.f = value;
  return result;
}

WJniClass WJniTraits<jfloat>::GetStaticType()
{
  return WJniClass("java/lang/Float").UnsafeGetStaticField<WJniClass>("TYPE", "Ljava/lang/Class;");
}

WJniClass WJniTraits<jfloat>::GetRuntimeType(jfloat)
{
  return GetStaticType();
}

jfloat WJniTraits<jfloat>::GetEmptyObject()
{
  return nanf("");
}

template <typename... Args>
jfloat WJniTraits<jfloat>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  WJniImpl::UnpackArgs(array, args...);
  return WJniAttachment::GetEnv()->CallFloatMethodA(self, method, array);
}

template <typename... Args>
jfloat WJniTraits<jfloat>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  WJniImpl::UnpackArgs(array, args...);
  return WJniAttachment::GetEnv()->CallStaticFloatMethodA(clazz, method, array);
}

void WJniTraits<jfloat>::SetField(jobject self, jfieldID field, jfloat arg)
{
  return WJniAttachment::GetEnv()->SetFloatField(self, field, arg);
}

jfloat WJniTraits<jfloat>::GetField(jobject self, jfieldID field)
{
  return WJniAttachment::GetEnv()->GetFloatField(self, field);
}

void WJniTraits<jfloat>::SetStaticField(jclass clazz, jfieldID field, jfloat arg)
{
  return WJniAttachment::GetEnv()->SetStaticFloatField(clazz, field, arg);
}

jfloat WJniTraits<jfloat>::GetStaticField(jclass clazz, jfieldID field)
{
  return WJniAttachment::GetEnv()->GetStaticFloatField(clazz, field);
}

bool WJniTraits<jfloat>::AppendSignature(jfloat, WStringBuilder& str)
{
  str.Append("F");
  return true;
}

const char* WJniTraits<jfloat>::GetSignatureStatic()
{
  return "F";
}

jvalue WJniTraits<jdouble>::ToValue(jdouble value)
{
  jvalue result;
  result.d = value;
  return result;
}

WJniClass WJniTraits<jdouble>::GetStaticType()
{
  return WJniClass("java/lang/Double").UnsafeGetStaticField<WJniClass>("TYPE", "Ljava/lang/Class;");
}

WJniClass WJniTraits<jdouble>::GetRuntimeType(jdouble)
{
  return GetStaticType();
}

jdouble WJniTraits<jdouble>::GetEmptyObject()
{
  return nan("");
}

template <typename... Args>
jdouble WJniTraits<jdouble>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  WJniImpl::UnpackArgs(array, args...);
  return WJniAttachment::GetEnv()->CallDoubleMethodA(self, method, array);
}

template <typename... Args>
jdouble WJniTraits<jdouble>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  WJniImpl::UnpackArgs(array, args...);
  return WJniAttachment::GetEnv()->CallStaticDoubleMethodA(clazz, method, array);
}

void WJniTraits<jdouble>::SetField(jobject self, jfieldID field, jdouble arg)
{
  return WJniAttachment::GetEnv()->SetDoubleField(self, field, arg);
}

jdouble WJniTraits<jdouble>::GetField(jobject self, jfieldID field)
{
  return WJniAttachment::GetEnv()->GetDoubleField(self, field);
}

void WJniTraits<jdouble>::SetStaticField(jclass clazz, jfieldID field, jdouble arg)
{
  return WJniAttachment::GetEnv()->SetStaticDoubleField(clazz, field, arg);
}

jdouble WJniTraits<jdouble>::GetStaticField(jclass clazz, jfieldID field)
{
  return WJniAttachment::GetEnv()->GetStaticDoubleField(clazz, field);
}

bool WJniTraits<jdouble>::AppendSignature(jdouble, WStringBuilder& str)
{
  str.Append("D");
  return true;
}

const char* WJniTraits<jdouble>::GetSignatureStatic()
{
  return "D";
}

jvalue WJniTraits<WJniObject>::ToValue(const WJniObject& value)
{
  jvalue result;
  result.l = value.GetHandle();
  return result;
}

WJniClass WJniTraits<WJniObject>::GetStaticType()
{
  return WJniClass("java/lang/Object");
}

WJniClass WJniTraits<WJniObject>::GetRuntimeType(const WJniObject& arg)
{
  return arg.GetClass();
}

WJniObject WJniTraits<WJniObject>::GetEmptyObject()
{
  return WJniObject();
}

template <typename... Args>
WJniObject WJniTraits<WJniObject>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  WJniImpl::UnpackArgs(array, args...);
  return WJniObject(WJniAttachment::GetEnv()->CallObjectMethodA(self, method, array), WJniOwnerShip::OWN);
}

template <typename... Args>
WJniObject WJniTraits<WJniObject>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  WJniImpl::UnpackArgs(array, args...);
  return WJniObject(WJniAttachment::GetEnv()->CallStaticObjectMethodA(clazz, method, array), WJniOwnerShip::OWN);
}

void WJniTraits<WJniObject>::SetField(jobject self, jfieldID field, const WJniObject& arg)
{
  return WJniAttachment::GetEnv()->SetObjectField(self, field, arg.GetHandle());
}

WJniObject WJniTraits<WJniObject>::GetField(jobject self, jfieldID field)
{
  return WJniObject(WJniAttachment::GetEnv()->GetObjectField(self, field), WJniOwnerShip::OWN);
}

void WJniTraits<WJniObject>::SetStaticField(jclass clazz, jfieldID field, const WJniObject& arg)
{
  return WJniAttachment::GetEnv()->SetStaticObjectField(clazz, field, arg.GetHandle());
}

WJniObject WJniTraits<WJniObject>::GetStaticField(jclass clazz, jfieldID field)
{
  return WJniObject(WJniAttachment::GetEnv()->GetStaticObjectField(clazz, field), WJniOwnerShip::OWN);
}

bool WJniTraits<WJniObject>::AppendSignature(const WJniObject& obj, WStringBuilder& str)
{
  if (obj.IsNull())
  {
    // Ensure null objects never generate valid signatures in order to force using the reflection path
    return false;
  }
  else
  {
    str.Append("L");
    str.Append(obj.GetClass().UnsafeCall<WJniString>("getName", "()Ljava/lang/String;").GetData());
    str.ReplaceAll(".", "/");
    str.Append(";");
    return true;
  }
}

const char* WJniTraits<WJniObject>::GetSignatureStatic()
{
  return "Ljava/lang/Object;";
}

jvalue WJniTraits<WJniClass>::ToValue(const WJniClass& value)
{
  jvalue result;
  result.l = value.GetHandle();
  return result;
}

WJniClass WJniTraits<WJniClass>::GetStaticType()
{
  return WJniClass("java/lang/Class");
}

WJniClass WJniTraits<WJniClass>::GetRuntimeType(const WJniClass& arg)
{
  // Assume there are no types derived from Class
  return GetStaticType();
}

WJniClass WJniTraits<WJniClass>::GetEmptyObject()
{
  return WJniClass();
}

template <typename... Args>
WJniClass WJniTraits<WJniClass>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  WJniImpl::UnpackArgs(array, args...);
  return WJniClass(jclass(WJniAttachment::GetEnv()->CallObjectMethodA(self, method, array)), WJniOwnerShip::OWN);
}

template <typename... Args>
WJniClass WJniTraits<WJniClass>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  WJniImpl::UnpackArgs(array, args...);
  return WJniClass(jclass(WJniAttachment::GetEnv()->CallStaticObjectMethodA(clazz, method, array)), WJniOwnerShip::OWN);
}

void WJniTraits<WJniClass>::SetField(jobject self, jfieldID field, const WJniClass& arg)
{
  return WJniAttachment::GetEnv()->SetObjectField(self, field, arg.GetHandle());
}

WJniClass WJniTraits<WJniClass>::GetField(jobject self, jfieldID field)
{
  return WJniClass(jclass(WJniAttachment::GetEnv()->GetObjectField(self, field)), WJniOwnerShip::OWN);
}

void WJniTraits<WJniClass>::SetStaticField(jclass clazz, jfieldID field, const WJniClass& arg)
{
  return WJniAttachment::GetEnv()->SetStaticObjectField(clazz, field, arg.GetHandle());
}

WJniClass WJniTraits<WJniClass>::GetStaticField(jclass clazz, jfieldID field)
{
  return WJniClass(jclass(WJniAttachment::GetEnv()->GetStaticObjectField(clazz, field)), WJniOwnerShip::OWN);
}

bool WJniTraits<WJniClass>::AppendSignature(const WJniClass& obj, WStringBuilder& str)
{
  str.Append("Ljava/lang/Class;");
  return true;
}

const char* WJniTraits<WJniClass>::GetSignatureStatic()
{
  return "Ljava/lang/Class;";
}

jvalue WJniTraits<WJniString>::ToValue(const WJniString& value)
{
  jvalue result;
  result.l = value.GetHandle();
  return result;
}

WJniClass WJniTraits<WJniString>::GetStaticType()
{
  return WJniClass("java/lang/String");
}

WJniClass WJniTraits<WJniString>::GetRuntimeType(const WJniString& arg)
{
  // Assume there are no types derived from String
  return GetStaticType();
}

WJniString WJniTraits<WJniString>::GetEmptyObject()
{
  return WJniString();
}

template <typename... Args>
WJniString WJniTraits<WJniString>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  WJniImpl::UnpackArgs(array, args...);
  return WJniString(jstring(WJniAttachment::GetEnv()->CallObjectMethodA(self, method, array)), WJniOwnerShip::OWN);
}

template <typename... Args>
WJniString WJniTraits<WJniString>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  WJniImpl::UnpackArgs(array, args...);
  return WJniString(jstring(WJniAttachment::GetEnv()->CallStaticObjectMethodA(clazz, method, array)), WJniOwnerShip::OWN);
}

void WJniTraits<WJniString>::SetField(jobject self, jfieldID field, const WJniString& arg)
{
  return WJniAttachment::GetEnv()->SetObjectField(self, field, arg.GetHandle());
}

WJniString WJniTraits<WJniString>::GetField(jobject self, jfieldID field)
{
  return WJniString(jstring(WJniAttachment::GetEnv()->GetObjectField(self, field)), WJniOwnerShip::OWN);
}

void WJniTraits<WJniString>::SetStaticField(jclass clazz, jfieldID field, const WJniString& arg)
{
  return WJniAttachment::GetEnv()->SetStaticObjectField(clazz, field, arg.GetHandle());
}

WJniString WJniTraits<WJniString>::GetStaticField(jclass clazz, jfieldID field)
{
  return WJniString(jstring(WJniAttachment::GetEnv()->GetStaticObjectField(clazz, field)), WJniOwnerShip::OWN);
}

bool WJniTraits<WJniString>::AppendSignature(const WJniString& obj, WStringBuilder& str)
{
  str.Append("Ljava/lang/String;");
  return true;
}

const char* WJniTraits<WJniString>::GetSignatureStatic()
{
  return "Ljava/lang/String;";
}

WJniClass WJniTraits<void>::GetStaticType()
{
  return WJniClass("java/lang/Void").UnsafeGetStaticField<WJniClass>("TYPE", "Ljava/lang/Class;");
}

void WJniTraits<void>::GetEmptyObject()
{
  return;
}

template <typename... Args>
void WJniTraits<void>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  WJniImpl::UnpackArgs(array, args...);
  return WJniAttachment::GetEnv()->CallVoidMethodA(self, method, array);
}

template <typename... Args>
void WJniTraits<void>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  WJniImpl::UnpackArgs(array, args...);
  return WJniAttachment::GetEnv()->CallStaticVoidMethodA(clazz, method, array);
}

const char* WJniTraits<void>::GetSignatureStatic()
{
  return "V";
}

template <typename... Args>
WJniObject WJniClass::CreateInstance(const Args&... args) const
{
  if (WJniAttachment::FailOnPendingErrorOrException())
  {
    return WJniObject();
  }

  const size_t N = sizeof...(args);

  WJniClass inputTypes[N];
  WJniImpl::CollectArgumentTypes(inputTypes, args...);

  WJniObject foundMethod = FindConstructor(*this, inputTypes, N);

  if (foundMethod.IsNull())
  {
    return WJniObject();
  }

  jmethodID method = WJniAttachment::GetEnv()->FromReflectedMethod(foundMethod.GetHandle());

  jvalue array[sizeof...(args)];
  WJniImpl::UnpackArgs(array, args...);
  return WJniObject(WJniAttachment::GetEnv()->NewObjectA(GetHandle(), method, array), WJniOwnerShip::OWN);
}

template <typename Ret, typename... Args>
Ret WJniClass::CallStatic(const char* name, const Args&... args) const
{
  if (WJniAttachment::FailOnPendingErrorOrException())
  {
    return WJniTraits<Ret>::GetEmptyObject();
  }

  if (!GetJObject())
  {
    WLog::Error("Attempting to call static method '{}' on null class.", name);
    WJniAttachment::SetLastError(WJniErrorState::CALL_ON_NULL_OBJECT);
    return WJniTraits<Ret>::GetEmptyObject();
  }

  WStringBuilder signature;
  if (WJniImpl::BuildMethodSignature<Ret>(signature, args...))
  {
    jmethodID method = WJniAttachment::GetEnv()->GetStaticMethodID(GetHandle(), name, signature.GetData());

    if (method)
    {
      return WJniTraits<Ret>::CallStaticMethod(GetHandle(), method, args...);
    }
    else
    {
      WJniAttachment::GetEnv()->ExceptionClear();
    }
  }

  const size_t N = sizeof...(args);

  WJniClass returnType = WJniTraits<Ret>::GetStaticType();

  WJniClass inputTypes[N];
  WJniImpl::CollectArgumentTypes(inputTypes, args...);

  WJniObject foundMethod = FindMethod(true, name, *this, returnType, inputTypes, N);

  if (foundMethod.IsNull())
  {
    return WJniTraits<Ret>::GetEmptyObject();
  }

  jmethodID method = WJniAttachment::GetEnv()->FromReflectedMethod(foundMethod.GetHandle());
  return WJniTraits<Ret>::CallStaticMethod(GetHandle(), method, args...);
}

template <typename Ret, typename... Args>
Ret WJniClass::UnsafeCallStatic(const char* name, const char* signature, const Args&... args) const
{
  if (!GetJObject())
  {
    WLog::Error("Attempting to call static method '{}' on null class.", name);
    WLog::Error("Attempting to call static method '{}' on null class.", name);
    WJniAttachment::SetLastError(WJniErrorState::CALL_ON_NULL_OBJECT);
    return WJniTraits<Ret>::GetEmptyObject();
  }

  jmethodID method = WJniAttachment::GetEnv()->GetStaticMethodID(GetHandle(), name, signature);
  if (!method)
  {
    WLog::Error("No such static method: '{}' with signature '{}' in class '{}'.", name, signature, ToString().GetData());
    WJniAttachment::SetLastError(WJniErrorState::NO_MATCHING_METHOD);
    return WJniTraits<Ret>::GetEmptyObject();
  }
  else
  {
    return WJniTraits<Ret>::CallStaticMethod(GetHandle(), method, args...);
  }
}

template <typename Ret>
Ret WJniClass::GetStaticField(const char* name) const
{
  if (WJniAttachment::FailOnPendingErrorOrException())
  {
    return WJniTraits<Ret>::GetEmptyObject();
  }

  if (!GetJObject())
  {
    WLog::Error("Attempting to get static field '{}' on null class.", name);
    WJniAttachment::SetLastError(WJniErrorState::CALL_ON_NULL_OBJECT);
    return WJniTraits<Ret>::GetEmptyObject();
  }

  jfieldID fieldID = WJniAttachment::GetEnv()->GetStaticFieldID(GetHandle(), name, WJniTraits<Ret>::GetSignatureStatic());
  if (fieldID)
  {
    return WJniTraits<Ret>::GetStaticField(GetHandle(), fieldID);
  }
  else
  {
    WJniAttachment::GetEnv()->ExceptionClear();
  }

  WJniObject field = UnsafeCall<WJniObject>("getField", "(Ljava/lang/String;)Ljava/lang/reflect/Field;", WJniString(name));

  if (WJniAttachment::GetEnv()->ExceptionOccurred())
  {
    WJniAttachment::GetEnv()->ExceptionClear();

    WLog::Error("No field named '{}' found in class '{}'.", name, ToString().GetData());
    WJniAttachment::SetLastError(WJniErrorState::NO_MATCHING_FIELD);

    return WJniTraits<Ret>::GetEmptyObject();
  }

  if ((field.UnsafeCall<jint>("getModifiers", "()I") & WJniModifiers::STATIC) == 0)
  {
    WLog::Error("Field named '{}' in class '{}' isn't static.", name, ToString().GetData());
    WJniAttachment::SetLastError(WJniErrorState::NO_MATCHING_FIELD);
    return WJniTraits<Ret>::GetEmptyObject();
  }

  WJniClass fieldType = field.UnsafeCall<WJniClass>("getType", "()Ljava/lang/Class;");

  WJniClass returnType = WJniTraits<Ret>::GetStaticType();

  if (!returnType.IsAssignableFrom(fieldType))
  {
    WLog::Error("Field '{}' of type '{}' in class '{}' can't be assigned to return type '{}'.", name, fieldType.ToString().GetData(), ToString().GetData(), returnType.ToString().GetData());
    WJniAttachment::SetLastError(WJniErrorState::NO_MATCHING_FIELD);
    return WJniTraits<Ret>::GetEmptyObject();
  }

  return WJniTraits<Ret>::GetStaticField(GetHandle(), WJniAttachment::GetEnv()->FromReflectedField(field.GetHandle()));
}

template <typename Ret>
Ret WJniClass::UnsafeGetStaticField(const char* name, const char* signature) const
{
  if (!GetJObject())
  {
    WLog::Error("Attempting to get static field '{}' on null class.", name);
    WJniAttachment::SetLastError(WJniErrorState::CALL_ON_NULL_OBJECT);
    return WJniTraits<Ret>::GetEmptyObject();
  }

  jfieldID field = WJniAttachment::GetEnv()->GetStaticFieldID(GetHandle(), name, signature);
  if (!field)
  {
    WLog::Error("No such field: '{}' with signature '{}'.", name, signature);
    WJniAttachment::SetLastError(WJniErrorState::NO_MATCHING_FIELD);
    return WJniTraits<Ret>::GetEmptyObject();
  }
  else
  {
    return WJniTraits<Ret>::GetStaticField(GetHandle(), field);
  }
}

template <typename T>
void WJniClass::SetStaticField(const char* name, const T& arg) const
{
  if (WJniAttachment::FailOnPendingErrorOrException())
  {
    return;
  }

  if (!GetJObject())
  {
    WLog::Error("Attempting to set static field '{}' on null class.", name);
    WJniAttachment::SetLastError(WJniErrorState::CALL_ON_NULL_OBJECT);
    return;
  }

  WJniObject field = UnsafeCall<WJniObject>("getField", "(Ljava/lang/String;)Ljava/lang/reflect/Field;", WJniString(name));

  if (WJniAttachment::GetEnv()->ExceptionOccurred())
  {
    WJniAttachment::GetEnv()->ExceptionClear();

    WLog::Error("No field named '{}' found in class '{}'.", name, ToString().GetData());
    WJniAttachment::SetLastError(WJniErrorState::NO_MATCHING_FIELD);

    return;
  }

  WJniClass modifierClass("java/lang/reflect/Modifier");
  jint modifiers = field.UnsafeCall<jint>("getModifiers", "()I");

  if ((modifiers & WJniModifiers::STATIC) == 0)
  {
    WLog::Error("Field named '{}' in class '{}' isn't static.", name, ToString().GetData());
    WJniAttachment::SetLastError(WJniErrorState::NO_MATCHING_FIELD);
    return;
  }

  if ((modifiers & WJniModifiers::FINAL) != 0)
  {
    WLog::Error("Field named '{}' in class '{}' is final.", name, ToString().GetData());
    WJniAttachment::SetLastError(WJniErrorState::NO_MATCHING_FIELD);
    return;
  }

  WJniClass fieldType = field.UnsafeCall<WJniClass>("getType", "()Ljava/lang/Class;");

  WJniClass argType = WJniTraits<T>::GetRuntimeType(arg);

  if (argType.IsNull())
  {
    if (fieldType.IsPrimitive())
    {
      WLog::Error("Field '{}' of type '{}' can't be assigned null because it is a primitive type.", name, fieldType.ToString().GetData());
      WJniAttachment::SetLastError(WJniErrorState::NO_MATCHING_FIELD);
      return;
    }
  }
  else
  {
    if (!fieldType.IsAssignableFrom(argType))
    {
      WLog::Error("Field '{}' of type '{}' can't be assigned from type '{}'.", name, fieldType.ToString().GetData(), argType.ToString().GetData());
      WJniAttachment::SetLastError(WJniErrorState::NO_MATCHING_FIELD);
      return;
    }
  }

  return WJniTraits<T>::SetStaticField(GetHandle(), WJniAttachment::GetEnv()->FromReflectedField(field.GetHandle()), arg);
}

template <typename T>
void WJniClass::UnsafeSetStaticField(const char* name, const char* signature, const T& arg) const
{
  if (!GetJObject())
  {
    WLog::Error("Attempting to set static field '{}' on null class.", name);
    WJniAttachment::SetLastError(WJniErrorState::CALL_ON_NULL_OBJECT);
    return;
  }

  jfieldID field = WJniAttachment::GetEnv()->GetStaticFieldID(GetHandle(), name, signature);
  if (!field)
  {
    WLog::Error("No such field: '{}' with signature '{}'.", name, signature);
    WJniAttachment::SetLastError(WJniErrorState::NO_MATCHING_FIELD);
    return;
  }
  else
  {
    return WJniTraits<T>::SetStaticField(GetHandle(), field, arg);
  }
}

template <typename Ret, typename... Args>
Ret WJniObject::Call(const char* name, const Args&... args) const
{
  if (WJniAttachment::FailOnPendingErrorOrException())
  {
    return WJniTraits<Ret>::GetEmptyObject();
  }

  if (!m_object)
  {
    WLog::Error("Attempting to call method '{}' on null object.", name);
    WJniAttachment::SetLastError(WJniErrorState::CALL_ON_NULL_OBJECT);
    return WJniTraits<Ret>::GetEmptyObject();
  }

  // Fast path: Lookup method via signature built from parameters.
  // This only works for exact matches, but is roughly 50 times faster.
  WStringBuilder signature;
  if (WJniImpl::BuildMethodSignature<Ret>(signature, args...))
  {
    jmethodID method = WJniAttachment::GetEnv()->GetMethodID(reinterpret_cast<jclass>(GetClass().GetHandle()), name, signature.GetData());

    if (method)
    {
      return WJniTraits<Ret>::CallInstanceMethod(m_object, method, args...);
    }
    else
    {
      WJniAttachment::GetEnv()->ExceptionClear();
    }
  }

  // Fallback to slow path using reflection
  const size_t N = sizeof...(args);

  WJniClass returnType = WJniTraits<Ret>::GetStaticType();

  WJniClass inputTypes[N];
  WJniImpl::CollectArgumentTypes(inputTypes, args...);

  WJniObject foundMethod = FindMethod(false, name, GetClass(), returnType, inputTypes, N);

  if (foundMethod.IsNull())
  {
    return WJniTraits<Ret>::GetEmptyObject();
  }

  jmethodID method = WJniAttachment::GetEnv()->FromReflectedMethod(foundMethod.m_object);
  return WJniTraits<Ret>::CallInstanceMethod(m_object, method, args...);
}

template <typename Ret, typename... Args>
Ret WJniObject::UnsafeCall(const char* name, const char* signature, const Args&... args) const
{
  if (!m_object)
  {
    WLog::Error("Attempting to call method '{}' on null object.", name);
    WJniAttachment::SetLastError(WJniErrorState::CALL_ON_NULL_OBJECT);
    return WJniTraits<Ret>::GetEmptyObject();
  }

  jmethodID method = WJniAttachment::GetEnv()->GetMethodID(jclass(GetClass().m_object), name, signature);
  if (!method)
  {
    WLog::Error("No such method: '{}' with signature '{}' in class '{}'.", name, signature, GetClass().ToString().GetData());
    WJniAttachment::SetLastError(WJniErrorState::NO_MATCHING_METHOD);
    return WJniTraits<Ret>::GetEmptyObject();
  }
  else
  {
    return WJniTraits<Ret>::CallInstanceMethod(m_object, method, args...);
  }
}

template <typename T>
void WJniObject::SetField(const char* name, const T& arg) const
{
  if (WJniAttachment::FailOnPendingErrorOrException())
  {
    return;
  }

  if (!m_object)
  {
    WLog::Error("Attempting to set field '{}' on null object.", name);
    WJniAttachment::SetLastError(WJniErrorState::CALL_ON_NULL_OBJECT);
    return;
  }

  // No fast path here since we need to be able to report failures when attempting
  // to set final fields, which we can only do using reflection.

  WJniObject field = GetClass().UnsafeCall<WJniObject>("getField", "(Ljava/lang/String;)Ljava/lang/reflect/Field;", WJniString(name));

  if (WJniAttachment::GetEnv()->ExceptionOccurred())
  {
    WJniAttachment::GetEnv()->ExceptionClear();

    WLog::Error("No field named '{}' found.", name);
    WJniAttachment::SetLastError(WJniErrorState::NO_MATCHING_FIELD);

    return;
  }

  WJniClass modifierClass("java/lang/reflect/Modifier");
  jint modifiers = field.UnsafeCall<jint>("getModifiers", "()I");

  if ((modifiers & WJniModifiers::STATIC) != 0)
  {
    WLog::Error("Field named '{}' in class '{}' is static.", name, GetClass().ToString().GetData());
    WJniAttachment::SetLastError(WJniErrorState::NO_MATCHING_FIELD);
    return;
  }

  if ((modifiers & WJniModifiers::FINAL) != 0)
  {
    WLog::Error("Field named '{}' in class '{}' is final.", name, GetClass().ToString().GetData());
    WJniAttachment::SetLastError(WJniErrorState::NO_MATCHING_FIELD);
    return;
  }

  WJniClass fieldType = field.UnsafeCall<WJniClass>("getType", "()Ljava/lang/Class;");

  WJniClass argType = WJniTraits<T>::GetRuntimeType(arg);

  if (argType.IsNull())
  {
    if (fieldType.IsPrimitive())
    {
      WLog::Error("Field '{}' of type '{}'  in class '{}' can't be assigned null because it is a primitive type.", name, fieldType.ToString().GetData(), GetClass().ToString().GetData());
      WJniAttachment::SetLastError(WJniErrorState::NO_MATCHING_FIELD);
      return;
    }
  }
  else
  {
    if (!fieldType.IsAssignableFrom(argType))
    {
      WLog::Error("Field '{}' of type '{}' in class '{}' can't be assigned from type '{}'.", name, fieldType.ToString().GetData(), GetClass().ToString().GetData(), argType.ToString().GetData());
      WJniAttachment::SetLastError(WJniErrorState::NO_MATCHING_FIELD);
      return;
    }
  }

  return WJniTraits<T>::SetField(m_object, WJniAttachment::GetEnv()->FromReflectedField(field.GetHandle()), arg);
}

template <typename T>
void WJniObject::UnsafeSetField(const char* name, const char* signature, const T& arg) const
{
  if (!m_object)
  {
    WLog::Error("Attempting to set field '{}' on null class.", name);
    WJniAttachment::SetLastError(WJniErrorState::CALL_ON_NULL_OBJECT);
    return;
  }

  jfieldID field = WJniAttachment::GetEnv()->GetFieldID(jclass(GetClass().GetHandle()), name, signature);
  if (!field)
  {
    WLog::Error("No such field: '{}' with signature '{}'.", name, signature);
    WJniAttachment::SetLastError(WJniErrorState::NO_MATCHING_FIELD);
    return;
  }
  else
  {
    return WJniTraits<T>::SetField(m_object, field, arg);
  }
}

template <typename Ret>
Ret WJniObject::GetField(const char* name) const
{
  if (WJniAttachment::FailOnPendingErrorOrException())
  {
    return WJniTraits<Ret>::GetEmptyObject();
  }

  if (!m_object)
  {
    WLog::Error("Attempting to get field '{}' on null object.", name);
    WJniAttachment::SetLastError(WJniErrorState::CALL_ON_NULL_OBJECT);
    return WJniTraits<Ret>::GetEmptyObject();
  }

  jfieldID fieldID = WJniAttachment::GetEnv()->GetFieldID(GetClass().GetHandle(), name, WJniTraits<Ret>::GetSignatureStatic());
  if (fieldID)
  {
    return WJniTraits<Ret>::GetField(m_object, fieldID);
  }
  else
  {
    WJniAttachment::GetEnv()->ExceptionClear();
  }

  WJniObject field = GetClass().UnsafeCall<WJniObject>("getField", "(Ljava/lang/String;)Ljava/lang/reflect/Field;", WJniString(name));

  if (WJniAttachment::GetEnv()->ExceptionOccurred())
  {
    WJniAttachment::GetEnv()->ExceptionClear();

    WLog::Error("No field named '{}' found in class '{}'.", name, GetClass().ToString().GetData());
    WJniAttachment::SetLastError(WJniErrorState::NO_MATCHING_FIELD);

    return WJniTraits<Ret>::GetEmptyObject();
  }

  if ((field.UnsafeCall<jint>("getModifiers", "()I") & WJniModifiers::STATIC) != 0)
  {
    WLog::Error("Field named '{}' in class '{}' is static.", name, GetClass().ToString().GetData());
    WJniAttachment::SetLastError(WJniErrorState::NO_MATCHING_FIELD);
    return WJniTraits<Ret>::GetEmptyObject();
  }

  WJniClass fieldType = field.UnsafeCall<WJniClass>("getType", "()Ljava/lang/Class;");

  WJniClass returnType = WJniTraits<Ret>::GetStaticType();

  if (!returnType.IsAssignableFrom(fieldType))
  {
    WLog::Error("Field '{}' of type '{}' in class '{}' can't be assigned to return type '{}'.", name, fieldType.ToString().GetData(), GetClass().ToString().GetData(), returnType.ToString().GetData());
    WJniAttachment::SetLastError(WJniErrorState::NO_MATCHING_FIELD);
    return WJniTraits<Ret>::GetEmptyObject();
  }

  return WJniTraits<Ret>::GetField(m_object, WJniAttachment::GetEnv()->FromReflectedField(field.GetHandle()));
}

template <typename Ret>
Ret WJniObject::UnsafeGetField(const char* name, const char* signature) const
{
  if (!m_object)
  {
    WLog::Error("Attempting to get field '{}' on null class.", name);
    WJniAttachment::SetLastError(WJniErrorState::CALL_ON_NULL_OBJECT);
    return;
  }

  jfieldID field = WJniAttachment::GetEnv()->GetFieldID(GetClass().GetHandle(), name, signature);
  if (!field)
  {
    WLog::Error("No such field: '{}' with signature '{}'.", name, signature);
    WJniAttachment::SetLastError(WJniErrorState::NO_MATCHING_FIELD);
    return;
  }
  else
  {
    return WJniTraits<Ret>::GetField(m_object, field);
  }
}

template <>
struct WJniTraits<WJniNullPtr>
{

  static inline bool AppendSignature(const WJniNullPtr& object, WStringBuilder& str)
  {
    str.Append("L");
    str.Append(object.GetTypeSignature().GetData());
    str.Append(";");
    return true;
  }

  static inline jvalue ToValue(const WJniNullPtr& object)
  {
    jvalue j;
    j.l = nullptr;
    return j;
  }

  static inline WJniClass GetStaticType()
  {
    return WJniClass("java/lang/Object");
  }

  static inline WJniClass GetRuntimeType(const WJniNullPtr& arg)
  {
    return WJniClass(arg.GetTypeSignature().GetData());
  }
};
