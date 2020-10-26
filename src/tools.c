#include "tools.h"

void u64_lshift1(uint64_t *in, uint64_t n) {
    int carry = 0;
    for (uint64_t i = 0; i != n; ++i) {
        in[i] <<= 1;
        if (carry) {
            in[i] |= 0x01U;
        } else {
            in[i] &= ~0x01U;
        }
        carry = (in[i] & 0x80U) != 0;
    }
}

unsigned char set_bit(unsigned char in, unsigned short i, unsigned b) {
    if (b) {
        return in | (unsigned char)(0x01U << i);
    }
    return in & (unsigned char)(~(0x01U << i));
}
