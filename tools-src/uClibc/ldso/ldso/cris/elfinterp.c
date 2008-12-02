/*
 * CRIS ELF shared library loader support.
 *
 * Program to load an elf binary on a linux system, and run it.
 * References to symbols in sharable libraries can be resolved
 * by either an ELF sharable library or a linux style of shared
 * library.
 *
 * Copyright (C) 2002, Axis Communications AB
 * All rights reserved
 *
 * Author: Tobias Anderberg, <tobiasa@axis.com>
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

/* Support for the LD_DEBUG variable. */
#if defined (__SUPPORT_LD_DEBUG__)
static const char *_dl_reltypes_tab[] = {
	[0]		"R_CRIS_NONE", "R_CRIS_8", "R_CRIS_16", "R_CRIS_32",
	[4]		"R_CRIS_8_PCREL", "R_CRIS_16_PCREL", "R_CRIS_32_PCREL", "R_CRIS_GNU_VTINHERIT",
	[8]		"R_CRIS_GNU_VTENTRY", "R_CRIS_COPY", "R_CRIS_GLOB_DAT", "R_CRIS_JUMP_SLOT",
	[16]	"R_CRIS_RELATIVE", "R_CRIS_16_GOT", "R_CRIS_32_GOT", "R_CRIS_16_GOTPLT",
	[32]	"R_CRIS_32_GOTPLT", "R_CRIS_32_GOTREL", "R_CRIS_32_PLT_GOTREL", "R_CRIS_32_PLT_PCREL",
};

static const char *
_dl_reltypes(int type)
{ 
	static char buf[22];
	const char *str;
  
	if (type >= (sizeof(_dl_reltypes_tab) / sizeof(_dl_reltypes_tab[0])) ||
		NULL == (str = _dl_reltypes_tab[type])) { 
    		str = _dl_simple_ltoa(buf, (unsigned long)(type));
	}

	return str;
}

static void 
debug_sym(Elf32_Sym *symtab, char *strtab, int symtab_index)
{ 
	if (_dl_debug_symbols) { 
		if (symtab_index) {
			_dl_dprintf(_dl_debug_file, "\n%s\tvalue=%x\tsize=%x\tinfo=%x\tother=%x\tshndx=%x",
				strtab + symtab[symtab_index].st_name,
				symtab[symtab_index].st_value,
				symtab[symtab_index].st_size,
				symtab[symtab_index].st_info,
				symtab[symtab_index].st_other,
				symtab[symtab_index].st_shndx);
    	}
  	}
} 
    
static void
debug_reloc(Elf32_Sym *symtab, char *strtab, ELF_RELOC *rpnt)
{   
	if (_dl_debug_reloc) { 
		int symtab_index;
		const char *sym;

		symtab_index = ELF32_R_SYM(rpnt->r_info);
		sym = symtab_index ? strtab + symtab[symtab_index].st_name : "sym=0x0";
        
#ifdef ELF_USES_RELOCA
		_dl_dprintf(_dl_debug_file, "\n%s\toffset=%x\taddend=%x %s",
			_dl_reltypes(ELF32_R_TYPE(rpnt->r_info)), rpnt->r_offset, rpnt->r_addend, sym);
#else
		_dl_dprintf(_dl_debug_file, "\n%s\toffset=%x %s", _dl_reltypes(ELF32_R_TYPE(rpnt->r_info)),
			rpnt->r_offset, sym);
#endif
	}
}
#endif

/* Defined in resolve.S */
extern int _dl_linux_resolve(void);

