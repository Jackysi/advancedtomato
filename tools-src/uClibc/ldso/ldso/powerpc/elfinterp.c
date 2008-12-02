/* vi: set sw=4 ts=4: */
/* i386 ELF shared library loader suppport
 *
 * Copyright (C) 2001-2002,  David A. Schleef
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the above contributors may not be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* FIXME -- Disable this when __SUPPORT_LD_DEBUG__ is undefined */
#if defined (__SUPPORT_LD_DEBUG__)
static const char *_dl_reltypes[] =
	{ "R_PPC_NONE", "R_PPC_ADDR32", "R_PPC_ADDR24", "R_PPC_ADDR16",
	"R_PPC_ADDR16_LO", "R_PPC_ADDR16_HI", "R_PPC_ADDR16_HA",
	"R_PPC_ADDR14", "R_PPC_ADDR14_BRTAKEN", "R_PPC_ADDR14_BRNTAKEN",
	"R_PPC_REL24", "R_PPC_REL14", "R_PPC_REL14_BRTAKEN",
	"R_PPC_REL14_BRNTAKEN", "R_PPC_GOT16", "R_PPC_GOT16_LO",
	"R_PPC_GOT16_HI", "R_PPC_GOT16_HA", "R_PPC_PLTREL24",
	"R_PPC_COPY", "R_PPC_GLOB_DAT", "R_PPC_JMP_SLOT", "R_PPC_RELATIVE",
	"R_PPC_LOCAL24PC", "R_PPC_UADDR32", "R_PPC_UADDR16", "R_PPC_REL32",
	"R_PPC_PLT32", "R_PPC_PLTREL32", "R_PPC_PLT16_LO", "R_PPC_PLT16_HI",
	"R_PPC_PLT16_HA", "R_PPC_SDAREL16", "R_PPC_SECTOFF",
	"R_PPC_SECTOFF_LO", "R_PPC_SECTOFF_HI", "R_PPC_SECTOFF_HA",
};
#define N_RELTYPES (sizeof(_dl_reltypes)/sizeof(_dl_reltypes[0]))
#endif

/* Program to load an ELF binary on a linux system, and run it.
   References to symbols in sharable libraries can be resolved by either
   an ELF sharable library or a linux style of shared library. */

/* Disclaimer:  I have never seen any AT&T source code for SVr4, nor have
   I ever taken any courses on internals.  This program was developed using
   information available through the book "UNIX SYSTEM V RELEASE 4,
   Programmers guide: Ansi C and Programming Support Tools", which did
   a more than adequate job of explaining everything required to get this
   working. */


#ifdef __SUPPORT_LD_DEBUG__
static void debug_sym(Elf32_Sym *symtab,char *strtab,int symtab_index);
static void debug_reloc(ELF_RELOC *rpnt);
#define DPRINTF(fmt,args...) if (_dl_debug) _dl_dprintf(2,fmt,args)
#else
#define debug_sym(a,b,c)
#define debug_reloc(a)
#define DPRINTF(fmt,args...)
#endif

extern int _dl_linux_resolve(void);

