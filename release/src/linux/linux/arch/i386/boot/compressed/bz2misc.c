/*
 * misc.c

	This was butchered from misc.c to support bzip2 instead of gzip,
	by Tom Oehser, Tom@Toms.NET

 *
 * malloc by Hannu Savolainen 1993 and Matthias Urlichs 1994
 * puts by Nick Holloway 1993, better puts by Martin Mares 1995
 * High loaded stuff by Hans Lermen & Werner Almesberger, Feb. 1996
 */

#include <asm/segment.h>
#include <asm/io.h>

/*
 * gzip declarations
 */

#define OF(args)  args
#define STATIC static

#undef memset
#undef memcpy
#define memzero(s, n)     memset ((s), 0, (n))

typedef unsigned char  uch;
typedef unsigned short ush;
typedef unsigned long  ulg;

// #define WSIZE 0x8000		/* Window size must be at least 32k, */
				/* and a power of two */

// static uch *inbuf;	     /* input buffer */
// static uch window[WSIZE];    /* Sliding window buffer */

// static unsigned insize = 0;  /* valid bytes in inbuf */
// static unsigned inptr = 0;   /* index of next byte to be processed in inbuf */
// static unsigned outcnt = 0;  /* bytes in output buffer */

/* gzip flag byte */
#define ASCII_FLAG   0x01 /* bit 0 set: file probably ASCII text */
#define CONTINUATION 0x02 /* bit 1 set: continuation of multi-part gzip file */
#define EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define COMMENT      0x10 /* bit 4 set: file comment present */
#define ENCRYPTED    0x20 /* bit 5 set: file is encrypted */
#define RESERVED     0xC0 /* bit 6,7:   reserved */

#define get_byte()  (inptr < insize ? inbuf[inptr++] : fill_inbuf())
		
/* Diagnostic functions */
#ifdef DEBUG
#  define Assert(cond,msg) {if(!(cond)) error(msg);}
#  define Trace(x) fprintf x
#  define Tracev(x) {if (verbose) fprintf x ;}
#  define Tracevv(x) {if (verbose>1) fprintf x ;}
#  define Tracec(c,x) {if (verbose && (c)) fprintf x ;}
#  define Tracecv(c,x) {if (verbose>1 && (c)) fprintf x ;}
#else
#  define Assert(cond,msg)
#  define Trace(x)
#  define Tracev(x)
#  define Tracevv(x)
#  define Tracec(c,x)
#  define Tracecv(c,x)
#endif

//static int  fill_inbuf(void);
//static void flush_window(void);
static void error(char *m);
/*
static void gzip_mark(void **);
static void gzip_release(void **);
*/
  
/*
 * This is set up by the setup-routine at boot-time
 */
static unsigned char *real_mode; /* Pointer to real-mode data */

#define EXT_MEM_K   (*(unsigned short *)(real_mode + 0x2))
#ifndef STANDARD_MEMORY_BIOS_CALL
#define ALT_MEM_K   (*(unsigned long *)(real_mode + 0x1e0))
#endif
#define SCREEN_INFO (*(struct screen_info *)(real_mode+0))

extern char input_data[];
extern int  input_len;
extern int  end;

static long free_mem_ptr = (long)&end;

static long free_mem_end_ptr;
//static long bytes_out = 0;
//static unsigned long output_ptr = 0;

//static uch *output_data;
static void *malloc(int size);
static void free(void *where);
static void error(char *m);
static void puts(const char *);

#define INPLACE_MOVE_ROUTINE  0x1000
#define LOW_BUFFER_START      0x2000
#define LOW_BUFFER_MAX       0x90000
// #define HEAP_SIZE             0x2400
#define HEAP_SIZE             2500000 // big for bzip2
//static unsigned int low_buffer_end, low_buffer_size;
static int high_loaded =0;
static uch *high_buffer_start /* = (uch *)(((ulg)&end) + HEAP_SIZE)*/;

