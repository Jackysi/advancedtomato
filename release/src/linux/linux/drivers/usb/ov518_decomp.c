/* OV518 Decompression Support Module (No-MMX version)
 *
 * Copyright (c) 2002-2003 Mark W. McClelland. All rights reserved.
 * http://alpha.dyndns.org/ov511/
 *
 * Fast integer iDCT by Yuri van Oers <yvanoers AT xs4all.nl>
 * Original OV511 decompression code Copyright 1998-2000 OmniVision Technologies
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 */

#include <linux/config.h>

#if defined(OUTSIDE_KERNEL)
	#if defined(CONFIG_MODVERSIONS) && !defined(MODVERSIONS)
		#define MODVERSIONS
	#endif

	#include <linux/version.h>

	#ifdef MODVERSIONS
		#include <linux/modversions.h>
	#endif
#else
	#include <linux/version.h>
#endif

#include <linux/module.h>
#include <linux/init.h>

#include "ov51x.h"

/******************************************************************************
 * Compile-time Options
 ******************************************************************************/

/* Defining APPROXIMATE_MUL_BY_SHIFT increases performance by approximation
 * the multiplications by shifts. I think there's no change in the
 * calculated picture, but I'm not sure, so the choice is still in here. */
#undef APPROXIMATE_MUL_BY_SHIFT

/* Allows printing the dynamic quantization tables (only if debug >= 5) */
#define PRINT_QT

/******************************************************************************
 * Version Information
 ******************************************************************************/

#define DRIVER_VERSION "v1.3"
#define DRIVER_AUTHOR "Mark McClelland <mark@alpha.dyndns.org>, \
Yuri van Oers <yvanoers AT xs4all.nl>, OmniVision Technologies \
<http://www.ovt.com/>"
#define DRIVER_DESC "OV518 Decompression Module"

/******************************************************************************
 * Decompression Module Interface Constants
 ******************************************************************************/

static const int interface_ver = DECOMP_INTERFACE_VER;
static const int ov518 = 1;
static const int mmx = 0;

/******************************************************************************
 * Module Features
 ******************************************************************************/

static int debug = 0;

/* Static quantization. This uses a fixed quantization table versus the one
 * that is normally embedded in the data. Define this if you see very bad
 * contrast or "blockiness" in the decompressed output. */
static int staticquant = 0;

MODULE_PARM(debug, "i");
MODULE_PARM_DESC(debug, 
  "Debug level: 0=none, 1=inits, 2=warning, 3=config, 4=functions, 5=max");

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
#if defined(MODULE_LICENSE)	/* Introduced in ~2.4.10 */
MODULE_LICENSE("GPL");
#endif

/******************************************************************************
 * Prototypes
 ******************************************************************************/

extern int ov511_register_decomp_module(int ver, struct ov51x_decomp_ops *ops,
					int ov518, int mmx);
extern void ov511_deregister_decomp_module(int ov518, int mmx);

/******************************************************************************
 * Local Data Types
 ******************************************************************************/

/* Make sure this remains naturally aligned and 2^n bytes in size */
struct tree_node {
	short left;		/* Pointer to left child node */
	short right;		/* Pointer to right child node */
	signed char depth;	/* Depth (starting at 1) if leaf, else -1 */
	signed char coeffbits;	/* Size of coefficient data, or zero if none */
	signed char skip;	/* Number of zero coefficients. Unused w/ DC */
	char padding;		/* Pad out to 8 bytes */
};

struct comp_info {
	int bytes;		/* Number of processed input bytes */
	int bits;		/* Number of unprocessed input bits */
	int rawLen;		/* Total number of bytes in input buffer */
	unsigned char *qt;	/* Current quantization table */
};

/******************************************************************************
 * Constant Data Definitions
 ******************************************************************************/

/* Zig-Zag Table */
static const unsigned char ZigZag518[] = {
	0x00, 0x02, 0x03, 0x09,
	0x01, 0x04, 0x08, 0x0a,
	0x05, 0x07, 0x0b, 0x11,
	0x06, 0x0c, 0x10, 0x12,
	0x0d, 0x0f, 0x13, 0x19,
	0x0e, 0x14, 0x18, 0x1a,
	0x15, 0x17, 0x1b, 0x1e,
	0x16, 0x1c, 0x1d, 0x1f
};

/* Huffman trees */

