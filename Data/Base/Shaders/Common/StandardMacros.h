#pragma once

#ifndef W_PP_CONCAT

/// Concatenates two strings, even when the strings are macros themselves
#  define W_PP_CONCAT(x, y) W_PP_CONCAT_HELPER(x, y)
#  define W_PP_CONCAT_HELPER(x, y) W_PP_CONCAT_HELPER2(x, y)
#  define W_PP_CONCAT_HELPER2(x, y) x##y

#endif

#ifndef W_PP_STRINGIFY

/// Turns some piece of code (usually some identifier name) into a string. Even works on macros.
#  define W_PP_STRINGIFY(str) W_PP_STRINGIFY_HELPER(str)
#  define W_PP_STRINGIFY_HELPER(x) #x

#endif

#ifndef W_ON

/// Used in conjunction with W_ENABLED and W_DISABLED for safe checks. Define something to W_ON or W_OFF to work with those macros.
#  define W_ON =

/// Used in conjunction with W_ENABLED and W_DISABLED for safe checks. Define something to W_ON or W_OFF to work with those macros.
#  define W_OFF !

/// Used in conjunction with W_ON and W_OFF for safe checks. Use #if W_ENABLED(x) or #if W_DISABLED(x) in conditional compilation.
#  define W_ENABLED(x) (1 W_PP_CONCAT(x, =) 1)

/// Used in conjunction with W_ON and W_OFF for safe checks. Use #if W_ENABLED(x) or #if W_DISABLED(x) in conditional compilation.
#  define W_DISABLED(x) (1 W_PP_CONCAT(x, =) 2)

/// Checks whether x AND y are both defined as W_ON or W_OFF. Usually used to check whether configurations overlap, to issue an error.
#  define W_IS_NOT_EXCLUSIVE(x, y) ((1 W_PP_CONCAT(x, =) 1) == (1 W_PP_CONCAT(y, =) 1))

#endif

#define BG_FRAME 0
#define BG_RENDER_PASS 1
#define BG_MATERIAL 2
#define BG_DRAW_CALL 3
#define SLOT_AUTO AUTO

/// Binds the resource to the given bind group and slot. Note, that this does not produce valid HLSL code, the code will instead be patched by the shader compiler.
#define BIND_RESOURCE(Slot, BindGroup) : register(W_PP_CONCAT(x, Slot), W_PP_CONCAT(space, BindGroup))

/// Binds the resource to the given bind group. Note, that this does not produce valid HLSL code, the code will instead be patched by the shader compiler.
#define BIND_GROUP(BindGroup) BIND_RESOURCE(SLOT_AUTO, BindGroup)
