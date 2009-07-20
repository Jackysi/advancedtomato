#!/usr/bin/python

# Copyright (c) 2009, Frank Yellin, (fy@fyellin.com).  All rights reserved.
# 
# Author: Frank Yellin, fy@fyellin.com
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer. 
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.

import os
import sys
import optparse

RegisterUsage = {
          'X0': 't0', 'X1': 't1', 'X2': 't2', 'X3': 't3',
          'Y0': 't4', 'Y1': 't5', 'Y2': 't6', 'Y3': 't3',
          'T0': 't7', 'T1': 't8', 'T2': 't9',
          'KEY': 'a2', 'INPUT':'a0', 'OUTPUT':'a1',
          'ROUNDS': 'v0',
          'TABLE': 'v1',
          'unused': ','.join(('g3', 'AT', 'a3',))
          }

AllRegisters = ('t0', 't1', 't2', 't3', 't4', 't5', 't6', 't7',
                't8', 't9', 'a0', 'a1', 'a2', 'a3', 'v0', 'v1',
                'gp', 'AT')

UnusedRegisters = tuple(set(AllRegisters) - set(RegisterUsage.values()))

# Note that the code does not use gp, a3, at.
# The code could also use a0 after it is finished reading in the input

def GenerateFile(fileName):
   """Generate assembly code to perform an AES encryption and write
      the result out to the specified filename."""

   stdout = sys.stdout
   if fileName:
      fileOut = open(fileName, 'w') 
   else:
      fileOut = sys.stdout

   try:
      sys.stdout = fileOut
      intro = r"""
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

#define INPUT  %(INPUT)s
#define OUTPUT %(OUTPUT)s
#define KEY    %(KEY)s
#define ROUNDS %(ROUNDS)s
#define TABLE  %(TABLE)s
#define TABLE_REG %(TABLE_REG)s   /* Actual register number of TABLE */

/* Hold values at the start of even rounds */
#define X0     %(X0)s
#define X1     %(X1)s
#define X2     %(X2)s
#define X3     %(X3)s

/* Holds values at the start of odd rounds */
#define Y0     %(Y0)s
#define Y1     %(Y1)s
#define Y2     %(Y2)s
#define Y3     %(Y3)s   /* Okay that it's the same as X3 */

/* Scratch registers */
#define T0     %(T0)s
#define T1     %(T1)s
#define T2     %(T2)s

/**
 * The following three registers are currently unused.
 *    %(unused)s
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
 * After the input is read, %(INPUT)s (INPUT) is also available as a temp.
 * After the next-to-last round, %(ROUNDS)s (ROUNDS) is no longer used, either.
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
 * The code can't use "la TABLE, tableName" because that pseudo-instruction
 * uses gp, which this code is (purposely) not initializing.  However MIPS
 * calls a library routine using the code sequence:
 *      la   $25, <functionName>
 *      jalr $25
 * so that $25 initially contains the address of the current function.
 *
 * The second alternative, though one instruction longer, is simpler and less
 * implementation dependent. Alas, it doesn't seem to work for PIC code.
 *
 * For unknown reasons, gas treats "lw TABLE, absolute-offset($25)" as a
 * macro and generates three instructions using AT and gp.  Let's assemble
 * the code directly
 */

#define LW(dst,src,offset)\
    .word 0x8c000000 + (src << 21) + (dst << 16) + ((offset) & 0xFFFF)

#define LOAD_ENCRYPTION_TABLE_ALT0(tableName) \
    LW(TABLE_REG, 25, 9f - 0b);

#define LOAD_ENCRYPTION_TABLE_ALT1(tableName) \
    lui   TABLE, %%hi(tableName); \
    addi  TABLE, %%lo(tableName);

#define LOAD_ENCRYPTION_TABLE_ALT2(tableName) \
      move  t0, ra; \
  10: bal   11f; \
  11: LW(TABLE_REG, 31, 9f - (10b + 8)); \
      move ra, t0

#define LOAD_ENCRYPTION_TABLE_ALT3(tableName) \
      .set noreorder; \
      .cpload $25; \
      .set reorder; \
      la TABLE, tableName

#define LOAD_ENCRYPTION_TABLE LOAD_ENCRYPTION_TABLE_ALT0

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
#define LOAD_INPUT_WORD(target, offset) \
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
#define LOAD_INPUT_WORD(target, offset) \
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

# define FULL_ROUND(out, in0, in1, in2, in3, keyIndex, where) \
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

# define LAST_ROUND(in0, in1, in2, in3, keyIndex, outputOff, type, \
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

# define GET_TABLE_WORD(target, src, table, shift, where) \
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

# define GET_LAST_TABLE_WORD_ENCRYPT(target, src, shift) \
    GET_WORD_OFFSET_NORMAL_ ## shift(target, src); \
    add target, TABLE; \
    lbu  target, 1(target)

/** Sets target = TD4[(src >> shift) & 0xFF] */

# define GET_LAST_TABLE_WORD_DECRYPT(target, src, shift) \
    GET_BYTE_OFFSET_NORMAL_ ## shift(target, src); \
    add target, TABLE; \
    lbu  target, 4 * 0x400(target)

/**
 * Converts the specified byte of src into an offset for a word-sized array.
 * The input is little-endian, but the shift is specified big-endian, so we
 * adjust accordingly.
 */

# define GET_WORD_OFFSET_BYTESWAPPED_24(target, src) \
       GET_WORD_OFFSET_NORMAL_0(target, src)

# define GET_WORD_OFFSET_BYTESWAPPED_16(target, src) \
       GET_WORD_OFFSET_NORMAL_8(target, src)

# define GET_WORD_OFFSET_BYTESWAPPED_8(target, src) \
       GET_WORD_OFFSET_NORMAL_16(target, src)

# define GET_WORD_OFFSET_BYTESWAPPED_0(target, src) \
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

# define GET_BYTE_OFFSET_NORMAL_24(target, src) \
      srl target, src, 24  /* No masking needed */
# define GET_BYTE_OFFSET_NORMAL_16(target, src) \
      srl target, src, 16; andi target, 0xFF
# define GET_BYTE_OFFSET_NORMAL_8(target, src) \
      srl target, src,  8; andi target, 0xFF
# define GET_BYTE_OFFSET_NORMAL_0(target, src) \
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
# define GENERATE_OUTPUT(key, byte0, byte1, byte2, byte3, outOffset, temp) \
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
# define GENERATE_OUTPUT(key, byte0, byte1, byte2, byte3, outOffset, temp) \
    /* Shift the bytes to the appropriate output position */ \
    SHIFT_BYTES_FOR_OUTPUT(byte0, byte1, byte2, byte3); \
    xor key, byte0; \
    xor key, byte1; \
    xor key, byte2; \
    xor key, byte3; \
    usw key, outOffset(OUTPUT)

#  if defined MIPSEL_ENABLE_KEY_MODIFICATION
#  define SHIFT_BYTES_FOR_OUTPUT(byte0, byte1, byte2, byte3) \
      sll byte1, 8; \
      sll byte2, 16; \
      sll byte3, 24;
#  else
#  define SHIFT_BYTES_FOR_OUTPUT(byte0, byte1, byte2, byte3) \
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
"""
      assert RegisterUsage['TABLE'] == 'v1' # to justify TABLE_REG
      temp = dict(RegisterUsage)
      temp['TABLE_REG'] = 3
      temp['unused'] = UnusedRegisters

      print intro % temp

      _print_header('AES_swap_bytes')
      print """
         li  v0, ENABLE_KEY_MODIFICATION
         jr  ra"""
      _print_footer('AES_swap_bytes')

      encryptor = CodeConstructor(True)
      encryptor.GenerateCode();
      encryptor._print_code();

      decryptor = CodeConstructor(False)
      decryptor.GenerateCode();
      decryptor._print_code();
   except:
      sys.stdout = stdout
      print >> sys.stderr, 'Deleting file ', fileName
      if fileName:
         fileOut.close(); os.remove(fileName); fileName = None
      raise

   sys.stdout = stdout
   if fileName: fileOut.close()


