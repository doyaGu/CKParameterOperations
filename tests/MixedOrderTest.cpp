#include "CKAll.h"
#include <cstdint>
#include <cstdio>
#include <cstring>

void CKBoolInfIntFloat(CKContext *, CKParameterOut *, CKParameterIn *, CKParameterIn *);
void CKBoolInfEqualIntFloat(CKContext *, CKParameterOut *, CKParameterIn *, CKParameterIn *);
void CKBoolSupIntFloat(CKContext *, CKParameterOut *, CKParameterIn *, CKParameterIn *);
void CKBoolSupEqualIntFloat(CKContext *, CKParameterOut *, CKParameterIn *, CKParameterIn *);
void CKBoolInfFloatInt(CKContext *, CKParameterOut *, CKParameterIn *, CKParameterIn *);
void CKBoolInfEqualFloatInt(CKContext *, CKParameterOut *, CKParameterIn *, CKParameterIn *);
void CKBoolSupFloatInt(CKContext *, CKParameterOut *, CKParameterIn *, CKParameterIn *);
void CKBoolSupEqualFloatInt(CKContext *, CKParameterOut *, CKParameterIn *, CKParameterIn *);

static bool compare(CKContext *ctx) {
    auto *integer = ctx->CreateCKParameterLocal(nullptr, CKPGUID_INT);
    auto *real = ctx->CreateCKParameterLocal(nullptr, CKPGUID_FLOAT);
    auto *i = ctx->CreateCKParameterIn(nullptr, CKPGUID_INT);
    auto *f = ctx->CreateCKParameterIn(nullptr, CKPGUID_FLOAT);
    auto *out = ctx->CreateCKParameterOut(nullptr, CKPGUID_BOOL);
    if (!i || !f || !integer || !real || !out || out->GetDataSize() != 4 ||
        i->SetDirectSource(integer) != CK_OK || f->SetDirectSource(real) != CK_OK) return false;

    const CK_PARAMETEROPERATION functions[][4] = {
        {CKBoolInfIntFloat, CKBoolInfEqualIntFloat, CKBoolSupIntFloat, CKBoolSupEqualIntFloat},
        {CKBoolInfFloatInt, CKBoolInfEqualFloatInt, CKBoolSupFloatInt, CKBoolSupEqualFloatInt}
    };
    enum { less = 3, equal = 10, greater = 12, unordered = 0 }; // Bits in <, <=, >, >= order.
    struct Sample { int32_t integer; uint32_t real; unsigned expected[2]; };
    const Sample samples[] = {
        {16777217, 0x4b800000, {greater, less}}, {16777216, 0x4b800000, {equal, equal}},
        {-16777217, 0xcb800000, {less, greater}}, {0, 0, {equal, equal}},
        {0, 0x80000000, {equal, equal}}, {0, 0x7fc01234, {unordered, unordered}},
        {0, 0x7f801234, {unordered, unordered}}, {1, 0x3fc00000, {less, greater}}
    };
    for (const auto &s : samples) {
        std::memcpy(integer->GetWriteDataPtr(), &s.integer, 4);
        std::memcpy(real->GetWriteDataPtr(), &s.real, 4);
        for (int order = 0; order < 2; ++order) {
            for (int rel = 0; rel < 4; ++rel) {
                uint32_t word = 0xa55ac340;
                std::memcpy(out->GetWriteDataPtr(), &word, 4);
                functions[order][rel](ctx, out, order ? f : i, order ? i : f);
                std::memcpy(&word, out->GetReadDataPtr(FALSE), 4);
                if (word != ((s.expected[order] >> rel) & 1u)) {
                    std::fprintf(stderr, "INT %d, FLOAT 0x%08x, order %d relation %d returned 0x%08x\n",
                                 s.integer, s.real, order, rel, word);
                    return false;
                }
            }
        }
    }
    return true;
}

int main() {
    CKContext *ctx = nullptr;
    if (CKStartUp() != CK_OK) return 1;
    bool ok = CKCreateContext(&ctx, nullptr, 0, 0) == CK_OK && ctx && compare(ctx);
    if (ctx) CKCloseContext(ctx);
    CKShutdown();
    return ok ? 0 : 1;
}