unsigned long
_dl_linux_resolver(struct elf_resolve *tpnt, int reloc_offset)
{
  
	int reloc_type;
	int symtab_index;
	char *strtab;
	char *new_addr;
	char **got_addr;
	ELF_RELOC *reloc;
	Elf32_Sym *symtab;
	Elf32_Addr instr_addr;
	
	reloc = (ELF_RELOC *) (tpnt->dynamic_info[DT_JMPREL] + tpnt->loadaddr) + (reloc_offset >> 3);

	reloc_type = ELF32_R_TYPE(reloc->r_info);
	symtab_index = ELF32_R_SYM(reloc->r_info);

	symtab = (Elf32_Sym *) (tpnt->dynamic_info[DT_SYMTAB] + tpnt->loadaddr);
	strtab = (char *) (tpnt->dynamic_info[DT_STRTAB] + tpnt->loadaddr);

	if (reloc_type != R_CRIS_JUMP_SLOT) {
		_dl_dprintf(_dl_debug_file, "%s: Incorrect relocation type for jump relocations.\n", _dl_progname);
		_dl_exit(1);
	}

	/* Fetch the address of the jump instruction to fix up. */
	instr_addr = ((Elf32_Addr) reloc->r_offset + (Elf32_Addr) tpnt->loadaddr);
	got_addr = (char **) instr_addr;

#ifdef DL_DEBUG_SYMBOLS
	_dl_dprintf(_dl_debug_file, "Resolving symbol: %s\n", strtab + symtab[symtab_index].st_name);
#endif

	/* Fetch the address of the GOT entry. */
	new_addr = _dl_find_hash(strtab + symtab[symtab_index].st_name, tpnt->symbol_scope, tpnt, 0);
	
	if (!new_addr) {
		_dl_dprintf(_dl_debug_file, "%s: Can't resolv symbol '%s'\n", _dl_progname, strtab + symtab[symtab_index].st_name);
		_dl_exit(1);
	}

#if defined (__SUPPORT_LD_DEBUG__)
	if (_dl_debug_bindings) {
		_dl_dprintf(_dl_debug_file, "\nresolve function: %s", strtab + symtab[symtab_index].st_name);
		
		if (_dl_debug_detail)
			_dl_dprintf(_dl_debug_file, "\tpatch %x ==> %x @ %x", *got_addr, new_addr, got_addr);
	}
#endif

	*got_addr = new_addr;
	return (unsigned long) new_addr;
}

void
_dl_parse_lazy_relocation_information(struct elf_resolve *tpnt, unsigned long rel_addr, unsigned long rel_size, int type)
{
	int i;
	int reloc_type;
	int symtab_index;
	char *strtab;
	Elf32_Sym *symtab;
	ELF_RELOC *rpnt;
	Elf32_Addr *reloc_addr;

	/* Parse relocation information. */
	rpnt = (ELF_RELOC *) (rel_addr + tpnt->loadaddr);
	rel_size = rel_size / sizeof(ELF_RELOC);
	symtab = (Elf32_Sym *) (tpnt->dynamic_info[DT_SYMTAB] + tpnt->loadaddr);
	strtab = (char *) (tpnt->dynamic_info[DT_STRTAB] + tpnt->loadaddr);

	for (i = 0; i < rel_size; i++, rpnt++) {
		reloc_addr = (Elf32_Addr *) (tpnt->loadaddr + (Elf32_Addr) rpnt->r_offset);
		reloc_type = ELF32_R_TYPE(rpnt->r_info);
		symtab_index = ELF32_R_SYM(rpnt->r_info);

		/*
		 * Make sure we don't resolv the same symbols as we did
		 * when ld.so bootstrapped itself.
		 */
		if (!symtab_index && tpnt->libtype == program_interpreter)
		 	continue;
		if (symtab_index && tpnt->libtype == program_interpreter &&
			_dl_symbol(strtab + symtab[symtab_index].st_name))
			continue;

#if defined (__SUPPORT_LD_DEBUG__)
	{
		unsigned long old_val = *reloc_addr;

#endif

		switch (reloc_type) {
			case R_CRIS_NONE:
				break;
			case R_CRIS_JUMP_SLOT:
				*reloc_addr += (Elf32_Addr) tpnt->loadaddr;
				break;
			default:
				_dl_dprintf(_dl_debug_file, "%s: Can't handle relocation type (lazy).\n",
					_dl_progname);
#ifdef __SUPPORT_LD_DEBUG__
					_dl_dprintf(_dl_debug_file, "%s ", _dl_reltypes(reloc_type));
#endif
				if (symtab_index)
					_dl_dprintf(_dl_debug_file, "'%s'\n", strtab + symtab[symtab_index].st_name);

				_dl_exit(1);
		}
#if defined(__SUPPORT_LD_DEBUG__)
		if (_dl_debug_reloc && _dl_debug_detail)
			_dl_dprintf(_dl_debug_file, "\tpatch: %x ==> %x @ %x", old_val, *reloc_addr, reloc_addr);
	}
#endif
	}
}

