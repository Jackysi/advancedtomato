/*
 *    By RidgeRun Inc.
 *
 *    The input to this program is intended to be
 *    a compressed linux kernel. The output of this
 *    program is then a constructed *.S file which
 *    defines a large data structure -- the contents
 *    of which represent the compressed kernel which
 *    can subsequently be used in a program designed
 *    to access that struture for decompression at
 *    runtime and then subsequent kernel bootup.
 *
 *    Example Usage:
 *       ./doit < piggy.gz > piggy.S
 *
 */

#include <stdio.h>

void printval(int i)
{
	int tth, th, h, t, d;

	if (i > 99999) {
		printf("Error - printval outofbounds\n");
		return;
	}

	tth = 0;
	th = 0;
	//tth = (i) / 10000;
	//th  = (i - (tth * 10000)) / 1000;
	h = (i - ((tth * 10000) + (th * 1000))) / 100;
	t = (i - ((tth * 10000) + (th * 1000) + (h * 100))) / 10;
	d = (i - ((tth * 10000) + (th * 1000) + (h * 100) + (t * 10)));
	//putchar(tth + '0');
	//putchar(th + '0');
	putchar(h + '0');
	putchar(t + '0');
	putchar(d + '0');
}

main(int argc, char **argv)
{
	int val;
	int size = 0;
	unsigned char c;

	printf("gcc2_compiled.:\n");
	printf("__gnu_compiled_c:\n");
	printf("\t.globl linux_compressed_start\n");
	printf("\t.text\n");
	printf("\t.align 2\n");
	printf("\t.type linux_compressed_start,@object\n");
	printf("linux_compressed_start:\n");


	val = getchar();
	while (val != EOF) {
		size++;
		c = (unsigned char) (val & 0x00ff);
		printf("\t.byte  ");
		printval((int) c);
		printf("\n");
		val = getchar();
	}
	printf("\t.size linux_compressed_start,%d\n", size);
	printf("\t.globl linux_compressed_size\n");
	printf("\t.text\n");
	printf("\t.align 2\n");
	printf("\t.type linux_compressed_size,@object\n");
	printf("\t.size linux_compressed_size,4\n");
	printf("linux_compressed_size:\n");
	printf("\t.word %d\n", size);
}
