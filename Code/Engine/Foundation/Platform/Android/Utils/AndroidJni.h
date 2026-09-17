#pragma once

#if W_ENABLED(W_PLATFORM_ANDROID)

#  include <Foundation/Logging/Log.h>
#  include <Foundation/Strings/StringBuilder.h>
#  include <Foundation/Types/Delegate.h>
#  include <Foundation/Types/Enum.h>
#  include <jni.h>

class WJniObject;
class WJniClass;
class WJniString;

struct WJniError
{
  using StorageType = WUInt8;
  enum Enum
  {
    /// No JNI error occurred.
    SUCCESS = 0,

    /// The method could not be executed because the JVM is still holding a pending exception.
    PENDING_EXCEPTION,

    /// No method matching the passed parameters or return type was found.
    NO_MATCHING_METHOD,

    /// A call could not be resolved due to multiple matching overloads.
    AMBIGUOUS_CALL,

    /// No field matching the return type, static-ness or writability was found.
    NO_MATCHING_FIELD,

    /// A field or method was requested on a null object.
    CALL_ON_NULL_OBJECT,

    /// A class was not found.
    CLASS_NOT_FOUND,

    Default = SUCCESS
  };
  W_ENUM_TO_STRING(SUCCESS, PENDING_EXCEPTION, NO_MATCHING_METHOD, AMBIGUOUS_CALL, NO_MATCHING_FIELD, CALL_ON_NULL_OBJECT, CLASS_NOT_FOUND);
};

using WJniErrorState = WEnum<WJniError>;
using WJniErrorHandler = WDelegate<void(WJniErrorState)>;

/// Attaches the current thread to the Java virtual machine.
///
/// Instantiating this class attaches the current thread to the JVM. You can nest attachments,
/// and each attachment will push a new local reference frame on the JVM.
///
/// \example
///   \code
///     void foo()
///     {
///       WJniAttachment attachment;
///       WJniObject activity = attachment.GetActivity();
///
///       // Perform Java calls
///       WJniClass myClassType = activity.Call<WJniObject>("getClassLoader").Call<WJniClass>("loadClass", WJniString("com.myproject.MyClass"));
///     }
///   \endcode
///
/// Note that all JNICalls may fail, including the construction of WJniClass and WJniObject instances.
/// You may use GetLastError() and ClearLastError() to check for, and do your own error-specific handling.
/// \example
///   \code
///     void foo()
///     {
///       WJniAttachment attachment;
///       WJniObject activity = attachment.GetActivity();
///       WJniClass myClassType = activity.Call<WJniObject>("getClassLoader").Call<WJniClass>("loadClass", WJniString("com.myproject.MyClass"));
///       if(attachment.GetLastError() == WJniErrorState::NO_MATCHING_METHOD){SpecificErrorHandling();}
///     }
///   \endcode
///
/// You may also call InstallErrorHandler() after creation to install your own error handler.
/// Check out its documentation for details. This reduces boilerplate and enables method chaining:
/// \example
///   \code
///     void foo()
///     {
///       WJniAttachment attachment;
///       attachment.InstallErrorHandler(&ThrowRuntimeError);
///
///       try{
///         attachment
///           .GetActivity()
///           .Call<WJniObject>("getClassLoader")
///           .Call<WJniClass>("loadClass", WJniString("com.myproject.MyClass")
///           .CreateInstance()
///           .Call<WJniObject>("MyMethod");
///       }
///       catch(const std::runtime_error& error)}
///       }
///     }
///   \endcode
///
/// Either way, the error is always internally logged with WLog::Error.
/// ClearLastError() is called whenever a WJniAttachment is destroyed, so you needn't worry about cleanup in that case.
class W_FOUNDATION_DLL WJniAttachment
{
public:
  /// Constructor.
  WJniAttachment();

  /// Destructor.
  ~WJniAttachment();

  /// Returns the Activity of the native application.
  ///
  /// The returned object wraps around activity->clazz of the native application.
  static WJniObject GetActivity();

  /// Returns the environment the current thread is attached to.
  static JNIEnv* GetEnv();

  /// Returns the last error that occurred while trying to perform a checked JNI call.
  ///
  /// This error state covers general failures during JNI interop, but not exceptions that occurred during Java method execution.
  /// However, attempting to perform a Java call while an exception is pending will result in an error.
  static WJniErrorState GetLastError();

