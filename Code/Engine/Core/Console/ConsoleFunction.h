#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/Variant.h>
#include <Foundation/Utilities/EnumerableClass.h>

/// Base class for all types of WConsoleFunction, represents functions to be exposed to WConsole.
///
/// Console functions are similar to WCVar's in that they can be executed from the WConsole.
/// A console function can wrap many different types of functions with differing number and types of parameters.
/// WConsoleFunction uses an WDelegate internally to store the function reference, so even member functions would be possible.
///
/// All console functions are enumerable, as their base class WConsoleFunctionBase is an WEnumerable class.
///
/// Console functions can have between zero and six parameters. The LuaInterpreter for WConsole only supports parameter types
/// (unsigned) int, float/double, bool and string and uses the conversion feature of WVariant to map the lua input to the final function.
///
/// To make a function available as a console function, create a global variable of type WConsoleFunction with the proper template
/// arguments to mirror its parameters and return type.
/// Note that although functions with return types are accepted, the return value is currently always ignored.
///
/// \code{.cpp}
///   void MyConsoleFunc1(int a, float b, WStringView sz) { ... }
///   WConsoleFunction<void ()> ConFunc_MyConsoleFunc1("MyConsoleFunc1", "()", MyConsoleFunc1);
///
///   int MyConsoleFunc2(int a, float b, WStringView sz) { ... }
///   WConsoleFunction<int (int, float, WString)> ConFunc_MyConsoleFunc2("MyConsoleFunc2", "(int a, float b, string c)", MyConsoleFunc2);
/// \endcode
///
/// Here the global function MyConsoleFunc2 is exposed to the console. The return value type and parameter types are passed as template
/// arguments. ConFunc_MyConsoleFunc2 is now the global variable that represents the function for the console.
/// The first string is the name with which the function is exposed, which is also used for auto-completion.
/// The second string is the description of the function. Here we inserted the parameter list with types, so that the user knows how to
/// use it. Finally the last parameter is the actual function to expose.
class W_CORE_DLL WConsoleFunctionBase : public WEnumerable<WConsoleFunctionBase>
{
  W_DECLARE_ENUMERABLE_CLASS(WConsoleFunctionBase);

public:
  /// The constructor takes the function name and description as it should appear in the console.
  WConsoleFunctionBase(WStringView sFunctionName, WStringView sDescription)
    : m_sFunctionName(sFunctionName)
    , m_sDescription(sDescription)
  {
  }

  /// Returns the name of the function as it should be exposed in the console.
  WStringView GetName() const { return m_sFunctionName; }

  /// Returns the description of the function as it should appear in the console.
  WStringView GetDescription() const { return m_sDescription; }

  /// Returns the number of parameters that this function takes.
  virtual WUInt32 GetNumParameters() const = 0;

  /// Returns the type of the n-th parameter.
  virtual WVariant::Type::Enum GetParameterType(WUInt32 uiParam) const = 0;

  /// Calls the function. Each parameter must be put into an WVariant and all of them are passed along as an array.
  ///
  /// Returns W_FAILURE, if the number of parameters did not match, or any parameter was not convertible to the actual type that
  /// the function expects.
  virtual WResult Call(WArrayPtr<WVariant> params) = 0;

private:
  WStringView m_sFunctionName;
  WStringView m_sDescription;
};


/// Implements the functionality of WConsoleFunctionBase for functions with different parameter types. See WConsoleFunctionBase for more
/// details.
template <typename R>
class WConsoleFunction : public WConsoleFunctionBase
{
};


#define ARG_COUNT 0
#include <Core/Console/Implementation/ConsoleFunctionHelper_inl.h>
#undef ARG_COUNT

#define ARG_COUNT 1
#include <Core/Console/Implementation/ConsoleFunctionHelper_inl.h>
#undef ARG_COUNT

#define ARG_COUNT 2
#include <Core/Console/Implementation/ConsoleFunctionHelper_inl.h>
#undef ARG_COUNT

#define ARG_COUNT 3
#include <Core/Console/Implementation/ConsoleFunctionHelper_inl.h>
#undef ARG_COUNT

#define ARG_COUNT 4
#include <Core/Console/Implementation/ConsoleFunctionHelper_inl.h>
#undef ARG_COUNT

#define ARG_COUNT 5
#include <Core/Console/Implementation/ConsoleFunctionHelper_inl.h>
#undef ARG_COUNT

#define ARG_COUNT 6
#include <Core/Console/Implementation/ConsoleFunctionHelper_inl.h>
#undef ARG_COUNT
