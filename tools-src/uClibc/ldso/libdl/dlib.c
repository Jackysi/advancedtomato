/*
 * libdl.c
 * 
 * Functions required for dlopen et. al.
 */

#include <stdlib.h>
#include <features.h>
#include "dlfcn.h"
#include "linuxelf.h"
#include "ld_syscall.h"
#include "ld_hash.h"
#include "ld_string.h"

extern struct r_debug *_dl_debug_addr;

extern void *(*_dl_malloc_function) (size_t size);

static int do_fixup(struct elf_resolve *tpnt, int flag);
static int do_dlclose(void *, int need_fini);

void *dlopen(const char *, int) __attribute__ ((__weak__, __alias__ ("_dlopen")));
const char *dlerror(void) __attribute__ ((__weak__, __alias__ ("_dlerror")));
void *dlsym(void *, const char *) __attribute__ ((__weak__, __alias__ ("_dlsym")));
int dlclose(void *) __attribute__ ((__weak__, __alias__ ("_dlclose")));
int dladdr(void *, Dl_info *) __attribute__ ((__weak__, __alias__ ("_dladdr")));

#ifdef __PIC__
/* This is a real hack.  We need access to the dynamic linker, but we
also need to make it possible to link against this library without any
unresolved externals.  We provide these weak symbols to make the link
possible, but at run time the normal symbols are accessed. */
static void __attribute__ ((unused)) foobar(void)
{
	const char msg[]="libdl library not correctly linked\n";
	_dl_write(2, msg, _dl_strlen(msg));
	_dl_exit(1);
}

static int __attribute__ ((unused)) foobar1 = (int) foobar;	/* Use as pointer */
extern void _dl_dprintf(int, const char *, ...) __attribute__ ((__weak__, __alias__ ("foobar")));
extern char *_dl_find_hash(const char *, struct dyn_elf *, struct elf_resolve *, enum caller_type)
	__attribute__ ((__weak__, __alias__ ("foobar")));
extern struct elf_resolve * _dl_load_shared_library(int, struct dyn_elf **, struct elf_resolve *, char *)
	__attribute__ ((__weak__, __alias__ ("foobar")));
extern int _dl_parse_relocation_information(struct elf_resolve *, unsigned long, unsigned long, int)
	__attribute__ ((__weak__, __alias__ ("foobar")));
extern void _dl_parse_lazy_relocation_information(struct elf_resolve *, unsigned long, unsigned long, int)
	__attribute__ ((__weak__, __alias__ ("foobar")));
#ifdef __mips__
extern void _dl_perform_mips_global_got_relocations(struct elf_resolve *tpnt)
	__attribute__ ((__weak__, __alias__ ("foobar")));
#endif
#ifdef USE_CACHE
int _dl_map_cache(void) __attribute__ ((__weak__, __alias__ ("foobar")));
int _dl_unmap_cache(void) __attribute__ ((__weak__, __alias__ ("foobar")));
#endif	

extern struct dyn_elf *_dl_symbol_tables __attribute__ ((__weak__, __alias__ ("foobar1")));
extern struct dyn_elf *_dl_handles __attribute__ ((__weak__, __alias__ ("foobar1")));
extern struct elf_resolve *_dl_loaded_modules __attribute__ ((__weak__, __alias__ ("foobar1")));
extern struct r_debug *_dl_debug_addr __attribute__ ((__weak__, __alias__ ("foobar1")));
extern unsigned long _dl_error_number __attribute__ ((__weak__, __alias__ ("foobar1")));
extern void *(*_dl_malloc_function)(size_t) __attribute__ ((__weak__, __alias__ ("foobar1")));
#else
#ifdef __SUPPORT_LD_DEBUG__
static char *_dl_debug  = 0;
static char *_dl_debug_symbols = 0;
static char *_dl_debug_move    = 0;
static char *_dl_debug_reloc   = 0;
static char *_dl_debug_detail  = 0;
static char *_dl_debug_nofixups  = 0;
static char *_dl_debug_bindings  = 0;
static int   _dl_debug_file = 2;
#elif defined __SUPPORT_LD_DEBUG_EARLY__
#define _dl_debug_file 2
#endif
char *_dl_library_path = 0;
char *_dl_ldsopath = 0;
struct r_debug *_dl_debug_addr = NULL;
static char *_dl_malloc_addr, *_dl_mmap_zero;
#include "../ldso/ldso.h"               /* Pull in the name of ld.so */
#include "../ldso/hash.c"
#include "../ldso/readelflib1.c"
#endif

