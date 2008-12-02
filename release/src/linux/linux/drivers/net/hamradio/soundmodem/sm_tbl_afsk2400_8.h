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
 * afsk2400 specific tables (tcm3105 clk 8000000.000000Hz)
 */
#define AFSK24_TX_FREQ_LO 2165
#define AFSK24_TX_FREQ_HI 3970
#define AFSK24_BITPLL_INC 9830
#define AFSK24_SAMPLERATE 16000

static const int afsk24_tx_lo_i[] = {
	   10,   11,   -4,  -51,  -86,  -49,   47,  118,   97,   17,  -37,  -35,  -11,    0 
};
#define SUM_AFSK24_TX_LO_I 27

static const int afsk24_tx_lo_q[] = {
	    0,   12,   35,   34,  -22, -100, -115,  -40,   55,   87,   48,    2,  -11,  -10 
};
#define SUM_AFSK24_TX_LO_Q -25

static const int afsk24_tx_hi_i[] = {
	   10,    0,  -35,   -2,   89,    6, -124,  -10,  111,    9,  -61,   -4,   16,    1 
};
#define SUM_AFSK24_TX_HI_I 6

static const int afsk24_tx_hi_q[] = {
	    0,   16,    0,  -61,   -4,  112,    8, -124,  -10,   88,    7,  -35,   -2,   10 
};
#define SUM_AFSK24_TX_HI_Q 5

