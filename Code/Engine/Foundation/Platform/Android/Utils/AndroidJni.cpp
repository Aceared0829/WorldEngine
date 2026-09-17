#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_ANDROID)
#  include <Foundation/Platform/Android/Utils/AndroidJni.h>
#  include <Foundation/Platform/Android/Utils/AndroidUtils.h>
#  include <Foundation/Threading/Thread.h>
#  include <android_native_app_glue.h>

thread_local JNIEnv* WJniAttachment::s_env;
thread_local bool WJniAttachment::s_ownsEnv;
thread_local int WJniAttachment::s_attachCount;
thread_local WJniErrorState WJniAttachment::s_lastError;
thread_local WJniErrorHandler s_onError;

WJniAttachment::WJniAttachment()
{
  if (s_attachCount > 0)
  {
    s_env->PushLocalFrame(16);
  }
  else
  {
    JNIEnv* env = nullptr;
    jint envStatus = WAndroidUtils::GetAndroidJavaVM()->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    bool ownsEnv = (envStatus != JNI_OK);
    if (ownsEnv)
    {
      const char* szThreadName = "W JNI";
      if (const WThread* pThread = WThread::GetCurrentThread())
      {
        szThreadName = pThread->GetThreadName();
      }
      else if (WThreadUtils::IsMainThread())
      {
        szThreadName = "W Main Thread";
      }
      // Assign name to attachment since ART complains about it not being set.
      JavaVMAttachArgs args = {JNI_VERSION_1_6, szThreadName, nullptr};
      WAndroidUtils::GetAndroidJavaVM()->AttachCurrentThread(&env, &args);
    }
    else
    {
      // Assume already existing JNI environment will be alive as long as this object exists.
      W_ASSERT_DEV(env != nullptr, "");
      env->PushLocalFrame(16);
    }

    s_env = env;
    s_ownsEnv = ownsEnv;
  }

  s_attachCount++;
  W_ASSERT_ALWAYS(s_onError.IsValid() == false, "Can't install error handler for more than one instance.");
}

WJniAttachment::~WJniAttachment()
{
  s_onError = nullptr;
  s_attachCount--;

  if (s_attachCount == 0)
  {
    ClearLastError();

    if (s_ownsEnv)
    {
      WAndroidUtils::GetAndroidJavaVM()->DetachCurrentThread();
    }
    else
    {
      s_env->PopLocalFrame(nullptr);
    }

    s_env = nullptr;
    s_ownsEnv = false;
  }
  else
  {
    s_env->PopLocalFrame(nullptr);
  }
}

WJniObject WJniAttachment::GetActivity()
{
  return WJniObject(WAndroidUtils::GetAndroidNativeActivity(), WJniOwnerShip::BORROW);
}

JNIEnv* WJniAttachment::GetEnv()
{
  W_ASSERT_DEV(s_env != nullptr, "Thread not attached to the JVM - you forgot to create an instance of WJniAttachment in the current scope.");

#  if W_ENABLED(W_COMPILE_FOR_DEBUG)
  void* unused;
  W_ASSERT_DEBUG(WAndroidUtils::GetAndroidJavaVM()->GetEnv(&unused, JNI_VERSION_1_6) == JNI_OK,
    "Current thread has lost its attachment to the JVM - some OS calls can cause this to happen. Try to reduce the attachment to a smaller scope.");
#  endif

  return s_env;
}

WJniErrorState WJniAttachment::GetLastError()
{
  WJniErrorState state = s_lastError;
  return state;
}

void WJniAttachment::ClearLastError()
{
  s_lastError = WJniErrorState::SUCCESS;
}

void WJniAttachment::SetLastError(WJniErrorState state)
{
  s_lastError = state;
  if (s_onError.IsValid() && s_lastError != WJniErrorState::SUCCESS)
  {
    s_onError(s_lastError);
  }
}

bool WJniAttachment::HasPendingException()
{
  return GetEnv()->ExceptionCheck();
}

void WJniAttachment::ClearPendingException()
{
  return GetEnv()->ExceptionClear();
}

WJniObject WJniAttachment::GetPendingException()
{
  return WJniObject(GetEnv()->ExceptionOccurred(), WJniOwnerShip::OWN);
}

