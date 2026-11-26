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

/* Carry the binary64 product and division residuals through one Taylor term.
 * Midpoints of adjacent FLOATs have at most 25 significant bits, so m*m and
 * x*m below are exact in binary64. */
struct x87_atan_term { double high, low; };

static inline struct x87_atan_term x87_next_atan_term(struct x87_atan_term t, double factor, int divisor)
{
    const double product = t.high * factor;
    const double next = product / divisor;
    const double tail = fma(-next, divisor, product) + fma(t.high, factor, -product) + t.low * factor;
    const struct x87_atan_term result = {next, tail / divisor};
    return result;
}

static inline void x87_accumulate_atan_term(double *sum, double *error, struct x87_atan_term term)
{
    const double total = *sum + term.high;
    const double part = total - *sum;
    *error += ((*sum - (total - part)) + (term.high - part)) + term.low;
    *sum = total;
}

/* Near a FLOAT midpoint m use y*cos(m) - x*sin(m) to locate the angle.
 * Twenty-four terms of each series suffice on [0, pi] (pi^48/48! < 2^-123).
 * FPATAN leaves a 64-significand-bit angle before FSTP rounds to binary32. */
static inline uint32_t x87_round_atan2(double y, double x)
{
    const double angle = atan2(y, x);  /* y is positive */
    const float rounded = (float)angle;
    uint32_t candidate;
    memcpy(&candidate, &rounded, sizeof(candidate));
    if ((double)rounded == angle) return candidate;
    const uint32_t lower = candidate - ((double)rounded > angle);
    const uint32_t upper = lower + 1;
    float low, high;
    memcpy(&low, &lower, sizeof(low));
    memcpy(&high, &upper, sizeof(high));
    const double mid = ((double)low + high) * 0.5;
    if (fabs(angle - mid) > mid * 0x1p-50) return candidate;

    /* CK2's FPATAN stores the lower neighbor on this measured slope, even
     * though correctly rounding the angle to extended precision gives the
     * midpoint. The binary32 coordinates times these 23-bit integers are exact. */
    if (x * 6274657.0 == -y * 5098559.0) return lower;

    struct x87_atan_term even = {y, 0.0}, odd = {-x * mid, 0.0};
    const double factor = -(mid * mid);
    double sum = 0.0, error = 0.0;
    for (int n = 1; n <= 47; n += 2) {
        x87_accumulate_atan_term(&sum, &error, even);
        x87_accumulate_atan_term(&sum, &error, odd);
        even = x87_next_atan_term(even, factor, n * (n + 1));
        odd = x87_next_atan_term(odd, factor, (n + 1) * (n + 2));
    }
    const double cross = sum + error;
    const double half_ulp = ldexp(hypot(y, x), ilogb(mid) - 64);
    if (cross > half_ulp) return upper;
    /* The midpoint above zero is a power of two, with a smaller ulp below. */
    if (cross < -(lower == 0 ? half_ulp * 0.5 : half_ulp)) return lower;
    return lower + (lower & 1u);
}

/* FPATAN operates on two registers. Both FLDs quiet their NaN before
 * selection by significand, with positive preferred for equal payloads. */
static inline uint32_t x87_atan2_word(uint32_t y, uint32_t x)
{
    const uint32_t infinity = 0x7f800000u;
    uint32_t ym = y & 0x7fffffffu, xm = x & 0x7fffffffu;
    if (ym > infinity || xm > infinity) {
        if (ym > infinity) ym |= 0x00400000u;
        if (xm > infinity) xm |= 0x00400000u;
        const uint32_t selected = xm > ym || (xm == ym && x < y) ? x : y;
        return selected | 0x00400000u;
    }

    const uint32_t sign = y & 0x80000000u;
    const int negative_x = (x & 0x80000000u) != 0;
    if (ym == infinity && xm == infinity) return sign | (negative_x ? 0x4016cbe4u : 0x3f490fdbu);
    if (ym == 0 || xm == infinity) return sign | (negative_x ? 0x40490fdbu : 0u);
    if (xm == 0 || ym == infinity) return sign | 0x3fc90fdbu;

    float yf, xf;
    memcpy(&yf, &y, sizeof(yf));
    memcpy(&xf, &x, sizeof(xf));
    return sign | x87_round_atan2(fabs((double)yf), (double)xf);
}

#endif
