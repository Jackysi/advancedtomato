#ifndef PWCX_H
#define PWCX_H

#include <linux/linkage.h>
#include "pwc-ioctl.h"

#ifdef __cplusplus
extern "C"{
#endif

/** functions **/
extern asmlinkage void pwcx_init_decompress_Nala(int type, int release, void *mode, void *table);
extern asmlinkage void pwcx_exit_decompress_Nala(void);
extern asmlinkage void pwcx_decompress_Nala(struct pwc_coord *image, struct pwc_coord *view, struct pwc_coord *offset, void *src, void *dst, int flags, void *table, int bandlength);

extern asmlinkage void pwcx_init_decompress_Timon(int type, int release, void *mode, void *table);
extern asmlinkage void pwcx_exit_decompress_Timon(void);
extern asmlinkage void pwcx_decompress_Timon(struct pwc_coord *image, struct pwc_coord *view, struct pwc_coord *offset, void *src, void *dst, int flags, void *table, int bandlength);

extern asmlinkage void pwcx_init_decompress_Kiara(int type, int release, void *mode, void *table);
extern asmlinkage void pwcx_exit_decompress_Kiara(void);
extern asmlinkage void pwcx_decompress_Kiara(struct pwc_coord *image, struct pwc_coord *view, struct pwc_coord *offset, void *src, void *dst, int flags, void *table, int bandlength);

#ifdef __cplusplus
}
#endif

#endif
