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

AllowReorder = False

def GenerateFile(fileName):
   """Generate assembly code to perform an AES encryption and write
      the result out to the specified filename.
      If byteswap is True, then the generated code assumes that the first four words and the last four words
      of the encryption and decryption key have been pre-byteswapped.  This allows it to generate much more
      efficient code."""
   global AllowReorder
   AllowReorder = True

   stdout = sys.stdout
   encryptCode = GenerateCode(True)
   decryptCode = GenerateCode(False)
   if fileName:
      fileOut = open(fileName, 'w') 
   else:
      fileOut = sys.stdout

   try:
      sys.stdout = fileOut
      print '# AllowReorder: %s' % (AllowReorder)
      print """
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

#define zero $0
#define att $1
#define v0 $2
#define v1 $3
#define a0 $4
#define a1 $5
#define a2 $6
#define a3 $7
#define t0 $8
#define t1 $9
#define t2 $10
#define t3 $11
#define t4 $12
#define t5 $13
#define t6 $14
#define t7 $15
#define t8 $24
#define t9 $25
#define gp $28
#define sp $29
#define fp $30
#define ra $31

      # Life is simpler if we divide the world into three cases:
      #    MIPSEB
      #    MIPSEL_DISABLE_KEY_MODIFICATION
      #    MIPSEL_ENABLE_KEY_MODIFICATION
      # exactly one of these will be defined.  (For MIPSEB, we really
      # don't care about KEY_MODIFICATION.  We can produce improved
      # code in either case

#ifdef MIPSEL
#  ifdef DISABLE_KEY_MODIFICATION
#    define MIPSEL_DISABLE_KEY_MODIFICATION
#  else
#    define MIPSEL_ENABLE_KEY_MODIFICATION
#  endif
#endif

      # Macros used for the first round, where the input word is
      # byte-swapped for MIPSEL_ENABLE_KEY_MODIFICATION

#ifdef MIPSEL_ENABLE_KEY_MODIFICATION
# define SHIFT_BYTE_0_TO_WORD_INDEX(target, reg) sll target, reg, 2
# define SHIFT_BYTE_1_TO_WORD_INDEX(target, reg) srl target, reg, 6
# define SHIFT_BYTE_2_TO_WORD_INDEX(target, reg) srl target, reg, 14
# define SHIFT_BYTE_3_TO_WORD_INDEX(target, reg) srl target, reg, 22
# define ENABLE_KEY_MODIFICATION 1
#else
# define SHIFT_BYTE_0_TO_WORD_INDEX(target, reg) srl target, reg, 22
# define SHIFT_BYTE_1_TO_WORD_INDEX(target, reg) srl target, reg, 14
# define SHIFT_BYTE_2_TO_WORD_INDEX(target, reg) srl target, reg, 6
# define SHIFT_BYTE_3_TO_WORD_INDEX(target, reg) sll target, reg, 2
# define ENABLE_KEY_MODIFICATION 0
#endif

      # Macros used for writing out the last word.  These are only used by
      # MIPSEB and MIPSEL_ENABLE_KEY_MODIFICATION

#ifdef MIPSEL_ENABLE_KEY_MODIFICATION
# define SHIFT_TO_BYTE_0(reg)
# define SHIFT_TO_BYTE_1(reg) sll reg, reg, 8
# define SHIFT_TO_BYTE_2(reg) sll reg, reg, 16
# define SHIFT_TO_BYTE_3(reg) sll reg, reg, 24
#else
# define SHIFT_TO_BYTE_0(reg) sll reg, reg, 24
# define SHIFT_TO_BYTE_1(reg) sll reg, reg, 16
# define SHIFT_TO_BYTE_2(reg) sll reg, reg, 8
# define SHIFT_TO_BYTE_3(reg) 
#endif

      \t.text
"""

      header('AES_swap_bytes')
      print """
\tli\tv0, ENABLE_KEY_MODIFICATION
\tjr\tra"""
      footer('AES_swap_bytes')

      header('AES_encrypt')
      printCode(encryptCode)
      footer('AES_encrypt')

      header('AES_decrypt')
      printCode(decryptCode)
      footer('AES_decrypt')
   except:
      print >> sys.stderr, 'Deleting file ', fileName
      if fileName: os.remove(fileName); fileName = None
      raise
   finally:
      sys.stdout = stdout
      if fileName: fileOut.close()


