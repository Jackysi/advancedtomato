/*
 * This file Copyright (C) 2007-2014 Mnemosyne LLC
 *
 * It may be used under the GNU GPL versions 2 or 3
 * or any future license endorsed by Mnemosyne LLC.
 *
 * $Id: crypto-utils.h 14422 2015-01-01 21:16:36Z mikedld $
 */

#ifndef TR_CRYPTO_UTILS_H
#define TR_CRYPTO_UTILS_H

#include <inttypes.h>
#include <stddef.h>

#include "transmission.h" /* SHA_DIGEST_LENGTH */
#include "utils.h" /* TR_GNUC_MALLOC, TR_GNUC_NULL_TERMINATED */

#ifdef __cplusplus
extern "C" {
#endif

/**
*** @addtogroup utils Utilities
*** @{
**/

 /** @brief Opaque SHA1 context type. */
typedef void * tr_sha1_ctx_t;
 /** @brief Opaque RC4 context type. */
typedef void * tr_rc4_ctx_t;
 /** @brief Opaque DH context type. */
typedef void * tr_dh_ctx_t;
 /** @brief Opaque DH secret key type. */
typedef void * tr_dh_secret_t;

/**
 * @brief Generate a SHA1 hash from one or more chunks of memory.
 */
bool             tr_sha1               (uint8_t        * hash,
                                        const void     * data1,
                                        int              data1_length,
                                                         ...) TR_GNUC_NULL_TERMINATED;

/**
 * @brief Allocate and initialize new SHA1 hasher context.
 */
tr_sha1_ctx_t    tr_sha1_init          (void);

/**
 * @brief Update SHA1 hash.
 */
bool             tr_sha1_update        (tr_sha1_ctx_t    handle,
                                        const void     * data,
                                        size_t           data_length);

/**
 * @brief Finalize and export SHA1 hash, free hasher context.
 */
bool             tr_sha1_final         (tr_sha1_ctx_t    handle,
                                        uint8_t        * hash);

/**
 * @brief Allocate and initialize new RC4 cipher context.
 */
tr_rc4_ctx_t     tr_rc4_new            (void);

/**
 * @brief Free RC4 cipher context.
 */
void             tr_rc4_free           (tr_rc4_ctx_t     handle);

/**
 * @brief Set RC4 cipher key.
 */
void             tr_rc4_set_key        (tr_rc4_ctx_t     handle,
                                        const uint8_t  * key,
                                        size_t           key_length);

/**
 * @brief Process memory block with RC4 cipher.
 */
void             tr_rc4_process        (tr_rc4_ctx_t     handle,
                                        const void     * input,
                                        void           * output,
                                        size_t           length);

/**
 * @brief Allocate and initialize new Diffie-Hellman (DH) key exchange context.
 */
tr_dh_ctx_t      tr_dh_new             (const uint8_t  * prime_num,
                                        size_t           prime_num_length,
                                        const uint8_t  * generator_num,
                                        size_t           generator_num_length);

/**
 * @brief Free DH key exchange context.
 */
void             tr_dh_free            (tr_dh_ctx_t      handle);

/**
 * @brief Generate private and public DH keys, export public key.
 */
bool             tr_dh_make_key        (tr_dh_ctx_t      handle,
                                        size_t           private_key_length,
                                        uint8_t        * public_key,
                                        size_t         * public_key_length);

/**
 * @brief Perform DH key exchange, generate secret key.
 */
tr_dh_secret_t   tr_dh_agree           (tr_dh_ctx_t      handle,
                                        const uint8_t  * other_public_key,
                                        size_t           other_public_key_length);

/**
 * @brief Calculate SHA1 hash of DH secret key, prepending and/or appending
 *        given data to the key during calculation.
 */
bool             tr_dh_secret_derive   (tr_dh_secret_t   handle,
                                        const void     * prepend_data,
                                        size_t           prepend_data_size,
                                        const void     * append_data,
                                        size_t           append_data_size,
                                        uint8_t        * hash);

/**
 * @brief Free DH secret key returned by @ref tr_dh_agree.
 */
void             tr_dh_secret_free     (tr_dh_secret_t   handle);

/**
 * @brief Align DH key (big-endian number) to required length (internal, do not use).
 */
void             tr_dh_align_key       (uint8_t        * key_buffer,
                                        size_t           key_size,
                                        size_t           buffer_size);

/**
 * @brief Returns a random number in the range of [0...upper_bound).
 */
int              tr_rand_int           (int              upper_bound);

/**
 * @brief Returns a pseudorandom number in the range of [0...upper_bound).
 *
 * This is faster, BUT WEAKER, than tr_rand_int () and never be used in sensitive cases.
 * @see tr_rand_int ()
 */
int              tr_rand_int_weak      (int              upper_bound);

/**
 * @brief Fill a buffer with random bytes.
 */
bool             tr_rand_buffer        (void           * buffer,
                                        size_t           length);

/**
 * @brief Generate a SSHA password from its plaintext source.
 */
char           * tr_ssha1              (const char     * plain_text) TR_GNUC_MALLOC;

/**
 * @brief Validate a test password against the a ssha1 password.
 */
bool             tr_ssha1_matches      (const char     * ssha1,
                                        const char     * plain_text);

/**
 * @brief Translate a block of bytes into base64.
 * @return a newly-allocated null-terminated string that can be freed with tr_free ()
 */
void           * tr_base64_encode      (const void     * input,
                                        size_t           input_length,
                                        size_t         * output_length) TR_GNUC_MALLOC;

/**
 * @brief Translate null-terminated string into base64.
 * @return a newly-allocated null-terminated string that can be freed with tr_free ()
 */
void           * tr_base64_encode_str  (const char     * input,
                                        size_t         * output_length) TR_GNUC_MALLOC;

/**
 * @brief Translate a block of bytes from base64 into raw form.
 * @return a newly-allocated null-terminated string that can be freed with tr_free ()
 */
void           * tr_base64_decode      (const void     * input,
                                        size_t           input_length,
                                        size_t         * output_length) TR_GNUC_MALLOC;

/**
 * @brief Translate null-terminated string from base64 into raw form.
 * @return a newly-allocated null-terminated string that can be freed with tr_free ()
 */
void           * tr_base64_decode_str  (const char     * input,
                                        size_t         * output_length) TR_GNUC_MALLOC;

/**
 * @brief Wrapper around tr_binary_to_hex () for SHA_DIGEST_LENGTH.
 */
static inline void
tr_sha1_to_hex (char          * hex,
                const uint8_t * sha1)
{
  tr_binary_to_hex (sha1, hex, SHA_DIGEST_LENGTH);
}

/**
 * @brief Wrapper around tr_hex_to_binary () for SHA_DIGEST_LENGTH.
 */
static inline void
tr_hex_to_sha1 (uint8_t    * sha1,
                const char * hex)
{
  tr_hex_to_binary (hex, sha1, SHA_DIGEST_LENGTH);
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* TR_CRYPTO_UTILS_H */
