/* vi: set sw=4 ts=4: */
/* mips/mipsel ELF shared library loader suppport
 *
   Copyright (C) 2002, Steven J. Hill (sjhill@realitydiluted.com)
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


extern int _dl_linux_resolve(void);

#define OFFSET_GP_GOT 0x7ff0

unsigned long _dl_linux_resolver(unsigned long sym_index,
	unsigned long old_gpreg)
{
	unsigned long *got = (unsigned long *) (old_gpreg - OFFSET_GP_GOT);
	struct elf_resolve *tpnt = (struct elf_resolve *) got[1];
	Elf32_Sym *sym;
	char *strtab;
	unsigned long local_gotno;
	unsigned long gotsym;
	unsigned long value;

	gotsym = tpnt->mips_gotsym;
	local_gotno = tpnt->mips_local_gotno;

	sym = ((Elf32_Sym *) (tpnt->dynamic_info[DT_SYMTAB] + tpnt->loadaddr)) +
		sym_index;

	strtab = (char *) (tpnt->dynamic_info[DT_STRTAB] + tpnt->loadaddr);

	value = (unsigned long) _dl_find_hash(strtab + sym->st_name,
		 tpnt->symbol_scope, tpnt, resolver);

	*(got + local_gotno + sym_index - gotsym) = value;

#if defined (__SUPPORT_LD_DEBUG__)
	_dl_dprintf(2, "---RESOLVER---\n");
	_dl_dprintf(2, "SYMTAB INDEX: %i\n", sym_index);
	_dl_dprintf(2, "      GOTSYM: %i\n", gotsym);
	_dl_dprintf(2, " LOCAL GOTNO: %i\n", local_gotno);
	_dl_dprintf(2, "       VALUE: %x\n", value);
	_dl_dprintf(2, "      SYMBOL: %s\n\n", strtab + sym->st_name);
#endif

	return value;
}

void _dl_parse_lazy_relocation_information(struct elf_resolve *tpnt,
	unsigned long rel_addr, unsigned long rel_size, int type)
{
	/* Nothing to do */
	return;
}

int _dl_parse_copy_information(struct dyn_elf *xpnt, unsigned long rel_addr,
	unsigned long rel_size, int type)
{
	/* Nothing to do */
	return 0;
}
        
int _dl_parse_relocation_information(struct elf_resolve *tpnt, 
	unsigned long rel_addr, unsigned long rel_size, int type)
{
	Elf32_Sym *symtab;
	Elf32_Rel *rpnt;
	char *strtab;
	unsigned long *got;
	unsigned long *reloc_addr;
	unsigned long symbol_addr;
	int i, reloc_type, symtab_index;

	/* Now parse the relocation information */
	rel_size = rel_size / sizeof(Elf32_Rel);
	rpnt = (Elf32_Rel *) (rel_addr + tpnt->loadaddr);

	symtab = (Elf32_Sym *) (tpnt->dynamic_info[DT_SYMTAB] + tpnt->loadaddr);
	strtab = (char *) (tpnt->dynamic_info[DT_STRTAB] + tpnt->loadaddr);
	got = (unsigned long *) (tpnt->dynamic_info[DT_PLTGOT] + tpnt->loadaddr);

	for (i = 0; i < rel_size; i++, rpnt++) {
		reloc_addr = (unsigned long *) (tpnt->loadaddr +
			(unsigned long) rpnt->r_offset);
		reloc_type = ELF32_R_TYPE(rpnt->r_info);
		symtab_index = ELF32_R_SYM(rpnt->r_info);
		symbol_addr = 0;

		if (!symtab_index && tpnt->libtype == program_interpreter)
			continue;

		switch (reloc_type) {
		case R_MIPS_REL32:
			if (symtab_index) {
				if (symtab_index < tpnt->mips_gotsym)
					*reloc_addr +=
						symtab[symtab_index].st_value +
						(unsigned long) tpnt->loadaddr;
				else {
					*reloc_addr += got[symtab_index + tpnt->mips_local_gotno -
						tpnt->mips_gotsym];
				}
			}
			else {
				*reloc_addr += (unsigned long) tpnt->loadaddr;
			}
			break;
		case R_MIPS_NONE:
			break;
		default:
			_dl_dprintf(2, "%s: can't handle reloc type ", _dl_progname);
			if (symtab_index)
				_dl_dprintf(2, "'%s'\n", strtab + symtab[symtab_index].st_name);
			_dl_exit(1);
		};

	};
	return 0;
}

void _dl_perform_mips_global_got_relocations(struct elf_resolve *tpnt)
{
	Elf32_Sym *sym;
	char *strtab;
	unsigned long i;
	unsigned long *got_entry;

	for (; tpnt ; tpnt = tpnt->next) {

		/* We don't touch the dynamic linker */
		if (tpnt->libtype == program_interpreter)
			continue;

		/* Setup the loop variables */
		got_entry = (unsigned long *) (tpnt->loadaddr +
			tpnt->dynamic_info[DT_PLTGOT]) + tpnt->mips_local_gotno;
		sym = (Elf32_Sym *) (tpnt->dynamic_info[DT_SYMTAB] +
			(unsigned long) tpnt->loadaddr) + tpnt->mips_gotsym;
		strtab = (char *) (tpnt->dynamic_info[DT_STRTAB] +
			(unsigned long) tpnt->loadaddr);
		i = tpnt->mips_symtabno - tpnt->mips_gotsym;

		/* Relocate the global GOT entries for the object */
		while(i--) {
#if defined (__SUPPORT_LD_DEBUG__)
			_dl_dprintf(2,"BEFORE: %s=%x\n", strtab + sym->st_name,
				*got_entry);
#endif
			if (sym->st_shndx == SHN_UNDEF) {
				if (ELF32_ST_TYPE(sym->st_info) == STT_FUNC && sym->st_value)
					*got_entry = sym->st_value + (unsigned long) tpnt->loadaddr;
				else {
					*got_entry = (unsigned long) _dl_find_hash(strtab +
						sym->st_name, tpnt->symbol_scope, NULL, copyrel);
				}
			}
			else if (sym->st_shndx == SHN_COMMON) {
				*got_entry = (unsigned long) _dl_find_hash(strtab +
					sym->st_name, tpnt->symbol_scope, NULL, copyrel);
			}
			else if (ELF32_ST_TYPE(sym->st_info) == STT_FUNC &&
				*got_entry != sym->st_value)
				*got_entry += (unsigned long) tpnt->loadaddr;
			else if (ELF32_ST_TYPE(sym->st_info) == STT_SECTION) {
				if (sym->st_other == 0)
					*got_entry += (unsigned long) tpnt->loadaddr;
			}
			else {
				*got_entry = (unsigned long) _dl_find_hash(strtab +
					sym->st_name, tpnt->symbol_scope, NULL, copyrel);
			}

#if defined (__SUPPORT_LD_DEBUG__)
			if (*got_entry == 0)
				_dl_dprintf(2,"ZERO: %s\n", strtab + sym->st_name);
			else
				_dl_dprintf(2," AFTER: %s=%x\n", strtab + sym->st_name,
					*got_entry);
#endif

			got_entry++;
			sym++;
		}
	}
}
