/* crypto/aes/aes_cbc.c -*- mode:C; c-file-style: "eay" -*- */
/* ====================================================================
 * Copyright (c) 1998-2002 The OpenSSL Project.  All rights reserved.
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

#ifndef AES_DEBUG
# ifndef NDEBUG
#  define NDEBUG
# endif
#endif
#include <assert.h>

#include <openssl/aes.h>
#include "aes_locl.h"


#ifdef mips

#define GET16(a, b, c, d, out)            \
       __asm__ __volatile__(              \
            "ulw %0,0(%4); ulw %1, 4(%4); ulw %2 ,8(%4); ulw %3, 12(%4); " \
            : "=&r"(a), "=&r"(b), "=&r"(c), "=&r"(d) \
            : "r"(out))


#define SET16(a, b, c, d, out)  \
       __asm__ __volatile__(    \
            "usw %0,0(%4); usw %1, 4(%4); usw %2 ,8(%4); usw %3, 12(%4); " \
            : : "r"(a), "r"(b), "r"(c), "r"(d), "r"(out) : "memory")


#define MEMCOPY16(out, in)  do { int a, b, c, d;  GET16(a,b,c,d, in); SET16(a,b,c,d,out); } while(0)

#define XOR_UNALIGNED_16(out, in1, in2) do {                                 \
        int a0, a1, a2, a3, b0, b1, b2, b3;  \
        GET16(a0, a1, a2, a3, in1);          \
        GET16(b0, b1, b2, b3, in2);          \
        SET16(a0 ^ b0, a1 ^ b1, a2 ^ b2, a3 ^ b3, out); } while (0)


#else 

#define XOR_UNALIGNED_16(out, in1, in2) {                                \
        for(n=0; n < AES_BLOCK_SIZE; ++n) out[n] = in1[n] ^ in2[n]; }

#define MEMCOPY16(out, in) memcpy(out, in, AES_BLOCK_SIZE)


#endif


void AES_cbc_encrypt(const unsigned char *in, unsigned char *out,
		     const unsigned long length, const AES_KEY *key,
		     unsigned char *ivec, const int enc) {
	unsigned long n;
	unsigned long len = length;
	unsigned char tmp[AES_BLOCK_SIZE];
	const unsigned char *iv = ivec;

	assert(in && out && key && ivec);
	assert((AES_ENCRYPT == enc)||(AES_DECRYPT == enc));

	if (AES_ENCRYPT == enc) {
		while (len >= AES_BLOCK_SIZE) {
                        XOR_UNALIGNED_16(out, in, iv);
			AES_encrypt(out, out, key);
			iv = out;
			len -= AES_BLOCK_SIZE;
			in += AES_BLOCK_SIZE;
			out += AES_BLOCK_SIZE;
		}
		if (len) {
			for(n=0; n < len; ++n)
				out[n] = in[n] ^ iv[n];
			for(n=len; n < AES_BLOCK_SIZE; ++n)
				out[n] = iv[n];
			AES_encrypt(out, out, key);
			iv = out;
		}
                MEMCOPY16(ivec, iv);
	} else if (in != out) {
		while (len >= AES_BLOCK_SIZE) {
			AES_decrypt(in, out, key);
                        XOR_UNALIGNED_16(out, out, iv);
			iv = in;
			len -= AES_BLOCK_SIZE;
			in  += AES_BLOCK_SIZE;
			out += AES_BLOCK_SIZE;
		}
		if (len) {
			AES_decrypt(in,tmp,key);
			for(n=0; n < len; ++n)
				out[n] = tmp[n] ^ iv[n];
			iv = in;
		}
                MEMCOPY16(ivec, iv);
	} else {
		while (len >= AES_BLOCK_SIZE) {
                        MEMCOPY16(tmp, in);
			AES_decrypt(in, out, key);
                        XOR_UNALIGNED_16(out, out, ivec);
                        MEMCOPY16(ivec, tmp);
			len -= AES_BLOCK_SIZE;
			in += AES_BLOCK_SIZE;
			out += AES_BLOCK_SIZE;
		}
		if (len) {
                        MEMCOPY16(tmp, in);
			AES_decrypt(tmp, out, key);
			for(n=0; n < len; ++n)
				out[n] ^= ivec[n];
			for(n=len; n < AES_BLOCK_SIZE; ++n)
				out[n] = tmp[n];
                        MEMCOPY16(ivec, tmp);
		}
	}
}