static char *vidmem = (char *)0xb8000;
static int vidport;
static int lines, cols;

#include "../../../../lib/bzip2_inflate.c"

static void *malloc(int size)
{
	void *p;

	if (size <0) error("Malloc error\n");
	if (free_mem_ptr <= 0) error("Memory error\n");

	free_mem_ptr = (free_mem_ptr + 3) & ~3;	/* Align */

	p = (void *)free_mem_ptr;
	free_mem_ptr += size;

	if (free_mem_ptr >= free_mem_end_ptr)
		error("\nOut of memory\n");

	return p;
}

static void free(void *where)
{	/* Don't care */
}

static void scroll(void)
{
	int i;

	memcpy ( vidmem, vidmem + cols * 2, ( lines - 1 ) * cols * 2 );
	for ( i = ( lines - 1 ) * cols * 2; i < lines * cols * 2; i += 2 )
		vidmem[i] = ' ';
}

static void puts(const char *s)
{
	int x,y,pos;
	char c;

	x = SCREEN_INFO.orig_x;
	y = SCREEN_INFO.orig_y;

	while ( ( c = *s++ ) != '\0' ) {
		if ( c == '\n' ) {
			x = 0;
			if ( ++y >= lines ) {
				scroll();
				y--;
			}
		} else {
			vidmem [ ( x + cols * y ) * 2 ] = c;
			if ( ++x >= cols ) {
				x = 0;
				if ( ++y >= lines ) {
					scroll();
					y--;
				}
			}
		}
	}

	SCREEN_INFO.orig_x = x;
	SCREEN_INFO.orig_y = y;

	pos = (x + cols * y) * 2;	/* Update cursor position */
	outb_p(14, vidport);
	outb_p(0xff & (pos >> 9), vidport+1);
	outb_p(15, vidport);
	outb_p(0xff & (pos >> 1), vidport+1);
}

/*
static void memset(void* s, int c, size_t n)
{
	int i;
	char *ss = (char*)s;

	for (i=0;i<n;i++) ss[i] = c;
}
*/

static void memcpy(void* __dest, __const void* __src,
			    size_t __n)
{
	int i;
	char *d = (char *)__dest, *s = (char *)__src;

	for (i=0;i<__n;i++) d[i] = s[i];
}

static void error(char *x)
{
	puts("\n\n");
	puts(x);
	puts("\n\n -- System halted");
	while(1);	/* Halt */
}

#define STACK_SIZE (4096)

long user_stack [STACK_SIZE];

struct {
	long * a;
	short b;
	} stack_start = { & user_stack [STACK_SIZE] , __KERNEL_DS };

struct moveparams {
	uch *low_buffer_start;  int lcount;
	uch *high_buffer_start; int hcount;
};

void setup_output_buffer_if_we_run_high(struct moveparams *mv)
{
	high_buffer_start = (uch *)(((ulg)&end) + HEAP_SIZE);
	high_loaded = 1;
	free_mem_end_ptr = (long)high_buffer_start;
	mv->hcount = -1;
	mv->high_buffer_start = high_buffer_start;
}

int decompress_kernel(struct moveparams *mv, void *rmode)
{
	int ret;

	real_mode = rmode;

	if (SCREEN_INFO.orig_video_mode == 7) {
		vidmem = (char *) 0xb0000;
		vidport = 0x3b4;
	} else {
		vidmem = (char *) 0xb8000;
		vidport = 0x3d4;
	}

	lines = SCREEN_INFO.orig_video_lines;
	cols = SCREEN_INFO.orig_video_cols;

	setup_output_buffer_if_we_run_high(mv);
	puts("Bzip2: Uncompressing Linux...");
	ret = BZ2_bzBuffToBuffDecompress(mv->high_buffer_start, &(mv->hcount),
					 input_data, input_len, 1, 1);
	puts("\nOk, booting the kernel.\n");
	return high_loaded;
}


/* the-end. */