bool WJniAttachment::FailOnPendingErrorOrException()
{
  if (WJniAttachment::GetLastError() != WJniErrorState::SUCCESS)
  {
    WLog::Error("Aborting call because the previous error state was not cleared.");
    return true;
  }

  if (WJniAttachment::HasPendingException())
  {
    WLog::Error("Aborting call because a Java exception is still pending.");
    WJniAttachment::SetLastError(WJniErrorState::PENDING_EXCEPTION);
    return true;
  }

  return false;
}

void WJniAttachment::InstallErrorHandler(WJniErrorHandler onError)
{
  W_ASSERT_ALWAYS(s_attachCount == 1, "Can't install error handler for more than one instance.");
  s_onError = onError;
}

void WJniObject::DumpTypes(const WJniClass* inputTypes, int N, const WJniClass* returnType)
{
  if (returnType != nullptr)
  {
    WLog::Error("  With requested return type '{}'", returnType->ToString().GetData());
  }

  for (int paramIdx = 0; paramIdx < N; ++paramIdx)
  {
    WLog::Error("  With passed param type #{} '{}'", paramIdx, inputTypes[paramIdx].IsNull() ? "(null)" : inputTypes[paramIdx].ToString().GetData());
  }
}

int WJniObject::CompareMethodSpecificity(const WJniObject& method1, const WJniObject& method2)
{
  WJniClass returnType1 = method1.UnsafeCall<WJniClass>("getReturnType", "()Ljava/lang/Class;");
  WJniClass returnType2 = method2.UnsafeCall<WJniClass>("getReturnType", "()Ljava/lang/Class;");

  WJniObject paramTypes1 = method1.UnsafeCall<WJniObject>("getParameterTypes", "()[Ljava/lang/Class;");
  WJniObject paramTypes2 = method2.UnsafeCall<WJniObject>("getParameterTypes", "()[Ljava/lang/Class;");

  jsize N = WJniAttachment::GetEnv()->GetArrayLength(jarray(paramTypes1.m_object));

  int decision = returnType1.IsAssignableFrom(returnType2) - returnType2.IsAssignableFrom(returnType1);

  for (jsize paramIdx = 0; paramIdx < N; ++paramIdx)
  {
    WJniClass paramType1(
      jclass(WJniAttachment::GetEnv()->GetObjectArrayElement(jobjectArray(paramTypes1.m_object), paramIdx)), WJniOwnerShip::OWN);
    WJniClass paramType2(
      jclass(WJniAttachment::GetEnv()->GetObjectArrayElement(jobjectArray(paramTypes2.m_object), paramIdx)), WJniOwnerShip::OWN);

    int paramDecision = paramType1.IsAssignableFrom(paramType2) - paramType2.IsAssignableFrom(paramType1);

    if (decision == 0)
    {
      // No method is more specific yet
      decision = paramDecision;
    }
    else if (paramDecision != 0 && decision != paramDecision)
    {
      // There is no clear specificity ordering - one type is more specific, but the other less so
      return 0;
    }
  }

  return decision;
}

bool WJniObject::IsMethodViable(bool bStatic, const WJniObject& candidateMethod, const WJniClass& returnType, WJniClass* inputTypes, int N)
{
  // Check if staticness matches
  if (WJniClass("java/lang/reflect/Modifier").UnsafeCallStatic<bool>("isStatic", "(I)Z", candidateMethod.UnsafeCall<int>("getModifiers", "()I")) !=
      bStatic)
  {
    return false;
  }

  // Check if return type is assignable to the requested type
  WJniClass candidateReturnType = candidateMethod.UnsafeCall<WJniClass>("getReturnType", "()Ljava/lang/Class;");
  if (!returnType.IsAssignableFrom(candidateReturnType))
  {
    return false;
  }

  // Check number of parameters
  WJniObject parameterTypes = candidateMethod.UnsafeCall<WJniObject>("getParameterTypes", "()[Ljava/lang/Class;");
  jsize numCandidateParams = WJniAttachment::GetEnv()->GetArrayLength(jarray(parameterTypes.m_object));
  if (numCandidateParams != N)
  {
    return false;
  }

  // Check if input parameter types are assignable to the actual parameter types
  for (jsize paramIdx = 0; paramIdx < numCandidateParams; ++paramIdx)
  {
    WJniClass paramType(
      jclass(WJniAttachment::GetEnv()->GetObjectArrayElement(jobjectArray(parameterTypes.m_object), paramIdx)), WJniOwnerShip::OWN);

    if (inputTypes[paramIdx].IsNull())
    {
      if (paramType.IsPrimitive())
      {
        return false;
      }
    }
    else
    {
      if (!paramType.IsAssignableFrom(inputTypes[paramIdx]))
      {
        return false;
      }
    }
  }

  return true;
}

