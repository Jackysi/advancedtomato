/** Copyright (c) 2009, Frank Yellin, (fy@fyellin.com).  All rights reserved.
 *
 *   Author: Frank Yellin, fy@fyellin.com
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in
 *      the documentation and/or other materials provided with the
 *      distribution.
 *
 *
 * This program is intended to be a fast implementation of the AES
 * encryption and decryption algorithms on MIPS machines.
 *
 * It achieves its speed by:
 *
 * 1) Keeping all variables in registers
 * 2) Not building a stack frame
 * 3) Setting up the key schedule to be byte swapped for the first and
 * last round on little-endian machines.  This saves many instructions
 * on input, and a couple of instructions on output.
 */

#include <sys/regdef.h>

#define INPUT  a0
#define OUTPUT a1
#define KEY    a2
#define ROUNDS v0
#define TABLE  v1
#define TABLE_REG 3   /* Actual register number of TABLE */

/* Hold values at the start of even rounds */
#define X0     t0
#define X1     t1
#define X2     t2
#define X3     t3

/* Holds values at the start of odd rounds */
#define Y0     t4
#define Y1     t5
#define Y2     t6
#define Y3     t3   /* Okay that it's the same as X3 */

/* Scratch registers */
#define T0     t7
#define T1     t8
#define T2     t9

/**
 * The following three registers are currently unused.
 *    ('a3', 'AT', 'gp')
 * They are available if anyone wants to extend this code to, for example,
 * encode multiple blocks at a time, or implement CBC
 *
 * During the input phase, we read the values into X0-X3.
 * During the odd rounds, we use X0-X3 as the round input, and put the results
 * Y0-Y3.  During the even rounds, we go the other way.  For the last round,
 * we use Y0-Y3 as input, and write the results directly to the output; X0-X2
 * are available as extra temps.
 *
 * T0,T1,T2 are temporary registers that are implicit arguments to
 * FULL_ROUND, LAST_ROUND, and LOAD_INPUT_WORD. 
 *
 * After the input is read, a0 (INPUT) is also available as a temp.
 * After the next-to-last round, v0 (ROUNDS) is no longer used, either.
 */

/**
 *  Life is simpler if the world is divided into three cases:
 *    MIPSEB                            -- big endian
 *    MIPSEL_DISABLE_KEY_MODIFICATION   -- little endian,  key modification
 *    MIPSEL_ENABLE_KEY_MODIFICATION    -- little endain, no key mod
 * Exactly one of these will be defined.  (For MIPSEB, we really
 * don't care about the value of KEY_MODIFICATION; the algorithm is
 * big-endian anyway)
 */

#ifdef MIPSEL
#  ifdef DISABLE_KEY_MODIFICATION
#    define MIPSEL_DISABLE_KEY_MODIFICATION
#  else
#    define MIPSEL_ENABLE_KEY_MODIFICATION
#  endif
#endif

#ifdef MIPSEL_ENABLE_KEY_MODIFICATION
#  define FIRST BYTESWAPPED
#  define ENABLE_KEY_MODIFICATION 1
#else
#  define FIRST NORMAL
#  define ENABLE_KEY_MODIFICATION 0
#endif

/**
 * Load the address of the encryption or decryption table into register TABLE
 *
 * tableName is the name of the table whose address we want
 * 0: is the label at the very start of the program
 * 9: is the label of a data word containing the data
 *
 * This is surprisingly hard to do in an OS- and ABI-independent manner.
 *
 * The code can't use "la TABLE, tableName" because that pseudo-instruction
 * uses gp, which this code is (purposely) not initializing.  However MIPS
 * calls a library routine using the code sequence:
 *      la   $25, <functionName>
 *      jalr $25
 * so that $25 initially contains the address of the current function.
 *
 * The second alternative, though one instruction longer, is simpler and less
 * implementation dependent. Alas,%lo and %hi don't work correctly for
 * PIC code. 
 *
 * The third alternative is the slowest, but ought to work across all
 * ABIs.  It makes no assumptions about the calling sequence
 *
 * The fourth alternative is how it would be done by traditional code.
 *
 * For unknown reasons, gas treats
 *      lw TABLE, calculated-absolute-offset($25)
 * as a  macro and generates three instructions using AT and gp.
 * So this macro assembles the lw directly.
 */
#define LW(dst,src,offset)\
    .word 0x8c000000 + (src << 21) + (dst << 16) + ((offset) & 0xFFFF)