static const char *dl_error_names[] = {
	"",
	"File not found",
	"Unable to open /dev/zero",
	"Not an ELF file",
#if defined (__i386__)
	"Not i386 binary",
#elif defined (__sparc__)
	"Not sparc binary",
#elif defined (__mc68000__)
	"Not m68k binary",
#else
	"Unrecognized binary type",
#endif
	"Not an ELF shared library",
	"Unable to mmap file",
	"No dynamic section",
#ifdef ELF_USES_RELOCA
	"Unable to process REL relocs",
#else
	"Unable to process RELA relocs",
#endif
	"Bad handle",
	"Unable to resolve symbol"
};

static void __attribute__ ((destructor)) dl_cleanup(void)
{
	struct dyn_elf *d;

	for (d = _dl_handles; d; d = d->next_handle)
		if (d->dyn->libtype == loaded_file && d->dyn->dynamic_info[DT_FINI]) {
			(* ((int (*)(void)) (d->dyn->loadaddr + d->dyn->dynamic_info[DT_FINI]))) ();
			d->dyn->dynamic_info[DT_FINI] = 0;
		}
}

void *_dlopen(const char *libname, int flag)
{
	struct elf_resolve *tpnt, *tfrom;
	struct dyn_elf *rpnt = NULL;
	struct dyn_elf *dyn_chain;
	struct dyn_elf *dpnt;
	static int dl_init = 0;
	char *from;
	void (*dl_brk) (void);
#ifdef __PIC__
	int (*dl_elf_init) (void);
#endif

	/* A bit of sanity checking... */
	if (!(flag & (RTLD_LAZY|RTLD_NOW))) {
		_dl_error_number = LD_BAD_HANDLE;
		return NULL;
	}

	from = __builtin_return_address(0);

	/* Have the dynamic linker use the regular malloc function now */
	if (!dl_init) {
		dl_init++;
		_dl_malloc_function = malloc;
	}

	/* Cover the trivial case first */
	if (!libname)
		return _dl_symbol_tables;

#ifdef USE_CACHE
	_dl_map_cache();
#endif

	/*
	 * Try and locate the module we were called from - we
	 * need this so that we get the correct RPATH.  Note that
	 * this is the current behavior under Solaris, but the
	 * ABI+ specifies that we should only use the RPATH from
	 * the application.  Thus this may go away at some time
	 * in the future.
	 */
	tfrom = NULL;
	for (dpnt = _dl_symbol_tables; dpnt; dpnt = dpnt->next) {
		tpnt = dpnt->dyn;
		if (tpnt->loadaddr < from
			&& (tfrom == NULL || tfrom->loadaddr < tpnt->loadaddr))
			tfrom = tpnt;
	}

	if (!(tpnt = _dl_load_shared_library(0, &rpnt, tfrom, (char*)libname))) {
#ifdef USE_CACHE
		_dl_unmap_cache();
#endif
		return NULL;
	}
	//tpnt->libtype = loaded_file;

	dyn_chain = rpnt = (struct dyn_elf *) malloc(sizeof(struct dyn_elf));
	_dl_memset(rpnt, 0, sizeof(*rpnt));
	rpnt->dyn = tpnt;
	rpnt->flags = flag;
	if (!tpnt->symbol_scope)
		tpnt->symbol_scope = dyn_chain;

	rpnt->next_handle = _dl_handles;
	_dl_handles = rpnt;

	/*
	 * OK, we have the requested file in memory.  Now check for
	 * any other requested files that may also be required.
	 */
	  {
	    struct elf_resolve *tcurr;
	    struct elf_resolve * tpnt1;
	    Elf32_Dyn * dpnt;
	    char * lpnt;

	    tcurr = tpnt;
	    do{
	      for(dpnt = (Elf32_Dyn *) tcurr->dynamic_addr; dpnt->d_tag; dpnt++)
		{
	  
		  if(dpnt->d_tag == DT_NEEDED)
		    {
		      lpnt = tcurr->loadaddr + tcurr->dynamic_info[DT_STRTAB] + 
			dpnt->d_un.d_val;
		      if(!(tpnt1 = _dl_load_shared_library(0, &rpnt, tcurr, lpnt)))
			goto oops;

		      rpnt->next = (struct dyn_elf *) malloc(sizeof(struct dyn_elf));
		      _dl_memset (rpnt->next, 0, sizeof (*(rpnt->next)));
		      rpnt = rpnt->next;
		      if (!tpnt1->symbol_scope) tpnt1->symbol_scope = dyn_chain;
		      rpnt->dyn = tpnt1;
		    };
		}
	      
	      tcurr = tcurr->next;
	    } while(tcurr);
	  }
	 
	/*
	 * OK, now attach the entire chain at the end
	 */

	rpnt->next = _dl_symbol_tables;

	/*
	 * MIPS is special *sigh*
	 */
#ifdef __mips__
	_dl_perform_mips_global_got_relocations(tpnt);
#endif

	if (do_fixup(tpnt, flag)) {
		_dl_error_number = LD_NO_SYMBOL;
		goto oops;
	}

	if (_dl_debug_addr) {
	    dl_brk = (void (*)(void)) _dl_debug_addr->r_brk;
	    if (dl_brk != NULL) {
		_dl_debug_addr->r_state = RT_ADD;
		(*dl_brk) ();

		_dl_debug_addr->r_state = RT_CONSISTENT;
		(*dl_brk) ();
	    }
	}

#ifdef __PIC__
	for (rpnt = dyn_chain; rpnt; rpnt = rpnt->next) {
		tpnt = rpnt->dyn;
		/* Apparently crt1 for the application is responsible for handling this.
		 * We only need to run the init/fini for shared libraries
		 */
		if (tpnt->libtype == program_interpreter)
			continue;
		if (tpnt->libtype == elf_executable)
			continue;
		if (tpnt->init_flag & INIT_FUNCS_CALLED)
			continue;
		tpnt->init_flag |= INIT_FUNCS_CALLED;

		if (tpnt->dynamic_info[DT_INIT]) {
			dl_elf_init = (int (*)(void)) (tpnt->loadaddr + tpnt->dynamic_info[DT_INIT]);
			(*dl_elf_init) ();
		}
		if (tpnt->dynamic_info[DT_FINI]) {
			atexit((void (*)(void)) (tpnt->loadaddr + tpnt->dynamic_info[DT_FINI]));
		}

	}
#endif

#ifdef USE_CACHE
	_dl_unmap_cache();
#endif
	return (void *) dyn_chain;

  oops:
	/* Something went wrong.  Clean up and return NULL. */
#ifdef USE_CACHE
	_dl_unmap_cache();
#endif
	do_dlclose(dyn_chain, 0);
	return NULL;
}

