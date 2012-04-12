/* ANSI-C code produced by gperf version 3.0.2 */
/* Command-line: gperf -m 10 lib/aliases.gperf  */
/* Computed positions: -k'4-7,10,$' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gnu-gperf@gnu.org>."
#endif

#line 1 "lib/aliases.gperf"
struct alias { int name; unsigned int encoding_index; };

#define TOTAL_KEYWORDS 70
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 17
#define MIN_HASH_VALUE 4
#define MAX_HASH_VALUE 98
/* maximum key range = 95, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
aliases_hash (register const char *str, register unsigned int len)
{
  static const unsigned char asso_values[] =
    {
      99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99,  6, 99, 99, 22,  3,
       2,  2, 17,  3,  2,  7,  3,  2, 99, 99,
      99, 99, 99, 99, 99,  4, 53,  2, 21,  2,
      99,  7, 99, 15, 99, 99, 19, 99, 33,  2,
       4, 99, 11, 26, 17, 99, 99,  3, 26, 99,
      99, 99, 99, 99, 99, 10, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99
    };
  register int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[9]];
      /*FALLTHROUGH*/
      case 9:
      case 8:
      case 7:
        hval += asso_values[(unsigned char)str[6]];
      /*FALLTHROUGH*/
      case 6:
        hval += asso_values[(unsigned char)str[5]];
      /*FALLTHROUGH*/
      case 5:
        hval += asso_values[(unsigned char)str[4]];
      /*FALLTHROUGH*/
      case 4:
        hval += asso_values[(unsigned char)str[3]];
      /*FALLTHROUGH*/
      case 3:
      case 2:
        break;
    }
  return hval + asso_values[(unsigned char)str[len - 1]];
}

struct stringpool_t
  {
    char stringpool_str4[sizeof("L2")];
    char stringpool_str5[sizeof("L1")];
    char stringpool_str11[sizeof("MS-EE")];
    char stringpool_str12[sizeof("CP819")];
    char stringpool_str15[sizeof("UCS-2")];
    char stringpool_str16[sizeof("IBM819")];
    char stringpool_str17[sizeof("UTF-8")];
    char stringpool_str18[sizeof("UTF-32")];
    char stringpool_str19[sizeof("UTF-16")];
    char stringpool_str21[sizeof("CP367")];
    char stringpool_str22[sizeof("ISO8859-2")];
    char stringpool_str23[sizeof("ISO8859-1")];
    char stringpool_str24[sizeof("IBM367")];
    char stringpool_str25[sizeof("UTF-7")];
    char stringpool_str26[sizeof("CHAR")];
    char stringpool_str27[sizeof("ISO8859-15")];
    char stringpool_str28[sizeof("US")];
    char stringpool_str29[sizeof("ISO-8859-2")];
    char stringpool_str31[sizeof("ISO-8859-1")];
    char stringpool_str32[sizeof("ISO-8859-15")];
    char stringpool_str33[sizeof("ISO_8859-2")];
    char stringpool_str35[sizeof("ISO_8859-1")];
    char stringpool_str36[sizeof("ISO_8859-15")];
    char stringpool_str37[sizeof("KOI8-R")];
    char stringpool_str38[sizeof("UCS-2LE")];
    char stringpool_str39[sizeof("UTF-32LE")];
    char stringpool_str40[sizeof("UTF-16LE")];
    char stringpool_str41[sizeof("ISO_8859-15:1998")];
    char stringpool_str43[sizeof("ISO_8859-2:1987")];
    char stringpool_str44[sizeof("ISO_8859-1:1987")];
    char stringpool_str45[sizeof("UCS-4")];
    char stringpool_str47[sizeof("UNICODE-1-1")];
    char stringpool_str48[sizeof("ISO-IR-6")];
    char stringpool_str49[sizeof("CSKOI8R")];
    char stringpool_str50[sizeof("ASCII")];
    char stringpool_str51[sizeof("UNICODEBIG")];
    char stringpool_str52[sizeof("ISO-IR-203")];
    char stringpool_str53[sizeof("UCS-4LE")];
    char stringpool_str54[sizeof("ISO-IR-101")];
    char stringpool_str55[sizeof("CP1250")];
    char stringpool_str56[sizeof("ISO-10646-UCS-2")];
    char stringpool_str57[sizeof("UNICODE-1-1-UTF-7")];
    char stringpool_str58[sizeof("LATIN2")];
    char stringpool_str59[sizeof("UNICODELITTLE")];
    char stringpool_str60[sizeof("LATIN1")];
    char stringpool_str61[sizeof("ISO_646.IRV:1991")];
    char stringpool_str62[sizeof("ISO646-US")];
    char stringpool_str63[sizeof("CSUNICODE")];
    char stringpool_str64[sizeof("UCS-2-INTERNAL")];
    char stringpool_str65[sizeof("LATIN-9")];
    char stringpool_str66[sizeof("WCHAR_T")];
    char stringpool_str68[sizeof("CSUCS4")];
    char stringpool_str69[sizeof("CSUNICODE11")];
    char stringpool_str70[sizeof("US-ASCII")];
    char stringpool_str71[sizeof("ISO-10646-UCS-4")];
    char stringpool_str72[sizeof("UCS-2BE")];
    char stringpool_str73[sizeof("UTF-32BE")];
    char stringpool_str74[sizeof("UTF-16BE")];
    char stringpool_str75[sizeof("ANSI_X3.4-1986")];
    char stringpool_str76[sizeof("ANSI_X3.4-1968")];
    char stringpool_str77[sizeof("CSUNICODE11UTF7")];
    char stringpool_str78[sizeof("UCS-2-SWAPPED")];
    char stringpool_str79[sizeof("UCS-4-INTERNAL")];
    char stringpool_str80[sizeof("CSASCII")];
    char stringpool_str87[sizeof("UCS-4BE")];
    char stringpool_str88[sizeof("WINDOWS-1250")];
    char stringpool_str92[sizeof("ISO-IR-100")];
    char stringpool_str93[sizeof("UCS-4-SWAPPED")];
    char stringpool_str97[sizeof("CSISOLATIN2")];
    char stringpool_str98[sizeof("CSISOLATIN1")];
  };