static const struct tree_node treeYAC[] = {
	{  1,   4, -1,  0, -1}, {  2,   3, -1,  0, -1},
	{ -1,  -1,  2,  1,  0}, { -1,  -1,  2,  2,  0},
	{  5,   9, -1,  0, -1}, {  6,   7, -1,  0, -1},
	{ -1,  -1,  3,  3,  0}, {323,   8, -1,  0, -1},
	{ -1,  -1,  4,  4,  0}, { 10,  13, -1,  0, -1},
	{ 38,  11, -1,  0, -1}, { 12,  39, -1,  0, -1},
	{ -1,  -1,  5,  5,  0}, { 59,  14, -1,  0, -1},
	{ 15,  18, -1,  0, -1}, { 16, 113, -1,  0, -1},
	{ 17,  40, -1,  0, -1}, { -1,  -1,  7,  6,  0},
	{ 19,  22, -1,  0, -1}, { 20,  41, -1,  0, -1},
	{ 21,  61, -1,  0, -1}, { -1,  -1,  8,  7,  0},
	{ 23,  27, -1,  0, -1}, {169,  24, -1,  0, -1},
	{208,  25, -1,  0, -1}, { 26,  62, -1,  0, -1},
	{ -1,  -1, 10,  8,  0}, { 44,  28, -1,  0, -1},
	{ 63,  29, -1,  0, -1}, { 30, 191, -1,  0, -1},
	{ 31, 119, -1,  0, -1}, { 32,  82, -1,  0, -1},
	{ 33,  55, -1,  0, -1}, { 34,  48, -1,  0, -1},
	{171,  35, -1,  0, -1}, { 36,  37, -1,  0, -1},
	{ -1,  -1, 16,  9,  0}, { -1,  -1, 16, 10,  0},
	{ -1,  -1,  4,  1,  1}, { -1,  -1,  5,  2,  1},
	{ -1,  -1,  7,  3,  1}, {151,  42, -1,  0, -1},
	{ 43,  79, -1,  0, -1}, { -1,  -1,  9,  4,  1},
	{ 96,  45, -1,  0, -1}, {246,  46, -1,  0, -1},
	{ 47, 115, -1,  0, -1}, { -1,  -1, 11,  5,  1},
	{ 49,  52, -1,  0, -1}, { 50,  51, -1,  0, -1},
	{ -1,  -1, 16,  6,  1}, { -1,  -1, 16,  7,  1},
	{ 53,  54, -1,  0, -1}, { -1,  -1, 16,  8,  1},
	{ -1,  -1, 16,  9,  1}, { 56,  71, -1,  0, -1},
	{ 57,  68, -1,  0, -1}, { 58,  67, -1,  0, -1},
	{ -1,  -1, 16, 10,  1}, { 60,  77, -1,  0, -1},
	{ -1,  -1,  5,  1,  2}, { -1,  -1,  8,  2,  2},
	{ -1,  -1, 10,  3,  2}, {265,  64, -1,  0, -1},
	{ 65, 134, -1,  0, -1}, { 66,  80, -1,  0, -1},
	{ -1,  -1, 12,  4,  2}, { -1,  -1, 16,  5,  2},
	{ 69,  70, -1,  0, -1}, { -1,  -1, 16,  6,  2},
	{ -1,  -1, 16,  7,  2}, { 72,  75, -1,  0, -1},
	{ 73,  74, -1,  0, -1}, { -1,  -1, 16,  8,  2},
	{ -1,  -1, 16,  9,  2}, { 76,  81, -1,  0, -1},
	{ -1,  -1, 16, 10,  2}, { 78,  95, -1,  0, -1},
	{ -1,  -1,  6,  1,  3}, { -1,  -1,  9,  2,  3},
	{ -1,  -1, 12,  3,  3}, { -1,  -1, 16,  4,  3},
	{ 83, 101, -1,  0, -1}, { 84,  91, -1,  0, -1},
	{ 85,  88, -1,  0, -1}, { 86,  87, -1,  0, -1},
	{ -1,  -1, 16,  5,  3}, { -1,  -1, 16,  6,  3},
	{ 89,  90, -1,  0, -1}, { -1,  -1, 16,  7,  3},
	{ -1,  -1, 16,  8,  3}, { 92,  98, -1,  0, -1},
	{ 93,  94, -1,  0, -1}, { -1,  -1, 16,  9,  3},
	{ -1,  -1, 16, 10,  3}, { -1,  -1,  6,  1,  4},
	{ 97, 225, -1,  0, -1}, { -1,  -1, 10,  2,  4},
	{ 99, 100, -1,  0, -1}, { -1,  -1, 16,  3,  4},
	{ -1,  -1, 16,  4,  4}, {102, 109, -1,  0, -1},
	{103, 106, -1,  0, -1}, {104, 105, -1,  0, -1},
	{ -1,  -1, 16,  5,  4}, { -1,  -1, 16,  6,  4},
	{107, 108, -1,  0, -1}, { -1,  -1, 16,  7,  4},
	{ -1,  -1, 16,  8,  4}, {110, 116, -1,  0, -1},
	{111, 112, -1,  0, -1}, { -1,  -1, 16,  9,  4},
	{ -1,  -1, 16, 10,  4}, {114, 133, -1,  0, -1},
	{ -1,  -1,  7,  1,  5}, { -1,  -1, 11,  2,  5},
	{117, 118, -1,  0, -1}, { -1,  -1, 16,  3,  5},
	{ -1,  -1, 16,  4,  5}, {120, 156, -1,  0, -1},
	{121, 139, -1,  0, -1}, {122, 129, -1,  0, -1},
	{123, 126, -1,  0, -1}, {124, 125, -1,  0, -1},
	{ -1,  -1, 16,  5,  5}, { -1,  -1, 16,  6,  5},
	{127, 128, -1,  0, -1}, { -1,  -1, 16,  7,  5},
	{ -1,  -1, 16,  8,  5}, {130, 136, -1,  0, -1},
	{131, 132, -1,  0, -1}, { -1,  -1, 16,  9,  5},
	{ -1,  -1, 16, 10,  5}, { -1,  -1,  7,  1,  6},
	{135, 152, -1,  0, -1}, { -1,  -1, 12,  2,  6},
	{137, 138, -1,  0, -1}, { -1,  -1, 16,  3,  6},
	{ -1,  -1, 16,  4,  6}, {140, 147, -1,  0, -1},
	{141, 144, -1,  0, -1}, {142, 143, -1,  0, -1},
	{ -1,  -1, 16,  5,  6}, { -1,  -1, 16,  6,  6},
	{145, 146, -1,  0, -1}, { -1,  -1, 16,  7,  6},
	{ -1,  -1, 16,  8,  6}, {148, 153, -1,  0, -1},
	{149, 150, -1,  0, -1}, { -1,  -1, 16,  9,  6},
	{ -1,  -1, 16, 10,  6}, { -1,  -1,  8,  1,  7},
	{ -1,  -1, 12,  2,  7}, {154, 155, -1,  0, -1},
	{ -1,  -1, 16,  3,  7}, { -1,  -1, 16,  4,  7},
	{157, 175, -1,  0, -1}, {158, 165, -1,  0, -1},
	{159, 162, -1,  0, -1}, {160, 161, -1,  0, -1},
	{ -1,  -1, 16,  5,  7}, { -1,  -1, 16,  6,  7},
	{163, 164, -1,  0, -1}, { -1,  -1, 16,  7,  7},
	{ -1,  -1, 16,  8,  7}, {166, 172, -1,  0, -1},
	{167, 168, -1,  0, -1}, { -1,  -1, 16,  9,  7},
	{ -1,  -1, 16, 10,  7}, {170, 187, -1,  0, -1},
	{ -1,  -1,  9,  1,  8}, { -1,  -1, 15,  2,  8},
	{173, 174, -1,  0, -1}, { -1,  -1, 16,  3,  8},
	{ -1,  -1, 16,  4,  8}, {176, 183, -1,  0, -1},
	{177, 180, -1,  0, -1}, {178, 179, -1,  0, -1},
	{ -1,  -1, 16,  5,  8}, { -1,  -1, 16,  6,  8},
	{181, 182, -1,  0, -1}, { -1,  -1, 16,  7,  8},
	{ -1,  -1, 16,  8,  8}, {184, 188, -1,  0, -1},
	{185, 186, -1,  0, -1}, { -1,  -1, 16,  9,  8},
	{ -1,  -1, 16, 10,  8}, { -1,  -1,  9,  1,  9},
	{189, 190, -1,  0, -1}, { -1,  -1, 16,  2,  9},
	{ -1,  -1, 16,  3,  9}, {192, 258, -1,  0, -1},
	{193, 226, -1,  0, -1}, {194, 210, -1,  0, -1},
	{195, 202, -1,  0, -1}, {196, 199, -1,  0, -1},
	{197, 198, -1,  0, -1}, { -1,  -1, 16,  4,  9},
	{ -1,  -1, 16,  5,  9}, {200, 201, -1,  0, -1},
	{ -1,  -1, 16,  6,  9}, { -1,  -1, 16,  7,  9},
	{203, 206, -1,  0, -1}, {204, 205, -1,  0, -1},
	{ -1,  -1, 16,  8,  9}, { -1,  -1, 16,  9,  9},
	{207, 209, -1,  0, -1}, { -1,  -1, 16, 10,  9},
	{ -1,  -1,  9,  1, 10}, { -1,  -1, 16,  2, 10},
	{211, 218, -1,  0, -1}, {212, 215, -1,  0, -1},
	{213, 214, -1,  0, -1}, { -1,  -1, 16,  3, 10},
	{ -1,  -1, 16,  4, 10}, {216, 217, -1,  0, -1},
	{ -1,  -1, 16,  5, 10}, { -1,  -1, 16,  6, 10},
	{219, 222, -1,  0, -1}, {220, 221, -1,  0, -1},
	{ -1,  -1, 16,  7, 10}, { -1,  -1, 16,  8, 10},
	{223, 224, -1,  0, -1}, { -1,  -1, 16,  9, 10},
	{ -1,  -1, 16, 10, 10}, { -1,  -1, 10,  1, 11},
	{227, 242, -1,  0, -1}, {228, 235, -1,  0, -1},
	{229, 232, -1,  0, -1}, {230, 231, -1,  0, -1},
	{ -1,  -1, 16,  2, 11}, { -1,  -1, 16,  3, 11},
	{233, 234, -1,  0, -1}, { -1,  -1, 16,  4, 11},
	{ -1,  -1, 16,  5, 11}, {236, 239, -1,  0, -1},
	{237, 238, -1,  0, -1}, { -1,  -1, 16,  6, 11},
	{ -1,  -1, 16,  7, 11}, {240, 241, -1,  0, -1},
	{ -1,  -1, 16,  8, 11}, { -1,  -1, 16,  9, 11},
	{243, 251, -1,  0, -1}, {244, 248, -1,  0, -1},
	{245, 247, -1,  0, -1}, { -1,  -1, 16, 10, 11},
	{ -1,  -1, 10,  1, 12}, { -1,  -1, 16,  2, 12},
	{249, 250, -1,  0, -1}, { -1,  -1, 16,  3, 12},
	{ -1,  -1, 16,  4, 12}, {252, 255, -1,  0, -1},
	{253, 254, -1,  0, -1}, { -1,  -1, 16,  5, 12},
	{ -1,  -1, 16,  6, 12}, {256, 257, -1,  0, -1},
	{ -1,  -1, 16,  7, 12}, { -1,  -1, 16,  8, 12},
	{259, 292, -1,  0, -1}, {260, 277, -1,  0, -1},
	{261, 270, -1,  0, -1}, {262, 267, -1,  0, -1},
	{263, 264, -1,  0, -1}, { -1,  -1, 16,  9, 12},
	{ -1,  -1, 16, 10, 12}, {266, 322, -1,  0, -1},
	{ -1,  -1, 11,  1, 13}, {268, 269, -1,  0, -1},
	{ -1,  -1, 16,  2, 13}, { -1,  -1, 16,  3, 13},
	{271, 274, -1,  0, -1}, {272, 273, -1,  0, -1},
	{ -1,  -1, 16,  4, 13}, { -1,  -1, 16,  5, 13},
	{275, 276, -1,  0, -1}, { -1,  -1, 16,  6, 13},
	{ -1,  -1, 16,  7, 13}, {278, 285, -1,  0, -1},
	{279, 282, -1,  0, -1}, {280, 281, -1,  0, -1},
	{ -1,  -1, 16,  8, 13}, { -1,  -1, 16,  9, 13},
	{283, 284, -1,  0, -1}, { -1,  -1, 16, 10, 13},
	{ -1,  -1, 16,  1, 14}, {286, 289, -1,  0, -1},
	{287, 288, -1,  0, -1}, { -1,  -1, 16,  2, 14},
	{ -1,  -1, 16,  3, 14}, {290, 291, -1,  0, -1},
	{ -1,  -1, 16,  4, 14}, { -1,  -1, 16,  5, 14},
	{293, 308, -1,  0, -1}, {294, 301, -1,  0, -1},
	{295, 298, -1,  0, -1}, {296, 297, -1,  0, -1},
	{ -1,  -1, 16,  6, 14}, { -1,  -1, 16,  7, 14},
	{299, 300, -1,  0, -1}, { -1,  -1, 16,  8, 14},
	{ -1,  -1, 16,  9, 14}, {302, 305, -1,  0, -1},
	{303, 304, -1,  0, -1}, { -1,  -1, 16, 10, 14},
	{ -1,  -1, 16,  1, 15}, {306, 307, -1,  0, -1},
	{ -1,  -1, 16,  2, 15}, { -1,  -1, 16,  3, 15},
	{309, 316, -1,  0, -1}, {310, 313, -1,  0, -1},
	{311, 312, -1,  0, -1}, { -1,  -1, 16,  4, 15},
	{ -1,  -1, 16,  5, 15}, {314, 315, -1,  0, -1},
	{ -1,  -1, 16,  6, 15}, { -1,  -1, 16,  7, 15},
	{317, 320, -1,  0, -1}, {318, 319, -1,  0, -1},
	{ -1,  -1, 16,  8, 15}, { -1,  -1, 16,  9, 15},
	{321,  -1, -1,  0, -1}, { -1,  -1, 16, 10, 15},
	{ -1,  -1, 11,  0, 16}, { -1,  -1,  4,  0, -1},
};

