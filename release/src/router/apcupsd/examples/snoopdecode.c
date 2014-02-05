/*
 * snoopdecode.c
 *
 * Decodes traces captured by 'usbsnoop'
 * (http://benoit.papillault.free.fr/usbsnoop/index.php)
 *
 * See usbsnoop.txt for details.
 */

/*
 * Copyright (C) 2004-2005 Adam Kropelin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_LINE 1024

char line[MAX_LINE] = "";
FILE* file;

enum
{
	DIR_UNKNOWN = 0,
	DIR_TO_DEVICE,
	DIR_FROM_DEVICE
};

enum
{
	FUNC_UNKNOWN = 0,
	FUNC_CONTROL_TXFER,
	FUNC_CLASS_INTERFACE,
	FUNC_GET_DESC,
	FUNC_SELECT_CONFIG,
	FUNC_GET_DESC_FROM_IFACE,
	FUNC_BULK_TXFER,
	FUNC_RESET_PIPE,
	FUNC_ABORT_PIPE,
};

enum
{
	REQ_GET_REPORT = 1,
	REQ_GET_IDLE = 2,
	REQ_GET_PROTOCOL = 3,
	REQ_SET_REPORT = 9,
	REQ_SET_IDLE = 10,
	REQ_SET_PROTOCOL = 11
};

struct urb
{
	int seq;
	unsigned long len;
	unsigned char* data;
	unsigned long flags;
	unsigned long index;
	unsigned long type;
	unsigned long reserved;
	unsigned long request;
	unsigned long value;
	int time;
	int dir;
	int func;
};

struct rpt
{
	struct rpt* next;
	unsigned char* data;
	int seq;
	unsigned short len;
	unsigned char id;
	unsigned long value;
};

struct rpt* rpts = NULL;

/*
[197 ms]  >>>  URB 1 going down  >>> 
-- URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE:
  TransferBufferLength = 00000012
  TransferBuffer       = 864c9208
  TransferBufferMDL    = 00000000
  Index                = 00000000
  DescriptorType       = 00000001 (USB_DEVICE_DESCRIPTOR_TYPE)
  LanguageId           = 00000000
[203 ms] UsbSnoop - MyInternalIOCTLCompletion(f7b91db0) : fido=00000000, Irp=863df850, Context=86405d10, IRQL=2

[203 ms]  <<<  URB 1 coming back  <<< 
-- URB_FUNCTION_CONTROL_TRANSFER:
  PipeHandle           = 863e4150
  TransferFlags        = 0000000b (USBD_TRANSFER_DIRECTION_IN, USBD_SHORT_TRANSFER_OK)
  TransferBufferLength = 00000012
  TransferBuffer       = 864c9208
  TransferBufferMDL    = 8640f108
    00000000: 12 01 10 01 00 00 00 08 1d 05 02 00 06 00 03 01
    00000010: 02 01
  UrbLink              = 00000000
*/

/*
[59563 ms]  >>>  URB 346 going down  >>> 
-- URB_FUNCTION_CLASS_INTERFACE:
  TransferFlags          = 00000001 (USBD_TRANSFER_DIRECTION_IN, ~USBD_SHORT_TRANSFER_OK)
  TransferBufferLength = 00000005
  TransferBuffer       = f7f12ba0
  TransferBufferMDL    = 00000000
  UrbLink                 = 00000000
  RequestTypeReservedBits = 00000022
  Request                 = 00000001
  Value                   = 0000032f
  Index                   = 00000000
[59567 ms] UsbSnoop - MyInternalIOCTLCompletion(f7b91db0) : fido=86397288, Irp=85ff4b40, Context=85f4b3d8, IRQL=2
[59567 ms]  <<<  URB 346 coming back  <<< 
-- URB_FUNCTION_CONTROL_TRANSFER:
  PipeHandle           = 863e4150
  TransferFlags        = 0000000b (USBD_TRANSFER_DIRECTION_IN, USBD_SHORT_TRANSFER_OK)
  TransferBufferLength = 00000002
  TransferBuffer       = f7f12ba0
  TransferBufferMDL    = 85f102f0
    00000000: 2f 02
  UrbLink              = 00000000
  SetupPacket          =
    00000000: a1 01 2f 03 00 00 05 00
*/

int fetch_line()
{
	return !!fgets(line, sizeof(line), file);
}