class State(object) :
   def __init__(self):
      self.pendingInstructions = []
      self.code = []

   def add(self, *instruction) :
      assert isinstance(instruction[0], str)
      if AllowReorder and instruction[0] in ('ulw', 'usw'):
         self.add( *(instruction[0][1:] + 'l', instruction[1], instruction[2], instruction[3] + 3))
         self.add( *(instruction[0][1:] + 'r', instruction[1], instruction[2], instruction[3] + 0))
      else:
         self.code.append(instruction)

   def __iter__(self): 
      return self.code.__iter__()

   def addBefore(self, instruction) :
      if not AllowReorder:
         self.add(*instruction); return
      popped = []
      while True:
         lastInstruction = self.code.pop()
         if lastInstruction[0] not in ('label', 'comment', 'debug'): 
            break
         popped.append(lastInstruction)
      self.add('.set', 'noreorder')
      self.add('.set', 'nomacro')
      self.add(*instruction)
      self.add(*lastInstruction)
      self.add('.set', 'macro')
      self.add('.set', 'reorder')
      self.code += reversed(popped)

def GenerateCode(encrypt):
   (state, set1, set2, temps)  = CreateState(encrypt)
   loopLabel = GetLabel()

   state.add('comment', 'Get number of rounds and AES_Table')
   state.add('lw', state.reg['rounds'], state.reg['key'], 240)
   externalName = 'AES_Encrypt_Table' if state.encrypt else 'AES_Decrypt_Table'
   state.add('la', state.reg['table'], externalName)

   # Perform the opening round, reading from the input 
   # The code for reading the input is identical if byteswap
   GenerateReadInput(state, ins=set1, temps=temps + set2[0:-1])   # don't use set2[-1]
   state.add('comment', 'First Round: (%s, %s, %s, %s) := round(%s, %s, %s, %s)' % (set2 + set1))
   GenerateOneFullRound(state, ins=set1, outs=set2, temps=temps, keyIndex=4, firstRound=True)
   state.add('addi', state.reg['rounds'], -2)

   state.add('label', loopLabel)
   state.add('addi', state.reg['rounds'], -2)
   state.add('comment', 'Odd Round: (%s, %s, %s, %s) := round(%s, %s, %s, %s)' % (set1 + set2))
   GenerateOneFullRound(state, ins=set2, outs=set1, temps=temps, keyIndex=8, firstRound=False)
   state.add('blank')

   state.add('comment', 'Even Round: (%s, %s, %s, %s) := round(%s, %s, %s, %s)' % (set2 + set1))
   GenerateOneFullRound(state, ins=set1, outs=set2, temps=temps, keyIndex=12, firstRound=False)
   state.add('addi', state.reg['key'], 32)
   state.addBefore(('bgtz', state.reg['rounds'], loopLabel))
   state.add('blank')

   state.add('comment', 'Last Round: OUTPUT := round(%s, %s, %s, %s)' % set2)
   GenerateOnePartialRound(state, ins=set2, temps=temps + set1[0:-1], keyIndex=8)
   # Above generates the return
   return state.code

def CreateState(encrypt):
   # This is the only place that we do register allocation
   set1 = ('t0', 't1', 't2', 't3')      # It is okay that t3 is last element in both
   set2 = ('t4', 't5', 't6', 't3')
   temps = ('t7', 't8', 't9', 'att')
   rounds = 'v0'
   table = 'v1'
   key = 'a2'                           # Seems that you can modify inputs.

  # We could probably use 'gp', after grabbing the value of "table".  In case we ever need another register.

   state = State()
   # The register mapping of this state
   state.reg = {'table':table, 'rounds':rounds, 'input':'a0', 'output':'a1', 'key':key }
   if encrypt:
      state.shifter = lambda inIndex, outIndex : (inIndex - outIndex) & 3
      state.encrypt = True
   else:
      state.shifter = lambda inIndex, outIndex: (outIndex - inIndex) & 3
      state.encrypt = False
   if key != 'a2':
      state.add('move', key, 'a2')
   return (state, set1, set2, temps)

