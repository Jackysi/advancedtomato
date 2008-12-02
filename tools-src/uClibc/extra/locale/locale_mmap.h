/* #include "lt_defines.h" */

/* TODO - fix */
#define MAGIC_SIZE 64

/* TODO - fix */
#ifdef __WCHAR_ENABLED
#define WCctype_TBL_LEN		(WCctype_II_LEN + WCctype_TI_LEN + WCctype_UT_LEN)
#define WCuplow_TBL_LEN		(WCuplow_II_LEN + WCuplow_TI_LEN + WCuplow_UT_LEN)
#define WCuplow_diff_TBL_LEN (2 * WCuplow_diffs)
/* #define WCcomb_TBL_LEN		(WCcomb_II_LEN + WCcomb_TI_LEN + WCcomb_UT_LEN) */
#endif

#undef __PASTE2
#define __PASTE2(A,B)		A ## B
#undef __PASTE3
#define __PASTE3(A,B,C)		A ## B ## C

#define COMMON_MMAP(X) \
	unsigned char	__PASTE3(lc_,X,_data)[__PASTE3(__lc_,X,_data_LEN)];

#define COMMON_MMIDX(X) \
	unsigned char	__PASTE3(lc_,X,_rows)[__PASTE3(__lc_,X,_rows_LEN)]; \
	uint16_t		__PASTE3(lc_,X,_item_offsets)[__PASTE3(__lc_,X,_item_offsets_LEN)]; \
	uint16_t		__PASTE3(lc_,X,_item_idx)[__PASTE3(__lc_,X,_item_idx_LEN)]; \


typedef struct {
	unsigned char magic[MAGIC_SIZE];

#ifdef __CTYPE_HAS_8_BIT_LOCALES
	const unsigned char tbl8ctype[Cctype_TBL_LEN];
    const unsigned char tbl8uplow[Cuplow_TBL_LEN];
#ifdef __WCHAR_ENABLED
	const uint16_t tbl8c2wc[Cc2wc_TBL_LEN]; /* char > 0x7f to wide char */
	const unsigned char tbl8wc2c[Cwc2c_TBL_LEN];
	/* translit  */
#endif /* __WCHAR_ENABLED */
#endif /* __CTYPE_HAS_8_BIT_LOCALES */
#ifdef __WCHAR_ENABLED
	const unsigned char tblwctype[WCctype_TBL_LEN];
	const unsigned char tblwuplow[WCuplow_TBL_LEN];
	const int16_t tblwuplow_diff[WCuplow_diff_TBL_LEN];
/* 	const unsigned char tblwcomb[WCcomb_TBL_LEN]; */
	/* width?? */
#endif /* __WCHAR_ENABLED */

	COMMON_MMAP(ctype);
	COMMON_MMAP(numeric);
	COMMON_MMAP(monetary);
	COMMON_MMAP(time);
	/* collate is different */
	COMMON_MMAP(messages);


#ifdef __CTYPE_HAS_8_BIT_LOCALES
	const codeset_8_bit_t codeset_8_bit[NUM_CODESETS];
#endif /* __CTYPE_HAS_8_BIT_LOCALES */

	COMMON_MMIDX(ctype);
	COMMON_MMIDX(numeric);
	COMMON_MMIDX(monetary);
	COMMON_MMIDX(time);
	/* collate is different */
	COMMON_MMIDX(messages);

	const uint16_t collate_data[__lc_collate_data_LEN];

	unsigned char lc_common_item_offsets_LEN[CATEGORIES];
    size_t lc_common_tbl_offsets[CATEGORIES * 4];
	/* offsets from start of locale_mmap_t */
	/* rows, item_offsets, item_idx, data */

#ifdef NUM_LOCALES
	unsigned char locales[NUM_LOCALES * WIDTH_LOCALES];
	unsigned char locale_names5[5*NUM_LOCALE_NAMES];
	unsigned char locale_at_modifiers[LOCALE_AT_MODIFIERS_LENGTH];
#endif /* NUM_LOCALES */

	unsigned char lc_names[lc_names_LEN];
#ifdef __CTYPE_HAS_8_BIT_LOCALES
	unsigned char codeset_list[sizeof(CODESET_LIST)]; /* TODO - fix */
#endif /* __CTYPE_HAS_8_BIT_LOCALES */


} __locale_mmap_t;

extern const __locale_mmap_t *__locale_mmap;