int find_urb()
{
	do
	{
		if (line[0] == '[' && strstr(line, " URB ") &&
		   (strstr(line, "going down") || strstr(line, "coming back")))
		{
			return 1;
		}
	}
	while( fetch_line() );
	
	return 0;
}

int init_urb(struct urb* urb)
{
	memset(urb, 0, sizeof(*urb));
	
	if (!find_urb())
		return 0;
	
	urb->time = atoi(line+1);
	
	if (strstr(line, "going down"))
		urb->dir = DIR_TO_DEVICE;
	else if(strstr(line, "coming back"))
		urb->dir = DIR_FROM_DEVICE;
	else
		urb->dir = DIR_UNKNOWN;
		
	urb->seq = atoi(strstr(line,"URB ")+4);

	return 1;
}

unsigned long get_hex_value()
{
	char* ptr = strchr(line, '=');
	if (!ptr)
		return 0xffff;

	return strtoul(ptr+2, NULL, 16);
}

unsigned char* parse_data_dump(int len)
{
	int count = 0;
	char* ptr= NULL;
	
	unsigned char* data = (unsigned char *)malloc(len);	
	if (!data)
		return NULL;
		
	while(count < len)
	{
		if ((count % 16) == 0)
		{
			if (count)
			{
				if (!fetch_line())
				{
					free(data);
					return NULL;
				}
			}

			ptr = strchr(line, ':');
			if (!ptr)
			{
				free(data);
				return NULL;
			}

			ptr += 2;
		}

		data[count++] = strtoul(ptr, NULL, 16);
		ptr += 3;
	}
	
	return data;
}

void parse_urb_body(struct urb* urb)
{
	int setup_packet = 0;

	while (fetch_line())
	{
		if (line[0] == '[')
			break;
			
		if (strstr(line, "TransferBufferLength"))
		{
			urb->len = get_hex_value();
		}
		else if (strstr(line, "TransferFlags"))
		{
			urb->flags = get_hex_value();
		}
		else if (strstr(line, "Index"))
		{
			urb->index = get_hex_value();
		}
		else if (strstr(line, "DescriptorType"))
		{
			urb->type = get_hex_value();
		}
		else if (strstr(line, "RequestTypeReservedBits"))
		{
			urb->reserved = get_hex_value();
		}
		else if (strstr(line, "Request"))
		{
			urb->request = get_hex_value();
		}
		else if (strstr(line, "Value"))
		{
			urb->value = get_hex_value();
		}
		else if (strstr(line, "SetupPacket"))
		{
			setup_packet = 1;
		}
		else if (strstr(line, "00000000:"))
		{
			if (!setup_packet && !urb->data)
				urb->data = parse_data_dump(urb->len);
		}
		else if (strstr(line, "-- URB_FUNCTION_"))
		{
			if (strstr(line, "URB_FUNCTION_CONTROL_TRANSFER"))
				urb->func = FUNC_CONTROL_TXFER;
			else if (strstr(line, "URB_FUNCTION_CLASS_INTERFACE"))
				urb->func = FUNC_CLASS_INTERFACE;
			else if (strstr(line, "URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE"))
				urb->func = FUNC_GET_DESC;
			else if (strstr(line, "URB_FUNCTION_SELECT_CONFIGURATION"))
				urb->func = FUNC_SELECT_CONFIG;
			else if (strstr(line, "URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE"))
				urb->func = FUNC_GET_DESC_FROM_IFACE;
			else if (strstr(line, "URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER"))
				urb->func = FUNC_BULK_TXFER;
			else if (strstr(line, "URB_FUNCTION_RESET_PIPE"))
				urb->func = FUNC_RESET_PIPE;
			else if (strstr(line, "URB_FUNCTION_ABORT_PIPE"))
				urb->func = FUNC_ABORT_PIPE;
			else
			{
				urb->func = FUNC_UNKNOWN;
				printf("Unknown FUNC: %s\n", line);
				exit(0);
			}
		}
	}
}

void free_urb(struct urb* urb)
{
	free(urb->data);
}

void print_urb(struct urb* urb)
{
	unsigned int x;
	printf("[%08d ms] %05d %c ", urb->time, urb->seq,
		(urb->dir == DIR_TO_DEVICE) ? '>' : (urb->dir == DIR_FROM_DEVICE) ? '<' : '?');
		
	printf("req=%04lx value=%04lx", urb->request, urb->value);
	
	if( urb->data )
	{
		printf(" [");
		for(x=0; x<urb->len; x++)
		{
			printf( "%02x", urb->data[x] );
			if (x < urb->len-1)
				printf(" ");
		}
		printf("]");
	}
	
	printf("\n");
}