static int do_fixup(struct elf_resolve *tpnt, int flag)
{
	int goof = 0;

	if (tpnt->next)
		goof += do_fixup(tpnt->next, flag);

	if (tpnt->dynamic_info[DT_REL]) {
#ifdef ELF_USES_RELOCA
		goof++;
#else
		if (tpnt->init_flag & RELOCS_DONE)
			return goof;
		tpnt->init_flag |= RELOCS_DONE;

		goof += _dl_parse_relocation_information(tpnt, 
			tpnt->dynamic_info[DT_REL], tpnt->dynamic_info[DT_RELSZ], 0);
#endif
	}
	if (tpnt->dynamic_info[DT_RELA]) {
#ifdef ELF_USES_RELOCA
		if (tpnt->init_flag & RELOCS_DONE)
			return goof;
		tpnt->init_flag |= RELOCS_DONE;

		goof += _dl_parse_relocation_information(tpnt, 
			tpnt->dynamic_info[DT_RELA], tpnt->dynamic_info[DT_RELASZ], 0);
#else
		goof++;
#endif
	}
	if (tpnt->dynamic_info[DT_JMPREL]) {
		if (tpnt->init_flag & JMP_RELOCS_DONE)
			return goof;
		tpnt->init_flag |= JMP_RELOCS_DONE;

		if (flag == RTLD_LAZY) {
			_dl_parse_lazy_relocation_information(tpnt, 
				tpnt->dynamic_info[DT_JMPREL], 
				tpnt->dynamic_info[DT_PLTRELSZ], 0);
		} else {
			goof += _dl_parse_relocation_information(tpnt, 
				tpnt->dynamic_info[DT_JMPREL], 
				tpnt->dynamic_info[DT_PLTRELSZ], 0);
		}
	};
	return goof;
}