static const struct tree_node treeUVAC[] = {
	{  1,   3, -1,  0, -1}, {323,   2, -1,  0, -1},
	{ -1,  -1,  2,  1,  0}, {  4,   8, -1,  0, -1},
	{  5,   6, -1,  0, -1}, { -1,  -1,  3,  2,  0},
	{  7,  37, -1,  0, -1}, { -1,  -1,  4,  3,  0},
	{  9,  13, -1,  0, -1}, { 10,  60, -1,  0, -1},
	{ 11,  12, -1,  0, -1}, { -1,  -1,  5,  4,  0},
	{ -1,  -1,  5,  5,  0}, { 14,  17, -1,  0, -1},
	{ 15,  97, -1,  0, -1}, { 16,  38, -1,  0, -1},
	{ -1,  -1,  6,  6,  0}, { 18,  21, -1,  0, -1},
	{ 19,  39, -1,  0, -1}, { 20, 135, -1,  0, -1},
	{ -1,  -1,  7,  7,  0}, { 22,  26, -1,  0, -1},
	{ 82,  23, -1,  0, -1}, { 24,  99, -1,  0, -1},
	{ 25,  42, -1,  0, -1}, { -1,  -1,  9,  8,  0},
	{ 27,  31, -1,  0, -1}, {211,  28, -1,  0, -1},
	{248,  29, -1,  0, -1}, { 30,  63, -1,  0, -1},
	{ -1,  -1, 10,  9,  0}, { 43,  32, -1,  0, -1},
	{ 33,  48, -1,  0, -1}, {153,  34, -1,  0, -1},
	{ 35,  64, -1,  0, -1}, { 36,  47, -1,  0, -1},
	{ -1,  -1, 12, 10,  0}, { -1,  -1,  4,  1,  1},
	{ -1,  -1,  6,  2,  1}, {152,  40, -1,  0, -1},
	{ 41,  62, -1,  0, -1}, { -1,  -1,  8,  3,  1},
	{ -1,  -1,  9,  4,  1}, { 84,  44, -1,  0, -1},
	{322,  45, -1,  0, -1}, { 46, 136, -1,  0, -1},
	{ -1,  -1, 11,  5,  1}, { -1,  -1, 12,  6,  1},
	{ 49, 189, -1,  0, -1}, { 50, 119, -1,  0, -1},
	{ 51,  76, -1,  0, -1}, { 66,  52, -1,  0, -1},
	{ 53,  69, -1,  0, -1}, { 54,  57, -1,  0, -1},
	{ 55,  56, -1,  0, -1}, { -1,  -1, 16,  7,  1},
	{ -1,  -1, 16,  8,  1}, { 58,  59, -1,  0, -1},
	{ -1,  -1, 16,  9,  1}, { -1,  -1, 16, 10,  1},
	{ 61,  81, -1,  0, -1}, { -1,  -1,  5,  1,  2},
	{ -1,  -1,  8,  2,  2}, { -1,  -1, 10,  3,  2},
	{ 65,  86, -1,  0, -1}, { -1,  -1, 12,  4,  2},
	{286,  67, -1,  0, -1}, { 68, 304, -1,  0, -1},
	{ -1,  -1, 15,  5,  2}, { 70,  73, -1,  0, -1},
	{ 71,  72, -1,  0, -1}, { -1,  -1, 16,  6,  2},
	{ -1,  -1, 16,  7,  2}, { 74,  75, -1,  0, -1},
	{ -1,  -1, 16,  8,  2}, { -1,  -1, 16,  9,  2},
	{ 77, 102, -1,  0, -1}, { 78,  91, -1,  0, -1},
	{ 79,  88, -1,  0, -1}, { 80,  87, -1,  0, -1},
	{ -1,  -1, 16, 10,  2}, { -1,  -1,  5,  1,  3},
	{ 83, 171, -1,  0, -1}, { -1,  -1,  8,  2,  3},
	{ 85, 117, -1,  0, -1}, { -1,  -1, 10,  3,  3},
	{ -1,  -1, 12,  4,  3}, { -1,  -1, 16,  5,  3},
	{ 89,  90, -1,  0, -1}, { -1,  -1, 16,  6,  3},
	{ -1,  -1, 16,  7,  3}, { 92,  95, -1,  0, -1},
	{ 93,  94, -1,  0, -1}, { -1,  -1, 16,  8,  3},
	{ -1,  -1, 16,  9,  3}, { 96, 101, -1,  0, -1},
	{ -1,  -1, 16, 10,  3}, { 98, 116, -1,  0, -1},
	{ -1,  -1,  6,  1,  4}, {100, 188, -1,  0, -1},
	{ -1,  -1,  9,  2,  4}, { -1,  -1, 16,  3,  4},
	{103, 110, -1,  0, -1}, {104, 107, -1,  0, -1},
	{105, 106, -1,  0, -1}, { -1,  -1, 16,  4,  4},
	{ -1,  -1, 16,  5,  4}, {108, 109, -1,  0, -1},
	{ -1,  -1, 16,  6,  4}, { -1,  -1, 16,  7,  4},
	{111, 114, -1,  0, -1}, {112, 113, -1,  0, -1},
	{ -1,  -1, 16,  8,  4}, { -1,  -1, 16,  9,  4},
	{115, 118, -1,  0, -1}, { -1,  -1, 16, 10,  4},
	{ -1,  -1,  6,  1,  5}, { -1,  -1, 10,  2,  5},
	{ -1,  -1, 16,  3,  5}, {120, 156, -1,  0, -1},
	{121, 138, -1,  0, -1}, {122, 129, -1,  0, -1},
	{123, 126, -1,  0, -1}, {124, 125, -1,  0, -1},
	{ -1,  -1, 16,  4,  5}, { -1,  -1, 16,  5,  5},
	{127, 128, -1,  0, -1}, { -1,  -1, 16,  6,  5},
	{ -1,  -1, 16,  7,  5}, {130, 133, -1,  0, -1},
	{131, 132, -1,  0, -1}, { -1,  -1, 16,  8,  5},
	{ -1,  -1, 16,  9,  5}, {134, 137, -1,  0, -1},
	{ -1,  -1, 16, 10,  5}, { -1,  -1,  7,  1,  6},
	{ -1,  -1, 11,  2,  6}, { -1,  -1, 16,  3,  6},
	{139, 146, -1,  0, -1}, {140, 143, -1,  0, -1},
	{141, 142, -1,  0, -1}, { -1,  -1, 16,  4,  6},
	{ -1,  -1, 16,  5,  6}, {144, 145, -1,  0, -1},
	{ -1,  -1, 16,  6,  6}, { -1,  -1, 16,  7,  6},
	{147, 150, -1,  0, -1}, {148, 149, -1,  0, -1},
	{ -1,  -1, 16,  8,  6}, { -1,  -1, 16,  9,  6},
	{151, 155, -1,  0, -1}, { -1,  -1, 16, 10,  6},
	{ -1,  -1,  7,  1,  7}, {154, 267, -1,  0, -1},
	{ -1,  -1, 11,  2,  7}, { -1,  -1, 16,  3,  7},
	{157, 173, -1,  0, -1}, {158, 165, -1,  0, -1},
	{159, 162, -1,  0, -1}, {160, 161, -1,  0, -1},
	{ -1,  -1, 16,  4,  7}, { -1,  -1, 16,  5,  7},
	{163, 164, -1,  0, -1}, { -1,  -1, 16,  6,  7},
	{ -1,  -1, 16,  7,  7}, {166, 169, -1,  0, -1},
	{167, 168, -1,  0, -1}, { -1,  -1, 16,  8,  7},
	{ -1,  -1, 16,  9,  7}, {170, 172, -1,  0, -1},
	{ -1,  -1, 16, 10,  7}, { -1,  -1,  8,  1,  8},
	{ -1,  -1, 16,  2,  8}, {174, 181, -1,  0, -1},
	{175, 178, -1,  0, -1}, {176, 177, -1,  0, -1},
	{ -1,  -1, 16,  3,  8}, { -1,  -1, 16,  4,  8},
	{179, 180, -1,  0, -1}, { -1,  -1, 16,  5,  8},
	{ -1,  -1, 16,  6,  8}, {182, 185, -1,  0, -1},
	{183, 184, -1,  0, -1}, { -1,  -1, 16,  7,  8},
	{ -1,  -1, 16,  8,  8}, {186, 187, -1,  0, -1},
	{ -1,  -1, 16,  9,  8}, { -1,  -1, 16, 10,  8},
	{ -1,  -1,  9,  1,  9}, {190, 257, -1,  0, -1},
	{191, 224, -1,  0, -1}, {192, 207, -1,  0, -1},
	{193, 200, -1,  0, -1}, {194, 197, -1,  0, -1},
	{195, 196, -1,  0, -1}, { -1,  -1, 16,  2,  9},
	{ -1,  -1, 16,  3,  9}, {198, 199, -1,  0, -1},
	{ -1,  -1, 16,  4,  9}, { -1,  -1, 16,  5,  9},
	{201, 204, -1,  0, -1}, {202, 203, -1,  0, -1},
	{ -1,  -1, 16,  6,  9}, { -1,  -1, 16,  7,  9},
	{205, 206, -1,  0, -1}, { -1,  -1, 16,  8,  9},
	{ -1,  -1, 16,  9,  9}, {208, 217, -1,  0, -1},
	{209, 214, -1,  0, -1}, {210, 213, -1,  0, -1},
	{ -1,  -1, 16, 10,  9}, {212, 230, -1,  0, -1},
	{ -1,  -1,  9,  1, 10}, { -1,  -1, 16,  2, 10},
	{215, 216, -1,  0, -1}, { -1,  -1, 16,  3, 10},
	{ -1,  -1, 16,  4, 10}, {218, 221, -1,  0, -1},
	{219, 220, -1,  0, -1}, { -1,  -1, 16,  5, 10},
	{ -1,  -1, 16,  6, 10}, {222, 223, -1,  0, -1},
	{ -1,  -1, 16,  7, 10}, { -1,  -1, 16,  8, 10},
	{225, 241, -1,  0, -1}, {226, 234, -1,  0, -1},
	{227, 231, -1,  0, -1}, {228, 229, -1,  0, -1},
	{ -1,  -1, 16,  9, 10}, { -1,  -1, 16, 10, 10},
	{ -1,  -1,  9,  1, 11}, {232, 233, -1,  0, -1},
	{ -1,  -1, 16,  2, 11}, { -1,  -1, 16,  3, 11},
	{235, 238, -1,  0, -1}, {236, 237, -1,  0, -1},
	{ -1,  -1, 16,  4, 11}, { -1,  -1, 16,  5, 11},
	{239, 240, -1,  0, -1}, { -1,  -1, 16,  6, 11},
	{ -1,  -1, 16,  7, 11}, {242, 250, -1,  0, -1},
	{243, 246, -1,  0, -1}, {244, 245, -1,  0, -1},
	{ -1,  -1, 16,  8, 11}, { -1,  -1, 16,  9, 11},
	{247, 249, -1,  0, -1}, { -1,  -1, 16, 10, 11},
	{ -1,  -1,  9,  1, 12}, { -1,  -1, 16,  2, 12},
	{251, 254, -1,  0, -1}, {252, 253, -1,  0, -1},
	{ -1,  -1, 16,  3, 12}, { -1,  -1, 16,  4, 12},
	{255, 256, -1,  0, -1}, { -1,  -1, 16,  5, 12},
	{ -1,  -1, 16,  6, 12}, {258, 291, -1,  0, -1},
	{259, 275, -1,  0, -1}, {260, 268, -1,  0, -1},
	{261, 264, -1,  0, -1}, {262, 263, -1,  0, -1},
	{ -1,  -1, 16,  7, 12}, { -1,  -1, 16,  8, 12},
	{265, 266, -1,  0, -1}, { -1,  -1, 16,  9, 12},
	{ -1,  -1, 16, 10, 12}, { -1,  -1, 11,  1, 13},
	{269, 272, -1,  0, -1}, {270, 271, -1,  0, -1},
	{ -1,  -1, 16,  2, 13}, { -1,  -1, 16,  3, 13},
	{273, 274, -1,  0, -1}, { -1,  -1, 16,  4, 13},
	{ -1,  -1, 16,  5, 13}, {276, 283, -1,  0, -1},
	{277, 280, -1,  0, -1}, {278, 279, -1,  0, -1},
	{ -1,  -1, 16,  6, 13}, { -1,  -1, 16,  7, 13},
	{281, 282, -1,  0, -1}, { -1,  -1, 16,  8, 13},
	{ -1,  -1, 16,  9, 13}, {284, 288, -1,  0, -1},
	{285, 287, -1,  0, -1}, { -1,  -1, 16, 10, 13},
	{ -1,  -1, 14,  1, 14}, { -1,  -1, 16,  2, 14},
	{289, 290, -1,  0, -1}, { -1,  -1, 16,  3, 14},
	{ -1,  -1, 16,  4, 14}, {292, 308, -1,  0, -1},
	{293, 300, -1,  0, -1}, {294, 297, -1,  0, -1},
	{295, 296, -1,  0, -1}, { -1,  -1, 16,  5, 14},
	{ -1,  -1, 16,  6, 14}, {298, 299, -1,  0, -1},
	{ -1,  -1, 16,  7, 14}, { -1,  -1, 16,  8, 14},
	{301, 305, -1,  0, -1}, {302, 303, -1,  0, -1},
	{ -1,  -1, 16,  9, 14}, { -1,  -1, 16, 10, 14},
	{ -1,  -1, 15,  1, 15}, {306, 307, -1,  0, -1},
	{ -1,  -1, 16,  2, 15}, { -1,  -1, 16,  3, 15},
	{309, 316, -1,  0, -1}, {310, 313, -1,  0, -1},
	{311, 312, -1,  0, -1}, { -1,  -1, 16,  4, 15},
	{ -1,  -1, 16,  5, 15}, {314, 315, -1,  0, -1},
	{ -1,  -1, 16,  6, 15}, { -1,  -1, 16,  7, 15},
	{317, 320, -1,  0, -1}, {318, 319, -1,  0, -1},
	{ -1,  -1, 16,  8, 15}, { -1,  -1, 16,  9, 15},
	{321,  -1, -1,  0, -1}, { -1,  -1, 16, 10, 15},
	{ -1,  -1, 10,  0, 16}, { -1,  -1,  2,  0, -1},
};

