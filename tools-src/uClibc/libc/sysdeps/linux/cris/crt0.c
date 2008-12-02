/* Startup code compliant to the ELF CRIS ABI */

/* The first piece of initialized data.  */
int __data_start = 0;

/* 
 * It is important that this be the first function.
 * This file is the first thing in the text section.  
 */
void
_start ()
{
	/* 
	 * On the stack we have argc. We can calculate argv/envp
	 * from that and the succeeding stack location, but fix so
	 * we get the right calling convention (regs in r10/r11).
	 *
	 * Please view linux/fs/binfmt_elf.c for a complete
	 * understanding of this.
	 */
	__asm__ volatile("pop $r10");
	__asm__ volatile("move.d $sp, $r11");
	__asm__ volatile("jump start1");
}

#include <features.h>

extern void __uClibc_main(int argc, char **argv, char **envp)
         __attribute__ ((__noreturn__));
extern void __uClibc_start_main(int argc, char **argv, char **envp, 
	void (*app_init)(void), void (*app_fini)(void))
         __attribute__ ((__noreturn__));
extern void weak_function _init(void);
extern void weak_function _fini(void);

static void
start1 (int argc, char **argv)
{
	char** environ;

	/* The environment starts just after ARGV.  */
	environ = &argv[argc + 1];
	
	/* 
	 * If the first thing after ARGV is the arguments
	 * themselves, there is no environment.  
	 */
	if ((char *) environ == *argv)
		/* 
		 * The environment is empty.  Make environ
		 * point at ARGV[ARGC], which is NULL.  
		 */
		--environ;
	
#if defined L_crt0 || ! defined __UCLIBC_CTOR_DTOR__
	/* Leave control to the libc */
	__uClibc_main(argc, argv, environ);
#else
	__uClibc_start_main(argc, argv, environ, _init, _fini);
#endif
}