int
_dl_parse_relocation_information(struct elf_resolve *tpnt, unsigned long rel_addr, unsigned long rel_size, int type)
{
	int i;
	int goof;
	int reloc_type;
	int symtab_index;
	char *strtab;
	Elf32_Sym *symtab;
	ELF_RELOC *rpnt;
	Elf32_Addr *reloc_addr;
	Elf32_Addr symbol_addr;

	goof = 0;
	rpnt = (ELF_RELOC *) (rel_addr + tpnt->loadaddr);
	rel_size = rel_size / sizeof(ELF_RELOC);

	symtab = (Elf32_Sym *) (tpnt->dynamic_info[DT_SYMTAB] + tpnt->loadaddr);
	strtab = (char *) (tpnt->dynamic_info[DT_STRTAB] + tpnt->loadaddr);

	for (i = 0; i < rel_size; i++, rpnt++) {
		reloc_addr = (Elf32_Addr *) (tpnt->loadaddr + (Elf32_Addr) rpnt->r_offset);
		reloc_type = ELF32_R_TYPE(rpnt->r_info);
		symtab_index = ELF32_R_SYM(rpnt->r_info);
		symbol_addr = 0;

		if (!symtab_index && tpnt->libtype == program_interpreter)
			continue;

		if (symtab_index) {
			if (tpnt->libtype == program_interpreter &&
				_dl_symbol(strtab + symtab[symtab_index].st_name))
				continue;

			if (symtab[symtab_index].st_shndx != SHN_UNDEF && ELF32_ST_BIND(symtab[symtab_index].st_info) == STB_LOCAL)
				symbol_addr = (Elf32_Addr) tpnt->loadaddr;
			else
				symbol_addr = (Elf32_Addr) _dl_find_hash(strtab + symtab[symtab_index].st_name,
					tpnt->symbol_scope, (reloc_type == R_CRIS_JUMP_SLOT ? tpnt : NULL), 0);

			/*
			 * We want to allow undefined references to weak symbols - this
			 * might have been intentional. We should not be linking local
			 * symbols here, so all bases should be covered.
			 */
			if (!symbol_addr && ELF32_ST_BIND(symtab[symtab_index].st_info) == STB_GLOBAL) {
				_dl_dprintf(_dl_debug_file, "%s: Can't resolve '%s'\n",
					_dl_progname, strtab + symtab[symtab_index].st_name);
				goof++;
			}
			
			symbol_addr += rpnt->r_addend;
		}

#if defined(__SUPPORT_LD_DEBUG__)
	{
		unsigned long old_val = *reloc_addr;
		debug_sym(symtab,strtab,symtab_index);
		debug_reloc(symtab,strtab,rpnt);
#endif
		
		switch (reloc_type) {
			case R_CRIS_GLOB_DAT:
			case R_CRIS_JUMP_SLOT:
			case R_CRIS_32:
				*reloc_addr = symbol_addr;
				break;
			case R_CRIS_RELATIVE:
				*reloc_addr = (Elf32_Addr) tpnt->loadaddr + rpnt->r_addend;
				break;
			case R_CRIS_COPY:
				*reloc_addr = symbol_addr;
				break;
			case R_CRIS_8:
				*(char *) reloc_addr = symbol_addr;
				break;
			case R_CRIS_16:
				*(short *) reloc_addr = symbol_addr;
				break;
			case R_CRIS_8_PCREL:
				*(char *) reloc_addr = symbol_addr + rpnt->r_addend - (Elf32_Addr) reloc_addr - 1;
				break;
			case R_CRIS_16_PCREL:
				*(short *) reloc_addr = symbol_addr + rpnt->r_addend - (Elf32_Addr) reloc_addr - 2;
				break;
			case R_CRIS_32_PCREL:
				*reloc_addr = symbol_addr + rpnt->r_addend - (Elf32_Addr) reloc_addr - 4;
				break;
			case R_CRIS_NONE:
				break;
			default:
				_dl_dprintf(_dl_debug_file, "%s: Can't handle relocation type ", _dl_progname);
#ifdef __SUPPORT_LD_DEBUG__
				_dl_dprintf(_dl_debug_file, "%s\n", _dl_reltypes(reloc_type));
#endif
				if (symtab_index) {
					_dl_dprintf(_dl_debug_file, "'%s'\n", strtab + symtab[symtab_index].st_name);
					return -1;
				}
		}
#if defined(__SUPPORT_LD_DEBUG__)
		if (_dl_debug_reloc && _dl_debug_detail)
			_dl_dprintf(_dl_debug_file, "\tpatch: %x ==> %x @ %x", old_val, *reloc_addr, reloc_addr);
	}
#endif
	}
	return goof;
}

