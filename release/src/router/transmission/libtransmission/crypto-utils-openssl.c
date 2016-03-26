/*
 * This file Copyright (C) 2007-2014 Mnemosyne LLC
 *
 * It may be used under the GNU GPL versions 2 or 3
 * or any future license endorsed by Mnemosyne LLC.
 *
 * $Id: crypto-utils-openssl.c 14640 2015-12-28 23:53:55Z mikedld $
 */

#ifdef __APPLE__
 /* OpenSSL "deprecated" as of OS X 10.7, but we still use it */
 #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <assert.h>

#include <openssl/bn.h>
#include <openssl/dh.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/opensslv.h>

#include "transmission.h"
#include "crypto-utils.h"
#include "log.h"
#include "utils.h"

#define TR_CRYPTO_DH_SECRET_FALLBACK
#include "crypto-utils-fallback.c"

/***
****
***/

#define MY_NAME "tr_crypto_utils"

static void
log_openssl_error (const char * file,
                   int          line)
{
  const unsigned long error_code = ERR_get_error ();

  if (tr_logLevelIsActive (TR_LOG_ERROR))
    {
      char buf[512];

#ifndef TR_LIGHTWEIGHT
      static bool strings_loaded = false;
      if (!strings_loaded)
        {
          ERR_load_crypto_strings ();
          strings_loaded = true;
        }
#endif

      ERR_error_string_n (error_code, buf, sizeof (buf));
      tr_logAddMessage (file, line, TR_LOG_ERROR, MY_NAME, "OpenSSL error: %s", buf);
    }
}

#define log_error() log_openssl_error(__FILE__, __LINE__)

static bool
check_openssl_result (int          result,
                      int          expected_result,
                      bool         expected_equal,
                      const char * file,
                      int          line)
{
  const bool ret = (result == expected_result) == expected_equal;
  if (!ret)
    log_openssl_error (file, line);
  return ret;
}

#define check_result(result) check_openssl_result ((result), 1, true, __FILE__, __LINE__)
#define check_result_eq(result, x_result) check_openssl_result ((result), (x_result), true, __FILE__, __LINE__)
#define check_result_neq(result, x_result) check_openssl_result ((result), (x_result), false, __FILE__, __LINE__)

static bool
check_openssl_pointer (void       * pointer,
                       const char * file,
                       int          line)
{
  const bool ret = pointer != NULL;
  if (!ret)
    log_openssl_error (file, line);
  return ret;
}

#define check_pointer(pointer) check_openssl_pointer ((pointer), __FILE__, __LINE__)

/***
****
***/

tr_sha1_ctx_t
tr_sha1_init (void)
{
  EVP_MD_CTX * handle = EVP_MD_CTX_create ();

  if (check_result (EVP_DigestInit_ex (handle, EVP_sha1 (), NULL)))
    return handle;

  EVP_MD_CTX_destroy (handle);
  return NULL;
}

bool
tr_sha1_update (tr_sha1_ctx_t   handle,
                const void    * data,
                size_t          data_length)
{
  assert (handle != NULL);

  if (data_length == 0)
    return true;

  assert (data != NULL);

  return check_result (EVP_DigestUpdate (handle, data, data_length));
}

bool
tr_sha1_final (tr_sha1_ctx_t   handle,
               uint8_t       * hash)
{
  bool ret = true;

  if (hash != NULL)
    {
      unsigned int hash_length;

      assert (handle != NULL);

      ret = check_result (EVP_DigestFinal_ex (handle, hash, &hash_length));

      assert (!ret || hash_length == SHA_DIGEST_LENGTH);
    }

  EVP_MD_CTX_destroy (handle);
  return ret;
}

/***
****
***/

#if OPENSSL_VERSION_NUMBER < 0x0090802fL

static EVP_CIPHER_CTX *
openssl_evp_cipher_context_new (void)
{
  EVP_CIPHER_CTX * handle = tr_new (EVP_CIPHER_CTX, 1);
  if (handle != NULL)
    EVP_CIPHER_CTX_init (handle);
  return handle;
}