def GenerateReadInput(state, ins, temps) :
   state.add('comment', '(%s, %s, %s, %s) := INPUT ^ keys' % ins)
   state.add('#ifdef', 'MIPSEL_DISABLE_KEY_MODIFICATION')

   state.add('comment', ' First four words of key')
   ireg = state.reg['input']
   for (i, input) in enumerate(ins):
      state.add('lw', input, state.reg['key'], 4 * i)
   state.add('debug', 'EncryptionKey', ins[0], ins[1], ins[2], ins[3])
   for i in xrange(0, 4) : state.add('lbu', temps[i], ireg, 4 * i + 3)
   for b in xrange(0, 4) :
      for i in xrange(0, 4) :
         if b != 0: state.add('sll', temps[i], 8 * b)
         if True  : state.add('xor', ins[i], temps[i])
         if b != 3: state.add('lbu', temps[i], ireg, 4 * i + (2 - b))

   state.add('#else')

   # In the normal case, or with byteswap, we can just use ulw to load words
   for (i, input, temp) in zip(xrange(0,4), ins, temps): 
      state.add('lw',  input, state.reg['key'], 4 * i)
      state.add('ulw', temp, state.reg['input'], 4 * i)
   state.add('debug', 'EncryptionKey', ins[0], ins[1], ins[2], ins[3])
   state.add('debug', 'Input', temps[0], temps[1], temps[2], temps[3])
   for (input, temp) in zip(ins, temps):
      state.add('xor', input, temp)
   state.add('debug', 'End of Reading Input', ins[0], ins[1], ins[2], ins[3])

   state.add('#endif')

def GenerateOneFullRound(state, ins, outs, temps, keyIndex, firstRound) :
   """Performs one AES encryption round on the ins, and places the result
      into the outs.  The last of the ins and the last of the outs may be
      the same register.
      keyIndex indicates the (word) offset into the key schedule
      firstRound indicates that if this is a little-endian machine, the ins
          are byte-swapped.
"""
   # This state has been written so that the last one of the ins and
   # the last of the outs can be the same register, if necessary
   state.add('debug', 'Start Full Round', ins[0], ins[1], ins[2], ins[3])
   for (outIndex, out) in enumerate(outs):
      state.add('debug', '   Start Full Round Part #%d' % (outIndex), ins[0], ins[1], ins[2], ins[3])
      ordinal = ('first', 'second', 'third', 'fourth')[outIndex]
      state.add('comment', ' Calculate %s, the %s word of output' % (outs[outIndex], ordinal) )
      GenerateOneFullRoundPiece(state, ins, out, temps, outIndex, keyIndex, firstRound)
      state.add('debug', '   End Full Round Part #%d' % (outIndex), outs[outIndex])
   state.add('debug', 'End Full Round', outs[0], outs[1], outs[2], outs[3])


def GenerateOneFullRoundPiece(state, ins, out, temps, outIndex, keyIndex, firstRound) :
   """Performs one piece of one full AES encryption round, by munging the
      ins and placing it into "out", which is outs[outIndex].
      keyIndex indicates the (word) offset into the key schedule of the first word of output
      if firstRound is false, the inputs are always normal
      if firstRound is true, the inputs may be byte reversed on MIPSEL machinesn
"""
   assert len(temps) >= 4
   for (inIndex, input) in enumerate(ins):
      # Which byte of ins[inIndex] is used for outs[outIndex]?
      # This also indicates which encyrption table to use
      byte = state.shifter(inIndex, outIndex) 
      # The amount to right-shift the input so that the specified byte becomes the low byte
      if not firstRound:
         shift = 24 - 8 * byte          # Words are treated as if big-endian by AES
      else:
         shift = 'BYTE_' + str(byte)    # Flag indicating we want appropriate endian-dependent word
      # "byte" in the next line is the table to use
      GenerateLoadTableData(state, input, temps[inIndex], shift, byte)

   # Note that out isn't modified until we're done with ins.
   # Thus out can overlap with the ins if other state doesn't need the ins anymore
   GenerateLoadKeyWord(state, out, outIndex + keyIndex)
   # Carefully access the registers in the order they were read
   state.add('comment', '  xor all words from table into key')
   for i in xrange(1, 4):
      state.add('xor', temps[0], temps[i])
   state.add('xor', out, temps[0])