  /// Clears the last error that occurred while trying to perform a checked JNI call.
  ///
  /// This error state covers general failures during JNI interop, but not exceptions that occurred during Java method execution.
  /// However, attempting to perform a Java call while an exception is pending will result in an error.
  static void ClearLastError();

  /// Returns true if an exception has been thrown in the last called Java method.
  ///
  /// If an exception occurred, no other Java method may be called until ClearPendingException has been called.
  static bool HasPendingException();

  /// Returns the exception that has been thrown in the last called Java method, or a null object.
  ///
  /// If an exception occurred, no other Java method may be called until ClearPendingException has been called.
  static WJniObject GetPendingException();

  /// Clears the exception that has been thrown in the last called Java method, if any.
  static void ClearPendingException();

  /// Used internally. Sets the last error state.
  static void SetLastError(WJniErrorState state);

  /// Used internally. Returns true and logs a message is an error or exception is pending.
  static bool FailOnPendingErrorOrException();

  /// Installs an error handler valid for the current WJniAttachment's lifetime. Called for all WJniErrorStates but SUCCESS.
  /// There must not be any other WJniAttachments existing on the same thread for the call to succeed.
  /// There must not be any new WJniAttachments instances created before the current WJniAttachment is destroyed.
  /// These invariants should hold true with correct usage and are guarded by asserts.
  /// May be called more than once to replace an existing error handler on the same instance.
  void InstallErrorHandler(WJniErrorHandler onError);

private:
  static thread_local JNIEnv* s_env;
  static thread_local bool s_ownsEnv;
  static thread_local int s_attachCount;
  static thread_local WJniErrorState s_lastError;

  WJniAttachment(const WJniAttachment&);
  WJniAttachment& operator=(const WJniAttachment&);
};

/// Describes the ownership handling of the local JNI reference.
enum class WJniOwnerShip
{
  /// The local reference belongs to the class, and will be deleted when it goes out of scope.
  OWN,

  /// The class will create its own copy of the reference.
  COPY,

  /// The class will not delete the reference. The caller must ensure that the reference remains valid while the class instance is alive.
  BORROW
};

/// Class that manages a local reference to a Java object.
class W_FOUNDATION_DLL WJniObject
{
public:
  /// Creates a null object.
  WJniObject();

  /// Constructs an object from a JNI object handle.
  ///
  /// \param object The JNI object handle.
  /// \param ownerShip How the object handle should be managed. See WJniOwnerShip.
  inline WJniObject(jobject object, WJniOwnerShip ownerShip);

  /// Copy constructor. Both instances will reference the same Java object.
  inline WJniObject(const WJniObject& other);

  /// Move constructor.
  inline WJniObject(WJniObject&& other);

  /// Assignment operator.
  inline WJniObject& operator=(const WJniObject& other);

  /// Move assignment operator.
  inline WJniObject& operator=(WJniObject&& other);

  /// Destructor.
  inline virtual ~WJniObject();

  /// Compares if the two objects reference the same Java object.
  /// \param other The object to compare to.
  ///
  /// This method returns true if two WJniObjects reference the same Java object.
  ///
  /// In order to compare the objects using \c Object.equals, use the following code instead:
  ///
  /// \code
  ///   WJniObject o1, o2;
  ///   if(o1.Call<bool>("equals", o2))
  ///   {
  ///      // ...
  ///   }
  /// \endcode
  inline bool operator==(const WJniObject& other) const;

  /// Compares if the two objects reference different Java objects.
  /// \param other The object to compare to.
  ///
  /// This method returns true if two WJniObjects reference different Java objects.
  ///
  /// In order to compare the objects using \c Object.equals, use the following code instead:
  ///
  /// \code
  ///   WJniObject o1, o2;
  ///   if(!o1.Call<bool>("equals", o2))
  ///   {
  ///      // ...
  ///   }
  /// \endcode
  inline bool operator!=(const WJniObject& other) const;

  /// Returns true if the object is null.
  bool IsNull() const { return m_object == nullptr; }

  /// Returns the JNI handle of the object.
  jobject GetHandle() const;

  /// Returns the class type of the object.
  ///
  /// This Call is equivalent to \c o.getClass() in Java.
  WJniClass GetClass() const;

  /// Returns a string representation of the object.
  ///
  /// This call is equivalent to \c o.ToString() in Java.
  WJniString ToString() const;