static const struct tree_node treeYDC[] = {
	{  1,   6, -1,  0}, {  2,   3, -1,  0},
	{ -1,  -1,  2,  0}, {  4,   5, -1,  0},
	{ -1,  -1,  3,  1}, { -1,  -1,  3,  2},
	{  7,  10, -1,  0}, {  8,   9, -1,  0},
	{ -1,  -1,  3,  3}, { -1,  -1,  3,  4},
	{ 11,  12, -1,  0}, { -1,  -1,  3,  5},
	{ 13,  14, -1,  0}, { -1,  -1,  4,  6},
	{ 15,  16, -1,  0}, { -1,  -1,  5,  7},
	{ 17,  18, -1,  0}, { -1,  -1,  6,  8},
	{ 19,  20, -1,  0}, { -1,  -1,  7,  9},
	{ 21,  22, -1,  0}, { -1,  -1,  8, 10},
	{ 23,  -1, -1,  0}, { -1,  -1,  9, 11},
};

static const struct tree_node treeUVDC[] = {
	{  1,   4, -1,  0}, {  2,   3, -1,  0},
	{ -1,  -1,  2,  0}, { -1,  -1,  2,  1},
	{  5,   6, -1,  0}, { -1,  -1,  2,  2},
	{  7,   8, -1,  0}, { -1,  -1,  3,  3},
	{  9,  10, -1,  0}, { -1,  -1,  4,  4},
	{ 11,  12, -1,  0}, { -1,  -1,  5,  5},
	{ 13,  14, -1,  0}, { -1,  -1,  6,  6},
	{ 15,  16, -1,  0}, { -1,  -1,  7,  7},
	{ 17,  18, -1,  0}, { -1,  -1,  8,  8},
	{ 19,  20, -1,  0}, { -1,  -1,  9,  9},
	{ 21,  22, -1,  0}, { -1,  -1, 10, 10},
	{ 23,  -1, -1,  0}, { -1,  -1, 11, 11},
};

/******************************************************************************
 * Debugging
 ******************************************************************************/

#ifdef PRINT_QT
#define PRN_QT_ROW(a, i) PDEBUG(5, "%02x %02x %02x %02x %02x %02x %02x %02x", \
	(a)[(i)], (a)[(i)+1], (a)[(i)+2], (a)[(i)+3], (a)[(i)+4], (a)[(i)+5], \
	(a)[(i)+6], (a)[(i)+7])

static inline void
print_qt(unsigned char *qt)
{
	PDEBUG(5, "Y Quantization table:");
	PRN_QT_ROW(qt, 0);
	PRN_QT_ROW(qt, 8);
	PRN_QT_ROW(qt, 16);
	PRN_QT_ROW(qt, 24);
	PDEBUG(5, "UV Quantization table:");
	PRN_QT_ROW(qt, 32);
	PRN_QT_ROW(qt, 40);
	PRN_QT_ROW(qt, 48);
	PRN_QT_ROW(qt, 56);
}
#else
static inline void 
print_qt(unsigned char *qt) {  }
#endif	/* PRINT_QT */

/******************************************************************************
 * Huffman Decoder
 ******************************************************************************/

/* Note: There is no penalty for passing the tree as an argument, since dummy
 * args are passed anyway (to maintain 16-byte stack alignment), and since the
 * address is loaded into a register either way. */

/* If no node is found, coeffbits and skip will not be modified */
/* Return: Depth of node found, or -1 if invalid input code */
static int 
getNodeAC(unsigned int in, signed char *coeffbits, signed char *skip,
	  const struct tree_node *tree)
{
	int node = 0;
	int i = 0;
	int depth;

	do {
		if ((in & 0x80000000) == 0)
			node = tree[node].left;
		else
			node = tree[node].right;

		if (node == -1)
			break;

		depth = tree[node].depth;

		/* Is it a leaf? If not, branch downward */
		if (depth != -1) {
			*coeffbits = tree[node].coeffbits;
			*skip = tree[node].skip;
			return depth;
		}

		in <<= 1;
		++i;
	} while (i <= 15);

	return -1;
}

