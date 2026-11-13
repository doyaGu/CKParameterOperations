#include "CKAll.h"
#include <cmath>
#include <cstdio>

void CKIntRandomIntInt(CKContext *, CKParameterOut *, CKParameterIn *, CKParameterIn *);
void CKBoolRandom(CKContext *, CKParameterOut *, CKParameterIn *, CKParameterIn *);
void CKVectorRandom(CKContext *, CKParameterOut *, CKParameterIn *, CKParameterIn *);
void CK2dVectorRandom(CKContext *, CKParameterOut *, CKParameterIn *, CKParameterIn *);

int main() {
    int failures = 0;
    auto check = [&](bool condition, const char *message) {
        if (!condition) { ++failures; std::fprintf(stderr, "FAIL: %s\n", message); }
    };
    CKStartUp();
    CKContext *first = nullptr, *second = nullptr;
    CKCreateContext(&first, nullptr, 0, 0); CKCreateContext(&second, nullptr, 0, 0);
    if (!first || !second) return 2;
    auto *vector = first->CreateCKParameterOut((CKSTRING)"vector", CKPGUID_VECTOR);
    auto *other = second->CreateCKParameterOut((CKSTRING)"vector", CKPGUID_VECTOR);
    first->SetRandomSeed(1); second->SetRandomSeed(1);
    CKVectorRandom(first, vector, nullptr, nullptr);
    for (int i = 0; i < 50; ++i) first->Random();
    CKVectorRandom(second, other, nullptr, nullptr);
    VxVector a, b;
    vector->GetValue(&a); other->GetValue(&b);
    check(a.x == b.x && a.y == b.y && a.z == b.z, "operation streams belong to their contexts");
    check(std::fabs(a.x - 41 * 0.000030518509f) < 1e-7f && a.y >= 0 && a.y <= 1 && a.z >= 0 && a.z <= 1,
          "vector operation retains legacy 15-bit scaling");
    auto *p1 = first->CreateCKParameterIn((CKSTRING)"minimum", CKPGUID_INT);
    auto *p2 = first->CreateCKParameterIn((CKSTRING)"maximum", CKPGUID_INT);
    auto *low = first->CreateCKParameterLocal((CKSTRING)"low", CKPGUID_INT);
    auto *high = first->CreateCKParameterLocal((CKSTRING)"high", CKPGUID_INT);
    p1->SetDirectSource(low); p2->SetDirectSource(high);
    int minimum = 3, maximum = 9;
    low->SetValue(&minimum); high->SetValue(&maximum);
    auto *integer = first->CreateCKParameterOut((CKSTRING)"integer", CKPGUID_INT);
    first->SetRandomSeed(1);
    CKIntRandomIntInt(first, integer, p1, p2);
    int result = 0; integer->GetValue(&result);
    check(result == 9 && first->Random() == 18467, "integer operation retains inclusive modulo and one draw");
    auto *boolean = first->CreateCKParameterOut((CKSTRING)"bool", CKPGUID_BOOL);
    first->SetRandomSeed(1); CKBoolRandom(first, boolean, nullptr, nullptr);
    boolean->GetValue(&result);
    check(result == 1 && first->Random() == 18467, "boolean operation retains its low-bit result");
    auto *pair = first->CreateCKParameterOut((CKSTRING)"pair", CKPGUID_2DVECTOR);
    first->SetRandomSeed(1); CK2dVectorRandom(first, pair, nullptr, nullptr);
    Vx2DVector xy; pair->GetValue(&xy);
    check(xy.x == 41 * (1.0f / 32768.0f) && xy.y == 18467 * (1.0f / 32768.0f),
          "2D operation retains its exclusive scaling and component order");
    CKCloseContext(first); CKCloseContext(second); CKShutdown();
    return failures ? 1 : 0;
}