static void
openssl_evp_cipher_context_free (EVP_CIPHER_CTX * handle)
{
  if (handle == NULL)
    return;

  EVP_CIPHER_CTX_cleanup (handle);
  tr_free (handle);
}

#define EVP_CIPHER_CTX_new() openssl_evp_cipher_context_new ()
#define EVP_CIPHER_CTX_free(x) openssl_evp_cipher_context_free ((x))

#endif

tr_rc4_ctx_t
tr_rc4_new (void)
{
  EVP_CIPHER_CTX * handle = EVP_CIPHER_CTX_new ();

  if (check_result (EVP_CipherInit_ex (handle, EVP_rc4 (), NULL, NULL, NULL, -1)))
    return handle;

  EVP_CIPHER_CTX_free (handle);
  return NULL;
}

void
tr_rc4_free (tr_rc4_ctx_t handle)
{
  if (handle == NULL)
    return;

  EVP_CIPHER_CTX_free (handle);
}

void
tr_rc4_set_key (tr_rc4_ctx_t    handle,
                const uint8_t * key,
                size_t          key_length)
{
  assert (handle != NULL);
  assert (key != NULL);

  if (!check_result (EVP_CIPHER_CTX_set_key_length (handle, key_length)))
    return;
  check_result (EVP_CipherInit_ex (handle, NULL, NULL, key, NULL, -1));
}

void
tr_rc4_process (tr_rc4_ctx_t   handle,
                const void   * input,
                void         * output,
                size_t         length)
{
  int output_length;

  assert (handle != NULL);

  if (length == 0)
    return;

  assert (input != NULL);
  assert (output != NULL);

  check_result (EVP_CipherUpdate (handle, output, &output_length, input, length));
}

/***
****
***/

tr_dh_ctx_t
tr_dh_new (const uint8_t * prime_num,
           size_t          prime_num_length,
           const uint8_t * generator_num,
           size_t          generator_num_length)
{
  DH * handle = DH_new ();

  assert (prime_num != NULL);
  assert (generator_num != NULL);

  if (!check_pointer (handle->p = BN_bin2bn (prime_num, prime_num_length, NULL)) ||
      !check_pointer (handle->g = BN_bin2bn (generator_num, generator_num_length, NULL)))
    {
      DH_free (handle);
      handle = NULL;
    }

  return handle;
}

void
tr_dh_free (tr_dh_ctx_t handle)
{
  if (handle == NULL)
    return;

  DH_free (handle);
}

bool
tr_dh_make_key (tr_dh_ctx_t   raw_handle,
                size_t        private_key_length,
                uint8_t     * public_key,
                size_t      * public_key_length)
{
  DH * handle = raw_handle;
  int dh_size, my_public_key_length;

  assert (handle != NULL);
  assert (public_key != NULL);

  handle->length = private_key_length * 8;

  if (!check_result (DH_generate_key (handle)))
    return false;

  my_public_key_length = BN_bn2bin (handle->pub_key, public_key);
  dh_size = DH_size (handle);

  tr_dh_align_key (public_key, my_public_key_length, dh_size);

  if (public_key_length != NULL)
    *public_key_length = dh_size;

  return true;
}

tr_dh_secret_t
tr_dh_agree (tr_dh_ctx_t     handle,
             const uint8_t * other_public_key,
             size_t          other_public_key_length)
{
  struct tr_dh_secret * ret;
  int dh_size, secret_key_length;
  BIGNUM * other_key;

  assert (handle != NULL);
  assert (other_public_key != NULL);

  if (!check_pointer (other_key = BN_bin2bn (other_public_key, other_public_key_length, NULL)))
    return NULL;

  dh_size = DH_size (handle);
  ret = tr_dh_secret_new (dh_size);

  secret_key_length = DH_compute_key (ret->key, other_key, handle);
  if (check_result_neq (secret_key_length, -1))
    {
      tr_dh_secret_align (ret, secret_key_length);
    }
  else
    {
      tr_dh_secret_free (ret);
      ret = NULL;
    }

  BN_free (other_key);
  return ret;
}

/***
****
***/

bool
tr_rand_buffer (void   * buffer,
                size_t   length)
{
  assert (buffer != NULL);

  return check_result (RAND_bytes (buffer, (int) length));
}
