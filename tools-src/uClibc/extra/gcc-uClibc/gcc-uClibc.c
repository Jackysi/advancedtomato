/*
 * Copyright (C) 2000 Manuel Novoa III
 * Copyright (C) 2002 Erik Andersen
 *
 * This is a crude wrapper to use uClibc with gcc.
 * It was originally written to work around ./configure for ext2fs-utils.
 * It certainly can be improved, but it works for me in the normal cases.
 *
 * April 7, 2001
 *
 * A bug was fixed in building the gcc command line when dynamic linking.
 * The functions dlopen, etc. now work.  At this time, you must make sure
 * the correct libdl.so is included however.  It is safest to, for example,
 * add /lib/libdl.so.1 if using ld-linux.so.1 rather than adding -ldl to the
 * command line.
 *
 * Note: This is only a problem if devel and target archs are the same.  To
 * avoid the problem, you can use a customized dynamic linker.
 *
 *
 * April 18, 2001
 *
 * The wrapper now works with either installed and uninstalled uClibc versions.
 * If you want to use the uninstalled header files and libs, either include
 * the string "build" in the invocation name such as
 *       'ln -s <ARCH>-uclibc-gcc <ARCH>-uclibc-gcc-build'
 * or include it in the environment variable setting of UCLIBC_GCC.
 * Note: This automatically enables the "rpath" behavior described below.
 *
 * The wrapper will now pass the location of the uClibc shared libs used to
 * the linker with the "-rpath" option if the invocation name includes the
 * string "rpath" or if the environment variable UCLIBC_GCC include it (as
 * with "build" above).  This is primarily intended to be used on devel
 * platforms of the same arch as the target.  A good place to use this feature
 * would be in the uClibc/test directory.
 *
 * The wrapper now displays the command line passed to gcc when '-v' is used.
 *
 * May 31, 2001
 *
 * "rpath" and "build" behavior are now decoupled.  You can of course get
 * the old "build" behavior by setting UCLIBC_GCC="rpath-build".  Order
 * isn't important here, as only the substrings are searched for.
 *
 * Added environment variable check for UCLIBC_GCC_DLOPT to let user specify
 * an alternative dynamic linker at runtime without using command line args.
 * Since this wouldn't commonly be used, I made it easy on myself.  You have
 * to match the option you would have passed to the gcc wrapper.  As an
 * example,
 *
 *   export UCLIBC_GCC_DLOPT="-Wl,--dynamic-linker,/lib/ld-alt-linker.so.3"
 *
 * This is really only useful if target arch == devel arch and DEVEL_PREFIX
 * isn't empty.  It involves a recompile, but you can at least test apps
 * on your devel system if combined with the "rpath" behavor if by using
 * LD_LIBRARY_PATH, etc.
 *
 * Also added check for "-Wl,--dynamic-linker" on the command line.  The
 * use default dynamic linker or the envirnment-specified dynamic linker
 * is disabled in that case.
 *
 * Added options --uclibc-use-build-dir and --uclibc-use-rpath so that those
 * behaviors can be invoked from the command line.
 *
 */

/*
 *
 * TODO:
 * Check/modify gcc-specific environment variables?
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

#include "gcc-uClibc.h"

static char *our_usr_lib_path = "-L"UCLIBC_DEVEL_PREFIX"/lib";

static char static_linking[] = "-static";
static char nostdinc[] = "-nostdinc";
static char nostartfiles[] = "-nostartfiles";
static char nodefaultlibs[] = "-nodefaultlibs";
static char nostdlib[] = "-nostdlib";
#ifdef __UCLIBC_CTOR_DTOR__
static char nostdinc_plus[] = "-nostdinc++";
#endif


extern void *xmalloc(size_t size)
{
	void *ptr = malloc(size);

	if (!ptr) {
	    fprintf(stderr, "memory exhausted");
	    exit(EXIT_FAILURE);
	}
	return ptr;
}

void xstrcat(char **string, ...)
{
	const char *c;
	va_list p; 
	/* Don't bother to calculate how big exerything 
	 * will be, just be careful to not overflow...  */
	va_start(p, string);
	*string = xmalloc(BUFSIZ);
	**string = '\0';
	while(1) {
	    if (!(c = va_arg(p, const char *)))
		break;
	    strcat(*string, c); 
	}
	va_end(p);
}

