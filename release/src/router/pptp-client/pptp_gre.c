/* pptp_gre.c  -- encapsulate PPP in PPTP-GRE.
 *                Handle the IP Protocol 47 portion of PPTP.
 *                C. Scott Ananian <cananian@alumni.princeton.edu>
 *
 * $Id: pptp_gre.c,v 1.1.1.1 2002/07/25 06:52:39 honor Exp $
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "ppp_fcs.h"
#include "pptp_msg.h"
#include "util.h"

#define PACKET_MAX 8196
/* test for a 32 bit counter overflow */
#define WRAPPED( curseq, lastseq) \
    ((((curseq) & 0xffffff00)==0) && (((lastseq) & 0xffffff00 )==0xffffff00))

static u_int32_t ack_sent, ack_recv;
static u_int32_t seq_sent, seq_recv;
static u_int16_t pptp_gre_call_id, pptp_gre_peer_call_id;

/* decaps gets all the packets possible with ONE blocking read */
/* returns <0 if read() call fails */
int decaps_hdlc(int fd, int (*cb)(int cl, void *pack, unsigned int len), int cl);
int encaps_hdlc(int fd, void *pack, unsigned int len);
int decaps_gre (int fd, int (*cb)(int cl, void *pack, unsigned int len), int cl);
int encaps_gre (int fd, void *pack, unsigned int len);

#if 1
#include <stdio.h>
void print_packet(int fd, void *pack, unsigned int len) {
  unsigned char *b = (unsigned char *)pack;
  unsigned int i,j;
  FILE *out = fdopen(fd, "w");

  fprintf(out,"-- begin packet (%u) --\n", len);
  for (i=0; i<len; i+=16) {
    for (j=0; j<8; j++)
      if (i+2*j+1<len)
	fprintf(out, "%02x%02x ", 
		(unsigned int) b[i+2*j], (unsigned int) b[i+2*j+1]);
      else if (i+2*j<len)
	fprintf(out, "%02x ", (unsigned int) b[i+2*j]);
    fprintf(out, "\n");
  }
  fprintf(out, "-- end packet --\n");
  fflush(out);
}
#endif

void pptp_gre_copy(u_int16_t call_id, u_int16_t peer_call_id, 
		   int pty_fd, struct in_addr inetaddr) {
  struct sockaddr_in src_addr;
  int s, n;

  pptp_gre_call_id = call_id;
  pptp_gre_peer_call_id = peer_call_id;

  /* Open IP protocol socket */
  s = socket(AF_INET, SOCK_RAW, PPTP_PROTO);
  if (s<0) { warn("socket: %s", strerror(errno)); return; }
  src_addr.sin_family = AF_INET;
  src_addr.sin_addr   = inetaddr;
  src_addr.sin_port   = 0;
  if (connect(s, (struct sockaddr *) &src_addr, sizeof(src_addr))<0) {
    warn("connect: %s", strerror(errno)); return;
  }
  /* Pseudo-terminal already open. */
  
  ack_sent = ack_recv = seq_sent = seq_recv = 0;

  n = (s>pty_fd)?(s+1):(pty_fd+1); /* weird select semantics */

  /* Dispatch loop */
  for (;;) { /* until error happens on s or pty_fd */
    struct timeval tv = {0, 0}; /* non-blocking select */
    fd_set rfds;
    int retval;

    /* watch terminal and socket for input */
    FD_ZERO(&rfds);
    FD_SET(s, &rfds);
    FD_SET(pty_fd,&rfds);

    /* if there is a pending ACK, do non-blocking select,
       otherwise, block until data is available */
    retval = select(n, &rfds, NULL, NULL, (ack_sent != seq_recv) ? &tv : NULL);

    if (retval == 0 && ack_sent != seq_recv) /* if outstanding ack */
      encaps_gre(s, NULL, 0); /* send ack with no payload */

    if ((FD_ISSET(pty_fd, &rfds) && (decaps_hdlc(pty_fd, encaps_gre, s) < 0))
     || (FD_ISSET(s,  &rfds) && (decaps_gre(s, encaps_hdlc, pty_fd) < 0)))
        break;
  }

  /* Close up when done. */
  close(s); close(pty_fd);
}

#define HDLC_FLAG         0x7E
#define HDLC_ESCAPE       0x7D
#define HDLC_TRANSPARENCY 0x20

