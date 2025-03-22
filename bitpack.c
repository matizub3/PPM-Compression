/**************************************************************
*
*                     bitpack.c
*
*       Assignment: arith
*       Authors:    Mateusz, Annica
*       Date:       03/07/25
*
*       bitpack.c implements the Bitpack interface for packing and
*       unpacking multiple fields into a 64-bit word. It includes functions
*       to test whether a value fits in a specified number of bits, extract 
*       fields (signed and unsigned), and update fields. 
*
*       Functions include:
*               - Bitpack_fitsu: checks if a value fits in a given width
*               - Bitpack_fitss: checks if a signed value fits in a given width
*               - Bitpack_getu: extracts a bitfield from a word
*               - Bitpack_gets: extracts a signed bitfield from a word
*               - Bitpack_newu: inserts a bitfield into a word
*               - Bitpack_news: inserts a signed bitfield into a word
*
**************************************************************/
#ifndef BITPACK_INCLUDED
#define BITPACK_INCLUDED

#include "bitpack.h"

#include <stdbool.h>
#include <stdint.h>
#include "except.h"
#include "math.h"
#include "assert.h"
#include <stdio.h>

/* Exception for overflow */
Except_T Bitpack_Overflow = { "Overflow packing bits" };

/* Private helper function declarations */
static uint64_t left_shift(uint64_t n, unsigned shift);
static uint64_t right_shift_unsigned(uint64_t n, unsigned shift);
static uint64_t right_shift_signed(int64_t n, unsigned shift);


/******************************************************************************
 * 
 *                          WIDTH TEST FUNCTIONS
 *
 *****************************************************************************/


/********** Bitpack_fitsu **********
 *
 * Determines whether an unsigned integer n can be represented in a given number
 * of bits.
 *
 * Parameters:
 *      uint64_t n      - unsigned 64-bit integer
 *      unsigned width  - unsigned number of bits available for representing n
 *
 * Return:
 *      true, if an unsigned n can be represented using width number of bit  
 *      false, otherwise
 *
 * Expects:
 *      width <= 64
 *
 * Notes:
 *      If width is 0, then only the value 0 is representable
 *      Throws CRE if width > 64
 ************************/
bool Bitpack_fitsu(uint64_t n, unsigned width)
{
        /* check if width is less than 64 */
        assert(width <= 64);

        /* check if width is 0 */
        if (width == 0) {
                return n == 0;
        }
        
        /* left shift & return */
        return left_shift(1, width) - 1 >= n;
}


/********** Bitpack_fitss **********
 *
 * Determines whether a signed integer n can be represented in a given number
 *
 * Parameters:
 *      int64_t n       - signed integer 
 *      unsigned width  - unsigned number of bits available for representing n
 *
 * Return:
 *      true, if a signed n can be represented using width number of bit  
 *      false, otherwise
 *
 * Expects:
 *      width <= 64
 *      
 *
 * Notes:
 *      If width is 0, then only the value 0 fits in 0 bits
 *      Throws CRE if width > 64
 ************************/
bool Bitpack_fitss(int64_t n, unsigned width)
{
        /* check if width is less than 64 */
        assert(width <= 64);

        /* check if width is 0 */
        if (width == 0) {
                return n == 0;
        }

        /* check if n is negative */
        if (n < 0) {
                /* left shift the corresponding unsigned value */
                uint64_t corresponding_unsigned = -n;
                return left_shift(1, width - 1) >= corresponding_unsigned;
        } else {
                /* left shift the unsigned value */
                return left_shift(1, width - 1) - 1 >= (uint64_t)n;
        }
}


/******************************************************************************
 * 
 *                          FIELD EXTRACT FUNCTIONS
 *
 *****************************************************************************/


/********** Bitpack_getu **********
 *
 * Extracts an unsigned field from a 64-bit word
 *
 * Parameters:
 *      uint64_t word   - unsigned 64-bit word from which to extract the field
 *      unsigned width  - unsigned width (in bits) of the field
 *      unsigned lsb    - least-significant bit position of the field
 *
 * Return:
 *      The unsigned integer value extracted from the specified field
 *
 * Expects:
 *      width <=0 64 and width + lsb <= 64
 *
 * Notes:
 *      A field of width 0 returns 0
 *      It is a CRE if width > 64 or if width + lsb > 64
 ************************/
uint64_t Bitpack_getu(uint64_t word, unsigned width, unsigned lsb)
{
        /* check if width is less than 64 */
        assert(width <= 64);
        assert(width + lsb <= 64);

        /* check if width is 0 */
        if (width == 0) {
                return 0;
        }

        /* make the mask */
        uint64_t mask = __UINT64_MAX__;
        mask = right_shift_unsigned(mask, 64 - width);
        mask = left_shift(mask, lsb);
        
        /* get the values */
        word &= mask;

        /* align the values */
        return right_shift_unsigned(word, lsb);
}


/********** Bitpack_gets **********
 *
 * Extracts a signed field from a 64-bit word
 *
 * Parameters:
 *      uint64_t word   - unsigned 64-bit word from which to extract the field
 *      unsigned width  - unsigned width (in bits) of the field
 *      unsigned lsb    - least-significant bit position of the field
 *
 * Return:
 *      The signed integer value extracted from the specified field
 *
 * Expects:
 *      width <= 64 and width + lsb <= 64
 *
 * Notes:
 *      A field of width 0 returns 0
 *      It is a CRE if width > 64 or if width + lsb > 64
 ************************/
