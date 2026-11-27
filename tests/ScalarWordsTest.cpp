#include "CKAll.h"
#include "../ParameterMath.h"
#include <cmath>
#include "ParameterOperationTypes.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

#ifndef VX_HAS_PARAMETER_MATH
static float fromWord(uint32_t w) {
    float value;
    std::memcpy(&value, &w, sizeof(value));
    return value;
}

static uint32_t asWord(float value) {
    uint32_t w;
    std::memcpy(&w, &value, sizeof(w));
    return w;
}

static bool sameNativeFloat(uint32_t actual, uint32_t expected) {
    return actual == expected || (std::isnan(fromWord(actual)) && std::isnan(fromWord(expected)));
}

#endif

static bool checkPerSecond(CKContext *ctx)
{
    struct Sample { CKGUID type; uint32_t ms, rate, expected; };
    const Sample samples[] = {
        {CKPGUID_FLOAT, 0x434bae6f, 0x3ffbaffe, 0x3ecd0e64},
        {CKPGUID_FLOAT, 0x4218ed3f, 0x3fcad339, 0x3d782398},
        {CKPGUID_FLOAT, 0x04b8bbe8, 0x3fc50205, 0x0048c9c2},
        {CKPGUID_FLOAT, 0x4309a883, 0x01b73511, 0x0064e146},
        {CKPGUID_FLOAT, 0x41a00000, 0x41400000, 0x3e75c290},
        {CKPGUID_FLOAT, 0x7f800000, 0x00000000, 0xffc00000},
        {CKPGUID_FLOAT, 0xff805678, 0x7f801234, 0xffc05678},
        {CKPGUID_VECTOR, 0x401bd623, 0x3ffd00fd, 0x39fc5592},
        {CKPGUID_VECTOR, 0x40e49e22, 0x3fda68f6, 0x3a9fc89d},
        {CKPGUID_VECTOR, 0x04a237ca, 0x3f90260f, 0x0004ad3c},
        {CKPGUID_VECTOR, 0x43e9fd2b, 0x0130266d, 0x00107ca4},
        {CKPGUID_VECTOR, 0x41a00000, 0x41400000, 0x3cc49ba5},
        {CKPGUID_VECTOR, 0x7f800000, 0x00000000, 0xffc00000},
        {CKPGUID_VECTOR, 0xff805678, 0x7f801234, 0xffc05678},
    };
    for (const auto &s : samples) {
        auto *op = ctx->CreateCKParameterOperation(nullptr, CKOGUID_PER_SECOND, s.type, s.type, CKPGUID_NONE);
        auto *rate = ctx->CreateCKParameterLocal(nullptr, s.type);
        if (!op || !rate || !op->GetOperationFunction() || op->GetInParameter1()->SetDirectSource(rate) != CK_OK)
            return false;
        const int count = s.type == CKPGUID_VECTOR ? 3 : 1;
        const uint32_t rates[] = {s.rate, s.rate, s.rate};
        if (rate->SetValue(rates, count * sizeof(uint32_t)) != CK_OK) return false;
        float delta;
        std::memcpy(&delta, &s.ms, sizeof(delta));
        ctx->GetTimeManager()->SetLastDeltaTime(delta);
        if (op->DoOperation() != CK_OK) return false;
        uint32_t result[3] = {};
        if (op->GetOutParameter()->GetValue(result, FALSE) != CK_OK) return false;
        for (int i = 0; i < count; ++i) {
#ifdef VX_HAS_PARAMETER_MATH
            const uint32_t expected = s.expected;
            const bool matches = result[i] == expected;
#else
            const float scale = delta * (s.type == CKPGUID_VECTOR ? 0.0001f : 0.001f);
            const uint32_t expected = asWord(fromWord(s.rate) * scale);
            const bool matches = sameNativeFloat(result[i], expected);
#endif
            if (!matches) {
                std::fprintf(stderr, "Per Second[%d] delta=%08x rate=%08x expected=%08x got=%08x\n",
                             i, s.ms, s.rate, expected, result[i]);
                return false;
            }
        }
    }
    return true;
}

struct Signature {
    CKGUID op, result, first, second;
    uint32_t left, right, expected;
};

