#ifndef RANDOM_H
#define RANDOM_H

const uint g_randomMultiplier  = 0xCE66DU;
const uint g_randomAddend      = 0xBU;
const uint g_randomMask        = (1U << 24U) - 1;

uint random_next_seed(uint seed) {
    return (seed * g_randomMultiplier + g_randomAddend) & g_randomMask;
}

int random_next_int(uint seed, int min, int max) {
    int value = int(seed >> 16U);
    return min + value % (max - min);
}

float random_next_float(uint seed) {
    return float(random_next_int(seed, 0, 100)) / 100.0;
}

#endif // RANDOM_H