#define LOAD_ENCRYPTION_TABLE_ALT1(tableName) \
    LW(TABLE_REG, 25, 9f - 0b);

#define LOAD_ENCRYPTION_TABLE_ALT2(tableName) \
    lui   TABLE, %hi(tableName); \
    addi  TABLE, %lo(tableName);

#define LOAD_ENCRYPTION_TABLE_ALT3(tableName) \
      move  t0, ra; \
  10: bal   11f; \
  11: LW(TABLE_REG, 31, 9f - (10b + 8)); \
      move ra, t0

#define LOAD_ENCRYPTION_TABLE_ALT4(tableName) \
      .set noreorder; \
      .cpload $25; \
      .set reorder; \
      la TABLE, tableName

#define LOAD_ENCRYPTION_TABLE LOAD_ENCRYPTION_TABLE_ALT1

/**
 * Load the four bytes starting at INPUT + offset as an int into the specified
 * target register
 *
 * If MIPSEL_ENABLE_KEY_MODIFICATION, treat the four bytes as little-endian;
 * otherwise, treat them as big endian.
 *
 * Uses T0, T1, T2 as temporaries
 */

#ifdef MIPSEL_DISABLE_KEY_MODIFICATION
#  define LOAD_INPUT_WORD(target, offset) \
      /* Get the four bytes */ \
      lbu T0, offset + 0(INPUT); \
      lbu T1, offset + 1(INPUT); \
      lbu T2, offset + 2(INPUT); \
      lbu target, offset + 3(INPUT); \
      /* Shift appropriately */     \
      sll T0, 24; sll T1, 16; sll T2, 8; \
      /* Join the values together */  \
      or T0, T1; or T0, T2; or target, T0
#else
#  define LOAD_INPUT_WORD(target, offset) \
      /* Load the four bytes in native endian format */ \
      ulw target, offset(INPUT)
#endif

/**
 * This is the heart of the AES algorithm. Calculate
 *      out = Tx0[(in0 >> 24) & 0xFF]
 *          ^ Tx1[(in1 >> 16) & 0xFF]
 *          ^ Tx2[(in2 >> 8) & 0xFF]
 *          ^ Tx3[(in3 >> 0) & 0xFF]
 *          ^ key[keyIndex]
 * Where the tables are TE0..TE3 for encryption and TD0..TD3 for decryption.
 * (The generated code is the same in either case.)
 *
 * The arg 'where' will either be NORMAL or BYTESWAPPED.
 * If it is BYTESWAPPED, then the input is byteswapped because this is the
 * first round and MIPSEL_ENABLE_KEY_MODIFICATION is defined.  This shifts in
 * the above calculation are adjusted accordingly, but the output is not
 * affected.
 *
 * "out" is allowed to be the same as one of the ins, as this code
 * code doesn't modify this register until it is finished with
 * the four input registers.
 *
 * Uses T0, T1, T2 as temporaries
 */

#define FULL_ROUND(out, in0, in1, in2, in3, keyIndex, where) \
    /* Get the five values that need to be xor'ed */ \
    GET_TABLE_WORD(T0, in0, 0, 24, where); \
    GET_TABLE_WORD(T1, in1, 1, 16, where); \
    GET_TABLE_WORD(T2, in2, 2,  8, where); \
    GET_TABLE_WORD(out,in3, 3,  0, where); \
    xor T0, T1; \
    lw  T1, 4 * keyIndex(KEY); \
    xor T0, T2; \
    xor T0, out; \
    xor out, T0, T1

/**
 * Perform the final round of the AES algorithm. Calculate:
 *      byte0 = Tx4[(in0 >> 24) & 0xFF] ^ ((key[keyIndex] >> 24) & 0xFF)
 *      byte1 = Tx4[(in1 >> 16) & 0xFF] ^ ((key[keyIndex] >> 16) & 0xFF)
 *      byte2 = Tx4[(in1 >>  8) & 0xFF] ^ ((key[keyIndex] >> 8)  & 0xFF)
 *      byte3 = Tx4[(in1 >>  8) & 0xFF] ^ ((key[keyIndex] >> 0)  & 0xFF)
 * Write out the four resulting bytes to the four bytes of memory starting
 * at OUTPUT + outputOff.  Use TE4 for encryption and TD4 for decryption.
 *
 * IF MIPSEL_ENABLE_KEY_MODIFICATION, the word at key[keyIndex] is
 * byte-swapped, and the shifts of key[keyIndex] should be adjusted
 * accordingly.  (See GENERATE_OUTPUT for more info).
 *
 * The variable "type" will either be ENCRYPT or DECRYPT.
 *
 * Uses T0, T0, T1, xtemp0, xtemp1, xtemp2 as temporaries
 */