void *_dlsym(void *vhandle, const char *name)
{
	struct elf_resolve *tpnt, *tfrom;
	struct dyn_elf *handle;
	char *from;
	struct dyn_elf *rpnt;
	void *ret;

	handle = (struct dyn_elf *) vhandle;

	/* First of all verify that we have a real handle
	   of some kind.  Return NULL if not a valid handle. */

	if (handle == NULL)
		handle = _dl_symbol_tables;
	else if (handle != RTLD_NEXT && handle != _dl_symbol_tables) {
		for (rpnt = _dl_handles; rpnt; rpnt = rpnt->next_handle)
			if (rpnt == handle)
				break;
		if (!rpnt) {
			_dl_error_number = LD_BAD_HANDLE;
			return NULL;
		}
	} else if (handle == RTLD_NEXT) {
		/*
		 * Try and locate the module we were called from - we
		 * need this so that we know where to start searching
		 * from.  We never pass RTLD_NEXT down into the actual
		 * dynamic loader itself, as it doesn't know
		 * how to properly treat it.
		 */
		from = __builtin_return_address(0);

		tfrom = NULL;
		for (rpnt = _dl_symbol_tables; rpnt; rpnt = rpnt->next) {
			tpnt = rpnt->dyn;
			if (tpnt->loadaddr < from
				&& (tfrom == NULL || tfrom->loadaddr < tpnt->loadaddr)) {
				tfrom = tpnt;
				handle = rpnt->next;
			}
		}
	}

	ret = _dl_find_hash((char*)name, handle, NULL, copyrel);

	/*
	 * Nothing found.
	 */
	if (!ret)
		_dl_error_number = LD_NO_SYMBOL;
	return ret;
}

int _dlclose(void *vhandle)
{
	return do_dlclose(vhandle, 1);
}