struct rpt* find_report_by_seq(int seq)
{
	struct rpt* rpt = rpts;

	while( rpt )
	{
		if (rpt->seq == seq)
			break;
			
		rpt = rpt->next;
	}
	
	return rpt;
}

struct rpt* find_report_by_id(unsigned char id)
{
	struct rpt* rpt = rpts;

	while( rpt )
	{
		if (rpt->id == id)
			break;

		rpt = rpt->next;
	}
	
	return rpt;
}

void del_report(struct urb* urb)
{
	unsigned char id = urb->data[0];
	struct rpt* rpt = rpts;
	struct rpt** prev = &rpts;
	
	while( rpt )
	{
		if (rpt->id == id)
			break;
			
		prev = &(rpt->next);
		rpt = rpt->next;
	}
	
	if (rpt)
	{
		*prev = rpt->next;
		free(rpt);
	}
}

void add_report(struct urb* urb)
{
	struct rpt* rpt;
	unsigned char id;

	if (urb->data)
		id = urb->data[0];
	else
		id = urb->value & 0xff;

	if ((rpt = find_report_by_id(id)))
	{
		rpt->seq = urb->seq;
		rpt->value = urb->value;
		return;
	}

	rpt = (struct rpt*)malloc(sizeof(*rpt));
	memset(rpt, 0, sizeof(*rpt));

	rpt->id = id;
	rpt->seq = urb->seq;
	rpt->value = urb->value;

	rpt->next = rpts;
	rpts = rpt;
}

int check_and_update_report(struct urb* urb)
{
	struct rpt* rpt;

	rpt = find_report_by_seq(urb->seq);
	if (!rpt)
		return 0;

	if (rpt->data && !memcmp(rpt->data, urb->data+1, urb->len-1))
		return 0;

	free(rpt->data);
	rpt->len = urb->len-1;
	rpt->data = (unsigned char *)malloc(rpt->len);
	memcpy(rpt->data, urb->data+1, rpt->len);

	return 1;
}

void display_report(struct urb* urb)
{
	unsigned long data = 0;
	char buf[9];
	unsigned int n;

	n = printf( "[%04d s] %05d %c %s 0x%02x (%03u) ",
		urb->time/1000, urb->seq, urb->func == FUNC_BULK_TXFER ? '*' : ' ',
		urb->dir==DIR_TO_DEVICE ? "WRITE" : " READ", urb->data[0], urb->data[0]);

	switch (urb->len)
	{
		case 5:
			data = (data << 8) | urb->data[4];
		case 4:
			data = (data << 8) | urb->data[3];
		case 3:
			data = (data << 8) | urb->data[2];
		case 2:
			data = (data << 8) | urb->data[1];
			sprintf(buf, "%0*lx", (int)(urb->len-1)*2, data);
			n += printf( "%8s (%lu)", buf, data );
			break;
			
		default:
			n += printf( "-------- (----------)" );
			break;
	}

	printf( "%*c", 55-n, ' ');
	for (n=0; n<urb->len-1; n++)
		printf("%02x ", urb->data[n+1]);
	printf("\n");
	fflush(stdout);
}

void update_reports(struct urb* urb)
{
	if (urb->dir == DIR_TO_DEVICE)
	{
		// Only care about class requests
		if (urb->func != FUNC_CLASS_INTERFACE)
			return;

		if (urb->request == REQ_GET_REPORT)
		{
			add_report(urb);
		}
		else if (urb->request == REQ_SET_REPORT)
		{
			display_report(urb);
			del_report(urb);
		}
	}
	else
	{
		if (urb->func == FUNC_CONTROL_TXFER)
		{
			if (check_and_update_report(urb))
				display_report(urb);
		}
		else if (urb->data && urb->func == FUNC_BULK_TXFER)
		{
			add_report(urb);
			if (check_and_update_report(urb))
				display_report(urb);
		}
	}
}

int main(int argc, char* argv[])
{
	struct urb urb;

	file = stdin;

	while(init_urb(&urb))
	{
		parse_urb_body(&urb);
		
		update_reports(&urb);
		
		free_urb(&urb);
	}

	return 0;
}