#define LAST_ROUND(in0, in1, in2, in3, keyIndex, outputOff, type, \
                   xtemp0, xtemp1, xtemp2)  \
    lw   T0, 4 * keyIndex(KEY); \
    GET_LAST_TABLE_WORD_ ## type(T1, in0, 24); \
    GET_LAST_TABLE_WORD_ ## type(T2, in1, 16); \
    GET_LAST_TABLE_WORD_ ## type(xtemp0, in2,  8); \
    GET_LAST_TABLE_WORD_ ## type(xtemp1, in3,  0); \
    GENERATE_OUTPUT(T0, T1, T2, xtemp0, xtemp1, outputOff, xtemp2)

/**
 * Sets target to the value
 *     TE"table"[(src >> shift) & 0xFF]        if where==NORMAL
 *     TE"table"[(src >> (32 - shift)) & 0xFF] if where==BYTESWAPPED
 */

#define GET_TABLE_WORD(target, src, table, shift, where) \
    GET_WORD_OFFSET_ ## where ## _ ## shift(target, src); \
    add target, TABLE; \
    lw  target, (0x400 * table)(target)

/**
 * Sets target = TE4[(src >> byte) & 0xFF]
 *
 * Note that TE4 doesn't really exist, but the code instead uses a middle byte
 * of TE0. This works on both little and big-endian machines, since the middle
 * two bytes of TE0 (either of which could be "Byte 1") are the same.
 */

#define GET_LAST_TABLE_WORD_ENCRYPT(target, src, shift) \
    GET_WORD_OFFSET_NORMAL_ ## shift(target, src); \
    add target, TABLE; \
    lbu  target, 1(target)

/** Sets target = TD4[(src >> shift) & 0xFF] */

#define GET_LAST_TABLE_WORD_DECRYPT(target, src, shift) \
    GET_BYTE_OFFSET_NORMAL_ ## shift(target, src); \
    add target, TABLE; \
    lbu  target, 4 * 0x400(target)

/**
 * Converts the specified byte of src into an offset for a word-sized array.
 * The input is little-endian, but the shift is specified big-endian, so we
 * adjust accordingly.
 */

#define GET_WORD_OFFSET_BYTESWAPPED_24(target, src) \
       GET_WORD_OFFSET_NORMAL_0(target, src)

#define GET_WORD_OFFSET_BYTESWAPPED_16(target, src) \
       GET_WORD_OFFSET_NORMAL_8(target, src)

#define GET_WORD_OFFSET_BYTESWAPPED_8(target, src) \
       GET_WORD_OFFSET_NORMAL_16(target, src)

#define GET_WORD_OFFSET_BYTESWAPPED_0(target, src) \
       GET_WORD_OFFSET_NORMAL_24(target, src)

/**
 * Converts the specified big-endian byte of src into an offset for a
 * word sized array.
 */

#define GET_WORD_OFFSET_NORMAL_24(target, src) \
      srl target, src, 22; andi target, 0xFF<<2
#define GET_WORD_OFFSET_NORMAL_16(target, src) \
      srl target, src, 14; andi target, 0xFF<<2
#define GET_WORD_OFFSET_NORMAL_8(target, src) \
      srl target, src,  6; andi target, 0xFF<<2
#define GET_WORD_OFFSET_NORMAL_0(target, src) \
      andi target, src, 0xFF; sll target, 2

/**
 * Converts the specified big-endian byte of src into an offset
 * for a byte-sized array
 */

#define GET_BYTE_OFFSET_NORMAL_24(target, src) \
      srl target, src, 24  /* No masking needed */
#define GET_BYTE_OFFSET_NORMAL_16(target, src) \
      srl target, src, 16; andi target, 0xFF
#define GET_BYTE_OFFSET_NORMAL_8(target, src) \
      srl target, src,  8; andi target, 0xFF
#define GET_BYTE_OFFSET_NORMAL_0(target, src) \
      andi target, src, 0xFF