static const struct stringpool_t stringpool_contents =
  {
    "L2",
    "L1",
    "MS-EE",
    "CP819",
    "UCS-2",
    "IBM819",
    "UTF-8",
    "UTF-32",
    "UTF-16",
    "CP367",
    "ISO8859-2",
    "ISO8859-1",
    "IBM367",
    "UTF-7",
    "CHAR",
    "ISO8859-15",
    "US",
    "ISO-8859-2",
    "ISO-8859-1",
    "ISO-8859-15",
    "ISO_8859-2",
    "ISO_8859-1",
    "ISO_8859-15",
    "KOI8-R",
    "UCS-2LE",
    "UTF-32LE",
    "UTF-16LE",
    "ISO_8859-15:1998",
    "ISO_8859-2:1987",
    "ISO_8859-1:1987",
    "UCS-4",
    "UNICODE-1-1",
    "ISO-IR-6",
    "CSKOI8R",
    "ASCII",
    "UNICODEBIG",
    "ISO-IR-203",
    "UCS-4LE",
    "ISO-IR-101",
    "CP1250",
    "ISO-10646-UCS-2",
    "UNICODE-1-1-UTF-7",
    "LATIN2",
    "UNICODELITTLE",
    "LATIN1",
    "ISO_646.IRV:1991",
    "ISO646-US",
    "CSUNICODE",
    "UCS-2-INTERNAL",
    "LATIN-9",
    "WCHAR_T",
    "CSUCS4",
    "CSUNICODE11",
    "US-ASCII",
    "ISO-10646-UCS-4",
    "UCS-2BE",
    "UTF-32BE",
    "UTF-16BE",
    "ANSI_X3.4-1986",
    "ANSI_X3.4-1968",
    "CSUNICODE11UTF7",
    "UCS-2-SWAPPED",
    "UCS-4-INTERNAL",
    "CSASCII",
    "UCS-4BE",
    "WINDOWS-1250",
    "ISO-IR-100",
    "UCS-4-SWAPPED",
    "CSISOLATIN2",
    "CSISOLATIN1"
  };
