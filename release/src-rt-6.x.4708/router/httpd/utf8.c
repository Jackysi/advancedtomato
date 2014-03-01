/*

	Tomato Firmware

*/

#include "tomato.h"

#include <ctype.h>
#include <sys/types.h>

// Returns the amount of 16-bit elements in UTF-16LE needed
// (without the terminating null) to store given UTF-8 string
static int utf8_to_utf16_size(const char *s)
{
	int ret = -1;
	unsigned int byte;
	size_t count = 0;

	while ((byte = *((const unsigned char *)s++))) {
		++count;
		if (byte >= 0xc0) {
			if (byte >= 0xF5) {
				errno = EILSEQ;
				goto out;
			}
			if (!*s) 
				break;
			if (byte >= 0xC0) 
				s++;
			if (!*s) 
				break;
			if (byte >= 0xE0) 
				s++;
			if (!*s) 
				break;
			if (byte >= 0xF0) {
				s++;
				++count;
			}
		}
	}
	ret = count;
out:
	return ret;
}

// Converts one UTF-8 sequence to cpu-endian Unicode value
static int utf8_to_unicode(uint32_t *wc, const char *s)
{
    	unsigned int byte = *((const unsigned char *)s);

					/* single byte */
	if (byte == 0) {
		*wc = (uint32_t) 0;
		return 0;
	} else if (byte < 0x80) {
		*wc = (uint32_t) byte;
		return 1;
					/* double byte */
	} else if (byte < 0xc2) {
		goto fail;
	} else if (byte < 0xE0) {
		if ((s[1] & 0xC0) == 0x80) {
			*wc = ((uint32_t)(byte & 0x1F) << 6)
			    | ((uint32_t)(s[1] & 0x3F));
			return 2;
		} else
			goto fail;
					/* three-byte */
	} else if (byte < 0xF0) {
		if (((s[1] & 0xC0) == 0x80) && ((s[2] & 0xC0) == 0x80)) {
			*wc = ((uint32_t)(byte & 0x0F) << 12)
			    | ((uint32_t)(s[1] & 0x3F) << 6)
			    | ((uint32_t)(s[2] & 0x3F));
			/* Check valid ranges */
			if (((*wc >= 0x800) && (*wc <= 0xD7FF))
			  || ((*wc >= 0xe000) && (*wc <= 0xFFFF)))
				return 3;
		}
		goto fail;
					/* four-byte */
	} else if (byte < 0xF5) {
		if (((s[1] & 0xC0) == 0x80) && ((s[2] & 0xC0) == 0x80)
		  && ((s[3] & 0xC0) == 0x80)) {
			*wc = ((uint32_t)(byte & 0x07) << 18)
			    | ((uint32_t)(s[1] & 0x3F) << 12)
			    | ((uint32_t)(s[2] & 0x3F) << 6)
			    | ((uint32_t)(s[3] & 0x3F));
		/* Check valid ranges */
		if ((*wc <= 0x10ffff) && (*wc >= 0x10000))
			return 4;
		}
		goto fail;
	}
fail:
	errno = EILSEQ;
	return -1;
}

#if 0
// Converts a UTF-8 string to a UTF-16LE string (little endian version)
// Returns length of output buffer in utf16 characters
int utf8_to_utf16_string(const char *ins, uint16_t **outs)
{
	const char *t = ins;
	uint32_t wc;
	int allocated;
	uint16_t *outpos;
	int shorts, ret = -1;

	shorts = utf8_to_utf16_size(ins);
	if (shorts < 0)
		goto fail;

	allocated = 0;
	if (!*outs) {
		*outs = malloc((shorts + 1) * sizeof(uint16_t));
		if (!*outs)
			goto fail;
		allocated = 1;
	}
	outpos = *outs;

	while(1) {
		int m = utf8_to_unicode(&wc, t);
		if (m <= 0) {
			if (m < 0) {
				/* do not leave space allocated if failed */
				if (allocated) {
					free(*outs);
					*outs = (uint16_t *)NULL;
				}
				goto fail;
			}
			*outpos++ = (uint16_t)0;
			break;
		}
		if (wc < 0x10000)
			*outpos++ = (uint16_t)wc;
		else {
			wc -= 0x10000;
			*outpos++ = (uint16_t)((wc >> 10) + 0xd800);
			*outpos++ = (uint16_t)((wc & 0x3ff) + 0xdc00);
		}
		t += m;
	}

	ret = --outpos - *outs;

fail:
	return ret;
}
#endif

static inline char *js_utf16_char(const uint16_t u, char *b)
{
	unsigned char c = (unsigned char)u;

	if (u <= 0xFF) {
		if ((c == '"') || (c == '\'') || (c == '\\') || (!isprint(c)))
			b += sprintf(b, "\\x%02x", c);
		else
			*b++ = c;
	}
	else
		b += sprintf(b, "\\u%04x", u);

	return b;
}

static inline char *html_utf16_char(const uint16_t u, char *b)
{
	unsigned char c = (unsigned char)u;

	if ((u > 0xFF) || (c == '&') || (c == '<') || (c == '>') || (c == '"') || (c == '\'') || (!isprint(c)))
		b += sprintf(b, "&#%d;", u);
	else
		*b++ = c;

	return b;
}

typedef char *(*utf16_conv)(const uint16_t u, char *b);

static char *utf8_to_string(const char *ins, int csize, utf16_conv conv)
{
	const char *t = ins;
	uint32_t wc;
	char *outpos, *outs = NULL;
	int shorts;

	shorts = utf8_to_utf16_size(ins);
	if (shorts <= 0)
		return NULL;

	outpos = outs = malloc((shorts + 1) * csize + 1);
	if (!outs)
		return NULL;

	while(1) {
		int m = utf8_to_unicode(&wc, t);
		if (m <= 0) {
			if (m < 0) {
				/* do not leave space allocated if failed */
				free(outs);
				return NULL;
			}
			break;
		}

		if (wc < 0x10000) {
			outpos = conv((uint16_t)wc, outpos);
		}
		else {
			wc -= 0x10000;
			outpos = conv((uint16_t)((wc >> 10) + 0xd800), outpos);
			outpos = conv((uint16_t)((wc & 0x3ff) + 0xdc00), outpos);
		}
		t += m;
	}

	*outpos = '\0';
	return outs;
}

char *utf8_to_js_string(const char *ins)
{
	return utf8_to_string(ins, 6, js_utf16_char);
}

char *utf8_to_html_string(const char *ins)
{
	return utf8_to_string(ins, 8, html_utf16_char);
}