def GenerateOnePartialRound(state, ins, temps, keyIndex) :
   """Generate the final round of AES encryption on the ins and write the result out.
      keyIndex is the (word) offset into the key table
      byteswap indicates that if this is a little-endian machine, these
         words of the key schedule are byte-swapped, so we can produce better code
 """
   state.add('debug', 'Start Last Round', ins[0], ins[1], ins[2], ins[3])
   for outIndex in range(0, 4) :
      state.add('debug', '   Start Last Round Part #%d' % (outIndex), ins[0], ins[1], ins[2], ins[3])
      state.add('comment', ' Calculate Output Word %d' % outIndex)
      GenerateOnePartialRoundPiece(state, ins, temps, outIndex, keyIndex)
      state.add('debug', '   End Last Round #%d' % (outIndex), ins[0], ins[1], ins[2], ins[3])


def GenerateOnePartialRoundPiece(state, ins, temps, outIndex, keyIndex):
   """Generate and write out one word of the final AES encryption round.
      keyIndex is the (word) offset into the key table of the first output word
      byteswap indicates that if this is a little-endian machine, these
         words of the key schedule are byte-swapped, so we can produce better code
 """
   key = temps[4]                       # Temp to use as key
   GenerateLoadKeyWord(state, key, outIndex + keyIndex)

   for (inIndex, input, temp) in zip(xrange(0,4), ins, temps):
      # Which byte of ins[inIndex] is used to generate outs[outIndex]
      byte = state.shifter(inIndex, outIndex)
      # Use that byte as an index from TE4 or TD4.   (Note TE4 is a "virtual" table)
      GenerateLoadTableData(state, input, temp, 24 - 8 * byte, 4)

   state.add('#ifdef', 'MIPSEL_DISABLE_KEY_MODIFICATION')

   # Easiest to generate the output one byte at a time, and output it that way
   for (inIndex, temp) in enumerate(temps[0:4]):
      byte = state.shifter(inIndex, outIndex)
      rightShift = 24 - 8 * byte     # amount we need to shift the key to match the table word
      state.add('comment',
                 ' out[%d] = %s ^ (%s >> %d)' % (byte + 4 * outIndex, temp, key, rightShift))
      if rightShift > 0:
         state.add('srl', temps[5], key, rightShift)
         state.add('xor', temp, temps[5])
      else:
         state.add('xor', temp, key)
      state.add('sb', temp, state.reg['output'], byte + (4 * outIndex))
   if (outIndex == 3): state.addBefore(('jr', 'ra'))

   state.add('#else')

   # The key word has been byte-swapped on little-endian machines, so that it is in the
   # same order as byte in the output that it effects
   # We can just shift each byte grabbed from the table and xor it into the key material
   state.add('comment', '  xor bytes from table into possibly byte-swapped key')
   for (inIndex, temp) in enumerate(temps[0:4]):
      byte = state.shifter(inIndex, outIndex)
      state.add('macro', 'SHIFT_TO_BYTE_' + str(byte), temp)
   for (inIndex, temp) in enumerate(temps[0:4]):
      state.add('xor', key, temp)

   state.add('comment', '  write the result')
   state.add('usw', key, state.reg['output'], 4 * outIndex)
   if (outIndex == 3): state.addBefore(('jr', 'ra'))

   state.add('#endif')