static int do_dlclose(void *vhandle, int need_fini)
{
	struct dyn_elf *rpnt, *rpnt1;
	struct dyn_elf *spnt, *spnt1;
	elf_phdr *ppnt;
	struct elf_resolve *tpnt;
	int (*dl_elf_fini) (void);
	void (*dl_brk) (void);
	struct dyn_elf *handle;
	unsigned int end;
	int i = 0;

	handle = (struct dyn_elf *) vhandle;
	rpnt1 = NULL;
	for (rpnt = _dl_handles; rpnt; rpnt = rpnt->next_handle) {
		if (rpnt == handle) {
			break;
		}
		rpnt1 = rpnt;
	}

	if (!rpnt) {
		_dl_error_number = LD_BAD_HANDLE;
		return 1;
	}

	/* OK, this is a valid handle - now close out the file.
	 * We check if we need to call fini () on the handle. */
	spnt = need_fini ? handle : handle->next;
	for (; spnt; spnt = spnt1) {
		spnt1 = spnt->next;

		/* We appended the module list to the end - when we get back here, 
		   quit. The access counts were not adjusted to account for being here. */
		if (spnt == _dl_symbol_tables)
			break;
		if (spnt->dyn->usage_count == 1
			&& spnt->dyn->libtype == loaded_file) {
			tpnt = spnt->dyn;
			/* Apparently crt1 for the application is responsible for handling this.
			 * We only need to run the init/fini for shared libraries
			 */

			if (tpnt->dynamic_info[DT_FINI]) {
				dl_elf_fini = (int (*)(void)) (tpnt->loadaddr + 
					tpnt->dynamic_info[DT_FINI]);
				(*dl_elf_fini) ();
			}
		}
	}
	if (rpnt1)
		rpnt1->next_handle = rpnt->next_handle;
	else
		_dl_handles = rpnt->next_handle;

	/* OK, this is a valid handle - now close out the file */
	for (rpnt = handle; rpnt; rpnt = rpnt1) {
		rpnt1 = rpnt->next;

		/* We appended the module list to the end - when we get back here, 
		   quit. The access counts were not adjusted to account for being here. */
		if (rpnt == _dl_symbol_tables)
			break;

		rpnt->dyn->usage_count--;
		if (rpnt->dyn->usage_count == 0
			&& rpnt->dyn->libtype == loaded_file) {
			tpnt = rpnt->dyn;
			/* Apparently crt1 for the application is responsible for handling this.
			 * We only need to run the init/fini for shared libraries
			 */
#if 0

			/* We have to do this above, before we start closing objects.  
			 * Otherwise when the needed symbols for _fini handling are 
			 * resolved a coredump would occur. Rob Ryan (robr@cmu.edu)*/ 
			if (tpnt->dynamic_info[DT_FINI]) { 
			    dl_elf_fini = (int (*)(void)) (tpnt->loadaddr + tpnt->dynamic_info[DT_FINI]);
				(*dl_elf_fini) ();
			}
#endif
			end = 0;
			for (i = 0, ppnt = rpnt->dyn->ppnt;
				 i < rpnt->dyn->n_phent; ppnt++, i++) {
				if (ppnt->p_type != PT_LOAD)
					continue;
				if (end < ppnt->p_vaddr + ppnt->p_memsz)
					end = ppnt->p_vaddr + ppnt->p_memsz;
			}
			_dl_munmap(rpnt->dyn->loadaddr, end);
			/* Next, remove rpnt->dyn from the loaded_module list */
			if (_dl_loaded_modules == rpnt->dyn) {
				_dl_loaded_modules = rpnt->dyn->next;
				if (_dl_loaded_modules)
					_dl_loaded_modules->prev = 0;
			} else
				for (tpnt = _dl_loaded_modules; tpnt; tpnt = tpnt->next)
					if (tpnt->next == rpnt->dyn) {
						tpnt->next = tpnt->next->next;
						if (tpnt->next)
							tpnt->next->prev = tpnt;
						break;
					}
			free(rpnt->dyn->libname);
			free(rpnt->dyn);
		}
		free(rpnt);
	}


	if (_dl_debug_addr) {
	    dl_brk = (void (*)(void)) _dl_debug_addr->r_brk;
	    if (dl_brk != NULL) {
		_dl_debug_addr->r_state = RT_DELETE;
		(*dl_brk) ();

		_dl_debug_addr->r_state = RT_CONSISTENT;
		(*dl_brk) ();
	    }
	}

	return 0;
}

const char *_dlerror(void)
{
	const char *retval;

	if (!_dl_error_number)
		return NULL;
	retval = dl_error_names[_dl_error_number];
	_dl_error_number = 0;
	return retval;
}