/* ONE blocking read per call; dispatches all packets possible */
/* returns 0 on success, or <0 on read failure                 */
int decaps_hdlc(int fd, int (*cb)(int cl, void *pack, unsigned int len), int cl) {
  unsigned char buffer[PACKET_MAX];
  unsigned int start = 0;
  int end;
  int status;

  static unsigned int len = 0, escape = 0;
  static unsigned char copy[PACKET_MAX];
  
  /* start is start of packet.  end is end of buffer data */
  /*  this is the only blocking read we will allow */

  if ((end = read (fd, buffer, sizeof(buffer))) <= 0) {
    log ("short read (%u): %s", end, strerror(errno));
    return -1;
  }
  
  while (start < end) {

    /* Copy to 'copy' and un-escape as we go. */

    while (buffer[start] != HDLC_FLAG) {
      if ((escape == 0) && buffer[start] == HDLC_ESCAPE) {
	escape = HDLC_TRANSPARENCY;
      } else {
        if (len < PACKET_MAX)
	  copy [len++] = buffer[start] ^ escape;
        escape = 0;
      }
      start++;

      if (start >= end)
        return 0; /* No more data, but the frame is not complete yet. */
    }

    /* found flag.  skip past it */
    start++;

    /* check for over-short packets and silently discard, as per RFC1662 */
    if ((len < 4) || (escape != 0)) {
      len = 0; escape = 0;
      continue;
    }
    /* check, then remove the 16-bit FCS checksum field */
    if (pppfcs16 (PPPINITFCS16, copy, len) != PPPGOODFCS16)
      log("Bad Frame Check Sequence during PPP to GRE decapsulation");
    len -= sizeof(u_int16_t);

    /* so now we have a packet of length 'len' in 'copy' */
    if ((status = cb (cl, copy, len)) < 0)
      return status; /* error-check */

    /* Great!  Let's do more! */
    len = 0; escape = 0;
  }

  return 0;
  /* No more data to process. */
}

/* Make stripped packet into HDLC packet */
int encaps_hdlc(int fd, void *pack, unsigned int len) {
  unsigned char *source = (unsigned char *)pack;
  unsigned char dest[2*PACKET_MAX+2]; /* largest expansion possible */
  unsigned int pos=0, i;
  u_int16_t fcs;

  /* Compute the FCS */
  fcs = pppfcs16(PPPINITFCS16, source, len) ^ 0xFFFF;

  /* start character */
  dest[pos++]=HDLC_FLAG;
  /* escape the payload */
  for (i=0; i<len+2; i++) {
    /* wacked out assignment to add FCS to end of source buffer */
    unsigned char c = (i<len)?source[i]:(i==len)?(fcs&0xFF):((fcs>>8)&0xFF);
    if (pos>=sizeof(dest)) break; /* truncate on overflow */
    if ( (c<0x20) || (c==HDLC_FLAG) || (c==HDLC_ESCAPE) ) {
      dest[pos++]=HDLC_ESCAPE;
      if (pos<sizeof(dest))
	dest[pos++]=c^0x20;
    } else
      dest[pos++]=c;
  }
  /* tack on the end-flag */
  if (pos<sizeof(dest))
    dest[pos++]=HDLC_FLAG;
  
  /* now write this packet */
  return write(fd, dest, pos);
}

