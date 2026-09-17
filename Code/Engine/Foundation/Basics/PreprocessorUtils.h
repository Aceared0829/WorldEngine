
#pragma once

/// \file

/// Used to pass a token through without modification
/// Useful to separate tokens that have no whitespace in between and thus would otherwise form one string.
#define W_PP_IDENTITY(x) x

/// Concatenates two strings, even when the strings are macros themselves
#define W_PP_CONCAT(x, y) W_PP_CONCAT_HELPER(x, y)
#define W_PP_CONCAT_HELPER(x, y) W_PP_CONCAT_HELPER2(x, y)
#define W_PP_CONCAT_HELPER2(x, y) x##y

/// Concatenates two strings, even when the strings are macros themselves
#define W_PP_CONCAT(x, y) W_PP_CONCAT_HELPER(x, y)

/// Turns some piece of code (usually some identifier name) into a string. Even works on macros.
#define W_PP_STRINGIFY(str) W_PP_STRINGIFY_HELPER(str)
#define W_PP_STRINGIFY_HELPER(x) #x

/// Max value of two compile-time constant expression.
#define W_COMPILE_TIME_MAX(a, b) ((a) > (b) ? (a) : (b))

/// Min value of two compile-time constant expression.
#define W_COMPILE_TIME_MIN(a, b) ((a) < (b) ? (a) : (b))


/// Creates a bit mask with only the n-th Bit set. Useful when creating enum values for flags.
#define W_BIT(n) (1ull << (n))
