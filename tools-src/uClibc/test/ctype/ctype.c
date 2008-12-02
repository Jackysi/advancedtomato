/* vi: set sw=4 ts=4: */
/*
 * Test application for functions defined in ctype.h
 *
 * Copyright (C) 2000 by Lineo, inc. and Erik Andersen
 * Copyright (C) 2000,2001 by Erik Andersen <andersen@uclibc.org>
 * Written by Erik Andersen <andersen@uclibc.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Library General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include "../testsuite.h"

#define TRUE	1

int main( int argc, char **argv)
{
	int i, c;


    init_testsuite("Testing functions defined in ctype.h\n");

	/* isalnum() */
	{
		int buffer[]={ '1', '4', 'a', 'z', 'A', 'Z', '5', -1};
		for(i=0; buffer[i]!=-1; i++) {
			c = buffer[i];
			TEST( isalnum(c)==TRUE);
		}
	}
	{
		int buffer[]={  2, 128, 254, '\n', -1};
		for(i=0; buffer[i]!=-1; i++) {
			c = buffer[i];
			TEST( isalnum(c)!=TRUE);
		}
	}



	/* isalpha() */
	{
		int buffer[]={ 'a', 'z', 'A', 'Z', -1};
		for(i=0; buffer[i]!=-1; i++) {
			c = buffer[i];
			TEST( isalpha(c)==TRUE);
		}
	}
	{
		int buffer[]={  2, 63, 128, 254, '\n', -1};
		for(i=0; buffer[i]!=-1; i++) {
			c = buffer[i];
			TEST( isalpha(c)!=TRUE);
		}
	}



	/* isascii() */
	{
		int buffer[]={ 'a', 'z', 'A', 'Z', '\n', -1};
		for(i=0; buffer[i]!=-1; i++) {
			c = buffer[i];
			TEST( isascii(c)==TRUE);
		}
	}
	{
		int buffer[]={  128, 254, -1};
		for(i=0; buffer[i]!=-1; i++) {
			c = buffer[i];
			TEST( isascii(c)!=TRUE);
		}
	}


	/* iscntrl() */
	{
		int buffer[]={ 0x7F, 6, '\t', '\n', 0x7F, -1};
		for(i=0; buffer[i]!=-1; i++) {
			c = buffer[i];
			TEST( iscntrl(c)==TRUE);
		}
	}
	{
		int buffer[]={  63, 128, 254, -1};
		for(i=0; buffer[i]!=-1; i++) {
			c = buffer[i];
			TEST( iscntrl(c)!=TRUE);
		}
	}


	/* isdigit() */
	{
		int buffer[]={ '1', '5', '7', '9', -1};
		for(i=0; buffer[i]!=-1; i++) {
			c = buffer[i];
			TEST( isdigit(c)==TRUE);
		}
	}
	{
		int buffer[]={  2, 'a', 'z', 'A', 'Z', 63, 128, 254, '\n', -1};
		for(i=0; buffer[i]!=-1; i++) {
			c = buffer[i];
			TEST( isdigit(c)!=TRUE);
		}
	}



	/* isgraph() */
	{
		int buffer[]={ ')', '~', '9', -1};
		for(i=0; buffer[i]!=-1; i++) {
			c = buffer[i];
			TEST( isgraph(c)==TRUE);
		}
	}
	{
		int buffer[]={ 9, ' ', '\t', '\n', 200, 0x7F, -1};
		for(i=0; buffer[i]!=-1; i++) {
			c = buffer[i];
			TEST( isgraph(c)!=TRUE);
		}
	}


	/* islower() */
	{
		int buffer[]={ 'a', 'g', 'z', -1};
		for(i=0; buffer[i]!=-1; i++) {
			c = buffer[i];
			TEST( islower(c)==TRUE);
		}
	}
	{
		int buffer[]={ 9, 'A', 'Z', 128, 254, ' ', '\t', '\n', 0x7F, -1};
		for(i=0; buffer[i]!=-1; i++) {
			c = buffer[i];
			TEST( islower(c)!=TRUE);
		}
	}


	/* isprint() */
	{
		int buffer[]={ ' ', ')', '~', '9', -1};
		for(i=0; buffer[i]!=-1; i++) {
			c = buffer[i];
			TEST( isprint(c)==TRUE);
		}
	}
	{
		int buffer[]={ '\b', '\t', '\n', 9, 128, 254, 200, 0x7F, -1};
		for(i=0; buffer[i]!=-1; i++) {
			c = buffer[i];
			TEST( isprint(c)!=TRUE);
		}
	}


	/* ispunct() */
	{
		int buffer[]={ '.', '#', '@', ';', -1};
		for(i=0; buffer[i]!=-1; i++) {
			c = buffer[i];
			TEST( ispunct(c)==TRUE);
		}
	}
	{
		int buffer[]={  2, 'a', 'Z', '1', 128, 254, '\n', -1};
		for(i=0; buffer[i]!=-1; i++) {
			c = buffer[i];
			TEST( ispunct(c)!=TRUE);
		}
	}


	/* isspace() */
	{
		int buffer[]={ ' ', '\t', '\r', '\v', '\n', -1};
		for(i=0; buffer[i]!=-1; i++) {
			c = buffer[i];
			TEST( isspace(c)==TRUE);
		}
	}
	{
		int buffer[]={  2, 'a', 'Z', '1', 128, 254, -1};
		for(i=0; buffer[i]!=-1; i++) {
			c = buffer[i];
			TEST( isspace(c)!=TRUE);
		}
	}


	/* isupper() */
	{
		int buffer[]={ 'A', 'G', 'Z', -1};
		for(i=0; buffer[i]!=-1; i++) {
			c = buffer[i];
			TEST( isupper(c)==TRUE);
		}
	}
	{
		int buffer[]={  2, 'a', 'z', '1', 128, 254, -1};
		for(i=0; buffer[i]!=-1; i++) {
			c = buffer[i];
			TEST( isupper(c)!=TRUE);
		}
	}



	/* isxdigit() */
	{
		int buffer[]={ 'f', 'A', '1', '8', -1};
		for(i=0; buffer[i]!=-1; i++) {
			c = buffer[i];
			TEST( isxdigit(c)==TRUE);
		}
	}
	{
		int buffer[]={  2, 'g', 'G', 'x', '\n', -1};
		for(i=0; buffer[i]!=-1; i++) {
			c = buffer[i];
			TEST( isxdigit(c)!=TRUE);
		}
	}


	/* tolower() */
	c='A';
	TEST_NUMERIC( tolower(c), 'a');
	c='a';
	TEST_NUMERIC( tolower(c), 'a');
	c='#';
	TEST_NUMERIC( tolower(c), c);

	/* toupper() */
	c='a';
	TEST_NUMERIC( toupper(c), 'A');
	c='A';
	TEST_NUMERIC( toupper(c), 'A');
	c='#';
	TEST_NUMERIC( toupper(c), c);

	exit(0);
}