void _dl_init_got(unsigned long *plt,struct elf_resolve *tpnt)
{
	unsigned long target_addr = (unsigned long)_dl_linux_resolve;
	unsigned int n_plt_entries;
	unsigned long *tramp;
	unsigned long data_words;
	unsigned int rel_offset_words;

	DPRINTF("init_got plt=%x, tpnt=%x\n",
		(unsigned long)plt,(unsigned long)tpnt);

	n_plt_entries = tpnt->dynamic_info[DT_PLTRELSZ] / sizeof(ELF_RELOC);
	DPRINTF("n_plt_entries %d\n",n_plt_entries);

	rel_offset_words = PLT_DATA_START_WORDS(n_plt_entries);
	DPRINTF("rel_offset_words %x\n",rel_offset_words);
	data_words = (unsigned long)(plt + rel_offset_words);
	DPRINTF("data_words %x\n",data_words);

	tpnt->data_words = data_words;

	plt[PLT_LONGBRANCH_ENTRY_WORDS] = OPCODE_ADDIS_HI(11, 11, data_words);
	plt[PLT_LONGBRANCH_ENTRY_WORDS+1] = OPCODE_LWZ(11,data_words,11);

	plt[PLT_LONGBRANCH_ENTRY_WORDS+2] = OPCODE_MTCTR(11);
	plt[PLT_LONGBRANCH_ENTRY_WORDS+3] = OPCODE_BCTR();

	/* [4] */
	/* [5] */

	tramp = plt + PLT_TRAMPOLINE_ENTRY_WORDS;
	tramp[0] = OPCODE_ADDIS_HI(11,11,-data_words);
	tramp[1] = OPCODE_ADDI(11,11,-data_words);
	tramp[2] = OPCODE_SLWI(12,11,1);
	tramp[3] = OPCODE_ADD(11,12,11);
	tramp[4] = OPCODE_LI(12,target_addr);
	tramp[5] = OPCODE_ADDIS_HI(12,12,target_addr);
	tramp[6] = OPCODE_MTCTR(12);
	tramp[7] = OPCODE_LI(12,(unsigned long)tpnt);
	tramp[8] = OPCODE_ADDIS_HI(12,12,(unsigned long)tpnt);
	tramp[9] = OPCODE_BCTR();

	/* [16] unused */
	/* [17] unused */

	/* instructions were modified */
	PPC_DCBST(plt);
	PPC_DCBST(plt+4);
	PPC_DCBST(plt+8);
	PPC_SYNC;
	PPC_ICBI(plt);
	PPC_ICBI(plt+4);
	PPC_ICBI(plt+8);
	PPC_ISYNC;
}

unsigned long _dl_linux_resolver(struct elf_resolve *tpnt, int reloc_entry)
{
	int reloc_type;
	ELF_RELOC *this_reloc;
	char *strtab;
	Elf32_Sym *symtab;
	ELF_RELOC *rel_addr;
	int symtab_index;
	unsigned long insn_addr;
	unsigned long *insns;
	unsigned long targ_addr;
	int delta;

	//DPRINTF("linux_resolver tpnt=%x reloc_entry=%x\n", tpnt, reloc_entry);

	rel_addr = (ELF_RELOC *) (tpnt->dynamic_info[DT_JMPREL] + tpnt->loadaddr);

	this_reloc = (void *)rel_addr + reloc_entry;
	reloc_type = ELF32_R_TYPE(this_reloc->r_info);
	symtab_index = ELF32_R_SYM(this_reloc->r_info);

	symtab = (Elf32_Sym *) (tpnt->dynamic_info[DT_SYMTAB] + tpnt->loadaddr);
	strtab = (char *) (tpnt->dynamic_info[DT_STRTAB] + tpnt->loadaddr);

	//debug_reloc(this_reloc);

	if (reloc_type != R_PPC_JMP_SLOT) {
#if defined (__SUPPORT_LD_DEBUG__)
		_dl_dprintf(2, "%s: Incorrect relocation type [%s] in jump relocation\n",
			_dl_progname,
			(reloc_type<N_RELTYPES)?_dl_reltypes[reloc_type]:"unknown");
#else
		_dl_dprintf(2, "%s: Incorrect relocation type in jump relocation\n", _dl_progname);
#endif
		_dl_exit(1);
	};

	/* Address of dump instruction to fix up */
	insn_addr = (unsigned long) tpnt->loadaddr +
		(unsigned long) this_reloc->r_offset;

	DPRINTF("Resolving symbol %s %x --> ", 
		strtab + symtab[symtab_index].st_name,
		insn_addr);

	/* Get the address of the GOT entry */
	targ_addr = (unsigned long) _dl_find_hash(
		strtab + symtab[symtab_index].st_name, 
		tpnt->symbol_scope, tpnt, resolver);
	if (!targ_addr) {
		_dl_dprintf(2, "%s: can't resolve symbol '%s'\n", 
			_dl_progname, strtab + symtab[symtab_index].st_name);
		_dl_exit(1);
	};
	DPRINTF("%x\n", targ_addr);

	insns = (unsigned long *)insn_addr;
	delta = targ_addr - insn_addr;

	if(delta<<6>>6 == delta){
		insns[0] = OPCODE_B(delta);
	}else if (targ_addr <= 0x01fffffc || targ_addr >= 0xfe000000){
		insns[0] = OPCODE_BA (targ_addr);
	}else{
		/* Warning: we don't handle double-sized PLT entries */
		unsigned long plt_addr;
		unsigned long lbranch_addr;
		unsigned long *ptr;
		int index;

		plt_addr = (unsigned long)tpnt->dynamic_info[DT_PLTGOT] + 
			(unsigned long)tpnt->loadaddr;
		lbranch_addr = plt_addr + PLT_LONGBRANCH_ENTRY_WORDS*4;
		delta = lbranch_addr - insn_addr;
		index = (insn_addr - plt_addr - PLT_INITIAL_ENTRY_WORDS*4)/8;

		ptr = (unsigned long *)tpnt->data_words;
		DPRINTF("plt_addr=%x delta=%x index=%x ptr=%x\n",
			plt_addr, delta, index, ptr);
		ptr[index] = targ_addr;
		/* icache sync is not necessary, since this will be a data load */
		//PPC_DCBST(ptr+index);
		//PPC_SYNC;
		//PPC_ICBI(ptr+index);
		//PPC_ISYNC;
		insns[1] = OPCODE_B(delta - 4);
	}

	/* instructions were modified */
	PPC_DCBST(insn_addr);
	PPC_SYNC;
	PPC_ICBI(insn_addr);
	PPC_ISYNC;

	return targ_addr;
}

