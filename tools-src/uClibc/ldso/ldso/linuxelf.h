#ifndef LINUXELF_H
#define LINUXELF_H


#include <sys/types.h>
#include "ld_sysdep.h" /* before elf.h to get ELF_USES_RELOCA right */
#include <elf.h>

/* Forward declarations for stuff defined in hash.h */
struct dyn_elf;
struct elf_resolve;


/* Some function prototypes */
extern int _dl_map_cache(void);
extern int _dl_unmap_cache(void);
int _dl_copy_fixups(struct dyn_elf * tpnt);
extern int _dl_parse_copy_information(struct dyn_elf *rpnt,
	unsigned long rel_addr, unsigned long rel_size, int type);
extern void _dl_parse_lazy_relocation_information(struct elf_resolve *tpnt,
	unsigned long rel_addr, unsigned long rel_size, int type);
extern int _dl_parse_relocation_information(struct elf_resolve *tpnt,
	unsigned long rel_addr, unsigned long rel_size, int type);
extern struct elf_resolve * _dl_load_shared_library(int secure, 
	struct dyn_elf **rpnt, struct elf_resolve *tpnt, char *full_libname);
extern struct elf_resolve * _dl_load_elf_shared_library(int secure, 
	struct dyn_elf **rpnt, char *libname);
extern int _dl_linux_resolve(void);
#define ELF_CLASS   ELFCLASS32


#if ELF_CLASS == ELFCLASS32

#define elfhdr		Elf32_Ehdr
#define elf_phdr	Elf32_Phdr
#define elf_note	Elf32_Nhdr
/*
 * Datatype of a relocation on this platform
 */
#ifdef ELF_USES_RELOCA
# define ELF_RELOC	Elf32_Rela
#else
# define ELF_RELOC	Elf32_Rel
#endif

#else

#define elfhdr		Elf64_Ehdr
#define elf_phdr	Elf64_Phdr
#define elf_note	Elf64_Nhdr
/*
 * Datatype of a relocation on this platform
 */
#ifdef ELF_USES_RELOCA
# define ELF_RELOC	Elf64_Rela
#else
# define ELF_RELOC	Elf64_Rel
#endif

#endif


/* Convert between the Linux flags for page protections and the
   ones specified in the ELF standard. */
#define LXFLAGS(X) ( (((X) & PF_R) ? PROT_READ : 0) | \
		    (((X) & PF_W) ? PROT_WRITE : 0) | \
		    (((X) & PF_X) ? PROT_EXEC : 0))

#endif	/* LINUXELF_H */