  /// Returns true if the object is an instance of the given type.
  bool IsInstanceOf(const WJniClass& clazz) const;

  /// Calls an instance method on the object.
  /// \param name The name of the method to call.
  /// \param args The function arguments to pass.
  ///
  /// This function searches for a public method of the given name that is compatible with the
  /// passed arguments and the return type that is given by the template argument.
  /// If there are multiple suitable methods, dynamic overload resolution is performed to select
  /// the overload that is the most specific. Parameters that null are always assumed to be of type Object.
  ///
  /// In case the method of the given name isn't found, or there exists a method of the given name that doesn't
  /// match the requested return and argument types, or overload resolution can't find a single best method to call,
  /// this function logs a detailed error message and returns a dummy object instead.
  ///
  /// Note that no conversions between primitive types are performed, nor any boxing/unboxing conversions that are usually implicit
  /// in normal Java code. See the examples below on how to handle these cases.
  ///
  /// Varargs methods are currently not supported.
  ///
  /// \example
  ///   \code
  ///     WJniObject myClassInstance;
  ///
  ///     // --- Overload resolution
  ///
  ///     // Java declaration: two overloads
  ///     //   public Player getPlayer(int player)
  ///     //   public Player getPlayer(String playerName)
  ///
  ///     // Call the first overload
  ///     WJniObject player = myClassInstance.Call<WJniObject>("getPlayer", 0);
  ///
  ///     // Call the second overload
  ///     WJniObject player = myClassInstance.Call<WJniObject>("getPlayer", WJniString("player1"));
  ///
  ///     // This call will fail at runtime since there is no method getPlayer that returns int.
  ///     int player = myClassInstance.Call<int>("getPlayer", 0);
  ///
  ///     // --- Manual argument conversion
  ///
  ///     // Java declaration: public Player getPlayerById(long playerId)
  ///
  ///     // This call will fail at runtime: There is no method named getPlayerById that takes a parameter of type int.
  ///     WJniObject player = myClassInstance.Call<WJniObject>("getPlayerById", 0);
  ///
  ///     // Instead, explicitly cast the function parameter to the expected type according to the following table:
  ///     //
  ///     //   Java type        C++ type
  ///     //    boolean     <=>  bool (do not use jboolean!)
  ///     //    byte        <=>  jbyte or signed char
  ///     //    char        <=>  jchar or unsigned short
  ///     //    short       <=>  jshort or signed short
  ///     //    int         <=>  jint or signed int
  ///     //    long        <=>  jlong or signed long long
  ///     //    float       <=>  jfloat or float
  ///     //    double      <=>  jdouble or double
  ///     //
  ///     WJniObject player = myClassInstance.Call<WJniObject>("getPlayerById", jlong(0));
  ///
  ///    // --- Manual boxing/unboxing
  ///
  ///     // Java declaration: public Long SomeMethodWithBoxedTypes(Integer param)
  ///
  ///     // This call will fail at runtime: SomeMethodWithBoxedTypes takes Integer, not int, and returns Long, not long
  ///     jint param = 1234;
  ///     jlong result = myClassInstance.Call<jlong>("SomeMethodWithBoxedTypes", param);
  ///
  ///     // Instead, convert parameter into boxed type...
  ///     WJniObject boxedParam = WJniClass("java/lang/Integer").CreateInstance(param);
  ///
  ///     // .. Call the method...
  ///     WJniObject boxedResult = myClassInstance.Call<WJniObject>("SomeMethodWithBoxedTypes", boxedParam);
  ///
  ///     // ...and unbox the result
  ///     jlong result = boxedResult.Call<jlong>("longValue");
  ///   \endcode
  template <typename Ret = void, typename... Args>
  Ret Call(const char* name, const Args&... args) const;

  /// Returns the value of the field with the given name.
  /// \param name The name of the field.
  ///
  /// The field type must be assignable to the type specified by the template argument.
  /// \example
  ///   \code
  ///     jint intField = myClassInstance.GetField<jint>("IntField");
  ///     WJniObject objectField = myClassInstance.GetField<WJniObject>("ObjectField");
  ///   \endcode
  template <typename Ret>
  Ret GetField(const char* name) const;

  /// Sets the value of the field with the given name.
  /// \param name The name of the field.
  /// \param arg The new value of the field.
  ///
  /// The field type must be assignable from the given argument type.
  /// \example
  ///   \code
  ///     myClassInstance.SetField("IntField", jint(1234));
  ///     myClassInstance.SetField("ObjectField", WJniString("SomeString");
  ///   \endcode
  template <typename T>
  void SetField(const char* name, const T& arg) const;

