
/* control character used for download */
#define ETX	CNTRL('c')
#define ACK	CNTRL('f')
#define NAK	CNTRL('u')
#define XON	CNTRL('q')
#define XOFF	CNTRL('s')

unsigned int csum;
unsigned int dl_entry;

static const unsigned char hextab[256] = {
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	    255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	    255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	    255, 255, 255,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 255, 255, 255, 255, 255, 255,
	255, 10, 11, 12, 13, 14, 15, 255, 255, 255, 255, 255, 255, 255,
	    255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	    255, 255, 255,
	255, 10, 11, 12, 13, 14, 15, 255, 255, 255, 255, 255, 255, 255,
	    255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	    255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	    255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	    255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	    255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	    255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	    255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	    255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	    255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	    255, 255, 255,
};


unsigned char ascii_to_bin(unsigned char c)
{
	return hextab[c];
}

unsigned char read_char_direct(void)
{
	unsigned char c, *cp;
	cp = (unsigned char *) 0xbd000020;
	while (1) {
		if (*(cp + 0x14) & 0x01) {
			c = (volatile unsigned char) *cp;
			return c;
		}
		cp++;
		cp--;
	}
}

unsigned char get_pair(void)
{
	unsigned char byte;

	byte = ascii_to_bin(read_char_direct()) << 4;
	byte |= ascii_to_bin(read_char_direct());
	csum += byte;
	return (byte);
}


void serial_putc(int ch)
{
	unsigned long temp;
	for (temp = 0; temp < 1000; temp++) {
	}
	*(char *) 0xbd000020 = (char) ch;
}


int inline serial_getc(void)
{
	return read_char_direct();
}

int serial_ischar(void)
{

	unsigned char c, *cp;
	unsigned count;
	cp = (unsigned char *) 0xbd000020;
	count = 0;
	while (count != 100) {
		if (*(cp + 0x14) & 0x01) {
			c = (volatile unsigned char) *cp;
			return c;
		}
		cp++;
		cp--;
		count++;
	}
	return 0;
}

int serial_init(void)
{
	return 0;
}


int galileo_dl(void)
{
#define display_char '.'
#define display_error 'E'
#define display_error_bad_7 '7'
#define display_error_unknown 'U'
#define display_error_length 'L'

	register int length, address, save_csum;
	int i, first, done, eof, reccount, type, client_pc;
	int src, dbl_length;
	unsigned char *buffptr, databuff[258], tempo;
	register int display_counter, chunks, leftovers, putter,
	    bytes_per_chunk;
	display_counter = 0;
	bytes_per_chunk = 16;
	csum = 0;

	reccount = 1;
	for (first = 1, done = 0; !done; first = 0, reccount++) {
		while (read_char_direct() != 'S')
			continue;
		csum = 0;
		type = read_char_direct();
		length = get_pair();
		if (length < 0 || length >= 256) {
			*(char *) 0xbd000020 = display_error_length;
			//      *(char*)0xbd00000c = display_error_length;
			return 0;
		}
		length--;
		switch (type) {
		case '0':
			while (length-- > 0)
				get_pair();
			break;
		case '3':
			address = 0;
			for (i = 0; i < 4; i++) {
				address <<= 8;
				address |= get_pair();
				length--;
			}
			if (address == -1) {
				eof = 1;
				continue;
			}
			buffptr = &databuff[0];
			dbl_length = length << 1;
			chunks = dbl_length / bytes_per_chunk;
			leftovers = dbl_length % bytes_per_chunk;
			putter = bytes_per_chunk >> 1;
			while (chunks--) {
				for (i = 0; i < bytes_per_chunk; i++)
					databuff[i] = read_char_direct();
				src = i = 0;
				while (i++ < putter) {
					tempo =
					    (ascii_to_bin(databuff[src++])
					     << 4) |
					    ascii_to_bin(databuff[src++]);
					csum += tempo;
					*(char *) address++ = tempo;
				}
			}
			if (leftovers) {
				putter = leftovers / 2;
				for (i = 0; i < leftovers; i++)
					databuff[i] = read_char_direct();
				src = i = 0;
				while (i++ < putter) {
					tempo =
					    (ascii_to_bin(databuff[src++])
					     << 4) |
					    ascii_to_bin(databuff[src++]);
					csum += tempo;
					*(char *) address++ = tempo;
				}
			}
			break;

		case '7':
			address = 0;
			for (i = 0; i < 4; i++) {
				address <<= 8;
				address |= get_pair();
				length--;
			}
			if (address == -1) {
				eof = 1;
				continue;
			}
			client_pc = address;
			if (length) {
				*(char *) 0xbd000020 = display_error_bad_7;
				//                      *(char*)0xbd00000c = display_error_bad_7;
			}

			done = 1;
			break;

		default:
			*(char *) 0xbd000020 = display_error_unknown;
			//              *(char*)0xbd00000c = display_error_unknown;

			break;
		}
		save_csum = (~csum) & 0xff;
		if ((csum = get_pair()) < 0) {
			eof = 1;
			continue;
		}
		if (csum != save_csum) {
			*(char *) 0xbd000020 = display_error;
			//                *(char*)0xbd00000c = display_error;
		} else {

			if (display_counter % 50 == 0) {
				*(char *) 0xbd000020 = display_char;
				display_counter = 0;
			}
			display_counter++;

		}
	}
	--reccount;
	dl_entry = client_pc;
	return dl_entry;	/* Success */
}