void _dl_parse_lazy_relocation_information(struct elf_resolve *tpnt, 
	unsigned long rel_addr, unsigned long rel_size, int type)
{
	int i;
	char *strtab;
	int reloc_type;
	int symtab_index;
	Elf32_Sym *symtab;
	ELF_RELOC *rpnt;
	unsigned long reloc_addr;
	unsigned long *insns;
	unsigned long *plt;
	int index;

	DPRINTF("_dl_parse_lazy_relocation_information(tpnt=%x, rel_addr=%x, rel_size=%x, type=%d)\n",
		tpnt,rel_addr,rel_size,type);

	/* Now parse the relocation information */
	rpnt = (ELF_RELOC *) (rel_addr + tpnt->loadaddr);
	rel_size = rel_size / sizeof(ELF_RELOC);

	symtab = (Elf32_Sym *) (tpnt->dynamic_info[DT_SYMTAB] + tpnt->loadaddr);
	strtab = (char *) (tpnt->dynamic_info[DT_STRTAB] + tpnt->loadaddr);
	plt = (unsigned long *)(tpnt->dynamic_info[DT_PLTGOT] + tpnt->loadaddr);

	for (i = 0; i < rel_size; i++, rpnt++) {
		reloc_addr = (unsigned long)tpnt->loadaddr +
			(unsigned long) rpnt->r_offset;
		reloc_type = ELF32_R_TYPE(rpnt->r_info);
		symtab_index = ELF32_R_SYM(rpnt->r_info);

		/* When the dynamic linker bootstrapped itself, it resolved some symbols.
		   Make sure we do not do them again */
		if (!symtab_index && tpnt->libtype == program_interpreter)
			continue;
		if (symtab_index && tpnt->libtype == program_interpreter &&
			_dl_symbol(strtab + symtab[symtab_index].st_name))
			continue;

		DPRINTF("L %x %s %s %x %x\n",
			reloc_addr, _dl_reltypes[reloc_type],
			symtab_index?strtab + symtab[symtab_index].st_name:"",0,0);

		switch (reloc_type) {
		case R_PPC_NONE:
			break;
		case R_PPC_JMP_SLOT:
			{
			int delta;
			
			delta = (unsigned long)(plt+PLT_TRAMPOLINE_ENTRY_WORDS+2)
				- (reloc_addr+4);

			index = (reloc_addr -
				(unsigned long)(plt+PLT_INITIAL_ENTRY_WORDS))
				/sizeof(unsigned long);
			index /= 2;
			DPRINTF("        index %x delta %x\n",index,delta);
			insns = (unsigned long *)reloc_addr;
			insns[0] = OPCODE_LI(11,index*4);
			insns[1] = OPCODE_B(delta);
			break;
			}
		default:
			_dl_dprintf(2, "%s: (LAZY) can't handle reloc type ", 
				_dl_progname);
#if defined (__SUPPORT_LD_DEBUG__)
			_dl_dprintf(2, "%s ", _dl_reltypes[reloc_type]);
#endif
			if (symtab_index)
				_dl_dprintf(2, "'%s'\n", strtab + symtab[symtab_index].st_name);
			_dl_exit(1);
		};

		/* instructions were modified */
		PPC_DCBST(reloc_addr);
		PPC_SYNC;
		PPC_ICBI(reloc_addr);
	};
}

