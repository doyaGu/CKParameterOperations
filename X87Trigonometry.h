#ifndef X87_TRIG_H
#define X87_TRIG_H

#include <math.h>
#include <stdint.h>
#include <string.h>

enum x87_trig_op { X87_SINE, X87_COSINE, X87_TANGENT };

/* Intel SDM vol. 1, "Approximation of Pi": x87 pi/4 is exactly
 * 0x3243f6a8885a308d3 * 2^-66. Reduce in integer units of 2^-66 so
 * large angles keep the low bits of the remainder, even near a pole.
 * The caller supplies a positive binary32 magnitude in [0.5, 2^63). */
static inline double x87_reduce_angle(uint32_t magnitude, unsigned *quadrant)
{
    const uint64_t octant_hi = UINT64_C(0x3243f6a88);
    const uint32_t octant_lo = 0x85a308d3u;
    const uint32_t significand = 0x800000u | (magnitude & 0x7fffffu);
    /* Start at significand * 2^40, then shift to the input exponent.
     * hi:lo is the remainder, with a 32-bit low limb. */
    uint64_t hi = (uint64_t)significand << 8;
    uint32_t lo = 0;
    unsigned octant = 0;
    for (unsigned shifts = (magnitude >> 23) - 124; shifts > 0; --shifts) {
        hi = (hi << 1) | (lo >> 31);
        lo <<= 1;
        octant <<= 1;
        if (hi > octant_hi || (hi == octant_hi && lo >= octant_lo)) {
            hi -= octant_hi + (lo < octant_lo);
            lo -= octant_lo;
            ++octant;
        }
    }
    *quadrant = ((octant + 1) >> 1) & 3;
    /* In odd octants take the distance to the next quadrant before rounding. */
    if (octant & 1) {
        hi = octant_hi - hi - (octant_lo < lo);
        lo = octant_lo - lo;
    }
    const double radians = (double)hi * 0x1p-34 + (double)lo * 0x1p-66;
    return octant & 1 ? -radians : radians;
}

static inline uint32_t x87_trig_word(enum x87_trig_op op, uint32_t input)
{
    const uint32_t magnitude = input & 0x7fffffffu;
    if (magnitude > 0x7f800000u) return input | 0x00400000u;
    if (magnitude == 0x7f800000u) return 0xffc00000u;
    /* With C2 set, FPTAN skips its push. CK2 pops twice on the otherwise
     * empty x87 stack and stores the masked stack-underflow indefinite. */
    if (magnitude >= 0x5f000000u) return op == X87_TANGENT ? 0xffc00000u : input;
    if (magnitude == 0) return op == X87_COSINE ? 0x3f800000u : input;

    unsigned quadrant = 0;
    double angle;
    if (magnitude < 0x3f000000u) {
        float value;
        memcpy(&value, &magnitude, sizeof(value));
        angle = value;
    } else {
        angle = x87_reduce_angle(magnitude, &quadrant);
    }
    double result;
    if (op == X87_TANGENT) {
        result = tan(angle);
        if (quadrant & 1) result = -1.0 / result;
    } else {
        if (op == X87_COSINE) ++quadrant;
        result = quadrant & 1 ? cos(angle) : sin(angle);
        if (quadrant & 2) result = -result;
    }
    if (op != X87_COSINE && (input & 0x80000000u)) result = -result;
    const float value = (float)result;
    uint32_t word;
    memcpy(&word, &value, sizeof(word));
    return word;
}

#endif