/* If no node is found, coeffbits will not be modified */
/* Return: Depth of node found, or -1 if invalid input code */
static int 
getNodeDC(unsigned int in, signed char *coeffbits, const struct tree_node *tree)
{
	int node = 0;
	int i = 0;
	int depth;

	do {
		if ((in & 0x80000000) == 0)
			node = tree[node].left;
		else
			node = tree[node].right;

		if (node == -1)
			break;

		depth = tree[node].depth;

		/* Is it a leaf? If not, branch downward */
		if (depth != -1) {
			*coeffbits = tree[node].coeffbits;
			return depth;
		}

		in <<= 1;
		++i;
	} while (i <= 15);

	return -1;
}

static inline unsigned int 
getBytes(int *rawData, struct comp_info *cinfo)
{
	int bufLen = cinfo->rawLen;
	int bits = cinfo->bits;
	int bytes = cinfo->bytes;
	unsigned char *in = bytes + (unsigned char *) rawData;
	unsigned char b1, b2, b3, b4, b5;
	unsigned int packedIn;

	/* Pull 5 bytes out of raw data */
	if (bytes < bufLen - 4) {
		b1 = in[0];
		b2 = in[1];
		b3 = in[2];
		b4 = in[3];
		b5 = in[4];
	} else {
		if (bytes < bufLen - 3) {
			b1 = in[0];
			b2 = in[1];
			b3 = in[2];
			b4 = in[3];
		} else {
			if (bytes < bufLen - 2) {
				b1 = in[0];
				b2 = in[1];
				b3 = in[2];
			} else {
				if (bytes < bufLen - 1) {
					b1 = in[0];
					b2 = in[1];
				} else {
					if (bytes <= bufLen) {
						b1 = in[0];
					} else {
						b1 = 0;
					}
					b2 = 0;
				}
				b3 = 0;
			}
			b4 = 0;
		}
		b5 = 0;
	}

	/* Pack the bytes */
	packedIn  = b1 << 24;
	packedIn += b2 << 16;
	packedIn += b3 << 8;
	packedIn += b4;

	if (bits != 0) {
		packedIn = packedIn << bits;
		packedIn += b5 >> (8 - bits);
	}

	return packedIn;
}

static int 
getACCoefficient(int *rawData, int *coeff, struct comp_info *cinfo,
		 const struct tree_node *tree)
{
	int input, bits, bytes, tmp_c;
	signed char coeffbits = 0;
	signed char skip = 0;

	input = getBytes(rawData, cinfo);
	bits = getNodeAC(input, &coeffbits, &skip, tree);

	if (coeffbits) {
		input = input << (bits - 1);
		input &= 0x7fffffff;
		if (! (input & 0x40000000))
			input |= 0x80000000;

		tmp_c = input >> (31 - coeffbits);
		if (tmp_c < 0)
			tmp_c++;
		*coeff = tmp_c;

		bits += coeffbits;
	}

	bytes = (bits + cinfo->bits) >> 3;
	cinfo->bytes += bytes;
	cinfo->bits += bits - (bytes << 3);

	return skip;
}

static void 
getDCCoefficient(int *rawData, int *coeff, struct comp_info *cinfo,
		 const struct tree_node *tree)
{
	int input, bits, bytes, tmp_c;
	signed char coeffbits = 0;

	input = getBytes(rawData, cinfo);
	bits = getNodeDC(input, &coeffbits, tree);

	if (bits == -1) {
		bits = 1;	/* Try to re-sync at the next bit */
		*coeff = 0;	/* Indicates no change from last DC */
	} else {

		input = input << (bits - 1);
		input &= 0x7fffffff;
		if (! (input & 0x40000000))
			input |= 0x80000000;

		tmp_c = input >> (31 - coeffbits);
		if (tmp_c < 0)
			tmp_c++;
		*coeff = tmp_c;

		bits += coeffbits;
	}

	bytes = (bits + cinfo->bits) >> 3;
	cinfo->bytes += bytes;
	cinfo->bits += bits - (bytes << 3);
}

/* For AC coefficients, here is what the "skip" value means:
 *   -1: Either the 8x4 block has ended, or the decoding failed.
 *    0: Use the returned coeff. Don't skip anything.
 * 1-15: The next <skip> coeffs are zero. The returned coeff is used.
 *   16: The next 16 coeffs are zero. The returned coeff is ignored.
 *
 * You must ensure that the C[] array not be overrun, or stack corruption will
 * result.
 */
static void 
huffmanDecoderY(int *C, int *pIn, struct comp_info *cinfo)
{
	int coeff = 0;
	int i = 1;
	int k, skip;

	getDCCoefficient(pIn, C, cinfo, treeYDC);

	i = 1;
	do {
		skip = getACCoefficient(pIn, &coeff, cinfo, treeYAC);

		if (skip == -1) {
			break;
		} else if (skip == 0) {
			C[i++] = coeff;
		} else if (skip == 16) {
			k = 16;
			if (i > 16)
				k = 32 - i;

			while (k--)
				C[i++] = 0;
		} else {
			k = skip;
			if (skip > 31 - i)
				k = 31 - i;

			while (k--)
				C[i++] = 0;

			C[i++] = coeff;
		}
	} while (i <= 31);

	if (skip == -1)
		while (i <= 31)  C[i++] = 0;
	else
		getACCoefficient(pIn, &coeff, cinfo, treeYAC);
}

/* Same as huffmanDecoderY, except for the tables used */
static void 
huffmanDecoderUV(int *C, int *pIn, struct comp_info *cinfo)
{
	int coeff = 0;
	int i = 1;
	int k, skip;

	getDCCoefficient(pIn, C, cinfo, treeUVDC);

	i = 1;
	do {
		skip = getACCoefficient(pIn, &coeff, cinfo, treeUVAC);

		if (skip == -1) {
			break;
		} else if (skip == 0) {
			C[i++] = coeff;
		} else if (skip == 16) {
			k = 16;
			if (i > 16)
				k = 32 - i;

			while (k--)
				C[i++] = 0;
		} else {
			k = skip;
			if (skip > 31 - i)
				k = 31 - i;

			while (k--)
				C[i++] = 0;

			C[i++] = coeff;
		}
	} while (i <= 31);

	if (skip == -1)
		while (i <= 31)  C[i++] = 0;
	else
		getACCoefficient(pIn, &coeff, cinfo, treeUVAC);
}

/******************************************************************************
 * iDCT Functions
 ******************************************************************************/

#ifndef APPROXIMATE_MUL_BY_SHIFT

#define IDCT_MESSAGE "iDCT with multiply"

#define TIMES_16382(u)	((u)? 16382 * (u):0)
#define TIMES_23168(u)	((u)? 23168 * (u):0)
#define TIMES_30270(u)	((u)? 30270 * (u):0)
#define TIMES_41986(u)	((u)? 41986 * (u):0)
#define TIMES_35594(u)	((u)? 35594 * (u):0)
#define TIMES_23783(u)	((u)? 23783 * (u):0)
#define TIMES_8351(u)	((u)? 8351  * (u):0)
#define TIMES_17391(u)	((u)? 17391 * (u):0)
#define TIMES_14743(u)	((u)? 14743 * (u):0)
#define TIMES_9851(u)	((u)? 9851  * (u):0)
#define TIMES_3459(u)	((u)? 3459  * (u):0)
#define TIMES_32134(u)	((u)? 32134 * (u):0)
#define TIMES_27242(u)	((u)? 27242 * (u):0)
#define TIMES_18202(u)	((u)? 18202 * (u):0)
#define TIMES_6392(u)	((u)? 6392  * (u):0)
#define TIMES_39550(u)	((u)? 39550 * (u):0)
#define TIMES_6785(u)	((u)? 6785  * (u):0)
#define TIMES_12538(u)	((u)? 12538 * (u):0)

#else

#define IDCT_MESSAGE "iDCT with shift"