int _dl_parse_relocation_information(struct elf_resolve *tpnt, 
	unsigned long rel_addr, unsigned long rel_size, int type)
{
	int i;
	char *strtab;
	int reloc_type;
	int goof = 0;
	Elf32_Sym *symtab;
	ELF_RELOC *rpnt;
	unsigned long *reloc_addr;
	unsigned long symbol_addr;
	int symtab_index;
	unsigned long addend;
	unsigned long *plt;

	DPRINTF("_dl_parse_relocation_information(tpnt=%x, rel_addr=%x, rel_size=%x, type=%d)\n",
		tpnt,rel_addr,rel_size,type);

	/* Now parse the relocation information */

	rpnt = (ELF_RELOC *) (rel_addr + tpnt->loadaddr);
	rel_size = rel_size / sizeof(ELF_RELOC);

	symtab = (Elf32_Sym *) (tpnt->dynamic_info[DT_SYMTAB] + tpnt->loadaddr);
	strtab = (char *) (tpnt->dynamic_info[DT_STRTAB] + tpnt->loadaddr);
	plt = (unsigned long *)(tpnt->dynamic_info[DT_PLTGOT] + tpnt->loadaddr);

	for (i = 0; i < rel_size; i++, rpnt++) {
		debug_reloc(rpnt);

		reloc_addr = (unsigned long *) (tpnt->loadaddr + (unsigned long) rpnt->r_offset);
		reloc_type = ELF32_R_TYPE(rpnt->r_info);
		symtab_index = ELF32_R_SYM(rpnt->r_info);
		addend = rpnt->r_addend;
		symbol_addr = 0;

		if (!symtab_index && tpnt->libtype == program_interpreter)
			continue;

		if (symtab_index) {

			if (tpnt->libtype == program_interpreter &&
				_dl_symbol(strtab + symtab[symtab_index].st_name))
				continue;

			symbol_addr = (unsigned long) _dl_find_hash(strtab + symtab[symtab_index].st_name, 
					tpnt->symbol_scope,
					(reloc_type == R_PPC_JMP_SLOT ? tpnt : NULL), symbolrel);

			/*
			 * We want to allow undefined references to weak symbols - this might
			 * have been intentional.  We should not be linking local symbols
			 * here, so all bases should be covered.
			 */
			if (!symbol_addr &&
				ELF32_ST_BIND(symtab[symtab_index].st_info) == STB_GLOBAL) {
				_dl_dprintf(2, "%s: can't resolve symbol '%s'\n", 
					_dl_progname, strtab + symtab[symtab_index].st_name);
				goof++;
			}
		}
		debug_sym(symtab,strtab,symtab_index);

		switch (reloc_type) {
		case R_PPC_NONE:
			break;
		case R_PPC_REL24:
#if 0
			{
			int delta = symbol_addr - (unsigned long)reloc_addr;
			if(delta<<6>>6 != delta){
				_dl_dprintf(2,"R_PPC_REL24: Reloc out of range\n");
				_dl_exit(1);
			}
			*reloc_addr &= 0xfc000003;
			*reloc_addr |= delta&0x03fffffc;
			}
			break;
#else
			_dl_dprintf(2, "%s: symbol '%s' is type R_PPC_REL24\n\tCompile shared libraries with -fPIC!\n",
					_dl_progname, strtab + symtab[symtab_index].st_name);
			_dl_exit(1);
#endif
		case R_PPC_RELATIVE:
			*reloc_addr = (unsigned long)tpnt->loadaddr + addend;
			break;
		case R_PPC_ADDR32:
			*reloc_addr += symbol_addr;
			break;
		case R_PPC_ADDR16_HA:
			/* XXX is this correct? */
			*(short *)reloc_addr += (symbol_addr+0x8000)>>16;
			break;
		case R_PPC_ADDR16_HI:
			*(short *)reloc_addr += symbol_addr>>16;
			break;
		case R_PPC_ADDR16_LO:
			*(short *)reloc_addr += symbol_addr;
			break;
		case R_PPC_JMP_SLOT:
			{
			unsigned long targ_addr = (unsigned long)*reloc_addr;
			int delta = targ_addr - (unsigned long)reloc_addr;
			if(delta<<6>>6 == delta){
				*reloc_addr = OPCODE_B(delta);
			}else if (targ_addr <= 0x01fffffc || targ_addr >= 0xfe000000){
				*reloc_addr = OPCODE_BA (targ_addr);
			}else{
	{
	int delta;
	int index;
	
	delta = (unsigned long)(plt+PLT_TRAMPOLINE_ENTRY_WORDS+2)
		- (unsigned long)(reloc_addr+1);

	index = ((unsigned long)reloc_addr -
		(unsigned long)(plt+PLT_INITIAL_ENTRY_WORDS))
		/sizeof(unsigned long);
	index /= 2;
	DPRINTF("        index %x delta %x\n",index,delta);
	reloc_addr[0] = OPCODE_LI(11,index*4);
	reloc_addr[1] = OPCODE_B(delta);
	}
			}
			break;
			}
		case R_PPC_GLOB_DAT:
			*reloc_addr += symbol_addr;
			break;
		case R_PPC_COPY:
			// handled later
			break;
		default:
			_dl_dprintf(2, "%s: can't handle reloc type ", _dl_progname);
#if defined (__SUPPORT_LD_DEBUG__)
			_dl_dprintf(2, "%s ", _dl_reltypes[reloc_type]);
#endif
			if (symtab_index)
				_dl_dprintf(2, "'%s'\n", strtab + symtab[symtab_index].st_name);
			_dl_exit(1);
		};

		/* instructions were modified */
		PPC_DCBST(reloc_addr);
		PPC_SYNC;
		PPC_ICBI(reloc_addr);

		DPRINTF("reloc_addr %x: %x\n",reloc_addr,*reloc_addr);
	};
	return goof;
}


