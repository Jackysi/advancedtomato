/*
 * THIS FILE IS GENERATED AUTOMATICALLY BY ./gentbl, DO NOT EDIT!
 */


/*
 * small cosine table in U8 format
 */
#define OFFSCOSTABBITS 6
#define OFFSCOSTABSIZE (1<<OFFSCOSTABBITS)

static unsigned char offscostab[OFFSCOSTABSIZE] = {
	 255, 254, 252, 249, 245, 240, 233, 226,
	 217, 208, 198, 187, 176, 164, 152, 140,
	 128, 115, 103,  91,  79,  68,  57,  47,
	  38,  29,  22,  15,  10,   6,   3,   1,
	   1,   1,   3,   6,  10,  15,  22,  29,
	  38,  47,  57,  68,  79,  91, 103, 115,
	 127, 140, 152, 164, 176, 187, 198, 208,
	 217, 226, 233, 240, 245, 249, 252, 254
};

#define OFFSCOS(x) offscostab[((x)>>10)&0x3f]


/*
 * more accurate cosine table
 */

static const short costab[64] = {
	 32767,  32609,  32137,  31356,  30272,  28897,  27244,  25329, 
	 23169,  20787,  18204,  15446,  12539,   9511,   6392,   3211, 
	     0,  -3211,  -6392,  -9511, -12539, -15446, -18204, -20787, 
	-23169, -25329, -27244, -28897, -30272, -31356, -32137, -32609, 
	-32767, -32609, -32137, -31356, -30272, -28897, -27244, -25329, 
	-23169, -20787, -18204, -15446, -12539,  -9511,  -6392,  -3211, 
	     0,   3211,   6392,   9511,  12539,  15446,  18204,  20787, 
	 23169,  25329,  27244,  28897,  30272,  31356,  32137,  32609
};

#define COS(x) costab[((x)>>10)&0x3f]
#define SIN(x) COS((x)+0xc000)


/*
 * afsk2666 specific tables
 */
#define AFSK26_DEMCORRLEN 12
#define AFSK26_SAMPLERATE 16000

static const unsigned int afsk26_carfreq[2] = { 0x2000, 0x3555 };


static const struct {
	int i[12];
	int q[12];
} afsk26_dem_tables[2][2] = {
	{
		{{      1,      7,    -18,    -73,   -100,    -47,     47,    100,     73,     18,     -7,     -1 }, {      0,     17,     43,     30,    -41,   -115,   -115,    -41,     30,     43,     17,      0 }},
#define AFSK26_DEM_SUM_I_0_0 0
#define AFSK26_DEM_SUM_Q_0_0 -132
		{{      1,     -7,    -46,    -10,    100,     76,    -75,   -100,     10,     46,      7,     -1 }, {      1,     17,     -6,    -79,    -41,     99,     99,    -41,    -79,     -6,     17,      1 }}
#define AFSK26_DEM_SUM_I_0_1 1
#define AFSK26_DEM_SUM_Q_0_1 -18
	},
	{
		{{      8,     22,      0,    -67,   -118,    -89,      0,     67,     63,     22,      0,      0 }, {      0,     22,     63,     67,      0,    -89,   -118,    -67,      0,     22,      8,      0 }},
#define AFSK26_DEM_SUM_I_1_0 -92
#define AFSK26_DEM_SUM_Q_1_0 -92
		{{      8,      8,    -54,    -67,     59,    122,      0,    -91,    -31,     22,      7,      0 }, {      0,     30,     31,    -67,   -102,     32,    118,     24,    -54,    -22,      4,      0 }}
#define AFSK26_DEM_SUM_I_1_1 -17
#define AFSK26_DEM_SUM_Q_1_1 -6
	}
};