  /// Calls an instance method by supplying the JNI function signature without performing any type checks.
  template <typename Ret, typename... Args>
  Ret UnsafeCall(const char* name, const char* signature, const Args&... args) const;

  /// Returns the value of the field with the given name and signature without performing any type checks.
  template <typename Ret>
  Ret UnsafeGetField(const char* name, const char* signature) const;

  /// Sets the value of the field with the given name and signature without performing any type checks.
  template <typename T>
  void UnsafeSetField(const char* name, const char* signature, const T& arg) const;

protected:
  inline void Reset();
  inline jobject GetJObject() const;

  static void DumpTypes(const WJniClass* inputTypes, int N, const WJniClass* returnType);

  static int CompareMethodSpecificity(const WJniObject& method1, const WJniObject& method2);
  static bool IsMethodViable(bool bStatic, const WJniObject& candidateMethod, const WJniClass& returnType, WJniClass* inputTypes, int N);
  static WJniObject FindMethod(bool bStatic, const char* name, const WJniClass& type, const WJniClass& returnType, WJniClass* inputTypes, int N);

  static int CompareConstructorSpecificity(const WJniObject& method1, const WJniObject& method2);
  static bool IsConstructorViable(const WJniObject& candidateMethod, WJniClass* inputTypes, int N);
  static WJniObject FindConstructor(const WJniClass& type, WJniClass* inputTypes, int N);

private:
  jobject m_object;
  jclass m_class;
  bool m_own;
};

/// Class holding a local reference to a Java object of type String.
///
/// Conversion to/from const char* uses the modified UTF-8 encoding as described by the JNI specification.
/// This encoding is identical to UTF-8, except that null characters inside the string are encoded as 0xC0, 0x80,
/// and that code points above 0xFFFF are represented by separately encoding each of the two UTF-16 surrogate characters
/// as 3 bytes each.
class W_FOUNDATION_DLL WJniString : public WJniObject
{
public:
  /// Constructs a null String.
  WJniString();

  /// Constructs a String from a modified UTF-8 string.
  WJniString(const char* str);

  /// Constructs a String from a JNI string  handle.
  ///
  /// \param string The JNI string handle.
  /// \param ownerShip How the object handle should be managed. See WJniObject::OwnerShip.
  WJniString(jstring string, WJniOwnerShip ownerShip);

  /// Copy constructor. Both instances will reference the same Java String.
  WJniString(const WJniString& other);

  /// Move constructor.
  WJniString(WJniString&& other);

  /// Assignment operator. Both instances will reference the same Java String.
  WJniString& operator=(const WJniString& other);

  /// Move assignment operator.
  WJniString& operator=(WJniString&& other);

  /// Destructor.
  virtual ~WJniString();

  /// Returns the string as a modified UTF-8 string. The pointer is only valid over the lifetime of this object.
  const char* GetData() const;

private:
  const char* m_utf;
};

/// Class holding a local reference to a Java object of type Class.
class W_FOUNDATION_DLL WJniClass : public WJniObject
{
public:
  /// Constructs a null Class.
  WJniClass();

  /// Constructs a class by searching for the class of the given name.
  ///
  /// \param className
  ///   The class name encoded in the JNI class name format, e.g. "java/lang/Object". Note that this
  ///   is different from the format used by ClassLoader.loadClass, which uses "java.lang.Object".
  ///
  /// If the class could not be found, isNull() will return true and WJniAttachment::getLastError will return WJniAttachment::CLASS_NOT_FOUND.
  ///
  /// In order to load classes from the application package, you will have to use the activity's class loader instead.
  /// For example:
  /// \code
  ///   WJniObject classLoader = attachment.GetActivity().Call<WJniObject>("getClassLoader");
  ///   WJniClass myClass = classLoader.Call<WJniClass>("loadClass", WJniString("com.myproject.MyClass"));
  /// \endcode
  WJniClass(const char* className);

  /// Constructs a Class from a JNI class handle.
  ///
  /// \param clazz The JNI class handle.
  /// \param ownerShip How the object handle should be managed. See WJniObject::OwnerShip.
  WJniClass(jclass clazz, WJniOwnerShip ownerShip);

