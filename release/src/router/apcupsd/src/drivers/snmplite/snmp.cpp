/*
 * snmp.cpp
 *
 * SNMP client interface
 */

/*
 * Copyright (C) 2009 Adam Kropelin
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

#include "apc.h"
#include "snmp.h"
#include "asn.h"

#ifdef __WIN32__
#define close(x) closesocket(x)
#endif

using namespace Snmp;

SnmpEngine::SnmpEngine() :
   _socket(-1),
   _trapsock(-1),
   _reqid(0)
{
}

SnmpEngine::~SnmpEngine()
{
   close(_socket);
   close(_trapsock);
}

bool SnmpEngine::Open(const char *host, unsigned short port, const char *comm, bool trap)
{
   // In case we are already open
   close(_socket);
   close(_trapsock);

   // Remember new community name
   _community = comm;

   // Generate starting request id
   struct timeval now;
   gettimeofday(&now, NULL);
   _reqid = now.tv_usec;

   // Look up destination address
   memset(&_destaddr, 0, sizeof(_destaddr));
   _destaddr.sin_family = AF_INET;
   _destaddr.sin_port = htons(port);
   _destaddr.sin_addr.s_addr = inet_addr(host);
   if (_destaddr.sin_addr.s_addr == INADDR_NONE) 
   {
      struct hostent he;
      char *tmphstbuf = NULL;
      size_t hstbuflen = 0;
      struct hostent *hp = gethostname_re(host, &he, &tmphstbuf, &hstbuflen);
      if (!hp || hp->h_length != sizeof(_destaddr.sin_addr.s_addr) || 
          hp->h_addrtype != AF_INET)
      {
         free(tmphstbuf);
         return false;
      }

      memcpy(&_destaddr.sin_addr.s_addr, hp->h_addr, 
             sizeof(_destaddr.sin_addr.s_addr));
      free(tmphstbuf);
   }

   // Get a UDP socket
   _socket = socket(PF_INET, SOCK_DGRAM, 0);
   if (_socket == -1)
   {
      perror("socket");
      return false;
   }

   // Bind to rx on any interface
   struct sockaddr_in addr;
   memset(&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_port = 0;
   addr.sin_addr.s_addr = INADDR_ANY;
   int rc = bind(_socket, (struct sockaddr*)&addr, sizeof(addr));
   if (rc == -1)
   {
      perror("bind");
      return false;
   }

   // Open socket for receiving traps, if clients wants one
   if (trap)
   {
      _trapsock = socket(PF_INET, SOCK_DGRAM, 0);
      if (_trapsock == -1)
      {
         perror("socket");
         return false;
      }

      memset(&addr, 0, sizeof(addr));
      addr.sin_family = AF_INET;
      addr.sin_port = htons(SNMP_TRAP_PORT);
      addr.sin_addr.s_addr = INADDR_ANY;
      rc = bind(_trapsock, (struct sockaddr*)&addr, sizeof(addr));
      if (rc == -1)
      {
         perror("bind");
         return false;
      }
   }

   return true;
}

void SnmpEngine::Close()
{
   close(_socket);
   _socket = -1;
   close(_trapsock);
   _trapsock = -1;
}

bool SnmpEngine::Set(const int oid[], Variable *data)
{
   // Send the request
   VbListMessage req(Asn::SET_REQ_PDU, _community, _reqid++);
   req.Append(oid, data);
   if (!issue(&req))
      return false;

   // Wait for a response
   VbListMessage *rsp = (VbListMessage*)rspwait(1000);
   if (!rsp)
      return false;

   // Check error status
   if (rsp->ErrorStatus())
   {
      delete rsp;
      return false;
   }

   delete rsp;
   return true;
}

bool SnmpEngine::Get(const int oid[], Variable *data)
{
   OidVar oidvar;
   oidvar.oid = oid;
   oidvar.data = *data;

   alist<OidVar> oids;
   oids.append(oidvar);

   bool ret = Get(oids);
   if (ret)
      *data = (*oids.begin()).data;

   return ret;
}

bool SnmpEngine::Get(alist<OidVar> &oids)
{
   // First, fetch all scalar (i.e. non-sequence) OIDs using a single
   // SNMP GETNEXT-REQUEST. Note we use GETNEXT instead of GET since all
   // OIDs omit the trailing 0.

   // Start with a request with no varbinds
   VbListMessage getreq(Asn::GETNEXT_REQ_PDU, _community, _reqid++);

   // Append one varbind for each oidvar from the caller
   alist<OidVar>::iterator iter;
   for (iter = oids.begin(); iter != oids.end(); ++iter)
      if (iter->data.type != Asn::SEQUENCE)
         getreq.Append(iter->oid);

   // Perform request if we put at least one OID in it
   if (getreq.Size() > 0)
   {
      // Send request & await response
      VbListMessage *rsp = perform(&getreq);
      if (!rsp)
         return false;

      // Verify response varbind size is same as request
      // (i.e. agent provided a response for each varbind we requested)
      if (rsp->Size() != getreq.Size())
      {
         delete rsp;
         return false;
      }

      // Copy response data into caller's oidvars. Although I believe the SNMP
      // spec requires the GET-RESPONSE to give the var-bind-list in the same
      // order as the GET-REQUEST, we're not going to count on that. A little
      // CPU time spent searching is ok to ensure widest compatiblity in case
      // we encounter a weak SNMP agent implementation.
      VbListMessage &response = *rsp;
      for (unsigned int i = 0; i < response.Size(); i++)
      {
         for (iter = oids.begin(); iter != oids.end(); ++iter)
         {
            if (response[i].Oid().IsChildOf(iter->oid))
            {
               response[i].Extract(&iter->data);
               break;
            }
         }
      }

      // Done with response
      delete rsp;
   }

   // Now process sequences. For each sequence we issue a series of
   // SNMP GETNEXT-REQUESTs until we detect the end of the sequence.
   for (iter = oids.begin(); iter != oids.end(); ++iter)
   {
      if (iter->data.type == Asn::SEQUENCE)
      {
         Asn::ObjectId nextoid = iter->oid;
         while (1)
         {
            // Create a GET-NEXT request
            VbListMessage nextreq(Asn::GETNEXT_REQ_PDU, _community, _reqid++);

            // Request a single OID
            nextreq.Append(nextoid);

            // Perform the request
            VbListMessage *rspmsg = perform(&nextreq);

            // If request failed, we're done (possibly at end of MIB so we
            // don't consider this an error)
            if (!rspmsg)
               break;

            // If result OID is not a child of the top-level OID we're fetching 
            // it means we've run off the end of the sequence so we're done
            VarBind &result = (*rspmsg)[0];
            if (!result.Oid().IsChildOf(iter->oid))
            {
               delete rspmsg;
               break;
            }

            // Extract data and append to sequence
            Variable tmp;
            result.Extract(&tmp);
            iter->data.seq.append(tmp);

            // Save returned OID for next iteration
            nextoid = result.Oid();
            delete rspmsg;
         }
      }
   }

   return true;
}

VbListMessage *SnmpEngine::perform(VbListMessage *req)
{
   // Send the request
   if (!issue(req))
      return NULL;

   // Wait for a response
   VbListMessage *rsp = (VbListMessage*)rspwait(2000);
   if (rsp && rsp->ErrorStatus())
   {
      delete rsp;
      return NULL;
   }

   return rsp;
}

TrapMessage *SnmpEngine::TrapWait(unsigned int msec)
{
   if (_trapsock == -1)
      return NULL;

   return (TrapMessage*)rspwait(msec, true);
}

bool SnmpEngine::issue(Message *msg)
{
   // Marshal the data
   unsigned char data[8192];
   unsigned int buflen = sizeof(data);
   unsigned char *buffer = data;
   if (!msg->Marshal(buffer, buflen))
      return false;

   // Send data to destination
   int datalen = buffer - data;
   int rc = sendto(_socket, (char*)data, datalen, 0, 
                   (struct sockaddr*)&_destaddr, sizeof(_destaddr));
   if (rc != datalen)
   {
      perror("sendto");
      return false;
   }

   return true;
}

Message *SnmpEngine::rspwait(unsigned int msec, bool trap)
{
   static unsigned char data[8192];
   struct sockaddr_in fromaddr;

   int sock = trap ? _trapsock : _socket;

   // Calculate exit time
   struct timeval exittime;
   gettimeofday(&exittime, NULL);
   exittime.tv_usec += msec * 1000;
   while (exittime.tv_usec >= 1000000)
   {
      exittime.tv_usec -= 1000000;
      exittime.tv_sec++;
   }

   while(1)
   {
      // See if we've run out of time
      struct timeval now;
      gettimeofday(&now, NULL);
      if (now.tv_sec > exittime.tv_sec ||
          (now.tv_sec == exittime.tv_sec &&
           now.tv_usec >= exittime.tv_usec))
         return NULL;

      // Calculate new timeout
      struct timeval timeout;
      timeout.tv_sec = exittime.tv_sec - now.tv_sec;
      timeout.tv_usec = exittime.tv_usec - now.tv_usec;
      while (timeout.tv_usec < 0)
      {
         timeout.tv_usec += 1000000;
         timeout.tv_sec--;
      }

      // Wait for a datagram to arrive
      fd_set fds;
      FD_ZERO(&fds);
      FD_SET(sock, &fds);
      int rc = select(sock+1, &fds, NULL, NULL, &timeout);
      if (rc == -1)
      {
         if (errno == EAGAIN || errno == EINTR)
            continue;
         return NULL;
      }

      // Timeout
      if (rc == 0)
         return NULL;

      // Read datagram
      socklen_t fromlen = sizeof(fromaddr);
      rc = recvfrom(sock, (char*)data, sizeof(data), 0, 
                    (struct sockaddr*)&fromaddr, &fromlen);
      if (rc == -1)
      {
         if (errno == EAGAIN || errno == EINTR)
            continue;
         return NULL;
      }

      // Ignore packet if it's not from our agent
      if (fromaddr.sin_addr.s_addr != _destaddr.sin_addr.s_addr)
         continue;

      // Got a packet from our agent: decode it
      unsigned char *buffer = data;
      unsigned int buflen = rc;
      Message *msg = Message::Demarshal(buffer, buflen);
      if (!msg)
         continue;

      // Check message type
      if (trap && msg->Type() == Asn::TRAP_PDU)
      {
         return msg;
      }
      else if (!trap && msg->Type() == Asn::GET_RSP_PDU &&
               ((VbListMessage *)msg)->RequestId() == _reqid-1)
      {
         return msg;
      }
      else
      {
         printf("Unhandled SNMP message type: %02x\n", msg->Type());
      }

      // Throw it out and try again
      delete msg;
   }
}

// *****************************************************************************
// VarBind
// *****************************************************************************

VarBind::VarBind(const Asn::ObjectId &oid, Variable *data)
{
   _oid = new Asn::ObjectId(oid);

   if (data)
   {
      switch (data->type)
      {
      case Asn::INTEGER:
         _data = new Asn::Integer(data->type);
         *(_data->AsInteger()) = data->i32;
         break;
      case Asn::TIMETICKS:
      case Asn::COUNTER:
      case Asn::GAUGE:
         _data = new Asn::Integer(data->type);
         *(_data->AsInteger()) = data->u32;
         break;
      case Asn::OCTETSTRING:
         _data = new Asn::OctetString(data->str);
         break;
      case Asn::NULLL:
      default:
         _data = new Asn::Null();
         break;
      }
   }
   else
   {
      _data = new Asn::Null();
   }
}

VarBind::VarBind(Asn::Sequence &seq)
{
   if (seq.Size() == 2 && 
       seq[0]->IsObjectId())
   {
      _oid = seq[0]->copy()->AsObjectId();
      _data = seq[1]->copy();
   }
   else
   {
      _oid = new Asn::ObjectId();
      _data = new Asn::Null();
   }
}

VarBind::~VarBind()
{
   delete _data;
   delete _oid;
}

bool VarBind::Extract(Variable *out)
{
   out->type = _data->Type();
   if (_data->IsInteger())
   {
      out->i32 = _data->AsInteger()->IntValue();
      out->u32 = _data->AsInteger()->UintValue();
   }
   else if (_data->IsOctetString())
   {
      out->str = *_data->AsOctetString();
   }
   else
   {
      printf("Unsupported Asn::Object::AsnType: %d\n", _data->Type());
      return false;
   }
   return true;
}

Asn::Sequence *VarBind::GetAsn()
{
   Asn::Sequence *seq = new Asn::Sequence();
   seq->Append(_oid->copy());
   seq->Append(_data->copy());
   return seq;
}

// *****************************************************************************
// VarBindList
// *****************************************************************************

VarBindList::VarBindList(Asn::Sequence &seq)
{
   for (unsigned int i = 0; i < seq.Size(); i++)
   {
      if (seq[i]->IsSequence())
         _vblist.append(new VarBind(*seq[i]->AsSequence()));
   }
}

VarBindList::~VarBindList()
{
   for (unsigned int i = 0; i < _vblist.size(); i++)
      delete _vblist[i];
}

void VarBindList::Append(const Asn::ObjectId &oid, Variable *data)
{
   _vblist.append(new VarBind(oid, data));
}

Asn::Sequence *VarBindList::GetAsn()
{
   Asn::Sequence *seq = new Asn::Sequence();
   for (unsigned int i = 0; i < _vblist.size(); i++)
      seq->Append(_vblist[i]->GetAsn());
   return seq;
}

// *****************************************************************************
// VbListMessage
// *****************************************************************************
VbListMessage *VbListMessage::CreateFromSequence(
   Asn::Identifier type, const char *community, Asn::Sequence &seq)
{
   // Verify format: We should have 4 parts.
   if (seq.Size() != 4 ||
       !seq[0]->IsInteger() ||  // request-id
       !seq[1]->IsInteger() ||  // error-status
       !seq[2]->IsInteger() ||  // error-index
       !seq[3]->IsSequence())   // variable-bindings
      return NULL;

   // Extract data
   return new VbListMessage(type, community, seq);
}

VbListMessage::VbListMessage(
   Asn::Identifier type,
   const char *community,
   Asn::Sequence &seq) :
      Message(type, community)
{
   // Format was already verified in CreateFromSequence()
   _reqid = seq[0]->AsInteger()->IntValue();
   _errstatus = seq[1]->AsInteger()->IntValue();
   _errindex = seq[2]->AsInteger()->IntValue();
   _vblist = new VarBindList(*seq[3]->AsSequence());
}

VbListMessage::VbListMessage(
   Asn::Identifier type,
   const char *community,
   int reqid) :
      Message(type, community),
      _reqid(reqid),
      _errstatus(0),
      _errindex(0),
      _vblist(new VarBindList())
{
}

void VbListMessage::Append(const Asn::ObjectId &oid, Variable *data)
{
   _vblist->Append(oid, data);
}

Asn::Sequence *VbListMessage::GetAsn()
{
   Asn::Sequence *seq = new Asn::Sequence(_type);
   seq->Append(new Asn::Integer(_reqid));
   seq->Append(new Asn::Integer(_errstatus));
   seq->Append(new Asn::Integer(_errindex));
   seq->Append(_vblist->GetAsn());
   return seq;
}

// *****************************************************************************
// Message
// *****************************************************************************
Message *Message::Demarshal(unsigned char *&buffer, unsigned int &buflen)
{
   Message *ret = NULL;
   astring community;
   Asn::Identifier type;

   Asn::Object *obj = Asn::Object::Demarshal(buffer, buflen);
   if (!obj)
      return NULL;

   // Data demarshalled okay. Now walk the object tree to parse the message.

   // Top-level object should be a sequence of length 3
   Asn::Sequence &seq = *(Asn::Sequence*)obj;
   if (!obj->IsSequence() || seq.Size() != 3)
      goto error;

   // First item in sequence is an integer specifying SNMP version
   if (!seq[0]->IsInteger() ||
       seq[0]->AsInteger()->IntValue() != SNMP_VERSION_1)
      goto error;

   // Second item is the community string
   if (!seq[1]->IsOctetString())
      goto error;
   community = *seq[1]->AsOctetString();

   // Third is another sequence containing the PDU
   type = seq[2]->Type();
   switch (type)
   {
   case Asn::GET_REQ_PDU:
   case Asn::GETNEXT_REQ_PDU:
   case Asn::GET_RSP_PDU:
      ret = VbListMessage::CreateFromSequence(
         type, community, *seq[2]->AsSequence());
      break;
   case Asn::TRAP_PDU:
      ret = TrapMessage::CreateFromSequence(
         type, community, *seq[2]->AsSequence());
      break;
   default:
      break;
   }

error:
   delete obj;
   return ret;
}

bool Message::Marshal(unsigned char *&buffer, unsigned int &buflen)
{
   Asn::Sequence *seq = new Asn::Sequence();
   seq->Append(new Asn::Integer(SNMP_VERSION_1));
   seq->Append(new Asn::OctetString(_community));
   seq->Append(GetAsn());

   bool ret = seq->Marshal(buffer, buflen);
   delete seq;
   return ret;
}

// *****************************************************************************
// TrapMessage
// *****************************************************************************
TrapMessage *TrapMessage::CreateFromSequence(
   Asn::Identifier type, const char *community, Asn::Sequence &seq)
{
   // Verify format: We should have 6 parts.
   if (seq.Size() != 6 ||
       !seq[0]->IsObjectId() ||     // enterprise
       !seq[1]->IsOctetString() ||  // agent-addr
       !seq[2]->IsInteger() ||      // generic-trap
       !seq[3]->IsInteger() ||      // specific-trap
       !seq[4]->IsInteger() ||      // time-stamp
       !seq[5]->IsSequence())       // variable-bindings
      return NULL;

   // Extract data
   return new TrapMessage(type, community, seq);
}

TrapMessage::TrapMessage(
   Asn::Identifier type,
   const char *community,
   Asn::Sequence &seq) :
      Message(type, community)
{
   // Format was already verified in CreateFromSequence()
   _enterprise = seq[0]->copy()->AsObjectId();
   _generic = seq[2]->AsInteger()->IntValue();
   _specific = seq[3]->AsInteger()->IntValue();
   _timestamp = seq[4]->AsInteger()->UintValue();
   _vblist = new VarBindList(*seq[5]->AsSequence());
}
