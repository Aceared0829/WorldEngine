#pragma once

#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/ConstructionCounter.h>

#include <Foundation/Basics.h>
#include <Foundation/Basics/Assert.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/TypeTraits.h>
#include <Foundation/Types/Types.h>
#include <Foundation/Types/Variant.h>

#include <Foundation/Math/Declarations.h>

using WMathTestType = float;

using WVec2T = WVec2Template<WMathTestType>;                           ///< This is only for testing purposes
using WVec3T = WVec3Template<WMathTestType>;                           ///< This is only for testing purposes
using WVec4T = WVec4Template<WMathTestType>;                           ///< This is only for testing purposes
using WMat3T = WMat3Template<WMathTestType>;                           ///< This is only for testing purposes
using WMat4T = WMat4Template<WMathTestType>;                           ///< This is only for testing purposes
using WQuatT = WQuatTemplate<WMathTestType>;                           ///< This is only for testing purposes
using WAngleT = WAngleTemplate<WMathTestType>;                         ///< This is only for testing purposes
using WPlaneT = WPlaneTemplate<WMathTestType>;                         ///< This is only for testing purposes
using WBoundingBoxT = WBoundingBoxTemplate<WMathTestType>;             ///< This is only for testing purposes
using WBoundingBoxSphereT = WBoundingBoxSphereTemplate<WMathTestType>; ///< This is only for testing purposes
using WBoundingSphereT = WBoundingSphereTemplate<WMathTestType>;       ///< This is only for testing purposes
using WTransformT = WTransformTemplate<WMathTestType>;

#define WFoundationTest_Plugin1 "WFoundationTest_Plugin1"
#define WFoundationTest_Plugin2 "WFoundationTest_Plugin2"
