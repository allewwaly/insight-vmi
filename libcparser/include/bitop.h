#ifndef BITOP_H
#define BITOP_H

#include <QtGlobal>

/**
 * Cyclic rotate a 32 bit integer bitwise to the left
 * @param i value to rotate bitwise
 * @param rot_amount the number of bits to rotate
 * @return the value of \a i cyclicly rotated y \a rot_amount bits
 */
inline uint rotl32(uint i, uint rot_amount)
{
    rot_amount %= 32;
    uint mask = (1 << rot_amount) - 1;
    return (i << rot_amount) | ((i >> (32 - rot_amount)) & mask);
}

#endif // BITOP_H