#define TIMES_16382(u) ( (u)? x=(u) , (x<<14) - (x<<1) :0 )
#define TIMES_23168(u) ( (u)? x=(u) , (x<<14) + (x<<12) + (x<<11) + (x<<9) :0 )
#define TIMES_30270(u) ( (u)? x=(u) , (x<<15) - (x<<11) :0 )
#define TIMES_41986(u) ( (u)? x=(u) , (x<<15) + (x<<13) + (x<<10) :0 )
#define TIMES_35594(u) ( (u)? x=(u) , (x<<15) + (x<<11) + (x<<9) + (x<<8) :0 )
#define TIMES_23783(u) ( (u)? x=(u) , (x<<14) + (x<<13) - (x<<9) - (x<<8) :0 )
#define TIMES_8351(u)  ( (u)? x=(u) , (x<<13) :0 )
#define TIMES_17391(u) ( (u)? x=(u) , (x<<14) + (x<<10) :0 )
#define TIMES_14743(u) ( (u)? x=(u) , (x<<14) - (x<<10) - (x<<9) :0 )
#define TIMES_9851(u)  ( (u)? x=(u) , (x<<13) + (x<<10) + (x<<9) :0 )
#define TIMES_3459(u)  ( (u)? x=(u) , (x<<12) - (x<<9) :0 )
#define TIMES_32134(u) ( (u)? x=(u) , (x<<15) - (x<<9) :0 )
#define TIMES_27242(u) ( (u)? x=(u) , (x<<14) + (x<<13) + (x<<11) + (x<<9) :0 )
#define TIMES_18202(u) ( (u)? x=(u) , (x<<14) + (x<<11) - (x<<8) :0 )
#define TIMES_6392(u)  ( (u)? x=(u) , (x<<13) - (x<<11) + (x<<8) :0 )
#define TIMES_39550(u) ( (u)? x=(u) , (x<<15) + (x<<12) + (x<<11) + (x<<9) :0 )
#define TIMES_6785(u)  ( (u)? x=(u) , (x<<12) + (x<<11) + (x<<9) :0 )
#define TIMES_12538(u) ( (u)? x=(u) , (x<<13) + (x<<12) + (x<<8) :0 )

/*
 * The variables C0, C4, C16 and C20 can also be removed from the algorithm
 * if APPROXIMATE_MUL_BY_SHIFTS is defined. They store correction values
 * and can be considered insignificant.
 */

#endif

