#include "CKAll.h"
#include <cstdint>
#include <cstdio>
#include <cstring>

void CKFloatMinFloatFloat(CKContext *, CKParameterOut *, CKParameterIn *, CKParameterIn *);
void CKFloatMaxFloatFloat(CKContext *, CKParameterOut *, CKParameterIn *, CKParameterIn *);

static bool check(CKContext *ctx)
{
    auto *a = ctx->CreateCKParameterLocal(nullptr, CKPGUID_FLOAT);
    auto *b = ctx->CreateCKParameterLocal(nullptr, CKPGUID_FLOAT);
    auto *l = ctx->CreateCKParameterIn(nullptr, CKPGUID_FLOAT);
    auto *r = ctx->CreateCKParameterIn(nullptr, CKPGUID_FLOAT);
    auto *out = ctx->CreateCKParameterOut(nullptr, CKPGUID_FLOAT);
    auto *copy = ctx->CreateCKParameterOperation(nullptr, CKGUID(0x701cca10, 0x399eada1),
                                                 CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE);
    if (!a || !b || !l || !r || !out || !copy || copy->GetOperationFunction() ||
        copy->GetInParameter1()->SetDirectSource(a) != CK_OK ||
        copy->GetOutParameter()->AddDestination(b) != CK_OK || r->SetDirectSource(b) != CK_OK) return false;

    const CK_PARAMETEROPERATION functions[] = {CKFloatMinFloatFloat, CKFloatMaxFloatFloat};
    struct Sample { uint32_t left, right, expected[2]; bool feedback; };
    const Sample samples[] = {
        {0x40400000, 0xc0e00000, {0x40400000, 0x40400000}, true},
        {0x40400000, 0x40e00000, {0x40400000, 0x40400000}, true},
        {0, 0x80000000, {0x80000000, 0x80000000}, false},
        {0x80000000, 0, {0, 0}, false},
        {0x7fc01234, 0xff801234, {0xff801234, 0xff801234}, false},
        {0xff805678, 0x3f800000, {0x3f800000, 0x3f800000}, false},
        {0xbf800000, 0xbf800001, {0xbf800001, 0xbf800000}, false}
    };
    for (const auto &s : samples) {
        CKParameter *source = a;
        if (s.feedback) source = copy->GetOutParameter();
        if (l->SetDirectSource(source) != CK_OK) return false;
        for (int k = 0; k < 2; ++k) {
            std::memcpy(a->GetWriteDataPtr(), &s.left, 4);
            std::memcpy(b->GetWriteDataPtr(), &s.right, 4);
            functions[k](ctx, out, l, r);
            uint32_t word;
            std::memcpy(&word, out->GetReadDataPtr(FALSE), 4);
            if (word != s.expected[k]) {
                std::fprintf(stderr, "%s(%08x, %08x) feedback=%d expected %08x got %08x\n",
                             k ? "Max" : "Min", s.left, s.right, s.feedback, s.expected[k], word);
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