/* This is done as a separate step, because there are cases where
   information is first copied and later initialized.  This results in
   the wrong information being copied.  Someone at Sun was complaining about
   a bug in the handling of _COPY by SVr4, and this may in fact be what he
   was talking about.  Sigh. */

/* No, there are cases where the SVr4 linker fails to emit COPY relocs
   at all */

int _dl_parse_copy_information(struct dyn_elf *xpnt, unsigned long rel_addr, 
	unsigned long rel_size, int type)
{
	int i;
	char *strtab;
	int reloc_type;
	int goof = 0;
	Elf32_Sym *symtab;
	ELF_RELOC *rpnt;
	unsigned long *reloc_addr;
	unsigned long symbol_addr;
	struct elf_resolve *tpnt;
	int symtab_index;

	DPRINTF("parse_copy xpnt=%x rel_addr=%x rel_size=%x type=%d\n",
		(int)xpnt,rel_addr,rel_size,type);

	/* Now parse the relocation information */

	tpnt = xpnt->dyn;

	rpnt = (ELF_RELOC *) (rel_addr + tpnt->loadaddr);
	rel_size = rel_size / sizeof(ELF_RELOC);

	symtab = (Elf32_Sym *) (tpnt->dynamic_info[DT_SYMTAB] + tpnt->loadaddr);
	strtab = (char *) (tpnt->dynamic_info[DT_STRTAB] + tpnt->loadaddr);

	for (i = 0; i < rel_size; i++, rpnt++) {
		reloc_addr = (unsigned long *) (tpnt->loadaddr + (unsigned long) rpnt->r_offset);
		reloc_type = ELF32_R_TYPE(rpnt->r_info);
		if (reloc_type != R_PPC_COPY)
			continue;

		debug_reloc(rpnt);

		symtab_index = ELF32_R_SYM(rpnt->r_info);
		symbol_addr = 0;
		if (!symtab_index && tpnt->libtype == program_interpreter)
			continue;
		if (symtab_index) {

			if (tpnt->libtype == program_interpreter &&
				_dl_symbol(strtab + symtab[symtab_index].st_name))
				continue;

			symbol_addr = (unsigned long) _dl_find_hash(strtab + 
				symtab[symtab_index].st_name, xpnt->next, 
				NULL, copyrel);
			if (!symbol_addr) {
				_dl_dprintf(2, "%s: can't resolve symbol '%s'\n", 
					_dl_progname, strtab + symtab[symtab_index].st_name);
				goof++;
			};
		};

		debug_sym(symtab,strtab,symtab_index);

		DPRINTF("copy: to=%x from=%x size=%x\n",
			symtab[symtab_index].st_value, 
			symbol_addr, symtab[symtab_index].st_size);

		if (!goof) {
			_dl_memcpy((char *) symtab[symtab_index].st_value, 
				(char *) symbol_addr,
				symtab[symtab_index].st_size);
		}
	};
	return goof;
}