def GenerateLoadTableData(state, input, target, rightShift, tableIndex):
   if 0 <= tableIndex <= 3:
      (tableOffset, opcode, indexShift) = (tableIndex * 0x400, 'lw', 2)
   elif tableIndex == 4 and not state.encrypt:
      (tableOffset, opcode, indexShift) = (tableIndex * 0x400, 'lbu', 0)
   elif tableIndex == 4 and state.encrypt:
      # 1, 2, 0x800, 0x803 would all work in the next line
      (tableOffset, opcode, indexShift) = (1,              'lbu', 2)
   tableName = ('TE' if state.encrypt else 'TD') + str(tableIndex)

   if isinstance(rightShift, int):
      shift = rightShift - indexShift      # Amount we need to shift the word to look like a table offset
      if rightShift != 0:
         state.add('comment', '  %s = %s[(%s >> %d) & 0xFF]' % (target, tableName, input, rightShift))
      else:
         state.add('comment', '  %s = %s[%s & 0xFF]' % (target, tableName, input))
      if shift == 24:
         # We can elide the mask, since the shift will take care of it
         assert indexShift == 0
         state.add('srl', target, input, shift)
      elif shift > 0 :
         # Shift and mask
         state.add('srl', target, input, shift)
         state.add('andi', target, target, 0xff << indexShift)
      elif shift < 0: 
         state.add('sll', target, input, -shift)
         state.add('andi', target, target, 0xff << indexShift)
      else:
         state.add('andi', target, input, 0xff << indexShift)
   else:
      assert indexShift == 2
      state.add('comment', '  %s = %s[%s(%s)]' % (target, tableName, rightShift, input))
      state.add('macro', 'SHIFT_' + rightShift + '_TO_WORD_INDEX', target, input)
      state.add('andi', target, target, 0xff << indexShift)

   # Load in the value at state.reg['table] + target + tableOffset
   state.add('add', target, state.reg['table'])
   state.add(opcode,  target, target, tableOffset)

def GenerateLoadKeyWord(state, target, index):
   state.add('comment', '  %s = Key[%d]' % (target, index))
   state.add('lw', target, state.reg['key'], 4 * index)
   # Note that out isn't modified until we're done with ins.
   # Thus out can overlap with the ins if other state doesn't need the ins anymore
   state.add('debug', '      EncryptionKey', target)

def GetLabel() :
   state = '$L' + str(GetLabel.count)
   GetLabel.count += 1
   return state
GetLabel.count = 0

def header(name) :
   print '\t.align 2'
   print '\t.globl %s' % (name)
   print '\t.ent %s' % (name)
   print '\t.type %s,@function' % (name)
   print '%s:' % (name)
   print '\t.set\tnoreorder'
   print '\t.set\tnoat'
   print '\t.cpload $25'

def footer(name) :
   print '\t.set\treorder'
   print '\t.set\tat'
   print '\t.end\t%s' % (name)
   print

def printCode(state) :
   for line in state:
      op = line[0]
      if op == 'label' :
         out = '%s:' % line[1]
      elif op == 'debug' :
         continue
      elif op == 'comment' :
         out = '\t# %s' % (line[1])
      elif op == 'la' or op == 'li' :
         out = '\t%s\t%s, %s' % line
      elif op in ('lw', 'lbu', 'sb', 'ulw', 'usw', 'swl', 'swr', 'lwl', 'lwr') and len(line) == 4:
         out = '\t%s\t%s, %s(%s)' % (op, line[1], line[3], line[2])
      elif op in ('='):
         out = '\t%s = %s' % (line[1], line[2])
      elif op == 'macro':
         out = "\t%s(%s)" % (line[1], ", ".join(line[2:]))
      elif op == 'blank':
         out = ""
      elif op[0] == '#':
         out = " ".join(line)
      else:
         if op in ('xor', 'add', 'addi', 'and', 'andi', 'sll', 'srl', 'or', 'ori') \
                and len(line) == 3:
            # Some assemblers don't mind 2-operand instructions, but why push it
            line = line[0:2] + line[1:3]
         if op == 'addi' and isinstance(line[3], tuple) and line[3][0] == 'delta':
            # Not currently used.
            line = (line[0], line[1], line[2], "%s - %s" % (line[3][1], line[3][2]))
         args = ', '.join(map(str, line[1:]))
         out = '\t%s\t%s' % (op, args)
      print out

def main():
   global Interpreted
   global AllowReorder
   p = optparse.OptionParser(version='%prog 1.0')
   p.add_option('--out',      '-o', action='store', type='string')
   p.add_option('--reorder',  '-r', action='store_true', default=False)
   p.add_option('--byteswap', '-b', action='store_true', default=False)
   (options, arguments) = p.parse_args()

   Interpreted = False
   AllowReorder = options.reorder
   GenerateFile(options.out)

if __name__ == '__main__':
   main()
