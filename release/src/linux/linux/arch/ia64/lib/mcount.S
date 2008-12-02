/*
 * Code from glibc written by David Mosberger <davidm@hpl.hp.com>
 * Modifications by Goutham Rao <goutham.rao@intel.com>
 */
#include <asm/asmmacro.h>

GLOBAL_ENTRY(_mcount)
        alloc loc0 = ar.pfs, 4, 3, 3, 0
        mov loc1 = rp
	movl loc2 = mcount_hook
        ;;
	ld8 loc2=[loc2]
	;;
        mov out0 = in2
        mov out1 = rp
	cmp.eq p6,p7=r0,loc2
	;;
(p6)	br .here
(p7)	ld8 loc2=[loc2]
	;;
	mov b7=loc2
	br.call.sptk.few rp = b7
        ;;
.here:
{
        .mii
        mov gp = in1
        mov r2 = ip
        mov ar.pfs = loc0
}
        ;;
        adds r2 = 1f - .here, r2
        mov b7 = loc1
        mov rp = in2
        ;;
        mov b6 = r2
        br.ret.sptk.few b6

1:      alloc r2 = ar.pfs, 0, 0, 9, 0
        mov ar.pfs = r40
        br b7
END(_mcount)

