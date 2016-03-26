/*
 * This file Copyright (C) 2007-2014 Mnemosyne LLC
 *
 * It may be used under the GNU GPL versions 2 or 3
 * or any future license endorsed by Mnemosyne LLC.
 *
 * $Id: crypto-utils.c 14618 2015-12-13 01:29:39Z mikedld $
 */

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h> /* abs (), srand (), rand () */
#include <string.h> /* memcpy (), memmove (), memset (), strcmp (), strlen () */

#include <b64/cdecode.h>
#include <b64/cencode.h>

#include "transmission.h"
#include "crypto-utils.h"
#include "utils.h"

/***
****
***/

void
tr_dh_align_key (uint8_t * key_buffer,
                 size_t    key_size,
                 size_t    buffer_size)
{
  assert (key_size <= buffer_size);

  /* DH can generate key sizes that are smaller than the size of
     key buffer with exponentially decreasing probability, in which case
     the msb's of key buffer need to be zeroed appropriately. */
  if (key_size < buffer_size)
    {
      const size_t offset = buffer_size - key_size;
      memmove (key_buffer + offset, key_buffer, key_size);
      memset (key_buffer, 0, offset);
    }
}

/***
****
***/

bool
tr_sha1 (uint8_t    * hash,
         const void * data1,
         int          data1_length,
                      ...)
{
  tr_sha1_ctx_t sha;

  if ((sha = tr_sha1_init ()) == NULL)
    return false;

  if (tr_sha1_update (sha, data1, data1_length))
    {
      va_list vl;
      const void * data;

      va_start (vl, data1_length);
      while ((data = va_arg (vl, const void *)) != NULL)
        {
          const int data_length = va_arg (vl, int);
          assert (data_length >= 0);
          if (!tr_sha1_update (sha, data, data_length))
            break;
        }
      va_end (vl);

      /* did we reach the end of argument list? */
      if (data == NULL)
        return tr_sha1_final (sha, hash);
    }

  tr_sha1_final (sha, NULL);
  return false;
}

/***
****
***/

int
tr_rand_int (int upper_bound)
{
  int noise;

  assert (upper_bound > 0);

  while (tr_rand_buffer (&noise, sizeof (noise)))
    {
      noise = abs(noise) % upper_bound;
      /* abs(INT_MIN) is undefined and could return negative value */
      if (noise >= 0)
        return noise;
    }

  /* fall back to a weaker implementation... */
  return tr_rand_int_weak (upper_bound);
}

int
tr_rand_int_weak (int upper_bound)
{
  static bool init = false;

  assert (upper_bound > 0);

  if (!init)
    {
      srand (tr_time_msec ());
      init = true;
    }

  return rand () % upper_bound;
}

/***
****
***/

char *
tr_ssha1 (const char * plain_text)
{
  enum { saltval_len = 8,
         salter_len  = 64 };
  static const char * salter = "0123456789"
                               "abcdefghijklmnopqrstuvwxyz"
                               "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                               "./";

  size_t i;
  unsigned char salt[saltval_len];
  uint8_t sha[SHA_DIGEST_LENGTH];
  char buf[2 * SHA_DIGEST_LENGTH + saltval_len + 2];

  tr_rand_buffer (salt, saltval_len);
  for (i = 0; i < saltval_len; ++i)
    salt[i] = salter[salt[i] % salter_len];

  tr_sha1 (sha, plain_text, strlen (plain_text), salt, (size_t) saltval_len, NULL);
  tr_sha1_to_hex (&buf[1], sha);
  memcpy (&buf[1 + 2 * SHA_DIGEST_LENGTH], &salt, saltval_len);
  buf[1 + 2 * SHA_DIGEST_LENGTH + saltval_len] = '\0';
  buf[0] = '{'; /* signal that this is a hash. this makes saving/restoring easier */

  return tr_strdup (buf);
}

bool
tr_ssha1_matches (const char * ssha1,
                  const char * plain_text)
{
  char * salt;
  size_t saltlen;
  char * my_ssha1;
  uint8_t buf[SHA_DIGEST_LENGTH];
  bool result;
  const size_t sourcelen = strlen (ssha1);

  /* extract the salt */
  if (sourcelen < 2 * SHA_DIGEST_LENGTH - 1)
    return false;
  saltlen = sourcelen - 2 * SHA_DIGEST_LENGTH - 1;
  salt = tr_malloc (saltlen);
  memcpy (salt, ssha1 + 2 * SHA_DIGEST_LENGTH + 1, saltlen);

  /* hash pass + salt */
  my_ssha1 = tr_malloc (2 * SHA_DIGEST_LENGTH + saltlen + 2);
  tr_sha1 (buf, plain_text, strlen (plain_text), salt, saltlen, NULL);
  tr_sha1_to_hex (&my_ssha1[1], buf);
  memcpy (my_ssha1 + 1 + 2 * SHA_DIGEST_LENGTH, salt, saltlen);
  my_ssha1[1 + 2 * SHA_DIGEST_LENGTH + saltlen] = '\0';
  my_ssha1[0] = '{';

  result = strcmp (ssha1, my_ssha1) == 0;

  tr_free (my_ssha1);
  tr_free (salt);

  return result;
}

/***
****
***/

void *
tr_base64_encode (const void * input,
                  size_t       input_length,
                  size_t     * output_length)
{
  char * ret;

  if (input != NULL)
    {
      if (input_length != 0)
        {
          size_t ret_length = 4 * ((input_length + 2) / 3);
          base64_encodestate state;

#ifdef USE_SYSTEM_B64
          /* Additional space is needed for newlines if we're using unpatched libb64 */
          ret_length += ret_length / 72 + 1;
#endif

          ret = tr_new (char, ret_length + 8);

          base64_init_encodestate (&state);
          ret_length = base64_encode_block (input, input_length, ret, &state);
          ret_length += base64_encode_blockend (ret + ret_length, &state);

          if (output_length != NULL)
            *output_length = ret_length;

          ret[ret_length] = '\0';

          return ret;
        }
      else
        ret = tr_strdup ("");
    }
  else
    {
      ret = NULL;
    }

  if (output_length != NULL)
    *output_length = 0;

  return ret;
}

void *
tr_base64_encode_str (const char * input,
                      size_t     * output_length)
{
  return tr_base64_encode (input, input == NULL ? 0 : strlen (input), output_length);
}

void *
tr_base64_decode (const void * input,
                  size_t       input_length,
                  size_t     * output_length)
{
  char * ret;

  if (input != NULL)
    {
      if (input_length != 0)
        {
          size_t ret_length = input_length / 4 * 3;
          base64_decodestate state;

          ret = tr_new (char, ret_length + 8);

          base64_init_decodestate (&state);
          ret_length = base64_decode_block (input, input_length, ret, &state);

          if (output_length != NULL)
            *output_length = ret_length;

          ret[ret_length] = '\0';

          return ret;
        }
      else
        ret = tr_strdup ("");
    }
  else
    {
      ret = NULL;
    }

  if (output_length != NULL)
    *output_length = 0;

  return ret;
}

void *
tr_base64_decode_str (const char * input,
                      size_t     * output_length)
{
  return tr_base64_decode (input, input == NULL ? 0 : strlen (input), output_length);
}