/**
 * Calculate out0 = byte0 ^ ((key >> 24) & 0xFF)
 *           out1 = byte1 ^ ((key >> 16) & 0xFF)
 *           out2 = byte2 ^ ((key >> 8)  & 0xFF)
 *           out3 = byte3 ^ ((key >> 0)  & 0xFF)
 * and then write out these four bytes to the four bytes of
 * memory starting at OUTPUT + outputOff
 *
 * IF MIPSEL_ENABLE_KEY_MODIFICATION, the value of key is byte-swapped.
 *
 * We are guaranteed that 0 <= byte0, byte1, byte2, byte3 <= 255.
 */

#ifdef MIPSEL_DISABLE_KEY_MODIFICATION
#  define GENERATE_OUTPUT(key, byte0, byte1, byte2, byte3, outOffset, temp) \
     /* xor appropriate pieces of the key into the bytes */ \
     srl temp, key, 24; \
     xor byte0, temp; \
     sb byte0, outOffset+0(OUTPUT); \
     srl temp, key, 16; \
     xor byte1, temp; \
     sb byte1, outOffset+1(OUTPUT); \
     srl temp, key,  8; \
     xor byte2, temp; \
     sb byte2, outOffset+2(OUTPUT); \
     xor byte3, key; \
     sb byte3, outOffset+3(OUTPUT)
#else
#  define GENERATE_OUTPUT(key, byte0, byte1, byte2, byte3, outOffset, temp) \
     /* Shift the bytes to the appropriate output position */ \
     SHIFT_BYTES_FOR_OUTPUT(byte0, byte1, byte2, byte3); \
     xor key, byte0; \
     xor key, byte1; \
     xor key, byte2; \
     xor key, byte3; \
     usw key, outOffset(OUTPUT)
#  if defined MIPSEL_ENABLE_KEY_MODIFICATION
#    define SHIFT_BYTES_FOR_OUTPUT(byte0, byte1, byte2, byte3) \
       sll byte1, 8; \
       sll byte2, 16; \
       sll byte3, 24;
#  else
#    define SHIFT_BYTES_FOR_OUTPUT(byte0, byte1, byte2, byte3) \
       sll byte0, 24; \
       sll byte1; 16, \
       sll byte2, 8;
#  endif
#endif

/**
 * The value returned by AES_swap_bytes() indicates whether the
 * key schedule constructor needs to swap the first four and last four words
 * of the key schedule
 */

      .text

	.align 2
	.globl AES_swap_bytes
	.ent AES_swap_bytes
	.type AES_swap_bytes,@function
	.set	noat
AES_swap_bytes:

         li  v0, ENABLE_KEY_MODIFICATION
         jr  ra
	.set	at
	.end	AES_swap_bytes

	.align 2
	.globl AES_encrypt
	.ent AES_encrypt
	.type AES_encrypt,@function
	.set	noat
AES_encrypt:
0: 	# Get number of rounds and AES_Table
	LOAD_ENCRYPTION_TABLE(AES_Encrypt_Table)
	lw    ROUNDS, 240(KEY)

	LOAD_INPUT_WORD(X0,  0)
	LOAD_INPUT_WORD(X1,  4)
	LOAD_INPUT_WORD(X2,  8)
	LOAD_INPUT_WORD(X3, 12)
	lw    T0, 0(KEY)
	lw    T1, 4(KEY)
	lw    T2, 8(KEY)
	lw    Y0, 12(KEY)
	xor   X0, T0
	xor   X1, T1
	xor   X2, T2
	xor   X3, Y0

	# First Round: (Y0, Y1, Y2, Y3) := round(X0, X1, X2, X3)
	FULL_ROUND(Y0, X0, X1, X2, X3,  4, FIRST)
	FULL_ROUND(Y1, X1, X2, X3, X0,  5, FIRST)
	FULL_ROUND(Y2, X2, X3, X0, X1,  6, FIRST)
	FULL_ROUND(Y3, X3, X0, X1, X2,  7, FIRST)
	addi  ROUNDS, -2