#ifndef VX_HAS_PARAMETER_MATH
static uint32_t nativeExpected(const Signature &s) {
    const float a = fromWord(s.left), b = fromWord(s.right);
    if (s.second == CKPGUID_NONE) {
        if (s.op == CKOGUID_TANGENT) return asWord(std::tan(a));
        if (s.op == CKOGUID_COSINE) return asWord(std::cos(a));
        if (s.op == CKOGUID_SINE) return asWord(std::sin(a));
        if (s.op == CKOGUID_ARC_SINE) return asWord(std::asin(a));
        if (s.op == CKOGUID_ARC_COSINE) return asWord(std::acos(a));
        if (s.op == CKOGUID_SQUARE_ROOT) return asWord(std::sqrt(a));
        if (s.op == CKOGUID_OPPOSITE) return asWord(-a);
        if (s.op == CKOGUID_ABSOLUTE_VALUE) return asWord(std::fabs(a));
        if (s.op == CKOGUID_DEGRE_TO_RADIAN) return asWord(a * 0x1.1df46ap-6f);
        if (s.op == CKOGUID_RADIAN_TO_DEGRE) return asWord(a * 0x1.ca5dc0p+5f);
        return s.expected;
    }
    if (s.op == CKOGUID_ARC_TANGENT_2) return asWord(std::atan2(a, b));
    if (s.result == CKPGUID_INT && s.op == CKOGUID_ADDITION) return s.expected;
    const double left = s.first == CKPGUID_INT ? static_cast<double>(static_cast<int32_t>(s.left)) : a;
    const double right = s.second == CKPGUID_INT ? static_cast<double>(static_cast<int32_t>(s.right)) : b;
    double value;
    if (s.op == CKOGUID_ADDITION) value = left + right;
    else if (s.op == CKOGUID_SUBTRACTION) value = left - right;
    else if (s.op == CKOGUID_MULTIPLICATION) value = left * right;
    else value = left / (right == 0 ? 1 : right);
    const float stored = static_cast<float>(value);
    if (s.result == CKPGUID_FLOAT) return asWord(stored);
    return stored >= -0x1p63f && stored < 0x1p63f ? static_cast<uint32_t>(static_cast<int64_t>(stored)) : 0;
}
#endif