/*
 * This is done as a seperate step, because there are cases where
 * information is first copied and later initialized. This results
 * in the wrong information being copied.
 */
int
_dl_parse_copy_information(struct dyn_elf *xpnt, unsigned long rel_addr, unsigned long rel_size, int type)
{
	int i;
	int reloc_type;
	int goof;
	int symtab_index;
	char *strtab;
	struct elf_resolve *tpnt;
	Elf32_Sym *symtab;
	ELF_RELOC *rpnt;
	Elf32_Addr *reloc_addr;
	Elf32_Addr symbol_addr;

	goof = 0;
	tpnt = xpnt->dyn;

	rpnt = (ELF_RELOC *) (rel_addr + tpnt->loadaddr);
	rel_size = rel_size / sizeof(ELF_RELOC);

	symtab = (Elf32_Sym *) (tpnt->dynamic_info[DT_SYMTAB] + tpnt->loadaddr);
	strtab = (char *) (tpnt->dynamic_info[DT_STRTAB] + tpnt->loadaddr);

	for (i = 0; i < rel_size; i++, rpnt++) {
		reloc_addr = (Elf32_Addr *) (tpnt->loadaddr + (Elf32_Addr) rpnt->r_offset);
		reloc_type = ELF32_R_TYPE(rpnt->r_info);

		if (reloc_type != R_CRIS_COPY)
			continue;

		symtab_index = ELF32_R_SYM(rpnt->r_info);
		symbol_addr = 0;

		if (!symtab_index && tpnt->libtype == program_interpreter)
			continue;

		if (symtab_index) {
			if (tpnt->libtype == program_interpreter && 
				_dl_symbol(strtab + symtab[symtab_index].st_name))
				continue;

			symbol_addr = (Elf32_Addr) _dl_find_hash(strtab + 
				symtab[symtab_index].st_name, xpnt->next, NULL, 1);

			if (!symbol_addr) {
				_dl_dprintf(_dl_debug_file, "%s: Can't resolv symbol '%s'\n",
					_dl_progname, strtab + symtab[symtab_index].st_name);
				goof++;
			}
		}

		if (!goof) {
#if defined(__SUPPORT_LD_DEBUG__)
			if (_dl_debug_move)
				_dl_dprintf(_dl_debug_file, "\n%s move %x bytes from %x to %x",
					strtab + symtab[symtab_index].st_name,
					symtab[symtab_index].st_size,
					symbol_addr, symtab[symtab_index].st_value);
#endif
			_dl_memcpy((char *) symtab[symtab_index].st_value, (char *) symbol_addr,
				symtab[symtab_index].st_size);
		}
	}

	return goof;
}