1: 	# Odd Round: (X0, X1, X2, X3) := round(Y0, Y1, Y2, Y3)
	FULL_ROUND(X0, Y0, Y1, Y2, Y3,  8, NORMAL)
	FULL_ROUND(X1, Y1, Y2, Y3, Y0,  9, NORMAL)
	FULL_ROUND(X2, Y2, Y3, Y0, Y1, 10, NORMAL)
	FULL_ROUND(X3, Y3, Y0, Y1, Y2, 11, NORMAL)

	# Even Round: (Y0, Y1, Y2, Y3) := round(X0, X1, X2, X3)
	FULL_ROUND(Y0, X0, X1, X2, X3, 12, NORMAL)
	FULL_ROUND(Y1, X1, X2, X3, X0, 13, NORMAL)
	FULL_ROUND(Y2, X2, X3, X0, X1, 14, NORMAL)
	FULL_ROUND(Y3, X3, X0, X1, X2, 15, NORMAL)

	addi  ROUNDS, -2
	addi  KEY, 32
	bnez  ROUNDS, 1b

	# Last Round: OUTPUT := last_round(Y0, Y1, Y2, Y3)
	LAST_ROUND(Y0, Y1, Y2, Y3,  8,  0, ENCRYPT, X0, X1, X2)
	LAST_ROUND(Y1, Y2, Y3, Y0,  9,  4, ENCRYPT, X0, X1, X2)
	LAST_ROUND(Y2, Y3, Y0, Y1, 10,  8, ENCRYPT, X0, X1, X2)
	LAST_ROUND(Y3, Y0, Y1, Y2, 11, 12, ENCRYPT, X0, X1, X2)
	jr    ra
9: 	.word AES_Encrypt_Table
	.set	at
	.end	AES_encrypt

	.align 2
	.globl AES_decrypt
	.ent AES_decrypt
	.type AES_decrypt,@function
	.set	noat
AES_decrypt:
0: 	# Get number of rounds and AES_Table
	LOAD_ENCRYPTION_TABLE(AES_Decrypt_Table)
	lw    ROUNDS, 240(KEY)

	LOAD_INPUT_WORD(X0,  0)
	LOAD_INPUT_WORD(X1,  4)
	LOAD_INPUT_WORD(X2,  8)
	LOAD_INPUT_WORD(X3, 12)
	lw    T0, 0(KEY)
	lw    T1, 4(KEY)
	lw    T2, 8(KEY)
	lw    Y0, 12(KEY)
	xor   X0, T0
	xor   X1, T1
	xor   X2, T2
	xor   X3, Y0

	# First Round: (Y0, Y1, Y2, Y3) := round(X0, X1, X2, X3)
	FULL_ROUND(Y0, X0, X3, X2, X1,  4, FIRST)
	FULL_ROUND(Y1, X1, X0, X3, X2,  5, FIRST)
	FULL_ROUND(Y2, X2, X1, X0, X3,  6, FIRST)
	FULL_ROUND(Y3, X3, X2, X1, X0,  7, FIRST)
	addi  ROUNDS, -2

1: 	# Odd Round: (X0, X1, X2, X3) := round(Y0, Y1, Y2, Y3)
	FULL_ROUND(X0, Y0, Y3, Y2, Y1,  8, NORMAL)
	FULL_ROUND(X1, Y1, Y0, Y3, Y2,  9, NORMAL)
	FULL_ROUND(X2, Y2, Y1, Y0, Y3, 10, NORMAL)
	FULL_ROUND(X3, Y3, Y2, Y1, Y0, 11, NORMAL)

	# Even Round: (Y0, Y1, Y2, Y3) := round(X0, X1, X2, X3)
	FULL_ROUND(Y0, X0, X3, X2, X1, 12, NORMAL)
	FULL_ROUND(Y1, X1, X0, X3, X2, 13, NORMAL)
	FULL_ROUND(Y2, X2, X1, X0, X3, 14, NORMAL)
	FULL_ROUND(Y3, X3, X2, X1, X0, 15, NORMAL)

	addi  ROUNDS, -2
	addi  KEY, 32
	bnez  ROUNDS, 1b

	# Last Round: OUTPUT := last_round(Y0, Y1, Y2, Y3)
	LAST_ROUND(Y0, Y3, Y2, Y1,  8,  0, DECRYPT, X0, X1, X2)
	LAST_ROUND(Y1, Y0, Y3, Y2,  9,  4, DECRYPT, X0, X1, X2)
	LAST_ROUND(Y2, Y1, Y0, Y3, 10,  8, DECRYPT, X0, X1, X2)
	LAST_ROUND(Y3, Y2, Y1, Y0, 11, 12, DECRYPT, X0, X1, X2)
	jr    ra
9: 	.word AES_Decrypt_Table
	.set	at
	.end	AES_decrypt