int main(int argc, char **argv)
{
	int use_build_dir = 0, linking = 1, use_static_linking = 0;
	int use_stdinc = 1, use_start = 1, use_stdlib = 1, use_pic = 0;
	int source_count = 0, use_rpath = 0, verbose = 0;
	int i, j, k, l, m, n;
	char ** gcc_argv;
	char ** gcc_argument;
	char ** libraries;
	char ** libpath;
	char *dlstr;
	char *incstr;
	char *devprefix;
	char *builddir;
	char *libstr;
	char *build_dlstr;
	char *ep;
	char *rpath_link[2];
	char *rpath[2];
	char *uClibc_inc[2];
	char *our_lib_path[2];
	char *crt0_path[2];
	const char *application_name;
#ifdef __UCLIBC_CTOR_DTOR__
	char *crti_path[2];
	char *crtn_path[2];
	int len;
	int ctor_dtor = 1, cplusplus = 0, use_nostdinc_plus = 0;
	char *GPLUSPLUS_BIN = NULL;
#endif

	application_name = basename(argv[0]);
	if (application_name[0] == '-')
		application_name++;

#ifdef __UCLIBC_CTOR_DTOR__
	/* We must use strstr since g++ might be named like a
	 * cross compiler (i.e. arm-linux-g++).   We must also
	 * search carefully, in case we are searching something 
	 * like /opt/c++/gcc-3.1/bin/arm-linux-g++ or some similar 
	 * perversion...  */
	len = strlen(application_name);
	if ((strcmp(application_name+len-3, "g++")==0) ||
		(strcmp(application_name+len-3, "c++")==0)) {
	    char *gcc_bin = GCC_BIN;
	    len = strlen(gcc_bin);
	    if (strcmp(gcc_bin+len-3, "gcc")==0) {
		GPLUSPLUS_BIN = strdup(gcc_bin);
		GPLUSPLUS_BIN[len-1]='+';
		GPLUSPLUS_BIN[len-2]='+';
	    }
	    cplusplus = 1;
	    use_nostdinc_plus = 1;
	}
#endif

	devprefix = getenv("UCLIBC_DEVEL_PREFIX");
	if (!devprefix) {
		devprefix = UCLIBC_DEVEL_PREFIX;
	}

	builddir = getenv("UCLIBC_BUILD_DIR");
	if (!builddir) {
		builddir = UCLIBC_BUILD_DIR;
	}

	incstr = getenv("UCLIBC_GCC_INC");
	libstr = getenv("UCLIBC_GCC_LIB");

	xstrcat(&(rpath_link[0]), "-Wl,-rpath-link,", devprefix, "/lib", NULL);
	xstrcat(&(rpath_link[1]), "-Wl,-rpath-link,", builddir, "/lib", NULL);

	xstrcat(&(rpath[0]), "-Wl,-rpath,", devprefix, "/lib", NULL);
	xstrcat(&(rpath[1]), "-Wl,-rpath,", builddir, "/lib", NULL);

	xstrcat(&(uClibc_inc[0]), devprefix, "/include/", NULL);
	xstrcat(&(uClibc_inc[1]), builddir, "/include/", NULL);

#ifdef __UCLIBC_CTOR_DTOR__
	xstrcat(&(crt0_path[0]), devprefix, "/lib/crt1.o", NULL);
	xstrcat(&(crt0_path[1]), builddir, "/lib/crt1.o", NULL);
	xstrcat(&(crti_path[0]), devprefix, "/lib/crti.o", NULL);
	xstrcat(&(crti_path[1]), builddir, "/lib/crti.o", NULL);
	xstrcat(&(crtn_path[0]), devprefix, "/lib/crtn.o", NULL);
	xstrcat(&(crtn_path[1]), builddir, "/lib/crtn.o", NULL);
#else
	xstrcat(&(crt0_path[0]), devprefix, "/lib/crt0.o", NULL);
	xstrcat(&(crt0_path[1]), builddir, "/lib/crt0.o", NULL);
#endif

	xstrcat(&(our_lib_path[0]), "-L", devprefix, "/lib", NULL);
	xstrcat(&(our_lib_path[1]), "-L", builddir, "/lib", NULL);

	build_dlstr = "-Wl,--dynamic-linker," BUILD_DYNAMIC_LINKER;
	dlstr = getenv("UCLIBC_GCC_DLOPT");
	if (!dlstr) {
		dlstr = "-Wl,--dynamic-linker," DYNAMIC_LINKER;
	}

	ep = getenv("UCLIBC_GCC");
	if (!ep) {
		ep = "";
	}

	if (strstr(ep,"build") != 0) {
		use_build_dir = 1;
	}

	if (strstr(ep,"rpath") != 0) {
		use_rpath = 1;
	}

	m = 0;
	libraries = __builtin_alloca(sizeof(char*) * (argc));
	libraries[m] = '\0';

	n = 0;
	libpath = __builtin_alloca(sizeof(char*) * (argc));
	libpath[n] = '\0';

	for ( i = 1 ; i < argc ; i++ ) {
		if (argv[i][0] == '-') { /* option */
			switch (argv[i][1]) {
				case 'c':		/* compile or assemble */
				case 'S':		/* generate assembler code */
				case 'E':		/* preprocess only */
				case 'M':	    /* generate dependencies */
					linking = 0;
					break;
				case 'L': 		/* library */
					libpath[n++] = argv[i];
					libpath[n] = '\0';
					if (argv[i][2] == 0) {
					    argv[i] = '\0';
					    libpath[n++] = argv[++i];
					    libpath[n] = '\0';
					}
					argv[i] = '\0';
					break;
				case 'l': 		/* library */
					libraries[m++] = argv[i];
					libraries[m] = '\0';
					argv[i] = '\0';
					break;
				case 'v':		/* verbose */
					if (argv[i][2] == 0) verbose = 1;
					printf("Invoked as %s\n", argv[0]);
					break;
				case 'n':
					if (strcmp(nostdinc,argv[i]) == 0) {
						use_stdinc = 0;
					} else if (strcmp(nostartfiles,argv[i]) == 0) {
#ifdef __UCLIBC_CTOR_DTOR__
						ctor_dtor = 0;
#endif
						use_start = 0;
					} else if (strcmp(nodefaultlibs,argv[i]) == 0) {
						use_stdlib = 0;
						argv[i] = '\0';
					} else if (strcmp(nostdlib,argv[i]) == 0) {
#ifdef __UCLIBC_CTOR_DTOR__
						ctor_dtor = 0;
#endif
						use_start = 0;
						use_stdlib = 0;
					} 
#ifdef __UCLIBC_CTOR_DTOR__
					else if (strcmp(nostdinc_plus,argv[i]) == 0) {
						if (cplusplus==1) {
							use_nostdinc_plus = 0;
						}
					}
#endif
					break;
				case 's':
					if (strstr(argv[i],static_linking) != NULL) {
						use_static_linking = 1;
					}
					if (strcmp("-shared",argv[i]) == 0) {
						use_start = 0;
						use_pic = 1;
					}
					break;
				case 'W':		/* -static could be passed directly to ld */
					if (strncmp("-Wl,",argv[i],4) == 0) {
						if (strstr(argv[i],static_linking) != 0) {
							use_static_linking = 1;
						}
						if (strstr(argv[i],"--dynamic-linker") != 0) {
							dlstr = 0;
						}
					}
					break;
				case 'f':
					/* Check if we are doing PIC */
					if (strcmp("-fPIC",argv[i]) == 0) {
					    use_pic = 1;
					} else if (strcmp("-fpic",argv[i]) == 0) {
					    use_pic = 1;
					}
					break;

				case '-':
					if (strstr(argv[i]+1,static_linking) != NULL) {
					    use_static_linking = 1;
					    argv[i]='\0';
					} else if (strcmp("--uclibc-use-build-dir",argv[i]) == 0) {
					    use_build_dir = 1;
					    argv[i]='\0';
					} else if (strcmp("--uclibc-use-rpath",argv[i]) == 0) {
					    use_rpath = 1;
					    argv[i]='\0';
					}
#ifdef __UCLIBC_CTOR_DTOR__
					else if (strcmp("--uclibc-no-ctors",argv[i]) == 0) {
					    ctor_dtor = 0;
					    argv[i]='\0';
					}
#endif
					break;
			}
		} else {				/* assume it is an existing source file */
			++source_count;
		}
	}

	gcc_argv = __builtin_alloca(sizeof(char*) * (argc + 64));
	gcc_argument = __builtin_alloca(sizeof(char*) * (argc + 20));

	i = 0; k = 0;
#ifdef __UCLIBC_CTOR_DTOR__
	if (cplusplus && GPLUSPLUS_BIN)
	    gcc_argv[i++] = GPLUSPLUS_BIN;
	else
#endif
	    gcc_argv[i++] = GCC_BIN;
	
	for ( j = 1 ; j < argc ; j++ ) {
		if (argv[j]=='\0') {
		    continue;
		} else {
			gcc_argument[k++] = argv[j];
			gcc_argument[k] = '\0';
		}
	}

	if (linking && source_count) {
#if defined __HAVE_ELF__ && ! defined __UCLIBC_HAS_MMU__
	    gcc_argv[i++] = "-Wl,-elf2flt";
#endif
	    gcc_argv[i++] = nostdlib;
	    if (use_static_linking) {
		gcc_argv[i++] = static_linking;
	    }
	    if (!use_static_linking) {
		if (dlstr && use_build_dir) {
		    gcc_argv[i++] = build_dlstr;
		} else if (dlstr) {
		    gcc_argv[i++] = dlstr;
		}
		if (use_rpath) {
		    gcc_argv[i++] = rpath[use_build_dir];
		}
	    }
	    for ( l = 0 ; l < n ; l++ ) {
		if (libpath[l]) gcc_argv[i++] = libpath[l];
	    }
	    gcc_argv[i++] = rpath_link[use_build_dir]; /* just to be safe */
	    if( libstr )
		gcc_argv[i++] = libstr;
	    gcc_argv[i++] = our_lib_path[use_build_dir];
	    if (!use_build_dir) {
		gcc_argv[i++] = our_usr_lib_path;
	    }
	}
	if (use_stdinc && source_count) {
	    gcc_argv[i++] = nostdinc;
#ifdef __UCLIBC_CTOR_DTOR__
	    if (cplusplus) {
		char *cppinc;
		if (use_nostdinc_plus) {
			gcc_argv[i++] = nostdinc_plus;
		}
		xstrcat(&cppinc, uClibc_inc[use_build_dir], "g++/", NULL);
		gcc_argv[i++] = "-isystem";
		gcc_argv[i++] = cppinc;
		xstrcat(&cppinc, uClibc_inc[use_build_dir], "g++-v3/", NULL);
		gcc_argv[i++] = "-isystem";
		gcc_argv[i++] = cppinc;
	    }
#endif
	    gcc_argv[i++] = "-isystem";
	    gcc_argv[i++] = uClibc_inc[use_build_dir];
	    gcc_argv[i++] = "-iwithprefix";
	    gcc_argv[i++] = "include";
	    if( incstr )
		gcc_argv[i++] = incstr;
	}

	if (linking && source_count) {

#ifdef __UCLIBC_CTOR_DTOR__
	    if (ctor_dtor) {
	        gcc_argv[i++] = crti_path[use_build_dir];
	        if (use_pic) {
	      	    gcc_argv[i++] = LIBGCC_DIR "crtbeginS.o" ;
		} else {
		    gcc_argv[i++] = LIBGCC_DIR "crtbegin.o" ;
		}
	    }
#endif
	    if (use_start) {
		gcc_argv[i++] = crt0_path[use_build_dir];
	    }
	    for ( l = 0 ; l < k ; l++ ) {
		if (gcc_argument[l]) gcc_argv[i++] = gcc_argument[l];
	    }
	    if (use_stdlib) {
		//gcc_argv[i++] = "-Wl,--start-group";
		gcc_argv[i++] = "-lgcc";
	    }
	    for ( l = 0 ; l < m ; l++ ) {
		if (libraries[l]) gcc_argv[i++] = libraries[l];
	    }
	    if (use_stdlib) {
#ifdef __UCLIBC_CTOR_DTOR__
		if (cplusplus) {
		    gcc_argv[ i++ ] = "-lstdc++";
		    gcc_argv[ i++ ] = "-lm";
		}
#endif
		gcc_argv[i++] = "-lc";
		gcc_argv[i++] = "-lgcc";
		//gcc_argv[i++] = "-Wl,--end-group";
	    }
#ifdef __UCLIBC_CTOR_DTOR__
 	    if (ctor_dtor) {
		if (use_pic) {
		    gcc_argv[i++] = LIBGCC_DIR "crtendS.o" ;
		} else {
		    gcc_argv[i++] = LIBGCC_DIR "crtend.o" ;
		}    
	        gcc_argv[i++] = crtn_path[use_build_dir];
  	    }
#endif
	} else {
	    for ( l = 0 ; l < k ; l++ ) {
		if (gcc_argument[l]) gcc_argv[i++] = gcc_argument[l];
	    }
	}
	gcc_argv[i++] = NULL;

	if (verbose) {
		for ( j = 0 ; gcc_argv[j] ; j++ ) {
			printf("arg[%2i] = %s\n", j, gcc_argv[j]);
		}
		fflush(stdout);
	}
	//no need to free memory from xstrcat because we never return... 
#ifdef __UCLIBC_CTOR_DTOR__
	if (cplusplus && GPLUSPLUS_BIN)
	    return execvp(GPLUSPLUS_BIN, gcc_argv);
	else
#endif
	    return execvp(GCC_BIN, gcc_argv);
}
