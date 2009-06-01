/* ====================================================================
 * Copyright (c) 2001 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.openssl.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    openssl-core@openssl.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.openssl.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 */

#include <openssl/opensslconf.h>
#ifndef OPENSSL_NO_AES
#include <openssl/evp.h>
#include <openssl/err.h>
#include <string.h>
#include <assert.h>
#include <openssl/aes.h>

/* Macros to code block cipher wrappers */

/* Wrapper functions for each cipher mode */

#define BLOCK_CIPHER_ecb_loop() \
	unsigned int i, bl; \
	bl = ctx->cipher->block_size;\
	if(inl < bl) return 1;\
	inl -= bl; \
	for(i=0; i <= inl; i+=bl) 

#define BLOCK_CIPHER_func_ecb(cname)                  \
static int cname##_ecb_cipher(EVP_CIPHER_CTX *ctx, unsigned char *out, const unsigned char *in, unsigned int inl) \
{\
	BLOCK_CIPHER_ecb_loop() \
            AES_ecb_encrypt(in + i, out + i, &((EVP_AES_KEY *)&(ctx->c.aes_ks))->ks, ctx->encrypt); \
	return 1;\
}


#define BLOCK_CIPHER_func_cbc(cname) \
static int cname##_cbc_cipher(EVP_CIPHER_CTX *ctx, unsigned char *out, const unsigned char *in, unsigned int inl) \
{\
   AES_cbc_encrypt(in, out, (long)inl, &((EVP_AES_KEY *)&(ctx->c.aes_ks))->ks, ctx->iv, ctx->encrypt); \
   return 1;\
}


#define BLOCK_CIPHER_func_ofb(cname) \
static int cname##_ofb_cipher(EVP_CIPHER_CTX *ctx, unsigned char *out, const unsigned char *in, unsigned int inl) \
{\
   AES_ofb128_encrypt(in, out, (long)inl, &((EVP_AES_KEY *)&(ctx->c.aes_ks))->ks, ctx->iv, &ctx->num);\
   return 1;\
}

#define BLOCK_CIPHER_func_cfb(cname) \
static int cname##_cfb_cipher(EVP_CIPHER_CTX *ctx, unsigned char *out, const unsigned char *in, unsigned int inl) \
{\
   AES_cfb128_encrypt(in, out, (long)inl, &((EVP_AES_KEY *)&(ctx->c.aes_ks))->ks, ctx->iv, &ctx->num, ctx->encrypt);\
   return 1;\
}


#define BLOCK_CIPHER_def1(cname, nmode, mode, MODE, nid, key_len, block_size)      \
static const EVP_CIPHER cname##_##mode = { \
        nid##_##nmode, block_size, key_len, 16,      \
	EVP_CIPH_##MODE##_MODE, \
        aes_init_key, \
	cname##_##mode##_cipher, \
	NULL, \
	sizeof(EVP_CIPHER_CTX) - sizeof((((EVP_CIPHER_CTX *)NULL)->c)) + sizeof(EVP_AES_KEY), \
        EVP_CIPHER_set_asn1_iv, EVP_CIPHER_get_asn1_iv,  \
	NULL,              \
	NULL \
}; \
const EVP_CIPHER *EVP_##cname##_##mode(void) { return &cname##_##mode; }


#define BLOCK_CIPHER_def_ecb(cname, nid, key_len) \
    BLOCK_CIPHER_def1(cname, ecb, ecb, ECB, nid, key_len)

#define IMPLEMENT_BLOCK_CIPHER(bits)    \
        BLOCK_CIPHER_func_cbc(aes_##bits) \
        BLOCK_CIPHER_func_ecb(aes_##bits) \
        BLOCK_CIPHER_func_ofb(aes_##bits) \
        BLOCK_CIPHER_func_cfb(aes_##bits) \
        BLOCK_CIPHER_def1(aes_##bits, cbc,    cbc, CBC, NID_aes_##bits, (bits/8), 16) \
        BLOCK_CIPHER_def1(aes_##bits, ecb,    ecb, ECB, NID_aes_##bits, (bits/8), 16) \
        BLOCK_CIPHER_def1(aes_##bits, ofb128, ofb, OFB, NID_aes_##bits, (bits/8), 1)  \
        BLOCK_CIPHER_def1(aes_##bits, cfb128, cfb, CFB, NID_aes_##bits, (bits/8), 1) 


#define EVP_C_DATA(ctx)	((EVP_AES_KEY *)(ctx)->c.aes_ks)

static int aes_init_key(EVP_CIPHER_CTX *ctx, const unsigned char *key,
					const unsigned char *iv, int enc);

typedef struct
	{
	AES_KEY ks;
	} EVP_AES_KEY;

#define data(ctx)	EVP_C_DATA(EVP_AES_KEY,ctx)


IMPLEMENT_BLOCK_CIPHER(128)
IMPLEMENT_BLOCK_CIPHER(192)
IMPLEMENT_BLOCK_CIPHER(256)


static int aes_init_key(EVP_CIPHER_CTX *ctx, const unsigned char *key,
		   const unsigned char *iv, int enc)
	{
	int ret;

	if ((ctx->cipher->flags & EVP_CIPH_MODE) == EVP_CIPH_CFB_MODE
	    || (ctx->cipher->flags & EVP_CIPH_MODE) == EVP_CIPH_OFB_MODE
	    || enc) 
                ret=AES_set_encrypt_key(key, ctx->key_len * 8, &(ctx->c.aes_ks));
	else
		ret=AES_set_decrypt_key(key, ctx->key_len * 8, &(ctx->c.aes_ks));

	if(ret < 0)
		{
		EVPerr(EVP_F_AES_INIT_KEY,EVP_R_AES_KEY_SETUP_FAILED);
		return 0;
		}

	return 1;
	}

#endif