static void 
DCT_8x4(int *coeff, unsigned char *out)
/* pre: coeff == coefficients
   post: coeff != coefficients
   ** DO NOT ASSUME coeff TO BE THE SAME BEFORE AND AFTER CALLING THIS FUNCTION!
*/
{
	register int base,val1,val2,val3;
	int tmp1,tmp2;
	int C0,C4,C16,C20;
	int C2_18,C6_22,C1_17,C3_19,C5_21,C7_23;
	register int t;
#ifdef APPROXIMATE_MUL_BY_SHIFT
	register int x;
#endif

	C0=coeff[0];
	C4=coeff[4];
	C16=coeff[16];
	C20=coeff[20];

	coeff[0]=TIMES_23168(coeff[0]);
	coeff[4]=TIMES_23168(coeff[4]);
	coeff[16]=TIMES_23168(coeff[16]);
	coeff[20]=TIMES_23168(coeff[20]);

	C2_18 = coeff[2]+coeff[18];
	C6_22 = coeff[6]+coeff[22];
	C1_17 = coeff[1]+coeff[17];
	C3_19 = coeff[3]+coeff[19];
	C5_21 = coeff[5]+coeff[21];
	C7_23 = coeff[7]+coeff[23];

// 0,7,25,32

	base = 0x1000000;
	base += coeff[0]+coeff[4]+coeff[16]+coeff[20];
	base += TIMES_30270(C2_18);
	base += TIMES_12538(C6_22);

	val1 = TIMES_41986(coeff[9]);
	val1 += TIMES_35594(coeff[11]);
	val1 += TIMES_23783(coeff[13]);
	val1 += TIMES_8351(coeff[15]);
	val1 += TIMES_17391(coeff[25]);
	val1 += TIMES_14743(coeff[27]);
	val1 += TIMES_9851(coeff[29]);
	val1 += TIMES_3459(coeff[31]);

	val2 = TIMES_32134(C1_17);
	val2 += TIMES_27242(C3_19);
	val2 += TIMES_18202(C5_21);
	val2 += TIMES_6392(C7_23);

	val3 = TIMES_39550(coeff[10]);
	val3 += TIMES_16382(coeff[14]+coeff[26]);
	val3 += TIMES_6785(coeff[30]);
	val3 += TIMES_30270(coeff[8]+coeff[12]);
	val3 += TIMES_12538(coeff[24]+coeff[28]);

	t=(base + val1 + val2 + val3) >> 17;
	out[0]= t&0xFFFFFF00? t<0?0:255 : (unsigned char)t;
	t=(base - val1 - val2 + val3 - C4 - C20) >> 17;
	out[7]= t&0xFFFFFF00? t<0?0:255 : (unsigned char)t;
	t=(base - val1 + val2 - val3 - C16- C20) >> 17;
	out[24]= t&0xFFFFFF00? t<0?0:255 : (unsigned char)t;
	t=(base + val1 - val2 - val3 - C4 - C16 - C20) >> 17;
	out[31]= t&0xFFFFFF00? t<0?0:255 : (unsigned char)t;

//1,6,25,30

	base = 0x1000000;
	base += coeff[0]-coeff[4]+coeff[16]-coeff[20];
	base += TIMES_12538(C2_18);
	base -= TIMES_30270(C6_22);

	val1 = TIMES_35594(coeff[9]);
	val1 -= TIMES_8351(coeff[11]);
	val1 -= TIMES_41986(coeff[13]);
	val1 -= TIMES_23783(coeff[15]);
	val1 -= TIMES_14743(coeff[25]);
	val1 -= TIMES_3459(coeff[27]);
	val1 -= TIMES_17391(coeff[29]);
	val1 -= TIMES_9851(coeff[31]);

	val2 = TIMES_27242(C1_17);
	val2 -= TIMES_6392(C3_19);
	val2 -= TIMES_32134(C5_21);
	val2 -= TIMES_18202(C7_23);

	val3 = TIMES_16382(coeff[10]-coeff[30]);
	val3 -= TIMES_39550(coeff[14]);
	val3 += TIMES_6785(coeff[26]);
	val3 += TIMES_12538(coeff[24]-coeff[28]);
	val3 += TIMES_30270(coeff[8]-coeff[12]);

	t=(base + val1 + val2 + val3 + C4 + C20) >> 17;
	out[1]= t&0xFFFFFF00? t<0?0:255 : (unsigned char)t;
	t=(base - val1 - val2 + val3) >> 17;
	out[6]= t&0xFFFFFF00? t<0?0:255 : (unsigned char)t;
	t=(base - val1 + val2 - val3 + C4 - C16 + C20) >> 17;
	out[25]= t&0xFFFFFF00? t<0?0:255 : (unsigned char)t;
	t=(base + val1 - val2 - val3 + C20) >> 17;
	out[30]= t&0xFFFFFF00? t<0?0:255 : (unsigned char)t;

//2,5,26,29

	base = 0x1000000;
	base += coeff[0] - coeff[4] + coeff[16] - coeff[20];
	base -= TIMES_12538(C2_18);
	base += TIMES_30270(C6_22);

	val1 = TIMES_23783(coeff[9]);
	val1 -= TIMES_41986(coeff[11]);
	val1 += TIMES_8351(coeff[13]);
	val1 += TIMES_35594(coeff[15]);
	val1 += TIMES_9851(coeff[25]);
	val1 -= TIMES_17391(coeff[27]);
	val1 += TIMES_3459(coeff[29]);
	val1 += TIMES_14743(coeff[31]);

	val2 = TIMES_18202(C1_17);
	val2 -= TIMES_32134(C3_19);
	val2 += TIMES_6392(C5_21);
	val2 += TIMES_27242(C7_23);

	val3 = -TIMES_16382(coeff[10] - coeff[30]);
	val3 += TIMES_39550(coeff[14]);
	val3 -= TIMES_6785(coeff[26]);
	val3 += TIMES_12538(coeff[24] - coeff[28]);
	val3 += TIMES_30270(coeff[8] - coeff[12]);

	t=(base + val1 + val2 + val3) >> 17;
	out[2]= t&0xFFFFFF00? t<0?0:255 : (unsigned char)t;
	t=(base - val1 - val2 + val3) >> 17;
	out[5]= t&0xFFFFFF00? t<0?0:255 : (unsigned char)t;
	t=(base - val1 + val2 - val3 - C16) >> 17;
	out[26]= t&0xFFFFFF00? t<0?0:255 : (unsigned char)t;
	t=(base + val1 - val2 - val3 + C4 - C16 + C20) >> 17;
	out[29]= t&0xFFFFFF00? t<0?0:255 : (unsigned char)t;

//3,4,27,28

	base = 0x1000000;
	base += coeff[0] + coeff[4] + coeff[16] + coeff[20];
	base -= TIMES_30270(C2_18);
	base -= TIMES_12538(C6_22);

	val1 = TIMES_8351(coeff[9]);
	val1 -= TIMES_23783(coeff[11]);
	val1 += TIMES_35594(coeff[13]);
	val1 += TIMES_3459(coeff[25]);
	val1 -= TIMES_9851(coeff[27]);
	val1 += TIMES_14743(coeff[29]);

	val2 = TIMES_6392(C1_17);
	val2 -= TIMES_18202(C3_19);
	val2 += TIMES_27242(C5_21);

	val3 = -TIMES_39550(coeff[10]);
	val3 += TIMES_16382(coeff[14] + coeff[26]);
	val3 -= TIMES_6785(coeff[30]);
	val3 += TIMES_30270(coeff[8] + coeff[12]);
	val3 += TIMES_12538(coeff[24] + coeff[28]);

	tmp1 = TIMES_32134(C7_23);
	tmp2 = TIMES_41986(coeff[15]) + TIMES_17391(coeff[31]);

	t=(base + val1 + val2 + val3 - tmp1 - tmp2 - C4 - C20) >> 17;
	out[3]= t&0xFFFFFF00? t<0?0:255 : (unsigned char)t;
	t=(base - val1 - val2 + val3) >> 17;
	out[4]= t&0xFFFFFF00? t<0?0:255 : (unsigned char)t;
	t=(base - val1 + val2 - val3 - tmp1 + tmp2) >> 17;
	out[27]= t&0xFFFFFF00? t<0?0:255 : (unsigned char)t;
	t=(base + val1 - val2 - val3 - C16 - C20) >> 17;
	out[28]= t&0xFFFFFF00? t<0?0:255 : (unsigned char)t;

// Second half
	C2_18 = coeff[2] - coeff[18];
	C6_22 = coeff[6] - coeff[22];
	C1_17 = coeff[1] - coeff[17];
	C3_19 = coeff[3] - coeff[19];
	C5_21 = coeff[5] - coeff[21];
	C7_23 = coeff[7] - coeff[23];

// 8,15,16,23

	base = 0x1000000;
	base += coeff[0] + coeff[4] - coeff[16] - coeff[20];
	base +=TIMES_30270(C2_18);
	base +=TIMES_12538(C6_22);

	val1 = TIMES_17391(coeff[9]);
	val1 += TIMES_14743(coeff[11]);
	val1 += TIMES_9851(coeff[13]);
	val1 += TIMES_3459(coeff[15]);
	val1 -= TIMES_41986(coeff[25]);
	val1 -= TIMES_35594(coeff[27]);
	val1 -= TIMES_23783(coeff[29]);
	val1 -= TIMES_8351(coeff[31]);

	val2 = TIMES_32134(C1_17);
	val2 += TIMES_27242(C3_19);
	val2 += TIMES_18202(C5_21);
	val2 += TIMES_6392(C7_23);

	val3 = TIMES_16382(coeff[10] - coeff[30]);
	val3 += TIMES_6785(coeff[14]);
	val3 -= TIMES_39550(coeff[26]);
	val3 -=TIMES_30270(coeff[24] + coeff[28]);
	val3 +=TIMES_12538(coeff[8] + coeff[12]);

	t=(base + val1 + val2 + val3) >> 17;
	out[8]= t&0xFFFFFF00? t<0?0:255 : (unsigned char)t;
	t=(base - val1 - val2 + val3 - C4 + C16 + C20) >> 17;
	out[15]= t&0xFFFFFF00? t<0?0:255 : (unsigned char)t;
	t=(base - val1 + val2 - val3) >> 17;
	out[16]= t&0xFFFFFF00? t<0?0:255 : (unsigned char)t;
	t=(base + val1 - val2 - val3 - C4 + C20) >> 17;
	out[23]= t&0xFFFFFF00? t<0?0:255 : (unsigned char)t;

//9,14,17,22

	base = 0x1000000;
	base += coeff[0] - coeff[4] - coeff[16] + coeff[20];
	base += TIMES_12538(C2_18);
	base -= TIMES_30270(C6_22);

	val1 = TIMES_14743(coeff[9]);
	val1 -= TIMES_3459(coeff[11]);
	val1 -= TIMES_17391(coeff[13]);
	val1 -= TIMES_9851(coeff[15]);
	val1 -= TIMES_35594(coeff[25]);
	val1 += TIMES_8351(coeff[27]);
	val1 += TIMES_41986(coeff[29]);
	val1 += TIMES_23783(coeff[31]);

	val2 = TIMES_27242(C1_17);
	val2 -= TIMES_6392(C3_19);
	val2 -= TIMES_32134(C5_21);
	val2 -= TIMES_18202(C7_23);

	val3 = TIMES_6785(coeff[10]);
	val3 -= TIMES_16382(coeff[14] + coeff[26]);
	val3 += TIMES_39550(coeff[30]);
	val3 += TIMES_12538(coeff[8] - coeff[12]);
	val3 -= TIMES_30270(coeff[24] - coeff[28]);

	t=(base + val1 + val2 + val3 + C4 + C16 - C20) >> 17;
	out[9]= t&0xFFFFFF00? t<0?0:255 : (unsigned char)t;
	t=(base - val1 - val2 + val3 + C16) >> 17;
	out[14]= t&0xFFFFFF00? t<0?0:255 : (unsigned char)t;
	t=(base - val1 + val2 - val3 + C4) >> 17;
	out[17]= t&0xFFFFFF00? t<0?0:255 : (unsigned char)t;
	t=(base + val1 - val2 - val3) >> 17;
	out[22]= t&0xFFFFFF00? t<0?0:255 : (unsigned char)t;

//10,13,18,21

	base = 0x1000000;
	base += coeff[0] - coeff[4] - coeff[16] + coeff[20];
	base -= TIMES_12538(C2_18);
	base += TIMES_30270(C6_22);

	val1 = TIMES_9851(coeff[9]);
	val1 -= TIMES_17391(coeff[11]);
	val1 += TIMES_3459(coeff[13]);
	val1 += TIMES_14743(coeff[15]);
	val1 -= TIMES_23783(coeff[25]);
	val1 += TIMES_41986(coeff[27]);
	val1 -= TIMES_8351(coeff[29]);
	val1 -= TIMES_35594(coeff[31]);

	val2 = TIMES_18202(C1_17);
	val2 -= TIMES_32134(C3_19);
	val2 += TIMES_6392(C5_21);
	val2 += TIMES_27242(C7_23);

	val3 = -TIMES_6785(coeff[10]);
	val3 += TIMES_16382(coeff[14]+coeff[26]);
	val3 -= TIMES_39550(coeff[30]);
	val3 += TIMES_12538(coeff[8]-coeff[12]);
	val3 -= TIMES_30270(coeff[24]-coeff[28]);

	t=(base + val1 + val2 + val3) >> 17;
	out[10]= t&0xFFFFFF00? t<0?0:255 : (unsigned char)t;
	t=(base - val1 - val2 + val3 + C4 + C16 - C20) >> 17;
	out[13]= t&0xFFFFFF00? t<0?0:255 : (unsigned char)t;
	t=(base - val1 + val2 - val3) >> 17;
	out[18]= t&0xFFFFFF00? t<0?0:255 : (unsigned char)t;
	t=(base + val1 - val2 - val3 + C4) >> 17;
	out[21]= t&0xFFFFFF00? t<0?0:255 : (unsigned char)t;

// 11,12,19,20

	base = 0x1000000;
	base += coeff[0]+coeff[4]-coeff[16]-coeff[20];
	base -= TIMES_30270(C2_18);
	base -= TIMES_12538(C6_22);

	val1 = TIMES_3459(coeff[9]);
	val1 -= TIMES_9851(coeff[11]);
	val1 += TIMES_14743(coeff[13]);
	val1 -= TIMES_8351(coeff[25]);
	val1 += TIMES_23783(coeff[27]);
	val1 -= TIMES_35594(coeff[29]);

	val2 = TIMES_6392(C1_17);
	val2 -= TIMES_18202(C3_19);
	val2 += TIMES_27242(C5_21);

	val3 = -TIMES_16382(coeff[10] - coeff[30]);
	val3 -= TIMES_6785(coeff[14]);
	val3 += TIMES_39550(coeff[26]);
	val3 -= TIMES_30270(coeff[24]+coeff[28]);
	val3 += TIMES_12538(coeff[8]+coeff[12]);

	tmp1 = TIMES_32134(C7_23);
	tmp2 = -TIMES_17391(coeff[15]) + TIMES_41986(coeff[31]);

	t=(base + val1 + val2 + val3 - tmp1 + tmp2 + C16 + C20) >> 17;
	out[11]= t&0xFFFFFF00? t<0?0:255 : (unsigned char)t;
	t=(base - val1 - val2 + val3 + C16 + C20) >> 17;
	out[12]= t&0xFFFFFF00? t<0?0:255 : (unsigned char)t;
	t=(base - val1 + val2 - val3 - tmp1 - tmp2 - C4 + C20) >> 17;
	out[19]= t&0xFFFFFF00? t<0?0:255 : (unsigned char)t;
	t=(base + val1 - val2 - val3) >> 17;
	out[20]= t&0xFFFFFF00? t<0?0:255 : (unsigned char)t;
}

#undef TIMES_16382
#undef TIMES_23168
#undef TIMES_30270
#undef TIMES_41986
#undef TIMES_35594
#undef TIMES_23783
#undef TIMES_8351
#undef TIMES_17391
#undef TIMES_14743
#undef TIMES_9851
#undef TIMES_3459
#undef TIMES_32134
#undef TIMES_27242
#undef TIMES_18202
#undef TIMES_6392
#undef TIMES_39550
#undef TIMES_6785
#undef TIMES_12538

/******************************************************************************
 * Main Decoder Functions
 ******************************************************************************/

/* This function handles the decompression of a single 8x4 block. It is
 * independent of the palette (YUV422, YUV420, YUV400, GBR422...). cinfo->bytes
 * determines the positin in the input buffer.
 */
static int 
decompress8x4(unsigned char	*pOut,
	      unsigned char	*pIn,
	      int		*lastDC,
	      int		uvFlag,
	      struct comp_info	*cinfo)
{
	int i, x, y, dc;
	int coeffs[32];
	int deZigZag[32];
	int *dest;
	int *src;
	unsigned char *qt = cinfo->qt;