#ifdef unused
static void fixup_jmpslot(unsigned long reloc_addr, unsigned long targ_addr)
{
	int delta = targ_addr - reloc_addr;
	int index;
	
	if(delta<<6>>6 == delta){
		*reloc_addr = OPCODE_B(delta);
	}else if (targ_addr <= 0x01fffffc || targ_addr >= 0xfe000000){
		*reloc_addr = OPCODE_BA (targ_addr);
	}else{
		delta = (unsigned long)(plt+PLT_TRAMPOLINE_ENTRY_WORDS+2)
			- (unsigned long)(reloc_addr+1);

		index = ((unsigned long)reloc_addr -
			(unsigned long)(plt+PLT_INITIAL_ENTRY_WORDS))
			/sizeof(unsigned long);
		index /= 2;

		DPRINTF("        index %x delta %x\n",index,delta);

		reloc_addr[0] = OPCODE_LI(11,index*4);
		reloc_addr[1] = OPCODE_B(delta);
	}
}
#endif


#ifdef __SUPPORT_LD_DEBUG__
static void debug_sym(Elf32_Sym *symtab,char *strtab,int symtab_index)
{
	if (_dl_debug_symbols) {
		if(symtab_index){
			_dl_dprintf(2, "sym: name=%s value=%x size=%x info=%x other=%x shndx=%x\n",
					strtab + symtab[symtab_index].st_name,
					symtab[symtab_index].st_value,
					symtab[symtab_index].st_size,
					symtab[symtab_index].st_info,
					symtab[symtab_index].st_other,
					symtab[symtab_index].st_shndx);
		}else{
			_dl_dprintf(2, "sym: null\n");
		}
	}
}

static void debug_reloc(ELF_RELOC *rpnt)
{
	if (_dl_debug_reloc) {
		_dl_dprintf(2, "reloc: offset=%x type=%x sym=%x addend=%x\n",
				rpnt->r_offset,
				ELF32_R_TYPE(rpnt->r_info),
				ELF32_R_SYM(rpnt->r_info),
				rpnt->r_addend);
	}
}

#endif


