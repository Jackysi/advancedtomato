

int  bnep_crc32_init(void);
void bnep_crc32_cleanup(void);
u32  bnep_crc32(u32 crc, unsigned char const *p, size_t len);