class CodeConstructor(object) :
   def __init__(self, encrypt):
      self.code = []
      if encrypt:
         self._type = "encrypt"
         self.shifter =(lambda ins, out_index:
                        ins[out_index:] + ins[:out_index])
      else:
         self._type = "decrypt"
         self.shifter = (lambda ins, out_index:
                          ins[out_index::-1] + ins[:out_index:-1])

   def _external_table_name(self):
      return "AES_%s_Table" % self._type.capitalize()

   def _function_name(self):
      return "AES_" + self._type

   def add(self, *instruction) :
      assert isinstance(instruction[0], str)
      self.code.append(instruction)

   def GenerateCode(self):
      self.add('label', '0')
      self.add('comment', 'Get number of rounds and AES_Table')
      self.add('macro', 'LOAD_ENCRYPTION_TABLE', self._external_table_name())
      self.add('lw', 'ROUNDS', 'KEY', 240)
      self.add('blank')

      Set1 = ('X0', 'X1', 'X2', 'X3')
      Set2 = ('Y0', 'Y1', 'Y2', 'Y3')
      Temps = ('T0', 'T1', 'T2')

      # don't use Set2[-1]
      self._read_input(ins=Set1, xtemps=Set2[0:-1])
      self.add('blank')
      self.add('comment', 'First Round: (%s) := round(%s)' % (
          ", ".join(Set2), ", ".join(Set1)))
      self._full_round(ins=Set1, outs=Set2, key_index=4, is_first_round=True)
      self.add('addi', 'ROUNDS', -2)
      self.add('blank')

      self.add('label', '1')
      self.add('comment', 'Odd Round: (%s) := round(%s)' % (
          ", ".join(Set1), ", ".join(Set2)))
      self._full_round(ins=Set2, outs=Set1, key_index=8)
      self.add('blank')

      self.add('comment', 'Even Round: (%s) := round(%s)' % (
          ", ".join(Set2), ", ".join(Set1)))
      self._full_round(ins=Set1, outs=Set2, key_index=12)
      self.add('blank')

      self.add('addi', 'ROUNDS', -2)
      self.add('addi', 'KEY', 32)
      self.add('bnez', 'ROUNDS', '1b')
      self.add('blank')

      self.add('comment', 'Last Round: OUTPUT := last_round(%s)' % (
          ", ".join(Set2),))
      self._last_round(ins=Set2, xtemps=Set1[0:-1] + ('ROUNDS',), key_index=8)
      self.add('jr', 'ra')

      # LOAD_ENCRYPTION_TABLE uses this offset
      self.add('label', '9')
      self.add('.word', self._external_table_name())


   def _read_input(self, ins, xtemps) :
      temps = ('T0', 'T1', 'T2') + xtemps[0:1] # currently need only one extra
      assert len(temps) == 4
      for (i, input) in enumerate(ins):
         self.add('macro', 'LOAD_INPUT_WORD', input, 4 * i)
      self.add('debug', 'InputWords', ins[0], ins[1], ins[2], ins[3])
      for (i, temp) in enumerate(temps):
         self.add('lw',  temp, 'KEY', 4 * i)
      self.add('debug', 'EncryptionKey', temps[0], temps[1], temps[2], temps[3])
      for (i, input, temp) in zip(xrange(0,4), ins, temps):
         self.add('xor', input, temp)
      self.add('debug', 'End of Reading Input', ins[0], ins[1], ins[2], ins[3])


   def _full_round(self, ins, outs, key_index, is_first_round = False) :
      """Performs one AES encryption round on the ins, and places the result
         into the outs.  The last of the ins and the last of the outs may be
         the same register.
         key_index indicates the (word) offset into the key schedule
             are byte-swapped.
      """
      self.add('debug', 'Start Full Round', ins[0], ins[1], ins[2], ins[3])
      for (out_index, out) in enumerate(outs):
         self.add('debug', '   Start Full Round Part #%d' % (out_index),
                   ins[0], ins[1], ins[2], ins[3])
         # Permute the ins such that we take byte 0 of the first and look it
         # up in TE0, we take byte 1 of the second and look it up in TE1, etc.
         byte_order = self.shifter(ins, out_index)
         round_type = 'FIRST' if is_first_round else 'NORMAL'
         self.add('macro', 'FULL_ROUND', out,
                  byte_order[0], byte_order[1], byte_order[2], byte_order[3],
                  (out_index + key_index), round_type)
         self.add('debug', '   End Full Round Part #%d' %
                  (out_index), outs[out_index])
      self.add('debug', 'End Full Round', outs[0], outs[1], outs[2], outs[3])


   def _last_round(self, ins, xtemps, key_index) :
      """Generate the final round of AES encryption on the ins and output
         the results to meory.
         key_index is the (word) offset into the key table
    """
      self.add('debug', 'Start Last Round', ins[0], ins[1], ins[2], ins[3])
      for out_index in range(0, 4) :
         self.add('debug', '   Start Last Round Part #%d' % (out_index),
                   ins[0], ins[1], ins[2], ins[3])
         # Permute the ins such that we take byte 0 of the first, byte 1 of
         # the second, etc.
         byte_order = self.shifter(ins, out_index)
         self.add('macro', 'LAST_ROUND',
                   byte_order[0], byte_order[1], byte_order[2], byte_order[3],
                   out_index + key_index, out_index * 4, self._type.upper(),
                   xtemps[0] , xtemps[1], xtemps[2])
         self.add('debug', '   End Last Round #%d' % (out_index),
                   ins[0], ins[1], ins[2], ins[3])

   def _get_label() :
      state = '$L' + str(self._get_label.count)
      self._get_label.count += 1
      return state

   _get_label.count = 0                 # Static.

   def _print_code(self):
      _print_header(self._function_name())

      for line in self.code:
         op = line[0]
         if op == 'label' :
            out = '%s:' % line[1]
            print out,
            continue
         elif op == 'debug' :
            continue;
            if False:
               # For really desperate debugging.  We redefine the function
               # so it takes an array as its third arg.  We dump debugging
               # information into it.
               registers = line[2:]
               for (index, reg) in enumerate(registers):
                  out += 'sw %s, %d(a3); ' % (reg, 4 * index)
               out += 'sw $0, %d(a3); ' % (4 * len(registers))
               out += 'addi a3, %d;' % (4 * (len(registers) + 1))
         elif op == 'comment' :
            out = '\t# %s' % (line[1])
         elif (op in ('lw', 'lbu', 'sb', 'ulw', 'usw', 'swl',
                      'swr', 'lwl', 'lwr')
               and len(line) == 4):
            out = '\t%s %s, %s(%s)' % (op.ljust(5), line[1], line[3], line[2])
         elif op in ('='):
            out = '\t%s = %s' % (line[1], line[2])
         elif op == 'macro':
            def min2s(x): return str(x).rjust(2)
            out = '\t%s(%s)' % (line[1], ', '.join(map(min2s, line[2:])))
         elif op == 'blank':
            out = ''
         else:
            args = ', '.join(map(str, line[1:]))
            out = '\t%s %s' % (op.ljust(5), args)
         print out
      _print_footer(self._function_name())

def _print_header(name):
   print '\t.align 2'
   print '\t.globl %s' % (name)
   print '\t.ent %s' % (name)
   print '\t.type %s,@function' % (name)
   print '\t.set\tnoat'
   print '%s:' % (name)
   if False:
      # We don't use the gp anymore
      print '\t.set\tnoreorder'
      print '\t.cpload $25'
      print '\t.set\treorder'

def _print_footer(name):
   print '\t.set\tat'
   print '\t.end\t%s' % (name)
   print


def GenerateCode(encrypt):
   generator = CodeConstructor(encrypt)
   generator.GenerateCode()
   return generator.code

def main():
   global Interpreted
   p = optparse.OptionParser(version='%prog 1.0')
   p.add_option('--out',      '-o', action='store', type='string')
   p.add_option('--byteswap', '-b', action='store_true', default=False)
   (options, arguments) = p.parse_args()
   Interpreted = False
   GenerateFile(options.out)

if __name__ == '__main__':
   main()