WJniObject WJniObject::FindMethod(
  bool bStatic, const char* name, const WJniClass& searchClass, const WJniClass& returnType, WJniClass* inputTypes, int N)
{
  if (searchClass.IsNull())
  {
    WLog::Error("Attempting to find constructor for null type.");
    WJniAttachment::SetLastError(WJniErrorState::CALL_ON_NULL_OBJECT);
    return WJniObject();
  }

  WTempHybridArray<WJniObject, 32> bestCandidates;

  // In case of no parameters, fetch the method directly.
  if (N == 0)
  {
    WJniObject candidateMethod = searchClass.UnsafeCall<WJniObject>(
      "getMethod", "(Ljava/lang/String;[Ljava/lang/Class;)Ljava/lang/reflect/Method;", WJniString(name), WJniObject());

    if (!WJniAttachment::GetEnv()->ExceptionCheck() && IsMethodViable(bStatic, candidateMethod, returnType, inputTypes, N))
    {
      bestCandidates.PushBack(candidateMethod);
    }
    else
    {
      WJniAttachment::GetEnv()->ExceptionClear();
    }
  }
  else
  {
    // For methods with parameters, loop over all methods to find one with the correct name and matching parameter types

    WJniObject methodArray = searchClass.UnsafeCall<WJniObject>("getMethods", "()[Ljava/lang/reflect/Method;");

    jsize numMethods = WJniAttachment::GetEnv()->GetArrayLength(jarray(methodArray.m_object));
    for (jsize methodIdx = 0; methodIdx < numMethods; ++methodIdx)
    {
      WJniObject candidateMethod(
        WJniAttachment::GetEnv()->GetObjectArrayElement(jobjectArray(methodArray.m_object), methodIdx), WJniOwnerShip::OWN);

      WJniString methodName = candidateMethod.UnsafeCall<WJniString>("getName", "()Ljava/lang/String;");

      if (strcmp(name, methodName.GetData()) != 0)
      {
        continue;
      }

      if (!IsMethodViable(bStatic, candidateMethod, returnType, inputTypes, N))
      {
        continue;
      }

      bool isMoreSpecific = true;
      for (int candidateIdx = 0; candidateIdx < bestCandidates.GetCount(); ++candidateIdx)
      {
        int comparison = CompareMethodSpecificity(bestCandidates[candidateIdx], candidateMethod);

        if (comparison == 1)
        {
          // Remove less specific candidate and continue looping
          bestCandidates.RemoveAtAndSwap(candidateIdx);
          candidateIdx--;
        }
        else if (comparison == -1)
        {
          // We're less specific, so by transitivity there are no other methods less specific than ours that we could throw out,
          // and we can abort the loop
          isMoreSpecific = false;
          break;
        }
        else
        {
          // No relation, so do nothing
        }
      }

      if (isMoreSpecific)
      {
        bestCandidates.PushBack(candidateMethod);
      }
    }
  }

  if (bestCandidates.GetCount() == 1)
  {
    return bestCandidates[0];
  }
  else if (bestCandidates.GetCount() == 0)
  {
    WLog::Error("Overload resolution failed: No method '{}' in class '{}' matches the requested return and parameter types.", name,
      searchClass.ToString().GetData());
    DumpTypes(inputTypes, N, &returnType);
    WJniAttachment::SetLastError(WJniErrorState::NO_MATCHING_METHOD);
    return WJniObject();
  }
  else
  {
    WLog::Error("Overload resolution failed: Call to '{}' in class '{}' is ambiguous. Cannot decide between the following candidates:", name,
      searchClass.ToString().GetData());
    for (int candidateIdx = 0; candidateIdx < bestCandidates.GetCount(); ++candidateIdx)
    {
      WLog::Error("  Candidate #{}: '{}'", candidateIdx, bestCandidates[candidateIdx].ToString().GetData());
    }
    DumpTypes(inputTypes, N, &returnType);
    WJniAttachment::SetLastError(WJniErrorState::AMBIGUOUS_CALL);
    return WJniObject();
  }
}

