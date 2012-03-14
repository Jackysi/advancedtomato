
#include <config.h>
#include <sys/types.h>

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dnscrypt.h"
#include "salsa20_random.h"
#include "randombytes.h"
#include "utils.h"

size_t
dnscrypt_response_header_size(void)
{
    return sizeof DNSCRYPT_MAGIC_RESPONSE - 1U
        + crypto_box_NONCEBYTES + crypto_box_MACBYTES;
}

size_t
dnscrypt_query_header_size(void)
{
    return DNSCRYPT_MAGIC_QUERY_LEN
        + crypto_box_PUBLICKEYBYTES + crypto_box_HALF_NONCEBYTES
        + crypto_box_MACBYTES;
}

size_t
dnscrypt_pad(uint8_t *buf, const size_t len, const size_t max_len)
{
    uint8_t  *buf_padding_area = buf + len;
    size_t    padded_len, padding_len;

    if (max_len < len + DNSCRYPT_MIN_PAD_LEN) {
        return len;
    }
    padded_len = len + DNSCRYPT_MIN_PAD_LEN +
        salsa20_random_uniform(max_len - len - DNSCRYPT_MIN_PAD_LEN + 1U);
    padded_len += DNSCRYPT_BLOCK_SIZE - padded_len % DNSCRYPT_BLOCK_SIZE;
    if (padded_len > max_len) {
        padded_len = max_len;
    }
    assert(padded_len >= len);
    padding_len = padded_len - len;

#ifdef DNSCRYPT_USE_ONLY_ONE_BYTE_FROM_PRNG_FOR_PADDING
    memset(buf_padding_area, (int) salsa20_random(), padding_len);
#else
    salsa20_random_buf(buf_padding_area, padding_len);
    assert(max_len >= padded_len);
#endif
    return padded_len;
}

void
randombytes(unsigned char * const buf, const unsigned long long buf_len)
{
    assert(buf_len <= SIZE_MAX);
    salsa20_random_buf(buf, buf_len);
}

void
dnscrypt_key_to_fingerprint(char fingerprint[80U], const uint8_t * const key)
{
    const size_t fingerprint_size = 80U;
    size_t       fingerprint_pos = (size_t) 0U;
    size_t       key_pos = (size_t) 0U;

    COMPILER_ASSERT(crypto_box_PUBLICKEYBYTES == 32U);
    COMPILER_ASSERT(crypto_box_SECRETKEYBYTES == 32U);
    for (;;) {
        assert(fingerprint_size > fingerprint_pos);
        snprintf(&fingerprint[fingerprint_pos],
                 fingerprint_size - fingerprint_pos, "%02X%02X",
                 key[key_pos], key[key_pos + 1U]);
        key_pos += 2U;
        if (key_pos >= crypto_box_PUBLICKEYBYTES) {
            break;
        }
        fingerprint[fingerprint_pos + 4U] = ':';
        fingerprint_pos += 5U;
    }
}

static int
_dnscrypt_parse_char(uint8_t key[crypto_box_PUBLICKEYBYTES],
                     size_t * const key_pos_p, int * const state_p,
                     const int c, uint8_t * const val_p)
{
    uint8_t c_val;

    switch (*state_p) {
    case 0:
    case 1:
        if (isspace(c) || (c == ':' && *state_p == 0)) {
            break;
        }
        if (c == '#') {
            *state_p = 2;
            break;
        }
        if (!isxdigit(c)) {
            return -1;
        }
        c_val = (c >= '0' && c <= '9') ? c - '0' : c - 'a' + 10;
        assert(c_val < 16U);
        if (*state_p == 0) {
            *val_p = c_val * 16U;
            *state_p = 1;
        } else {
            *val_p |= c_val;
            key[(*key_pos_p)++] = *val_p;
            if (*key_pos_p >= crypto_box_PUBLICKEYBYTES) {
                return 0;
            }
            *state_p = 0;
        }
        break;
    case 2:
        if (c == '\n') {
            *state_p = 0;
        }
    }
    return 1;
}

int
dnscrypt_fingerprint_to_key(const char * const fingerprint,
                            uint8_t key[crypto_box_PUBLICKEYBYTES])
{
    const char *p = fingerprint;
    size_t      key_pos = (size_t) 0U;
    int         c;
    int         ret;
    int         state = 0;
    uint8_t     val = 0U;

    if (fingerprint == NULL) {
        return -1;
    }
    while ((c = tolower((int) (unsigned char) *p)) != 0) {
        ret = _dnscrypt_parse_char(key, &key_pos, &state, c, &val);
        if (ret <= 0) {
            return ret;
        }
        p++;
    }
    return -1;
}
