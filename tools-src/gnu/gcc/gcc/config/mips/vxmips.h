/* Definitions of target machine for GNU compiler.  VxWorks MIPS version.
   Copyright (C) 1996,2001 Free Software Foundation, Inc.

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#undef SUBTARGET_CPP_SPEC
#define SUBTARGET_CPP_SPEC "%{!DCPU=*:			\
  %{!mips1: %{!mips2: %{!mips3: %{!mips4:  -DCPU=MIPS32}}}}     \
  %{mips1: -DCPU=MIPS32}                                        \
  %{mips2: -DCPU=MIPS32}                                        \
  %{mips3: -DCPU=MIPS64}                                        \
  %{mips4: -DCPU=MIPS64}} -D__CPU__=CPU"

#undef CPP_PREDEFINES
#define CPP_PREDEFINES "-D__vxworks -D__mips__ -D__MIPSEB__ -Asystem(vxworks)"

/* There is no OS support for this, yet.  */
#undef MIPS_DEFAULT_GVALUE
#define MIPS_DEFAULT_GVALUE	0

/* VxWorks uses object files, not loadable images.  make linker just
   combine objects. */
#undef LINK_SPEC
#define LINK_SPEC "-r"

  /* Use GNU soft FP naming conventions */
#undef INIT_SUBTARGET_OPTABS
#define INIT_SUBTARGET_OPTABS