static bool check(CKContext *ctx)
{
    const Signature cases[] = {
        {CKOGUID_DIVISION, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0x7f800000, 0x7f800000, 0xffc00000},
        {CKOGUID_DIVISION, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0xff800000, 0x7f800000, 0xffc00000},
        {CKOGUID_DIVISION, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0xff801234, 0x7f805678, 0xffc01234},
        {CKOGUID_DIVISION, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0x7f805678, 0xff801234, 0x7fc05678},
        {CKOGUID_DIVISION, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0xff801234, 0x7fc05678, 0x7fc05678},
        {CKOGUID_DIVISION, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0xffc01234, 0x7f805678, 0xffc01234},
        {CKOGUID_DIVISION, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0xffc01234, 0x7fc01234, 0x7fc01234},
        {CKOGUID_DIVISION, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0x7fc01234, 0xffc01234, 0x7fc01234},
        {CKOGUID_DIVISION, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0x3f800000, 0xff801234, 0xffc01234},
        {CKOGUID_DIVISION, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0xff801234, 0x80000000, 0xffc01234},
        {CKOGUID_DIVISION, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0x00c00002, 0x40400000, 0x00400000},
        {CKOGUID_ADDITION, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0x7f800000, 0xff800000, 0xffc00000},
        {CKOGUID_MULTIPLICATION, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0x7fc01234, 0xff805678, 0xffc05678},
        {CKOGUID_SUBTRACTION, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0x7fc01234, 0xff805678, 0x7fc01234},
        {CKOGUID_ADDITION, CKPGUID_INT, CKPGUID_INT, CKPGUID_FLOAT, 7, 0xbfc00000, 6},
        {CKOGUID_ADDITION, CKPGUID_INT, CKPGUID_INT, CKPGUID_FLOAT, 7, 0x4f800001, 0x207},
        {CKOGUID_ADDITION, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_INT, 0x2f000000, 16777217, 0x4b800001},
        {CKOGUID_MULTIPLICATION, CKPGUID_INT, CKPGUID_INT, CKPGUID_FLOAT, 0x67c14431, 0x3fbb94d1, 0x980cfb00},
        {CKOGUID_MULTIPLICATION, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_INT, 0x3fbb94d1, 0x67c14431, 0x4f180cfb},
        {CKOGUID_MULTIPLICATION, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_INT, 0x7f800000, 0, 0xffc00000},
        {CKOGUID_DIVISION, CKPGUID_INT, CKPGUID_INT, CKPGUID_FLOAT, 0x5fc17223, 0x3ff28768, 0x32898340},
        {CKOGUID_DIVISION, CKPGUID_INT, CKPGUID_FLOAT, CKPGUID_INT, 0x4b800000, 16777217, 0},
        {CKOGUID_DIVISION, CKPGUID_FLOAT, CKPGUID_INT, CKPGUID_FLOAT, 1, 0x7f000003, 0x003ffffe},
        {CKOGUID_DIVISION, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_INT, 0x00c00002, 3, 0x00400000},
        {CKOGUID_DIVISION, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_INT, 0x3ff28768, 0x5fc17223, 0x30a2193b},
        {CKOGUID_SUBTRACTION, CKPGUID_FLOAT, CKPGUID_INT, CKPGUID_FLOAT, 16777217, 0x4b800000, 0x3f800000},
        {CKOGUID_SUBTRACTION, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_INT, 0x4b800000, 16777217, 0xbf800000},
        {CKOGUID_SUBTRACTION, CKPGUID_INT, CKPGUID_INT, CKPGUID_FLOAT, 0x7fffffff, 0xcf000000, 0},
        {CKOGUID_SUBTRACTION, CKPGUID_INT, CKPGUID_FLOAT, CKPGUID_INT, 0xcf000000, 0x7fffffff, 0},
        {CKOGUID_SUBTRACTION, CKPGUID_FLOAT, CKPGUID_INT, CKPGUID_FLOAT, 16777217, 0xaf000000, 0x4b800001},
        {CKOGUID_INVERSE, CKPGUID_FLOAT, CKPGUID_INT, CKPGUID_NONE, 16777217, 0, 0x337fffff},
        {CKOGUID_INVERSE, CKPGUID_INT, CKPGUID_FLOAT, CKPGUID_NONE, 0x2f7fffff, 0, 0x200},
        {CKOGUID_INVERSE, CKPGUID_INT, CKPGUID_FLOAT, CKPGUID_NONE, 0, 0, 0},
        {CKOGUID_OPPOSITE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0xff801234, 0, 0x7fc01234},
        {CKOGUID_OPPOSITE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x7f800123, 0, 0xffc00123},
        {CKOGUID_OPPOSITE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x00000000, 0, 0x80000000},
        {CKOGUID_ABSOLUTE_VALUE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0xff801234, 0, 0x7fc01234},
        {CKOGUID_ABSOLUTE_VALUE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x7f800123, 0, 0x7fc00123},
        {CKOGUID_ABSOLUTE_VALUE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x80000000, 0, 0x00000000},
        {CKOGUID_CONVERT, CKPGUID_INT, CKPGUID_FLOAT, CKPGUID_NONE, 0x4f800001, 0, 0x00000200},
        {CKOGUID_CONVERT, CKPGUID_INT, CKPGUID_FLOAT, CKPGUID_NONE, 0xcf800001, 0, 0xfffffe00},
        {CKOGUID_CONVERT, CKPGUID_INT, CKPGUID_FLOAT, CKPGUID_NONE, 0x5f000000, 0, 0x00000000},
        {CKOGUID_CONVERT, CKPGUID_INT, CKPGUID_FLOAT, CKPGUID_NONE, 0xff801234, 0, 0x00000000},
        {CKOGUID_SQUARE_ROOT, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0xbf800000, 0, 0xffc00000},
        {CKOGUID_SQUARE_ROOT, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x80000000, 0, 0x80000000},
        {CKOGUID_ARC_SINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0xbf800000, 0, 0xbfc90fdb},
        {CKOGUID_ARC_SINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x3f800001, 0, 0xffc00000},
        {CKOGUID_ARC_SINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0xff801234, 0, 0xffc01234},
        {CKOGUID_ARC_COSINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0xbf800001, 0, 0xffc00000},
        {CKOGUID_ARC_COSINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0xbf800000, 0, 0x40490fdb},
        {CKOGUID_ARC_COSINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0xbf7fffff, 0, 0x40490a32},
        {CKOGUID_ARC_COSINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x3f7fffe9, 0, 0x3ad908db},
        {CKOGUID_ARC_COSINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x3f7fffff, 0, 0x39b504f3},
        {CKOGUID_ARC_COSINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x3f800000, 0, 0x00000000},
        {CKOGUID_ARC_COSINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x3f800001, 0, 0xffc00000},
        {CKOGUID_ARC_COSINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x7f800000, 0, 0xffc00000},
        {CKOGUID_ARC_COSINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0xffc05678, 0, 0xffc05678},
        {CKOGUID_ARC_COSINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x7f800123, 0, 0x7fc00123},
        {CKOGUID_DEGRE_TO_RADIAN, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x033b230f, 0, 0x0068845a},
        {CKOGUID_DEGRE_TO_RADIAN, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x833b230f, 0, 0x8068845a},
        {CKOGUID_DEGRE_TO_RADIAN, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x4333ffff, 0, 0x40490fd9},
        {CKOGUID_DEGRE_TO_RADIAN, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x43340000, 0, 0x40490fdb},
        {CKOGUID_DEGRE_TO_RADIAN, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0xff801234, 0, 0xffc01234},
        {CKOGUID_DEGRE_TO_RADIAN, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x80000000, 0, 0x80000000},
        {CKOGUID_RADIAN_TO_DEGRE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x00017733, 0, 0x0053f954},
        {CKOGUID_RADIAN_TO_DEGRE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x80017733, 0, 0x8053f954},
        {CKOGUID_RADIAN_TO_DEGRE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x3fb118d5, 0, 0x429e8ba4},
        {CKOGUID_RADIAN_TO_DEGRE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x40490fdb, 0, 0x43340000},
        {CKOGUID_RADIAN_TO_DEGRE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0xff7fffff, 0, 0xff800000},
        {CKOGUID_RADIAN_TO_DEGRE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x7fc05678, 0, 0x7fc05678},
        {CKOGUID_SINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x80000000, 0, 0x80000000},
        {CKOGUID_SINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x3fc90fda, 0, 0x3f800000},
        {CKOGUID_SINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x3fc90fdb, 0, 0x3f800000},
        {CKOGUID_SINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x5effffff, 0, 0x3e544316},
        {CKOGUID_SINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0xdf000000, 0, 0xdf000000},
        {CKOGUID_SINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x7f800000, 0, 0xffc00000},
        {CKOGUID_SINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0xff801234, 0, 0xffc01234},
        {CKOGUID_SINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x3f21e6f7, 0, 0x3f1752c9},
        {CKOGUID_COSINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x80000000, 0, 0x3f800000},
        {CKOGUID_COSINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x3fc90fda, 0, 0x33a22169},
        {CKOGUID_COSINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x3fc90fdb, 0, 0xb33bbd2e},
        {CKOGUID_COSINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x5effffff, 0, 0xbf7a7091},
        {CKOGUID_COSINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0xdf000000, 0, 0xdf000000},
        {CKOGUID_COSINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x7f800000, 0, 0xffc00000},
        {CKOGUID_COSINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0xff801234, 0, 0xffc01234},
        {CKOGUID_COSINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x402f73d9, 0, 0xbf6bc6e8},
        {CKOGUID_TANGENT, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x80000000, 0, 0x80000000},
        {CKOGUID_TANGENT, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x3fc90fda, 0, 0x4b4a1bda},
        {CKOGUID_TANGENT, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x3fc90fdb, 0, 0xcbae8a4a},
        {CKOGUID_TANGENT, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x5effffff, 0, 0xbe58f987},
        {CKOGUID_TANGENT, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0xdf000000, 0, 0xffc00000},
        {CKOGUID_TANGENT, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x7f800000, 0, 0xffc00000},
        {CKOGUID_TANGENT, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0xff801234, 0, 0xffc01234},
        {CKOGUID_TANGENT, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x405df227, 0, 0x3ead4404},
        {CKOGUID_ARC_TANGENT_2, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0x40400000, 0x40000000, 0x3f7b985f},
        {CKOGUID_ARC_TANGENT_2, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0x40400000, 0xc0000000, 0x400a29c3},
        {CKOGUID_ARC_TANGENT_2, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0xc0400000, 0xc0000000, 0xc00a29c3},
        {CKOGUID_ARC_TANGENT_2, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0xc0400000, 0x40000000, 0xbf7b985f},
        {CKOGUID_ARC_TANGENT_2, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0x3f07521b, 0x3eccbcae, 0x3f6c53f0},
        {CKOGUID_ARC_TANGENT_2, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0x00000000, 0x80000000, 0x40490fdb},
        {CKOGUID_ARC_TANGENT_2, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0x80000000, 0x80000000, 0xc0490fdb},
        {CKOGUID_ARC_TANGENT_2, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0x80000000, 0x00000000, 0x80000000},
        {CKOGUID_ARC_TANGENT_2, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0x7f800000, 0xff800000, 0x4016cbe4},
        {CKOGUID_ARC_TANGENT_2, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0xff800000, 0x7f800000, 0xbf490fdb},
        {CKOGUID_ARC_TANGENT_2, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0xff800000, 0xc0000000, 0xbfc90fdb},
        {CKOGUID_ARC_TANGENT_2, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0xc0400000, 0x7f800000, 0x80000000},
        {CKOGUID_ARC_TANGENT_2, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0xff805678, 0x7fc01234, 0xffc05678},
        {CKOGUID_ARC_TANGENT_2, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0xffc01234, 0x7f801234, 0x7fc01234},
        {CKOGUID_ARC_TANGENT_2, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0x7fc01234, 0xff805678, 0xffc05678},
        {CKOGUID_ARC_TANGENT_2, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0xff801234, 0x40000000, 0xffc01234},
        {CKOGUID_ARC_TANGENT_2, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0x49867c20, 0x4addbdc6, 0x3e1a16c3},
        {CKOGUID_ARC_TANGENT_2, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0x4af0c9c2, 0x4b5df0a3, 0x3efe7bb7},
        {CKOGUID_ARC_TANGENT_2, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0x4afcfc0e, 0x4b7927d3, 0x3ef08678},
        {CKOGUID_ARC_TANGENT_2, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0x4b7443af, 0xca414938, 0x3fe20f8a},
        {CKOGUID_ARC_TANGENT_2, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0x4abf7cc2, 0xca9b987e, 0x401033a1},
        {CKOGUID_ARC_TANGENT_2, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0x805fbe61, 0x804dcc3f, 0xc01033a1},
    };
    for (const auto &s : cases) {
        auto *op = ctx->CreateCKParameterOperation(nullptr, s.op, s.result, s.first, s.second);
        auto *first = ctx->CreateCKParameterLocal(nullptr, s.first);
        if (!op || !op->GetOperationFunction() || !first || op->GetInParameter1()->SetDirectSource(first) != CK_OK)
            return false;
        std::memcpy(first->GetWriteDataPtr(), &s.left, 4);
        if (s.second != CKPGUID_NONE) {
            auto *second = ctx->CreateCKParameterLocal(nullptr, s.second);
            if (!second || op->GetInParameter2()->SetDirectSource(second) != CK_OK) return false;
            std::memcpy(second->GetWriteDataPtr(), &s.right, 4);
        }
        uint32_t word = 0xa55ac34e;
        auto *out = op->GetOutParameter();
        std::memcpy(out->GetWriteDataPtr(), &word, 4);
        if (op->DoOperation() != CK_OK) return false;
        std::memcpy(&word, out->GetReadDataPtr(FALSE), 4);
#ifdef VX_HAS_PARAMETER_MATH
        // MPFR at 256 bits rounds directly to ...a1. The binary80 store
        // rounds to the exact FLOAT midpoint and the subsequent store to ...a2.
        const bool midpoint = s.op == CKOGUID_ARC_TANGENT_2 &&
            ((s.left == 0x4abf7cc2 && s.right == 0xca9b987e) ||
             (s.left == 0x805fbe61 && s.right == 0x804dcc3f));
        const uint32_t expected = s.expected;
        const bool matches = word == expected || (midpoint && word == expected + 1);
#else
        const uint32_t expected = nativeExpected(s);
        const bool matches = s.result == CKPGUID_FLOAT ? sameNativeFloat(word, expected) : word == expected;
#endif
        if (!matches) {
            std::fprintf(stderr, "%08x %08x expected %08x got %08x\n", s.left, s.right, expected, word);
            return false;
        }
    }
    return true;
}

#ifdef CK_LIB
#define CKInitializeOperationFunctions CKParamOpInitializeOperationFunctions
#define CKInitializeOperationTypes CKParamOpInitializeOperationTypes
#endif
void CKInitializeOperationTypes(CKContext *);
void CKInitializeOperationFunctions(CKContext *);

int main()
{
    CKContext *ctx = nullptr;
    if (CKStartUp() != CK_OK) return 1;
    bool ok = CKCreateContext(&ctx, nullptr, 0, 0) == CK_OK && ctx;
    if (ok) {
        CKInitializeOperationTypes(ctx);
        CKInitializeOperationFunctions(ctx);
        ok = check(ctx) && checkPerSecond(ctx);
    }
    if (ctx) CKCloseContext(ctx);
    CKShutdown();
    return ok ? 0 : 1;
}