int64_t Bitpack_gets(uint64_t word, unsigned width, unsigned lsb)
{
        /* check if width is less than 64 */
        assert(width <= 64);
        assert(width + lsb <= 64);

        /* check if width is 0 */
        if (width == 0) {
                return 0;
        }

        /* make the mask */
        uint64_t mask = __UINT64_MAX__;
        mask = right_shift_unsigned(mask, 64 - width);
        mask = left_shift(mask, lsb);

        /* get the values */
        word &= mask;

        /* align the values */
        word = right_shift_signed(word, lsb);
        word = left_shift(word, 64 - width);
        word = right_shift_signed(word, 64 - width);

        return word;
}


/******************************************************************************
 * 
 *                          FIELD UPDATE FUNCTIONS
 *
 *****************************************************************************/


/********** Bitpack_newu **********
 *
 * Inserts an unsigned integer value into a specified field of a 64-bit word
 *
 * Parameters:
 *      uint64_t word   - unsigned 64-bit original word
 *      unsigned width  - unsigned width (in bits) of the field
 *      unsigned lsb    - unsigned least-significant bit position of the field
 *      uint64_t value  - unsigned 64-bit value to insert
 *
 * Return:
 *      new 64-bit word with the specified field replaced by value
 *
 * Expects:
 *      width <= 64 and width + lsb <= 64.
 *      The value must fit in width
 *
 * Notes:
 *      It is a CRE if the value does not fit in width
 *      It is a CRE if width > 64 or width + lsb > 64
 ************************/
uint64_t Bitpack_newu(uint64_t word, unsigned width, unsigned lsb, 
                      uint64_t value)
{
        /* check if width is less than 64 */
        assert(width <= 64);
        assert(width + lsb <= 64);

        /* check if value fits in width */
        if (!Bitpack_fitsu(value, width)) {
                RAISE(Bitpack_Overflow);
        }

        /* make the mask */
        uint64_t mask = __UINT64_MAX__;
        mask = right_shift_unsigned(mask, 64 - width);
        mask = left_shift(mask, lsb);

        /* clear the field in word */
        word &= ~mask;

        /* shift & return the value */
        return word | left_shift(value, lsb);
}


/********** Bitpack_news **********
 *
 * Inserts a signed integer value into a specified field of a 64-bit word
 *
 * Parameters:
 *      uint64_t word   - unsigned 64-bit original word
 *      unsigned width  - unsigned width (in bits) of the field
 *      unsigned lsb    - least-significant bit position of the field
 *      int64_t value   - signed 64-bit value to insert
 *
 * Return:
 *      A new 64-bit word with the specified field replaced by value
 *
 * Expects:
 *      width <= 64 and width + lsb <= 64
 *      The value must fit in width
 *
 * Notes:
 *      It is a CRE if the value does not fit in width
 *      It is a CRE if width > 64 or width + lsb > 64
 ************************/
uint64_t Bitpack_news(uint64_t word, unsigned width, unsigned lsb,  
                      int64_t value) 
{
        /* check if width is less than 64 */
        assert(width <= 64);
        assert(width + lsb <= 64);

        /* check if value fits in width */
        if (!Bitpack_fitss(value, width)) {
                RAISE(Bitpack_Overflow);
        }

        /* make the mask */
        uint64_t mask = __UINT64_MAX__;
        mask = right_shift_unsigned(mask, 64 - width);
        mask = left_shift(mask, lsb);

        /* clear the field in word */
        word &= ~mask;

        /* insert the value */
        value = left_shift(value, 64 - width);
        value = right_shift_unsigned(value, 64 - width);
        value = left_shift(value, lsb);

        /* return the new word */
        return word | value;
}


/******************************************************************************
 * 
 *                          PRIVATE HELPER FUNCTIONS
 *
 *****************************************************************************/

 
/********** left_shift **********
 *
 * Left-shifts a 64-bit unsigned integer by a specified number of bits
 *
 * Parameters:
 *      uint64_t n      - The number to shift
 *      unsigned shift  - The number of bits to shift left
 *
 * Return:
 *      The result of shifting n left by shift bits, or 0 if shift >= 64
 *
 * Expects:
 *      None
 *
 * Notes:
 *      Function prevents the shift amount from exceeding 64 bits
 ************************/
static uint64_t left_shift(uint64_t n, unsigned shift)
{
        /* check if shift is greater than 64, return 0 */
        if (shift >= 64) {
                return 0;
        }
        /* shift the value */
        return n << shift;
}


/********** right_shift_unsigned **********
 *
 * Right shifts a 64-bit unsigned integer by a specified number of bits
 *
 * Parameters:
 *      uint64_t n      - The number to shift
 *      unsigned shift  - The number of bits to shift right
 *
 * Return:
 *      The result of shifting n right by shift bits, or 0 if shift >= 64
 *
 * Expects:
 *      None
 *
 * Notes:
 *      Function prevents the shift amount from exceeding 64 bits
 ************************/
static uint64_t right_shift_unsigned(uint64_t n, unsigned shift)
{
        /* check if shift is greater than 64, return 0 */
        if (shift >= 64) {
                return 0;
        }
        /* shift the value */
        return n >> shift;
}


/********** right_shift_signed **********
 *
 * Right shift on on a 64-bit signed integer, preserving the sign bit
 *
 * Parameters:
 *      int64_t n       - The signed integer to shift
 *      unsigned shift  - The number of bits to shift right
 *
 * Return:
 *      The result of shifting n right by shift bits
 *      If shift >= 64, 
 *              returns __UINT64_MAX__ if n is negative 
 *              returns 0 if n is non-negative
 *
 * Expects:
 *      None
 *
 * Notes:
 *      Function prevents the shift amount from exceeding 64 bits
 ************************/
static uint64_t right_shift_signed(int64_t n, unsigned shift)
{
        /* check if shift is greater than 64, return max or 0 */
        if (shift >= 64) {
                if (n < 0) {
                        return __UINT64_MAX__;
                } else {
                        return 0;
                }
        }
        /* shift the value */
        return n >> shift;
}

#endif