int WJniObject::CompareConstructorSpecificity(const WJniObject& method1, const WJniObject& method2)
{
  WJniObject paramTypes1 = method1.UnsafeCall<WJniObject>("getParameterTypes", "()[Ljava/lang/Class;");
  WJniObject paramTypes2 = method2.UnsafeCall<WJniObject>("getParameterTypes", "()[Ljava/lang/Class;");

  jsize N = WJniAttachment::GetEnv()->GetArrayLength(jarray(paramTypes1.m_object));

  int decision = 0;

  for (jsize paramIdx = 0; paramIdx < N; ++paramIdx)
  {
    WJniClass paramType1(
      jclass(WJniAttachment::GetEnv()->GetObjectArrayElement(jobjectArray(paramTypes1.m_object), paramIdx)), WJniOwnerShip::OWN);
    WJniClass paramType2(
      jclass(WJniAttachment::GetEnv()->GetObjectArrayElement(jobjectArray(paramTypes2.m_object), paramIdx)), WJniOwnerShip::OWN);

    int paramDecision = paramType1.IsAssignableFrom(paramType2) - paramType2.IsAssignableFrom(paramType1);

    if (decision == 0)
    {
      // No method is more specific yet
      decision = paramDecision;
    }
    else if (paramDecision != 0 && decision != paramDecision)
    {
      // There is no clear specificity ordering - one type is more specific, but the other less so
      return 0;
    }
  }

  return decision;
}

bool WJniObject::IsConstructorViable(const WJniObject& candidateMethod, WJniClass* inputTypes, int N)
{
  // Check number of parameters
  WJniObject parameterTypes = candidateMethod.UnsafeCall<WJniObject>("getParameterTypes", "()[Ljava/lang/Class;");
  jsize numCandidateParams = WJniAttachment::GetEnv()->GetArrayLength(jarray(parameterTypes.m_object));
  if (numCandidateParams != N)
  {
    return false;
  }

  // Check if input parameter types are assignable to the actual parameter types
  for (jsize paramIdx = 0; paramIdx < numCandidateParams; ++paramIdx)
  {
    WJniClass paramType(
      jclass(WJniAttachment::GetEnv()->GetObjectArrayElement(jobjectArray(parameterTypes.m_object), paramIdx)), WJniOwnerShip::OWN);

    if (inputTypes[paramIdx].IsNull())
    {
      if (paramType.IsPrimitive())
      {
        return false;
      }
    }
    else
    {
      if (!paramType.IsAssignableFrom(inputTypes[paramIdx]))
      {
        return false;
      }
    }
  }

  return true;
}

