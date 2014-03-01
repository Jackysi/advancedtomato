/*
 * snmp.h
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

#ifndef __SNMP_H
#define __SNMP_H

#include "apc.h"
#include "astring.h"
#include "aarray.h"
#include "alist.h"
#include "asn.h"

namespace Snmp
{
   // **************************************************************************
   // Types
   // **************************************************************************
   struct Variable
   {
      Asn::Identifier type;
      int i32;
      unsigned int u32;
      astring str;
      alist<Variable> seq;
   };

   // **************************************************************************
   // VarBind
   // **************************************************************************
   class VarBind
   {
   public:
      VarBind(const Asn::ObjectId &oid, Variable *data = NULL);
      VarBind(Asn::Sequence &seq);
      ~VarBind();

      bool Extract(Variable *data);
      Asn::ObjectId &Oid() { return *_oid; }

      Asn::Sequence *GetAsn();

   private:
      Asn::ObjectId *_oid;
      Asn::Object *_data;

      VarBind(const VarBind &rhs);
      VarBind &operator=(const VarBind &rhs);
   };

   // **************************************************************************
   // VarBindList
   // **************************************************************************
   class VarBindList
   {
   public:
      VarBindList() {}
      VarBindList(Asn::Sequence &seq);
      ~VarBindList();

      void Append(const Asn::ObjectId &oid, Variable *data = NULL);

      unsigned int Size() const { return _vblist.size(); }
      VarBind *operator[](unsigned int idx) { return _vblist[idx]; }

      Asn::Sequence *GetAsn();

   private:
      aarray<VarBind *> _vblist;

      VarBindList(const VarBindList &rhs);
      VarBindList &operator=(const VarBindList &rhs);
   };

   // **************************************************************************
   // Message
   // **************************************************************************
   class Message
   {
   public:
      virtual ~Message() {}

      Asn::Identifier Type() const { return _type; }
      astring Community() const    { return _community; }

      static Message *Demarshal(unsigned char *&buffer, unsigned int &buflen);
      bool Marshal(unsigned char *&buffer, unsigned int &buflen);

   protected:
      Message() {}
      Message(Asn::Identifier type, const char *community) : 
         _type(type), _community(community) {}
      static const int SNMP_VERSION_1 = 0;
      virtual Asn::Sequence *GetAsn() = 0;

      Asn::Identifier _type;
      astring _community;
   };

   // **************************************************************************
   // VbListMessage
   // **************************************************************************
   class VbListMessage: public Message
   {
   public:
      VbListMessage(Asn::Identifier type, const char *community, int reqid);
      virtual ~VbListMessage() { delete _vblist; }

      int RequestId()   const { return _reqid;     }
      int ErrorStatus() const { return _errstatus; }
      int ErrorIndex()  const { return _errindex;  }

      void Append(const Asn::ObjectId &oid, Variable *data = NULL);
      unsigned int Size() const { return _vblist->Size(); }
      VarBind &operator[](unsigned int idx) { return *((*_vblist)[idx]); }

      static VbListMessage *CreateFromSequence(
         Asn::Identifier type, const char *community, Asn::Sequence &seq);

   protected:
      VbListMessage(
         Asn::Identifier type,
         const char *community,
         Asn::Sequence &seq);

      virtual Asn::Sequence *GetAsn();

      int _reqid;
      int _errstatus;
      int _errindex;
      VarBindList *_vblist;
   };

   // **************************************************************************
   // TrapMessage
   // **************************************************************************
   class TrapMessage: public Message
   {
   public:
      virtual ~TrapMessage() { delete _vblist; }

      int Generic()            const { return _generic;   }
      int Specific()           const { return _specific;  }
      unsigned int Timestamp() const { return _timestamp; }

      static TrapMessage *CreateFromSequence(
         Asn::Identifier type, const char *community, Asn::Sequence &seq);

   protected:
      TrapMessage(Asn::Identifier type, const char *community, Asn::Sequence &seq);
      virtual Asn::Sequence *GetAsn() { return NULL; }

      Asn::ObjectId *_enterprise;
      int _generic;
      int _specific;
      unsigned int _timestamp;
      VarBindList *_vblist;
   };

   // **************************************************************************
   // SnmpEngine
   // **************************************************************************
   class SnmpEngine
   {
   public:

      SnmpEngine();
      ~SnmpEngine();

      bool Open(const char *host, unsigned short port = SNMP_AGENT_PORT, 
                const char *comm = "public", bool trap = false);
      void Close();

      struct OidVar
      {
         const int *oid;
         Variable data;
      };

      bool Get(const int oid[], Variable *data);
      bool Get(alist<OidVar> &oids);
      void GetSequence(const int oid[], Variable *data);

      bool Set(const int oid[], Variable *data);

      TrapMessage *TrapWait(unsigned int msec);

      void SetCommunity(const char *comm) { _community = comm; }

   private:

      bool issue(Message *pdu);
      Message *rspwait(unsigned int msec, bool trap = false);
      VbListMessage *perform(VbListMessage *req);

      static const unsigned short SNMP_TRAP_PORT = 162;
      static const unsigned short SNMP_AGENT_PORT = 161;

      int _socket;
      int _trapsock;
      int _reqid;
      astring _community;
      struct sockaddr_in _destaddr;
   };
};

#endif
