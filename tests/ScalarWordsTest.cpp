#include "CKAll.h"
#include "ParameterOperationTypes.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

static bool check(CKContext *ctx)
{
    struct Signature {
        CKGUID op, result, first, second;
        uint32_t left, right, expected;
    };
    const Signature cases[] = {
        {CKOGUID_ADDITION, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0x7f800000, 0xff800000, 0xffc00000},
        {CKOGUID_MULTIPLICATION, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0x7fc01234, 0xff805678, 0xffc05678},
        {CKOGUID_SUBTRACTION, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, 0x7fc01234, 0xff805678, 0x7fc01234},
        {CKOGUID_SUBTRACTION, CKPGUID_FLOAT, CKPGUID_INT, CKPGUID_FLOAT, 16777217, 0x4b800000, 0x3f800000},
        {CKOGUID_SUBTRACTION, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_INT, 0x4b800000, 16777217, 0xbf800000},
        {CKOGUID_SUBTRACTION, CKPGUID_INT, CKPGUID_INT, CKPGUID_FLOAT, 0x7fffffff, 0xcf000000, 0},
        {CKOGUID_SUBTRACTION, CKPGUID_INT, CKPGUID_FLOAT, CKPGUID_INT, 0xcf000000, 0x7fffffff, 0},
        {CKOGUID_SUBTRACTION, CKPGUID_FLOAT, CKPGUID_INT, CKPGUID_FLOAT, 16777217, 0xaf000000, 0x4b800001},
        {CKOGUID_INVERSE, CKPGUID_FLOAT, CKPGUID_INT, CKPGUID_NONE, 16777217, 0, 0x337fffff},
        {CKOGUID_INVERSE, CKPGUID_INT, CKPGUID_FLOAT, CKPGUID_NONE, 0x2f7fffff, 0, 0x200},
        {CKOGUID_INVERSE, CKPGUID_INT, CKPGUID_FLOAT, CKPGUID_NONE, 0, 0, 0},
        {CKOGUID_SQUARE_ROOT, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0xbf800000, 0, 0xffc00000},
        {CKOGUID_SQUARE_ROOT, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x80000000, 0, 0x80000000},
        {CKOGUID_ARC_SINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0xbf800000, 0, 0xbfc90fdb},
        {CKOGUID_ARC_SINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0x3f800001, 0, 0xffc00000},
        {CKOGUID_ARC_SINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, 0xff801234, 0, 0xffc01234},
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
        if (word != s.expected) {
            std::fprintf(stderr, "%08x %08x expected %08x got %08x\n", s.left, s.right, s.expected, word);
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
        ok = check(ctx);
    }
    if (ctx) CKCloseContext(ctx);
    CKShutdown();
    return ok ? 0 : 1;
}