WJniObject WJniObject::FindConstructor(const WJniClass& type, WJniClass* inputTypes, int N)
{
  if (type.IsNull())
  {
    WLog::Error("Attempting to find constructor for null type.");
    WJniAttachment::SetLastError(WJniErrorState::CALL_ON_NULL_OBJECT);
    return WJniObject();
  }

  WTempHybridArray<WJniObject, 32> bestCandidates;

  // In case of no parameters, fetch the method directly.
  if (N == 0)
  {
    WJniObject candidateMethod =
      type.UnsafeCall<WJniObject>("getConstructor", "([Ljava/lang/Class;)Ljava/lang/reflect/Constructor;", WJniObject());

    if (!WJniAttachment::GetEnv()->ExceptionCheck() && IsConstructorViable(candidateMethod, inputTypes, N))
    {
      bestCandidates.PushBack(candidateMethod);
    }
    else
    {
      WJniAttachment::GetEnv()->ExceptionClear();
    }
  }
  else
  {
    // For methods with parameters, loop over all methods to find one with the correct name and matching parameter types

    WJniObject methodArray = type.UnsafeCall<WJniObject>("getConstructors", "()[Ljava/lang/reflect/Constructor;");

    jsize numMethods = WJniAttachment::GetEnv()->GetArrayLength(jarray(methodArray.m_object));
    for (jsize methodIdx = 0; methodIdx < numMethods; ++methodIdx)
    {
      WJniObject candidateMethod(
        WJniAttachment::GetEnv()->GetObjectArrayElement(jobjectArray(methodArray.m_object), methodIdx), WJniOwnerShip::OWN);

      if (!IsConstructorViable(candidateMethod, inputTypes, N))
      {
        continue;
      }

      bool isMoreSpecific = true;
      for (int candidateIdx = 0; candidateIdx < bestCandidates.GetCount(); ++candidateIdx)
      {
        int comparison = CompareConstructorSpecificity(bestCandidates[candidateIdx], candidateMethod);

        if (comparison == 1)
        {
          // Remove less specific candidate and continue looping
          bestCandidates.RemoveAtAndSwap(candidateIdx);
          candidateIdx--;
        }
        else if (comparison == -1)
        {
          // We're less specific, so by transitivity there are no other methods less specific than ours that we could throw out,
          // and we can abort the loop
          isMoreSpecific = false;
          break;
        }
        else
        {
          // No relation, so do nothing
        }
      }

      if (isMoreSpecific)
      {
        bestCandidates.PushBack(candidateMethod);
      }
    }
  }

  if (bestCandidates.GetCount() == 1)
  {
    return bestCandidates[0];
  }
  else if (bestCandidates.GetCount() == 0)
  {
    WLog::Error("Overload resolution failed: No constructor in class '{}' matches the requested parameter types.", type.ToString().GetData());
    DumpTypes(inputTypes, N, nullptr);
    WJniAttachment::SetLastError(WJniErrorState::NO_MATCHING_METHOD);
    return WJniObject();
  }
  else
  {
    WLog::Error("Overload resolution failed: Call to constructor in class '{}' is ambiguous. Cannot decide between the following candidates:",
      type.ToString().GetData());
    for (int candidateIdx = 0; candidateIdx < bestCandidates.GetCount(); ++candidateIdx)
    {
      WLog::Error("  Candidate #{}: '{}'", candidateIdx, bestCandidates[candidateIdx].ToString().GetData());
    }
    DumpTypes(inputTypes, N, nullptr);
    WJniAttachment::SetLastError(WJniErrorState::AMBIGUOUS_CALL);
    return WJniObject();
  }
}

WJniObject::WJniObject()
  : m_object(nullptr)
  , m_class(nullptr)
  , m_own(false)
{
}

jobject WJniObject::GetHandle() const
{
  return m_object;
}

WJniClass WJniObject::GetClass() const
{
  if (!m_object)
  {
    return WJniClass();
  }

  if (!m_class)
  {
    const_cast<WJniObject*>(this)->m_class = WJniAttachment::GetEnv()->GetObjectClass(m_object);
  }

  return WJniClass(m_class, WJniOwnerShip::BORROW);
}

WJniString WJniObject::ToString() const
{
  if (WJniAttachment::FailOnPendingErrorOrException())
  {
    return WJniString();
  }

  // Implement ToString without UnsafeCall, since UnsafeCall requires ToString for diagnostic output.
  if (IsNull())
  {
    WLog::Error("Attempting to call method 'toString' on null object.");
    WJniAttachment::SetLastError(WJniErrorState::CALL_ON_NULL_OBJECT);
    return WJniString();
  }

  jmethodID method = WJniAttachment::GetEnv()->GetMethodID(jclass(GetClass().m_object), "toString", "()Ljava/lang/String;");
  W_ASSERT_DEV(method, "Could not find JNI method toString()");

  return WJniTraits<WJniString>::CallInstanceMethod(m_object, method);
}

bool WJniObject::IsInstanceOf(const WJniClass& clazz) const
{
  if (IsNull())
  {
    return false;
  }

  return clazz.IsAssignableFrom(GetClass());
}

WJniString::WJniString()
  : WJniObject()
  , m_utf(nullptr)
{
}

WJniString::WJniString(const char* str)
  : WJniObject(WJniAttachment::GetEnv()->NewStringUTF(str), WJniOwnerShip::OWN)
  , m_utf(nullptr)
{
}

WJniString::WJniString(jstring string, WJniOwnerShip ownerShip)
  : WJniObject(string, ownerShip)
  , m_utf(nullptr)
{
}

