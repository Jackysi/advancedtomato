/*
    NetWinder Floating Point Emulator
    (c) Rebel.COM, 1998,1999

    Direct questions, comments to Scott Bambrough <scottb@netwinder.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "fpa11.h"
#include "softfloat.h"
#include "fpopcode.h"

floatx80 floatx80_exp(floatx80 Fm);
floatx80 floatx80_ln(floatx80 Fm);
floatx80 floatx80_sin(floatx80 rFm);
floatx80 floatx80_cos(floatx80 rFm);
floatx80 floatx80_arcsin(floatx80 rFm);
floatx80 floatx80_arctan(floatx80 rFm);
floatx80 floatx80_log(floatx80 rFm);
floatx80 floatx80_tan(floatx80 rFm);
floatx80 floatx80_arccos(floatx80 rFm);
floatx80 floatx80_pow(floatx80 rFn,floatx80 rFm);
floatx80 floatx80_pol(floatx80 rFn,floatx80 rFm);

unsigned int ExtendedCPDO(const unsigned int opcode)
{
   FPA11 *fpa11 = GET_FPA11();
   floatx80 rFm, rFn;
   unsigned int Fd, Fm, Fn, nRc = 1;

   //printk("ExtendedCPDO(0x%08x)\n",opcode);
   
   Fm = getFm(opcode);
   if (CONSTANT_FM(opcode))
   {
     rFm = getExtendedConstant(Fm);
   }
   else
   {  
     switch (fpa11->fType[Fm])
     {
        case typeSingle:
          rFm = float32_to_floatx80(fpa11->fpreg[Fm].fSingle);
        break;

        case typeDouble:
          rFm = float64_to_floatx80(fpa11->fpreg[Fm].fDouble);
        break;
        
        case typeExtended:
          rFm = fpa11->fpreg[Fm].fExtended;
        break;
        
        default: return 0;
     }
   }
   
   if (!MONADIC_INSTRUCTION(opcode))
   {
      Fn = getFn(opcode);
      switch (fpa11->fType[Fn])
      {
        case typeSingle:
          rFn = float32_to_floatx80(fpa11->fpreg[Fn].fSingle);
        break;

        case typeDouble:
          rFn = float64_to_floatx80(fpa11->fpreg[Fn].fDouble);
        break;
        
        case typeExtended:
          rFn = fpa11->fpreg[Fn].fExtended;
        break;
        
        default: return 0;
      }
   }

   Fd = getFd(opcode);
   switch (opcode & MASK_ARITHMETIC_OPCODE)
   {
      /* dyadic opcodes */
      case ADF_CODE:
         fpa11->fpreg[Fd].fExtended = floatx80_add(rFn,rFm);
      break;

      case MUF_CODE:
      case FML_CODE:
         fpa11->fpreg[Fd].fExtended = floatx80_mul(rFn,rFm);
      break;

      case SUF_CODE:
         fpa11->fpreg[Fd].fExtended = floatx80_sub(rFn,rFm);
      break;

      case RSF_CODE:
         fpa11->fpreg[Fd].fExtended = floatx80_sub(rFm,rFn);
      break;

      case DVF_CODE:
      case FDV_CODE:
         fpa11->fpreg[Fd].fExtended = floatx80_div(rFn,rFm);
      break;

      case RDF_CODE:
      case FRD_CODE:
         fpa11->fpreg[Fd].fExtended = floatx80_div(rFm,rFn);
      break;


      case RMF_CODE:
         fpa11->fpreg[Fd].fExtended = floatx80_rem(rFn,rFm);
      break;


      /* monadic opcodes */
      case MVF_CODE:
         fpa11->fpreg[Fd].fExtended = rFm;
      break;

      case MNF_CODE:
         rFm.high ^= 0x8000;
         fpa11->fpreg[Fd].fExtended = rFm;
      break;

      case ABS_CODE:
         rFm.high &= 0x7fff;
         fpa11->fpreg[Fd].fExtended = rFm;
      break;

      case RND_CODE:
      case URD_CODE:
         fpa11->fpreg[Fd].fExtended = floatx80_round_to_int(rFm);
      break;

      case SQT_CODE:
         fpa11->fpreg[Fd].fExtended = floatx80_sqrt(rFm);
      break;


      case NRM_CODE:
      break;
      
      default:
      {
        nRc = 0;
      }
   }
   
   if (0 != nRc) fpa11->fType[Fd] = typeExtended;
   return nRc;
}

