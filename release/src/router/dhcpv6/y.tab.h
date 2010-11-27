/* A Bison parser, made by GNU Bison 2.1.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     INTERFACE = 258,
     IFNAME = 259,
     PREFIX_INTERFACE = 260,
     SLA_ID = 261,
     SLA_LEN = 262,
     DUID_ID = 263,
     ID_ASSOC = 264,
     IA_PD = 265,
     IAID = 266,
     IA_NA = 267,
     ADDRESS = 268,
     REQUEST = 269,
     SEND = 270,
     ALLOW = 271,
     PREFERENCE = 272,
     HOST = 273,
     HOSTNAME = 274,
     DUID = 275,
     OPTION = 276,
     RAPID_COMMIT = 277,
     DNS_SERVERS = 278,
     DNS_NAME = 279,
     NTP_SERVERS = 280,
     REFRESHTIME = 281,
     SIP_SERVERS = 282,
     SIP_NAME = 283,
     NIS_SERVERS = 284,
     NIS_NAME = 285,
     NISP_SERVERS = 286,
     NISP_NAME = 287,
     BCMCS_SERVERS = 288,
     BCMCS_NAME = 289,
     INFO_ONLY = 290,
     SCRIPT = 291,
     DELAYEDKEY = 292,
     AUTHENTICATION = 293,
     PROTOCOL = 294,
     ALGORITHM = 295,
     DELAYED = 296,
     RECONFIG = 297,
     HMACMD5 = 298,
     MONOCOUNTER = 299,
     AUTHNAME = 300,
     RDM = 301,
     KEY = 302,
     KEYINFO = 303,
     REALM = 304,
     KEYID = 305,
     SECRET = 306,
     KEYNAME = 307,
     EXPIRE = 308,
     ADDRPOOL = 309,
     POOLNAME = 310,
     RANGE = 311,
     TO = 312,
     ADDRESS_POOL = 313,
     INCLUDE = 314,
     NUMBER = 315,
     SLASH = 316,
     EOS = 317,
     BCL = 318,
     ECL = 319,
     STRING = 320,
     QSTRING = 321,
     PREFIX = 322,
     INFINITY = 323,
     COMMA = 324
   };
#endif
/* Tokens.  */
#define INTERFACE 258
#define IFNAME 259
#define PREFIX_INTERFACE 260
#define SLA_ID 261
#define SLA_LEN 262
#define DUID_ID 263
#define ID_ASSOC 264
#define IA_PD 265
#define IAID 266
#define IA_NA 267
#define ADDRESS 268
#define REQUEST 269
#define SEND 270
#define ALLOW 271
#define PREFERENCE 272
#define HOST 273
#define HOSTNAME 274
#define DUID 275
#define OPTION 276
#define RAPID_COMMIT 277
#define DNS_SERVERS 278
#define DNS_NAME 279
#define NTP_SERVERS 280
#define REFRESHTIME 281
#define SIP_SERVERS 282
#define SIP_NAME 283
#define NIS_SERVERS 284
#define NIS_NAME 285
#define NISP_SERVERS 286
#define NISP_NAME 287
#define BCMCS_SERVERS 288
#define BCMCS_NAME 289
#define INFO_ONLY 290
#define SCRIPT 291
#define DELAYEDKEY 292
#define AUTHENTICATION 293
#define PROTOCOL 294
#define ALGORITHM 295
#define DELAYED 296
#define RECONFIG 297
#define HMACMD5 298
#define MONOCOUNTER 299
#define AUTHNAME 300
#define RDM 301
#define KEY 302
#define KEYINFO 303
#define REALM 304
#define KEYID 305
#define SECRET 306
#define KEYNAME 307
#define EXPIRE 308
#define ADDRPOOL 309
#define POOLNAME 310
#define RANGE 311
#define TO 312
#define ADDRESS_POOL 313
#define INCLUDE 314
#define NUMBER 315
#define SLASH 316
#define EOS 317
#define BCL 318
#define ECL 319
#define STRING 320
#define QSTRING 321
#define PREFIX 322
#define INFINITY 323
#define COMMA 324




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 126 "cfparse.y"
typedef union YYSTYPE {
	long long num;
	char* str;
	struct cf_list *list;
	struct dhcp6_prefix *prefix;
	struct dhcp6_range *range;
	struct dhcp6_poolspec *pool;
} YYSTYPE;
/* Line 1447 of yacc.c.  */
#line 185 "y.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