WJniString::WJniString(const WJniString& other)
  : WJniObject(other)
  , m_utf(nullptr)
{
}

WJniString::WJniString(WJniString&& other)
  : WJniObject(other)
  , m_utf(nullptr)
{
  m_utf = other.m_utf;
  other.m_utf = nullptr;
}

WJniString& WJniString::operator=(const WJniString& other)
{
  if (m_utf)
  {
    WJniAttachment::GetEnv()->ReleaseStringUTFChars(jstring(GetJObject()), m_utf);
    m_utf = nullptr;
  }

  WJniObject::operator=(other);

  return *this;
}

WJniString& WJniString::operator=(WJniString&& other)
{
  if (m_utf)
  {
    WJniAttachment::GetEnv()->ReleaseStringUTFChars(jstring(GetJObject()), m_utf);
    m_utf = nullptr;
  }

  WJniObject::operator=(other);

  m_utf = other.m_utf;
  other.m_utf = nullptr;

  return *this;
}

WJniString::~WJniString()
{
  if (m_utf)
  {
    WJniAttachment::GetEnv()->ReleaseStringUTFChars(jstring(GetJObject()), m_utf);
    m_utf = nullptr;
  }
}

const char* WJniString::GetData() const
{
  if (IsNull())
  {
    WLog::Error("Calling AsChar() on null Java String");
    return "<null>";
  }

  if (!m_utf)
  {
    const_cast<WJniString*>(this)->m_utf = WJniAttachment::GetEnv()->GetStringUTFChars(jstring(GetJObject()), nullptr);
  }

  return m_utf;
}


WJniClass::WJniClass()
  : WJniObject()
{
}

WJniClass::WJniClass(const char* className)
  : WJniObject(WJniAttachment::GetEnv()->FindClass(className), WJniOwnerShip::OWN)
{
  if (IsNull())
  {
    WLog::Error("Class '{}' not found.", className);
    WJniAttachment::SetLastError(WJniErrorState::CLASS_NOT_FOUND);
  }
}

WJniClass::WJniClass(jclass clazz, WJniOwnerShip ownerShip)
  : WJniObject(clazz, ownerShip)
{
}

WJniClass::WJniClass(const WJniClass& other)
  : WJniObject(static_cast<const WJniObject&>(other))
{
}

WJniClass::WJniClass(WJniClass&& other)
  : WJniObject(other)
{
}

WJniClass& WJniClass::operator=(const WJniClass& other)
{
  WJniObject::operator=(other);
  return *this;
}

WJniClass& WJniClass::operator=(WJniClass&& other)
{
  WJniObject::operator=(other);
  return *this;
}

jclass WJniClass::GetHandle() const
{
  return static_cast<jclass>(GetJObject());
}

bool WJniClass::IsAssignableFrom(const WJniClass& other) const
{
  static bool checkedApiOrder = false;
  static bool reverseArgs = false;

  JNIEnv* env = WJniAttachment::GetEnv();

  // Guard against JNI bug reversing order of arguments - fixed in
  // https://android.googlesource.com/platform/art/+/1268b742c8cff7318dc0b5b283cbaeabfe0725ba
  if (!checkedApiOrder)
  {
    WJniClass objectClass("java/lang/Object");
    WJniClass stringClass("java/lang/String");

    if (env->IsAssignableFrom(jclass(objectClass.GetJObject()), jclass(stringClass.GetJObject())))
    {
      reverseArgs = true;
    }
    checkedApiOrder = true;
  }

  if (!reverseArgs)
  {
    return env->IsAssignableFrom(jclass(other.GetJObject()), jclass(GetJObject()));
  }
  else
  {
    return env->IsAssignableFrom(jclass(GetJObject()), jclass(other.GetJObject()));
  }
}

bool WJniClass::IsPrimitive()
{
  return UnsafeCall<bool>("isPrimitive", "()Z");
}

WJniNullPtr::WJniNullPtr(WJniClass& clazz)
{
  m_class = clazz;
}

const WJniString WJniNullPtr::GetTypeSignature() const
{
  WJniString jSignature = m_class.UnsafeCall<WJniString>("getName", "()Ljava/lang/String;");
  WStringBuilder signature{jSignature.GetData()};
  signature.ReplaceAll(".", "/");
  return WJniString{signature.GetData()};
}

#endif