/*
 * Dump information to stderrr about the current loaded modules
 */
static char *type[] = { "Lib", "Exe", "Int", "Mod" };

void _dlinfo(void)
{
	struct elf_resolve *tpnt;
	struct dyn_elf *rpnt, *hpnt;

	_dl_dprintf(2, "List of loaded modules\n");
	/* First start with a complete list of all of the loaded files. */
	for (tpnt = _dl_loaded_modules; tpnt; tpnt = tpnt->next) { 
		_dl_dprintf(2, "\t%x %x %x %s %d %s\n", 
			(unsigned) tpnt->loadaddr, (unsigned) tpnt,
			(unsigned) tpnt->symbol_scope,
			type[tpnt->libtype],
			tpnt->usage_count, tpnt->libname);
	}

	/* Next dump the module list for the application itself */
	_dl_dprintf(2, "\nModules for application (%x):\n",
				 (unsigned) _dl_symbol_tables);
	for (rpnt = _dl_symbol_tables; rpnt; rpnt = rpnt->next)
		_dl_dprintf(2, "\t%x %s\n", (unsigned) rpnt->dyn, rpnt->dyn->libname);

	for (hpnt = _dl_handles; hpnt; hpnt = hpnt->next_handle) {
		_dl_dprintf(2, "Modules for handle %x\n", (unsigned) hpnt);
		for (rpnt = hpnt; rpnt; rpnt = rpnt->next)
			_dl_dprintf(2, "\t%x %s\n", (unsigned) rpnt->dyn, 
				rpnt->dyn->libname);
	}
}

int _dladdr(void *__address, Dl_info * __dlip)
{
	struct elf_resolve *pelf;
	struct elf_resolve *rpnt;

#ifdef USE_CACHE
	_dl_map_cache();
#endif

	/*
	 * Try and locate the module address is in
	 */
	pelf = NULL;

#if 0
	_dl_dprintf(2, "dladdr( 0x%p, 0x%p )\n", __address, __dlip);
#endif

	for (rpnt = _dl_loaded_modules; rpnt; rpnt = rpnt->next) {
		struct elf_resolve *tpnt;

		tpnt = rpnt;
#if 0
		_dl_dprintf(2, "Module \"%s\" at 0x%p\n", 
			tpnt->libname, tpnt->loadaddr);
#endif
		if (tpnt->loadaddr < (char *) __address
			&& (pelf == NULL || pelf->loadaddr < tpnt->loadaddr)) {
		    pelf = tpnt;
		}
	}

	if (!pelf) {
		return 0;
	}

	/*
	 * Try and locate the symbol of address
	 */

	{
		char *strtab;
		Elf32_Sym *symtab;
		int hn, si;
		int sf;
		int sn = 0;
		void *sa = 0;

		symtab = (Elf32_Sym *) (pelf->dynamic_info[DT_SYMTAB] + pelf->loadaddr);
		strtab = (char *) (pelf->dynamic_info[DT_STRTAB] + pelf->loadaddr);

		sf = 0;
		for (hn = 0; hn < pelf->nbucket; hn++) {
			for (si = pelf->elf_buckets[hn]; si; si = pelf->chains[si]) {
				void *symbol_addr;

				symbol_addr = pelf->loadaddr + symtab[si].st_value;
				if (symbol_addr <= __address && (!sf || sa < symbol_addr)) {
					sa = symbol_addr;
					sn = si;
					sf = 1;
				}
#if 0
				_dl_dprintf(2, "Symbol \"%s\" at 0x%p\n", 
					strtab + symtab[si].st_name, symbol_addr);
#endif
			}
		}

		if (sf) {
			__dlip->dli_fname = pelf->libname;
			__dlip->dli_fbase = pelf->loadaddr;
			__dlip->dli_sname = strtab + symtab[sn].st_name;
			__dlip->dli_saddr = sa;
		}
		return 1;
	}
}
