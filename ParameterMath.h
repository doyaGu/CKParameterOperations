#ifndef CK_PARAMETER_MATH_H
#define CK_PARAMETER_MATH_H

#if __has_include("VxParameterMath.h")
#include "VxParameterMath.h"
#endif

// Stock SDKs and native VxMath builds use the host floating-point operations.
#ifndef VX_HAS_PARAMETER_MATH
#include <cmath>
#include <cstdint>
namespace VxParameterMath {
inline float Add(float a, float b) { return a + b; }
inline float Subtract(float a, float b) { return a - b; }
inline float Multiply(float a, float b) { return a * b; }
inline float Divide(float a, float b) { return a / b; }
inline float Scale(float value, float factor) { return value * factor; }
inline float MixedDifference(double a, double b) { return (float)(a - b); }
inline float MixedProduct(int32_t integer, float real) { return (float)((double)integer * real); }
inline float MixedQuotient(double a, double b) { return (float)(a / b); }
inline float Negate(float value) { return -value; }
inline float Absolute(float value) { return fabsf(value); }
inline float Sqrt(float value) { return sqrtf(value); }
inline float Asin(float value) { return asinf(value); }
inline float Acos(float value) { return acosf(value); }
inline float Sin(float value) { return sinf(value); }
inline float Cos(float value) { return cosf(value); }
inline float Tan(float value) { return tanf(value); }
inline float Atan2(float y, float x) { return atan2f(y, x); }
} // namespace VxParameterMath
#endif
#endif
