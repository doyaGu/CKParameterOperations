#include "CKAll.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

void CKIntMinIntFloat(CKContext *, CKParameterOut *, CKParameterIn *, CKParameterIn *);
void CKIntMaxIntFloat(CKContext *, CKParameterOut *, CKParameterIn *, CKParameterIn *);
void CKFloatMinFloatInt(CKContext *, CKParameterOut *, CKParameterIn *, CKParameterIn *);
void CKFloatMaxFloatInt(CKContext *, CKParameterOut *, CKParameterIn *, CKParameterIn *);

static bool check(CKContext *ctx)
{
    auto *integer = ctx->CreateCKParameterLocal(nullptr, CKPGUID_INT);
    auto *real = ctx->CreateCKParameterLocal(nullptr, CKPGUID_FLOAT);
    auto *i = ctx->CreateCKParameterIn(nullptr, CKPGUID_INT);
    auto *f = ctx->CreateCKParameterIn(nullptr, CKPGUID_FLOAT);
    auto *intOut = ctx->CreateCKParameterOut(nullptr, CKPGUID_INT);
    auto *floatOut = ctx->CreateCKParameterOut(nullptr, CKPGUID_FLOAT);
    if (!integer || !real || !i || !f || !intOut || !floatOut ||
        i->SetDirectSource(integer) != CK_OK || f->SetDirectSource(real) != CK_OK) return false;

    struct Sample { int32_t integer; uint32_t real, expected[4]; };
    const CK_PARAMETEROPERATION functions[] = {CKIntMinIntFloat, CKIntMaxIntFloat, CKFloatMinFloatInt, CKFloatMaxFloatInt};
    const Sample samples[] = {
        {16777217, 0x4b800000, {0x01000000, 0x01000000, 0x4b800000, 0x4b800000}},
        {0, 0x80000000, {0, 0, 0, 0}},
        {-1, 0x80000000, {0xffffffff, 0, 0xbf800000, 0x80000000}},
        {37, 0x7fc01234, {0, 0, 0x42140000, 0x42140000}},
        {37, 0xff801234, {0, 0, 0x42140000, 0x42140000}},
        {-2, 0xbfe00000, {0xfffffffe, 0xffffffff, 0xc0000000, 0xbfe00000}},
        {0, 0x4f800001, {0, 0x200, 0, 0x4f800001}},
        {0, 0xcf800001, {0xfffffe00, 0, 0xcf800001, 0}},
        {5, 0x7f800000, {5, 0, 0x40a00000, 0x7f800000}},
        {17, 0x5f000000, {17, 0, 0x41880000, 0x5f000000}}
    };
    for (const auto &s : samples) {
        std::memcpy(integer->GetWriteDataPtr(), &s.integer, 4);
        std::memcpy(real->GetWriteDataPtr(), &s.real, 4);
        for (int k = 0; k < 4; ++k) {
            const bool floatResult = k >= 2;
            auto *out = floatResult ? floatOut : intOut;
            uint32_t word = 0xa55ac34e;
            std::memcpy(out->GetWriteDataPtr(), &word, 4);
            functions[k](ctx, out, floatResult ? f : i, floatResult ? i : f);
            std::memcpy(&word, out->GetReadDataPtr(FALSE), 4);
            if (word != s.expected[k]) {
                std::fprintf(stderr, "INT %d FLOAT %08x function %d expected %08x got %08x\n",
                             s.integer, s.real, k, s.expected[k], word);
                return false;
            }
        }
    }
    return true;
}

int main()
{
    CKContext *ctx = nullptr;
    if (CKStartUp() != CK_OK) return 1;
    bool ok = CKCreateContext(&ctx, nullptr, 0, 0) == CK_OK && ctx && check(ctx);
    if (ctx) CKCloseContext(ctx);
    CKShutdown();
    return ok ? 0 : 1;
}