  /// Copy constructor. Both instances will reference the same Java class.
  WJniClass(const WJniClass& other);

  /// Move constructor.
  WJniClass(WJniClass&& other);

  /// Assignment operator. Both instances will reference the same Java class.
  WJniClass& operator=(const WJniClass& other);

  /// Move assignment operator.
  WJniClass& operator=(WJniClass&& other);

  /// Returns the JNI handle of the object.
  jclass GetHandle() const;

  /// Constructs an instance of the class type with the given parameters.
  ///
  /// \param args The constructor arguments to pass.
  ///
  ///
  /// \example
  ///   \code
  ///     // Same as Class myClassType = activity.GetClassLoader().LoadClass("com.myproject.MyClass") in Java
  ///     WJniClass myClassType = activity.Call<WJniObject>("getClassLoader").Call<WJniClass>("loadClass", WJniString("com.myproject.MyClass"));
  ///
  ///     // Same as MyClass myClassInstance = new MyClass(true, "some string", 12345) in Java
  ///     WJniObject myClassInstance = myClassType.CreateInstance(true, WJniString("some string"), 12345);
  ///   \endcode
  ///
  /// See WJniObject::Call for more information on overload resolution and argument conversion.
  template <typename... Args>
  WJniObject CreateInstance(const Args&... args) const;

  /// Returns true if an instance of this class can be assigned from \c other.
  /// \param other A Java class.
  ///
  /// This call is equivalent to Class.IsAssignableFrom() in Java.
  bool IsAssignableFrom(const WJniClass& other) const;

  /// Returns true if this class is primitive, i.e., one of boolean, byte, char, short, int, long, float, or double.
  bool IsPrimitive();

  /// Calls a static method of the class type.
  ///
  /// \param name The name of the method to call.
  /// \param args The function arguments to pass.
  ///
  /// See WJniObject::Call() for details on argument handling.
  ///
  /// \example
  ///   \code
  ///     // To call a static method of a type directly:
  ///     WJniClass myClassType;
  ///     myClassType.CallStatic("SomeStaticMethod");
  ///     WJniObject result = myClassType.CallStatic<WJniObject>("SomeStaticMethodReturningObject");
  ///
  ///     // To call a static method of the type of a class instance:
  ///     WJniObject myClassInstance;
  ///     myClassInstance.getClass().CallStatic("SomeStaticMethod");
  ///     WJniObject result = myClassInstance.GetClass().CallStatic<WJniObject>("SomeStaticMethodReturningObject");
  ///   \endcode
  template <typename Ret = void, typename... Args>
  Ret CallStatic(const char* name, const Args&... args) const;

  /// Returns the value of the static field with the given name.
  /// \param name The name of the static field.
  /// \sa WJniObject::GetField()
  template <typename Ret>
  Ret GetStaticField(const char* name) const;

  /// Sets the value of the static field with the given name.
  /// \param name The name of the static field.
  /// \param arg The new value of the static field.
  /// \sa WJniObject::SetField()
  template <typename T>
  void SetStaticField(const char* name, const T& arg) const;

  /// Calls a static method of the class type without performing any type checks.
  template <typename Ret, typename... Args>
  Ret UnsafeCallStatic(const char* name, const char* signature, const Args&... args) const;

  /// Returns the value of the static field with the given name without performing any type checks.
  template <typename Ret>
  Ret UnsafeGetStaticField(const char* name, const char* signature) const;

  /// Sets the value of the static field with the given name without performing any type checks.
  template <typename T>
  void UnsafeSetStaticField(const char* name, const char* signature, const T& arg) const;
};

/// Represents the null value of an WJniClass.
/// Passing null / nullptr directly to WJni results in type information being lost,
/// without which WJni can't make the appropriate JNI calls.
/// create an WJniClass instance instead, and wrap it in an WJniNullPtr instance.
/// You may then pass that WJniNullPtr instance to any WJni calls you make.
class W_FOUNDATION_DLL WJniNullPtr
{
  WJniClass m_class;

public:
  /// Constructs a Class from a WJniClass.
  ///
  /// \param clazz The WJniClass.
  explicit WJniNullPtr(WJniClass& clazz);

  /// Returns the fully qualified name of the WJniClass that was passed into the constructor, e.g. "java/lang/String"
  const WJniString GetTypeSignature() const;
};

#  include <Foundation/Platform/Android/Utils/AndroidJni.inl>

#endif
