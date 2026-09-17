# Coding Guidelines

The coding guidelines in WorldEngine are enforced through clang-tidy. You can either run clang-tidy locally or use the automated CI process that runs in every PR into WorldEngine. CI will provide a git patch with all suggested changes which you can apply locally. After applying the suggested changes please make sure everything still compiles. The fixes done by clang-tidy are not garantueed to work in all cases.

## Type dependent prefixes 

Type dependent prefixes are mandatory for member variables and function parameters. Function names and variables in functions bodies can be named to the programmer's liking.

 * If the variable / member is an `WInt8`, `WInt16`, `WInt32`, `WInt64`, `WAtomicInteger32`, `WAtomicInteger64`, `ptrdiff_t`, `std::atomic<WInt*>` or any other signed integer type the prefix is 'i': `WInt32 iMyVar;`
 * If the variable / member is an `WUint8`, `WUInt16,` `WUInt32`, `WUInt64`, `size_t`, `std::atomic<WUInt*>` or any other unsigned integer type the prefix is 'ui': `WUInt32 uiMyVar;`
 * If the variable / member is a `float` or `double` the prefix is 'f': `float fMyVar;`
 * If the variable / member is a `bool`,`WAtomicBool` or `std::atomic<bool>` the prefix is 'b': `bool bMyVar;`
 * If the variable / member is a handle, the prefix is 'h': `WSpatialDataHandle hMyVar;`
 * If the variable / member is a raw pointer, `WSharedPtr`,`WUniquePtr`,`std::shared_ptr`,`std::unique_ptr` or `QPointer` the prefix is 'p': `WUInt32* pMyVar;`
 * If the variable / member is a `const char*` the prefix is 'sz' if it represent a zero terminated string, 'p' otherwise: `const char* szMyVar;`
 * If the variable / member is an WorldEngine string (`WString`, `WStringView`, etc) the prefix is 's': `WString sMyVar;`
 * If the variable / member is an WorldEngine vector (`WVec3`, `WVec4`, `WSimdVec4f`, etc) the prefix is 'v': `WVec3 vMyVar;`
 * If the variable / member is an WorldEngine quaternion (`WQuat`, `WQuatd`, `WSimdQuat`) the prefix is 'q': `eqQuat qMyVar;`
 * If the variable / member is an WorldEngine matrix (`WMat3`, `WMat4`, `WSimdMat4f`, etc) the prefix is 'm': `WMat3 mMyVar;`
 * If the variable / member is a fixed size array the prefix can be chosen freely. E.g. `bool m_bSomeBools[3];` or `bool m_SomeBools[3];`
 * In all other cases no prefix should be used.

## Members of structs and Classes

### Non-Static

If the member is public, no rules apply.

For private and protected members, the following rules apply:
 * All members must start with 'm_' (this comes before the type dependent prefix)
 * The name of the static member must be in PascalCase: `m_MyMember` 



 ### Static members
If the member is public, no rules apply.

For private and protected members, the following rules apply:
 * If the member is a constant, it should be marked *constexpr*: `static constexpr WInt32 MyConstant = 5;`
 * Otherwise the member must start with 's_' (this comes before the type dependent prefix)
 * The name of the static member must be in PascalCase: `s_MyMember` 

 ### Method / Function Parameters
 
 * Parameters use the same type specific prefixes
 * If a parameter is a regular reference, it is treated as if the reference didn't exist in regards to the type specific prefixes. E.g. `const bool& bValue`.
 * Single character parameters are allowed without type specific prefix.
 * The following special names are also allowed without type specific prefix: 
   - `lhs`
   - `rhs`
   - `other`
   - `value`
 * Non-const regular references must either start with `in_`, `inout_` or `out_`. Clang-tidy will automatically insert `ref_` as it can't decide the correct usage.
   - Use `in_` if the referenced value is not modified inside the function (for example when casting it to another type)
   - Use `inout_` if the referenced value is modified and the inital state of the object matters.
   - Use `out_` if the referenced value is completely overwritten inside the function and the original value doesn't matter.
   - Look out for `ref_` inserted by clang-tidy and replace it with either `in_`, `inout_` or `out_`
 * Pointers can have an `out_` prefix to indicate that this is an optional out parameter. 