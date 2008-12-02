#define RDEP(x)	(1UL<<(x))

#ifdef CONFIG_ITANIUM
#error "This file should not be used when CONFIG_ITANIUM is defined"
#endif

static pfm_reg_desc_t pmc_desc[256]={
/* pmc0  */ { PFM_REG_CONTROL, 0, NULL, NULL, {0UL,0UL, 0UL, 0UL}, {0UL,0UL, 0UL, 0UL}},
/* pmc1  */ { PFM_REG_CONTROL, 0, NULL, NULL, {0UL,0UL, 0UL, 0UL}, {0UL,0UL, 0UL, 0UL}},
/* pmc2  */ { PFM_REG_CONTROL, 0, NULL, NULL, {0UL,0UL, 0UL, 0UL}, {0UL,0UL, 0UL, 0UL}},
/* pmc3  */ { PFM_REG_CONTROL, 0, NULL, NULL, {0UL,0UL, 0UL, 0UL}, {0UL,0UL, 0UL, 0UL}},
/* pmc4  */ { PFM_REG_COUNTING, 0, NULL, NULL, {RDEP(4),0UL, 0UL, 0UL}, {0UL,0UL, 0UL, 0UL}},
/* pmc5  */ { PFM_REG_COUNTING, 0, NULL, NULL, {RDEP(5),0UL, 0UL, 0UL}, {0UL,0UL, 0UL, 0UL}},
/* pmc6  */ { PFM_REG_COUNTING, 0, NULL, NULL, {RDEP(6),0UL, 0UL, 0UL}, {0UL,0UL, 0UL, 0UL}},
/* pmc7  */ { PFM_REG_COUNTING, 0, NULL, NULL, {RDEP(7),0UL, 0UL, 0UL}, {0UL,0UL, 0UL, 0UL}},
	    { PFM_REG_NONE, 0, NULL, NULL, {0,}, {0,}}, /* end marker */
};

static pfm_reg_desc_t pmd_desc[256]={
/* pmd0  */ { PFM_REG_NOTIMPL, 0, NULL, NULL, {0,}, {0,}},
/* pmd1  */ { PFM_REG_NOTIMPL, 0, NULL, NULL, {0,}, {0,}},
/* pmd2  */ { PFM_REG_NOTIMPL, 0, NULL, NULL, {0,}, {0,}},
/* pmd3  */ { PFM_REG_NOTIMPL, 0, NULL, NULL, {0,}, {0,}},
/* pmd4  */ { PFM_REG_COUNTING, 0, NULL, NULL, {0UL,0UL, 0UL, 0UL}, {RDEP(4),0UL, 0UL, 0UL}},
/* pmd5  */ { PFM_REG_COUNTING, 0, NULL, NULL, {0UL,0UL, 0UL, 0UL}, {RDEP(5),0UL, 0UL, 0UL}},
/* pmd6  */ { PFM_REG_COUNTING, 0, NULL, NULL, {0UL,0UL, 0UL, 0UL}, {RDEP(6),0UL, 0UL, 0UL}},
/* pmd7  */ { PFM_REG_COUNTING, 0, NULL, NULL, {0UL,0UL, 0UL, 0UL}, {RDEP(7),0UL, 0UL, 0UL}},
	    { PFM_REG_NONE, 0, NULL, NULL, {0,}, {0,}}, /* end marker */
};
