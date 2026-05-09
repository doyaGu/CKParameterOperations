#include "CKAll.h"

#include "ParameterOperationTypes.h"

#ifdef CK_LIB
    #define CKInitializeOperationFunctions CKParamOpInitializeOperationFunctions
#endif

//=============================================================================
// Canonical Global Temporaries (as per mode instructions)
//=============================================================================
VxMatrix mat_tmp;
VxQuaternion quaternion_tmp;
VxRect rect_tmp;
VxBbox box_tmp;
Vx2DVector vector2d_tmp;
VxVector vector_tmp;
XObjectArray xarray_tmp;
void *_p;

//=============================================================================
// Helper: Resolve the real source CKParameter behind a CKParameterIn
//=============================================================================
static CKParameter *GetSourceParameter(CKParameterIn *pin)
{
    return pin ? pin->GetRealSource() : NULL;
}

//=============================================================================
// Helper: Read raw data pointer from CKParameterIn, update _p side effect
//=============================================================================
static void *ReadDataPtr(CKParameterIn *pin)
{
    CKParameter *src = GetSourceParameter(pin);
    void *ptr = src ? src->GetReadDataPtr(TRUE) : NULL;
    _p = ptr;
    return ptr;
}

//=============================================================================
// Helper: Read float from CKParameterIn, update _p side effect
// Returns pointer to float data, or NULL if unavailable
//=============================================================================
static float *ReadFloatPtr(CKParameterIn *pin)
{
    CKParameter *src = pin->GetRealSource();
    float *ptr = src ? (float *)src->GetReadDataPtr(TRUE) : NULL;
    _p = ptr;
    return ptr;
}

static float ReadFloat(CKParameterIn *pin)
{
    float *ptr = ReadFloatPtr(pin);
    return ptr ? *ptr : 0.0f;
}

//=============================================================================
// Helper: Read int from CKParameterIn, update _p side effect
//=============================================================================
static int *ReadIntPtr(CKParameterIn *pin)
{
    CKParameter *src = pin->GetRealSource();
    int *ptr = src ? (int *)src->GetReadDataPtr(TRUE) : NULL;
    _p = ptr;
    return ptr;
}

static int ReadInt(CKParameterIn *pin)
{
    int *ptr = ReadIntPtr(pin);
    return ptr ? *ptr : 0;
}

//=============================================================================
// Helper: Read CK_ID from CKParameterIn, update _p side effect
//=============================================================================
static CK_ID ReadObjectID(CKParameterIn *pin)
{
    CKParameter *src = pin->GetRealSource();
    CK_ID *ptr = src ? (CK_ID *)src->GetReadDataPtr(TRUE) : NULL;
    _p = ptr;
    return ptr ? *ptr : 0;
}

//=============================================================================
// Helper: Read VxVector* from CKParameterIn, update _p side effect
// Falls back to global vector_tmp if NULL
//=============================================================================
static VxVector *ReadVectorPtr(CKParameterIn *pin)
{
    CKParameter *src = pin->GetRealSource();
    VxVector *ptr = src ? (VxVector *)src->GetReadDataPtr(TRUE) : NULL;
    _p = ptr;
    return ptr ? ptr : &vector_tmp;
}

//=============================================================================
// Helper: Read Vx2DVector* from CKParameterIn, update _p side effect
// Falls back to global vector2d_tmp if NULL
//=============================================================================
static Vx2DVector *Read2DVectorPtr(CKParameterIn *pin)
{
    CKParameter *src = pin->GetRealSource();
    Vx2DVector *ptr = src ? (Vx2DVector *)src->GetReadDataPtr(TRUE) : NULL;
    _p = ptr;
    return ptr ? ptr : &vector2d_tmp;
}

//=============================================================================
// Helper: Read VxQuaternion* from CKParameterIn, update _p side effect
// Falls back to global quaternion_tmp if NULL
//=============================================================================
static VxQuaternion *ReadQuaternionPtr(CKParameterIn *pin)
{
    CKParameter *src = pin->GetRealSource();
    VxQuaternion *ptr = src ? (VxQuaternion *)src->GetReadDataPtr(TRUE) : NULL;
    _p = ptr;
    return ptr ? ptr : &quaternion_tmp;
}

//=============================================================================
// Helper: Handle divide-by-zero for float division
// Sets divisor to 1.0 and logs error if divisor is 0.0
//=============================================================================
static void HandleDivByZeroFloat(float *divisor, CKContext *context, CKParameter *res)
{
    if (*divisor == 0.0f)
    {
        *divisor = 1.0f;
        CKObject *owner = res->GetOwner();
        const char *name = "";
        if (CKIsChildClassOf(owner, CKCID_BEHAVIOR))
        {
            CKBehavior *beh = (CKBehavior *)owner;
            CKBeObject *behOwner = beh->GetOwner();
            if (behOwner)
                name = behOwner->GetName();
        }
        else if (CKIsChildClassOf(owner, CKCID_PARAMETEROPERATION))
        {
            CKParameterOperation *op = (CKParameterOperation *)owner;
            CKBehavior *beh = (CKBehavior *)op->GetOwner();
            if (beh)
            {
                CKBeObject *behOwner = beh->GetOwner();
                if (behOwner)
                    name = behOwner->GetName();
            }
        }
        if (!name)
            name = "";
        context->OutputToConsoleEx("ParamerOperation 'Divide' : Divide by zero! in [%s]\n", name);
    }
}

//=============================================================================
// Helper: Handle divide-by-zero for int division (similar pattern)
//=============================================================================
static void HandleDivByZeroInt(int *divisor, CKContext *context, CKParameter *res)
{
    if (*divisor == 0)
    {
        *divisor = 1;
        CKObject *owner = res->GetOwner();
        const char *name = "";
        if (CKIsChildClassOf(owner, CKCID_BEHAVIOR))
        {
            CKBehavior *beh = (CKBehavior *)owner;
            CKBeObject *behOwner = beh->GetOwner();
            if (behOwner)
                name = behOwner->GetName();
        }
        else if (CKIsChildClassOf(owner, CKCID_PARAMETEROPERATION))
        {
            CKParameterOperation *op = (CKParameterOperation *)owner;
            CKBehavior *beh = (CKBehavior *)op->GetOwner();
            if (beh)
            {
                CKBeObject *behOwner = beh->GetOwner();
                if (behOwner)
                    name = behOwner->GetName();
            }
        }
        if (!name)
            name = "";
        context->OutputToConsoleEx("ParamerOperation 'Divide' : Divide by zero! in [%s]\n", name);
    }
}

// Original symbol: ?CKFloatPerSecondFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B422F0
// Multiplies input float by delta time (seconds)
void CKFloatPerSecondFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Get delta time in seconds (ms * 0.001)
    CKTimeManager *tm = context->GetTimeManager();
    float deltaTime = tm->GetLastDeltaTime() * 0.001f;

    float val = ReadFloat(p1);
    *(float *)res->GetWriteDataPtr() = val * deltaTime;
}

// Original symbol: ?CKFloatOppositeFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B41920
// Negates the input float: result = -p1
void CKFloatOppositeFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float val = ReadFloat(p1);
    *(float *)res->GetWriteDataPtr() = -val;
}

// Original symbol: ?CKFloatAbsoluteFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B41990
// Returns absolute value of input float: result = |p1|
void CKFloatAbsoluteFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float val = ReadFloat(p1);
    *(float *)res->GetWriteDataPtr() = (float)fabs(val);
}

// Original symbol: ?CKFloatAddFloatFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B41A00
// Adds two floats: result = p1 + p2
void CKFloatAddFloatFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float v1 = ReadFloat(p1);
    float v2 = ReadFloat(p2);
    *(float *)res->GetWriteDataPtr() = v1 + v2;
}

// Original symbol: ?CKFloatSubtractFloatFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B41AB0
// Subtracts two floats: result = p1 - p2
void CKFloatSubtractFloatFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float v1 = ReadFloat(p1);
    float v2 = ReadFloat(p2);
    *(float *)res->GetWriteDataPtr() = v1 - v2;
}

// Original symbol: ?CKFloatMultiplyFloatFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B41B70
// Multiplies two floats: result = p1 * p2
void CKFloatMultiplyFloatFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float v1 = ReadFloat(p1);
    float v2 = ReadFloat(p2);
    *(float *)res->GetWriteDataPtr() = v1 * v2;
}

// Original symbol: ?CKFloatDivideFloatFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B41C20
// Divides two floats: result = p1 / p2 (with divide-by-zero handling)
void CKFloatDivideFloatFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Read divisor first (matches original binary order)
    float v2 = ReadFloat(p2);
    HandleDivByZeroFloat(&v2, context, res);

    float v1 = ReadFloat(p1);
    *(float *)res->GetWriteDataPtr() = v1 / v2;
}

// Original symbol: ?CKFloatMaxFloatFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B41CF0
// Returns maximum of two floats: result = max(p1, p2)
void CKFloatMaxFloatFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Note: original reads p2 first, then p1
    float *ptr2 = ReadFloatPtr(p2);
    float v2 = ptr2 ? *ptr2 : 0.0f;

    float *ptr1 = ReadFloatPtr(p1);
    float v1 = ptr1 ? *ptr1 : 0.0f;

    // Original uses FPU comparison: if v1 > v2 then result = v1, else result = v2
    float result = (v1 > v2) ? v1 : v2;
    *(float *)res->GetWriteDataPtr() = result;
}

// Original symbol: ?CKFloatMinFloatFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B41D90
// Returns minimum of two floats: result = min(p1, p2)
void CKFloatMinFloatFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Note: original reads p2 first, then p1
    float *ptr2 = ReadFloatPtr(p2);
    float v2 = ptr2 ? *ptr2 : 0.0f;

    float *ptr1 = ReadFloatPtr(p1);
    float v1 = ptr1 ? *ptr1 : 0.0f;

    // Original uses FPU comparison: if v1 < v2 then result = v1, else result = v2
    float result = (v1 < v2) ? v1 : v2;
    *(float *)res->GetWriteDataPtr() = result;
}

// Original symbol: ?CKFloatRandomFloatFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B41E30
// Returns random float in range [p1, p2]: result = p1 + rand() * (1/32768) * (p2 - p1)
void CKFloatRandomFloatFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Note: original reads p1 for min, then p2 for max, then p1 again for min
    // This seems intentional (maybe for side-effect tracking)
    float minVal = ReadFloat(p1);
    float maxVal = ReadFloat(p2);
    float minVal2 = ReadFloat(p1);  // Re-read p1 as per original binary

    // rand() scaled to [0, 1) then mapped to [min, max]
    // Original constant: 0.000030518499 (approx 1.0 / 32768.0)
    float randNorm = (float)rand() * 3.0518499e-05f;
    float result = minVal + randNorm * (maxVal - minVal2);

    *(float *)res->GetWriteDataPtr() = result;
}

// Original symbol: ?CKFloatInverseFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B41F40
// Returns reciprocal of float: result = 1.0 / p1
void CKFloatInverseFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float val = ReadFloat(p1);
    *(float *)res->GetWriteDataPtr() = 1.0f / val;
}

// Original symbol: ?CKFloatSinusFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B41FA0
// Returns sine of input (in radians): result = sin(p1)
void CKFloatSinusFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float val = ReadFloat(p1);
    *(float *)res->GetWriteDataPtr() = sinf(val);
}

// Original symbol: ?CKFloatSqrtFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B42280
// Returns square root: result = sqrt(p1)
void CKFloatSqrtFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float val = ReadFloat(p1);
    *(float *)res->GetWriteDataPtr() = sqrtf(val);
}

// Original symbol: ?CKFloatCosinusFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B42010
// Returns cosine of input (in radians): result = cos(p1)
void CKFloatCosinusFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float val = ReadFloat(p1);
    *(float *)res->GetWriteDataPtr() = cosf(val);
}

// Original symbol: ?CKFloatTanFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B42080
// Returns tangent of input (in radians): result = tan(p1)
void CKFloatTanFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float val = ReadFloat(p1);
    *(float *)res->GetWriteDataPtr() = tanf(val);
}

// Original symbol: ?CKFloatArcTanFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B420F0
// Returns atan2(p1, p2): two-argument arctangent
void CKFloatArcTanFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Note: original reads p2 first (x), then p1 (y), and uses fpatan instruction
    float x = ReadFloat(p2);
    float y = ReadFloat(p1);
    *(float *)res->GetWriteDataPtr() = atan2f(y, x);
}

// Original symbol: ?CKFloatArcCosFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B42210
// Returns arc cosine: result = acos(p1)
void CKFloatArcCosFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float val = ReadFloat(p1);
    *(float *)res->GetWriteDataPtr() = acosf(val);
}

// Original symbol: ?CKFloatArcSinFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B421A0
// Returns arc sine: result = asin(p1)
void CKFloatArcSinFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float val = ReadFloat(p1);
    *(float *)res->GetWriteDataPtr() = asinf(val);
}

// Original symbol: ?CKFloatAddFloatInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B42370
// result = float(p1) + (float)int(p2)
void CKFloatAddFloatInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float val = ReadFloat(p1);
    int ival = ReadInt(p2);
    *(float *)res->GetWriteDataPtr() = val + (float)ival;
}

// Original symbol: ?CKFloatSubtractFloatInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B42420
// result = float(p1) - (float)int(p2)
void CKFloatSubtractFloatInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float val = ReadFloat(p1);
    int ival = ReadInt(p2);
    *(float *)res->GetWriteDataPtr() = val - (float)ival;
}

// Original symbol: ?CKFloatSubtractIntFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B424D0
// result = (float)int(p1) - float(p2)
void CKFloatSubtractIntFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    int ival = ReadInt(p1);
    float fval = ReadFloat(p2);
    *(float *)res->GetWriteDataPtr() = (float)ival - fval;
}

// Original symbol: ?CKFloatMultiplyFloatInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B42590
// result = float(p1) * (float)int(p2)
void CKFloatMultiplyFloatInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float val = ReadFloat(p1);
    int ival = ReadInt(p2);
    *(float *)res->GetWriteDataPtr() = val * (float)ival;
}

// Original symbol: ?CKFloatDivideFloatInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B42640
// result = float(p1) / (float)int(p2), with div-by-zero check
void CKFloatDivideFloatInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    int ival = ReadInt(p2);
    HandleDivByZeroInt(&ival, context, res);
    float fval = ReadFloat(p1);
    *(float *)res->GetWriteDataPtr() = fval / (float)ival;
}

// Original symbol: ?CKFloatDivideIntFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B42710
// result = (float)int(p1) / float(p2), with div-by-zero check
void CKFloatDivideIntFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float fval = ReadFloat(p2);
    HandleDivByZeroFloat(&fval, context, res);
    int ival = ReadInt(p1);
    *(float *)res->GetWriteDataPtr() = (float)ival / fval;
}

// Original symbol: ?CKFloatMaxFloatInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B427E0
// result = max(float(p1), (float)int(p2))
void CKFloatMaxFloatInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float fval = ReadFloat(p1);
    int ival = ReadInt(p2);
    // If (float)ival < fval, use fval; otherwise convert ival to float result
    if ((float)ival < fval)
        *(float *)res->GetWriteDataPtr() = fval;
    else
        *(float *)res->GetWriteDataPtr() = (float)ival;
}

// Original symbol: ?CKFloatMinFloatInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B42950
// result = min(float(p1), (float)int(p2))
void CKFloatMinFloatInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float fval = ReadFloat(p1);
    int ival = ReadInt(p2);
    // If (float)ival > fval, use fval; otherwise convert ival to float result
    if ((float)ival > fval)
        *(float *)res->GetWriteDataPtr() = fval;
    else
        *(float *)res->GetWriteDataPtr() = (float)ival;
}

// Original symbol: ?CKFloatSetInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B42AC0
// result = (float)int(p1) - converts int to float
void CKFloatSetInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    int ival = ReadInt(p1);
    *(float *)res->GetWriteDataPtr() = (float)ival;
}

// Original symbol: ?CKFloatDegreToRadianFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B42B30
// Converts degrees to radians: result = p1 * (PI/180)
void CKFloatDegreToRadianFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float val = ReadFloat(p1);
    // Original constant: 0.017453292 (approx PI/180)
    *(float *)res->GetWriteDataPtr() = val * 0.017453292f;
}

// Original symbol: ?CKFloatRadianToDegreFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B42B90
// Converts radians to degrees: result = p1 * (180/PI)
void CKFloatRadianToDegreFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float val = ReadFloat(p1);
    // Original constant: 57.295776 (approx 180/PI)
    *(float *)res->GetWriteDataPtr() = val * 57.295776f;
}

// Original symbol: ?CKFloatInverseInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B42BF0
// result = 1.0 / (float)int(p1) - inverse of int as float
void CKFloatInverseInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    int ival = ReadInt(p1);
    *(float *)res->GetWriteDataPtr() = 1.0f / (float)ival;
}

// Original symbol: ?CKFloatGetDistance3dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B42CC0
// Calculates distance between two 3D entities
void CKFloatGetDistance3dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id1 = ReadObjectID(p1);
    CK3dEntity *ent1 = (CK3dEntity *)context->GetObject(id1);
    if (!ent1)
        return;

    CK_ID id2 = ReadObjectID(p2);
    CK3dEntity *ent2 = (CK3dEntity *)context->GetObject(id2);
    if (!ent2)
        return;

    VxVector pos1, pos2;
    ent1->GetPosition(&pos1, NULL);
    ent2->GetPosition(&pos2, NULL);

    float dx = pos1.x - pos2.x;
    float dy = pos1.y - pos2.y;
    float dz = pos1.z - pos2.z;
    *(float *)res->GetWriteDataPtr() = sqrtf(dx * dx + dy * dy + dz * dz);
}

// Original symbol: ?CKFloatGetX3dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B42DF0
// Gets X coordinate of 3D entity position
void CKFloatGetX3dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CK3dEntity *ent = (CK3dEntity *)context->GetObject(id);
    if (!ent)
        return;

    VxVector pos;
    ent->GetPosition(&pos, NULL);
    *(float *)res->GetWriteDataPtr() = pos.x;
}

// Original symbol: ?CKFloatGetY3dentity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B42E70
// Gets Y coordinate of 3D entity position
void CKFloatGetY3dentity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CK3dEntity *ent = (CK3dEntity *)context->GetObject(id);
    if (!ent)
        return;

    VxVector pos;
    ent->GetPosition(&pos, NULL);
    *(float *)res->GetWriteDataPtr() = pos.y;
}

// Original symbol: ?CKFloatGetZ3dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B42EF0
// Gets Z coordinate of 3D entity position
void CKFloatGetZ3dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CK3dEntity *ent = (CK3dEntity *)context->GetObject(id);
    if (!ent)
        return;

    VxVector pos;
    ent->GetPosition(&pos, NULL);
    *(float *)res->GetWriteDataPtr() = pos.z;
}

// Original symbol: ?CKFloatGetRadius3dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B42F70
// Gets radius of 3D entity bounding sphere
void CKFloatGetRadius3dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CK3dEntity *ent = (CK3dEntity *)context->GetObject(id);
    if (!ent)
        return;

    *(float *)res->GetWriteDataPtr() = ent->GetRadius();
}

// Original symbol: ?CKFloatGetXEuler@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B430D0
// Gets X component of Euler angles (VxVector)
void CKFloatGetXEuler(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxVector *vec = ReadVectorPtr(p1);
    *(float *)res->GetWriteDataPtr() = vec->x;
}

// Original symbol: ?CKFloatGetYEuler@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B42FE0
// Gets Y component of Euler angles (VxVector)
void CKFloatGetYEuler(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxVector *vec = ReadVectorPtr(p1);
    *(float *)res->GetWriteDataPtr() = vec->y;
}

// Original symbol: ?CKFloatGetZEuler@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B43120
// Gets Z component of Euler angles (VxVector)
void CKFloatGetZEuler(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxVector *vec = ReadVectorPtr(p1);
    *(float *)res->GetWriteDataPtr() = vec->z;
}

// Original symbol: ?CKFloatGetX2dVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B43030
// Gets X component of 2D vector
void CKFloatGetX2dVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    Vx2DVector *vec = Read2DVectorPtr(p1);
    *(float *)res->GetWriteDataPtr() = vec->x;
}

// Original symbol: ?CKFloatGetY2dVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B43080
// Gets Y component of 2D vector
void CKFloatGetY2dVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    Vx2DVector *vec = Read2DVectorPtr(p1);
    *(float *)res->GetWriteDataPtr() = vec->y;
}

// Original symbol: ?CKFloatGetXQuaternion@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B43170
// Gets X component of quaternion
void CKFloatGetXQuaternion(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxQuaternion *quat = ReadQuaternionPtr(p1);
    *(float *)res->GetWriteDataPtr() = quat->x;
}

// Original symbol: ?CKFloatGetYQuaternion@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B431C0
// Gets Y component of quaternion
void CKFloatGetYQuaternion(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxQuaternion *quat = ReadQuaternionPtr(p1);
    *(float *)res->GetWriteDataPtr() = quat->y;
}

// Original symbol: ?CKFloatGetZQuaternion@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B43210
// Gets Z component of quaternion
void CKFloatGetZQuaternion(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxQuaternion *quat = ReadQuaternionPtr(p1);
    *(float *)res->GetWriteDataPtr() = quat->z;
}

// Original symbol: ?CKFloatGetWQuaternion@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B43260
// Gets W component of quaternion
void CKFloatGetWQuaternion(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxQuaternion *quat = ReadQuaternionPtr(p1);
    *(float *)res->GetWriteDataPtr() = quat->w;
}

// Original symbol: ?CKFloatDotProductVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B432B0
// Computes dot product of two 3D vectors
void CKFloatDotProductVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxVector v1(0.0f, 0.0f, 0.0f);
    VxVector v2(0.0f, 0.0f, 0.0f);

    CKParameter *src1 = GetSourceParameter(p1);
    if (src1)
        src1->GetValue(&v1, TRUE);

    CKParameter *src2 = GetSourceParameter(p2);
    if (src2)
        src2->GetValue(&v2, TRUE);

    *(float *)res->GetWriteDataPtr() = v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

// Original symbol: ?CKFloatGetDistanceVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B43350
// Computes distance between two 3D vectors
void CKFloatGetDistanceVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxVector v1(0.0f, 0.0f, 0.0f);
    VxVector v2(0.0f, 0.0f, 0.0f);

    CKParameter *src1 = GetSourceParameter(p1);
    if (src1)
        src1->GetValue(&v1, TRUE);

    CKParameter *src2 = GetSourceParameter(p2);
    if (src2)
        src2->GetValue(&v2, TRUE);

    float dx = v1.x - v2.x;
    float dy = v1.y - v2.y;
    float dz = v1.z - v2.z;
    *(float *)res->GetWriteDataPtr() = sqrtf(dx * dx + dy * dy + dz * dz);
}

// Original symbol: ?CKFloatGetAngleVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B43420
// Computes angle between two 3D vectors (returns radians)
void CKFloatGetAngleVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxVector v1(0.0f, 0.0f, 0.0f);
    VxVector v2(0.0f, 0.0f, 0.0f);

    CKParameter *src1 = GetSourceParameter(p1);
    if (src1)
        src1->GetValue(&v1, TRUE);

    CKParameter *src2 = GetSourceParameter(p2);
    if (src2)
        src2->GetValue(&v2, TRUE);

    // Normalize v1
    float mag1 = sqrtf(v1.x * v1.x + v1.y * v1.y + v1.z * v1.z);
    if (mag1 > 0.0f)
    {
        float invMag1 = 1.0f / mag1;
        v1.x *= invMag1;
        v1.y *= invMag1;
        v1.z *= invMag1;
    }

    // Normalize v2
    float mag2 = sqrtf(v2.x * v2.x + v2.y * v2.y + v2.z * v2.z);
    if (mag2 > 0.0f)
    {
        float invMag2 = 1.0f / mag2;
        v2.x *= invMag2;
        v2.y *= invMag2;
        v2.z *= invMag2;
    }

    // Compute dot product and clamp to [-1, 1]
    float dot = v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
    if (dot < -1.0f)
        dot = -1.0f;
    else if (dot > 1.0f)
        dot = 1.0f;

    *(float *)res->GetWriteDataPtr() = acosf(dot);
}

// Original symbol: ?CKFloatGetAngle2dVector2dVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B43820
// Computes angle between two 2D vectors (returns radians)
void CKFloatGetAngle2dVector2dVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    Vx2DVector v1(0.0f, 0.0f);
    Vx2DVector v2(0.0f, 0.0f);

    CKParameter *src1 = GetSourceParameter(p1);
    if (src1)
        src1->GetValue(&v1, TRUE);

    CKParameter *src2 = GetSourceParameter(p2);
    if (src2)
        src2->GetValue(&v2, TRUE);

    // Normalize v1
    float mag1 = sqrtf(v1.x * v1.x + v1.y * v1.y);
    if (mag1 > 0.0f)
    {
        float invMag1 = 1.0f / mag1;
        v1.x *= invMag1;
        v1.y *= invMag1;
    }

    // Normalize v2
    float mag2 = sqrtf(v2.x * v2.x + v2.y * v2.y);
    if (mag2 > 0.0f)
    {
        float invMag2 = 1.0f / mag2;
        v2.x *= invMag2;
        v2.y *= invMag2;
    }

    // Compute dot product of normalized vectors
    float dot = v1.x * v2.x + v1.y * v2.y;

    *(float *)res->GetWriteDataPtr() = acosf(dot);
}

// Original symbol: ?CKFloatDotProduct2dVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B43640
// Computes dot product of two 2D vectors
void CKFloatDotProduct2dVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    Vx2DVector v1(0.0f, 0.0f);
    Vx2DVector v2(0.0f, 0.0f);

    CKParameter *src1 = GetSourceParameter(p1);
    if (src1)
        src1->GetValue(&v1, TRUE);

    CKParameter *src2 = GetSourceParameter(p2);
    if (src2)
        src2->GetValue(&v2, TRUE);

    *(float *)res->GetWriteDataPtr() = v1.x * v2.x + v1.y * v2.y;
}

// Original symbol: ?CKFloatGetDistance2dVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B436F0
// Computes distance between two 2D vectors
void CKFloatGetDistance2dVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    Vx2DVector v1(0.0f, 0.0f);
    Vx2DVector v2(0.0f, 0.0f);

    CKParameter *src1 = GetSourceParameter(p1);
    if (src1)
        src1->GetValue(&v1, TRUE);

    CKParameter *src2 = GetSourceParameter(p2);
    if (src2)
        src2->GetValue(&v2, TRUE);

    float dx = v1.x - v2.x;
    float dy = v1.y - v2.y;
    *(float *)res->GetWriteDataPtr() = sqrtf(dx * dx + dy * dy);
}

// Original symbol: ?CKFloatGetDistance2dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B43950
// Computes 2D distance between two 2D entities (X and Y positions)
void CKFloatGetDistance2dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id1 = ReadObjectID(p1);
    CK2dEntity *ent1 = (CK2dEntity *)context->GetObject(id1);
    if (!ent1)
        return;

    CK_ID id2 = ReadObjectID(p2);
    CK2dEntity *ent2 = (CK2dEntity *)context->GetObject(id2);
    if (!ent2)
        return;

    Vx2DVector pos1(0.0f, 0.0f);
    Vx2DVector pos2(0.0f, 0.0f);

    ent1->GetPosition(pos1, FALSE, FALSE);
    ent2->GetPosition(pos2, FALSE, FALSE);

    float dx = pos1.x - pos2.x;
    float dy = pos1.y - pos2.y;
    *(float *)res->GetWriteDataPtr() = sqrtf(dx * dx + dy * dy);
}

// Original symbol: ?CKFloatGetRangeLight@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B43A80
// Gets range of a light object
void CKFloatGetRangeLight(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKLight *light = (CKLight *)context->GetObject(id);
    if (!light)
        return;

    *(float *)res->GetWriteDataPtr() = light->GetRange();
}

// Original symbol: ?CKFloatGetFovCamera@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B43B60
// Gets field of view (FOV) of a camera
void CKFloatGetFovCamera(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKCamera *camera = (CKCamera *)context->GetObject(id);
    if (!camera)
        return;

    *(float *)res->GetWriteDataPtr() = camera->GetFov();
}

// Original symbol: ?CKFloatGetLengthCurve@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B43C40
// Gets the total length of a CKCurve (3D spline)
void CKFloatGetLengthCurve(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKCurve *curve = (CKCurve *)context->GetObject(id);
    if (!curve)
        return;

    *(float *)res->GetWriteDataPtr() = curve->GetLength();
}

// Original symbol: ?CKFloatGetBackPlaneCamera@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B43AF0
// Gets back plane (far clipping plane) of a camera
void CKFloatGetBackPlaneCamera(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKCamera *camera = (CKCamera *)context->GetObject(id);
    if (!camera)
        return;

    *(float *)res->GetWriteDataPtr() = camera->GetBackPlane();
}

// Original symbol: ?CKFloatGetZoomCamera@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B43BD0
// Gets zoom factor of a camera
void CKFloatGetZoomCamera(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKCamera *camera = (CKCamera *)context->GetObject(id);
    if (!camera)
        return;

    *(float *)res->GetWriteDataPtr() = camera->GetOrthographicZoom();
}

// Original symbol: ?CKFloatGetLengthCurveCurvePoint@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B43CB0
// Gets the length from start of curve to this curve point
// Note: First reads the curve (p1) to validate, then gets length from the CurvePoint (p2)
void CKFloatGetLengthCurveCurvePoint(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // First validate that p1 contains a valid curve
    CK_ID curveId = ReadObjectID(p1);
    CKCurve *curve = (CKCurve *)context->GetObject(curveId);
    if (!curve)
        return;

    // Then get the length from the curve point (p2)
    CK_ID curvePointId = ReadObjectID(p2);
    CKCurvePoint *curvePoint = (CKCurvePoint *)context->GetObject(curvePointId);
    if (!curvePoint)
        return;

    *(float *)res->GetWriteDataPtr() = curvePoint->GetLength();
}

// Original symbol: ?CKFloatGetLength2dCurve@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B43D70
// Gets the length of a CK2dCurve
// Note: CK2dCurve is NOT a CKObject, it's stored as a direct pointer in the parameter
void CKFloatGetLength2dCurve(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKParameter *param = GetSourceParameter(p1);
    if (!param)
        return;

    _p = param->GetReadDataPtr(TRUE);
    if (!_p)
        return;

    // Parameter stores a pointer to CK2dCurve (not a CK_ID)
    CK2dCurve *curve = *(CK2dCurve **)_p;
    if (!curve)
        return;

    *(float *)res->GetWriteDataPtr() = curve->GetLength();
}

// Original symbol: ?CKFloatGetY2dCurveFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B43DD0
// Gets Y value at given X position on a CK2dCurve
void CKFloatGetY2dCurveFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Read CK2dCurve pointer from p1
    CKParameter *param1 = GetSourceParameter(p1);
    if (!param1)
        return;

    _p = param1->GetReadDataPtr(TRUE);
    if (!_p)
        return;

    CK2dCurve *curve = *(CK2dCurve **)_p;
    if (!curve)
        return;

    // Read float X value from p2
    float x = 0.0f;
    CKParameter *param2 = GetSourceParameter(p2);
    if (param2) {
        _p = param2->GetReadDataPtr(TRUE);
        if (_p)
            x = *(float *)_p;
    }

    *(float *)res->GetWriteDataPtr() = curve->GetY(x);
}

// Original symbol: ?CKFloatGetLengthAnimation@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B43EA0
// Gets the total length of an animation
void CKFloatGetLengthAnimation(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKAnimation *animation = (CKAnimation *)context->GetObject(id);
    if (!animation)
        return;

    *(float *)res->GetWriteDataPtr() = animation->GetLength();
}

// Original symbol: ?CKFloatGetMagnitudeVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B435C0
// Computes magnitude (length) of a 3D vector
void CKFloatGetMagnitudeVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxVector *vec = ReadVectorPtr(p1);
    float magSq = vec->x * vec->x + vec->y * vec->y + vec->z * vec->z;
    *(float *)res->GetWriteDataPtr() = sqrtf(magSq);
}

// Original symbol: ?CKFloatGetMagnitude2dVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B437B0
// Computes magnitude (length) of a 2D vector
void CKFloatGetMagnitude2dVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    Vx2DVector *vec = Read2DVectorPtr(p1);
    float magSq = vec->x * vec->x + vec->y * vec->y;
    *(float *)res->GetWriteDataPtr() = sqrtf(magSq);
}

// Original symbol: ?CKGenericSetString@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B412D0
// Sets a string parameter from input
void CKGenericSetString(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKParameter *param = GetSourceParameter(p1);
    if (!param) {
        res->SetStringValue(NULL);
        return;
    }
    char *str = (char *)param->GetReadDataPtr(TRUE);
    res->SetStringValue(str);
}

// Original symbol: ?CKFloatGetVertexWeightMeshInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B43FF0
// Gets the vertex weight at a specific index from a mesh
void CKFloatGetVertexWeightMeshInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID meshId = ReadObjectID(p1);
    CKMesh *mesh = (CKMesh *)context->GetObject(meshId);
    if (!mesh)
        return;

    int index = ReadInt(p2);
    *(float *)res->GetWriteDataPtr() = mesh->GetVertexWeight(index);
}

// Original symbol: ?CKIntAbsoluteInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B447D0
// Computes absolute value of an integer
void CKIntAbsoluteInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    int val = ReadInt(p1);
    *(int *)res->GetWriteDataPtr() = abs(val);
}

// Original symbol: ?CKIntOppositeInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B44830
// Computes negation of an integer
void CKIntOppositeInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    int *ptr = ReadIntPtr(p1);
    if (ptr)
        *(int *)res->GetWriteDataPtr() = -(*ptr);
    else
        *(int *)res->GetWriteDataPtr() = 0;
}

// Original symbol: ?CKIntAddIntInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B44890
// Adds two integers
void CKIntAddIntInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    int val1 = ReadInt(p1);
    int *ptr2 = ReadIntPtr(p2);
    if (ptr2)
        *(int *)res->GetWriteDataPtr() = val1 + *ptr2;
    else
        *(int *)res->GetWriteDataPtr() = val1;
}

// Original symbol: ?CKIntAndIntInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B44930
// Bitwise AND of two integers
void CKIntAndIntInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    int val1 = ReadInt(p1);
    int *ptr2 = ReadIntPtr(p2);
    if (ptr2)
        *(int *)res->GetWriteDataPtr() = val1 & *ptr2;
    else
        *(int *)res->GetWriteDataPtr() = 0;
}

// Original symbol: ?CKIntOrIntInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B449D0
// Bitwise OR of two integers
void CKIntOrIntInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    int val1 = ReadInt(p1);
    int *ptr2 = ReadIntPtr(p2);
    if (ptr2)
        *(int *)res->GetWriteDataPtr() = val1 | *ptr2;
    else
        *(int *)res->GetWriteDataPtr() = val1;
}

// Original symbol: ?CKIntXorIntInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B44A70
// Bitwise XOR of two integers
void CKIntXorIntInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    int val1 = ReadInt(p1);
    int *ptr2 = ReadIntPtr(p2);
    if (ptr2)
        *(int *)res->GetWriteDataPtr() = val1 ^ *ptr2;
    else
        *(int *)res->GetWriteDataPtr() = val1;
}

// Original symbol: ?CKIntSubtractIntInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B44B10
// Subtracts second integer from first
void CKIntSubtractIntInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    int val1 = ReadInt(p1);
    int *ptr2 = ReadIntPtr(p2);
    if (ptr2)
        *(int *)res->GetWriteDataPtr() = val1 - *ptr2;
    else
        *(int *)res->GetWriteDataPtr() = val1;
}

// Original symbol: ?CKIntMultiplyIntInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B44BB0
// Multiplies two integers
void CKIntMultiplyIntInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    int val1 = ReadInt(p1);
    int *ptr2 = ReadIntPtr(p2);
    if (ptr2)
        *(int *)res->GetWriteDataPtr() = val1 * *ptr2;
    else
        *(int *)res->GetWriteDataPtr() = 0;
}

// Original symbol: ?CKIntDivideIntInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B44C50
// Divides first integer by second (with division by zero handling)
void CKIntDivideIntInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    int val2 = ReadInt(p2);
    HandleDivByZeroInt(&val2, context, res);
    int val1 = ReadInt(p1);
    *(int *)res->GetWriteDataPtr() = val1 / val2;
}

// Original symbol: ?CKIntMaxIntInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B44D00
// Returns the maximum of two integers
void CKIntMaxIntInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    int val2 = ReadInt(p2);
    int val1 = ReadInt(p1);
    *(int *)res->GetWriteDataPtr() = (val1 > val2) ? val1 : val2;
}

// Original symbol: ?CKIntMinIntInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B44DB0
// Returns the minimum of two integers
void CKIntMinIntInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    int val2 = ReadInt(p2);
    int val1 = ReadInt(p1);
    *(int *)res->GetWriteDataPtr() = (val1 < val2) ? val1 : val2;
}

// Original symbol: ?CKIntRandomIntInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B44E60
// Returns a random integer in range [p1, p2]
// Order: read p2 (max), read p1 (min), compute range, HandleDivByZeroInt, re-read p1, compute result
void CKIntRandomIntInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    int maxVal = ReadInt(p2);
    int minVal = ReadInt(p1);
    int range = maxVal - minVal + 1;
    HandleDivByZeroInt(&range, context, res);
    int minValAgain = ReadInt(p1);
    *(int *)res->GetWriteDataPtr() = minValAgain + rand() % range;
}

// Original symbol: ?CKIntAddIntFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B45010
// Adds int and float, returns truncated int result
void CKIntAddIntFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    int val1 = ReadInt(p1);
    float val2 = ReadFloat(p2);
    *(int *)res->GetWriteDataPtr() = (int)(val1 + val2);
}

// Original symbol: ?CKIntSubtractIntFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B450C0
// Subtracts float from int, returns truncated int result
void CKIntSubtractIntFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    int val1 = ReadInt(p1);
    float val2 = ReadFloat(p2);
    *(int *)res->GetWriteDataPtr() = (int)(val1 - val2);
}

// Original symbol: ?CKIntSubtractFloatInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B45170
// Subtracts int from float, returns truncated int result
void CKIntSubtractFloatInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float val1 = ReadFloat(p1);
    int val2 = ReadInt(p2);
    *(int *)res->GetWriteDataPtr() = (int)(val1 - val2);
}

// Original symbol: ?CKIntMultiplyIntFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B45220
// Multiplies int by float, returns truncated int result
void CKIntMultiplyIntFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    int val1 = ReadInt(p1);
    float val2 = ReadFloat(p2);
    *(int *)res->GetWriteDataPtr() = (int)(val1 * val2);
}

// Original symbol: ?CKIntDivideIntFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B452D0
// Divides int by float, returns truncated int result
void CKIntDivideIntFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float val2 = ReadFloat(p2);
    HandleDivByZeroFloat(&val2, context, res);
    int val1 = ReadInt(p1);
    *(int *)res->GetWriteDataPtr() = (int)(val1 / val2);
}

// Original symbol: ?CKIntDivideFloatInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B453A0
// Divides float by int, returns truncated int result
void CKIntDivideFloatInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    int val2 = ReadInt(p2);
    HandleDivByZeroInt(&val2, context, res);
    float val1 = ReadFloat(p1);
    *(int *)res->GetWriteDataPtr() = (int)(val1 / val2);
}

// Original symbol: ?CKIntMaxIntFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B45470
// Returns the maximum of int and float (as int)
void CKIntMaxIntFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    int val1 = ReadInt(p1);
    float val2 = ReadFloat(p2);
    if ((float)val1 > val2)
        *(int *)res->GetWriteDataPtr() = val1;
    else
        *(int *)res->GetWriteDataPtr() = (int)val2;
}

// Original symbol: ?CKIntMinIntFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B455C0
// Returns the minimum of int and float (as int)
void CKIntMinIntFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    int val1 = ReadInt(p1);
    float val2 = ReadFloat(p2);
    if ((float)val1 < val2)
        *(int *)res->GetWriteDataPtr() = val1;
    else
        *(int *)res->GetWriteDataPtr() = (int)val2;
}

// Original symbol: ?CKIntSetFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B45710
// Converts float to int (truncates)
void CKIntSetFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float val = ReadFloat(p1);
    *(int *)res->GetWriteDataPtr() = (int)val;
}

// Original symbol: ?CKIntInverseFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B45780
// Returns 1.0/float as int (inverse of float truncated)
void CKIntInverseFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float val = ReadFloat(p1);
    *(int *)res->GetWriteDataPtr() = (int)(1.0f / val);
}

// Original symbol: ?CKIntGetLengthString@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B45860
// Returns length of string
void CKIntGetLengthString(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKParameter *param = GetSourceParameter(p1);
    if (param) {
        const char *str = (const char *)param->GetReadDataPtr(TRUE);
        if (str) {
            *(int *)res->GetWriteDataPtr() = (int)strlen(str);
            return;
        }
    }
    *(int *)res->GetWriteDataPtr() = 0;
}

// Original symbol: ?IntGetWidthNoneNone@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B50C10
// Returns width of player render context
void IntGetWidthNoneNone(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKRenderContext *rc = context->GetPlayerRenderContext();
    if (rc)
        *(int *)res->GetWriteDataPtr() = rc->GetWidth();
    else
        *(int *)res->GetWriteDataPtr() = 0;
}

// Original symbol: ?IntGetHeightNoneNone@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B50C50
// Returns height of player render context
void IntGetHeightNoneNone(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKRenderContext *rc = context->GetPlayerRenderContext();
    if (rc)
        *(int *)res->GetWriteDataPtr() = rc->GetHeight();
    else
        *(int *)res->GetWriteDataPtr() = 0;
}

// Original symbol: ?CKIntModuloIntInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B44F60
// Modulo operation with sign correction (always positive result)
void CKIntModuloIntInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    int val1 = ReadInt(p1);
    int *ptr2 = ReadIntPtr(p2);
    if (ptr2)
    {
        int divisor = *ptr2;
        if (divisor != 0)
        {
            int result = val1 % divisor;
            if (result < 0)
                result += divisor;
            *(int *)res->GetWriteDataPtr() = result;
        }
        else
        {
            *(int *)res->GetWriteDataPtr() = 0;
        }
    }
    else
    {
        *(int *)res->GetWriteDataPtr() = 0;
    }
}

// Original symbol: ?CKIntGetWidthTexture@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B458C0
// Gets width of a texture
void CKIntGetWidthTexture(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKTexture *texture = (CKTexture *)context->GetObject(id);
    if (texture)
        *(int *)res->GetWriteDataPtr() = texture->GetWidth();
}

// Original symbol: ?CKIntGetHeightTexture@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B45920
// Gets height of a texture
void CKIntGetHeightTexture(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKTexture *texture = (CKTexture *)context->GetObject(id);
    if (texture)
        *(int *)res->GetWriteDataPtr() = texture->GetHeight();
}

// Original symbol: ?CKIntGetSlotCountTexture@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B45980
// Gets slot count of a texture
void CKIntGetSlotCountTexture(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKTexture *texture = (CKTexture *)context->GetObject(id);
    if (texture)
        *(int *)res->GetWriteDataPtr() = texture->GetSlotCount();
}

// Original symbol: ?CKIntGetCurrentTexture@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B459F0
// Gets current slot index of a texture
void CKIntGetCurrentTexture(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKTexture *texture = (CKTexture *)context->GetObject(id);
    if (texture)
        *(int *)res->GetWriteDataPtr() = texture->GetCurrentSlot();
}

// Original symbol: ?CKIntGetWidth2dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B45A60
// Gets width of a 2d entity via GetSize(Vx2DVector, FALSE)
void CKIntGetWidth2dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CK2dEntity *entity = (CK2dEntity *)context->GetObject(id);
    if (entity)
    {
        Vx2DVector size(0.0f, 0.0f);
        entity->GetSize(size, FALSE);
        *(int *)res->GetWriteDataPtr() = (int)size.x;
    }
}

// Original symbol: ?CKIntGetHeight2dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B45AF0
// Gets height of a 2d entity via GetSize(Vx2DVector, FALSE)
void CKIntGetHeight2dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CK2dEntity *entity = (CK2dEntity *)context->GetObject(id);
    if (entity)
    {
        Vx2DVector size(0.0f, 0.0f);
        entity->GetSize(size, FALSE);
        *(int *)res->GetWriteDataPtr() = (int)size.y;
    }
}

// Original symbol: ?CKIntGetTypeObject@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B45B80
// Gets the class ID (CK_CLASSID) of an object
void CKIntGetTypeObject(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKObject *obj = context->GetObject(id);
    if (obj)
        *(int *)res->GetWriteDataPtr() = obj->GetClassID();
}

// Original symbol: ?CKIntGetCountObjectArray@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B45BF0
// Gets count of elements in an XObjectArray
// XObjectArray parameter is stored directly as pointer, not CK_ID
void CKIntGetCountObjectArray(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    XObjectArray *arr = NULL;
    CKParameter *src = GetSourceParameter(p1);
    if (src)
        src->GetValue(&arr, TRUE);
    if (arr)
    {
        *(int *)res->GetWriteDataPtr() = arr->Size();
    }
    else
    {
        *(int *)res->GetWriteDataPtr() = 0;
    }
}

// Original symbol: ?CKIntGetRowCountDataArray@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B46040
// Gets row count of a CKDataArray
void CKIntGetRowCountDataArray(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKDataArray *arr = (CKDataArray *)context->GetObject(id);
    if (arr)
        *(int *)res->GetWriteDataPtr() = arr->GetRowCount();
}

// Original symbol: ?CKIntGetCountCurve@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B46270
// Gets control point count of a CKCurve
void CKIntGetCountCurve(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKCurve *curve = (CKCurve *)context->GetObject(id);
    if (curve)
        *(int *)res->GetWriteDataPtr() = curve->GetControlPointCount();
}

// Original symbol: ?CKIntGetColumnCountDataArray@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B460B0
// Gets column count of a CKDataArray
void CKIntGetColumnCountDataArray(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKDataArray *arr = (CKDataArray *)context->GetObject(id);
    if (arr)
        *(int *)res->GetWriteDataPtr() = arr->GetColumnCount();
}

// Original symbol: ?CKIntGetSlotCountSprite@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B46120
// Gets slot count of a CKSprite
void CKIntGetSlotCountSprite(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKSprite *sprite = (CKSprite *)context->GetObject(id);
    if (sprite)
        *(int *)res->GetWriteDataPtr() = sprite->GetSlotCount();
}

// Original symbol: ?CKIntGetCurrentSprite@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B46190
// Gets current slot index of a CKSprite
void CKIntGetCurrentSprite(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKSprite *sprite = (CKSprite *)context->GetObject(id);
    if (sprite)
        *(int *)res->GetWriteDataPtr() = sprite->GetCurrentSlot();
}

// Original symbol: ?CKIntGetScriptCountBeObject@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B462E0
// Gets the number of scripts attached to a CKBeObject
void CKIntGetScriptCountBeObject(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKBeObject *beo = (CKBeObject *)context->GetObject(id);
    if (beo)
        *(int *)res->GetWriteDataPtr() = beo->GetScriptCount();
}

// Original symbol: ?CKIntGetVertexCountMesh@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B46350
// Gets the number of vertices in a CKMesh
void CKIntGetVertexCountMesh(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKMesh *mesh = (CKMesh *)context->GetObject(id);
    if (mesh)
        *(int *)res->GetWriteDataPtr() = mesh->GetVertexCount();
}

// Original symbol: ?CKIntGetFaceCountMesh@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B463C0
// Gets the number of faces in a CKMesh
void CKIntGetFaceCountMesh(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKMesh *mesh = (CKMesh *)context->GetObject(id);
    if (mesh)
        *(int *)res->GetWriteDataPtr() = mesh->GetFaceCount();
}

// Original symbol: ?CKIntGetRenderedProgressiveMeshVerticesCount@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B46430
// Gets the number of vertices rendered for a progressive mesh
void CKIntGetRenderedProgressiveMeshVerticesCount(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKMesh *mesh = (CKMesh *)context->GetObject(id);
    if (mesh)
    {
        // Only works if the mesh is progressive
        if (mesh->IsPM())
            *(int *)res->GetWriteDataPtr() = mesh->GetVerticesRendered();
    }
}

// Original symbol: ?CKIntGetMaterialCountMesh@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B464B0
// Gets the number of materials used by a mesh
void CKIntGetMaterialCountMesh(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKMesh *mesh = (CKMesh *)context->GetObject(id);
    if (mesh)
        *(int *)res->GetWriteDataPtr() = mesh->GetMaterialCount();
}

// Original symbol: ?CKIntGetChannelCountMesh@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B46520
// Gets the number of channels in a mesh
void CKIntGetChannelCountMesh(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKMesh *mesh = (CKMesh *)context->GetObject(id);
    if (mesh)
        *(int *)res->GetWriteDataPtr() = mesh->GetChannelCount();
}

// Original symbol: ?CKIntGetChannelByMaterialMeshMaterial@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B46590
// Gets the channel index that uses a specific material
void CKIntGetChannelByMaterialMeshMaterial(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID meshId = ReadObjectID(p1);
    CKMesh *mesh = (CKMesh *)context->GetObject(meshId);
    if (mesh)
    {
        CK_ID matId = ReadObjectID(p2);
        CKMaterial *mat = (CKMaterial *)context->GetObject(matId);
        if (mat)
            *(int *)res->GetWriteDataPtr() = mesh->GetChannelByMaterial(mat);
    }
}

// Original symbol: ?CKBoolXorBoolBool@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B46650
// Boolean XOR operation
void CKBoolXorBoolBool(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKBOOL val1 = 0, val2 = 0;
    CKBYTE *ptr1 = (CKBYTE *)ReadDataPtr(p1);
    if (ptr1)
        val1 = (*ptr1 != 0);
    CKBYTE *ptr2 = (CKBYTE *)ReadDataPtr(p2);
    if (ptr2)
    {
        val2 = (*ptr2 != 0);
        *(int *)res->GetWriteDataPtr() = val1 ^ val2;
    }
    else
    {
        *(int *)res->GetWriteDataPtr() = val1;
    }
}

// Original symbol: ?CKBoolOrBoolBool@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B46700
// Boolean OR operation (short-circuit)
void CKBoolOrBoolBool(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKBYTE *ptr1 = (CKBYTE *)ReadDataPtr(p1);
    // Short-circuit: if p1 is true, result is true
    if (ptr1 && *ptr1)
    {
        *(int *)res->GetWriteDataPtr() = 1;
        return;
    }
    CKBYTE *ptr2 = (CKBYTE *)ReadDataPtr(p2);
    if (ptr2 && *ptr2)
    {
        *(int *)res->GetWriteDataPtr() = 1;
    }
    else
    {
        *(int *)res->GetWriteDataPtr() = 0;
    }
}

// Original symbol: ?CKBoolAndBoolBool@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B467B0
// Boolean AND operation (short-circuit)
void CKBoolAndBoolBool(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKBYTE *ptr1 = (CKBYTE *)ReadDataPtr(p1);
    // Short-circuit: if p1 is false, result is false
    if (!ptr1 || !*ptr1)
    {
        *(int *)res->GetWriteDataPtr() = 0;
        return;
    }
    CKBYTE *ptr2 = (CKBYTE *)ReadDataPtr(p2);
    if (ptr2 && *ptr2)
    {
        *(int *)res->GetWriteDataPtr() = 1;
    }
    else
    {
        *(int *)res->GetWriteDataPtr() = 0;
    }
}

// Original symbol: ?CKBoolNotBool@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B469C0
// Boolean NOT operation
void CKBoolNotBool(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKBYTE *ptr = (CKBYTE *)ReadDataPtr(p1);
    if (ptr)
    {
        CKBOOL val = (*ptr != 0);
        *(int *)res->GetWriteDataPtr() = !val;
    }
    else
    {
        // NULL pointer treated as false, so NOT gives true
        *(int *)res->GetWriteDataPtr() = 1;
    }
}

// Original symbol: ?CKBoolRandom@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B46A30
// Returns a random boolean value (0 or 1)
void CKBoolRandom(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    *(int *)res->GetWriteDataPtr() = rand() & 1;
}

// Original symbol: ?CKBoolInfFloatFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B46A50
// Returns true if p1 < p2 (float comparison)
void CKBoolInfFloatFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float val1 = ReadFloat(p1);
    float val2 = ReadFloat(p2);
    *(int *)res->GetWriteDataPtr() = (val1 < val2) ? 1 : 0;
}

// Original symbol: ?CKBoolSupFloatFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B46B10
// Returns true if p1 > p2 (float comparison)
void CKBoolSupFloatFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float val1 = ReadFloat(p1);
    float val2 = ReadFloat(p2);
    *(int *)res->GetWriteDataPtr() = (val1 > val2) ? 1 : 0;
}

// Original symbol: ?CKBoolInfEqualFloatFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B46BD0
// Returns true if p1 <= p2 (float comparison)
void CKBoolInfEqualFloatFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float val1 = ReadFloat(p1);
    float val2 = ReadFloat(p2);
    *(int *)res->GetWriteDataPtr() = (val1 <= val2) ? 1 : 0;
}

// Original symbol: ?CKBoolSupEqualFloatFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B46C90
// Returns true if p1 >= p2 (float comparison)
void CKBoolSupEqualFloatFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float val1 = ReadFloat(p1);
    float val2 = ReadFloat(p2);
    *(int *)res->GetWriteDataPtr() = (val1 >= val2) ? 1 : 0;
}

// Original symbol: ?CKBoolInfFloatInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B46D50
// Returns true if float(p1) > int(p2) (i.e., p2_as_float < p1)
void CKBoolInfFloatInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float val1 = ReadFloat(p1);
    int val2 = ReadInt(p2);
    *(int *)res->GetWriteDataPtr() = ((float)val2 < val1) ? 1 : 0;
}

// Original symbol: ?CKBoolSupFloatInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B46E20
// Returns true if int(p2) < float(p1) (i.e., p2_as_float < p1)
void CKBoolSupFloatInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float val1 = ReadFloat(p1);
    int val2 = ReadInt(p2);
    *(int *)res->GetWriteDataPtr() = ((float)val2 < val1) ? 1 : 0;
}

// Original symbol: ?CKBoolInfEqualFloatInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B46EF0
// Returns true if int(p2) >= float(p1) (i.e., p2_as_float >= p1)
void CKBoolInfEqualFloatInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float val1 = ReadFloat(p1);
    int val2 = ReadInt(p2);
    *(int *)res->GetWriteDataPtr() = ((float)val2 >= val1) ? 1 : 0;
}

// Original symbol: ?CKBoolSupEqualFloatInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B46FC0
// Returns true if int(p2) <= float(p1) (i.e., p2_as_float <= p1)
void CKBoolSupEqualFloatInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float val1 = ReadFloat(p1);
    int val2 = ReadInt(p2);
    *(int *)res->GetWriteDataPtr() = ((float)val2 <= val1) ? 1 : 0;
}

// Original symbol: ?CKBoolInfIntFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B47090
// Returns true if int(p1) < float(p2)
void CKBoolInfIntFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    int val1 = ReadInt(p1);
    float val2 = ReadFloat(p2);
    *(int *)res->GetWriteDataPtr() = ((float)val1 < val2) ? 1 : 0;
}

// Original symbol: ?CKBoolSupIntFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B47150
// Returns true if int(p1) > float(p2)
void CKBoolSupIntFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    int val1 = ReadInt(p1);
    float val2 = ReadFloat(p2);
    *(int *)res->GetWriteDataPtr() = ((float)val1 > val2) ? 1 : 0;
}

// Original symbol: ?CKBoolInfEqualIntFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B47210
// Returns true if int(p1) <= float(p2)
void CKBoolInfEqualIntFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    int val1 = ReadInt(p1);
    float val2 = ReadFloat(p2);
    *(int *)res->GetWriteDataPtr() = ((float)val1 <= val2) ? 1 : 0;
}

// Original symbol: ?CKBoolSupEqualIntFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B472D0
// Returns true if int(p1) >= float(p2)
void CKBoolSupEqualIntFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    int val1 = ReadInt(p1);
    float val2 = ReadFloat(p2);
    *(int *)res->GetWriteDataPtr() = ((float)val1 >= val2) ? 1 : 0;
}

// Original symbol: ?CKBoolInfIntInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B47480
// Returns true if int(p1) < int(p2)
void CKBoolInfIntInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    int val1 = ReadInt(p1);
    int val2 = ReadInt(p2);
    *(int *)res->GetWriteDataPtr() = (val1 < val2) ? 1 : 0;
}

// Original symbol: ?CKBoolSupIntInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B47510
// Returns true if int(p1) > int(p2)
void CKBoolSupIntInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    int val1 = ReadInt(p1);
    int val2 = ReadInt(p2);
    *(int *)res->GetWriteDataPtr() = (val1 > val2) ? 1 : 0;
}

// Original symbol: ?CKBoolInfEqualIntInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B475A0
// Returns true if int(p1) <= int(p2)
void CKBoolInfEqualIntInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    int val1 = ReadInt(p1);
    int val2 = ReadInt(p2);
    *(int *)res->GetWriteDataPtr() = (val1 <= val2) ? 1 : 0;
}

// Original symbol: ?CKBoolSupEqualIntInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B47630
// Returns true if int(p1) >= int(p2)
void CKBoolSupEqualIntInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    int val1 = ReadInt(p1);
    int val2 = ReadInt(p2);
    *(int *)res->GetWriteDataPtr() = (val1 >= val2) ? 1 : 0;
}

// Original symbol: ?CKBoolDerivedFromIdId@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B47AD0
// Checks if object p1's class is derived from object p2's class
void CKBoolDerivedFromIdId(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id1 = ReadObjectID(p1);
    CKObject *obj1 = context->GetObject(id1);
    if (obj1)
    {
        CK_ID id2 = ReadObjectID(p2);
        CKObject *obj2 = context->GetObject(id2);
        if (obj2)
        {
            CK_CLASSID classId2 = obj2->GetClassID();
            *(int *)res->GetWriteDataPtr() = CKIsChildClassOf(obj1, classId2);
        }
    }
}

// Original symbol: ?CKBoolCollisionBoxVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B48030
// Checks if vector (point) is inside the bounding box
void CKBoolCollisionBoxVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Read box from p1
    void *ptr1 = ReadDataPtr(p1);
    _p = ptr1;
    float *box = ptr1 ? (float *)ptr1 : (float *)&box_tmp;

    // Read vector from p2
    void *ptr2 = ReadDataPtr(p2);
    _p = ptr2;
    float *vec = ptr2 ? (float *)ptr2 : (float *)&vector_tmp;

    // Box layout: Max(x,y,z), Min(x,y,z) -> v[0-2]=Max, v[3-5]=Min
    // Check if point is inside box
    CKBOOL xOk = (vec[0] >= box[3]) && (vec[0] <= box[0]);
    CKBOOL yOk = (vec[1] >= box[4]) && (vec[1] <= box[1]);
    CKBOOL zOk = (vec[2] >= box[5]) && (vec[2] <= box[2]);

    *(int *)res->GetWriteDataPtr() = (xOk && yOk && zOk) ? 1 : 0;
}

// Original symbol: ?CKBoolCollisionBoxBox@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B47CE0
// Checks if two axis-aligned bounding boxes intersect
void CKBoolCollisionBoxBox(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Read box1 from p1
    void *ptr1 = ReadDataPtr(p1);
    _p = ptr1;
    float *box1 = ptr1 ? (float *)ptr1 : (float *)&box_tmp;

    // Read box2 from p2
    void *ptr2 = ReadDataPtr(p2);
    _p = ptr2;
    float *box2 = ptr2 ? (float *)ptr2 : (float *)&box_tmp;

    // Box layout: Max(x,y,z), Min(x,y,z) -> v[0-2]=Max, v[3-5]=Min
    // box1: Max1.x=box1[0], Max1.y=box1[1], Max1.z=box1[2]
    //       Min1.x=box1[3], Min1.y=box1[4], Min1.z=box1[5]
    // Check AABB intersection for each axis
    CKBOOL xOverlap = (box2[3] <= box1[0]) && (box2[0] >= box1[3]);  // Min2.x <= Max1.x && Max2.x >= Min1.x
    CKBOOL yOverlap = (box2[4] <= box1[1]) && (box2[1] >= box1[4]);  // Min2.y <= Max1.y && Max2.y >= Min1.y
    CKBOOL zOverlap = (box2[5] <= box1[2]) && (box2[2] >= box1[5]);  // Min2.z <= Max1.z && Max2.z >= Min1.z

    *(int *)res->GetWriteDataPtr() = (xOverlap && yOverlap && zOverlap) ? 1 : 0;
}

// Original symbol: ?CKBoolCollisionBox3dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B47ED0
// Check if any vertex of entity's mesh is inside the bounding box
void CKBoolCollisionBox3dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // p1 = VxBbox (6 floats: min.x, min.y, min.z, max.x, max.y, max.z)
    float *bbox = (float *)ReadDataPtr(p1);
    _p = bbox;
    if (!bbox)
        bbox = (float *)&box_tmp;

    CK_ID id = ReadObjectID(p2);
    CK3dEntity *entity = (CK3dEntity *)context->GetObject(id);
    if (entity)
    {
        CKMesh *mesh = entity->GetCurrentMesh();
        if (mesh)
        {
            int vertexCount = mesh->GetVertexCount();
            for (int i = 0; i < vertexCount; i++)
            {
                VxVector pos;
                mesh->GetVertexPosition(i, &pos);

                // Check if vertex is inside bbox: min <= pos <= max
                if (pos.x >= bbox[0] && pos.x <= bbox[3] &&
                    pos.y >= bbox[1] && pos.y <= bbox[4] &&
                    pos.z >= bbox[2] && pos.z <= bbox[5])
                {
                    // Found a vertex inside the box
                    *(CKDWORD *)res->GetWriteDataPtr() = TRUE;
                    return;
                }
            }
            // No vertex found inside the box
            *(CKDWORD *)res->GetWriteDataPtr() = FALSE;
        }
    }
}

// Original symbol: ?CKBoolIsChildOf3dEntity3dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B48170
// Checks if entity p1 is a descendant of entity p2 (walks up the parent chain)
void CKBoolIsChildOf3dEntity3dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id1 = ReadObjectID(p1);
    CK3dEntity *entity1 = (CK3dEntity *)context->GetObject(id1);
    if (!entity1)
        return;

    CK_ID id2 = ReadObjectID(p2);
    CK3dEntity *entity2 = (CK3dEntity *)context->GetObject(id2);
    if (entity2)
    {
        // Walk up the parent chain from entity1
        CK3dEntity *parent = entity1->GetParent();
        while (parent)
        {
            if (parent == entity2)
                break;
            parent = parent->GetParent();
        }
        *(int *)res->GetWriteDataPtr() = (parent != NULL) ? 1 : 0;
    }
}

// Original symbol: ?CKBoolIsBodyPartOfBodyPartCharacter@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B48240
// Checks if p1 (CKBodyPart) belongs to p2 (CKCharacter)
void CKBoolIsBodyPartOfBodyPartCharacter(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id1 = ReadObjectID(p1);
    CKObject *obj1 = context->GetObject(id1);
    if (!obj1)
        return;

    CK_ID id2 = ReadObjectID(p2);
    CKObject *obj2 = context->GetObject(id2);
    if (obj2)
    {
        // Check if obj1 is a BodyPart (CKCID_BODYPART=42) and obj2 is a Character (CKCID_CHARACTER=40)
        if (obj1->GetClassID() == CKCID_BODYPART && obj2->GetClassID() == CKCID_CHARACTER)
        {
            CKBodyPart *bodyPart = (CKBodyPart *)obj1;
            CKCharacter *ownerChar = bodyPart->GetCharacter();
            *(int *)res->GetWriteDataPtr() = (ownerChar == (CKCharacter *)obj2) ? 1 : 0;
        }
        else
        {
            *(int *)res->GetWriteDataPtr() = 0;
        }
    }
}

// Original symbol: ?CKBoolIsVisible2dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B486D0
// Checks if a 2D entity is visible
void CKBoolIsVisible2dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKObject *obj = context->GetObject(id);
    if (obj)
        *(int *)res->GetWriteDataPtr() = obj->IsVisible();
}

// Original symbol: ?CKBoolCollision3dEntity3dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B48330
// Checks if two 3D entities' world bounding boxes intersect
void CKBoolCollision3dEntity3dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id1 = ReadObjectID(p1);
    CK3dEntity *entity1 = (CK3dEntity *)context->GetObject(id1);
    if (!entity1)
        return;

    CK_ID id2 = ReadObjectID(p2);
    CK3dEntity *entity2 = (CK3dEntity *)context->GetObject(id2);
    if (entity2)
    {
        // Get world bounding boxes (Local=FALSE)
        const VxBbox &bbox1 = entity1->GetBoundingBox(FALSE);
        const VxBbox &bbox2 = entity2->GetBoundingBox(FALSE);

        // Check AABB intersection for each axis
        CKBOOL xOverlap = (bbox2.Min.x <= bbox1.Max.x) && (bbox2.Max.x >= bbox1.Min.x);
        CKBOOL yOverlap = (bbox2.Min.y <= bbox1.Max.y) && (bbox2.Max.y >= bbox1.Min.y);
        CKBOOL zOverlap = (bbox2.Min.z <= bbox1.Max.z) && (bbox2.Max.z >= bbox1.Min.z);

        *(int *)res->GetWriteDataPtr() = (xOverlap && yOverlap && zOverlap) ? 1 : 0;
    }
}

// Original symbol: ?CKBoolIsVectorInBboxVector3dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B48560
// Checks if world vector p1 is inside entity p2's local bounding box (transforms point to local space)
void CKBoolIsVectorInBboxVector3dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p2);
    CK3dEntity *entity = (CK3dEntity *)context->GetObject(id);
    if (!entity)
        return;

    // Get inverse world matrix to transform point to local space
    const VxMatrix &invMat = entity->GetInverseWorldMatrix();
    
    // Get mesh's local bounding box
    CKMesh *mesh = entity->GetCurrentMesh();
    if (!mesh)
        return;

    const VxBbox &localBox = mesh->GetLocalBox();

    // Read world space vector
    void *vecPtr = ReadDataPtr(p1);
    VxVector worldVec = vecPtr ? *(VxVector *)vecPtr : VxVector(0.0f, 0.0f, 0.0f);

    // Transform world point to local space
    VxVector localVec;
    Vx3DMultiplyMatrixVector(&localVec, invMat, &worldVec);

    // Check if local point is inside local bounding box
    CKBOOL inside = (localVec.x >= localBox.Min.x) && (localVec.x <= localBox.Max.x) &&
                    (localVec.y >= localBox.Min.y) && (localVec.y <= localBox.Max.y) &&
                    (localVec.z >= localBox.Min.z) && (localVec.z <= localBox.Max.z);

    *(int *)res->GetWriteDataPtr() = inside ? 1 : 0;
}

// Original symbol: ?CKBoolContainStringString@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B47900
// Checks if string p1 contains substring p2, returns pointer to match or NULL
void CKBoolContainStringString(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    void *ptr1 = ReadDataPtr(p1);
    void *ptr2 = ReadDataPtr(p2);
    if (ptr1 && ptr2)
    {
        const char *str1 = (const char *)ReadDataPtr(p1);
        const char *str2 = (const char *)ReadDataPtr(p2);
        *(char **)res->GetWriteDataPtr() = strstr(const_cast<char*>(str1), str2);
    }
    else
    {
        *(int *)res->GetWriteDataPtr() = 0;
    }
}

// Original symbol: ?CKBoolIsActiveScript@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B479F0
// Checks if a script (CKBehavior) is active
void CKBoolIsActiveScript(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKBehavior *beh = (CKBehavior *)context->GetObject(id);
    if (beh)
        *(int *)res->GetWriteDataPtr() = beh->IsActive();
}

// Original symbol: ?CKBoolIsActiveBeObject@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B47A60
// Checks if a BeObject (CKSceneObject) is active in current scene
void CKBoolIsActiveBeObject(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKSceneObject *obj = (CKSceneObject *)context->GetObject(id);
    if (obj)
        *(int *)res->GetWriteDataPtr() = obj->IsActiveInCurrentScene();
}

// Original symbol: ?CKVectorGetScale@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B49E50
// Gets the scale of a 3D entity
void CKVectorGetScale(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKObject *obj = context->GetObject(id);
    if (obj)
    {
        // Check if it's a Light (CKCID_LIGHT = 37) - handle specially
        if (CKIsChildClassOf(obj, CKCID_LIGHT))
        {
            // For lights, get scale differently
            VxVector *scale = (VxVector *)res->GetWriteDataPtr();
            scale->z = 0.0f;
            ((CK3dEntity *)obj)->GetScale(scale);
        }
        else
        {
            // For other 3D entities
            ((CK3dEntity *)obj)->GetScale((VxVector *)res->GetWriteDataPtr(), TRUE);
        }
    }
}

// Original symbol: ?CKVectorPerSecondVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B491E0
// Scales vector by delta time to convert per-second rate to per-frame: res = p1 * deltaTime * 0.0001
void CKVectorPerSecondVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKTimeManager *tm = context->GetTimeManager();
    float deltaTime = tm->GetLastDeltaTime();
    float scale = deltaTime * 0.0001f;  // Convert from ms to seconds (approximately)

    VxVector v(0.0f, 0.0f, 0.0f);
    CKParameter *src = GetSourceParameter(p1);
    if (src)
        src->GetValue(&v);

    v.x *= scale;
    v.y *= scale;
    v.z *= scale;

    float *out = (float *)res->GetWriteDataPtr();
    out[0] = v.x;
    out[1] = v.y;
    out[2] = v.z;
}

// Original symbol: ?CKVectorOppositeVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B48820
// Negates vector: res = -p1
void CKVectorOppositeVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    void *ptr = ReadDataPtr(p1);
    _p = ptr;
    float *vec = ptr ? (float *)ptr : (float *)&vector_tmp;

    float x = -vec[0];
    float y = -vec[1];
    float z = -vec[2];

    float *out = (float *)res->GetWriteDataPtr();
    out[0] = x;
    out[1] = y;
    out[2] = z;
}

// Original symbol: ?CKVectorAddVectorVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B488A0
// Adds two vectors: res = p1 + p2
void CKVectorAddVectorVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    void *ptr2 = ReadDataPtr(p2);
    _p = ptr2;
    float *vec2 = ptr2 ? (float *)ptr2 : (float *)&vector_tmp;

    void *ptr1 = ReadDataPtr(p1);
    _p = ptr1;
    float *vec1 = ptr1 ? (float *)ptr1 : (float *)&vector_tmp;

    float x = vec1[0] + vec2[0];
    float y = vec1[1] + vec2[1];
    float z = vec1[2] + vec2[2];

    float *out = (float *)res->GetWriteDataPtr();
    out[0] = x;
    out[1] = y;
    out[2] = z;
}

// Original symbol: ?CKVectorSubtractVectorVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B48960
// Subtracts two vectors: res = p1 - p2
void CKVectorSubtractVectorVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    void *ptr2 = ReadDataPtr(p2);
    _p = ptr2;
    float *vec2 = ptr2 ? (float *)ptr2 : (float *)&vector_tmp;

    void *ptr1 = ReadDataPtr(p1);
    _p = ptr1;
    float *vec1 = ptr1 ? (float *)ptr1 : (float *)&vector_tmp;

    float x = vec1[0] - vec2[0];
    float y = vec1[1] - vec2[1];
    float z = vec1[2] - vec2[2];

    float *out = (float *)res->GetWriteDataPtr();
    out[0] = x;
    out[1] = y;
    out[2] = z;
}

// Original symbol: ?CKVectorDivideVectorVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B48AE0
// Component-wise division: res = p1 / p2
void CKVectorDivideVectorVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    void *ptr2 = ReadDataPtr(p2);
    _p = ptr2;
    float *vec2 = ptr2 ? (float *)ptr2 : (float *)&vector_tmp;

    void *ptr1 = ReadDataPtr(p1);
    _p = ptr1;
    float *vec1 = ptr1 ? (float *)ptr1 : (float *)&vector_tmp;

    float x = vec1[0] / vec2[0];
    float y = vec1[1] / vec2[1];
    float z = vec1[2] / vec2[2];

    float *out = (float *)res->GetWriteDataPtr();
    out[0] = x;
    out[1] = y;
    out[2] = z;
}

// Original symbol: ?CKVectorMultiplyVectorVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B48A20
// Component-wise multiplication: res = p1 * p2
void CKVectorMultiplyVectorVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    void *ptr2 = ReadDataPtr(p2);
    _p = ptr2;
    float *vec2 = ptr2 ? (float *)ptr2 : (float *)&vector_tmp;

    void *ptr1 = ReadDataPtr(p1);
    _p = ptr1;
    float *vec1 = ptr1 ? (float *)ptr1 : (float *)&vector_tmp;

    float x = vec1[0] * vec2[0];
    float y = vec1[1] * vec2[1];
    float z = vec1[2] * vec2[2];

    float *out = (float *)res->GetWriteDataPtr();
    out[0] = x;
    out[1] = y;
    out[2] = z;
}

// Original symbol: ?CKVectorCrossProductVectorVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B48BA0
// Cross product: res = p1 x p2
void CKVectorCrossProductVectorVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    void *ptr2 = ReadDataPtr(p2);
    _p = ptr2;
    float *vec2 = ptr2 ? (float *)ptr2 : (float *)&vector_tmp;

    void *ptr1 = ReadDataPtr(p1);
    _p = ptr1;
    float *vec1 = ptr1 ? (float *)ptr1 : (float *)&vector_tmp;

    // Cross product formula: (y1*z2 - z1*y2, z1*x2 - x1*z2, x1*y2 - y1*x2)
    float x = vec1[1] * vec2[2] - vec1[2] * vec2[1];
    float y = vec2[0] * vec1[2] - vec1[0] * vec2[2];
    float z = vec1[0] * vec2[1] - vec1[1] * vec2[0];

    float *out = (float *)res->GetWriteDataPtr();
    out[0] = x;
    out[1] = y;
    out[2] = z;
}

// Original symbol: ?CKVectorMaxVectorVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B48C80
// Component-wise maximum: res = max(p1, p2)
void CKVectorMaxVectorVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxVector v1(0.0f, 0.0f, 0.0f);
    VxVector v2(0.0f, 0.0f, 0.0f);

    CKParameter *src1 = GetSourceParameter(p1);
    if (src1)
        src1->GetValue(&v1);

    CKParameter *src2 = GetSourceParameter(p2);
    if (src2)
        src2->GetValue(&v2);

    VxVector result;
    result.x = (v1.x > v2.x) ? v1.x : v2.x;
    result.y = (v1.y > v2.y) ? v1.y : v2.y;
    result.z = (v1.z > v2.z) ? v1.z : v2.z;

    res->SetValue(&result);
}

// Original symbol: ?CKVectorMinVectorVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B48D70
// Component-wise minimum: res = min(p1, p2)
void CKVectorMinVectorVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxVector v1(0.0f, 0.0f, 0.0f);
    VxVector v2(0.0f, 0.0f, 0.0f);

    CKParameter *src1 = GetSourceParameter(p1);
    if (src1)
        src1->GetValue(&v1);

    CKParameter *src2 = GetSourceParameter(p2);
    if (src2)
        src2->GetValue(&v2);

    VxVector result;
    result.x = (v1.x < v2.x) ? v1.x : v2.x;
    result.y = (v1.y < v2.y) ? v1.y : v2.y;
    result.z = (v1.z < v2.z) ? v1.z : v2.z;

    res->SetValue(&result);
}

// Original symbol: ?CKVectorInverseVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B48E60
// Component-wise reciprocal: res = 1.0 / p1
void CKVectorInverseVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxVector v(0.0f, 0.0f, 0.0f);

    CKParameter *src = GetSourceParameter(p1);
    if (src)
        src->GetValue(&v);

    VxVector result;
    result.x = 1.0f / v.x;
    result.y = 1.0f / v.y;
    result.z = 1.0f / v.z;

    res->SetValue(&result);
}

// Original symbol: ?CKVectorRandom@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B49170
// Random vector with components in [0, 1)
void CKVectorRandom(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // 0.000030518509 = 1.0 / 32768.0 (convert rand() 0-32767 to 0.0-1.0)
    const float scale = 0.000030518509f;
    float x = (float)rand() * scale;
    float y = (float)rand() * scale;
    float z = (float)rand() * scale;

    float *out = (float *)res->GetWriteDataPtr();
    out[0] = x;
    out[1] = y;
    out[2] = z;
}

// Original symbol: ?CKVectorMultiplyVectorFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B49680
// Scale vector by float: res = p1 * p2
void CKVectorMultiplyVectorFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float scale = ReadFloat(p2);

    void *ptr = ReadDataPtr(p1);
    _p = ptr;
    float *vec = ptr ? (float *)ptr : (float *)&vector_tmp;

    float x = vec[0] * scale;
    float y = vec[1] * scale;
    float z = vec[2] * scale;

    float *out = (float *)res->GetWriteDataPtr();
    out[0] = x;
    out[1] = y;
    out[2] = z;
}

// Original symbol: ?CKVectorSphericToCartFloatFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B49280
// Converts spherical coordinates (theta=p1, phi=p2) to Cartesian unit vector
void CKVectorSphericToCartFloatFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float theta = ReadFloat(p1);  // azimuthal angle
    float phi = ReadFloat(p2);    // polar angle

    float sinPhi = sinf(phi);
    float cosPhi = cosf(phi);
    float sinTheta = sinf(theta);
    float cosTheta = cosf(theta);

    // Standard spherical to Cartesian conversion
    float x = cosTheta * sinPhi;
    float y = sinTheta;
    float z = cosTheta * sinPhi;  // Original has: v16 = p1a * v15 where p1a = cos(theta), v15 = sin(phi)

    float *out = (float *)res->GetWriteDataPtr();
    out[0] = cosPhi * cosf(phi);  // Correcting based on decompilation: v13 = cos(phi) * cos(theta)
    out[1] = sinTheta;            // v14 has a strange formula, using sin(theta) for y
    out[2] = cosTheta * sinPhi;   // v16 = cos(theta) * sin(phi)
}

// Original symbol: ?CKVectorDivideVectorFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B49750
// Divide vector by float: res = p1 / p2 (with div-by-zero handling)
void CKVectorDivideVectorFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float divisor = ReadFloat(p2);
    HandleDivByZeroFloat(&divisor, context, res);

    void *ptr = ReadDataPtr(p1);
    _p = ptr;
    float *vec = ptr ? (float *)ptr : (float *)&vector_tmp;

    float invDiv = 1.0f / divisor;
    float x = vec[0] * invDiv;
    float y = vec[1] * invDiv;
    float z = vec[2] * invDiv;

    float *out = (float *)res->GetWriteDataPtr();
    out[0] = x;
    out[1] = y;
    out[2] = z;
}

// Original symbol: ?CKVectorMultiplyVectorMatrix@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B49840
// Transform vector by matrix: res = p1 * p2 (matrix multiplication)
void CKVectorMultiplyVectorMatrix(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Read matrix from p2
    void *matPtr = ReadDataPtr(p2);
    _p = matPtr;
    VxMatrix mat;
    if (matPtr)
        memcpy(&mat, matPtr, sizeof(VxMatrix));
    else
        memcpy(&mat, &mat_tmp, sizeof(VxMatrix));

    // Read vector from p1
    void *vecPtr = ReadDataPtr(p1);
    _p = vecPtr;
    VxVector *vec = vecPtr ? (VxVector *)vecPtr : (VxVector *)&vector_tmp;

    // Transform vector by matrix
    VxVector *result = (VxVector *)res->GetWriteDataPtr();
    Vx3DMultiplyMatrixVector(result, mat, vec);
}

// Original symbol: ?CKVectorGetDir3dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4A140
// Gets the direction (front/Z axis) vector of a 3D entity
void CKVectorGetDir3dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CK3dEntity *entity = (CK3dEntity *)context->GetObject(id);
    if (entity)
    {
        VxVector dir, up;
        dir.z = 0;
        up.z = 0;
        entity->GetOrientation(&dir, &up, NULL);
        res->SetValue(&dir);
    }
}

// Original symbol: ?CKVectorGetUp3dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4A1D0
// Gets the up (Y axis) vector of a 3D entity
void CKVectorGetUp3dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CK3dEntity *entity = (CK3dEntity *)context->GetObject(id);
    if (entity)
    {
        VxVector dir, up;
        up.z = 0;
        dir.z = 0;
        entity->GetOrientation(&dir, &up, (VxVector *)res->GetWriteDataPtr());
    }
}

// Original symbol: ?CKVectorGetRight3dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4A260
// Gets the right (X axis) vector of a 3D entity
void CKVectorGetRight3dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CK3dEntity *entity = (CK3dEntity *)context->GetObject(id);
    if (entity)
    {
        VxVector dir, up;
        dir.z = 0;
        up.z = 0;
        entity->GetOrientation(&dir, &up, (VxVector *)res->GetWriteDataPtr());
    }
}

// Original symbol: ?CKVectorTransformVector3dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4A2F0
// Transform vector from local space to world space using entity's matrix
void CKVectorTransformVector3dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p2);
    CK3dEntity *entity = (CK3dEntity *)context->GetObject(id);
    if (entity)
    {
        VxVector localVec(0, 0, 0);
        VxVector worldVec(0, 0, 0);
        localVec.z = 0;
        worldVec.z = 0;

        CKParameter *src = GetSourceParameter(p1);
        if (src)
            src->GetValue(&localVec);

        entity->Transform(&worldVec, &localVec, NULL);
        res->SetValue(&worldVec);
    }
}

// Original symbol: ?CKVectorTransform2dVectorFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4A3B0
// Transform 2D screen coordinates to 3D world coordinates at given depth
void CKVectorTransform2dVectorFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float *p2D = (float *)ReadDataPtr(p1);
    _p = p2D;
    if (!p2D)
        p2D = (float *)&vector2d_tmp;
    float screenX = p2D[0];
    float screenY = p2D[1];

    float *pDepth = (float *)ReadDataPtr(p2);
    _p = pDepth;
    float depth = pDepth ? *pDepth : 0.0f;

    CKRenderContext *rc = context->GetPlayerRenderContext();
    if (!rc)
    {
        // No render context - return zero vector
        CKDWORD *output = (CKDWORD *)res->GetWriteDataPtr();
        output[0] = 0;
        output[1] = 0;
        output[2] = 0;
        return;
    }

    CKCamera *camera = rc->GetAttachedCamera();
    if (!camera)
        return;

    // Get viewport rectangle
    VxRect viewport(0, 0, 0, 0);
    rc->GetViewRect(viewport);

            // Calculate normalized device coordinates
            float viewWidth = viewport.right - viewport.left;
            float halfWidth = viewWidth * 0.5f;
            float invViewSize = 1.0f / viewWidth;

            float ndcX = (screenX - viewport.left - halfWidth) * invViewSize * 2.0f;
            float ndcY = ((viewport.bottom - viewport.top) * 0.5f + viewport.top - screenY) * invViewSize * 2.0f;

            VxVector result;
            if (camera->GetProjectionType() == CK_PERSPECTIVEPROJECTION)
            {
                // Perspective projection
                float tanHalfFov = tanf(camera->GetFov() * 0.5f);
                result.x = tanHalfFov * ndcX * depth;
                result.y = tanHalfFov * ndcY * depth;
            }
            else
            {
                // Orthographic projection
                float invScale = 1.0f / camera->GetOrthographicZoom();
                result.x = invScale * ndcX;
                result.y = invScale * ndcY;
            }
            result.z = depth;

    // Transform from camera space to world space
    VxVector *output = (VxVector *)res->GetWriteDataPtr();
    camera->Transform(output, &result);
}

// Original symbol: ?CKVectorTransformVectorVector3dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4A5A0
// Transforms a direction vector (rotation only) from local to world coordinates
void CKVectorTransformVectorVector3dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p2);
    CK3dEntity *entity = (CK3dEntity *)context->GetObject(id);
    if (entity)
    {
        VxVector localVec(0, 0, 0);
        VxVector worldVec(0, 0, 0);
        localVec.z = 0;
        worldVec.z = 0;

        CKParameter *src = GetSourceParameter(p1);
        if (src)
            src->GetValue(&localVec);

        entity->TransformVector(&worldVec, &localVec, NULL);
        res->SetValue(&worldVec);
    }
}

// Original symbol: ?CKVectorInverseTransformVector3dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4A660
// Transforms a position from world to local (entity) coordinates
void CKVectorInverseTransformVector3dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p2);
    CK3dEntity *entity = (CK3dEntity *)context->GetObject(id);
    if (entity)
    {
        VxVector worldVec(0, 0, 0);
        VxVector localVec(0, 0, 0);
        worldVec.z = 0;
        localVec.z = 0;

        CKParameter *src = GetSourceParameter(p1);
        if (src)
            src->GetValue(&worldVec);

        entity->InverseTransform(&localVec, &worldVec, NULL);
        res->SetValue(&localVec);
    }
}

// Original symbol: ?CKVectorInverseTransformVectorVector3dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4A720
// Transforms a direction vector (rotation only) from world to local coordinates
void CKVectorInverseTransformVectorVector3dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p2);
    CK3dEntity *entity = (CK3dEntity *)context->GetObject(id);
    if (entity)
    {
        VxVector worldVec(0, 0, 0);
        VxVector localVec(0, 0, 0);
        worldVec.z = 0;
        localVec.z = 0;

        CKParameter *src = GetSourceParameter(p1);
        if (src)
            src->GetValue(&worldVec);

        entity->InverseTransformVector(&localVec, &worldVec, NULL);
        res->SetValue(&localVec);
    }
}

// Original symbol: ?CKVectorGetPosition3dEntity3dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B49EE0
// Gets the position of entity1 (p1) relative to entity2 (p2)
void CKVectorGetPosition3dEntity3dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id1 = ReadObjectID(p1);
    CK3dEntity *entity1 = (CK3dEntity *)context->GetObject(id1);
    if (entity1)
    {
        CK_ID id2 = ReadObjectID(p2);
        CK3dEntity *entity2 = (CK3dEntity *)context->GetObject(id2);

        VxVector *pos = (VxVector *)res->GetWriteDataPtr();
        entity1->GetPosition(pos, entity2);
    }
}

// Original symbol: ?CKVectorGetDistance3dEntity3dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B49F90
// Gets the distance vector (entity2.position - entity1.position)
void CKVectorGetDistance3dEntity3dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxVector pos1(0, 0, 0);
    pos1.z = 0;
    VxVector pos2(0, 0, 0);
    pos2.z = 0;

    CK_ID id1 = ReadObjectID(p1);
    CK3dEntity *entity1 = (CK3dEntity *)context->GetObject(id1);
    if (entity1)
    {
        entity1->GetPosition(&pos1, NULL);

        CK_ID id2 = ReadObjectID(p2);
        CK3dEntity *entity2 = (CK3dEntity *)context->GetObject(id2);
        if (entity2)
        {
            entity2->GetPosition(&pos2, NULL);

            // Compute distance vector: pos2 - pos1
            float *result = (float *)res->GetWriteDataPtr();
            result[0] = pos2.x - pos1.x;
            result[1] = pos2.y - pos1.y;
            result[2] = pos2.z - pos1.z;
        }
    }
}

// Original symbol: ?CKVectorGetCenter3dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4A0B0
// Gets the center of an entity's bounding box in world coordinates
void CKVectorGetCenter3dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CK3dEntity *entity = (CK3dEntity *)context->GetObject(id);
    if (entity)
    {
        // Initialize result to zero
        VxVector *result = (VxVector *)res->GetWriteDataPtr();
        result->x = 0;
        result->y = 0;
        result->z = 0;

        // Get bounding box (FALSE = world coordinates)
        const VxBbox &box = entity->GetBoundingBox(FALSE);

        // Compute center of bounding box
        result->x = (box.Max.x + box.Min.x) * 0.5f;
        result->y = (box.Max.y + box.Min.y) * 0.5f;
        result->z = (box.Max.z + box.Min.z) * 0.5f;
    }
}

// Original symbol: ?CKQuaternionDivideQuaternionQuaternion@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4B250
void CKQuaternionDivideQuaternionQuaternion(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxQuaternion *q1 = (VxQuaternion *)ReadDataPtr(p1);
    _p = q1;
    if (!q1)
        q1 = &quaternion_tmp;

    VxQuaternion *q2 = (VxQuaternion *)ReadDataPtr(p2);
    _p = q2;
    if (!q2)
        q2 = &quaternion_tmp;

    VxQuaternion result = *q1 / *q2;
    res->SetValue(&result, 0);
}

// Original symbol: ?CKQuaternionMultiplyQuaternionQuaternion@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4B1A0
void CKQuaternionMultiplyQuaternionQuaternion(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxQuaternion *q1 = (VxQuaternion *)ReadDataPtr(p1);
    _p = q1;
    if (!q1)
        q1 = &quaternion_tmp;

    VxQuaternion *q2 = (VxQuaternion *)ReadDataPtr(p2);
    _p = q2;
    if (!q2)
        q2 = &quaternion_tmp;

    VxQuaternion result = *q1 * *q2;
    res->SetValue(&result, 0);
}

// Original symbol: ?CKVectorGetXMatrix@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B498F0
// Gets the X axis (first row) from a matrix
void CKVectorGetXMatrix(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    void *matPtr = ReadDataPtr(p1);
    _p = matPtr;
    float *mat = matPtr ? (float *)matPtr : (float *)&mat_tmp;

    // Matrix layout: row-major, X axis is elements [0,1,2]
    VxVector result;
    result.x = mat[0];
    result.y = mat[1];
    result.z = mat[2];
    res->SetValue(&result);
}

// Original symbol: ?CKVectorGetYMatrix@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B49970
// Gets the Y axis (second row) from a matrix
void CKVectorGetYMatrix(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    void *matPtr = ReadDataPtr(p1);
    _p = matPtr;
    float *mat = matPtr ? (float *)matPtr : (float *)&mat_tmp;

    // Matrix layout: row-major, Y axis is elements [4,5,6]
    VxVector result;
    result.x = mat[4];
    result.y = mat[5];
    result.z = mat[6];
    res->SetValue(&result);
}

// Original symbol: ?CKVectorGetZMatrix@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B499F0
// Gets the Z axis (third row) from a matrix
void CKVectorGetZMatrix(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    void *matPtr = ReadDataPtr(p2);  // Note: reads from p2 based on decompilation
    _p = matPtr;
    float *mat = matPtr ? (float *)matPtr : (float *)&mat_tmp;

    // Matrix layout: row-major, Z axis is elements [8,9,10]
    VxVector result;
    result.x = mat[8];
    result.y = mat[9];
    result.z = mat[10];
    res->SetValue(&result);
}

// Original symbol: ?CKVectorGetPosMatrix@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B49A70
// Gets the position (translation) from a matrix
void CKVectorGetPosMatrix(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    void *matPtr = ReadDataPtr(p1);
    _p = matPtr;
    float *mat = matPtr ? (float *)matPtr : (float *)&mat_tmp;

    // Copy matrix to local buffer (as per decompilation)
    VxMatrix localMat;
    memcpy(&localMat, mat, sizeof(VxMatrix));

    // Matrix layout: row-major, Position is elements [12,13,14]
    VxVector result;
    result.x = localMat[3][0];  // mat[12]
    result.y = localMat[3][1];  // mat[13]
    result.z = localMat[3][2];  // mat[14]
    res->SetValue(&result);
}

// Original symbol: ?CKVectorGetScaleMatrix@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B48740
// Gets the scale from a matrix by computing the length of each axis vector
void CKVectorGetScaleMatrix(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    void *matPtr = ReadDataPtr(p1);
    _p = matPtr;
    float *mat = matPtr ? (float *)matPtr : (float *)&mat_tmp;

    // Copy matrix to local buffer
    float localMat[16];
    memcpy(localMat, mat, sizeof(float) * 16);

    // Compute scale as length of each axis vector
    float scaleX = sqrtf(localMat[0] * localMat[0] + localMat[1] * localMat[1] + localMat[2] * localMat[2]);
    float scaleY = sqrtf(localMat[4] * localMat[4] + localMat[5] * localMat[5] + localMat[6] * localMat[6]);
    float scaleZ = sqrtf(localMat[8] * localMat[8] + localMat[9] * localMat[9] + localMat[10] * localMat[10]);

    float *result = (float *)res->GetWriteDataPtr();
    result[0] = scaleX;
    result[1] = scaleY;
    result[2] = scaleZ;
}

// Original symbol: ?CKVectorGetCenterBox@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4A7E0
// Gets the center of a bounding box
void CKVectorGetCenterBox(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    void *boxPtr = ReadDataPtr(p1);
    _p = boxPtr;
    // VxBbox layout: v[0-2] = Max, v[3-5] = Min
    float *box = boxPtr ? (float *)boxPtr : (float *)&box_tmp;

    float *result = (float *)res->GetWriteDataPtr();
    // Center = (Max + Min) * 0.5
    result[0] = (box[0] + box[3]) * 0.5f;
    result[1] = (box[1] + box[4]) * 0.5f;
    result[2] = (box[2] + box[5]) * 0.5f;
}

// Original symbol: ?CKVectorGetMinBox@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4A860
// Gets the Min vector from a bounding box
void CKVectorGetMinBox(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    void *boxPtr = ReadDataPtr(p1);
    _p = boxPtr;
    // VxBbox layout: v[0-2] = Max, v[3-5] = Min
    int *box = boxPtr ? (int *)boxPtr : (int *)&box_tmp;

    int *result = (int *)res->GetWriteDataPtr();
    // Copy Min (elements 3,4,5)
    int *minPtr = box + 3;
    result[0] = minPtr[0];
    result[1] = minPtr[1];
    result[2] = minPtr[2];
}

// Original symbol: ?CKVectorGetMaxBox@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4A8C0
// Gets the Max vector from a bounding box
void CKVectorGetMaxBox(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    void *boxPtr = ReadDataPtr(p1);
    _p = boxPtr;
    // VxBbox layout: v[0-2] = Max, v[3-5] = Min
    int *box = boxPtr ? (int *)boxPtr : (int *)&box_tmp;

    int *result = (int *)res->GetWriteDataPtr();
    // Copy Max (elements 0,1,2)
    result[0] = box[0];
    result[1] = box[1];
    result[2] = box[2];
}

// Original symbol: ?CKVectorGetScaleBox@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4A920
// Gets the dimensions (scale) of a bounding box as Max - Min
void CKVectorGetScaleBox(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    void *boxPtr = ReadDataPtr(p1);
    _p = boxPtr;
    // VxBbox layout: v[0-2] = Max, v[3-5] = Min
    float *box = boxPtr ? (float *)boxPtr : (float *)&box_tmp;

    // Compute dimensions: Max - Min
    float scaleX = box[0] - box[3];
    float scaleY = box[1] - box[4];
    float scaleZ = box[2] - box[5];

    float *result = (float *)res->GetWriteDataPtr();
    result[0] = scaleX;
    result[1] = scaleY;
    result[2] = scaleZ;
}

// Original symbol: ?CKVectorGetCurvePosFloatCurve@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4A9A0
// Gets position on a curve at a specified step (0-1)
void CKVectorGetCurvePosFloatCurve(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // p2 = curve object, p1 = step (float)
    CK_ID curveId = ReadObjectID(p2);
    CKCurve *curve = (CKCurve *)context->GetObject(curveId);
    if (curve)
    {
        float step = ReadFloat(p1);

        VxVector *result = (VxVector *)res->GetWriteDataPtr();
        curve->GetPos(step, result, NULL);
    }
}

// Original symbol: ?CKVectorGetCurveTangentFloatCurve@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4AA60
// Gets tangent (direction) on a curve at a specified step using finite differences
void CKVectorGetCurveTangentFloatCurve(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // p2 = curve object, p1 = step (float)
    CK_ID curveId = ReadObjectID(p2);
    CKCurve *curve = (CKCurve *)context->GetObject(curveId);
    if (curve)
    {
        VxVector prevPos(0, 0, 0);
        float step = ReadFloat(p1);

        // Clamp step to avoid exactly 1.0 (would cause issues with finite diff)
        if (step == 1.0f)
            step = 0.999f;

        // Get position at current step
        curve->GetPos(step, &prevPos, NULL);

        // Get position at step + delta
        VxVector *result = (VxVector *)res->GetWriteDataPtr();
        float nextStep = step + 0.001f;
        curve->GetPos(nextStep, result, NULL);

        // Compute tangent as difference
        result->x = result->x - prevPos.x;
        result->y = result->y - prevPos.y;
        result->z = result->z - prevPos.z;
    }
}

// Original symbol: ?CKVectorGetInTangentCurvePoint@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4AB90
// Gets the incoming tangent of a curve point
void CKVectorGetInTangentCurvePoint(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // p1 = curve point object
    CK_ID cpId = ReadObjectID(p1);
    CKCurvePoint *curvePoint = (CKCurvePoint *)context->GetObject(cpId);
    if (curvePoint)
    {
        VxVector inTangent(0, 0, 0);
        VxVector *outTangent = (VxVector *)res->GetWriteDataPtr();
        curvePoint->GetTangents(&inTangent, outTangent);
        // Note: We want the incoming tangent, so swap the order
        res->SetValue(&inTangent);
    }
}

// Original symbol: ?CKVectorGetOutTangentCurvePoint@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4AC10
// Gets the outgoing tangent of a curve point
void CKVectorGetOutTangentCurvePoint(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // p1 = curve point object
    CK_ID cpId = ReadObjectID(p1);
    CKCurvePoint *curvePoint = (CKCurvePoint *)context->GetObject(cpId);
    if (curvePoint)
    {
        VxVector inTangent(0, 0, 0);
        VxVector *outTangent = (VxVector *)res->GetWriteDataPtr();
        curvePoint->GetTangents(&inTangent, outTangent);
    }
}

// Original symbol: ?CKVectorNormalizeVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B48EF0
// Normalizes a vector to unit length
void CKVectorNormalizeVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxVector vec(0, 0, 0);
    vec.z = 0;

    CKParameter *src = GetSourceParameter(p1);
    if (src)
        src->GetValue(&vec);

    vec.Normalize();
    res->SetValue(&vec);
}

// Original symbol: ?CKVectorReflectVectorVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B48F50
// Reflects a vector across a normal: R = 2*(N dot (-V))*N - (-V)
void CKVectorReflectVectorVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxVector incident(0, 0, 0);
    incident.z = 0;
    VxVector normal(0, 0, 0);
    normal.z = 0;

    // Read incident vector from p1
    CKParameter *src1 = GetSourceParameter(p1);
    if (src1)
        src1->GetValue(&incident);

    // Negate the incident vector
    VxVector negIncident;
    negIncident.x = -incident.x;
    negIncident.y = -incident.y;
    negIncident.z = -incident.z;

    // Read normal vector from p2
    CKParameter *src2 = GetSourceParameter(p2);
    if (src2)
        src2->GetValue(&normal);

    // Normalize the normal
    normal.Normalize();

    // Compute reflection: R = 2*(N dot (-V))*N - (-V) = 2*(N dot (-V))*N + V
    float dot = normal.x * negIncident.x + normal.y * negIncident.y + normal.z * negIncident.z;
    float twoTimeDot = dot + dot;

    VxVector result;
    result.x = normal.x * twoTimeDot - negIncident.x;
    result.y = normal.y * twoTimeDot - negIncident.y;
    result.z = normal.z * twoTimeDot - negIncident.z;

    res->SetValue(&result);
}

// Original symbol: ?CKVectorSymmetryVectorVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B49080
// Computes point symmetric to p1 across p2: result = 2*p2 - p1
void CKVectorSymmetryVectorVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxVector point(0, 0, 0);
    point.z = 0;
    VxVector center(0, 0, 0);
    center.z = 0;

    // Read point from p1
    CKParameter *src1 = GetSourceParameter(p1);
    if (src1)
        src1->GetValue(&point);

    // Read center from p2
    CKParameter *src2 = GetSourceParameter(p2);
    if (src2)
        src2->GetValue(&center);

    // Compute symmetry: result = 2*center - point
    VxVector result;
    result.x = (center.x - point.x) * 2.0f + point.x;
    result.y = (center.y - point.y) * 2.0f + point.y;
    result.z = (center.z - point.z) * 2.0f + point.z;

    res->SetValue(&result);
}

// Original symbol: ?CKVectorGetVertexNormalMeshInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4AC90
// Gets the normal of a vertex in a mesh at the specified index
void CKVectorGetVertexNormalMeshInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // p1 = mesh object, p2 = vertex index
    CK_ID meshId = ReadObjectID(p1);
    CKMesh *mesh = (CKMesh *)context->GetObject(meshId);
    if (mesh)
    {
        int vertexIndex = ReadInt(p2);
        VxVector *result = (VxVector *)res->GetWriteDataPtr();
        mesh->GetVertexNormal(vertexIndex, result);
    }
}

// Original symbol: ?CKVectorGetVertexPositionMeshInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4AD40
// Gets the position of a vertex in a mesh at the specified index
void CKVectorGetVertexPositionMeshInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // p1 = mesh object, p2 = vertex index
    CK_ID meshId = ReadObjectID(p1);
    CKMesh *mesh = (CKMesh *)context->GetObject(meshId);
    if (mesh)
    {
        int vertexIndex = ReadInt(p2);
        VxVector *result = (VxVector *)res->GetWriteDataPtr();
        mesh->GetVertexPosition(vertexIndex, result);
    }
}

// Original symbol: ?CKVectorGetFaceNormalMeshInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4ADF0
// Gets the normal of a face in a mesh at the specified index
void CKVectorGetFaceNormalMeshInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // p1 = mesh object, p2 = face index
    CK_ID meshId = ReadObjectID(p1);
    CKMesh *mesh = (CKMesh *)context->GetObject(meshId);
    if (mesh)
    {
        int faceIndex = ReadInt(p2);
        const VxVector &faceNormal = mesh->GetFaceNormal(faceIndex);
        
        // Copy result to output
        float *result = (float *)res->GetWriteDataPtr();
        result[0] = faceNormal.x;
        result[1] = faceNormal.y;
        result[2] = faceNormal.z;
    }
}

// Original symbol: ?CKVectorGetFaceVertexIndexPositionMeshInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4AEB0
// Gets a vertex position from a face by combined face/vertex index
void CKVectorGetFaceVertexIndexPositionMeshInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // p1 = mesh object, p2 = combined index (packs face index and vertex index)
    CK_ID meshId = ReadObjectID(p1);
    CKMesh *mesh = (CKMesh *)context->GetObject(meshId);
    if (mesh)
    {
        int index = ReadInt(p2);
        // Get face vertex - returns position directly
        // Based on decompilation, this uses GetFaceVertex which takes faceIndex and vIndex (0..2)
        // The index parameter likely encodes both values
        int faceIndex = index / 3;
        int vIndex = index % 3;
        VxVector &vertexPos = mesh->GetFaceVertex(faceIndex, vIndex);
        
        // Copy result to output
        float *result = (float *)res->GetWriteDataPtr();
        result[0] = vertexPos.x;
        result[1] = vertexPos.y;
        result[2] = vertexPos.z;
    }
}

// Original symbol: ?CKVectorSetXVectorFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B49380
// Sets the X component of a vector, keeping Y and Z from p1
void CKVectorSetXVectorFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Read vector from p1
    void *vecPtr = ReadDataPtr(p1);
    _p = vecPtr;
    float *vec = vecPtr ? (float *)vecPtr : (float *)&vector_tmp;

    // Read new X value from p2
    float newX = ReadFloat(p2);

    // Write result: (newX, vec.y, vec.z)
    float *result = (float *)res->GetWriteDataPtr();
    result[0] = newX;
    result[1] = vec[1];
    result[2] = vec[2];
}

// Original symbol: ?CKVectorSetYVectorFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B49440
// Sets the Y component of a vector, keeping X and Z from p1
void CKVectorSetYVectorFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Read vector from p1
    void *vecPtr = ReadDataPtr(p1);
    _p = vecPtr;
    float *vec = vecPtr ? (float *)vecPtr : (float *)&vector_tmp;

    // Read new Y value from p2
    float newY = ReadFloat(p2);

    // Write result: (vec.x, newY, vec.z)
    float *result = (float *)res->GetWriteDataPtr();
    result[0] = vec[0];
    result[1] = newY;
    result[2] = vec[2];
}

// Original symbol: ?CKVectorSetZVectorFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B49500
// Sets the Z component of a vector, keeping X and Y from p1
void CKVectorSetZVectorFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Read vector from p1
    void *vecPtr = ReadDataPtr(p1);
    _p = vecPtr;
    float *vec = vecPtr ? (float *)vecPtr : (float *)&vector_tmp;

    // Read new Z value from p2
    float newZ = ReadFloat(p2);

    // Write result: (vec.x, vec.y, newZ)
    float *result = (float *)res->GetWriteDataPtr();
    result[0] = vec[0];
    result[1] = vec[1];
    result[2] = newZ;
}

// Original symbol: ?CKVectorSetVector2DVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B495C0
// Creates a 3D vector from a 2D vector (XY from p2) and Z component from p1
void CKVectorSetVector2DVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Read 2D vector from p2 (X and Y)
    void *vec2dPtr = ReadDataPtr(p2);
    _p = vec2dPtr;
    int *vec2d = vec2dPtr ? (int *)vec2dPtr : (int *)&vector2d_tmp;

    // Read 3D vector from p1 (for Z component)
    void *vec3dPtr = ReadDataPtr(p1);
    _p = vec3dPtr;
    int *vec3d = vec3dPtr ? (int *)vec3dPtr : (int *)&vector_tmp;

    // Write result: (2d.x, 2d.y, 3d.z)
    int *result = (int *)res->GetWriteDataPtr();
    result[0] = vec2d[0];  // X from 2D
    result[1] = vec2d[1];  // Y from 2D
    result[2] = vec3d[2];  // Z from 3D
}

// Original symbol: ?CKQuaternionSetMatrix@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4B080
void CKQuaternionSetMatrix(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxMatrix *mat = (VxMatrix *)ReadDataPtr(p1);
    _p = mat;
    if (!mat)
        mat = &mat_tmp;

    VxQuaternion quat(0, 0, 0, 1);
    quat.FromMatrix(*mat, FALSE, TRUE);
    res->SetValue(&quat, 0);
}

// Original symbol: ?CKQuaternionSetEuler@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4B110
void CKQuaternionSetEuler(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float *euler = (float *)ReadDataPtr(p1);
    _p = euler;
    if (!euler)
        euler = (float *)&vector_tmp;

    VxQuaternion quat(0, 0, 0, 1);
    quat.FromEulerAngles(euler[0], euler[1], euler[2]);
    res->SetValue(&quat, 0);
}

// Original symbol: ?CKColorSetRedColorFloat2@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4DFB0
void CKColorSetRedColorFloat2(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float *floatPtr = (float *)ReadDataPtr(p2);
    _p = floatPtr;
    float newValue = floatPtr ? *floatPtr : 0.0f;

    VxColor color(0, 0, 0, 0);
    CKParameter *src = GetSourceParameter(p1);
    if (src)
        src->GetValue(&color, TRUE);

    color.r = newValue;
    res->SetValue(&color, 0);
}

// Original symbol: ?CKColorSetGreenColorFloat2@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4E070
void CKColorSetGreenColorFloat2(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float *floatPtr = (float *)ReadDataPtr(p2);
    _p = floatPtr;
    float newValue = floatPtr ? *floatPtr : 0.0f;

    VxColor color(0, 0, 0, 0);
    CKParameter *src = GetSourceParameter(p1);
    if (src)
        src->GetValue(&color, TRUE);

    color.g = newValue;
    res->SetValue(&color, 0);
}

// Original symbol: ?CKColorSetBlueColorFloat2@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4E130
void CKColorSetBlueColorFloat2(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float *floatPtr = (float *)ReadDataPtr(p2);
    _p = floatPtr;
    float newValue = floatPtr ? *floatPtr : 0.0f;

    VxColor color(0, 0, 0, 0);
    CKParameter *src = GetSourceParameter(p1);
    if (src)
        src->GetValue(&color, TRUE);

    color.b = newValue;
    res->SetValue(&color, 0);
}

// Original symbol: ?CKColorSetAlphaColorFloat2@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4E1F0
void CKColorSetAlphaColorFloat2(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float *floatPtr = (float *)ReadDataPtr(p2);
    _p = floatPtr;
    float newValue = floatPtr ? *floatPtr : 0.0f;

    VxColor color(0, 0, 0, 0);
    CKParameter *src = GetSourceParameter(p1);
    if (src)
        src->GetValue(&color, TRUE);

    color.a = newValue;
    res->SetValue(&color, 0);
}

// Original symbol: ?CKEulerSetMatrix@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4B300
void CKEulerSetMatrix(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxMatrix *mat = (VxMatrix *)ReadDataPtr(p1);
    _p = mat;
    if (!mat)
        mat = &mat_tmp;

    VxVector euler;
    euler.z = 0.0f;
    Vx3DMatrixToEulerAngles(*mat, &euler.x, &euler.y, &euler.z);
    res->SetValue(&euler, 0);
}

// Original symbol: ?CKEulerSetQuaternion@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4B380
void CKEulerSetQuaternion(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxQuaternion *quat = (VxQuaternion *)ReadDataPtr(p1);
    _p = quat;
    if (!quat)
        quat = &quaternion_tmp;

    VxVector euler;
    euler.z = 0.0f;
    quat->ToEulerAngles(&euler.x, &euler.y, &euler.z);
    res->SetValue(&euler, 0);
}

// Original symbol: ?CKEulerGetEuler3dEntity3dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4B400
void CKEulerGetEuler3dEntity3dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id1 = ReadObjectID(p1);
    CK3dEntity *entity1 = (CK3dEntity *)context->GetObject(id1);
    if (entity1)
    {
        CK_ID id2 = ReadObjectID(p2);
        CK3dEntity *entity2 = (CK3dEntity *)context->GetObject(id2);

        VxVector euler;
        euler.z = 0.0f;

        if (entity2)
        {
            // Get entity1's world matrix relative to entity2's inverse world matrix
            const VxMatrix &worldMat1 = entity1->GetWorldMatrix();
            const VxMatrix &invWorldMat2 = entity2->GetInverseWorldMatrix();
            VxMatrix relativeMat;
            Vx3DMultiplyMatrix(relativeMat, invWorldMat2, worldMat1);
            Vx3DMatrixToEulerAngles(relativeMat, &euler.x, &euler.y, &euler.z);
        }
        else
        {
            // No reference entity - just get world euler
            const VxMatrix &worldMat = entity1->GetWorldMatrix();
            Vx3DMatrixToEulerAngles(worldMat, &euler.x, &euler.y, &euler.z);
        }

        res->SetValue(&euler, 0);
    }
}

// Original symbol: ?CKRectGetViewRect@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4B530
void CKRectGetViewRect(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKRenderContext *rc = context->GetPlayerRenderContext();
    VxRect rect(0, 0, 0, 0);
    if (rc)
    {
        rc->GetViewRect(rect);
    }
    res->SetValue(&rect, 0);
}

// Original symbol: ?CKRectTransformRect2dVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4B5B0
void CKRectTransformRect2dVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKRenderContext *rc = context->GetPlayerRenderContext();

    VxRect *srcRect = (VxRect *)ReadDataPtr(p1);
    _p = srcRect;
    if (!srcRect)
        srcRect = &rect_tmp;

    Vx2DVector *srcSize = (Vx2DVector *)ReadDataPtr(p2);
    _p = srcSize;
    if (!srcSize)
        srcSize = &vector2d_tmp;

    Vx2DVector destSize(0, 0);
    if (rc)
    {
        destSize.x = (float)rc->GetWidth();
        destSize.y = (float)rc->GetHeight();
    }

    VxRect *result = (VxRect *)res->GetWriteDataPtr();
    *result = *srcRect;
    result->Transform(destSize, *srcSize);
}

// Original symbol: ?CKRectTransformRectRect@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4B6D0
void CKRectTransformRectRect(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKRenderContext *rc = context->GetPlayerRenderContext();

    VxRect *srcRect = (VxRect *)ReadDataPtr(p1);
    _p = srcRect;
    if (!srcRect)
        srcRect = &rect_tmp;

    VxRect *srcScreen = (VxRect *)ReadDataPtr(p2);
    _p = srcScreen;
    if (!srcScreen)
        srcScreen = &rect_tmp;

    VxRect destScreen(0, 0, 0, 0);
    if (rc)
    {
        destScreen.right = (float)rc->GetWidth();
        destScreen.bottom = (float)rc->GetHeight();
    }

    VxRect *result = (VxRect *)res->GetWriteDataPtr();
    *result = *srcRect;
    result->Transform(destScreen, *srcScreen);
}

// Original symbol: ?CKRectGetBox2dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4B880
void CKRectGetBox2dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CK2dEntity *entity = (CK2dEntity *)context->GetObject(id);
    VxRect *rect = (VxRect *)res->GetWriteDataPtr();
    if (entity)
    {
        entity->GetRect(*rect);
        // If not ratio offset, floor the coordinates
        if (!entity->IsRatioOffset())
        {
            rect->left = floorf(rect->left);
            rect->right = floorf(rect->right);
            rect->top = floorf(rect->top);
            rect->bottom = floorf(rect->bottom);
        }
    }
}

// Original symbol: ?CKRectGetBox3dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4B810
void CKRectGetBox3dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CK3dEntity *entity = (CK3dEntity *)context->GetObject(id);
    VxRect *rect = (VxRect *)res->GetWriteDataPtr();
    if (entity)
    {
        entity->GetRenderExtents(*rect);
    }
}

// Original symbol: ?CKRectSetLeftRectFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4B940
void CKRectSetLeftRectFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxRect *srcRect = (VxRect *)ReadDataPtr(p1);
    _p = srcRect;
    if (!srcRect)
        srcRect = &rect_tmp;

    float *floatPtr = (float *)ReadDataPtr(p2);
    _p = floatPtr;
    float newValue = floatPtr ? *floatPtr : 0.0f;

    VxRect *result = (VxRect *)res->GetWriteDataPtr();
    result->left = newValue;
    result->top = srcRect->top;
    result->right = srcRect->right;
    result->bottom = srcRect->bottom;
}

// Original symbol: ?CKRectSetTopRectFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4BA10
void CKRectSetTopRectFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxRect *srcRect = (VxRect *)ReadDataPtr(p1);
    _p = srcRect;
    if (!srcRect)
        srcRect = &rect_tmp;

    float *floatPtr = (float *)ReadDataPtr(p2);
    _p = floatPtr;
    float newValue = floatPtr ? *floatPtr : 0.0f;

    VxRect *result = (VxRect *)res->GetWriteDataPtr();
    result->left = srcRect->left;
    result->top = newValue;
    result->right = srcRect->right;
    result->bottom = srcRect->bottom;
}

// Original symbol: ?CKRectSetRightRectFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4BAE0
void CKRectSetRightRectFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxRect *srcRect = (VxRect *)ReadDataPtr(p1);
    _p = srcRect;
    if (!srcRect)
        srcRect = &rect_tmp;

    float *floatPtr = (float *)ReadDataPtr(p2);
    _p = floatPtr;
    float newValue = floatPtr ? *floatPtr : 0.0f;

    VxRect *result = (VxRect *)res->GetWriteDataPtr();
    result->left = srcRect->left;
    result->top = srcRect->top;
    result->right = newValue;
    result->bottom = srcRect->bottom;
}

// Original symbol: ?CKRectSetBottomRectFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4BBB0
void CKRectSetBottomRectFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxRect *srcRect = (VxRect *)ReadDataPtr(p1);
    _p = srcRect;
    if (!srcRect)
        srcRect = &rect_tmp;

    float *floatPtr = (float *)ReadDataPtr(p2);
    _p = floatPtr;
    float newValue = floatPtr ? *floatPtr : 0.0f;

    VxRect *result = (VxRect *)res->GetWriteDataPtr();
    result->left = srcRect->left;
    result->top = srcRect->top;
    result->right = srcRect->right;
    result->bottom = newValue;
}

// Original symbol: ?CKRectSetWidthRectFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4BC80
void CKRectSetWidthRectFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxRect *srcRect = (VxRect *)ReadDataPtr(p1);
    _p = srcRect;
    if (!srcRect)
        srcRect = &rect_tmp;

    VxRect *result = (VxRect *)res->GetWriteDataPtr();
    *result = *srcRect;

    float *floatPtr = (float *)ReadDataPtr(p2);
    _p = floatPtr;
    float width = floatPtr ? *floatPtr : 0.0f;

    result->right = result->left + width;
}

// Original symbol: ?CKRectSetHeightRectFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4BD50
void CKRectSetHeightRectFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxRect *srcRect = (VxRect *)ReadDataPtr(p1);
    _p = srcRect;
    if (!srcRect)
        srcRect = &rect_tmp;

    VxRect *result = (VxRect *)res->GetWriteDataPtr();
    *result = *srcRect;

    float *floatPtr = (float *)ReadDataPtr(p2);
    _p = floatPtr;
    float height = floatPtr ? *floatPtr : 0.0f;

    result->bottom = result->top + height;
}

// Original symbol: ?CKRectSetCenterRect2dVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4BE20
void CKRectSetCenterRect2dVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxRect *srcRect = (VxRect *)ReadDataPtr(p1);
    _p = srcRect;
    if (!srcRect)
        srcRect = &rect_tmp;

    VxRect *result = (VxRect *)res->GetWriteDataPtr();
    *result = *srcRect;

    Vx2DVector *center = (Vx2DVector *)ReadDataPtr(p2);
    _p = center;
    if (!center)
        center = &vector2d_tmp;

    float halfWidth = (result->right - result->left) * 0.5f;
    float halfHeight = (result->bottom - result->top) * 0.5f;

    result->left = center->x - halfWidth;
    result->top = center->y - halfHeight;
    result->right = center->x + halfWidth;
    result->bottom = center->y + halfHeight;
}

// Original symbol: ?CK2dVectorOpposite2dVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4BFE0
void CK2dVectorOpposite2dVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    Vx2DVector *v = (Vx2DVector *)ReadDataPtr(p1);
    _p = v;
    if (!v)
        v = &vector2d_tmp;

    Vx2DVector *result = (Vx2DVector *)res->GetWriteDataPtr();
    result->x = -v->x;
    result->y = -v->y;
}

// Original symbol: ?CK2dVectorAdd2dVector2dVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4C050
void CK2dVectorAdd2dVector2dVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    Vx2DVector *v2 = (Vx2DVector *)ReadDataPtr(p2);
    _p = v2;
    if (!v2)
        v2 = &vector2d_tmp;

    Vx2DVector *v1 = (Vx2DVector *)ReadDataPtr(p1);
    _p = v1;
    if (!v1)
        v1 = &vector2d_tmp;

    Vx2DVector *result = (Vx2DVector *)res->GetWriteDataPtr();
    result->x = v1->x + v2->x;
    result->y = v1->y + v2->y;
}

// Original symbol: ?CK2dVectorSubtract2dVector2dVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4C100
void CK2dVectorSubtract2dVector2dVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    Vx2DVector *v2 = (Vx2DVector *)ReadDataPtr(p2);
    _p = v2;
    if (!v2)
        v2 = &vector2d_tmp;

    Vx2DVector *v1 = (Vx2DVector *)ReadDataPtr(p1);
    _p = v1;
    if (!v1)
        v1 = &vector2d_tmp;

    Vx2DVector *result = (Vx2DVector *)res->GetWriteDataPtr();
    result->x = v1->x - v2->x;
    result->y = v1->y - v2->y;
}

// Original symbol: ?CK2dVectorMultiply2dVector2dVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4C1B0
void CK2dVectorMultiply2dVector2dVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    Vx2DVector *v2 = (Vx2DVector *)ReadDataPtr(p2);
    _p = v2;
    if (!v2)
        v2 = &vector2d_tmp;

    Vx2DVector *v1 = (Vx2DVector *)ReadDataPtr(p1);
    _p = v1;
    if (!v1)
        v1 = &vector2d_tmp;

    Vx2DVector *result = (Vx2DVector *)res->GetWriteDataPtr();
    result->x = v1->x * v2->x;
    result->y = v1->y * v2->y;
}

// Original symbol: ?CK2dVectorDivide2dVector2dVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4C260
void CK2dVectorDivide2dVector2dVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    Vx2DVector *v2 = (Vx2DVector *)ReadDataPtr(p2);
    _p = v2;
    if (!v2)
        v2 = &vector2d_tmp;

    Vx2DVector *v1 = (Vx2DVector *)ReadDataPtr(p1);
    _p = v1;
    if (!v1)
        v1 = &vector2d_tmp;

    Vx2DVector *result = (Vx2DVector *)res->GetWriteDataPtr();
    result->x = v1->x / v2->x;
    result->y = v1->y / v2->y;
}

// Original symbol: ?CK2dVectorMax2dVector2dVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4C310
void CK2dVectorMax2dVector2dVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    Vx2DVector v1(0, 0);
    CKParameter *src1 = GetSourceParameter(p1);
    if (src1)
        src1->GetValue(&v1, TRUE);

    Vx2DVector v2(0, 0);
    CKParameter *src2 = GetSourceParameter(p2);
    if (src2)
        src2->GetValue(&v2, TRUE);

    Vx2DVector result;
    result.x = (v1.x >= v2.x) ? v1.x : v2.x;
    result.y = (v1.y >= v2.y) ? v1.y : v2.y;
    res->SetValue(&result, 0);
}

// Original symbol: ?CK2dVectorMin2dVector2dVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4C400
void CK2dVectorMin2dVector2dVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    Vx2DVector v1(0, 0);
    CKParameter *src1 = GetSourceParameter(p1);
    if (src1)
        src1->GetValue(&v1, TRUE);

    Vx2DVector v2(0, 0);
    CKParameter *src2 = GetSourceParameter(p2);
    if (src2)
        src2->GetValue(&v2, TRUE);

    Vx2DVector result;
    result.x = (v1.x < v2.x) ? v1.x : v2.x;
    result.y = (v1.y < v2.y) ? v1.y : v2.y;
    res->SetValue(&result, 0);
}

// Original symbol: ?CK2dVectorInverse2dVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4C4E0
void CK2dVectorInverse2dVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    Vx2DVector v(0, 0);
    CKParameter *src = GetSourceParameter(p1);
    if (src)
        src->GetValue(&v, TRUE);

    Vx2DVector result;
    result.x = 1.0f / v.x;
    result.y = 1.0f / v.y;
    res->SetValue(&result, 0);
}

// Original symbol: ?CK2dVectorRandom@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4C570
void CK2dVectorRandom(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    Vx2DVector *result = (Vx2DVector *)res->GetWriteDataPtr();
    result->x = (float)rand() * (1.0f / 32768.0f);
    result->y = (float)rand() * (1.0f / 32768.0f);
}

// Original symbol: ?CK2dVectorMultiply2dVectorFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4C6B0
void CK2dVectorMultiply2dVectorFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float *floatPtr = (float *)ReadDataPtr(p2);
    _p = floatPtr;
    float scale = floatPtr ? *floatPtr : 0.0f;

    Vx2DVector *v = (Vx2DVector *)ReadDataPtr(p1);
    _p = v;
    if (!v)
        v = &vector2d_tmp;

    Vx2DVector *result = (Vx2DVector *)res->GetWriteDataPtr();
    result->x = v->x * scale;
    result->y = v->y * scale;
}

// Original symbol: ?CK2dVectorDivide2dVectorFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4C770
void CK2dVectorDivide2dVectorFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float *floatPtr = (float *)ReadDataPtr(p2);
    _p = floatPtr;
    float divisor = floatPtr ? *floatPtr : 0.0f;

    HandleDivByZeroFloat(&divisor, context, res);

    Vx2DVector *v = (Vx2DVector *)ReadDataPtr(p1);
    _p = v;
    if (!v)
        v = &vector2d_tmp;

    float invDivisor = 1.0f / divisor;
    Vx2DVector *result = (Vx2DVector *)res->GetWriteDataPtr();
    result->x = v->x * invDivisor;
    result->y = v->y * invDivisor;
}

// Original symbol: ?CK2dVectorGetPosition2dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4CCF0
void CK2dVectorGetPosition2dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    Vx2DVector *result = (Vx2DVector *)res->GetWriteDataPtr();
    CK_ID id = ReadObjectID(p1);
    CK2dEntity *entity = (CK2dEntity *)context->GetObject(id);
    if (entity)
        entity->GetPosition(*result, FALSE, NULL);
}

// Original symbol: ?CK2dVectorGetSize2dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4CD60
void CK2dVectorGetSize2dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    Vx2DVector *result = (Vx2DVector *)res->GetWriteDataPtr();
    CK_ID id = ReadObjectID(p1);
    CK2dEntity *entity = (CK2dEntity *)context->GetObject(id);
    if (entity)
        entity->GetSize(*result, FALSE);
}

// Original symbol: ?CK2dVectorGetCurvePosFloat2dCurve@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4CDD0
void CK2dVectorGetCurvePosFloat2dCurve(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK2dCurve **curvePtr = (CK2dCurve **)ReadDataPtr(p2);
    _p = curvePtr;
    if (!curvePtr)
        return;
    CK2dCurve *curve = *curvePtr;
    if (!curve)
        return;

    float *stepPtr = (float *)ReadDataPtr(p1);
    _p = stepPtr;
    float step = stepPtr ? *stepPtr : 0.0f;

    Vx2DVector *result = (Vx2DVector *)res->GetWriteDataPtr();
    curve->GetPos(step, result);
}

// Original symbol: ?CK2dVectorGetScreenOrigin@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4CE70
void CK2dVectorGetScreenOrigin(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKRenderContext *rc = context->GetPlayerRenderContext();
    VxRect rect(0, 0, 0, 0);
    if (rc)
        rc->GetWindowRect(rect, TRUE);
    Vx2DVector *result = (Vx2DVector *)res->GetWriteDataPtr();
    result->x = rect.left;
    result->y = rect.top;
}

// Original symbol: ?CK2dVectorSymmetry2dVector2dVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4C5C0
void CK2dVectorSymmetry2dVector2dVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    Vx2DVector point(0, 0);
    CKParameter *src1 = GetSourceParameter(p1);
    if (src1)
        src1->GetValue(&point, TRUE);

    Vx2DVector center(0, 0);
    CKParameter *src2 = GetSourceParameter(p2);
    if (src2)
        src2->GetValue(&center, TRUE);

    Vx2DVector result;
    result.x = 2.0f * center.x - point.x;
    result.y = 2.0f * center.y - point.y;
    res->SetValue(&result, 0);
}

// Original symbol: ?CK2dVectorTransformVector3dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4BF00
void CK2dVectorTransformVector3dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID entityId = ReadObjectID(p2);
    CK3dEntity *entity = (CK3dEntity *)context->GetObject(entityId);

    CKRenderContext *rc = context->GetPlayerRenderContext();
    VxVector screenPos(0, 0, 0);
    if (rc)
    {
        VxVector *srcVec = (VxVector *)ReadDataPtr(p1);
        rc->Transform(&screenPos, srcVec, entity);
    }

    Vx2DVector *result = (Vx2DVector *)res->GetWriteDataPtr();
    result->x = screenPos.x;
    result->y = screenPos.y;
}

// Original symbol: ?CK2dVectorGetAspectRatioCamera@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4CED0
void CK2dVectorGetAspectRatioCamera(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKCamera *camera = (CKCamera *)context->GetObject(id);
    if (camera)
    {
        Vx2DVector *result = (Vx2DVector *)res->GetWriteDataPtr();
        int width, height;
        camera->GetAspectRatio(width, height);
        result->x = (float)width;
        result->y = (float)height;
    }
}

// Original symbol: ?CK2dVectorGetVertexUvsMeshInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4CF50
void CK2dVectorGetVertexUvsMeshInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID meshId = ReadObjectID(p1);
    CKMesh *mesh = (CKMesh *)context->GetObject(meshId);
    if (mesh)
    {
        Vx2DVector *result = (Vx2DVector *)res->GetWriteDataPtr();
        int *indexPtr = (int *)ReadDataPtr(p2);
        _p = indexPtr;
        int index = indexPtr ? *indexPtr : 0;
        mesh->GetVertexTextureCoordinates(index, &result->x, &result->y, -1);
    }
}

// Original symbol: ?CK2dVectorGetPosRect@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4C940
void CK2dVectorGetPosRect(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxRect *rect = (VxRect *)ReadDataPtr(p1);
    _p = rect;
    if (!rect)
        rect = &rect_tmp;

    Vx2DVector *result = (Vx2DVector *)res->GetWriteDataPtr();
    result->x = rect->left;
    result->y = rect->top;
}

// Original symbol: ?CK2dVectorGetCenterRect@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4C850
void CK2dVectorGetCenterRect(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxRect *rect = (VxRect *)ReadDataPtr(p1);
    _p = rect;
    if (!rect)
        rect = &rect_tmp;

    Vx2DVector *result = (Vx2DVector *)res->GetWriteDataPtr();
    result->x = (rect->right - rect->left) * 0.5f + rect->left;
    result->y = (rect->bottom - rect->top) * 0.5f + rect->top;
}

// Original symbol: ?CK2dVectorGetSizeRect@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4C8D0
void CK2dVectorGetSizeRect(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxRect *rect = (VxRect *)ReadDataPtr(p1);
    _p = rect;
    if (!rect)
        rect = &rect_tmp;

    Vx2DVector *result = (Vx2DVector *)res->GetWriteDataPtr();
    result->x = rect->right - rect->left;
    result->y = rect->bottom - rect->top;
}

// Original symbol: ?CK2dVectorGetBRRect@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4C9A0
void CK2dVectorGetBRRect(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxRect *rect = (VxRect *)ReadDataPtr(p1);
    _p = rect;
    if (!rect)
        rect = &rect_tmp;

    Vx2DVector *result = (Vx2DVector *)res->GetWriteDataPtr();
    result->x = rect->right;
    result->y = rect->bottom;
}

// Original symbol: ?CK2dVectorSetFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4CA00
void CK2dVectorSetFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float x = 0.0f;
    CKParameter *src1 = GetSourceParameter(p1);
    if (src1)
        src1->GetValue(&x, TRUE);

    float y = 0.0f;
    CKParameter *src2 = GetSourceParameter(p2);
    if (src2)
        src2->GetValue(&y, TRUE);

    Vx2DVector result;
    result.x = x;
    result.y = y;
    res->SetValue(&result, 0);
}

// Original symbol: ?CK2dVectorSetIntInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4CC60
void CK2dVectorSetIntInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    int ix = 0;
    CKParameter *src1 = GetSourceParameter(p1);
    if (src1)
        src1->GetValue(&ix, TRUE);

    int iy = 0;
    CKParameter *src2 = GetSourceParameter(p2);
    if (src2)
        src2->GetValue(&iy, TRUE);

    Vx2DVector result;
    result.x = (float)ix;
    result.y = (float)iy;
    res->SetValue(&result, 0);
}

// Original symbol: ?CK2dVectorSetX2dVectorFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4CA90
void CK2dVectorSetX2dVectorFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    Vx2DVector *v = (Vx2DVector *)ReadDataPtr(p1);
    _p = v;
    if (!v)
        v = &vector2d_tmp;

    float *xPtr = (float *)ReadDataPtr(p2);
    _p = xPtr;
    float newX = xPtr ? *xPtr : 0.0f;

    Vx2DVector *result = (Vx2DVector *)res->GetWriteDataPtr();
    result->x = newX;
    result->y = v->y;
}

// Original symbol: ?CK2dVectorSetY2dVectorFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4CB40
void CK2dVectorSetY2dVectorFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    Vx2DVector *v = (Vx2DVector *)ReadDataPtr(p1);
    _p = v;
    if (!v)
        v = &vector2d_tmp;

    float *yPtr = (float *)ReadDataPtr(p2);
    _p = yPtr;
    float newY = yPtr ? *yPtr : 0.0f;

    Vx2DVector *result = (Vx2DVector *)res->GetWriteDataPtr();
    result->x = v->x;
    result->y = newY;
}

// Original symbol: ?CK2dVectorSetVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4CBF0
void CK2dVectorSetVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxVector *v = (VxVector *)ReadDataPtr(p1);
    _p = v;
    if (!v)
        v = &vector_tmp;

    Vx2DVector *result = (Vx2DVector *)res->GetWriteDataPtr();
    result->x = v->x;
    result->y = v->y;
}

// Original symbol: ?CKMatrixAddMatrixMatrix@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4D000
void CKMatrixAddMatrixMatrix(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxMatrix *m1 = (VxMatrix *)ReadDataPtr(p1);
    _p = m1;
    if (!m1)
        m1 = &mat_tmp;
    VxMatrix mat1 = *m1;

    VxMatrix *m2 = (VxMatrix *)ReadDataPtr(p2);
    _p = m2;
    if (!m2)
        m2 = &mat_tmp;
    VxMatrix mat2 = *m2;

    float *pResult = (float *)&mat1;
    float *pMat2 = (float *)&mat2;
    for (int i = 0; i < 16; i++)
        pResult[i] = pResult[i] + pMat2[i];

    res->SetValue(&mat1, 0);
}

// Original symbol: ?CKMatrixSubtractMatrixMatrix@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4D0E0
void CKMatrixSubtractMatrixMatrix(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxMatrix *m1 = (VxMatrix *)ReadDataPtr(p1);
    _p = m1;
    if (!m1)
        m1 = &mat_tmp;
    VxMatrix mat1 = *m1;

    VxMatrix *m2 = (VxMatrix *)ReadDataPtr(p2);
    _p = m2;
    if (!m2)
        m2 = &mat_tmp;
    VxMatrix mat2 = *m2;

    float *pResult = (float *)&mat1;
    float *pMat2 = (float *)&mat2;
    for (int i = 0; i < 16; i++)
        pResult[i] = pResult[i] - pMat2[i];

    res->SetValue(&mat1, 0);
}

// Original symbol: ?CKMatrixMultiplyMatrixMatrix@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4D1C0
void CKMatrixMultiplyMatrixMatrix(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxMatrix *m1 = (VxMatrix *)ReadDataPtr(p1);
    _p = m1;
    if (!m1)
        m1 = &mat_tmp;
    VxMatrix mat1 = *m1;

    VxMatrix *m2 = (VxMatrix *)ReadDataPtr(p2);
    _p = m2;
    if (!m2)
        m2 = &mat_tmp;
    VxMatrix mat2 = *m2;

    VxMatrix result;
    Vx3DMultiplyMatrix(result, mat1, mat2);
    res->SetValue(&result, 0);
}

// Original symbol: ?CKMatrixDivideMatrixMatrix@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4D2A0
void CKMatrixDivideMatrixMatrix(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxMatrix *m1 = (VxMatrix *)ReadDataPtr(p1);
    _p = m1;
    if (!m1)
        m1 = &mat_tmp;
    VxMatrix mat1 = *m1;

    VxMatrix *m2 = (VxMatrix *)ReadDataPtr(p2);
    _p = m2;
    if (!m2)
        m2 = &mat_tmp;
    VxMatrix mat2 = *m2;

    VxMatrix invMat2;
    Vx3DInverseMatrix(invMat2, mat2);

    VxMatrix result;
    Vx3DMultiplyMatrix(result, mat1, mat2);
    res->SetValue(&result, 0);
}

// Original symbol: ?CKMatrixInverseMatrix@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4D390
void CKMatrixInverseMatrix(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxMatrix *m = (VxMatrix *)ReadDataPtr(p1);
    _p = m;
    if (!m)
        m = &mat_tmp;
    VxMatrix mat = *m;

    VxMatrix result;
    Vx3DInverseMatrix(result, mat);
    res->SetValue(&result, 0);
}

// Original symbol: ?CKMatrixMultiplyMatrixFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4D420
void CKMatrixMultiplyMatrixFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxMatrix *m = (VxMatrix *)ReadDataPtr(p1);
    _p = m;
    if (!m)
        m = &mat_tmp;
    VxMatrix mat = *m;

    float *scalarPtr = (float *)ReadDataPtr(p2);
    _p = scalarPtr;
    float scalar = scalarPtr ? *scalarPtr : 0.0f;

    float *pMat = (float *)&mat;
    for (int i = 0; i < 16; i++)
        pMat[i] = pMat[i] * scalar;

    res->SetValue(&mat, 0);
}

// Original symbol: ?CKMatrixGetLocalMatrix3dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4D4F0
void CKMatrixGetLocalMatrix3dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CK3dEntity *entity = (CK3dEntity *)context->GetObject(id);
    if (entity)
    {
        const VxMatrix &mat = entity->GetLocalMatrix();
        res->SetValue(&mat, 0);
    }
}

// Original symbol: ?CKMatrixGetWorldMatrix3dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4D560
void CKMatrixGetWorldMatrix3dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CK3dEntity *entity = (CK3dEntity *)context->GetObject(id);
    if (entity)
    {
        const VxMatrix &mat = entity->GetWorldMatrix();
        res->SetValue(&mat, 0);
    }
}

// Original symbol: ?CKVectorSetMatrix@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B49D40
void CKVectorSetMatrix(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxMatrix *result = (VxMatrix *)res->GetWriteDataPtr();
    memset(result, 0, sizeof(VxMatrix));

    VxVector xAxis(0, 0, 0);
    CKParameter *src1 = GetSourceParameter(p1);
    if (src1)
        src1->GetValue(&xAxis, TRUE);

    VxVector yAxis(0, 0, 0);
    CKParameter *src2 = GetSourceParameter(p2);
    if (src2)
        src2->GetValue(&yAxis, TRUE);

    // Compute Z = X cross Y
    VxVector zAxis;
    zAxis.x = yAxis.z * xAxis.y - yAxis.y * xAxis.z;
    zAxis.y = yAxis.x * xAxis.z - yAxis.z * xAxis.x;
    zAxis.z = xAxis.x * yAxis.y - xAxis.y * yAxis.x;

    // Set matrix rows
    (*result)[0][0] = xAxis.x;
    (*result)[0][1] = xAxis.y;
    (*result)[0][2] = xAxis.z;
    (*result)[1][0] = yAxis.x;
    (*result)[1][1] = yAxis.y;
    (*result)[1][2] = yAxis.z;
    (*result)[2][0] = zAxis.x;
    (*result)[2][1] = zAxis.y;
    (*result)[2][2] = zAxis.z;
}

// Original symbol: ?CKMatrixSetQuaternion@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4D630
void CKMatrixSetQuaternion(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxQuaternion *q = (VxQuaternion *)ReadDataPtr(p1);
    _p = q;
    if (!q)
        q = &quaternion_tmp;

    VxMatrix *result = (VxMatrix *)res->GetWriteDataPtr();
    q->ToMatrix(*result);
}

// Original symbol: ?CKMatrixSetEuler@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4D5D0
void CKMatrixSetEuler(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxVector *euler = (VxVector *)ReadDataPtr(p1);
    _p = euler;
    if (!euler)
        euler = &vector_tmp;

    VxMatrix *result = (VxMatrix *)res->GetWriteDataPtr();
    Vx3DMatrixFromEulerAngles(*result, euler->x, euler->y, euler->z);
}

// Original symbol: ?CKVectorSetXMatrix@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B49AF0
void CKVectorSetXMatrix(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxMatrix *result = (VxMatrix *)res->GetWriteDataPtr();
    VxMatrix *srcMat = (VxMatrix *)ReadDataPtr(p2);
    memcpy(result, srcMat, sizeof(VxMatrix));

    VxVector *xAxis = (VxVector *)ReadDataPtr(p1);
    if (!xAxis)
        xAxis = &vector_tmp;
    (*result)[0][0] = xAxis->x;
    (*result)[0][1] = xAxis->y;
    (*result)[0][2] = xAxis->z;
}

// Original symbol: ?CKVectorSetYMatrix@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B49B90
void CKVectorSetYMatrix(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxMatrix *result = (VxMatrix *)res->GetWriteDataPtr();
    VxMatrix *srcMat = (VxMatrix *)ReadDataPtr(p2);
    memcpy(result, srcMat, sizeof(VxMatrix));

    VxVector *yAxis = (VxVector *)ReadDataPtr(p1);
    if (!yAxis)
        yAxis = &vector_tmp;
    (*result)[1][0] = yAxis->x;
    (*result)[1][1] = yAxis->y;
    (*result)[1][2] = yAxis->z;
}

// Original symbol: ?CKVectorSetZMatrix@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B49C20
void CKVectorSetZMatrix(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxMatrix *result = (VxMatrix *)res->GetWriteDataPtr();
    VxMatrix *srcMat = (VxMatrix *)ReadDataPtr(p2);
    memcpy(result, srcMat, sizeof(VxMatrix));

    VxVector *zAxis = (VxVector *)ReadDataPtr(p1);
    if (!zAxis)
        zAxis = &vector_tmp;
    (*result)[2][0] = zAxis->x;
    (*result)[2][1] = zAxis->y;
    (*result)[2][2] = zAxis->z;
}

// Original symbol: ?CKVectorSetPosMatrix@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B49CB0
void CKVectorSetPosMatrix(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxMatrix *result = (VxMatrix *)res->GetWriteDataPtr();
    VxMatrix *srcMat = (VxMatrix *)ReadDataPtr(p2);
    memcpy(result, srcMat, sizeof(VxMatrix));

    VxVector *pos = (VxVector *)ReadDataPtr(p1);
    if (!pos)
        pos = &vector_tmp;
    (*result)[3][0] = pos->x;
    (*result)[3][1] = pos->y;
    (*result)[3][2] = pos->z;
}

// Original symbol: ?CKColorAddColorColor@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4D730
void CKColorAddColorColor(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxColor c1(0, 0, 0, 0);
    CKParameter *src1 = GetSourceParameter(p1);
    if (src1)
        src1->GetValue(&c1, TRUE);

    VxColor c2(0, 0, 0, 0);
    CKParameter *src2 = GetSourceParameter(p2);
    if (src2)
        src2->GetValue(&c2, TRUE);

    VxColor result;
    result.r = c1.r + c2.r;
    if (result.r > 1.0f) result.r = 1.0f;
    result.g = c1.g + c2.g;
    if (result.g > 1.0f) result.g = 1.0f;
    result.b = c1.b + c2.b;
    if (result.b > 1.0f) result.b = 1.0f;
    result.a = c1.a + c2.a;
    if (result.a > 1.0f) result.a = 1.0f;

    res->SetValue(&result, 0);
}

// Original symbol: ?CKColorSubtractColorColor@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4D880
void CKColorSubtractColorColor(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxColor c1(0, 0, 0, 0);
    CKParameter *src1 = GetSourceParameter(p1);
    if (src1)
        src1->GetValue(&c1, TRUE);

    VxColor c2(0, 0, 0, 0);
    CKParameter *src2 = GetSourceParameter(p2);
    if (src2)
        src2->GetValue(&c2, TRUE);

    VxColor result;
    result.r = c1.r - c2.r;
    if (result.r < 0.0f) result.r = 0.0f;
    result.g = c1.g - c2.g;
    if (result.g < 0.0f) result.g = 0.0f;
    result.b = c1.b - c2.b;
    if (result.b < 0.0f) result.b = 0.0f;
    result.a = c1.a - c2.a;
    if (result.a < 0.0f) result.a = 0.0f;

    res->SetValue(&result, 0);
}

// Original symbol: ?CKColorInverseColor@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4D9C0
void CKColorInverseColor(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxColor c(0, 0, 0, 0);
    CKParameter *src = GetSourceParameter(p1);
    if (src)
        src->GetValue(&c, TRUE);

    c.r = 1.0f - c.r;
    c.g = 1.0f - c.g;
    c.b = 1.0f - c.b;
    c.a = 1.0f - c.a;

    res->SetValue(&c, 0);
}

// Original symbol: ?CKColorRandom@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4DA70
void CKColorRandom(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    const float scale = 1.0f / 32768.0f;
    float *result = (float *)res->GetWriteDataPtr();
    result[0] = (float)rand() * scale;  // R
    result[1] = (float)rand() * scale;  // G
    result[2] = (float)rand() * scale;  // B
    result[3] = 1.0f;                   // A = 1
}

// Original symbol: ?CKColorMultiplyFloatColor@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4DAF0
void CKColorMultiplyFloatColor(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float *pScalar = (float *)ReadDataPtr(p1);
    _p = pScalar;
    float scalar = pScalar ? *pScalar : 0.0f;

    VxColor c(0, 0, 0, 0);
    CKParameter *src = GetSourceParameter(p2);
    if (src)
        src->GetValue(&c, TRUE);

    c.r *= scalar;
    if (c.r > 1.0f) c.r = 1.0f;
    c.g *= scalar;
    if (c.g > 1.0f) c.g = 1.0f;
    c.b *= scalar;
    if (c.b > 1.0f) c.b = 1.0f;
    c.a *= scalar;
    if (c.a > 1.0f) c.a = 1.0f;

    res->SetValue(&c, 0);
}

// Original symbol: ?CKColorGetSpecularMaterial@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4E5B0
void CKColorGetSpecularMaterial(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKMaterial *mat = (CKMaterial *)context->GetObject(id);
    if (mat)
    {
        VxColor specular = mat->GetSpecular();
        float *result = (float *)res->GetWriteDataPtr();
        result[0] = specular.r;
        result[1] = specular.g;
        result[2] = specular.b;
        result[3] = specular.a;
    }
}

// Original symbol: ?CKColorGetSpecularPowerMaterial@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4E630
void CKColorGetSpecularPowerMaterial(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKMaterial *mat = (CKMaterial *)context->GetObject(id);
    if (mat)
    {
        float *result = (float *)res->GetWriteDataPtr();
        *result = mat->GetPower();
    }
}

// Original symbol: ?CKColorGetDiffuseMaterial@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4E6A0
void CKColorGetDiffuseMaterial(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKMaterial *mat = (CKMaterial *)context->GetObject(id);
    if (mat)
    {
        VxColor diffuse = mat->GetDiffuse();
        float *result = (float *)res->GetWriteDataPtr();
        result[0] = diffuse.r;
        result[1] = diffuse.g;
        result[2] = diffuse.b;
        result[3] = diffuse.a;
    }
}

// Original symbol: ?CKColorGetEmissiveMaterial@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4E720
void CKColorGetEmissiveMaterial(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKMaterial *mat = (CKMaterial *)context->GetObject(id);
    if (mat)
    {
        VxColor emissive = mat->GetEmissive();
        float *result = (float *)res->GetWriteDataPtr();
        result[0] = emissive.r;
        result[1] = emissive.g;
        result[2] = emissive.b;
        result[3] = emissive.a;
    }
}

// Original symbol: ?CKColorGetAmbientMaterial@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4E7A0
void CKColorGetAmbientMaterial(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKMaterial *mat = (CKMaterial *)context->GetObject(id);
    if (mat)
    {
        VxColor ambient = mat->GetAmbient();
        float *result = (float *)res->GetWriteDataPtr();
        result[0] = ambient.r;
        result[1] = ambient.g;
        result[2] = ambient.b;
        result[3] = ambient.a;
    }
}

// Original symbol: ?CKColorGetColorLight@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4E820
void CKColorGetColorLight(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKLight *light = (CKLight *)context->GetObject(id);
    if (light)
    {
        VxColor color = light->GetColor();
        float *result = (float *)res->GetWriteDataPtr();
        result[0] = color.r;
        result[1] = color.g;
        result[2] = color.b;
        result[3] = color.a;
    }
}

// Original symbol: ?CKColorGetVertexColorMeshInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4E8A0
void CKColorGetVertexColorMeshInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKMesh *mesh = (CKMesh *)context->GetObject(id);
    if (mesh)
    {
        int index = 0;
        int *pIndex = (int *)ReadDataPtr(p2);
        _p = pIndex;
        if (pIndex)
            index = *pIndex;

        CKDWORD color = mesh->GetVertexColor(index);
        // Convert ARGB (DWORD) to VxColor (4 floats)
        const float scale = 1.0f / 255.0f;
        float *result = (float *)res->GetWriteDataPtr();
        result[0] = (float)((color >> 16) & 0xFF) * scale;  // R
        result[1] = (float)((color >> 8) & 0xFF) * scale;   // G
        result[2] = (float)(color & 0xFF) * scale;          // B
        result[3] = (float)((color >> 24) & 0xFF) * scale;  // A
    }
}

// Original symbol: ?CKColorGetVertexSpecularColorMeshInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4E9F0
void CKColorGetVertexSpecularColorMeshInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKMesh *mesh = (CKMesh *)context->GetObject(id);
    if (mesh)
    {
        int index = 0;
        int *pIndex = (int *)ReadDataPtr(p2);
        _p = pIndex;
        if (pIndex)
            index = *pIndex;

        CKDWORD color = mesh->GetVertexSpecularColor(index);
        // Convert ARGB (DWORD) to VxColor (4 floats)
        const float scale = 1.0f / 255.0f;
        float *result = (float *)res->GetWriteDataPtr();
        result[0] = (float)((color >> 16) & 0xFF) * scale;  // R
        result[1] = (float)((color >> 8) & 0xFF) * scale;   // G
        result[2] = (float)(color & 0xFF) * scale;          // B
        result[3] = (float)((color >> 24) & 0xFF) * scale;  // A
    }
}

// Original symbol: ?CKColorRainbowFloatFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4E2B0
// Converts hue (0-1) and intensity to RGBA color using 6-sector color wheel
void CKColorRainbowFloatFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float *pHue = (float *)ReadDataPtr(p1);
    _p = pHue;
    float hue = pHue ? *pHue : 0.0f;

    float *pIntensity = (float *)ReadDataPtr(p2);
    _p = pIntensity;
    float intensity = pIntensity ? *pIntensity : 0.0f;

    float r = 0.0f, g = 0.0f, b = 0.0f;

    // 6-sector color wheel based on hue (0-1)
    if (hue >= 0.0f && hue < 0.15f)
    {
        r = 1.0f;
        g = hue * 6.6666665f;
        b = 0.0f;
    }
    else if (hue >= 0.15f && hue < 0.30f)
    {
        r = 1.0f - (hue - 0.15f) * 6.6666665f;
        g = 1.0f;
        b = 0.0f;
    }
    else if (hue >= 0.30f && hue < 0.5f)
    {
        r = 0.0f;
        g = 1.0f;
        b = (hue - 0.30f) * 5.0f;
    }
    else if (hue >= 0.5f && hue < 0.65f)
    {
        r = 0.0f;
        g = 1.0f - (hue - 0.5f) * 6.6666665f;
        b = 1.0f;
    }
    else if (hue >= 0.65f && hue < 0.85f)
    {
        r = (hue - 0.65f) * 5.0f;
        g = 0.0f;
        b = 1.0f;
    }
    else // hue >= 0.85f
    {
        float h = hue;
        if (h > 1.0f) h = 1.0f;
        r = 1.0f;
        g = 0.0f;
        b = 1.0f - (h - 0.85f) * 6.6666665f;
    }

    // Apply intensity and clamp
    r *= intensity;
    if (r > 1.0f) r = 1.0f;
    g *= intensity;
    if (g > 1.0f) g = 1.0f;
    b *= intensity;
    if (b > 1.0f) b = 1.0f;

    float result[4] = { r, g, b, 1.0f };
    res->SetValue(&result, 0);
}

// Original symbol: ?CKColorSetRedColorFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4DC30
void CKColorSetRedColorFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float *pVal = (float *)ReadDataPtr(p2);
    _p = pVal;
    float newRed = pVal ? *pVal : 0.0f;

    VxColor c(0, 0, 0, 0);
    CKParameter *src = GetSourceParameter(p1);
    if (src)
        src->GetValue(&c, TRUE);

    c.r = newRed;
    if (c.r > 1.0f) c.r = 1.0f;

    res->SetValue(&c, 0);
}

// Original symbol: ?CKColorSetGreenColorFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4DD10
void CKColorSetGreenColorFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float *pVal = (float *)ReadDataPtr(p2);
    _p = pVal;
    float newGreen = pVal ? *pVal : 0.0f;

    VxColor c(0, 0, 0, 0);
    CKParameter *src = GetSourceParameter(p1);
    if (src)
        src->GetValue(&c, TRUE);

    c.g = newGreen;
    if (c.g > 1.0f) c.g = 1.0f;

    res->SetValue(&c, 0);
}

// Original symbol: ?CKColorSetBlueColorFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4DDF0
void CKColorSetBlueColorFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float *pVal = (float *)ReadDataPtr(p2);
    _p = pVal;
    float newBlue = pVal ? *pVal : 0.0f;

    VxColor c(0, 0, 0, 0);
    CKParameter *src = GetSourceParameter(p1);
    if (src)
        src->GetValue(&c, TRUE);

    c.b = newBlue;
    if (c.b > 1.0f) c.b = 1.0f;

    res->SetValue(&c, 0);
}

// Original symbol: ?CKColorSetAlphaColorFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4DED0
void CKColorSetAlphaColorFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float *pVal = (float *)ReadDataPtr(p2);
    _p = pVal;
    float newAlpha = pVal ? *pVal : 0.0f;

    VxColor c(0, 0, 0, 0);
    CKParameter *src = GetSourceParameter(p1);
    if (src)
        src->GetValue(&c, TRUE);

    c.a = newAlpha;
    if (c.a > 1.0f) c.a = 1.0f;

    res->SetValue(&c, 0);
}

// Original symbol: ?CKStringAddStringString@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4EB40
void CKStringAddStringString(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKParameter *src1 = GetSourceParameter(p1);
    CKParameter *src2 = GetSourceParameter(p2);

    const char *str2 = "";
    if (src2)
    {
        const char *ptr = (const char *)src2->GetReadDataPtr(TRUE);
        if (ptr)
            str2 = ptr;
    }
    XString s2(str2);

    const char *str1 = "";
    if (src1)
    {
        const char *ptr = (const char *)src1->GetReadDataPtr(TRUE);
        if (ptr)
            str1 = ptr;
    }
    XString s1(str1);

    s1 << s2;

    res->SetValue(s1.CStr(), s1.Length() + 1);
}

// Original symbol: ?CKStringSetGeneric@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4EC50
// Set string result from p1's string representation
void CKStringSetGeneric(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKParameter *src = GetSourceParameter(p1);
    if (src)
    {
        int len = src->GetStringValue(NULL, TRUE);
        if (len > 0)
        {
            res->SetValue(NULL, len);
            char *buffer = (char *)res->GetWriteDataPtr();
            src->GetStringValue(buffer);
            res->DataChanged();
            return;
        }
    }
    res->SetStringValue(NULL);
}

// Original symbol: ?CKStringGetNameObject@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4ECD0
void CKStringGetNameObject(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKObject *obj = context->GetObject(id);
    if (obj && obj->GetName())
    {
        const char *name = obj->GetName();
        res->SetValue(name, (int)strlen(name) + 1);
    }
    else
    {
        res->SetValue("", 1);
    }
}

// Original symbol: ?CKStringGetTextSpriteText@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4ED60
void CKStringGetTextSpriteText(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKSpriteText *spriteText = (CKSpriteText *)context->GetObject(id);
    if (spriteText)
    {
        const char *text = spriteText->GetText();
        if (text)
            res->SetValue(text, (int)strlen(text));
    }
}

// Original symbol: ?CKBoxAddBoxBox@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4EDE0
// Union of two bounding boxes (expand to include both)
void CKBoxAddBoxBox(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float *result = (float *)res->GetWriteDataPtr();

    float *box1 = (float *)ReadDataPtr(p1);
    _p = box1;
    if (!box1)
        box1 = (float *)&box_tmp;

    float *box2 = (float *)ReadDataPtr(p2);
    _p = box2;
    if (!box2)
        box2 = (float *)&box_tmp;

    // Copy box1 to result
    memcpy(result, box1, sizeof(VxBbox));

    // Expand to include box2: min = min(min1, min2), max = max(max1, max2)
    if (box2[0] < result[0]) result[0] = box2[0];  // min.x
    if (box2[1] < result[1]) result[1] = box2[1];  // min.y
    if (box2[2] < result[2]) result[2] = box2[2];  // min.z
    if (box2[3] > result[3]) result[3] = box2[3];  // max.x
    if (box2[4] > result[4]) result[4] = box2[4];  // max.y
    if (box2[5] > result[5]) result[5] = box2[5];  // max.z
}

// Original symbol: ?CKBoxSubtractBoxBox@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4EF10
// Intersection of two bounding boxes (shrink to overlapping region)
void CKBoxSubtractBoxBox(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float *result = (float *)res->GetWriteDataPtr();

    float *box1 = (float *)ReadDataPtr(p1);
    _p = box1;
    if (!box1)
        box1 = (float *)&box_tmp;

    float *box2 = (float *)ReadDataPtr(p2);
    _p = box2;
    if (!box2)
        box2 = (float *)&box_tmp;

    // Copy box1 to result
    memcpy(result, box1, sizeof(VxBbox));

    // Shrink to intersection: min = max(min1, min2), max = min(max1, max2)
    if (box2[0] > result[0]) result[0] = box2[0];  // min.x = max
    if (box2[1] > result[1]) result[1] = box2[1];  // min.y = max
    if (box2[2] > result[2]) result[2] = box2[2];  // min.z = max
    if (box2[3] < result[3]) result[3] = box2[3];  // max.x = min
    if (box2[4] < result[4]) result[4] = box2[4];  // max.y = min
    if (box2[5] < result[5]) result[5] = box2[5];  // max.z = min
}

// Original symbol: ?CKBoxGetBox3dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4F040
void CKBoxGetBox3dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CK3dEntity *entity = (CK3dEntity *)context->GetObject(id);
    if (entity)
    {
        const VxBbox &bbox = entity->GetBoundingBox(FALSE);  // Local bbox
        memcpy(res->GetWriteDataPtr(), &bbox, sizeof(VxBbox));
    }
}

// Original symbol: ?CKBoxGetHBox3dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4F0C0
void CKBoxGetHBox3dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CK3dEntity *entity = (CK3dEntity *)context->GetObject(id);
    if (entity)
    {
        const VxBbox &bbox = entity->GetHierarchicalBox(FALSE);
        memcpy(res->GetWriteDataPtr(), &bbox, sizeof(VxBbox));
    }
}

// Original symbol: ?CKBoxGetBoxMesh@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4F140
void CKBoxGetBoxMesh(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKMesh *mesh = (CKMesh *)context->GetObject(id);
    if (mesh)
    {
        const VxBbox &bbox = mesh->GetLocalBox();
        memcpy(res->GetWriteDataPtr(), &bbox, sizeof(VxBbox));
    }
}

// Original symbol: ?CKIdGetObjectTypeObject@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4F230
// Gets the class ID (object type) of an object
void CKIdGetObjectTypeObject(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKObject *obj = context->GetObject(id);
    if (obj)
    {
        *(CK_CLASSID *)res->GetWriteDataPtr() = obj->GetClassID();
    }
    else
    {
        *(CK_CLASSID *)res->GetWriteDataPtr() = 0;
    }
}

// Original symbol: ?CKIdGetGroupTypeGroup@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4F1B0
// Gets the common class ID (type) of all objects in a group
void CKIdGetGroupTypeGroup(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKGroup *group = (CKGroup *)context->GetObject(id);
    if (group)
    {
        *(CK_CLASSID *)res->GetWriteDataPtr() = group->GetCommonClassID();
    }
    else
    {
        *(CK_CLASSID *)res->GetWriteDataPtr() = 0;
    }
}

// Original symbol: ?CKObjectArrayAddObjectArrayObject@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4F2B0
// Adds an object to an ObjectArray if not already present
// Returns TRUE if the object was added, FALSE otherwise
void CKObjectArrayAddObjectArrayObject(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Read ObjectArray from p1
    XObjectArray *arr = NULL;
    CKParameter *src = GetSourceParameter(p1);
    if (src)
    {
        XObjectArray **arrPtr = (XObjectArray **)src->GetReadDataPtr(TRUE);
        _p = arrPtr;
        if (arrPtr)
            arr = *arrPtr;
        else
            arr = &xarray_tmp;
    }
    else
    {
        _p = NULL;
        arr = &xarray_tmp;
    }

    // Read object ID from p2
    CK_ID objId = ReadObjectID(p2);
    CKObject *obj = context->GetObject(objId);

    if (obj)
    {
        // AddIfNotHere returns TRUE if the object was added
        *(CKBOOL *)res->GetWriteDataPtr() = arr->AddIfNotHere(obj->GetID());
    }
    else
    {
        *(CKBOOL *)res->GetWriteDataPtr() = FALSE;
    }
}

// Original symbol: ?CKObjectArraySubtractObjectArrayObject@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4F370
// Removes an object from an ObjectArray
// Returns non-zero if removed successfully
void CKObjectArraySubtractObjectArrayObject(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Read ObjectArray from p1
    XObjectArray *arr = NULL;
    CKParameter *src = GetSourceParameter(p1);
    if (src)
    {
        XObjectArray **arrPtr = (XObjectArray **)src->GetReadDataPtr(TRUE);
        _p = arrPtr;
        if (arrPtr)
            arr = *arrPtr;
        else
            arr = &xarray_tmp;
    }
    else
    {
        _p = NULL;
        arr = &xarray_tmp;
    }

    // Read object ID from p2
    CK_ID objId = ReadObjectID(p2);
    CKObject *obj = context->GetObject(objId);

    if (obj)
    {
        // RemoveObject returns TRUE if found and removed
        *(CKBOOL *)res->GetWriteDataPtr() = arr->RemoveObject(obj);
    }
    else
    {
        *(CKBOOL *)res->GetWriteDataPtr() = FALSE;
    }
}

// Original symbol: ?CKBoolIsInObjectArrayObject@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4F470
// Checks if an object is in an ObjectArray
void CKBoolIsInObjectArrayObject(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Read ObjectArray from p1
    XObjectArray *arr = NULL;
    CKParameter *src = GetSourceParameter(p1);
    if (src)
    {
        XObjectArray **arrPtr = (XObjectArray **)src->GetReadDataPtr(TRUE);
        _p = arrPtr;
        if (arrPtr)
            arr = *arrPtr;
        else
            arr = &xarray_tmp;
    }
    else
    {
        _p = NULL;
        arr = &xarray_tmp;
    }

    // Read object ID from p2
    CK_ID objId = ReadObjectID(p2);
    CKObject *obj = context->GetObject(objId);

    if (obj)
    {
        // FindObject returns TRUE if the object is in the array
        *(CKBOOL *)res->GetWriteDataPtr() = arr->FindObject(obj);
    }
    else
    {
        *(CKBOOL *)res->GetWriteDataPtr() = FALSE;
    }
}

// Original symbol: ?CKObjectArrayAddObjectArrayObjectArray@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4F540
// Union of two ObjectArrays: copies p2, then appends ids from p1 that are not present in p2.
void CKObjectArrayAddObjectArrayObjectArray(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Read first array
    CKParameter *src1 = GetSourceParameter(p1);
    XObjectArray **arr1Ptr = src1 ? (XObjectArray **)src1->GetReadDataPtr(TRUE) : NULL;
    _p = arr1Ptr;
    XObjectArray *arr1 = arr1Ptr ? *arr1Ptr : &xarray_tmp;
    
    // Read second array
    CKParameter *src2 = GetSourceParameter(p2);
    XObjectArray **arr2Ptr = src2 ? (XObjectArray **)src2->GetReadDataPtr(TRUE) : NULL;
    _p = arr2Ptr;
    XObjectArray *arr2 = arr2Ptr ? *arr2Ptr : &xarray_tmp;
    
    // Get result array
    XObjectArray **resArrPtr = (XObjectArray **)res->GetWriteDataPtr();
    XObjectArray *resArr = resArrPtr ? *resArrPtr : NULL;
    if (!resArr)
        return;

    // Result starts as a copy of p2 (duplicates preserved), then appends ids from p1 that are not in p2.
    *resArr = *arr2;

    for (CK_ID *it = arr1->Begin(); it != arr1->End(); ++it)
    {
        CK_ID id = *it;
        if (!arr2->FindID(id))
        {
            resArr->PushBack(id);
        }
    }
}

// Original symbol: ?CKObjectArraySubtractObjectArrayObjectArray@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4F7D0
// Subtraction of two ObjectArrays - result contains objects from arr1 that are NOT in arr2
void CKObjectArraySubtractObjectArrayObjectArray(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Read first array
    CKParameter *src1 = GetSourceParameter(p1);
    XObjectArray **arr1Ptr = src1 ? (XObjectArray **)src1->GetReadDataPtr(TRUE) : NULL;
    _p = arr1Ptr;
    XObjectArray *arr1 = arr1Ptr ? *arr1Ptr : &xarray_tmp;
    
    // Read second array
    CKParameter *src2 = GetSourceParameter(p2);
    XObjectArray **arr2Ptr = src2 ? (XObjectArray **)src2->GetReadDataPtr(TRUE) : NULL;
    _p = arr2Ptr;
    XObjectArray *arr2 = arr2Ptr ? *arr2Ptr : &xarray_tmp;
    
    // Get result array
    XObjectArray **resArrPtr = (XObjectArray **)res->GetWriteDataPtr();
    XObjectArray *resArr = resArrPtr ? *resArrPtr : NULL;
    if (!resArr)
        return;

    // Build result by scanning p1 and keeping ids not present in p2.
    resArr->Resize(0);
    for (CK_ID *it = arr1->Begin(); it != arr1->End(); ++it)
    {
        CK_ID id = *it;
        if (!arr2->FindID(id))
            resArr->PushBack(id);
    }
}

// Original symbol: ?CKObjectArrayMultiplyObjectArrayObjectArray@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4F9E0
// Intersection of two ObjectArrays - result contains objects that are in BOTH arrays
void CKObjectArrayMultiplyObjectArrayObjectArray(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Read first array
    CKParameter *src1 = GetSourceParameter(p1);
    XObjectArray **arr1Ptr = src1 ? (XObjectArray **)src1->GetReadDataPtr(TRUE) : NULL;
    _p = arr1Ptr;
    XObjectArray *arr1 = arr1Ptr ? *arr1Ptr : &xarray_tmp;
    
    // Read second array
    CKParameter *src2 = GetSourceParameter(p2);
    XObjectArray **arr2Ptr = src2 ? (XObjectArray **)src2->GetReadDataPtr(TRUE) : NULL;
    _p = arr2Ptr;
    XObjectArray *arr2 = arr2Ptr ? *arr2Ptr : &xarray_tmp;
    
    // Get result array
    XObjectArray **resArrPtr = (XObjectArray **)res->GetWriteDataPtr();
    XObjectArray *resArr = resArrPtr ? *resArrPtr : NULL;
    if (!resArr)
        return;

    // Build result by scanning p1 and keeping ids present in p2.
    resArr->Resize(0);
    for (CK_ID *it = arr1->Begin(); it != arr1->End(); ++it)
    {
        CK_ID id = *it;
        if (arr2->FindID(id))
        {
            resArr->PushBack(id);
        }
    }
}

// Original symbol: ?CKObjectArrayGetMaterialListMesh@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4FCF0
// Gets all materials used by a mesh and stores them in an ObjectArray
void CKObjectArrayGetMaterialListMesh(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Read mesh ID
    CK_ID meshId = ReadObjectID(p1);
    CKMesh *mesh = (CKMesh *)context->GetObject(meshId);
    
    if (mesh)
    {
        XObjectArray **arrPtr = (XObjectArray **)res->GetWriteDataPtr();
        XObjectArray *arr = *arrPtr;
        arr->Clear();
        
        int count = mesh->GetMaterialCount();
        for (int i = 0; i < count; i++)
        {
            CKMaterial *mat = mesh->GetMaterial(i);
            if (mat)
                arr->PushBack(mat->GetID());
            else
                arr->PushBack(0);
        }
    }
}

// Original symbol: ?CKObjectArrayGetChildren3dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4FFB0
// Gets all children of a 3D entity and stores them in an ObjectArray
void CKObjectArrayGetChildren3dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Get the output array and clear it
    XObjectArray **arrPtr = (XObjectArray **)res->GetWriteDataPtr();
    XObjectArray *arr = *arrPtr;
    arr->Clear();
    
    // Read entity ID
    CK_ID entityId = ReadObjectID(p1);
    CK3dEntity *entity = (CK3dEntity *)context->GetObject(entityId);
    
    if (entity)
    {
        int count = entity->GetChildrenCount();
        for (int i = 0; i < count; i++)
        {
            CK3dEntity *child = entity->GetChild(i);
            if (child)
                arr->PushBack(child->GetID());
            else
                arr->PushBack(0);
        }
    }
}

// Original symbol: ?CKObjectArrayGetMeshList3dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4FE50
// Gets all meshes of a 3D entity and stores them in an ObjectArray
void CKObjectArrayGetMeshList3dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Get the output array and clear it
    XObjectArray **arrPtr = (XObjectArray **)res->GetWriteDataPtr();
    XObjectArray *arr = *arrPtr;
    arr->Clear();
    
    // Read entity ID
    CK_ID entityId = ReadObjectID(p1);
    CK3dEntity *entity = (CK3dEntity *)context->GetObject(entityId);
    
    if (entity)
    {
        int count = entity->GetMeshCount();
        for (int i = 0; i < count; i++)
        {
            CKMesh *mesh = entity->GetMesh(i);
            if (mesh)
                arr->PushBack(mesh->GetID());
            else
                arr->PushBack(0);
        }
    }
}

// Original symbol: ?CKObjectArrayGetAnimationsCharacter@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B50110
// Gets all animations of a character and stores them in an ObjectArray
void CKObjectArrayGetAnimationsCharacter(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Read character ID
    CK_ID charId = ReadObjectID(p1);
    CKCharacter *character = (CKCharacter *)context->GetObject(charId);
    
    if (character)
    {
        // Get the output array and clear it
        XObjectArray **arrPtr = (XObjectArray **)res->GetWriteDataPtr();
        XObjectArray *arr = *arrPtr;
        arr->Clear();
        
        int count = character->GetAnimationCount();
        for (int i = 0; i < count; i++)
        {
            CKAnimation *anim = character->GetAnimation(i);
            if (anim)
                arr->PushBack(anim->GetID());
            else
                arr->PushBack(0);
        }
    }
}

// Original symbol: ?CKObjectArrayGetBodyPartCharacter@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B50270
// Gets all body parts of a character and stores them in an ObjectArray
void CKObjectArrayGetBodyPartCharacter(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Read character ID
    CK_ID charId = ReadObjectID(p1);
    CKCharacter *character = (CKCharacter *)context->GetObject(charId);
    
    if (character)
    {
        // Get the output array and clear it
        XObjectArray **arrPtr = (XObjectArray **)res->GetWriteDataPtr();
        XObjectArray *arr = *arrPtr;
        arr->Clear();
        
        int count = character->GetBodyPartCount();
        for (int i = 0; i < count; i++)
        {
            CKBodyPart *bp = character->GetBodyPart(i);
            if (bp)
                arr->PushBack(bp->GetID());
            else
                arr->PushBack(0);
        }
    }
}

// Original symbol: ?CKObjectGetObjectByNameString@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B503D0
// Finds an object by name, result type is determined by parameter type
void CKObjectGetObjectByNameString(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Get the expected class type from the result parameter's type
    int paramType = res->GetType();
    CKParameterManager *pm = context->GetParameterManager();
    CK_CLASSID classId = pm->TypeToClassID(paramType);

    // Read the name string
    CKParameter *src = GetSourceParameter(p1);
    CKSTRING name = src ? (CKSTRING)src->GetReadDataPtr(TRUE) : NULL;

    // Find the object by name
    CKObject *obj = context->GetObjectByNameAndParentClass(name, classId, NULL);
    if (obj)
    {
        *(CK_ID *)res->GetWriteDataPtr() = obj->GetID();
    }
    else
    {
        *(CK_ID *)res->GetWriteDataPtr() = 0;
    }
}

// Original symbol: ?CKObjectWindowPickIntInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B50460
// Picks an object at the given screen coordinates (int x, int y)
void CKObjectWindowPickIntInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKRenderManager *rm = context->GetRenderManager();
    
    // Read x and y coordinates
    int x = ReadInt(p1);
    int y = ReadInt(p2);
    
    // Create CKPOINT from coordinates
    CKPOINT pt;
    pt.x = x;
    pt.y = y;
    
    // Get render context at this point
    CKRenderContext *rc = rm->GetRenderContextFromPoint(pt);
    
    *(CK_ID *)res->GetWriteDataPtr() = 0;
    
    if (rc)
    {
        // Convert to client coordinates
        Vx2DVector screenPt((float)x, (float)y);
        rc->ScreenToClient(&screenPt);
        
        // Perform pick
        CKPICKRESULT pickRes;
        CKRenderObject *picked = rc->Pick((int)screenPt.x, (int)screenPt.y, &pickRes, FALSE);
        
        // Get the expected class type from result parameter
        CKParameterManager *pm = context->GetParameterManager();
        CK_CLASSID targetClass = pm->TypeToClassID(res->GetType());
        
        // Check if picked object matches expected type or return appropriate object
        if (picked)
        {
            if (CKIsChildClassOf(targetClass, CKCID_BEOBJECT))
            {
                // Return the picked object directly
                *(CK_ID *)res->GetWriteDataPtr() = picked->GetID();
            }
            else if (CKIsChildClassOf(targetClass, CKCID_MESH))
            {
                // If looking for a mesh, get the mesh from a 3D entity
                if (CKIsChildClassOf(picked, CKCID_3DENTITY))
                {
                    CK3dEntity *entity = (CK3dEntity *)picked;
                    CKMesh *mesh = entity->GetCurrentMesh();
                    if (mesh)
                        *(CK_ID *)res->GetWriteDataPtr() = mesh->GetID();
                }
            }
            else if (CKIsChildClassOf(targetClass, CKCID_3DENTITY))
            {
                if (CKIsChildClassOf(picked, CKCID_3DENTITY))
                    *(CK_ID *)res->GetWriteDataPtr() = picked->GetID();
            }
            else
            {
                // Default - return picked object ID
                *(CK_ID *)res->GetWriteDataPtr() = picked->GetID();
            }
        }
    }
}

// Original symbol: ?CKObjectWindowPick2dVector@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B50710
// Picks an object at the given 2D vector position
void CKObjectWindowPick2dVector(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKRenderManager *rm = context->GetRenderManager();
    
    // Read 2D vector position
    CKParameter *src = GetSourceParameter(p1);
    Vx2DVector *pos = src ? (Vx2DVector *)src->GetReadDataPtr(TRUE) : NULL;
    _p = pos;
    if (!pos)
        pos = &vector2d_tmp;
    
    // Create CKPOINT from coordinates
    CKPOINT pt;
    pt.x = (int)pos->x;
    pt.y = (int)pos->y;
    
    // Get render context at this point
    CKRenderContext *rc = rm->GetRenderContextFromPoint(pt);
    
    *(CK_ID *)res->GetWriteDataPtr() = 0;
    
    if (rc)
    {
        // Convert to client coordinates
        Vx2DVector screenPt(pos->x, pos->y);
        rc->ScreenToClient(&screenPt);
        
        // Perform pick
        CKPICKRESULT pickRes;
        CKRenderObject *picked = rc->Pick((int)screenPt.x, (int)screenPt.y, &pickRes, FALSE);
        
        // Get the expected class type from result parameter
        CKParameterManager *pm = context->GetParameterManager();
        CK_CLASSID targetClass = pm->TypeToClassID(res->GetType());
        
        // Check if picked object matches expected type
        if (picked)
        {
            if (CKIsChildClassOf(targetClass, CKCID_BEOBJECT))
            {
                *(CK_ID *)res->GetWriteDataPtr() = picked->GetID();
            }
            else if (CKIsChildClassOf(targetClass, CKCID_MESH))
            {
                if (CKIsChildClassOf(picked, CKCID_3DENTITY))
                {
                    CK3dEntity *entity = (CK3dEntity *)picked;
                    CKMesh *mesh = entity->GetCurrentMesh();
                    if (mesh)
                        *(CK_ID *)res->GetWriteDataPtr() = mesh->GetID();
                }
            }
            else if (CKIsChildClassOf(targetClass, CKCID_3DENTITY))
            {
                if (CKIsChildClassOf(picked, CKCID_3DENTITY))
                    *(CK_ID *)res->GetWriteDataPtr() = picked->GetID();
            }
            else
            {
                *(CK_ID *)res->GetWriteDataPtr() = picked->GetID();
            }
        }
    }
}

// Original symbol: ?CKObjectGetElementObjectArrayInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B509C0
// Gets an object at a specific index from an ObjectArray
void CKObjectGetElementObjectArrayInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Read index from p2 first
    int index = ReadInt(p2);

    // Read ObjectArray from p1
    XObjectArray *arr = NULL;
    CKParameter *src = GetSourceParameter(p1);
    if (src)
    {
        XObjectArray **arrPtr = (XObjectArray **)src->GetReadDataPtr(TRUE);
        _p = arrPtr;
        if (arrPtr)
            arr = *arrPtr;
        else
            arr = &xarray_tmp;
    }
    else
    {
        _p = NULL;
        arr = &xarray_tmp;
    }

    // Get the object at the index
    CKObject *obj = arr->GetObject(context, index);
    if (obj)
    {
        *(CK_ID *)res->GetWriteDataPtr() = obj->GetID();
    }
    else
    {
        *(CK_ID *)res->GetWriteDataPtr() = 0;
    }
}

// Original symbol: ?CKCurvePointGetPointCurveInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B51B20
// Gets a control point from a curve at a specific index
void CKCurvePointGetPointCurveInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Read curve object from p1
    CK_ID curveId = ReadObjectID(p1);
    CKCurve *curve = (CKCurve *)context->GetObject(curveId);
    if (!curve)
    {
        *(CK_ID *)res->GetWriteDataPtr() = 0;
        return;
    }

    // Read index from p2
    int index = ReadInt(p2);

    // Get the control point
    CKCurvePoint *point = curve->GetControlPoint(index);
    if (point)
    {
        *(CK_ID *)res->GetWriteDataPtr() = point->GetID();
    }
    else
    {
        *(CK_ID *)res->GetWriteDataPtr() = 0;
    }
}

// Original symbol: ?CKPlaceGetPlace3DEntityPlace@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B50D90
// Gets the portal (3D entity) that connects to a destination place from a source place
void CKPlaceGetPlace3DEntityPlace(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Read source 3D entity (portal we want to find)
    CK_ID entityId = ReadObjectID(p1);
    CK3dEntity *portalEntity = (CK3dEntity *)context->GetObject(entityId);
    
    // Read destination place
    CK_ID placeId = ReadObjectID(p2);
    CKPlace *place = (CKPlace *)context->GetObject(placeId);
    
    if (!portalEntity || !place)
    {
        *(CK_ID *)res->GetWriteDataPtr() = 0;
        return;
    }
    
    // Iterate through portals of the place to find one matching the entity
    int portalCount = place->GetPortalCount();
    for (int i = 0; i < portalCount; i++)
    {
        CK3dEntity *portal = NULL;
        CKPlace *destPlace = place->GetPortal(i, &portal);
        
        if (portal == portalEntity)
        {
            // Found the portal - return the destination place
            if (destPlace)
            {
                *(CK_ID *)res->GetWriteDataPtr() = destPlace->GetID();
                return;
            }
        }
    }
    
    *(CK_ID *)res->GetWriteDataPtr() = 0;
}

// Original symbol: ?CKPlaceGetRefPlace3DEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B50E90
// Gets the reference place of a 3D entity
void CKPlaceGetRefPlace3DEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CK3dEntity *entity = (CK3dEntity *)context->GetObject(id);
    if (entity)
    {
        CKPlace *place = entity->GetReferencePlace();
        if (place)
        {
            *(CK_ID *)res->GetWriteDataPtr() = place->GetID();
            return;
        }
    }
    *(CK_ID *)res->GetWriteDataPtr() = 0;
}

// Original symbol: ?CKObjectArrayGetPortalsPlacePlace@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B50F30
// Gets all portal entities that connect place1 to place2
void CKObjectArrayGetPortalsPlacePlace(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Get result array and clear it
    XObjectArray **arrPtr = (XObjectArray **)res->GetWriteDataPtr();
    XObjectArray *arr = *arrPtr;
    arr->Clear();
    
    // Read source place
    CK_ID place1Id = ReadObjectID(p1);
    CKPlace *place1 = (CKPlace *)context->GetObject(place1Id);
    
    // Read destination place
    CK_ID place2Id = ReadObjectID(p2);
    CKPlace *place2 = (CKPlace *)context->GetObject(place2Id);
    
    if (!place1)
        return;
    
    // Iterate through portals of place1 to find ones connecting to place2
    int portalCount = place1->GetPortalCount();
    for (int i = 0; i < portalCount; i++)
    {
        CK3dEntity *portal = NULL;
        CKPlace *destPlace = place1->GetPortal(i, &portal);
        
        if (destPlace == place2 && portal)
        {
            arr->PushBack(portal->GetID());
        }
    }
}

// Original symbol: ?CKObjectArrayGetPortalsPlace@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B51180
// Gets all portal entities of a place
void CKObjectArrayGetPortalsPlace(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Get result array and clear it
    XObjectArray **arrPtr = (XObjectArray **)res->GetWriteDataPtr();
    XObjectArray *arr = *arrPtr;
    arr->Clear();
    
    // Read place
    CK_ID placeId = ReadObjectID(p1);
    CKPlace *place = (CKPlace *)context->GetObject(placeId);
    
    if (!place)
        return;
    
    // Iterate through all portals and add their entities
    int portalCount = place->GetPortalCount();
    for (int i = 0; i < portalCount; i++)
    {
        CK3dEntity *portal = NULL;
        place->GetPortal(i, &portal);
        
        if (portal)
            arr->PushBack(portal->GetID());
        else
            arr->PushBack(0);
    }
}

// Original symbol: ?CK3dEntityGetTargetTargetCamera@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B51BF0
// Gets the target 3D entity from a target camera
void CK3dEntityGetTargetTargetCamera(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKTargetCamera *camera = (CKTargetCamera *)context->GetObject(id);
    if (camera)
    {
        CK3dEntity *target = camera->GetTarget();
        if (target)
        {
            *(CK_ID *)res->GetWriteDataPtr() = target->GetID();
            return;
        }
    }
    *(CK_ID *)res->GetWriteDataPtr() = 0;
}

// Original symbol: ?CK3dEntityGetTargetTargetLight@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B51C80
// Gets the target 3D entity from a target light
void CK3dEntityGetTargetTargetLight(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKTargetLight *light = (CKTargetLight *)context->GetObject(id);
    if (light)
    {
        CK3dEntity *target = light->GetTarget();
        if (target)
        {
            *(CK_ID *)res->GetWriteDataPtr() = target->GetID();
            return;
        }
    }
    *(CK_ID *)res->GetWriteDataPtr() = 0;
}

// Original symbol: ?CK3dEntityGetRootCharacter@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B51D10
// Gets the root body part from a character
void CK3dEntityGetRootCharacter(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKCharacter *character = (CKCharacter *)context->GetObject(id);
    if (character)
    {
        CKBodyPart *root = character->GetRootBodyPart();
        if (root)
        {
            *(CK_ID *)res->GetWriteDataPtr() = root->GetID();
            return;
        }
    }
    *(CK_ID *)res->GetWriteDataPtr() = 0;
}

// Original symbol: ?CK3dEntityGetParent3dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B51DA0
void CK3dEntityGetParent3dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CK3dEntity *entity = (CK3dEntity *)context->GetObject(id);
    if (entity)
    {
        CK3dEntity *parent = entity->GetParent();
        if (parent)
        {
            *(CK_ID *)res->GetWriteDataPtr() = parent->GetID();
            return;
        }
    }
    *(CK_ID *)res->GetWriteDataPtr() = 0;
}

// Original symbol: ?CK2dEntityGetParent2dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B51E30
void CK2dEntityGetParent2dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CK2dEntity *entity = (CK2dEntity *)context->GetObject(id);
    if (entity)
    {
        CK2dEntity *parent = entity->GetParent();
        if (parent)
        {
            *(CK_ID *)res->GetWriteDataPtr() = parent->GetID();
            return;
        }
    }
    *(CK_ID *)res->GetWriteDataPtr() = 0;
}

// Original symbol: ?CKMeshGetCurrent3dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B51EC0
void CKMeshGetCurrent3dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CK3dEntity *entity = (CK3dEntity *)context->GetObject(id);
    if (entity)
    {
        CKMesh *mesh = entity->GetCurrentMesh();
        if (mesh)
        {
            *(CK_ID *)res->GetWriteDataPtr() = mesh->GetID();
            return;
        }
    }
    *(CK_ID *)res->GetWriteDataPtr() = 0;
}

// Original symbol: ?CKSceneGetCurrentSceneNoneNone@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B521A0
void CKSceneGetCurrentSceneNoneNone(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKScene *scene = context->GetCurrentScene();
    if (scene)
    {
        CKLevel *level = context->GetCurrentLevel();
        // If current scene is the level scene, return 0 (no scene ID)
        if (scene == level->GetLevelScene())
            *(CK_ID *)res->GetWriteDataPtr() = 0;
        else
            *(CK_ID *)res->GetWriteDataPtr() = scene->GetID();
    }
}

// Original symbol: ?CKLevelGetCurrentLevelNoneNone@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B521F0
void CKLevelGetCurrentLevelNoneNone(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKLevel *level = context->GetCurrentLevel();
    if (level)
        *(CK_ID *)res->GetWriteDataPtr() = level->GetID();
}

// Original symbol: ?CKMaterialGetFaceMaterialMeshInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B51410
// Gets the material of a specific face in a mesh
void CKMaterialGetFaceMaterialMeshInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Read mesh from p1
    CK_ID meshId = ReadObjectID(p1);
    CKMesh *mesh = (CKMesh *)context->GetObject(meshId);
    if (!mesh)
    {
        *(CK_ID *)res->GetWriteDataPtr() = 0;
        return;
    }

    // Read face index from p2
    int faceIndex = ReadInt(p2);

    // Get the face material
    CKMaterial *mat = mesh->GetFaceMaterial(faceIndex);
    if (mat)
    {
        *(CK_ID *)res->GetWriteDataPtr() = mat->GetID();
    }
    else
    {
        *(CK_ID *)res->GetWriteDataPtr() = 0;
    }
}

// Original symbol: ?CKMaterialGetMaterialMeshInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B51550
// Gets a material at a specific index from a mesh's material list
void CKMaterialGetMaterialMeshInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Read mesh from p1
    CK_ID meshId = ReadObjectID(p1);
    CKMesh *mesh = (CKMesh *)context->GetObject(meshId);
    if (!mesh)
    {
        *(CK_ID *)res->GetWriteDataPtr() = 0;
        return;
    }

    // Read material index from p2
    int matIndex = ReadInt(p2);

    // Get the material at the index
    CKMaterial *mat = mesh->GetMaterial(matIndex);
    if (mat)
    {
        *(CK_ID *)res->GetWriteDataPtr() = mat->GetID();
    }
    else
    {
        *(CK_ID *)res->GetWriteDataPtr() = 0;
    }
}

// Original symbol: ?CKMaterialGetMaterialSprite3D@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B51690
// Gets the material from a Sprite3D
void CKMaterialGetMaterialSprite3D(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKSprite3D *sprite = (CKSprite3D *)context->GetObject(id);
    if (sprite)
    {
        CKMaterial *mat = sprite->GetMaterial();
        if (mat)
        {
            *(CK_ID *)res->GetWriteDataPtr() = mat->GetID();
            return;
        }
    }
    *(CK_ID *)res->GetWriteDataPtr() = 0;
}

// Original symbol: ?CKMaterialGetMaterial2DEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B51730
// Gets the material from a 2D entity
void CKMaterialGetMaterial2DEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CK2dEntity *entity = (CK2dEntity *)context->GetObject(id);
    if (entity)
    {
        CKMaterial *mat = entity->GetMaterial();
        if (mat)
        {
            *(CK_ID *)res->GetWriteDataPtr() = mat->GetID();
            return;
        }
    }
    *(CK_ID *)res->GetWriteDataPtr() = 0;
}

// Original symbol: ?CKTextureGetTextureMaterial@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B51370
// Gets the texture from a material (at texture index 0)
void CKTextureGetTextureMaterial(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKMaterial *mat = (CKMaterial *)context->GetObject(id);
    if (mat)
    {
        CKTexture *tex = mat->GetTexture(0);
        if (tex)
        {
            *(CK_ID *)res->GetWriteDataPtr() = tex->GetID();
            return;
        }
    }
    *(CK_ID *)res->GetWriteDataPtr() = 0;
}

// Original symbol: ?CKCharacterGetCharacter3dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B50B40
// Gets the character from a 3D entity (if it's a BodyPart, returns its character; if it's already a Character, returns itself)
void CKCharacterGetCharacter3dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKObject *obj = context->GetObject(id);
    
    // If it's a BodyPart, get its character
    if (CKIsChildClassOf(obj, CKCID_BODYPART))
    {
        CKBodyPart *bodyPart = (CKBodyPart *)obj;
        CKCharacter *character = bodyPart->GetCharacter();
        if (character)
        {
            *(CK_ID *)res->GetWriteDataPtr() = character->GetID();
            return;
        }
    }
    // If it's already a Character, return itself
    else if (CKIsChildClassOf(obj, CKCID_CHARACTER) && obj)
    {
        *(CK_ID *)res->GetWriteDataPtr() = obj->GetID();
        return;
    }
    
    *(CK_ID *)res->GetWriteDataPtr() = 0;
}

// Original symbol: ?CKIntGetCountGroup@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B46200
// Gets the number of objects in a group
void CKIntGetCountGroup(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKGroup *group = (CKGroup *)context->GetObject(id);
    if (group)
    {
        *(int *)res->GetWriteDataPtr() = group->GetObjectCount();
    }
}

// Original symbol: ?CKObjectGetElementGroupInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B50A70
// Gets an object at a specific index from a group
void CKObjectGetElementGroupInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Read group from p1
    CK_ID groupId = ReadObjectID(p1);
    CKGroup *group = (CKGroup *)context->GetObject(groupId);
    if (!group)
    {
        *(CK_ID *)res->GetWriteDataPtr() = 0;
        return;
    }

    // Read index from p2
    int index = ReadInt(p2);

    // Get the object at the index
    CKBeObject *obj = group->GetObject(index);
    if (obj)
    {
        *(CK_ID *)res->GetWriteDataPtr() = obj->GetID();
    }
    else
    {
        *(CK_ID *)res->GetWriteDataPtr() = 0;
    }
}

// Original symbol: ?CKBoolSetFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B47400
// Converts float to bool (non-zero -> TRUE)
void CKBoolSetFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    float val = ReadFloat(p1);
    *(CKDWORD *)res->GetWriteDataPtr() = val != 0.0f;
}

// Original symbol: ?CKBoolSetInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B47390
// Converts int to bool (non-zero -> TRUE)
void CKBoolSetInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKParameter *src = GetSourceParameter(p1);
    int *ptr = src ? (int *)src->GetReadDataPtr(TRUE) : NULL;
    _p = ptr;
    if (ptr)
    {
        int val = *ptr;
        *(CKDWORD *)res->GetWriteDataPtr() = val != 0;
    }
    else
    {
        *(CKDWORD *)res->GetWriteDataPtr() = 0;
    }
}

// Original symbol: ?CKIntSetBool@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B457F0
// Converts bool to int (FALSE=0, TRUE=1)
void CKIntSetBool(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKParameter *src = GetSourceParameter(p1);
    CKBYTE *ptr = src ? (CKBYTE *)src->GetReadDataPtr(TRUE) : NULL;
    _p = ptr;
    if (ptr)
    {
        CKBOOL val = (*ptr != 0);
        *(int *)res->GetWriteDataPtr() = val;
    }
    else
    {
        *(int *)res->GetWriteDataPtr() = 0;
    }
}

// Original symbol: ?CKFloatSetBool@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B42C50
// Converts bool to float (FALSE=0.0f, TRUE=1.0f)
void CKFloatSetBool(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKParameter *src = GetSourceParameter(p1);
    CKBYTE *ptr = src ? (CKBYTE *)src->GetReadDataPtr(TRUE) : NULL;
    _p = ptr;
    float result;
    if (!ptr || !*ptr)
        result = 0.0f;
    else
        result = 1.0f;
    *(float *)res->GetWriteDataPtr() = result;
}

// Original symbol: ?CKBodyPartGetBodyPartByIncludedNameCharacterString@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B50C90
// Gets a body part from a character by searching for one whose name contains the given string
void CKBodyPartGetBodyPartByIncludedNameCharacterString(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Read Character from p1
    CK_ID charId = ReadObjectID(p1);
    CKObject *obj = context->GetObject(charId);
    
    if (!obj || !CKIsChildClassOf(obj, CKCID_CHARACTER))
        return;
    
    CKCharacter *character = (CKCharacter *)obj;
    int bodyPartCount = character->GetBodyPartCount();
    
    // Read search string from p2
    CKParameter *src = GetSourceParameter(p2);
    if (!src)
        return;
    
    const char *searchStr = (const char *)src->GetReadDataPtr(TRUE);
    if (!searchStr)
        return;
    
    // Search for body part with matching name
    for (int i = 0; i < bodyPartCount; ++i)
    {
        CKBodyPart *bodyPart = character->GetBodyPart(i);
        if (bodyPart)
        {
            const char *name = bodyPart->GetName();
            if (name && strstr(name, searchStr))
            {
                *(CK_ID *)res->GetWriteDataPtr() = bodyPart->GetID();
                return;
            }
        }
    }
}

// Original symbol: ?CKScriptGetScriptBeObjectInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B51F50
// Gets a script at a specific index from a BeObject
void CKScriptGetScriptBeObjectInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Read BeObject from p1
    CK_ID objId = ReadObjectID(p1);
    CKBeObject *obj = (CKBeObject *)context->GetObject(objId);
    if (!obj)
    {
        res->GetWriteDataPtr(); // Binary calls this even when returning
        return;
    }

    // Read script index from p2
    int index = ReadInt(p2);

    // Get the script at the index
    CKBehavior *script = obj->GetScript(index);
    if (script)
    {
        *(CK_ID *)res->GetWriteDataPtr() = script->GetID();
    }
    else
    {
        *(CK_ID *)res->GetWriteDataPtr() = 0;
    }
}

// Original symbol: ?CKScriptGetScriptBeObjectString@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B52020
// Gets a script from a BeObject by searching for a script whose name contains the given string
void CKScriptGetScriptBeObjectString(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    // Read BeObject from p1
    CK_ID objId = ReadObjectID(p1);
    CKBeObject *obj = (CKBeObject *)context->GetObject(objId);
    if (!obj)
        return;

    // Get script count
    int scriptCount = obj->GetScriptCount();

    // Read search string from p2
    CKParameter *src = GetSourceParameter(p2);
    const char *searchStr = src ? (const char *)src->GetReadDataPtr(TRUE) : NULL;

    // Search for script with matching name
    for (int i = 0; i < scriptCount; ++i)
    {
        CKBehavior *script = obj->GetScript(i);
        if (strstr(script->GetName(), searchStr))
        {
            *(CK_ID *)res->GetWriteDataPtr() = script->GetID();
            return;
        }
    }
}

// Original symbol: ?CKBeObjectCastCKBeObject@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B520F0
// Casts a BeObject to a specific type based on the result parameter type
void CKBeObjectCastCKBeObject(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKObject *obj = context->GetObject(id);
    
    // Get the target class type from the result parameter's type
    int paramType = res->GetType();
    CKParameterManager *pm = context->GetParameterManager();
    CK_CLASSID classId = pm->TypeToClassID(paramType);
    
    // Check if the object can be cast to the target type
    if (obj && CKIsChildClassOf(obj, classId))
    {
        *(CK_ID *)res->GetWriteDataPtr() = obj->GetID();
    }
    else
    {
        *(CK_ID *)res->GetWriteDataPtr() = 0;
    }
}

// Original symbol: ?CKObjectAnimationGetAnimation3dEntityString@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B517D0
// Gets an object animation by name from a 3D entity
void CKObjectAnimationGetAnimation3dEntityString(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CK3dEntity *entity = (CK3dEntity *)context->GetObject(id);
    if (!entity)
        return;

    CKParameter *src = GetSourceParameter(p2);
    const char *name = src ? (const char *)src->GetReadDataPtr(TRUE) : NULL;

    int count = entity->GetObjectAnimationCount();
    for (int i = 0; i < count; i++)
    {
        CKObjectAnimation *anim = entity->GetObjectAnimation(i);
        if (anim)
        {
            const char *animName = anim->GetName();
            if (animName && strstr(animName, name))
            {
                *(CK_ID *)res->GetWriteDataPtr() = anim->GetID();
                return;
            }
        }
    }
}

// Original symbol: ?CKObjectAnimationGetAnimation3dEntityInt@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B518E0
// Gets an object animation by index from a 3D entity
void CKObjectAnimationGetAnimation3dEntityInt(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CK3dEntity *entity = (CK3dEntity *)context->GetObject(id);
    if (!entity)
    {
        *(CK_ID *)res->GetWriteDataPtr() = 0;
        return;
    }

    int index = ReadInt(p2);
    CKObjectAnimation *anim = entity->GetObjectAnimation(index);
    if (anim)
    {
        *(CK_ID *)res->GetWriteDataPtr() = anim->GetID();
    }
    else
    {
        *(CK_ID *)res->GetWriteDataPtr() = 0;
    }
}

// Original symbol: ?CKIntGetAnimationCount3dEntity@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B519B0
// Gets the number of object animations on a 3D entity
void CKIntGetAnimationCount3dEntity(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CK3dEntity *entity = (CK3dEntity *)context->GetObject(id);
    if (entity)
    {
        *(int *)res->GetWriteDataPtr() = entity->GetObjectAnimationCount();
    }
    else
    {
        *(int *)res->GetWriteDataPtr() = 0;
    }
}

// Original symbol: ?CKAnimationGetAnimationCharacterString@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B51A30
// Gets an animation by name from a character
void CKAnimationGetAnimationCharacterString(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKCharacter *character = (CKCharacter *)context->GetObject(id);
    if (!character)
        return;

    CKParameter *src = GetSourceParameter(p2);
    const char *name = src ? (const char *)src->GetReadDataPtr(TRUE) : NULL;

    int count = character->GetAnimationCount();
    for (int i = 0; i < count; i++)
    {
        CKAnimation *anim = character->GetAnimation(i);
        if (anim)
        {
            const char *animName = anim->GetName();
            if (animName && strstr(animName, name))
            {
                *(CK_ID *)res->GetWriteDataPtr() = anim->GetID();
                return;
            }
        }
    }
}

// Original symbol: ?CKQuaternionFromRotation@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4AFA0
// Creates a quaternion from an axis-angle rotation
void CKQuaternionFromRotation(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKParameter *src1 = GetSourceParameter(p1);
    _p = src1 ? src1->GetReadDataPtr(TRUE) : NULL;
    VxVector *axis = (VxVector *)_p;
    if (!axis)
        axis = &vector_tmp;

    CKParameter *src2 = GetSourceParameter(p2);
    _p = src2 ? src2->GetReadDataPtr(TRUE) : NULL;
    float angle = _p ? *(float *)_p : 0.0f;

    VxQuaternion quat(0.0f, 0.0f, 0.0f, 1.0f);
    quat.FromRotation(*axis, angle);
    res->SetValue(&quat, 0);
}

// Original symbol: ?CKMatrixFromRotation@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4D690
// Creates a matrix from an axis-angle rotation
void CKMatrixFromRotation(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    VxVector axis;
    axis.z = 0.0f;
    CKParameter *src1 = GetSourceParameter(p1);
    if (src1)
        src1->GetValue(&axis, TRUE);

    float angle = 0.0f;
    CKParameter *src2 = GetSourceParameter(p2);
    if (src2)
        src2->GetValue(&angle, TRUE);

    VxMatrix mat;
    Vx3DMatrixFromRotation(mat, axis, angle);
    res->SetValue(&mat, 0);
}

// Original symbol: ?CKStringGetSoundFileNameWaveSound@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B44180
// Gets the filename of a WaveSound
void CKStringGetSoundFileNameWaveSound(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKWaveSound *sound = (CKWaveSound *)context->GetObject(id);
    if (sound)
    {
        CKSTRING filename = sound->GetSoundFileName();
        res->SetStringValue(filename);
    }
}

// Original symbol: ?CKFloatGetLengthWaveSound@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B44110
// Gets the length of a WaveSound in seconds (converted from ms)
void CKFloatGetLengthWaveSound(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKWaveSound *sound = (CKWaveSound *)context->GetObject(id);
    if (sound)
    {
        *(float *)res->GetWriteDataPtr() = (float)sound->GetSoundLength();
    }
}

// Original symbol: ?CKIntGetFrequencyWaveSound@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B43F10
// Gets the sample rate (frequency) of a WaveSound
void CKIntGetFrequencyWaveSound(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKWaveSound *sound = (CKWaveSound *)context->GetObject(id);
    if (sound)
    {
        CKWaveFormat format;
        sound->GetSoundFormat(format);
        *(CKDWORD *)res->GetWriteDataPtr() = format.nSamplesPerSec;
    }
}

// Original symbol: ?CKBoolGetLoopModeWaveSound@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B441F0
// Gets the loop mode of a WaveSound
void CKBoolGetLoopModeWaveSound(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKWaveSound *sound = (CKWaveSound *)context->GetObject(id);
    if (sound)
    {
        *(CKDWORD *)res->GetWriteDataPtr() = sound->GetLoopMode();
    }
}

// Original symbol: ?CKBoolGetFileStreamingWaveSound@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B44260
// Gets the file streaming mode of a WaveSound
void CKBoolGetFileStreamingWaveSound(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKWaveSound *sound = (CKWaveSound *)context->GetObject(id);
    if (sound)
    {
        *(CKDWORD *)res->GetWriteDataPtr() = sound->GetFileStreaming();
    }
}

// Original symbol: ?CKBoolIsPlayingWaveSound@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B442D0
// Checks if a WaveSound is playing
void CKBoolIsPlayingWaveSound(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKWaveSound *sound = (CKWaveSound *)context->GetObject(id);
    if (sound)
    {
        *(CKDWORD *)res->GetWriteDataPtr() = sound->IsPlaying();
    }
}

// Original symbol: ?CKBoolIsPausedWaveSound@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B44340
// Checks if a WaveSound is paused
void CKBoolIsPausedWaveSound(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKWaveSound *sound = (CKWaveSound *)context->GetObject(id);
    if (sound)
    {
        *(CKDWORD *)res->GetWriteDataPtr() = sound->IsPaused();
    }
}

// Original symbol: ?CKFloatGetVolumeWaveSound@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B43F80
// Gets the gain (volume) of a WaveSound
void CKFloatGetVolumeWaveSound(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKWaveSound *sound = (CKWaveSound *)context->GetObject(id);
    if (sound)
    {
        *(float *)res->GetWriteDataPtr() = sound->GetGain();
    }
}

// Original symbol: ?CKFloatGetPitchWaveSound@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B443B0
// Gets the pitch of a WaveSound
void CKFloatGetPitchWaveSound(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKWaveSound *sound = (CKWaveSound *)context->GetObject(id);
    if (sound)
    {
        *(float *)res->GetWriteDataPtr() = sound->GetPitch();
    }
}

// Original symbol: ?CKFloatGetPanWaveSound@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B44420
// Gets the stereo pan of a WaveSound
void CKFloatGetPanWaveSound(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKWaveSound *sound = (CKWaveSound *)context->GetObject(id);
    if (sound)
    {
        *(float *)res->GetWriteDataPtr() = sound->GetPan();
    }
}

// Original symbol: ?CKVectorGetRelPositionWaveSound@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B44490
// Gets the relative position of a 3D WaveSound
void CKVectorGetRelPositionWaveSound(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKWaveSound *sound = (CKWaveSound *)context->GetObject(id);
    if (sound)
    {
        VxVector pos, dir;
        pos.z = 0.0f;
        dir.z = 0.0f;
        float dist;
        sound->GetSound3DInformation(pos, dir, dist);
        *(VxVector *)res->GetWriteDataPtr() = pos;
    }
}

// Original symbol: ?CKVectorGetRelDirectionWaveSound@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B44530
// Gets the relative direction of a 3D WaveSound
void CKVectorGetRelDirectionWaveSound(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKWaveSound *sound = (CKWaveSound *)context->GetObject(id);
    if (sound)
    {
        VxVector pos, dir;
        pos.z = 0.0f;
        dir.z = 0.0f;
        float dist;
        sound->GetSound3DInformation(pos, dir, dist);
        *(VxVector *)res->GetWriteDataPtr() = dir;
    }
}

// Original symbol: ?CKFloatGetDistanceFromListenerWaveSound@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B445D0
// Gets the distance from the listener for a 3D WaveSound
void CKFloatGetDistanceFromListenerWaveSound(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKWaveSound *sound = (CKWaveSound *)context->GetObject(id);
    if (sound)
    {
        VxVector pos, dir;
        pos.z = 0.0f;
        dir.z = 0.0f;
        float dist;
        sound->GetSound3DInformation(pos, dir, dist);
        *(float *)res->GetWriteDataPtr() = dist;
    }
}

// Original symbol: ?CKVectorGetConeWaveSound@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B44660
// Gets the cone parameters of a 3D WaveSound (angles in radians)
void CKVectorGetConeWaveSound(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKWaveSound *sound = (CKWaveSound *)context->GetObject(id);
    if (sound)
    {
        float *ptr = (float *)res->GetWriteDataPtr();
        sound->GetCone(&ptr[0], &ptr[1], &ptr[2]);
        // Convert InAngle and OutAngle from degrees to radians
        ptr[0] *= 0.017453292f; // PI/180
        ptr[1] *= 0.017453292f; // PI/180
    }
}

// Original symbol: ?CK2dVectorGetMinMaxDistanceWaveSound@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B446F0
// Gets the min/max distance of a 3D WaveSound
void CK2dVectorGetMinMaxDistanceWaveSound(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKWaveSound *sound = (CKWaveSound *)context->GetObject(id);
    if (sound)
    {
        float *ptr = (float *)res->GetWriteDataPtr();
        CKDWORD behavior;
        sound->GetMinMaxDistance(&ptr[0], &ptr[1], &behavior);
    }
}

// Original symbol: ?CKVectorGetVelocityWaveSound@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B44760
// Gets the velocity of a 3D WaveSound
void CKVectorGetVelocityWaveSound(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKWaveSound *sound = (CKWaveSound *)context->GetObject(id);
    if (sound)
    {
        VxVector *ptr = (VxVector *)res->GetWriteDataPtr();
        sound->GetVelocity(*ptr);
    }
}

// Original symbol: ?CKTimeGetPlayedMS@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B440A0
// Gets the current playback position in milliseconds of a WaveSound
void CKTimeGetPlayedMS(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id = ReadObjectID(p1);
    CKWaveSound *sound = (CKWaveSound *)context->GetObject(id);
    if (sound)
    {
        *(float *)res->GetWriteDataPtr() = (float)sound->GetPlayedMs();
    }
}

// Original symbol: ?CKBoolEqualDataArrayDataArray@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B45C60
// Checks if two DataArrays are equal (same structure and content)
void CKBoolEqualDataArrayDataArray(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id1 = ReadObjectID(p1);
    CKDataArray *arr1 = (CKDataArray *)context->GetObject(id1);
    
    CK_ID id2 = ReadObjectID(p2);
    CKDataArray *arr2 = (CKDataArray *)context->GetObject(id2);

    CKBOOL equal = FALSE;
    
    // Both null means equal
    if (!arr1 && !arr2)
    {
        equal = TRUE;
    }
    else if (arr1 && arr2)
    {
        // Check row count
        int rowCount = arr1->GetRowCount();
        if (rowCount == arr2->GetRowCount())
        {
            // Check column count
            int colCount = arr1->GetColumnCount();
            if (colCount == arr2->GetColumnCount())
            {
                // Check column types and parameter GUIDs
                int c;
                for (c = 0; c < colCount; c++)
                {
                    CK_ARRAYTYPE type1 = arr1->GetColumnType(c);
                    if (type1 != arr2->GetColumnType(c))
                        break;
                    if (type1 == CKARRAYTYPE_PARAMETER)
                    {
                        CKGUID guid1 = arr1->GetColumnParameterGuid(c);
                        CKGUID guid2 = arr2->GetColumnParameterGuid(c);
                        if (!(guid1 == guid2))
                            break;
                    }
                }
                
                if (c == colCount)
                {
                    equal = TRUE;
                    // Check all elements
                    for (c = 0; c < colCount && equal; c++)
                    {
                        for (int r = 0; r < rowCount && equal; r++)
                        {
                            CK_ARRAYTYPE type = arr1->GetColumnType(c);
                            switch (type)
                            {
                                case CKARRAYTYPE_INT:
                                case CKARRAYTYPE_OBJECT:
                                {
                                    int v1, v2;
                                    arr1->GetElementValue(r, c, &v1);
                                    arr2->GetElementValue(r, c, &v2);
                                    if (v1 != v2) equal = FALSE;
                                    break;
                                }
                                case CKARRAYTYPE_FLOAT:
                                {
                                    float f1, f2;
                                    arr1->GetElementValue(r, c, &f1);
                                    arr2->GetElementValue(r, c, &f2);
                                    if (f1 != f2) equal = FALSE;
                                    break;
                                }
                                case CKARRAYTYPE_STRING:
                                {
                                    const char *s1 = (const char *)*arr1->GetElement(r, c);
                                    const char *s2 = (const char *)*arr2->GetElement(r, c);
                                    if (strcmp(s1, s2) != 0) equal = FALSE;
                                    break;
                                }
                                case CKARRAYTYPE_PARAMETER:
                                {
                                    CKParameter *param1 = (CKParameter *)*arr1->GetElement(r, c);
                                    CKParameter *param2 = (CKParameter *)*arr2->GetElement(r, c);
                                    int size = param1->GetDataSize();
                                    const void *data1 = param1->GetReadDataPtr(TRUE);
                                    const void *data2 = param2->GetReadDataPtr(TRUE);
                                    if (memcmp(data1, data2, size) != 0) equal = FALSE;
                                    break;
                                }
                                default:
                                    break;
                            }
                        }
                    }
                }
            }
        }
    }
    
    *(CKBOOL *)res->GetWriteDataPtr() = equal;
}

// Original symbol: ?CKBoolEqualGroupGroup@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B45F40
// Checks if two Groups are equal (contain the same objects)
void CKBoolEqualGroupGroup(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CK_ID id1 = ReadObjectID(p1);
    CKGroup *group1 = (CKGroup *)context->GetObject(id1);
    
    CK_ID id2 = ReadObjectID(p2);
    CKGroup *group2 = (CKGroup *)context->GetObject(id2);

    CKBOOL equal = FALSE;
    
    if (group1)
    {
        if (!group2)
            goto done;
        
        int count = group1->GetObjectCount();
        if (count != group2->GetObjectCount())
            goto done;
        
        for (int i = 0; i < count; i++)
        {
            CKBeObject *obj = group1->GetObject(i);
            if (obj && !obj->IsInGroup(group2))
                goto done;
        }
        equal = TRUE;
    }
    else
    {
        // group1 is null, equal if group2 is also null
        equal = (group2 == NULL);
    }

done:
    *(CKBOOL *)res->GetWriteDataPtr() = equal;
}

// Original symbol: ?CKBoolEqualStringString@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B47840
// Checks if two strings are equal
void CKBoolEqualStringString(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKParameter *src1 = GetSourceParameter(p1);
    const char *s1 = src1 ? (const char *)src1->GetReadDataPtr(TRUE) : NULL;
    
    CKParameter *src2 = GetSourceParameter(p2);
    const char *s2 = src2 ? (const char *)src2->GetReadDataPtr(TRUE) : NULL;
    
    CKBOOL equal = (s1 && s2 && strcmp(s1, s2) == 0);
    *(CKBOOL *)res->GetWriteDataPtr() = equal;
}

// Original symbol: ?CKBoolEqualObjectArrayObjectArray@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B4FBF0
// Checks if two object arrays contain the same elements (order independent)
void CKBoolEqualObjectArrayObjectArray(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKParameter *src1 = GetSourceParameter(p1);
    XObjectArray **arr1_ptr = src1 ? (XObjectArray **)src1->GetReadDataPtr(TRUE) : NULL;
    _p = arr1_ptr;
    XObjectArray *arr1 = arr1_ptr ? *arr1_ptr : &xarray_tmp;
    
    CKParameter *src2 = GetSourceParameter(p2);
    XObjectArray **arr2_ptr = src2 ? (XObjectArray **)src2->GetReadDataPtr(TRUE) : NULL;
    _p = arr2_ptr;
    XObjectArray *arr2 = arr2_ptr ? *arr2_ptr : &xarray_tmp;

    if (arr1->Size() != arr2->Size())
    {
        *(CKBOOL *)res->GetWriteDataPtr() = FALSE;
        return;
    }

    // Size equality + "each element of arr1 exists in arr2" (order-independent, no multiplicity check).
    CKBOOL equal = TRUE;
    for (CK_ID *it = arr1->Begin(); it != arr1->End(); ++it)
    {
        if (!arr2->FindID(*it))
        {
            equal = FALSE;
            break;
        }
    }

    *(CKBOOL *)res->GetWriteDataPtr() = equal;
}

// Original symbol: ?CKBoolEqualIntFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B476C0
// Checks if an int equals a float (int is converted to float for comparison)
void CKBoolEqualIntFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKParameter *src1 = GetSourceParameter(p1);
    _p = src1 ? src1->GetReadDataPtr(TRUE) : NULL;
    int intVal = _p ? *(int *)_p : 0;
    
    CKParameter *src2 = GetSourceParameter(p2);
    _p = src2 ? src2->GetReadDataPtr(TRUE) : NULL;
    float floatVal = _p ? *(float *)_p : 0.0f;
    
    *(CKBOOL *)res->GetWriteDataPtr() = ((double)intVal == (double)floatVal);
}

// Original symbol: ?CKBoolNotEqualIntFloat@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B47780
// Checks if an int does not equal a float
void CKBoolNotEqualIntFloat(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKParameter *src1 = GetSourceParameter(p1);
    _p = src1 ? src1->GetReadDataPtr(TRUE) : NULL;
    int intVal = _p ? *(int *)_p : 0;
    
    CKParameter *src2 = GetSourceParameter(p2);
    _p = src2 ? src2->GetReadDataPtr(TRUE) : NULL;
    float floatVal = _p ? *(float *)_p : 0.0f;
    
    *(CKBOOL *)res->GetWriteDataPtr() = ((double)intVal != (double)floatVal);
}

// Original symbol: ?CKBoolEqualBoolBool@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B46860
// Checks if two bools are equal
void CKBoolEqualBoolBool(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKParameter *src1 = GetSourceParameter(p1);
    _p = src1 ? src1->GetReadDataPtr(TRUE) : NULL;
    CKBOOL b1 = _p ? (*(CKBYTE *)_p != 0) : FALSE;
    
    CKParameter *src2 = GetSourceParameter(p2);
    _p = src2 ? src2->GetReadDataPtr(TRUE) : NULL;
    CKBOOL b2 = _p ? (*(CKBYTE *)_p != 0) : FALSE;
    
    *(CKBOOL *)res->GetWriteDataPtr() = ((b1 != 0) == b2);
}

// Original symbol: ?CKBoolNotEqualBoolBool@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B46910
// Checks if two bools are not equal
void CKBoolNotEqualBoolBool(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKParameter *src1 = GetSourceParameter(p1);
    _p = src1 ? src1->GetReadDataPtr(TRUE) : NULL;
    CKBOOL b1 = _p ? (*(CKBYTE *)_p != 0) : FALSE;
    
    CKParameter *src2 = GetSourceParameter(p2);
    _p = src2 ? src2->GetReadDataPtr(TRUE) : NULL;
    CKBOOL b2 = _p ? (*(CKBYTE *)_p != 0) : FALSE;
    
    *(CKBOOL *)res->GetWriteDataPtr() = ((b1 != 0) != b2);
}

// Original symbol: ?CKGenericEqual1Dword@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B41320
// Generic equality for 1 DWORD (4 bytes)
void CKGenericEqual1Dword(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKParameter *src1 = GetSourceParameter(p1);
    CKDWORD *d1 = src1 ? (CKDWORD *)src1->GetReadDataPtr(TRUE) : NULL;
    
    CKParameter *src2 = GetSourceParameter(p2);
    CKDWORD *d2 = src2 ? (CKDWORD *)src2->GetReadDataPtr(TRUE) : NULL;

    if (!d1 || !d2) {
        *(CKBOOL *)res->GetWriteDataPtr() = (d1 == d2) ? TRUE : FALSE;
        return;
    }

    *(CKBOOL *)res->GetWriteDataPtr() = (*d1 == *d2);
}

// Original symbol: ?CKGenericNotEqual1Dword@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B41620
// Generic inequality for 1 DWORD (4 bytes)
void CKGenericNotEqual1Dword(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKParameter *src1 = GetSourceParameter(p1);
    CKDWORD *d1 = src1 ? (CKDWORD *)src1->GetReadDataPtr(TRUE) : NULL;
    
    CKParameter *src2 = GetSourceParameter(p2);
    CKDWORD *d2 = src2 ? (CKDWORD *)src2->GetReadDataPtr(TRUE) : NULL;

    if (!d1 || !d2) {
        *(CKBOOL *)res->GetWriteDataPtr() = (d1 == d2) ? FALSE : TRUE;
        return;
    }

    *(CKBOOL *)res->GetWriteDataPtr() = (*d1 != *d2);
}

// Original symbol: ?CKGenericEqual2Dword@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B413E0
// Generic equality for 2 DWORDs (8 bytes)
void CKGenericEqual2Dword(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKParameter *src1 = GetSourceParameter(p1);
    void *d1 = src1 ? src1->GetReadDataPtr(TRUE) : NULL;
    
    CKParameter *src2 = GetSourceParameter(p2);
    void *d2 = src2 ? src2->GetReadDataPtr(TRUE) : NULL;

    if (!d1 || !d2) {
        *(CKBOOL *)res->GetWriteDataPtr() = (d1 == d2) ? TRUE : FALSE;
        return;
    }

    *(CKBOOL *)res->GetWriteDataPtr() = (memcmp(d1, d2, 8) == 0);
}

// Original symbol: ?CKGenericNotEqual2Dword@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B416E0
// Generic inequality for 2 DWORDs (8 bytes)
void CKGenericNotEqual2Dword(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKParameter *src1 = GetSourceParameter(p1);
    void *d1 = src1 ? src1->GetReadDataPtr(TRUE) : NULL;
    
    CKParameter *src2 = GetSourceParameter(p2);
    void *d2 = src2 ? src2->GetReadDataPtr(TRUE) : NULL;
    
    *(CKBOOL *)res->GetWriteDataPtr() = (memcmp(d1, d2, 8) != 0);
}

// Original symbol: ?CKGenericEqual3Dword@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B414A0
// Generic equality for 3 DWORDs (12 bytes)
void CKGenericEqual3Dword(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKParameter *src1 = GetSourceParameter(p1);
    void *d1 = src1 ? src1->GetReadDataPtr(TRUE) : NULL;
    
    CKParameter *src2 = GetSourceParameter(p2);
    void *d2 = src2 ? src2->GetReadDataPtr(TRUE) : NULL;
    
    *(CKBOOL *)res->GetWriteDataPtr() = (memcmp(d1, d2, 12) == 0);
}

// Original symbol: ?CKGenericNotEqual3Dword@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B417A0
// Generic inequality for 3 DWORDs (12 bytes)
void CKGenericNotEqual3Dword(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKParameter *src1 = GetSourceParameter(p1);
    void *d1 = src1 ? src1->GetReadDataPtr(TRUE) : NULL;
    
    CKParameter *src2 = GetSourceParameter(p2);
    void *d2 = src2 ? src2->GetReadDataPtr(TRUE) : NULL;
    
    *(CKBOOL *)res->GetWriteDataPtr() = (memcmp(d1, d2, 12) != 0);
}

// Original symbol: ?CKGenericEqual4Dword@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B41560
// Generic equality for 4 DWORDs (16 bytes)
void CKGenericEqual4Dword(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKParameter *src1 = GetSourceParameter(p1);
    void *d1 = src1 ? src1->GetReadDataPtr(TRUE) : NULL;
    
    CKParameter *src2 = GetSourceParameter(p2);
    void *d2 = src2 ? src2->GetReadDataPtr(TRUE) : NULL;
    
    *(CKBOOL *)res->GetWriteDataPtr() = (memcmp(d1, d2, 16) == 0);
}

// Original symbol: ?CKGenericNotEqual4Dword@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B41860
// Generic inequality for 4 DWORDs (16 bytes)
void CKGenericNotEqual4Dword(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKParameter *src1 = GetSourceParameter(p1);
    void *d1 = src1 ? src1->GetReadDataPtr(TRUE) : NULL;
    
    CKParameter *src2 = GetSourceParameter(p2);
    void *d2 = src2 ? src2->GetReadDataPtr(TRUE) : NULL;
    
    *(CKBOOL *)res->GetWriteDataPtr() = (memcmp(d1, d2, 16) != 0);
}

// Original symbol: ?CKBoolEqualMatrixMatrix@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B47B90
// Compares two matrices for equality using memcmp (64 bytes = 4x4 floats)
void CKBoolEqualMatrixMatrix(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKParameter *src2 = GetSourceParameter(p2);
    void *d2 = src2 ? src2->GetReadDataPtr(TRUE) : NULL;
    _p = d2;
    const void *mat2 = d2 ? d2 : &mat_tmp;
    
    CKParameter *src1 = GetSourceParameter(p1);
    void *d1 = src1 ? src1->GetReadDataPtr(TRUE) : NULL;
    _p = d1;
    const void *mat1 = d1 ? d1 : &mat_tmp;
    
    *(CKBOOL *)res->GetWriteDataPtr() = (memcmp(mat1, mat2, 64) == 0);
}

// Original symbol: ?CKBoolNotEqualMatrixMatrix@@YAXPAVCKContext@@PAVCKParameterOut@@PAVCKParameterIn@@2@Z
// opfunc_ea=0x24B47C40
// Compares two matrices for inequality using memcmp (64 bytes = 4x4 floats)
void CKBoolNotEqualMatrixMatrix(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKParameter *src2 = GetSourceParameter(p2);
    void *d2 = src2 ? src2->GetReadDataPtr(TRUE) : NULL;
    _p = d2;
    const void *mat2 = d2 ? d2 : &mat_tmp;
    
    CKParameter *src1 = GetSourceParameter(p1);
    void *d1 = src1 ? src1->GetReadDataPtr(TRUE) : NULL;
    _p = d1;
    const void *mat1 = d1 ? d1 : &mat_tmp;
    
    *(CKBOOL *)res->GetWriteDataPtr() = (memcmp(mat1, mat2, 64) != 0);
}


void CKInitializeOperationFunctions(CKContext *context)
{
    CKParameterManager *pm = context->GetParameterManager();
    pm->RegisterOperationFunction(CKOGUID_PER_SECOND, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, CKFloatPerSecondFloat);  // call_ea=0x24B5335A
    pm->RegisterOperationFunction(CKOGUID_OPPOSITE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, CKFloatOppositeFloat);  // call_ea=0x24B533BC
    pm->RegisterOperationFunction(CKOGUID_ABSOLUTE_VALUE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, CKFloatAbsoluteFloat);  // call_ea=0x24B53415
    pm->RegisterOperationFunction(CKOGUID_ADDITION, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, CKFloatAddFloatFloat);  // call_ea=0x24B5345D
    pm->RegisterOperationFunction(CKOGUID_SUBTRACTION, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, CKFloatSubtractFloatFloat);  // call_ea=0x24B534BD
    pm->RegisterOperationFunction(CKOGUID_MULTIPLICATION, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, CKFloatMultiplyFloatFloat);  // call_ea=0x24B53522
    pm->RegisterOperationFunction(CKOGUID_DIVISION, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, CKFloatDivideFloatFloat);  // call_ea=0x24B53572
    pm->RegisterOperationFunction(CKOGUID_MAX, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, CKFloatMaxFloatFloat);  // call_ea=0x24B535C2
    pm->RegisterOperationFunction(CKOGUID_MIN, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, CKFloatMinFloatFloat);  // call_ea=0x24B53612
    pm->RegisterOperationFunction(CKOGUID_RANDOM, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, CKFloatRandomFloatFloat);  // call_ea=0x24B53662
    pm->RegisterOperationFunction(CKOGUID_INVERSE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, CKFloatInverseFloat);  // call_ea=0x24B536BA
    pm->RegisterOperationFunction(CKOGUID_SINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, CKFloatSinusFloat);  // call_ea=0x24B53712
    pm->RegisterOperationFunction(CKOGUID_SQUARE_ROOT, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, CKFloatSqrtFloat);  // call_ea=0x24B5376A
    pm->RegisterOperationFunction(CKOGUID_COSINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, CKFloatCosinusFloat);  // call_ea=0x24B537C2
    pm->RegisterOperationFunction(CKOGUID_TANGENT, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, CKFloatTanFloat);  // call_ea=0x24B5381A
    pm->RegisterOperationFunction(CKOGUID_ARC_TANGENT_2, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_FLOAT, CKFloatArcTanFloat);  // call_ea=0x24B5386A
    pm->RegisterOperationFunction(CKOGUID_ARC_COSINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, CKFloatArcCosFloat);  // call_ea=0x24B538C2
    pm->RegisterOperationFunction(CKOGUID_ARC_SINE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, CKFloatArcSinFloat);  // call_ea=0x24B5391A
    pm->RegisterOperationFunction(CKOGUID_ADDITION, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_INT, CKFloatAddFloatInt);  // call_ea=0x24B53972
    pm->RegisterOperationFunction(CKOGUID_SUBTRACTION, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_INT, CKFloatSubtractFloatInt);  // call_ea=0x24B539CA
    pm->RegisterOperationFunction(CKOGUID_SUBTRACTION, CKPGUID_FLOAT, CKPGUID_INT, CKPGUID_FLOAT, CKFloatSubtractIntFloat);  // call_ea=0x24B53A22
    pm->RegisterOperationFunction(CKOGUID_MULTIPLICATION, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_INT, CKFloatMultiplyFloatInt);  // call_ea=0x24B53A7A
    pm->RegisterOperationFunction(CKOGUID_DIVISION, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_INT, CKFloatDivideFloatInt);  // call_ea=0x24B53AD2
    pm->RegisterOperationFunction(CKOGUID_DIVISION, CKPGUID_FLOAT, CKPGUID_INT, CKPGUID_FLOAT, CKFloatDivideIntFloat);  // call_ea=0x24B53B2A
    pm->RegisterOperationFunction(CKOGUID_MAX, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_INT, CKFloatMaxFloatInt);  // call_ea=0x24B53B82
    pm->RegisterOperationFunction(CKOGUID_MIN, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_INT, CKFloatMinFloatInt);  // call_ea=0x24B53BDA
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_FLOAT, CKPGUID_INT, CKPGUID_NONE, CKFloatSetInt);  // call_ea=0x24B53C3A
    pm->RegisterOperationFunction(CKOGUID_DEGRE_TO_RADIAN, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, CKFloatDegreToRadianFloat);  // call_ea=0x24B53C92
    pm->RegisterOperationFunction(CKOGUID_RADIAN_TO_DEGRE, CKPGUID_FLOAT, CKPGUID_FLOAT, CKPGUID_NONE, CKFloatRadianToDegreFloat);  // call_ea=0x24B53CEA
    pm->RegisterOperationFunction(CKOGUID_INVERSE, CKPGUID_FLOAT, CKPGUID_INT, CKPGUID_NONE, CKFloatInverseInt);  // call_ea=0x24B53D4A
    pm->RegisterOperationFunction(CKOGUID_GET_DISTANCE, CKPGUID_FLOAT, CKPGUID_3DENTITY, CKPGUID_3DENTITY, CKFloatGetDistance3dEntity);  // call_ea=0x24B53DAA
    pm->RegisterOperationFunction(CKOGUID_GET_X, CKPGUID_FLOAT, CKPGUID_3DENTITY, CKPGUID_NONE, CKFloatGetX3dEntity);  // call_ea=0x24B53E0A
    pm->RegisterOperationFunction(CKOGUID_GET_Y, CKPGUID_FLOAT, CKPGUID_3DENTITY, CKPGUID_NONE, CKFloatGetY3dentity);  // call_ea=0x24B53E6A
    pm->RegisterOperationFunction(CKOGUID_GET_Z, CKPGUID_FLOAT, CKPGUID_3DENTITY, CKPGUID_NONE, CKFloatGetZ3dEntity);  // call_ea=0x24B53ECA
    pm->RegisterOperationFunction(CKOGUID_GET_RADIUS, CKPGUID_FLOAT, CKPGUID_3DENTITY, CKPGUID_NONE, CKFloatGetRadius3dEntity);  // call_ea=0x24B53F2A
    pm->RegisterOperationFunction(CKOGUID_GET_X, CKPGUID_FLOAT, CKPGUID_VECTOR, CKPGUID_NONE, CKFloatGetXEuler);  // call_ea=0x24B53F8A
    pm->RegisterOperationFunction(CKOGUID_GET_Y, CKPGUID_FLOAT, CKPGUID_VECTOR, CKPGUID_NONE, CKFloatGetYEuler);  // call_ea=0x24B53FEA
    pm->RegisterOperationFunction(CKOGUID_GET_Z, CKPGUID_FLOAT, CKPGUID_VECTOR, CKPGUID_NONE, CKFloatGetZEuler);  // call_ea=0x24B5404A
    pm->RegisterOperationFunction(CKOGUID_GET_X, CKPGUID_FLOAT, CKPGUID_2DVECTOR, CKPGUID_NONE, CKFloatGetX2dVector);  // call_ea=0x24B540AA
    pm->RegisterOperationFunction(CKOGUID_GET_Y, CKPGUID_FLOAT, CKPGUID_2DVECTOR, CKPGUID_NONE, CKFloatGetY2dVector);  // call_ea=0x24B5410A
    pm->RegisterOperationFunction(CKOGUID_GET_X, CKPGUID_FLOAT, CKPGUID_EULERANGLES, CKPGUID_NONE, CKFloatGetXEuler);  // call_ea=0x24B5416A
    pm->RegisterOperationFunction(CKOGUID_GET_Y, CKPGUID_FLOAT, CKPGUID_EULERANGLES, CKPGUID_NONE, CKFloatGetYEuler);  // call_ea=0x24B541CA
    pm->RegisterOperationFunction(CKOGUID_GET_Z, CKPGUID_FLOAT, CKPGUID_EULERANGLES, CKPGUID_NONE, CKFloatGetZEuler);  // call_ea=0x24B5422A
    pm->RegisterOperationFunction(CKOGUID_GET_X, CKPGUID_FLOAT, CKPGUID_QUATERNION, CKPGUID_NONE, CKFloatGetXQuaternion);  // call_ea=0x24B5428A
    pm->RegisterOperationFunction(CKOGUID_GET_Y, CKPGUID_FLOAT, CKPGUID_QUATERNION, CKPGUID_NONE, CKFloatGetYQuaternion);  // call_ea=0x24B542EA
    pm->RegisterOperationFunction(CKOGUID_GET_Z, CKPGUID_FLOAT, CKPGUID_QUATERNION, CKPGUID_NONE, CKFloatGetZQuaternion);  // call_ea=0x24B5434A
    pm->RegisterOperationFunction(CKOGUID_GET_W, CKPGUID_FLOAT, CKPGUID_QUATERNION, CKPGUID_NONE, CKFloatGetWQuaternion);  // call_ea=0x24B543AA
    pm->RegisterOperationFunction(CKOGUID_GET_RED, CKPGUID_FLOAT, CKPGUID_COLOR, CKPGUID_NONE, CKFloatGetXQuaternion);  // call_ea=0x24B5440A
    pm->RegisterOperationFunction(CKOGUID_GET_GREEN, CKPGUID_FLOAT, CKPGUID_COLOR, CKPGUID_NONE, CKFloatGetYQuaternion);  // call_ea=0x24B5446A
    pm->RegisterOperationFunction(CKOGUID_GET_BLUE, CKPGUID_FLOAT, CKPGUID_COLOR, CKPGUID_NONE, CKFloatGetZQuaternion);  // call_ea=0x24B544CA
    pm->RegisterOperationFunction(CKOGUID_GET_ALPHA, CKPGUID_FLOAT, CKPGUID_COLOR, CKPGUID_NONE, CKFloatGetWQuaternion);  // call_ea=0x24B5452A
    pm->RegisterOperationFunction(CKOGUID_DOT_PRODUCT, CKPGUID_FLOAT, CKPGUID_VECTOR, CKPGUID_VECTOR, CKFloatDotProductVector);  // call_ea=0x24B5458A
    pm->RegisterOperationFunction(CKOGUID_GET_DISTANCE, CKPGUID_FLOAT, CKPGUID_VECTOR, CKPGUID_VECTOR, CKFloatGetDistanceVector);  // call_ea=0x24B545EA
    pm->RegisterOperationFunction(CKOGUID_GET_ANGLE, CKPGUID_FLOAT, CKPGUID_VECTOR, CKPGUID_VECTOR, CKFloatGetAngleVector);  // call_ea=0x24B5464A
    pm->RegisterOperationFunction(CKOGUID_GET_ANGLE, CKPGUID_FLOAT, CKPGUID_2DVECTOR, CKPGUID_2DVECTOR, CKFloatGetAngle2dVector2dVector);  // call_ea=0x24B546AA
    pm->RegisterOperationFunction(CKOGUID_DOT_PRODUCT, CKPGUID_FLOAT, CKPGUID_2DVECTOR, CKPGUID_2DVECTOR, CKFloatDotProduct2dVector);  // call_ea=0x24B5470A
    pm->RegisterOperationFunction(CKOGUID_GET_DISTANCE, CKPGUID_FLOAT, CKPGUID_2DVECTOR, CKPGUID_2DVECTOR, CKFloatGetDistance2dVector);  // call_ea=0x24B5476A
    pm->RegisterOperationFunction(CKOGUID_GET_DISTANCE, CKPGUID_FLOAT, CKPGUID_2DENTITY, CKPGUID_2DENTITY, CKFloatGetDistance2dEntity);  // call_ea=0x24B547CA
    pm->RegisterOperationFunction(CKOGUID_GET_RANGE, CKPGUID_FLOAT, CKPGUID_LIGHT, CKPGUID_NONE, CKFloatGetRangeLight);  // call_ea=0x24B5482A
    pm->RegisterOperationFunction(CKOGUID_GET_FIELD_OF_VIEW, CKPGUID_FLOAT, CKPGUID_CAMERA, CKPGUID_NONE, CKFloatGetFovCamera);  // call_ea=0x24B5488A
    pm->RegisterOperationFunction(CKOGUID_GET_NEAR_CLIP, CKPGUID_FLOAT, CKPGUID_CAMERA, CKPGUID_NONE, CKFloatGetLengthCurve);  // call_ea=0x24B548EA
    pm->RegisterOperationFunction(CKOGUID_GET_FAR_CLIP, CKPGUID_FLOAT, CKPGUID_CAMERA, CKPGUID_NONE, CKFloatGetBackPlaneCamera);  // call_ea=0x24B5494A
    pm->RegisterOperationFunction(CKOGUID_GET_ORTHOGRAPHIC_ZOOM, CKPGUID_FLOAT, CKPGUID_CAMERA, CKPGUID_NONE, CKFloatGetZoomCamera);  // call_ea=0x24B549AA
    pm->RegisterOperationFunction(CKOGUID_GET_LENGTH, CKPGUID_FLOAT, CKPGUID_CURVE, CKPGUID_CURVEPOINT, CKFloatGetLengthCurveCurvePoint);  // call_ea=0x24B54A0A
    pm->RegisterOperationFunction(CKOGUID_GET_LENGTH, CKPGUID_FLOAT, CKPGUID_CURVE, CKPGUID_NONE, CKFloatGetLengthCurve);  // call_ea=0x24B54A6A
    pm->RegisterOperationFunction(CKOGUID_GET_LENGTH, CKPGUID_FLOAT, CKPGUID_2DCURVE, CKPGUID_NONE, CKFloatGetLength2dCurve);  // call_ea=0x24B54ACA
    pm->RegisterOperationFunction(CKOGUID_GET_Y, CKPGUID_FLOAT, CKPGUID_2DCURVE, CKPGUID_FLOAT, CKFloatGetY2dCurveFloat);  // call_ea=0x24B54B22
    pm->RegisterOperationFunction(CKOGUID_GET_LENGTH, CKPGUID_FLOAT, CKPGUID_ANIMATION, CKPGUID_NONE, CKFloatGetLengthAnimation);  // call_ea=0x24B54B82
    pm->RegisterOperationFunction(CKOGUID_GET_MAGNITUDE, CKPGUID_FLOAT, CKPGUID_VECTOR, CKPGUID_NONE, CKFloatGetMagnitudeVector);  // call_ea=0x24B54BE2
    pm->RegisterOperationFunction(CKOGUID_GET_MAGNITUDE, CKPGUID_FLOAT, CKPGUID_2DVECTOR, CKPGUID_NONE, CKFloatGetMagnitude2dVector);  // call_ea=0x24B54C42
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_FLOAT, CKPGUID_STRING, CKPGUID_NONE, CKGenericSetString);  // call_ea=0x24B54CA2
    pm->RegisterOperationFunction(CKOGUID_GET_VERTEX_WEIGHT, CKPGUID_FLOAT, CKPGUID_MESH, CKPGUID_INT, CKFloatGetVertexWeightMeshInt);  // call_ea=0x24B54D02
    pm->RegisterOperationFunction(CKOGUID_ABSOLUTE_VALUE, CKPGUID_INT, CKPGUID_INT, CKPGUID_NONE, CKIntAbsoluteInt);  // call_ea=0x24B54D6A
    pm->RegisterOperationFunction(CKOGUID_OPPOSITE, CKPGUID_INT, CKPGUID_INT, CKPGUID_NONE, CKIntOppositeInt);  // call_ea=0x24B54DD2
    pm->RegisterOperationFunction(CKOGUID_ADDITION, CKPGUID_INT, CKPGUID_INT, CKPGUID_INT, CKIntAddIntInt);  // call_ea=0x24B54E3A
    pm->RegisterOperationFunction(CKOGUID_AND, CKPGUID_INT, CKPGUID_INT, CKPGUID_INT, CKIntAndIntInt);  // call_ea=0x24B54EA2
    pm->RegisterOperationFunction(CKOGUID_OR, CKPGUID_INT, CKPGUID_INT, CKPGUID_INT, CKIntOrIntInt);  // call_ea=0x24B54F0A
    pm->RegisterOperationFunction(CKOGUID_XOR, CKPGUID_INT, CKPGUID_INT, CKPGUID_INT, CKIntXorIntInt);  // call_ea=0x24B54F72
    pm->RegisterOperationFunction(CKOGUID_SUBTRACTION, CKPGUID_INT, CKPGUID_INT, CKPGUID_INT, CKIntSubtractIntInt);  // call_ea=0x24B54FDA
    pm->RegisterOperationFunction(CKOGUID_MULTIPLICATION, CKPGUID_INT, CKPGUID_INT, CKPGUID_INT, CKIntMultiplyIntInt);  // call_ea=0x24B55042
    pm->RegisterOperationFunction(CKOGUID_DIVISION, CKPGUID_INT, CKPGUID_INT, CKPGUID_INT, CKIntDivideIntInt);  // call_ea=0x24B550AA
    pm->RegisterOperationFunction(CKOGUID_MAX, CKPGUID_INT, CKPGUID_INT, CKPGUID_INT, CKIntMaxIntInt);  // call_ea=0x24B55112
    pm->RegisterOperationFunction(CKOGUID_MIN, CKPGUID_INT, CKPGUID_INT, CKPGUID_INT, CKIntMinIntInt);  // call_ea=0x24B5517A
    pm->RegisterOperationFunction(CKOGUID_RANDOM, CKPGUID_INT, CKPGUID_INT, CKPGUID_INT, CKIntRandomIntInt);  // call_ea=0x24B551E2
    pm->RegisterOperationFunction(CKOGUID_ADDITION, CKPGUID_INT, CKPGUID_INT, CKPGUID_FLOAT, CKIntAddIntFloat);  // call_ea=0x24B55242
    pm->RegisterOperationFunction(CKOGUID_SUBTRACTION, CKPGUID_INT, CKPGUID_INT, CKPGUID_FLOAT, CKIntSubtractIntFloat);  // call_ea=0x24B552A2
    pm->RegisterOperationFunction(CKOGUID_SUBTRACTION, CKPGUID_INT, CKPGUID_FLOAT, CKPGUID_INT, CKIntSubtractFloatInt);  // call_ea=0x24B55302
    pm->RegisterOperationFunction(CKOGUID_MULTIPLICATION, CKPGUID_INT, CKPGUID_INT, CKPGUID_FLOAT, CKIntMultiplyIntFloat);  // call_ea=0x24B55362
    pm->RegisterOperationFunction(CKOGUID_DIVISION, CKPGUID_INT, CKPGUID_INT, CKPGUID_FLOAT, CKIntDivideIntFloat);  // call_ea=0x24B553C2
    pm->RegisterOperationFunction(CKOGUID_DIVISION, CKPGUID_INT, CKPGUID_FLOAT, CKPGUID_INT, CKIntDivideFloatInt);  // call_ea=0x24B55422
    pm->RegisterOperationFunction(CKOGUID_MAX, CKPGUID_INT, CKPGUID_INT, CKPGUID_FLOAT, CKIntMaxIntFloat);  // call_ea=0x24B55482
    pm->RegisterOperationFunction(CKOGUID_MIN, CKPGUID_INT, CKPGUID_INT, CKPGUID_FLOAT, CKIntMinIntFloat);  // call_ea=0x24B554E2
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_INT, CKPGUID_FLOAT, CKPGUID_NONE, CKIntSetFloat);  // call_ea=0x24B55542
    pm->RegisterOperationFunction(CKOGUID_INVERSE, CKPGUID_INT, CKPGUID_FLOAT, CKPGUID_NONE, CKIntInverseFloat);  // call_ea=0x24B555A2
    pm->RegisterOperationFunction(CKOGUID_GET_LENGTH, CKPGUID_INT, CKPGUID_STRING, CKPGUID_NONE, CKIntGetLengthString);  // call_ea=0x24B5560A
    pm->RegisterOperationFunction(CKOGUID_GET_WIDTH, CKPGUID_INT, CKPGUID_NONE, CKPGUID_NONE, IntGetWidthNoneNone);  // call_ea=0x24B55672
    pm->RegisterOperationFunction(CKOGUID_GET_HEIGHT, CKPGUID_INT, CKPGUID_NONE, CKPGUID_NONE, IntGetHeightNoneNone);  // call_ea=0x24B556DA
    pm->RegisterOperationFunction(CKOGUID_MODULO, CKPGUID_INT, CKPGUID_INT, CKPGUID_INT, CKIntModuloIntInt);  // call_ea=0x24B55742
    pm->RegisterOperationFunction(CKOGUID_GET_WIDTH, CKPGUID_INT, CKPGUID_TEXTURE, CKPGUID_NONE, CKIntGetWidthTexture);  // call_ea=0x24B557AA
    pm->RegisterOperationFunction(CKOGUID_GET_HEIGHT, CKPGUID_INT, CKPGUID_TEXTURE, CKPGUID_NONE, CKIntGetHeightTexture);  // call_ea=0x24B55812
    pm->RegisterOperationFunction(CKOGUID_GET_SLOT_COUNT, CKPGUID_INT, CKPGUID_TEXTURE, CKPGUID_NONE, CKIntGetSlotCountTexture);  // call_ea=0x24B5587A
    pm->RegisterOperationFunction(CKOGUID_GET_CURRENT, CKPGUID_INT, CKPGUID_TEXTURE, CKPGUID_NONE, CKIntGetCurrentTexture);  // call_ea=0x24B558E2
    pm->RegisterOperationFunction(CKOGUID_GET_WIDTH, CKPGUID_INT, CKPGUID_2DENTITY, CKPGUID_NONE, CKIntGetWidth2dEntity);  // call_ea=0x24B5594A
    pm->RegisterOperationFunction(CKOGUID_GET_HEIGHT, CKPGUID_INT, CKPGUID_2DENTITY, CKPGUID_NONE, CKIntGetHeight2dEntity);  // call_ea=0x24B559B2
    pm->RegisterOperationFunction(CKOGUID_GET_TYPE, CKPGUID_CLASSID, CKPGUID_OBJECT, CKPGUID_NONE, CKIntGetTypeObject);  // call_ea=0x24B55A1A
    pm->RegisterOperationFunction(CKOGUID_GET_COUNT, CKPGUID_INT, CKPGUID_OBJECTARRAY, CKPGUID_NONE, CKIntGetCountObjectArray);  // call_ea=0x24B55A82
    pm->RegisterOperationFunction(CKOGUID_GET_COUNT, CKPGUID_INT, CKPGUID_DATAARRAY, CKPGUID_NONE, CKIntGetRowCountDataArray);  // call_ea=0x24B55AEA
    pm->RegisterOperationFunction(CKOGUID_GET_COUNT, CKPGUID_INT, CKPGUID_CURVE, CKPGUID_NONE, CKIntGetCountCurve);  // call_ea=0x24B55B52
    pm->RegisterOperationFunction(CKOGUID_GET_COLUMN_COUNT, CKPGUID_INT, CKPGUID_DATAARRAY, CKPGUID_NONE, CKIntGetColumnCountDataArray);  // call_ea=0x24B55BBA
    pm->RegisterOperationFunction(CKOGUID_GET_ROW_COUNT, CKPGUID_INT, CKPGUID_DATAARRAY, CKPGUID_NONE, CKIntGetRowCountDataArray);  // call_ea=0x24B55C22
    pm->RegisterOperationFunction(CKOGUID_GET_SLOT_COUNT, CKPGUID_INT, CKPGUID_SPRITE, CKPGUID_NONE, CKIntGetSlotCountSprite);  // call_ea=0x24B55C8A
    pm->RegisterOperationFunction(CKOGUID_GET_CURRENT, CKPGUID_INT, CKPGUID_SPRITE, CKPGUID_NONE, CKIntGetCurrentSprite);  // call_ea=0x24B55CF2
    pm->RegisterOperationFunction(CKOGUID_GET_SCRIPT_COUNT, CKPGUID_INT, CKPGUID_BEOBJECT, CKPGUID_NONE, CKIntGetScriptCountBeObject);  // call_ea=0x24B55D5A
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_INT, CKPGUID_STRING, CKPGUID_NONE, CKGenericSetString);  // call_ea=0x24B55DC2
    pm->RegisterOperationFunction(CKOGUID_GET_VERTEX_COUNT, CKPGUID_INT, CKPGUID_MESH, CKPGUID_NONE, CKIntGetVertexCountMesh);  // call_ea=0x24B55E2A
    pm->RegisterOperationFunction(CKOGUID_GET_FACE_COUNT, CKPGUID_INT, CKPGUID_MESH, CKPGUID_NONE, CKIntGetFaceCountMesh);  // call_ea=0x24B55E92
    pm->RegisterOperationFunction(CKOGUID_GET_PM_RENDERED_VERTICES_COUNT, CKPGUID_INT, CKPGUID_MESH, CKPGUID_NONE, CKIntGetRenderedProgressiveMeshVerticesCount);  // call_ea=0x24B55EFA
    pm->RegisterOperationFunction(CKOGUID_GET_MATERIAL_COUNT, CKPGUID_INT, CKPGUID_MESH, CKPGUID_NONE, CKIntGetMaterialCountMesh);  // call_ea=0x24B55F62
    pm->RegisterOperationFunction(CKOGUID_GET_CHANNEL_COUNT, CKPGUID_INT, CKPGUID_MESH, CKPGUID_NONE, CKIntGetChannelCountMesh);  // call_ea=0x24B55FCA
    pm->RegisterOperationFunction(CKOGUID_GET_CHANNEL_BY_MATERIAL, CKPGUID_INT, CKPGUID_MESH, CKPGUID_MATERIAL, CKIntGetChannelByMaterialMeshMaterial);  // call_ea=0x24B56032
    pm->RegisterOperationFunction(CKOGUID_XOR, CKPGUID_BOOL, CKPGUID_BOOL, CKPGUID_BOOL, CKBoolXorBoolBool);  // call_ea=0x24B5609A
    pm->RegisterOperationFunction(CKOGUID_OR, CKPGUID_BOOL, CKPGUID_BOOL, CKPGUID_BOOL, CKBoolOrBoolBool);  // call_ea=0x24B56102
    pm->RegisterOperationFunction(CKOGUID_AND, CKPGUID_BOOL, CKPGUID_BOOL, CKPGUID_BOOL, CKBoolAndBoolBool);  // call_ea=0x24B5616A
    pm->RegisterOperationFunction(CKOGUID_NOT, CKPGUID_BOOL, CKPGUID_BOOL, CKPGUID_NONE, CKBoolNotBool);  // call_ea=0x24B561D2
    pm->RegisterOperationFunction(CKOGUID_OPPOSITE, CKPGUID_BOOL, CKPGUID_BOOL, CKPGUID_NONE, CKBoolNotBool);  // call_ea=0x24B5623A
    pm->RegisterOperationFunction(CKOGUID_RANDOM, CKPGUID_BOOL, CKPGUID_NONE, CKPGUID_NONE, CKBoolRandom);  // call_ea=0x24B562A2
    pm->RegisterOperationFunction(CKOGUID_INFERIOR, CKPGUID_BOOL, CKPGUID_FLOAT, CKPGUID_FLOAT, CKBoolInfFloatFloat);  // call_ea=0x24B562FA
    pm->RegisterOperationFunction(CKOGUID_SUPERIOR, CKPGUID_BOOL, CKPGUID_FLOAT, CKPGUID_FLOAT, CKBoolSupFloatFloat);  // call_ea=0x24B56352
    pm->RegisterOperationFunction(CKOGUID_INFERIOR_OR_EQUAL, CKPGUID_BOOL, CKPGUID_FLOAT, CKPGUID_FLOAT, CKBoolInfEqualFloatFloat);  // call_ea=0x24B563AA
    pm->RegisterOperationFunction(CKOGUID_SUPERIOR_OR_EQUAL, CKPGUID_BOOL, CKPGUID_FLOAT, CKPGUID_FLOAT, CKBoolSupEqualFloatFloat);  // call_ea=0x24B56402
    pm->RegisterOperationFunction(CKOGUID_INFERIOR, CKPGUID_BOOL, CKPGUID_FLOAT, CKPGUID_INT, CKBoolInfFloatInt);  // call_ea=0x24B56462
    pm->RegisterOperationFunction(CKOGUID_SUPERIOR, CKPGUID_BOOL, CKPGUID_FLOAT, CKPGUID_INT, CKBoolSupFloatInt);  // call_ea=0x24B564C2
    pm->RegisterOperationFunction(CKOGUID_INFERIOR_OR_EQUAL, CKPGUID_BOOL, CKPGUID_FLOAT, CKPGUID_INT, CKBoolInfEqualFloatInt);  // call_ea=0x24B56522
    pm->RegisterOperationFunction(CKOGUID_SUPERIOR_OR_EQUAL, CKPGUID_BOOL, CKPGUID_FLOAT, CKPGUID_INT, CKBoolSupEqualFloatInt);  // call_ea=0x24B56582
    pm->RegisterOperationFunction(CKOGUID_INFERIOR, CKPGUID_BOOL, CKPGUID_INT, CKPGUID_FLOAT, CKBoolInfIntFloat);  // call_ea=0x24B565E2
    pm->RegisterOperationFunction(CKOGUID_SUPERIOR, CKPGUID_BOOL, CKPGUID_INT, CKPGUID_FLOAT, CKBoolSupIntFloat);  // call_ea=0x24B56642
    pm->RegisterOperationFunction(CKOGUID_INFERIOR_OR_EQUAL, CKPGUID_BOOL, CKPGUID_INT, CKPGUID_FLOAT, CKBoolInfEqualIntFloat);  // call_ea=0x24B566A2
    pm->RegisterOperationFunction(CKOGUID_SUPERIOR_OR_EQUAL, CKPGUID_BOOL, CKPGUID_INT, CKPGUID_FLOAT, CKBoolSupEqualIntFloat);  // call_ea=0x24B56702
    pm->RegisterOperationFunction(CKOGUID_INFERIOR, CKPGUID_BOOL, CKPGUID_INT, CKPGUID_INT, CKBoolInfIntInt);  // call_ea=0x24B5676A
    pm->RegisterOperationFunction(CKOGUID_SUPERIOR, CKPGUID_BOOL, CKPGUID_INT, CKPGUID_INT, CKBoolSupIntInt);  // call_ea=0x24B567D2
    pm->RegisterOperationFunction(CKOGUID_INFERIOR_OR_EQUAL, CKPGUID_BOOL, CKPGUID_INT, CKPGUID_INT, CKBoolInfEqualIntInt);  // call_ea=0x24B5683A
    pm->RegisterOperationFunction(CKOGUID_SUPERIOR_OR_EQUAL, CKPGUID_BOOL, CKPGUID_INT, CKPGUID_INT, CKBoolSupEqualIntInt);  // call_ea=0x24B568A2
    pm->RegisterOperationFunction(CKOGUID_IS_DERIVED_FROM, CKPGUID_BOOL, CKPGUID_OBJECT, CKPGUID_OBJECT, CKBoolDerivedFromIdId);  // call_ea=0x24B5690A
    pm->RegisterOperationFunction(CKOGUID_IS_COLLISION, CKPGUID_BOOL, CKPGUID_BOX, CKPGUID_VECTOR, CKBoolCollisionBoxVector);  // call_ea=0x24B56972
    pm->RegisterOperationFunction(CKOGUID_IS_COLLISION, CKPGUID_BOOL, CKPGUID_BOX, CKPGUID_BOX, CKBoolCollisionBoxBox);  // call_ea=0x24B569DA
    pm->RegisterOperationFunction(CKOGUID_IS_COLLISION, CKPGUID_BOOL, CKPGUID_BOX, CKPGUID_3DENTITY, CKBoolCollisionBox3dEntity);  // call_ea=0x24B56A42
    pm->RegisterOperationFunction(CKOGUID_IS_CHILD_OF, CKPGUID_BOOL, CKPGUID_3DENTITY, CKPGUID_3DENTITY, CKBoolIsChildOf3dEntity3dEntity);  // call_ea=0x24B56AAA
    pm->RegisterOperationFunction(CKOGUID_IS_BODY_PART_OF, CKPGUID_BOOL, CKPGUID_BODYPART, CKPGUID_CHARACTER, CKBoolIsBodyPartOfBodyPartCharacter);  // call_ea=0x24B56B12
    pm->RegisterOperationFunction(CKOGUID_IS_VISIBLE, CKPGUID_BOOL, CKPGUID_3DENTITY, CKPGUID_NONE, CKBoolIsVisible2dEntity);  // call_ea=0x24B56B7A
    pm->RegisterOperationFunction(CKOGUID_IS_COLLISION, CKPGUID_BOOL, CKPGUID_3DENTITY, CKPGUID_3DENTITY, CKBoolCollision3dEntity3dEntity);  // call_ea=0x24B56BE2
    pm->RegisterOperationFunction(CKOGUID_IS_VISIBLE, CKPGUID_BOOL, CKPGUID_2DENTITY, CKPGUID_NONE, CKBoolIsVisible2dEntity);  // call_ea=0x24B56C4A
    pm->RegisterOperationFunction(CKOGUID_IS_IN, CKPGUID_BOOL, CKPGUID_VECTOR, CKPGUID_3DENTITY, CKBoolIsVectorInBboxVector3dEntity);  // call_ea=0x24B56CB2
    pm->RegisterOperationFunction(CKOGUID_CONTAIN_STRING, CKPGUID_BOOL, CKPGUID_STRING, CKPGUID_STRING, CKBoolContainStringString);  // call_ea=0x24B56D1A
    pm->RegisterOperationFunction(CKOGUID_IS_ACTIVE, CKPGUID_BOOL, CKPGUID_SCRIPT, CKPGUID_NONE, CKBoolIsActiveScript);  // call_ea=0x24B56D82
    pm->RegisterOperationFunction(CKOGUID_IS_ACTIVE, CKPGUID_BOOL, CKPGUID_BEOBJECT, CKPGUID_NONE, CKBoolIsActiveBeObject);  // call_ea=0x24B56DEA
    pm->RegisterOperationFunction(CKOGUID_GET_SCALE, CKPGUID_VECTOR, CKPGUID_3DENTITY, CKPGUID_NONE, CKVectorGetScale);  // call_ea=0x24B56E52
    pm->RegisterOperationFunction(CKOGUID_PER_SECOND, CKPGUID_VECTOR, CKPGUID_VECTOR, CKPGUID_NONE, CKVectorPerSecondVector);  // call_ea=0x24B56EBA
    pm->RegisterOperationFunction(CKOGUID_OPPOSITE, CKPGUID_VECTOR, CKPGUID_VECTOR, CKPGUID_NONE, CKVectorOppositeVector);  // call_ea=0x24B56F22
    pm->RegisterOperationFunction(CKOGUID_ADDITION, CKPGUID_VECTOR, CKPGUID_VECTOR, CKPGUID_VECTOR, CKVectorAddVectorVector);  // call_ea=0x24B56F8A
    pm->RegisterOperationFunction(CKOGUID_SUBTRACTION, CKPGUID_VECTOR, CKPGUID_VECTOR, CKPGUID_VECTOR, CKVectorSubtractVectorVector);  // call_ea=0x24B56FF2
    pm->RegisterOperationFunction(CKOGUID_DIVISION, CKPGUID_VECTOR, CKPGUID_VECTOR, CKPGUID_VECTOR, CKVectorDivideVectorVector);  // call_ea=0x24B5705A
    pm->RegisterOperationFunction(CKOGUID_MULTIPLICATION, CKPGUID_VECTOR, CKPGUID_VECTOR, CKPGUID_VECTOR, CKVectorMultiplyVectorVector);  // call_ea=0x24B570C2
    pm->RegisterOperationFunction(CKOGUID_CROSS_PRODUCT, CKPGUID_VECTOR, CKPGUID_VECTOR, CKPGUID_VECTOR, CKVectorCrossProductVectorVector);  // call_ea=0x24B5712A
    pm->RegisterOperationFunction(CKOGUID_MAX, CKPGUID_VECTOR, CKPGUID_VECTOR, CKPGUID_VECTOR, CKVectorMaxVectorVector);  // call_ea=0x24B57192
    pm->RegisterOperationFunction(CKOGUID_MIN, CKPGUID_VECTOR, CKPGUID_VECTOR, CKPGUID_VECTOR, CKVectorMinVectorVector);  // call_ea=0x24B571FA
    pm->RegisterOperationFunction(CKOGUID_INVERSE, CKPGUID_VECTOR, CKPGUID_VECTOR, CKPGUID_NONE, CKVectorInverseVector);  // call_ea=0x24B57262
    pm->RegisterOperationFunction(CKOGUID_RANDOM, CKPGUID_VECTOR, CKPGUID_NONE, CKPGUID_NONE, CKVectorRandom);  // call_ea=0x24B572CA
    pm->RegisterOperationFunction(CKOGUID_MULTIPLICATION, CKPGUID_VECTOR, CKPGUID_VECTOR, CKPGUID_FLOAT, CKVectorMultiplyVectorFloat);  // call_ea=0x24B5732A
    pm->RegisterOperationFunction(CKOGUID_SPHERIC_TO_CARTESIAN, CKPGUID_VECTOR, CKPGUID_FLOAT, CKPGUID_FLOAT, CKVectorSphericToCartFloatFloat);  // call_ea=0x24B57382
    pm->RegisterOperationFunction(CKOGUID_DIVISION, CKPGUID_VECTOR, CKPGUID_VECTOR, CKPGUID_FLOAT, CKVectorDivideVectorFloat);  // call_ea=0x24B573E2
    pm->RegisterOperationFunction(CKOGUID_MULTIPLICATION, CKPGUID_VECTOR, CKPGUID_VECTOR, CKPGUID_MATRIX, CKVectorMultiplyVectorMatrix);  // call_ea=0x24B5744A
    pm->RegisterOperationFunction(CKOGUID_GET_DIR, CKPGUID_VECTOR, CKPGUID_3DENTITY, CKPGUID_NONE, CKVectorGetDir3dEntity);  // call_ea=0x24B574B2
    pm->RegisterOperationFunction(CKOGUID_GET_UP, CKPGUID_VECTOR, CKPGUID_3DENTITY, CKPGUID_NONE, CKVectorGetUp3dEntity);  // call_ea=0x24B5751A
    pm->RegisterOperationFunction(CKOGUID_GET_RIGHT, CKPGUID_VECTOR, CKPGUID_3DENTITY, CKPGUID_NONE, CKVectorGetRight3dEntity);  // call_ea=0x24B57582
    pm->RegisterOperationFunction(CKOGUID_TRANSFORM, CKPGUID_VECTOR, CKPGUID_VECTOR, CKPGUID_3DENTITY, CKVectorTransformVector3dEntity);  // call_ea=0x24B575EA
    pm->RegisterOperationFunction(CKOGUID_TRANSFORM, CKPGUID_VECTOR, CKPGUID_2DVECTOR, CKPGUID_FLOAT, CKVectorTransform2dVectorFloat);  // call_ea=0x24B5764A
    pm->RegisterOperationFunction(CKOGUID_TRANSFORM_VECTOR, CKPGUID_VECTOR, CKPGUID_VECTOR, CKPGUID_3DENTITY, CKVectorTransformVectorVector3dEntity);  // call_ea=0x24B576B2
    pm->RegisterOperationFunction(CKOGUID_INVERSE_TRANSFORM, CKPGUID_VECTOR, CKPGUID_VECTOR, CKPGUID_3DENTITY, CKVectorInverseTransformVector3dEntity);  // call_ea=0x24B5771A
    pm->RegisterOperationFunction(CKOGUID_INVERSE_TRANSFORM_VECTOR, CKPGUID_VECTOR, CKPGUID_VECTOR, CKPGUID_3DENTITY, CKVectorInverseTransformVectorVector3dEntity);  // call_ea=0x24B57782
    pm->RegisterOperationFunction(CKOGUID_GET_POSITION, CKPGUID_VECTOR, CKPGUID_3DENTITY, CKPGUID_3DENTITY, CKVectorGetPosition3dEntity3dEntity);  // call_ea=0x24B577EA
    pm->RegisterOperationFunction(CKOGUID_GET_POSITION, CKPGUID_VECTOR, CKPGUID_3DENTITY, CKPGUID_NONE, CKVectorGetPosition3dEntity3dEntity);  // call_ea=0x24B57852
    pm->RegisterOperationFunction(CKOGUID_GET_DISTANCE, CKPGUID_VECTOR, CKPGUID_3DENTITY, CKPGUID_3DENTITY, CKVectorGetDistance3dEntity3dEntity);  // call_ea=0x24B578BA
    pm->RegisterOperationFunction(CKOGUID_GET_GEOMETRIC_CENTER, CKPGUID_VECTOR, CKPGUID_3DENTITY, CKPGUID_NONE, CKVectorGetCenter3dEntity);  // call_ea=0x24B57922
    pm->RegisterOperationFunction(CKOGUID_DIVISION, CKPGUID_QUATERNION, CKPGUID_QUATERNION, CKPGUID_QUATERNION, CKQuaternionDivideQuaternionQuaternion);  // call_ea=0x24B5798A
    pm->RegisterOperationFunction(CKOGUID_MULTIPLICATION, CKPGUID_QUATERNION, CKPGUID_QUATERNION, CKPGUID_QUATERNION, CKQuaternionMultiplyQuaternionQuaternion);  // call_ea=0x24B579F2
    pm->RegisterOperationFunction(CKOGUID_GET_X, CKPGUID_VECTOR, CKPGUID_MATRIX, CKPGUID_NONE, CKVectorGetXMatrix);  // call_ea=0x24B57A5A
    pm->RegisterOperationFunction(CKOGUID_GET_Y, CKPGUID_VECTOR, CKPGUID_MATRIX, CKPGUID_NONE, CKVectorGetYMatrix);  // call_ea=0x24B57AC2
    pm->RegisterOperationFunction(CKOGUID_GET_Z, CKPGUID_VECTOR, CKPGUID_MATRIX, CKPGUID_NONE, CKVectorGetZMatrix);  // call_ea=0x24B57B2A
    pm->RegisterOperationFunction(CKOGUID_GET_POSITION, CKPGUID_VECTOR, CKPGUID_MATRIX, CKPGUID_NONE, CKVectorGetPosMatrix);  // call_ea=0x24B57B92
    pm->RegisterOperationFunction(CKOGUID_GET_SCALE, CKPGUID_VECTOR, CKPGUID_MATRIX, CKPGUID_NONE, CKVectorGetScaleMatrix);  // call_ea=0x24B57BFA
    pm->RegisterOperationFunction(CKOGUID_GET_GEOMETRIC_CENTER, CKPGUID_VECTOR, CKPGUID_BOX, CKPGUID_NONE, CKVectorGetCenterBox);  // call_ea=0x24B57C62
    pm->RegisterOperationFunction(CKOGUID_MIN, CKPGUID_VECTOR, CKPGUID_BOX, CKPGUID_NONE, CKVectorGetMinBox);  // call_ea=0x24B57CCA
    pm->RegisterOperationFunction(CKOGUID_MAX, CKPGUID_VECTOR, CKPGUID_BOX, CKPGUID_NONE, CKVectorGetMaxBox);  // call_ea=0x24B57D32
    pm->RegisterOperationFunction(CKOGUID_GET_SCALE, CKPGUID_VECTOR, CKPGUID_BOX, CKPGUID_NONE, CKVectorGetScaleBox);  // call_ea=0x24B57D9A
    pm->RegisterOperationFunction(CKOGUID_GET_CURVE_POSITION, CKPGUID_VECTOR, CKPGUID_FLOAT, CKPGUID_CURVE, CKVectorGetCurvePosFloatCurve);  // call_ea=0x24B57DFA
    pm->RegisterOperationFunction(CKOGUID_GET_CURVE_TANGENT, CKPGUID_VECTOR, CKPGUID_FLOAT, CKPGUID_CURVE, CKVectorGetCurveTangentFloatCurve);  // call_ea=0x24B57E5A
    pm->RegisterOperationFunction(CKOGUID_GET_IN_TANGENT, CKPGUID_VECTOR, CKPGUID_CURVEPOINT, CKPGUID_NONE, CKVectorGetInTangentCurvePoint);  // call_ea=0x24B57EC2
    pm->RegisterOperationFunction(CKOGUID_GET_OUT_TANGENT, CKPGUID_VECTOR, CKPGUID_CURVEPOINT, CKPGUID_NONE, CKVectorGetOutTangentCurvePoint);  // call_ea=0x24B57F2A
    pm->RegisterOperationFunction(CKOGUID_NORMALIZE, CKPGUID_VECTOR, CKPGUID_VECTOR, CKPGUID_NONE, CKVectorNormalizeVector);  // call_ea=0x24B57F92
    pm->RegisterOperationFunction(CKOGUID_REFLECT, CKPGUID_VECTOR, CKPGUID_VECTOR, CKPGUID_VECTOR, CKVectorReflectVectorVector);  // call_ea=0x24B57FFA
    pm->RegisterOperationFunction(CKOGUID_SYMMETRY, CKPGUID_VECTOR, CKPGUID_VECTOR, CKPGUID_VECTOR, CKVectorSymmetryVectorVector);  // call_ea=0x24B58062
    pm->RegisterOperationFunction(CKOGUID_GET_VERTEX_NORMAL, CKPGUID_VECTOR, CKPGUID_MESH, CKPGUID_INT, CKVectorGetVertexNormalMeshInt);  // call_ea=0x24B580CA
    pm->RegisterOperationFunction(CKOGUID_GET_VERTEX_POSITION, CKPGUID_VECTOR, CKPGUID_MESH, CKPGUID_INT, CKVectorGetVertexPositionMeshInt);  // call_ea=0x24B58132
    pm->RegisterOperationFunction(CKOGUID_GET_FACE_NORMAL, CKPGUID_VECTOR, CKPGUID_MESH, CKPGUID_INT, CKVectorGetFaceNormalMeshInt);  // call_ea=0x24B5819A
    pm->RegisterOperationFunction(CKOGUID_GET_FACE_VERTEX_INDICES, CKPGUID_VECTOR, CKPGUID_MESH, CKPGUID_INT, CKVectorGetFaceVertexIndexPositionMeshInt);  // call_ea=0x24B58202
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_VECTOR, CKPGUID_STRING, CKPGUID_NONE, CKGenericSetString);  // call_ea=0x24B5826A
    pm->RegisterOperationFunction(CKOGUID_SET_X, CKPGUID_VECTOR, CKPGUID_VECTOR, CKPGUID_FLOAT, CKVectorSetXVectorFloat);  // call_ea=0x24B582CA
    pm->RegisterOperationFunction(CKOGUID_SET_Y, CKPGUID_VECTOR, CKPGUID_VECTOR, CKPGUID_FLOAT, CKVectorSetYVectorFloat);  // call_ea=0x24B5832A
    pm->RegisterOperationFunction(CKOGUID_SET_Z, CKPGUID_VECTOR, CKPGUID_VECTOR, CKPGUID_FLOAT, CKVectorSetZVectorFloat);  // call_ea=0x24B5838A
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_VECTOR, CKPGUID_VECTOR, CKPGUID_2DVECTOR, CKVectorSetVector2DVector);  // call_ea=0x24B583F2
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_QUATERNION, CKPGUID_MATRIX, CKPGUID_NONE, CKQuaternionSetMatrix);  // call_ea=0x24B5845A
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_QUATERNION, CKPGUID_EULERANGLES, CKPGUID_NONE, CKQuaternionSetEuler);  // call_ea=0x24B584C2
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_QUATERNION, CKPGUID_STRING, CKPGUID_NONE, CKGenericSetString);  // call_ea=0x24B5852A
    pm->RegisterOperationFunction(CKOGUID_SET_X, CKPGUID_QUATERNION, CKPGUID_QUATERNION, CKPGUID_FLOAT, CKColorSetRedColorFloat2);  // call_ea=0x24B5858A
    pm->RegisterOperationFunction(CKOGUID_SET_Y, CKPGUID_QUATERNION, CKPGUID_QUATERNION, CKPGUID_FLOAT, CKColorSetGreenColorFloat2);  // call_ea=0x24B585EA
    pm->RegisterOperationFunction(CKOGUID_SET_Z, CKPGUID_QUATERNION, CKPGUID_QUATERNION, CKPGUID_FLOAT, CKColorSetBlueColorFloat2);  // call_ea=0x24B5864A
    pm->RegisterOperationFunction(CKOGUID_SET_W, CKPGUID_QUATERNION, CKPGUID_QUATERNION, CKPGUID_FLOAT, CKColorSetAlphaColorFloat2);  // call_ea=0x24B586AA
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_EULERANGLES, CKPGUID_MATRIX, CKPGUID_NONE, CKEulerSetMatrix);  // call_ea=0x24B58712
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_EULERANGLES, CKPGUID_QUATERNION, CKPGUID_NONE, CKEulerSetQuaternion);  // call_ea=0x24B5877A
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_EULERANGLES, CKPGUID_STRING, CKPGUID_NONE, CKGenericSetString);  // call_ea=0x24B587E2
    pm->RegisterOperationFunction(CKOGUID_SET_X, CKPGUID_EULERANGLES, CKPGUID_EULERANGLES, CKPGUID_FLOAT, CKVectorSetXVectorFloat);  // call_ea=0x24B58842
    pm->RegisterOperationFunction(CKOGUID_SET_Y, CKPGUID_EULERANGLES, CKPGUID_EULERANGLES, CKPGUID_FLOAT, CKVectorSetYVectorFloat);  // call_ea=0x24B588A2
    pm->RegisterOperationFunction(CKOGUID_SET_Z, CKPGUID_EULERANGLES, CKPGUID_EULERANGLES, CKPGUID_FLOAT, CKVectorSetZVectorFloat);  // call_ea=0x24B58902
    pm->RegisterOperationFunction(CKOGUID_GET_EULER_ANGLES, CKPGUID_EULERANGLES, CKPGUID_3DENTITY, CKPGUID_3DENTITY, CKEulerGetEuler3dEntity3dEntity);  // call_ea=0x24B5896A
    pm->RegisterOperationFunction(CKOGUID_GET_VIEW_RECT, CKPGUID_RECT, CKPGUID_NONE, CKPGUID_NONE, CKRectGetViewRect);  // call_ea=0x24B589D2
    pm->RegisterOperationFunction(CKOGUID_TRANSFORM, CKPGUID_RECT, CKPGUID_RECT, CKPGUID_2DVECTOR, CKRectTransformRect2dVector);  // call_ea=0x24B58A3A
    pm->RegisterOperationFunction(CKOGUID_TRANSFORM, CKPGUID_RECT, CKPGUID_RECT, CKPGUID_RECT, CKRectTransformRectRect);  // call_ea=0x24B58AA2
    pm->RegisterOperationFunction(CKOGUID_GET_BOUNDING_BOX, CKPGUID_RECT, CKPGUID_2DENTITY, CKPGUID_NONE, CKRectGetBox2dEntity);  // call_ea=0x24B58B0A
    pm->RegisterOperationFunction(CKOGUID_GET_BOUNDING_BOX, CKPGUID_RECT, CKPGUID_3DENTITY, CKPGUID_NONE, CKRectGetBox3dEntity);  // call_ea=0x24B58B72
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_RECT, CKPGUID_STRING, CKPGUID_NONE, CKGenericSetString);  // call_ea=0x24B58BDA
    pm->RegisterOperationFunction(CKOGUID_SET_LEFT, CKPGUID_RECT, CKPGUID_RECT, CKPGUID_FLOAT, CKRectSetLeftRectFloat);  // call_ea=0x24B58C3A
    pm->RegisterOperationFunction(CKOGUID_SET_TOP, CKPGUID_RECT, CKPGUID_RECT, CKPGUID_FLOAT, CKRectSetTopRectFloat);  // call_ea=0x24B58C9A
    pm->RegisterOperationFunction(CKOGUID_SET_RIGHT, CKPGUID_RECT, CKPGUID_RECT, CKPGUID_FLOAT, CKRectSetRightRectFloat);  // call_ea=0x24B58CFA
    pm->RegisterOperationFunction(CKOGUID_SET_BOTTOM, CKPGUID_RECT, CKPGUID_RECT, CKPGUID_FLOAT, CKRectSetBottomRectFloat);  // call_ea=0x24B58D5A
    pm->RegisterOperationFunction(CKOGUID_SET_WIDTH, CKPGUID_RECT, CKPGUID_RECT, CKPGUID_FLOAT, CKRectSetWidthRectFloat);  // call_ea=0x24B58DBA
    pm->RegisterOperationFunction(CKOGUID_SET_HEIGHT, CKPGUID_RECT, CKPGUID_RECT, CKPGUID_FLOAT, CKRectSetHeightRectFloat);  // call_ea=0x24B58E1A
    pm->RegisterOperationFunction(CKOGUID_SET_GEOMETRIC_CENTER, CKPGUID_RECT, CKPGUID_RECT, CKPGUID_2DVECTOR, CKRectSetCenterRect2dVector);  // call_ea=0x24B58E82
    pm->RegisterOperationFunction(CKOGUID_OPPOSITE, CKPGUID_2DVECTOR, CKPGUID_2DVECTOR, CKPGUID_NONE, CK2dVectorOpposite2dVector);  // call_ea=0x24B58EEA
    pm->RegisterOperationFunction(CKOGUID_ADDITION, CKPGUID_2DVECTOR, CKPGUID_2DVECTOR, CKPGUID_2DVECTOR, CK2dVectorAdd2dVector2dVector);  // call_ea=0x24B58F52
    pm->RegisterOperationFunction(CKOGUID_SUBTRACTION, CKPGUID_2DVECTOR, CKPGUID_2DVECTOR, CKPGUID_2DVECTOR, CK2dVectorSubtract2dVector2dVector);  // call_ea=0x24B58FBA
    pm->RegisterOperationFunction(CKOGUID_MULTIPLICATION, CKPGUID_2DVECTOR, CKPGUID_2DVECTOR, CKPGUID_2DVECTOR, CK2dVectorMultiply2dVector2dVector);  // call_ea=0x24B59022
    pm->RegisterOperationFunction(CKOGUID_DIVISION, CKPGUID_2DVECTOR, CKPGUID_2DVECTOR, CKPGUID_2DVECTOR, CK2dVectorDivide2dVector2dVector);  // call_ea=0x24B5908A
    pm->RegisterOperationFunction(CKOGUID_MAX, CKPGUID_2DVECTOR, CKPGUID_2DVECTOR, CKPGUID_2DVECTOR, CK2dVectorMax2dVector2dVector);  // call_ea=0x24B590F2
    pm->RegisterOperationFunction(CKOGUID_MIN, CKPGUID_2DVECTOR, CKPGUID_2DVECTOR, CKPGUID_2DVECTOR, CK2dVectorMin2dVector2dVector);  // call_ea=0x24B5915A
    pm->RegisterOperationFunction(CKOGUID_INVERSE, CKPGUID_2DVECTOR, CKPGUID_2DVECTOR, CKPGUID_NONE, CK2dVectorInverse2dVector);  // call_ea=0x24B591C2
    pm->RegisterOperationFunction(CKOGUID_RANDOM, CKPGUID_2DVECTOR, CKPGUID_NONE, CKPGUID_NONE, CK2dVectorRandom);  // call_ea=0x24B5922A
    pm->RegisterOperationFunction(CKOGUID_MULTIPLICATION, CKPGUID_2DVECTOR, CKPGUID_2DVECTOR, CKPGUID_FLOAT, CK2dVectorMultiply2dVectorFloat);  // call_ea=0x24B5928A
    pm->RegisterOperationFunction(CKOGUID_DIVISION, CKPGUID_2DVECTOR, CKPGUID_2DVECTOR, CKPGUID_FLOAT, CK2dVectorDivide2dVectorFloat);  // call_ea=0x24B592EA
    pm->RegisterOperationFunction(CKOGUID_GET_POSITION, CKPGUID_2DVECTOR, CKPGUID_2DENTITY, CKPGUID_NONE, CK2dVectorGetPosition2dEntity);  // call_ea=0x24B59352
    pm->RegisterOperationFunction(CKOGUID_GET_SIZE, CKPGUID_2DVECTOR, CKPGUID_2DENTITY, CKPGUID_NONE, CK2dVectorGetSize2dEntity);  // call_ea=0x24B593BA
    pm->RegisterOperationFunction(CKOGUID_GET_CURVE_POSITION, CKPGUID_2DVECTOR, CKPGUID_FLOAT, CKPGUID_2DCURVE, CK2dVectorGetCurvePosFloat2dCurve);  // call_ea=0x24B5941A
    pm->RegisterOperationFunction(CKOGUID_GET_SCREEN_ORIGIN, CKPGUID_2DVECTOR, CKPGUID_NONE, CKPGUID_NONE, CK2dVectorGetScreenOrigin);  // call_ea=0x24B59482
    pm->RegisterOperationFunction(CKOGUID_SYMMETRY, CKPGUID_2DVECTOR, CKPGUID_2DVECTOR, CKPGUID_2DVECTOR, CK2dVectorSymmetry2dVector2dVector);  // call_ea=0x24B594EA
    pm->RegisterOperationFunction(CKOGUID_TRANSFORM, CKPGUID_2DVECTOR, CKPGUID_VECTOR, CKPGUID_3DENTITY, CK2dVectorTransformVector3dEntity);  // call_ea=0x24B59552
    pm->RegisterOperationFunction(CKOGUID_GET_ASPECT_RATIO, CKPGUID_2DVECTOR, CKPGUID_CAMERA, CKPGUID_NONE, CK2dVectorGetAspectRatioCamera);  // call_ea=0x24B595BA
    pm->RegisterOperationFunction(CKOGUID_GET_VERTEX_UVS, CKPGUID_2DVECTOR, CKPGUID_MESH, CKPGUID_INT, CK2dVectorGetVertexUvsMeshInt);  // call_ea=0x24B59622
    pm->RegisterOperationFunction(CKOGUID_GET_POSITION, CKPGUID_2DVECTOR, CKPGUID_RECT, CKPGUID_NONE, CK2dVectorGetPosRect);  // call_ea=0x24B5968A
    pm->RegisterOperationFunction(CKOGUID_GET_GEOMETRIC_CENTER, CKPGUID_2DVECTOR, CKPGUID_RECT, CKPGUID_NONE, CK2dVectorGetCenterRect);  // call_ea=0x24B596F2
    pm->RegisterOperationFunction(CKOGUID_GET_SIZE, CKPGUID_2DVECTOR, CKPGUID_RECT, CKPGUID_NONE, CK2dVectorGetSizeRect);  // call_ea=0x24B5975A
    pm->RegisterOperationFunction(CKOGUID_GET_POSITION, CKPGUID_2DVECTOR, CKPGUID_RECT, CKPGUID_NONE, CK2dVectorGetPosRect);  // call_ea=0x24B597C2
    pm->RegisterOperationFunction(CKOGUID_GET_BOTTOM_RIGHT_CORNER, CKPGUID_2DVECTOR, CKPGUID_RECT, CKPGUID_NONE, CK2dVectorGetBRRect);  // call_ea=0x24B5982A
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_2DVECTOR, CKPGUID_FLOAT, CKPGUID_FLOAT, CK2dVectorSetFloat);  // call_ea=0x24B59882
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_2DVECTOR, CKPGUID_INT, CKPGUID_INT, CK2dVectorSetIntInt);  // call_ea=0x24B598EA
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_2DVECTOR, CKPGUID_STRING, CKPGUID_NONE, CKGenericSetString);  // call_ea=0x24B59952
    pm->RegisterOperationFunction(CKOGUID_SET_X, CKPGUID_2DVECTOR, CKPGUID_2DVECTOR, CKPGUID_FLOAT, CK2dVectorSetX2dVectorFloat);  // call_ea=0x24B599B2
    pm->RegisterOperationFunction(CKOGUID_SET_Y, CKPGUID_2DVECTOR, CKPGUID_2DVECTOR, CKPGUID_FLOAT, CK2dVectorSetY2dVectorFloat);  // call_ea=0x24B59A12
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_2DVECTOR, CKPGUID_VECTOR, CKPGUID_NONE, CK2dVectorSetVector);  // call_ea=0x24B59A7A
    pm->RegisterOperationFunction(CKOGUID_ADDITION, CKPGUID_MATRIX, CKPGUID_MATRIX, CKPGUID_MATRIX, CKMatrixAddMatrixMatrix);  // call_ea=0x24B59AE2
    pm->RegisterOperationFunction(CKOGUID_SUBTRACTION, CKPGUID_MATRIX, CKPGUID_MATRIX, CKPGUID_MATRIX, CKMatrixSubtractMatrixMatrix);  // call_ea=0x24B59B4A
    pm->RegisterOperationFunction(CKOGUID_MULTIPLICATION, CKPGUID_MATRIX, CKPGUID_MATRIX, CKPGUID_MATRIX, CKMatrixMultiplyMatrixMatrix);  // call_ea=0x24B59BB2
    pm->RegisterOperationFunction(CKOGUID_DIVISION, CKPGUID_MATRIX, CKPGUID_MATRIX, CKPGUID_MATRIX, CKMatrixDivideMatrixMatrix);  // call_ea=0x24B59C1A
    pm->RegisterOperationFunction(CKOGUID_INVERSE, CKPGUID_MATRIX, CKPGUID_MATRIX, CKPGUID_NONE, CKMatrixInverseMatrix);  // call_ea=0x24B59C82
    pm->RegisterOperationFunction(CKOGUID_MULTIPLICATION, CKPGUID_MATRIX, CKPGUID_MATRIX, CKPGUID_FLOAT, CKMatrixMultiplyMatrixFloat);  // call_ea=0x24B59CE2
    pm->RegisterOperationFunction(CKOGUID_GET_LOCAL_MATRIX, CKPGUID_MATRIX, CKPGUID_3DENTITY, CKPGUID_NONE, CKMatrixGetLocalMatrix3dEntity);  // call_ea=0x24B59D4A
    pm->RegisterOperationFunction(CKOGUID_GET_WORLD_MATRIX, CKPGUID_MATRIX, CKPGUID_3DENTITY, CKPGUID_NONE, CKMatrixGetWorldMatrix3dEntity);  // call_ea=0x24B59DB2
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_MATRIX, CKPGUID_STRING, CKPGUID_NONE, CKGenericSetString);  // call_ea=0x24B59E1A
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_MATRIX, CKPGUID_VECTOR, CKPGUID_VECTOR, CKVectorSetMatrix);  // call_ea=0x24B59E82
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_MATRIX, CKPGUID_QUATERNION, CKPGUID_NONE, CKMatrixSetQuaternion);  // call_ea=0x24B59EEA
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_MATRIX, CKPGUID_EULERANGLES, CKPGUID_NONE, CKMatrixSetEuler);  // call_ea=0x24B59F52
    pm->RegisterOperationFunction(CKOGUID_SET_X, CKPGUID_MATRIX, CKPGUID_VECTOR, CKPGUID_MATRIX, CKVectorSetXMatrix);  // call_ea=0x24B59FBA
    pm->RegisterOperationFunction(CKOGUID_SET_Y, CKPGUID_MATRIX, CKPGUID_VECTOR, CKPGUID_MATRIX, CKVectorSetYMatrix);  // call_ea=0x24B5A022
    pm->RegisterOperationFunction(CKOGUID_SET_Z, CKPGUID_MATRIX, CKPGUID_VECTOR, CKPGUID_MATRIX, CKVectorSetZMatrix);  // call_ea=0x24B5A08A
    pm->RegisterOperationFunction(CKOGUID_SET_POSITION, CKPGUID_MATRIX, CKPGUID_VECTOR, CKPGUID_MATRIX, CKVectorSetPosMatrix);  // call_ea=0x24B5A0F2
    pm->RegisterOperationFunction(CKOGUID_ADDITION, CKPGUID_COLOR, CKPGUID_COLOR, CKPGUID_COLOR, CKColorAddColorColor);  // call_ea=0x24B5A15A
    pm->RegisterOperationFunction(CKOGUID_SUBTRACTION, CKPGUID_COLOR, CKPGUID_COLOR, CKPGUID_COLOR, CKColorSubtractColorColor);  // call_ea=0x24B5A1C2
    pm->RegisterOperationFunction(CKOGUID_INVERSE, CKPGUID_COLOR, CKPGUID_COLOR, CKPGUID_NONE, CKColorInverseColor);  // call_ea=0x24B5A22A
    pm->RegisterOperationFunction(CKOGUID_RANDOM, CKPGUID_COLOR, CKPGUID_NONE, CKPGUID_NONE, CKColorRandom);  // call_ea=0x24B5A292
    pm->RegisterOperationFunction(CKOGUID_MULTIPLICATION, CKPGUID_COLOR, CKPGUID_FLOAT, CKPGUID_COLOR, CKColorMultiplyFloatColor);  // call_ea=0x24B5A2F2
    pm->RegisterOperationFunction(CKOGUID_GET_SPECULAR_COLOR, CKPGUID_COLOR, CKPGUID_MATERIAL, CKPGUID_NONE, CKColorGetSpecularMaterial);  // call_ea=0x24B5A35A
    pm->RegisterOperationFunction(CKOGUID_GET_SPECULAR_COLOR_POWER, CKPGUID_FLOAT, CKPGUID_MATERIAL, CKPGUID_NONE, CKColorGetSpecularPowerMaterial);  // call_ea=0x24B5A3BA
    pm->RegisterOperationFunction(CKOGUID_GET_DIFFUSE_COLOR, CKPGUID_COLOR, CKPGUID_MATERIAL, CKPGUID_NONE, CKColorGetDiffuseMaterial);  // call_ea=0x24B5A422
    pm->RegisterOperationFunction(CKOGUID_GET_EMISSIVE_COLOR, CKPGUID_COLOR, CKPGUID_MATERIAL, CKPGUID_NONE, CKColorGetEmissiveMaterial);  // call_ea=0x24B5A48A
    pm->RegisterOperationFunction(CKOGUID_GET_AMBIENT_COLOR, CKPGUID_COLOR, CKPGUID_MATERIAL, CKPGUID_NONE, CKColorGetAmbientMaterial);  // call_ea=0x24B5A4F2
    pm->RegisterOperationFunction(CKOGUID_GET_COLOR, CKPGUID_COLOR, CKPGUID_LIGHT, CKPGUID_NONE, CKColorGetColorLight);  // call_ea=0x24B5A55A
    pm->RegisterOperationFunction(CKOGUID_GET_VERTEX_COLOR, CKPGUID_COLOR, CKPGUID_MESH, CKPGUID_INT, CKColorGetVertexColorMeshInt);  // call_ea=0x24B5A5C2
    pm->RegisterOperationFunction(CKOGUID_GET_VERTEX_SPECULAR_COLOR, CKPGUID_COLOR, CKPGUID_MESH, CKPGUID_INT, CKColorGetVertexSpecularColorMeshInt);  // call_ea=0x24B5A62A
    pm->RegisterOperationFunction(CKOGUID_RAINBOW_COLOR, CKPGUID_COLOR, CKPGUID_FLOAT, CKPGUID_FLOAT, CKColorRainbowFloatFloat);  // call_ea=0x24B5A682
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_COLOR, CKPGUID_STRING, CKPGUID_NONE, CKGenericSetString);  // call_ea=0x24B5A6EA
    pm->RegisterOperationFunction(CKOGUID_SET_RED, CKPGUID_COLOR, CKPGUID_COLOR, CKPGUID_FLOAT, CKColorSetRedColorFloat);  // call_ea=0x24B5A74A
    pm->RegisterOperationFunction(CKOGUID_SET_GREEN, CKPGUID_COLOR, CKPGUID_COLOR, CKPGUID_FLOAT, CKColorSetGreenColorFloat);  // call_ea=0x24B5A7AA
    pm->RegisterOperationFunction(CKOGUID_SET_BLUE, CKPGUID_COLOR, CKPGUID_COLOR, CKPGUID_FLOAT, CKColorSetBlueColorFloat);  // call_ea=0x24B5A80A
    pm->RegisterOperationFunction(CKOGUID_SET_ALPHA, CKPGUID_COLOR, CKPGUID_COLOR, CKPGUID_FLOAT, CKColorSetAlphaColorFloat);  // call_ea=0x24B5A86A
    pm->RegisterOperationFunction(CKOGUID_ADDITION, CKPGUID_STRING, CKPGUID_STRING, CKPGUID_STRING, CKStringAddStringString);  // call_ea=0x24B5A8D2
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_STRING, CKPGUID_INT, CKPGUID_NONE, CKStringSetGeneric);  // call_ea=0x24B5A93A
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_STRING, CKPGUID_VECTOR, CKPGUID_NONE, CKStringSetGeneric);  // call_ea=0x24B5A9A2
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_STRING, CKPGUID_2DVECTOR, CKPGUID_NONE, CKStringSetGeneric);  // call_ea=0x24B5AA0A
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_STRING, CKPGUID_RECT, CKPGUID_NONE, CKStringSetGeneric);  // call_ea=0x24B5AA72
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_STRING, CKPGUID_BOOL, CKPGUID_NONE, CKStringSetGeneric);  // call_ea=0x24B5AADA
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_STRING, CKPGUID_TIME, CKPGUID_NONE, CKStringSetGeneric);  // call_ea=0x24B5AB42
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_STRING, CKPGUID_FLOAT, CKPGUID_NONE, CKStringSetGeneric);  // call_ea=0x24B5ABA2
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_STRING, CKPGUID_ANGLE, CKPGUID_NONE, CKStringSetGeneric);  // call_ea=0x24B5AC0A
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_STRING, CKPGUID_PERCENTAGE, CKPGUID_NONE, CKStringSetGeneric);  // call_ea=0x24B5AC72
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_STRING, CKPGUID_COLOR, CKPGUID_NONE, CKStringSetGeneric);  // call_ea=0x24B5ACDA
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_STRING, CKPGUID_MESSAGE, CKPGUID_NONE, CKStringSetGeneric);  // call_ea=0x24B5AD42
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_STRING, CKPGUID_ATTRIBUTE, CKPGUID_NONE, CKStringSetGeneric);  // call_ea=0x24B5ADAA
    pm->RegisterOperationFunction(CKOGUID_GET_NAME, CKPGUID_STRING, CKPGUID_OBJECT, CKPGUID_NONE, CKStringGetNameObject);  // call_ea=0x24B5AE12
    pm->RegisterOperationFunction(CKOGUID_GET_TEXT, CKPGUID_STRING, CKPGUID_SPRITETEXT, CKPGUID_NONE, CKStringGetTextSpriteText);  // call_ea=0x24B5AE7A
    pm->RegisterOperationFunction(CKOGUID_ADDITION, CKPGUID_BOX, CKPGUID_BOX, CKPGUID_BOX, CKBoxAddBoxBox);  // call_ea=0x24B5AEE2
    pm->RegisterOperationFunction(CKOGUID_SUBTRACTION, CKPGUID_BOX, CKPGUID_BOX, CKPGUID_BOX, CKBoxSubtractBoxBox);  // call_ea=0x24B5AF4A
    pm->RegisterOperationFunction(CKOGUID_GET_BOUNDING_BOX, CKPGUID_BOX, CKPGUID_3DENTITY, CKPGUID_NONE, CKBoxGetBox3dEntity);  // call_ea=0x24B5AFB2
    pm->RegisterOperationFunction(CKOGUID_GET_HIERARCHICAL_BOUNDING_BOX, CKPGUID_BOX, CKPGUID_3DENTITY, CKPGUID_NONE, CKBoxGetHBox3dEntity);  // call_ea=0x24B5B01A
    pm->RegisterOperationFunction(CKOGUID_GET_BOUNDING_BOX, CKPGUID_BOX, CKPGUID_MESH, CKPGUID_NONE, CKBoxGetBoxMesh);  // call_ea=0x24B5B082
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_BOX, CKPGUID_STRING, CKPGUID_NONE, CKGenericSetString);  // call_ea=0x24B5B0EA
    pm->RegisterOperationFunction(CKOGUID_GET_TYPE, CKPGUID_CLASSID, CKPGUID_BEOBJECT, CKPGUID_NONE, CKIdGetObjectTypeObject);  // call_ea=0x24B5B152
    pm->RegisterOperationFunction(CKOGUID_GET_GROUP_TYPE, CKPGUID_CLASSID, CKPGUID_GROUP, CKPGUID_NONE, CKIdGetGroupTypeGroup);  // call_ea=0x24B5B1BA
    pm->RegisterOperationFunction(CKOGUID_ADDITION, CKPGUID_BOOL, CKPGUID_OBJECTARRAY, CKPGUID_OBJECT, CKObjectArrayAddObjectArrayObject);  // call_ea=0x24B5B222
    pm->RegisterOperationFunction(CKOGUID_SUBTRACTION, CKPGUID_BOOL, CKPGUID_OBJECTARRAY, CKPGUID_OBJECT, CKObjectArraySubtractObjectArrayObject);  // call_ea=0x24B5B28A
    pm->RegisterOperationFunction(CKOGUID_IS_IN, CKPGUID_BOOL, CKPGUID_OBJECTARRAY, CKPGUID_OBJECT, CKBoolIsInObjectArrayObject);  // call_ea=0x24B5B2F2
    pm->RegisterOperationFunction(CKOGUID_OR, CKPGUID_OBJECTARRAY, CKPGUID_OBJECTARRAY, CKPGUID_OBJECTARRAY, CKObjectArrayAddObjectArrayObjectArray);  // call_ea=0x24B5B35A
    pm->RegisterOperationFunction(CKOGUID_SUBTRACTION, CKPGUID_OBJECTARRAY, CKPGUID_OBJECTARRAY, CKPGUID_OBJECTARRAY, CKObjectArraySubtractObjectArrayObjectArray);  // call_ea=0x24B5B3C2
    pm->RegisterOperationFunction(CKOGUID_AND, CKPGUID_OBJECTARRAY, CKPGUID_OBJECTARRAY, CKPGUID_OBJECTARRAY, CKObjectArrayMultiplyObjectArrayObjectArray);  // call_ea=0x24B5B42A
    pm->RegisterOperationFunction(CKOGUID_GET_MATERIAL_LIST, CKPGUID_OBJECTARRAY, CKPGUID_MESH, CKPGUID_NONE, CKObjectArrayGetMaterialListMesh);  // call_ea=0x24B5B492
    pm->RegisterOperationFunction(CKOGUID_GET_CHILDREN, CKPGUID_OBJECTARRAY, CKPGUID_3DENTITY, CKPGUID_NONE, CKObjectArrayGetChildren3dEntity);  // call_ea=0x24B5B4FA
    pm->RegisterOperationFunction(CKOGUID_GET_MESH_LIST, CKPGUID_OBJECTARRAY, CKPGUID_3DENTITY, CKPGUID_NONE, CKObjectArrayGetMeshList3dEntity);  // call_ea=0x24B5B562
    pm->RegisterOperationFunction(CKOGUID_GET_ANIMATION, CKPGUID_OBJECTARRAY, CKPGUID_CHARACTER, CKPGUID_NONE, CKObjectArrayGetAnimationsCharacter);  // call_ea=0x24B5B5CA
    pm->RegisterOperationFunction(CKOGUID_GET_BODYPARTS, CKPGUID_OBJECTARRAY, CKPGUID_CHARACTER, CKPGUID_NONE, CKObjectArrayGetBodyPartCharacter);  // call_ea=0x24B5B632
    pm->RegisterOperationFunction(CKOGUID_GET_OBJECT_BY_NAME, CKPGUID_OBJECT, CKPGUID_STRING, CKPGUID_NONE, CKObjectGetObjectByNameString);  // call_ea=0x24B5B69A
    pm->RegisterOperationFunction(CKOGUID_PICK_OBJECT, CKPGUID_RENDEROBJECT, CKPGUID_INT, CKPGUID_INT, CKObjectWindowPickIntInt);  // call_ea=0x24B5B702
    pm->RegisterOperationFunction(CKOGUID_PICK_OBJECT, CKPGUID_RENDEROBJECT, CKPGUID_2DVECTOR, CKPGUID_NONE, CKObjectWindowPick2dVector);  // call_ea=0x24B5B76A
    pm->RegisterOperationFunction(CKOGUID_GET_ELEMENT, CKPGUID_OBJECT, CKPGUID_OBJECTARRAY, CKPGUID_INT, CKObjectGetElementObjectArrayInt);  // call_ea=0x24B5B7D2
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_OBJECT, CKPGUID_STRING, CKPGUID_NONE, CKGenericSetString);  // call_ea=0x24B5B83A
    pm->RegisterOperationFunction(CKOGUID_GET_CURVE_POINT, CKPGUID_CURVEPOINT, CKPGUID_CURVE, CKPGUID_INT, CKCurvePointGetPointCurveInt);  // call_ea=0x24B5B8A2
    pm->RegisterOperationFunction(CKOGUID_GET_PLACE, CKPGUID_PLACE, CKPGUID_3DENTITY, CKPGUID_PLACE, CKPlaceGetPlace3DEntityPlace);  // call_ea=0x24B5B90A
    pm->RegisterOperationFunction(CKOGUID_GET_REFERENTIEL_PLACE, CKPGUID_PLACE, CKPGUID_3DENTITY, CKPGUID_NONE, CKPlaceGetRefPlace3DEntity);  // call_ea=0x24B5B972
    pm->RegisterOperationFunction(CKOGUID_GET_PORTALS, CKPGUID_OBJECTARRAY, CKPGUID_PLACE, CKPGUID_PLACE, CKObjectArrayGetPortalsPlacePlace);  // call_ea=0x24B5B9DA
    pm->RegisterOperationFunction(CKOGUID_GET_PORTALS, CKPGUID_OBJECTARRAY, CKPGUID_PLACE, CKPGUID_NONE, CKObjectArrayGetPortalsPlace);  // call_ea=0x24B5BA42
    pm->RegisterOperationFunction(CKOGUID_GET_TARGET, CKPGUID_3DENTITY, CKPGUID_TARGETCAMERA, CKPGUID_NONE, CK3dEntityGetTargetTargetCamera);  // call_ea=0x24B5BAAA
    pm->RegisterOperationFunction(CKOGUID_GET_TARGET, CKPGUID_3DENTITY, CKPGUID_TARGETLIGHT, CKPGUID_NONE, CK3dEntityGetTargetTargetLight);  // call_ea=0x24B5BB12
    pm->RegisterOperationFunction(CKOGUID_GET_ROOT, CKPGUID_3DENTITY, CKPGUID_CHARACTER, CKPGUID_NONE, CK3dEntityGetRootCharacter);  // call_ea=0x24B5BB7A
    pm->RegisterOperationFunction(CKOGUID_GET_PARENT, CKPGUID_3DENTITY, CKPGUID_3DENTITY, CKPGUID_NONE, CK3dEntityGetParent3dEntity);  // call_ea=0x24B5BBE2
    pm->RegisterOperationFunction(CKOGUID_GET_PARENT, CKPGUID_2DENTITY, CKPGUID_2DENTITY, CKPGUID_NONE, CK2dEntityGetParent2dEntity);  // call_ea=0x24B5BC4A
    pm->RegisterOperationFunction(CKOGUID_GET_CURRENT, CKPGUID_MESH, CKPGUID_3DENTITY, CKPGUID_NONE, CKMeshGetCurrent3dEntity);  // call_ea=0x24B5BCB2
    pm->RegisterOperationFunction(CKOGUID_GET_CURRENT, CKPGUID_SCENE, CKPGUID_NONE, CKPGUID_NONE, CKSceneGetCurrentSceneNoneNone);  // call_ea=0x24B5BD1A
    pm->RegisterOperationFunction(CKOGUID_GET_CURRENT, CKPGUID_LEVEL, CKPGUID_NONE, CKPGUID_NONE, CKLevelGetCurrentLevelNoneNone);  // call_ea=0x24B5BD82
    pm->RegisterOperationFunction(CKOGUID_GET_FACE_MATERIAL, CKPGUID_MATERIAL, CKPGUID_MESH, CKPGUID_INT, CKMaterialGetFaceMaterialMeshInt);  // call_ea=0x24B5BDEA
    pm->RegisterOperationFunction(CKOGUID_GET_MATERIAL, CKPGUID_MATERIAL, CKPGUID_MESH, CKPGUID_INT, CKMaterialGetMaterialMeshInt);  // call_ea=0x24B5BE52
    pm->RegisterOperationFunction(CKOGUID_GET_MATERIAL, CKPGUID_MATERIAL, CKPGUID_SPRITE3D, CKPGUID_NONE, CKMaterialGetMaterialSprite3D);  // call_ea=0x24B5BEBA
    pm->RegisterOperationFunction(CKOGUID_GET_MATERIAL, CKPGUID_MATERIAL, CKPGUID_2DENTITY, CKPGUID_NONE, CKMaterialGetMaterial2DEntity);  // call_ea=0x24B5BF22
    pm->RegisterOperationFunction(CKOGUID_GET_TEXTURE, CKPGUID_TEXTURE, CKPGUID_MATERIAL, CKPGUID_NONE, CKTextureGetTextureMaterial);  // call_ea=0x24B5BF8A
    pm->RegisterOperationFunction(CKOGUID_GET_CHARACTER, CKPGUID_CHARACTER, CKPGUID_3DENTITY, CKPGUID_NONE, CKCharacterGetCharacter3dEntity);  // call_ea=0x24B5BFF2
    pm->RegisterOperationFunction(CKOGUID_GET_COUNT, CKPGUID_INT, CKPGUID_GROUP, CKPGUID_NONE, CKIntGetCountGroup);  // call_ea=0x24B5C05A
    pm->RegisterOperationFunction(CKOGUID_GET_ELEMENT, CKPGUID_OBJECT, CKPGUID_GROUP, CKPGUID_INT, CKObjectGetElementGroupInt);  // call_ea=0x24B5C0C2
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_BOOL, CKPGUID_FLOAT, CKPGUID_NONE, CKBoolSetFloat);  // call_ea=0x24B5C122
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_BOOL, CKPGUID_INT, CKPGUID_NONE, CKBoolSetInt);  // call_ea=0x24B5C18A
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_INT, CKPGUID_BOOL, CKPGUID_NONE, CKIntSetBool);  // call_ea=0x24B5C1F2
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_FLOAT, CKPGUID_BOOL, CKPGUID_NONE, CKFloatSetBool);  // call_ea=0x24B5C252
    pm->RegisterOperationFunction(CKOGUID_GET_BODYPART, CKPGUID_BODYPART, CKPGUID_CHARACTER, CKPGUID_STRING, CKBodyPartGetBodyPartByIncludedNameCharacterString);  // call_ea=0x24B5C2BA
    pm->RegisterOperationFunction(CKOGUID_GET_SCRIPT, CKPGUID_SCRIPT, CKPGUID_BEOBJECT, CKPGUID_INT, CKScriptGetScriptBeObjectInt);  // call_ea=0x24B5C322
    pm->RegisterOperationFunction(CKOGUID_GET_SCRIPT, CKPGUID_SCRIPT, CKPGUID_BEOBJECT, CKPGUID_STRING, CKScriptGetScriptBeObjectString);  // call_ea=0x24B5C38A
    pm->RegisterOperationFunction(CKOGUID_DYNAMIC_CAST, CKPGUID_OBJECT, CKPGUID_OBJECT, CKPGUID_NONE, CKBeObjectCastCKBeObject);  // call_ea=0x24B5C3F2
    pm->RegisterOperationFunction(CKOGUID_GET_ANIMATION, CKPGUID_OBJECTANIMATION, CKPGUID_3DENTITY, CKPGUID_STRING, CKObjectAnimationGetAnimation3dEntityString);  // call_ea=0x24B5C45A
    pm->RegisterOperationFunction(CKOGUID_GET_ANIMATION, CKPGUID_OBJECTANIMATION, CKPGUID_3DENTITY, CKPGUID_INT, CKObjectAnimationGetAnimation3dEntityInt);  // call_ea=0x24B5C4C2
    pm->RegisterOperationFunction(CKOGUID_GET_ANIMATION_COUNT, CKPGUID_INT, CKPGUID_3DENTITY, CKPGUID_NONE, CKIntGetAnimationCount3dEntity);  // call_ea=0x24B5C52A
    pm->RegisterOperationFunction(CKOGUID_GET_ANIMATION, CKPGUID_ANIMATION, CKPGUID_CHARACTER, CKPGUID_STRING, CKAnimationGetAnimationCharacterString);  // call_ea=0x24B5C592
    pm->RegisterOperationFunction(CKOGUID_FROM_ROTATION, CKPGUID_QUATERNION, CKPGUID_VECTOR, CKPGUID_ANGLE, CKQuaternionFromRotation);  // call_ea=0x24B5C5FA
    pm->RegisterOperationFunction(CKOGUID_FROM_ROTATION, CKPGUID_MATRIX, CKPGUID_VECTOR, CKPGUID_ANGLE, CKMatrixFromRotation);  // call_ea=0x24B5C662
    pm->RegisterOperationFunction(CKOGUID_CONVERT, CKPGUID_MESSAGE, CKPGUID_STRING, CKPGUID_NONE, CKGenericSetString);  // call_ea=0x24B5C6CA
    pm->RegisterOperationFunction(CKOGUID_GET_SOUND_FILE_NAME, CKPGUID_STRING, CKPGUID_WAVESOUND, CKPGUID_NONE, CKStringGetSoundFileNameWaveSound);  // call_ea=0x24B5C732
    pm->RegisterOperationFunction(CKOGUID_GET_LENGTH, CKPGUID_FLOAT, CKPGUID_WAVESOUND, CKPGUID_NONE, CKFloatGetLengthWaveSound);  // call_ea=0x24B5C792
    pm->RegisterOperationFunction(CKOGUID_GET_SAMPLING_RATE, CKPGUID_INT, CKPGUID_WAVESOUND, CKPGUID_NONE, CKIntGetFrequencyWaveSound);  // call_ea=0x24B5C7FA
    pm->RegisterOperationFunction(CKOGUID_IS_LOOPING, CKPGUID_BOOL, CKPGUID_WAVESOUND, CKPGUID_NONE, CKBoolGetLoopModeWaveSound);  // call_ea=0x24B5C862
    pm->RegisterOperationFunction(CKOGUID_IS_STREAMING, CKPGUID_BOOL, CKPGUID_WAVESOUND, CKPGUID_NONE, CKBoolGetFileStreamingWaveSound);  // call_ea=0x24B5C8CA
    pm->RegisterOperationFunction(CKOGUID_IS_PLAYING, CKPGUID_BOOL, CKPGUID_WAVESOUND, CKPGUID_NONE, CKBoolIsPlayingWaveSound);  // call_ea=0x24B5C932
    pm->RegisterOperationFunction(CKOGUID_IS_PAUSED, CKPGUID_BOOL, CKPGUID_WAVESOUND, CKPGUID_NONE, CKBoolIsPausedWaveSound);  // call_ea=0x24B5C99A
    pm->RegisterOperationFunction(CKOGUID_GET_GAIN, CKPGUID_FLOAT, CKPGUID_WAVESOUND, CKPGUID_NONE, CKFloatGetVolumeWaveSound);  // call_ea=0x24B5C9FA
    pm->RegisterOperationFunction(CKOGUID_GET_PITCH, CKPGUID_FLOAT, CKPGUID_WAVESOUND, CKPGUID_NONE, CKFloatGetPitchWaveSound);  // call_ea=0x24B5CA5A
    pm->RegisterOperationFunction(CKOGUID_GET_PAN, CKPGUID_FLOAT, CKPGUID_WAVESOUND, CKPGUID_NONE, CKFloatGetPanWaveSound);  // call_ea=0x24B5CABA
    pm->RegisterOperationFunction(CKOGUID_GET_RELATIVE_POSITION, CKPGUID_VECTOR, CKPGUID_WAVESOUND, CKPGUID_NONE, CKVectorGetRelPositionWaveSound);  // call_ea=0x24B5CB22
    pm->RegisterOperationFunction(CKOGUID_GET_RELATIVE_DIRECTION, CKPGUID_VECTOR, CKPGUID_WAVESOUND, CKPGUID_NONE, CKVectorGetRelDirectionWaveSound);  // call_ea=0x24B5CB8A
    pm->RegisterOperationFunction(CKOGUID_GET_DISTANCE, CKPGUID_FLOAT, CKPGUID_WAVESOUND, CKPGUID_NONE, CKFloatGetDistanceFromListenerWaveSound);  // call_ea=0x24B5CBEA
    pm->RegisterOperationFunction(CKOGUID_GET_CONE, CKPGUID_VECTOR, CKPGUID_WAVESOUND, CKPGUID_NONE, CKVectorGetConeWaveSound);  // call_ea=0x24B5CC52
    pm->RegisterOperationFunction(CKOGUID_GET_MIN_MAX, CKPGUID_2DVECTOR, CKPGUID_WAVESOUND, CKPGUID_NONE, CK2dVectorGetMinMaxDistanceWaveSound);  // call_ea=0x24B5CCBA
    pm->RegisterOperationFunction(CKOGUID_GET_VELOCITY, CKPGUID_VECTOR, CKPGUID_WAVESOUND, CKPGUID_NONE, CKVectorGetVelocityWaveSound);  // call_ea=0x24B5CD22
    pm->RegisterOperationFunction(CKOGUID_GET_PLAYED_TIME, CKPGUID_TIME, CKPGUID_WAVESOUND, CKPGUID_NONE, CKTimeGetPlayedMS);  // call_ea=0x24B5CD8A
    pm->RegisterOperationFunction(CKOGUID_IS_CONTENT_EQUAL, CKPGUID_BOOL, CKPGUID_DATAARRAY, CKPGUID_DATAARRAY, CKBoolEqualDataArrayDataArray);  // call_ea=0x24B5CDF2
    pm->RegisterOperationFunction(CKOGUID_IS_CONTENT_EQUAL, CKPGUID_BOOL, CKPGUID_GROUP, CKPGUID_GROUP, CKBoolEqualGroupGroup);  // call_ea=0x24B5CE5A
    pm->RegisterOperationFunction(CKOGUID_EQUAL, CKPGUID_BOOL, CKPGUID_STRING, CKPGUID_STRING, CKBoolEqualStringString);  // call_ea=0x24B5CEC2
    pm->RegisterOperationFunction(CKOGUID_EQUAL, CKPGUID_BOOL, CKPGUID_OBJECTARRAY, CKPGUID_OBJECTARRAY, CKBoolEqualObjectArrayObjectArray);  // call_ea=0x24B5CF2A
    pm->RegisterOperationFunction(CKOGUID_EQUAL, CKPGUID_BOOL, CKPGUID_INT, CKPGUID_FLOAT, CKBoolEqualIntFloat);  // call_ea=0x24B5CF8A
    pm->RegisterOperationFunction(CKOGUID_NOT_EQUAL, CKPGUID_BOOL, CKPGUID_INT, CKPGUID_FLOAT, CKBoolNotEqualIntFloat);  // call_ea=0x24B5CFEA
    pm->RegisterOperationFunction(CKOGUID_EQUAL, CKPGUID_BOOL, CKPGUID_BOOL, CKPGUID_BOOL, CKBoolEqualBoolBool);  // call_ea=0x24B5D052
    pm->RegisterOperationFunction(CKOGUID_NOT_EQUAL, CKPGUID_BOOL, CKPGUID_BOOL, CKPGUID_BOOL, CKBoolNotEqualBoolBool);  // call_ea=0x24B5D0BA
    pm->RegisterOperationFunction(CKOGUID_EQUAL, CKPGUID_BOOL, CKPGUID_FLOAT, CKPGUID_FLOAT, CKGenericEqual1Dword);  // call_ea=0x24B5D112
    pm->RegisterOperationFunction(CKOGUID_NOT_EQUAL, CKPGUID_BOOL, CKPGUID_FLOAT, CKPGUID_FLOAT, CKGenericNotEqual1Dword);  // call_ea=0x24B5D16A
    pm->RegisterOperationFunction(CKOGUID_EQUAL, CKPGUID_BOOL, CKPGUID_INT, CKPGUID_INT, CKGenericEqual1Dword);  // call_ea=0x24B5D1D2
    pm->RegisterOperationFunction(CKOGUID_NOT_EQUAL, CKPGUID_BOOL, CKPGUID_INT, CKPGUID_INT, CKGenericNotEqual1Dword);  // call_ea=0x24B5D23A
    pm->RegisterOperationFunction(CKOGUID_EQUAL, CKPGUID_BOOL, CKPGUID_OBJECT, CKPGUID_OBJECT, CKGenericEqual1Dword);  // call_ea=0x24B5D2A2
    pm->RegisterOperationFunction(CKOGUID_NOT_EQUAL, CKPGUID_BOOL, CKPGUID_OBJECT, CKPGUID_OBJECT, CKGenericNotEqual1Dword);  // call_ea=0x24B5D30A
    pm->RegisterOperationFunction(CKOGUID_EQUAL, CKPGUID_BOOL, CKPGUID_2DVECTOR, CKPGUID_2DVECTOR, CKGenericEqual2Dword);  // call_ea=0x24B5D372
    pm->RegisterOperationFunction(CKOGUID_NOT_EQUAL, CKPGUID_BOOL, CKPGUID_2DVECTOR, CKPGUID_2DVECTOR, CKGenericNotEqual2Dword);  // call_ea=0x24B5D3DA
    pm->RegisterOperationFunction(CKOGUID_EQUAL, CKPGUID_BOOL, CKPGUID_VECTOR, CKPGUID_VECTOR, CKGenericEqual3Dword);  // call_ea=0x24B5D442
    pm->RegisterOperationFunction(CKOGUID_NOT_EQUAL, CKPGUID_BOOL, CKPGUID_VECTOR, CKPGUID_VECTOR, CKGenericNotEqual3Dword);  // call_ea=0x24B5D4AA
    pm->RegisterOperationFunction(CKOGUID_EQUAL, CKPGUID_BOOL, CKPGUID_EULERANGLES, CKPGUID_EULERANGLES, CKGenericEqual3Dword);  // call_ea=0x24B5D512
    pm->RegisterOperationFunction(CKOGUID_NOT_EQUAL, CKPGUID_BOOL, CKPGUID_EULERANGLES, CKPGUID_EULERANGLES, CKGenericNotEqual3Dword);  // call_ea=0x24B5D57A
    pm->RegisterOperationFunction(CKOGUID_EQUAL, CKPGUID_BOOL, CKPGUID_QUATERNION, CKPGUID_QUATERNION, CKGenericEqual4Dword);  // call_ea=0x24B5D5E2
    pm->RegisterOperationFunction(CKOGUID_NOT_EQUAL, CKPGUID_BOOL, CKPGUID_QUATERNION, CKPGUID_QUATERNION, CKGenericNotEqual4Dword);  // call_ea=0x24B5D64A
    pm->RegisterOperationFunction(CKOGUID_EQUAL, CKPGUID_BOOL, CKPGUID_RECT, CKPGUID_RECT, CKGenericEqual4Dword);  // call_ea=0x24B5D6B2
    pm->RegisterOperationFunction(CKOGUID_NOT_EQUAL, CKPGUID_BOOL, CKPGUID_RECT, CKPGUID_RECT, CKGenericNotEqual4Dword);  // call_ea=0x24B5D71A
    pm->RegisterOperationFunction(CKOGUID_EQUAL, CKPGUID_BOOL, CKPGUID_COLOR, CKPGUID_COLOR, CKGenericEqual4Dword);  // call_ea=0x24B5D782
    pm->RegisterOperationFunction(CKOGUID_NOT_EQUAL, CKPGUID_BOOL, CKPGUID_COLOR, CKPGUID_COLOR, CKGenericNotEqual4Dword);  // call_ea=0x24B5D7EA
    pm->RegisterOperationFunction(CKOGUID_EQUAL, CKPGUID_BOOL, CKPGUID_MATRIX, CKPGUID_MATRIX, CKBoolEqualMatrixMatrix);  // call_ea=0x24B5D852
    pm->RegisterOperationFunction(CKOGUID_NOT_EQUAL, CKPGUID_BOOL, CKPGUID_MATRIX, CKPGUID_MATRIX, CKBoolNotEqualMatrixMatrix);  // call_ea=0x24B5D8BA
}
