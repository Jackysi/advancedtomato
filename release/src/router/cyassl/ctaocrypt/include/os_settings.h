/* os_settings.h
 *
 * Copyright (C) 2006-2010 Sawtooth Consulting Ltd.
 *
 * This file is part of CyaSSL.
 *
 * CyaSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * CyaSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

/* Place OS specific preprocessor flags, defines, includes here, will be
   included into every file because types.h includes it */

#ifndef CTAO_CRYPT_OS_SETTINGS_H
#define CTAO_CRYPT_OS_SETTINGS_H

#ifdef __cplusplus
    extern "C" {
#endif

/* Uncomment next line if using IPHONE */
/* #define IPHONE */

/* Uncomment next line if using ThreadX */
/* #define THREADX */

/* Uncomment next line if using Micrium ucOS */
/* #define MICRIUM */


#ifdef IPHONE
    #define SIZEOF_LONG_LONG 8
#endif

#ifdef THREADX 
    #define SIZEOF_LONG_LONG 8
#endif


#ifdef MICRIUM

    #include "net_cfg.h"
    #include "cyassl_cfg.h"
    #include "net_secure_os.h"

    #define CYASSL_TYPES

    typedef CPU_INT08U byte;
    typedef CPU_INT16U word16;
    typedef CPU_INT32U word32;

    #if (NET_SECURE_MGR_CFG_WORD_SIZE == CPU_WORD_SIZE_32)
        #define SIZEOF_LONG        8
        #undef  SIZEOF_LONG_LONG
    #else
        #undef  SIZEOF_LONG
        #define SIZEOF_LONG_LONG   8
    #endif

    #define STRING_USER

    #define XSTRLEN(pstr) ((CPU_SIZE_T)Str_Len((CPU_CHAR *)(pstr)))
    #define XSTRNCPY(pstr_dest, pstr_src, len_max) \
                    ((CPU_CHAR *)Str_Copy_N((CPU_CHAR *)(pstr_dest), \
                     (CPU_CHAR *)(pstr_src), (CPU_SIZE_T)(len_max)))
    #define XSTRNCMP(pstr_1, pstr_2, len_max) \
                    ((CPU_INT16S)Str_Cmp_N((CPU_CHAR *)(pstr_1), \
                     (CPU_CHAR *)(pstr_2), (CPU_SIZE_T)(len_max)))  
    #define XSTRSTR(pstr, pstr_srch) \
                    ((CPU_CHAR *)Str_Str((CPU_CHAR *)(pstr), \
                     (CPU_CHAR *)(pstr_srch)))
    #define XMEMSET(pmem, data_val, size) \
                    ((void)Mem_Set((void *)(pmem), (CPU_INT08U) (data_val), \
                    (CPU_SIZE_T)(size)))
    #define XMEMCPY(pdest, psrc, size) ((void)Mem_Copy((void *)(pdest), \
                     (void *)(psrc), (CPU_SIZE_T)(size)))
    #define XMEMCMP(pmem_1, pmem_2, size) \
                   (((CPU_BOOLEAN)Mem_Cmp((void *)(pmem_1), (void *)(pmem_2), \
                     (CPU_SIZE_T)(size))) ? DEF_NO : DEF_YES)

    #if (NET_SECURE_MGR_CFG_EN == DEF_ENABLED)
        #define MICRIUM_MALLOC
        #define XMALLOC(s, h, type) (((type) <= DYNAMIC_TYPE_SIGNER) ? \
                                 ((void *)NetSecure_Malloc((CPU_INT08U)(type), \
                                 (CPU_SIZE_T)(s))) : malloc(s))
        #define XFREE(p, h, type)   (((type) <= DYNAMIC_TYPE_SIGNER) ? \
                          (NetSecure_Free((CPU_INT08U)(type), (p))) : free((p)))
        #define XREALLOC(p, n, h, t) realloc((p), (n))
    #endif

    #if (NET_SECURE_MGR_CFG_FS_EN == DEF_ENABLED)
        #undef  NO_FILESYSTEM
    #else
        #define NO_FILESYSTEM
    #endif

    #if (CYASSL_CFG_TRACE_LEVEL == CYASSL_TRACE_LEVEL_DBG)
        #define DEBUG_CYASSL
    #else
        #undef  DEBUG_CYASSL
    #endif

    #if (CYASSL_CFG_OPENSSL_EN == DEF_ENABLED)
        #define OPENSSL_EXTRA
    #else
        #undef  OPENSSL_EXTRA
    #endif

    #if (CYASSL_CFG_MULTI_THREAD_EN == DEF_ENABLED)
        #undef  SINGLE_THREADED
    #else
        #define SINGLE_THREADED
    #endif

    #if (CYASSL_CFG_DH_EN == DEF_ENABLED)
        #undef  NO_DH
    #else
        #define NO_DH
    #endif

    #if (CYASSL_CFG_DSA_EN == DEF_ENABLED)
        #undef  NO_DSA
    #else
        #define NO_DSA
    #endif

    #if (CYASSL_CFG_PSK_EN == DEF_ENABLED)
        #undef  NO_PSK
    #else
        #define NO_PSK
    #endif

    #if (CYASSL_CFG_3DES_EN == DEF_ENABLED)
        #undef  NO_DES
    #else
        #define NO_DES
    #endif

    #if (CYASSL_CFG_AES_EN == DEF_ENABLED)
        #undef  NO_AES
    #else
        #define NO_AES
    #endif

    #if (CYASSL_CFG_RC4_EN == DEF_ENABLED)
        #undef  NO_RC4
    #else
        #define NO_RC4
    #endif

    #if (CYASSL_CFG_RABBIT_EN == DEF_ENABLED)
        #undef  NO_RABBIT
    #else
        #define NO_RABBIT
    #endif

    #if (CYASSL_CFG_HC128_EN == DEF_ENABLED)
        #undef  NO_HC128
    #else
        #define NO_HC128
    #endif

    #if (CPU_CFG_ENDIAN_TYPE == CPU_ENDIAN_TYPE_BIG)
        #define BIG_ENDIAN_ORDER
    #else
        #undef  BIG_ENDIAN_ORDER
        #define LITTLE_ENDIAN_ORDER
    #endif


    #define  NO_MD4
    #define  NO_WRITEV
    #define  NO_DEV_RANDOM
    #define  CYASSL_USER_IO
    #define  LARGE_STATIC_BUFFERS
    #define  CYASSL_DER_LOAD
    #undef   CYASSL_DTLS

#endif /* MICRIUM */

/* Place any other flags or defines here */


#ifdef __cplusplus
    }   /* extern "C" */
#endif


#endif /* CTAO_CRYPT_OS_SETTINGS_H */