#define stringpool ((const char *) &stringpool_contents)

static const struct alias aliases[] =
  {
    {-1}, {-1}, {-1}, {-1},
#line 66 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str4, ei_iso8859_2},
#line 58 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str5, ei_iso8859_1},
    {-1}, {-1}, {-1}, {-1}, {-1},
#line 79 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str11, ei_cp1250},
#line 55 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str12, ei_iso8859_1},
    {-1}, {-1},
#line 24 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str15, ei_ucs2},
#line 56 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str16, ei_iso8859_1},
#line 23 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str17, ei_utf8},
#line 41 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str18, ei_utf32},
#line 38 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str19, ei_utf16},
    {-1},
#line 19 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str21, ei_ascii},
#line 68 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str22, ei_iso8859_2},
#line 60 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str23, ei_iso8859_1},
#line 20 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str24, ei_ascii},
#line 44 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str25, ei_utf7},
#line 80 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str26, ei_local_char},
#line 74 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str27, ei_iso8859_15},
#line 21 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str28, ei_ascii},
#line 61 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str29, ei_iso8859_2},
    {-1},
#line 51 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str31, ei_iso8859_1},
#line 69 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str32, ei_iso8859_15},
#line 62 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str33, ei_iso8859_2},
    {-1},
#line 52 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str35, ei_iso8859_1},
#line 70 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str36, ei_iso8859_15},
#line 75 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str37, ei_koi8_r},
#line 31 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str38, ei_ucs2le},
#line 43 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str39, ei_utf32le},
#line 40 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str40, ei_utf16le},
#line 71 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str41, ei_iso8859_15},
    {-1},
#line 63 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str43, ei_iso8859_2},
#line 53 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str44, ei_iso8859_1},
#line 33 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str45, ei_ucs4},
    {-1},
#line 29 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str47, ei_ucs2be},
#line 16 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str48, ei_ascii},
#line 76 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str49, ei_koi8_r},
#line 13 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str50, ei_ascii},
#line 28 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str51, ei_ucs2be},
#line 72 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str52, ei_iso8859_15},
#line 37 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str53, ei_ucs4le},
#line 64 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str54, ei_iso8859_2},
#line 77 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str55, ei_cp1250},
#line 25 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str56, ei_ucs2},
#line 45 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str57, ei_utf7},
#line 65 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str58, ei_iso8859_2},
#line 32 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str59, ei_ucs2le},
#line 57 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str60, ei_iso8859_1},
#line 15 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str61, ei_ascii},
#line 14 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str62, ei_ascii},
#line 26 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str63, ei_ucs2},
#line 47 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str64, ei_ucs2internal},
#line 73 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str65, ei_iso8859_15},
#line 81 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str66, ei_local_wchar_t},
    {-1},
#line 35 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str68, ei_ucs4},
#line 30 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str69, ei_ucs2be},
#line 12 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str70, ei_ascii},
#line 34 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str71, ei_ucs4},
#line 27 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str72, ei_ucs2be},
#line 42 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str73, ei_utf32be},
#line 39 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str74, ei_utf16be},
#line 18 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str75, ei_ascii},
#line 17 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str76, ei_ascii},
#line 46 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str77, ei_utf7},
#line 48 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str78, ei_ucs2swapped},
#line 49 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str79, ei_ucs4internal},
#line 22 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str80, ei_ascii},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 36 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str87, ei_ucs4be},
#line 78 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str88, ei_cp1250},
    {-1}, {-1}, {-1},
#line 54 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str92, ei_iso8859_1},
#line 50 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str93, ei_ucs4swapped},
    {-1}, {-1}, {-1},
#line 67 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str97, ei_iso8859_2},
#line 59 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str98, ei_iso8859_1}
  };

#ifdef __GNUC__
__inline
#endif
const struct alias *
aliases_lookup (register const char *str, register unsigned int len)
{
  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = aliases_hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register int o = aliases[key].name;
          if (o >= 0)
            {
              register const char *s = o + stringpool;

              if (*str == *s && !strcmp (str + 1, s + 1))
                return &aliases[key];
            }
        }
    }
  return 0;
}