	if (! uvFlag) {
		huffmanDecoderY(coeffs, (int*) pIn, cinfo);

		/* iDPCM and dequantize first coefficient */
		dc = (*lastDC) + coeffs[0];
		coeffs[0] = dc * (qt[0] + 1);
		*lastDC = dc;

		/* ...and the second coefficient */
		coeffs[1] = ((qt[1] + 1) * coeffs[1]) >> 1;

		/* Dequantize, starting at 3rd element */
		for (i = 2; i < 32; i++)
			coeffs[i] = (qt[i] + 1) * coeffs[i];
	} else {
		huffmanDecoderUV(coeffs, (int*) pIn, cinfo);

		/* iDPCM */
		dc = (*lastDC) + coeffs[0];
		coeffs[0] = dc;
		*lastDC = dc;

		/* Dequantize */
		for (i = 0; i < 32; i++)
			coeffs[i] = (qt[32 + i] + 1) * coeffs[i];
	}

	/* Dezigzag */
	for (i = 0; i < 32; i++)
		deZigZag[i] = coeffs[ZigZag518[i]];

	/* Transpose the dezigzagged coefficient matrix */
	src = deZigZag;
	dest = coeffs;
	for (y = 0; y <= 3; ++y) {
		for (x = 0; x <= 7; ++x) {
			dest[x] = src[x * 4];
		}
		src += 1;
		dest += 8;
	}

	/* Do the inverse DCT transform */
	DCT_8x4(coeffs, pOut);

	return 0;	/* Always returns 0 */
}

static inline void 
copyBlock(unsigned char *src, unsigned char *dest, int destInc)
{
	int i;
	unsigned int *pSrc, *pDest;

	for (i = 0; i <= 3; i++) {
		pSrc = (unsigned int *) src;
		pDest = (unsigned int *) dest;
		pDest[0] = pSrc[0];
		pDest[1] = pSrc[1];
		src += 8;
		dest += destInc;
	}
}

static inline int
decompress400NoMMXOV518(unsigned char	 *pIn,
			unsigned char	 *pOut,
			unsigned char	 *pTmp,
			const int	 w, 
			const int	 h, 
			const int	 numpix,
			struct comp_info *cinfo)
{
	int iOutY, x, y;
	int lastYDC = 0;

	/* Start Y loop */
	y = 0;
	do {
		iOutY = w * y;
		x = 0;
		do {
			decompress8x4(pTmp, pIn, &lastYDC, 0, cinfo);
			copyBlock(pTmp, pOut + iOutY, w);
			iOutY += 8;
			x += 8;
		} while (x < w);
		y += 4;
	} while (y < h);

	/* Did we decode too much? */
	if (cinfo->bytes > cinfo->rawLen + 897)
		return 1;

	/* Did we decode enough? */
	if (cinfo->bytes >= cinfo->rawLen - 897)
		return 0;
	else
		return 1;
}

static inline int
decompress420NoMMXOV518(unsigned char	 *pIn,
			unsigned char	 *pOut,
			unsigned char	 *pTmp,
			const int	 w, 
			const int	 h, 
			const int	 numpix,
			struct comp_info *cinfo)
{
	unsigned char *pOutU = pOut + numpix;
	unsigned char *pOutV = pOutU + numpix / 4;
	int iOutY, iOutU, iOutV, x, y;
	int lastYDC = 0;
	int lastUDC = 0;
	int lastVDC = 0;

	/* Start Y loop */
	y = 0;
	do {
		iOutY = w * y;
		iOutV = iOutU = iOutY / 4;

		x = 0;
		do {
			decompress8x4(pTmp, pIn, &lastYDC, 0, cinfo);
			copyBlock(pTmp, pOut + iOutY, w);
			iOutY += 8;
			x += 8;
		} while (x < w);



		iOutY = w * (y + 4);
		x = 0;
		do {
			decompress8x4(pTmp, pIn, &lastUDC, 1, cinfo);
			copyBlock(pTmp, pOutU + iOutU, w/2);
			iOutU += 8;

			decompress8x4(pTmp, pIn, &lastVDC, 1, cinfo);
			copyBlock(pTmp, pOutV + iOutV, w/2);
			iOutV += 8;

			decompress8x4(pTmp, pIn, &lastYDC, 0, cinfo);
			copyBlock(pTmp, pOut + iOutY, w);
			iOutY += 8;

			decompress8x4(pTmp, pIn, &lastYDC, 0, cinfo);
			copyBlock(pTmp, pOut + iOutY, w);
			iOutY += 8;

			x += 16;
		} while (x < w);

		y += 8;
	} while (y < h);

	/* Did we decode too much? */
	if (cinfo->bytes > cinfo->rawLen + 897)
		return 1;

	/* Did we decode enough? */
	if (cinfo->bytes >= cinfo->rawLen - 897)
		return 0;
	else
		return 1;
}

/* Get quantization tables from static arrays
 * Returns: <0 if error, or >=0 otherwise */
static int
get_qt_static(struct comp_info *cinfo)
{
	unsigned char qtY[] = OV518_YQUANTABLE;
	unsigned char qtUV[] = OV518_UVQUANTABLE;
	unsigned char qt[64];

	memcpy(qt, qtY, 32);
	memcpy(qt + 32, qtUV, 32);
	cinfo->qt = qt;

	return 0;
}


/* Get quantization tables from input
 * Returns: <0 if error, or >=0 otherwise */
static int
get_qt_dynamic(unsigned char *pIn, struct comp_info *cinfo)
{
	int rawLen = cinfo->rawLen;

	/* Make sure input is actually big enough to hold trailer */
	if (rawLen < 72) {
		PDEBUG(1, "Not enough input to decompress");
		return -EINVAL;
	}

	cinfo->qt = pIn + rawLen - 64;

	print_qt(cinfo->qt);

	return 0;
}

/* Input format is raw isoc. data (with intact SOF header, packet numbers
 * stripped, and all-zero blocks removed).
 * Output format is planar YUV400
 * Returns uncompressed data length if success, or zero if error
 */
static int 
Decompress400(unsigned char *pIn,
	      unsigned char *pOut,
	      unsigned char *pTmp,
	      int	     w,
	      int	     h,
	      int	     inSize)
{
	struct comp_info cinfo;
	int numpix = w * h;

	PDEBUG(4, "%dx%d pIn=%p pOut=%p pTmp=%p inSize=%d", w, h, pIn, pOut,
	       pTmp, inSize);

	cinfo.bytes = 0;
	cinfo.bits = 0;
	cinfo.rawLen = inSize;

	if (staticquant) {
		if (get_qt_static(&cinfo) < 0)
			return 0;
	} else {
		if (get_qt_dynamic(pIn, &cinfo) < 0)
			return 0;
	}

	/* Decompress, skipping the 8-byte SOF header */
	if (decompress400NoMMXOV518(pIn + 8, pOut, pTmp, w, h, numpix, &cinfo))
//		return 0;
		; /* Don't return error yet */

	return (numpix);
}

/* Input format is raw isoc. data (with intact SOF header, packet numbers
 * stripped, and all-zero blocks removed).
 * Output format is planar YUV420
 * Returns uncompressed data length if success, or zero if error
 */
static int 
Decompress420(unsigned char *pIn,
	      unsigned char *pOut,
	      unsigned char *pTmp,
	      int	     w,
	      int	     h,
	      int	     inSize)
{
	struct comp_info cinfo;
	int numpix = w * h;

	PDEBUG(4, "%dx%d pIn=%p pOut=%p pTmp=%p inSize=%d", w, h, pIn, pOut,
	       pTmp, inSize);

	cinfo.bytes = 0;
	cinfo.bits = 0;
	cinfo.rawLen = inSize;

	if (staticquant) {
		if (get_qt_static(&cinfo) < 0)
			return 0;
	} else {
		if (get_qt_dynamic(pIn, &cinfo) < 0)
			return 0;
	}

	/* Decompress, skipping the 8-byte SOF header */
	if (decompress420NoMMXOV518(pIn + 8, pOut, pTmp, w, h, numpix, &cinfo))
//		return 0;
		; /* Don't return error yet */

	return (numpix * 3 / 2);
}

/******************************************************************************
 * Module Functions
 ******************************************************************************/

static struct ov51x_decomp_ops decomp_ops = {
	.decomp_400 =	Decompress400,	
	.decomp_420 =	Decompress420,	
	.owner =	THIS_MODULE,
};

static int __init 
decomp_init(void)
{
	int rc;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0)
	EXPORT_NO_SYMBOLS;
#endif

	rc = ov511_register_decomp_module(DECOMP_INTERFACE_VER, &decomp_ops,
					  ov518, mmx);
	if (rc) {
		err("Could not register with ov511 (rc=%d)", rc);
		return -1;
	}

	info(DRIVER_VERSION " : " DRIVER_DESC);
	PDEBUG(1, "Using %s, %s quantization", IDCT_MESSAGE, 
	       staticquant ? "static" : "dynamic");

	return 0;
}

static void __exit 
decomp_exit(void)
{
	ov511_deregister_decomp_module(ov518, mmx);
	info("deregistered");
}

module_init(decomp_init);
module_exit(decomp_exit);