int decaps_gre (int fd, int (*cb)(int cl, void *pack, unsigned int len), int cl) {
  unsigned char buffer[PACKET_MAX+64/*ip header*/];
  struct pptp_gre_header *header;
  int status, ip_len=0;
  static int first=1;

  if ((status = read (fd, buffer, sizeof(buffer))) <= 0) {
    log("short read (%u): %s", status, strerror(errno));
    return -1;
  }

  /* strip off IP header, if present */
  if ((buffer[0]&0xF0)==0x40) 
    ip_len = (buffer[0]&0xF)*4;
  header = (struct pptp_gre_header *)(buffer+ip_len);

  /* verify packet (else discard) */
  if (((ntoh8(header->ver)&0x7F)!=PPTP_GRE_VER) || /* version should be 1   */
      (ntoh16(header->protocol)!=PPTP_GRE_PROTO)|| /* GRE protocol for PPTP */
      PPTP_GRE_IS_C(ntoh8(header->flags)) ||    /* flag C should be clear   */
      PPTP_GRE_IS_R(ntoh8(header->flags)) ||    /* flag R should be clear   */
      (!PPTP_GRE_IS_K(ntoh8(header->flags))) || /* flag K should be set     */
      ((ntoh8(header->flags)&0xF)!=0)) { /* routing and recursion ctrl = 0  */
    /* if invalid, discard this packet */
    log("Discarding GRE: %X %X %X %X %X %X", 
	 ntoh8(header->ver)&0x7F, ntoh16(header->protocol), 
	 PPTP_GRE_IS_C(ntoh8(header->flags)),
	 PPTP_GRE_IS_R(ntoh8(header->flags)), 
	 PPTP_GRE_IS_K(ntoh8(header->flags)),
	 ntoh8(header->flags)&0xF);
    return 0;
  }
  if (PPTP_GRE_IS_A(ntoh8(header->ver))) { /* acknowledgement present */
    u_int32_t ack = (PPTP_GRE_IS_S(ntoh8(header->flags)))?
      header->ack:header->seq; /* ack in different place if S=0 */
    if (ack > ack_recv) ack_recv = ack;
    /* also handle sequence number wrap-around  */
    if (WRAPPED(ack,ack_recv)) ack_recv=ack;
  }
  if (PPTP_GRE_IS_S(ntoh8(header->flags))) { /* payload present */
    unsigned int headersize = sizeof(*header);
    unsigned int payload_len= ntoh16(header->payload_len);
    u_int32_t seq       = ntoh32(header->seq);
    if (!PPTP_GRE_IS_A(ntoh8(header->ver))) headersize-=sizeof(header->ack);
    /* check for incomplete packet (length smaller than expected) */
    if (status-headersize<payload_len) return 0; 
    /* check for out-of-order sequence number */
    /* (handle sequence number wrap-around, and try to do it right) */
    if ( first || (seq > seq_recv) || 
	WRAPPED( seq, seq_recv)){
      seq_recv = seq;
      first=0;
      return cb(cl, buffer+ip_len+headersize, payload_len);
    } else {
      log("discarding out-of-order  seq is %d seqrecv is %d", seq, seq_recv); 
      return 0; /* discard out-of-order packets */
    }
  }
  return 0; /* ack, but no payload */
}
int encaps_gre (int fd, void *pack, unsigned int len) {
  union {
    struct pptp_gre_header header;
    unsigned char buffer[PACKET_MAX+sizeof(struct pptp_gre_header)];
  } u;
  static u_int32_t seq=0;
  unsigned int header_len;

  /* package this up in a GRE shell. */
  u.header.flags	= hton8 (PPTP_GRE_FLAG_K);
  u.header.ver  	= hton8 (PPTP_GRE_VER);
  u.header.protocol	= hton16(PPTP_GRE_PROTO);
  u.header.payload_len	= hton16(len);
  u.header.call_id	= hton16(pptp_gre_peer_call_id);
  
  /* special case ACK with no payload */
  if (pack==NULL) {
    if (ack_sent != seq_recv) {
      u.header.ver |= hton8(PPTP_GRE_FLAG_A);
      u.header.payload_len = hton16(0);
      u.header.seq = hton32(seq_recv); /* ack is in odd place because S=0 */
      ack_sent = seq_recv;
      return write(fd, &u.header, sizeof(u.header)-sizeof(u.header.seq));
    } else return 0; /* we don't need to send ACK */
  } /* explicit brace to avoid ambiguous `else' warning */
  /* send packet with payload */
  u.header.flags |= hton8(PPTP_GRE_FLAG_S);
  u.header.seq    = hton32(seq);
  if (ack_sent != seq_recv) { /* send ack with this message */
    u.header.ver |= hton8(PPTP_GRE_FLAG_A);
    u.header.ack  = hton32(seq_recv);
    ack_sent = seq_recv;
    header_len = sizeof(u.header);
  } else { /* don't send ack */
    header_len = sizeof(u.header) - sizeof(u.header.ack);
  }
  if (header_len+len>=sizeof(u.buffer)) return 0; /* drop this, it's too big */
  /* copy payload into buffer */
  memcpy(u.buffer+header_len, pack, len);
  /* record and increment sequence numbers */
  seq_sent = seq; seq++;
  /* write this baby out to the net */
  /* print_packet(2, u.buffer, header_len+len); */
  return write(fd, u.buffer, header_len+len);
}


