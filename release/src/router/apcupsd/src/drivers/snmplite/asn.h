/*
 * asn.h
 *
 * ASN.1 type classes
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

#ifndef __ASN_H
#define __ASN_H

#include "astring.h"
#include "alist.h"

namespace Asn
{
   // Class field
   static const unsigned char UNIVERSAL    = 0x00;
   static const unsigned char APPLICATION  = 0x40;
   static const unsigned char CONTEXT      = 0x80;
   static const unsigned char PRIVATE      = 0xC0;

   // Primitive/Constructed field
   static const unsigned char CONSTRUCTED  = 0x20;
   static const unsigned char PRIMITIVE    = 0x00;

   // Built-in types from ASN.1
   typedef unsigned char Identifier;
   static const Identifier INTEGER         = UNIVERSAL   | PRIMITIVE   | 0x02;
   static const Identifier BITSTRING       = UNIVERSAL   | PRIMITIVE   | 0x03;
   static const Identifier OCTETSTRING     = UNIVERSAL   | PRIMITIVE   | 0x04;
   static const Identifier NULLL           = UNIVERSAL   | PRIMITIVE   | 0x05;
   static const Identifier OBJECTID        = UNIVERSAL   | PRIMITIVE   | 0x06;
   static const Identifier SEQUENCE        = UNIVERSAL   | CONSTRUCTED | 0x10;

   // SNMP-specific types
   static const Identifier IPADDRESS       = APPLICATION | PRIMITIVE   | 0x00;
   static const Identifier COUNTER         = APPLICATION | PRIMITIVE   | 0x01;
   static const Identifier GAUGE           = APPLICATION | PRIMITIVE   | 0x02;
   static const Identifier TIMETICKS       = APPLICATION | PRIMITIVE   | 0x03;
   static const Identifier GET_REQ_PDU     = CONTEXT     | CONSTRUCTED | 0x00;
   static const Identifier GETNEXT_REQ_PDU = CONTEXT     | CONSTRUCTED | 0x01;
   static const Identifier GET_RSP_PDU     = CONTEXT     | CONSTRUCTED | 0x02;
   static const Identifier SET_REQ_PDU     = CONTEXT     | CONSTRUCTED | 0x03;
   static const Identifier TRAP_PDU        = CONTEXT     | CONSTRUCTED | 0x04;

   // **************************************************************************
   // Forward declarations
   // **************************************************************************
   class Integer;
   class ObjectId;
   class OctetString;
   class Sequence;

   // **************************************************************************
   // Object
   // **************************************************************************
   class Object
   {
   public:

      Object(Identifier type): _type(type) {}
      virtual ~Object() {}

      static Object *Demarshal(unsigned char *&buffer, unsigned int &buflen);

      Identifier   Type()    const { return _type; }

      Integer     *AsInteger()     { return (Integer*)    this; }
      ObjectId    *AsObjectId()    { return (ObjectId*)   this; }
      OctetString *AsOctetString() { return (OctetString*)this; }
      Sequence    *AsSequence()    { return (Sequence*)   this; }

      virtual bool IsInteger()     { return false; }
      virtual bool IsObjectId()    { return false; }
      virtual bool IsOctetString() { return false; }
      virtual bool IsSequence()    { return false; }

      virtual bool Marshal(
         unsigned char *&buffer,
         unsigned int &buflen) const = 0;

      virtual Object *copy() const = 0;

   protected:

      virtual bool demarshal(unsigned char *&buffer, unsigned int &buflen) = 0;
      bool marshalType(unsigned char *&buffer, unsigned int &buflen) const;

      bool marshalLength(
         unsigned int len,
         unsigned char *&buffer, 
         unsigned int &buflen) const;

      bool demarshalLength(
         unsigned char *&buffer,
         unsigned int &buflen, 
         unsigned int &vallen) const;

      int numbits(unsigned int num) const;

      Identifier _type;
   };

   // **************************************************************************
   // Integer
   // **************************************************************************
   class Integer: public Object
   {
   public:

      Integer(Identifier id = INTEGER) : 
         Object(id), _value(0), _signed(false) {}

      Integer(int value) : 
         Object(INTEGER), _value(value), _signed(value < 0) {}

      virtual ~Integer() {}

      unsigned int UintValue() const { return _value; }
      int IntValue() const { return (int)_value; }

      Integer &operator=(int value)
         { _value = value; _signed = value < 0; return *this; }
      Integer &operator=(unsigned int value)
         { _value = value; _signed = false; return *this; }

      virtual Object *copy() const { return new Integer(*this); }
      virtual bool Marshal(unsigned char *&buffer, unsigned int &buflen) const;
      virtual bool IsInteger() { return true; }

   protected:

      virtual bool demarshal(unsigned char *&buffer, unsigned int &buflen);

      unsigned int _value;
      bool _signed;
   };

   // **************************************************************************
   // OctetString
   // **************************************************************************
   class OctetString: public Object
   {
   public:

      OctetString() : Object(OCTETSTRING), _data(NULL), _len(0) {}
      OctetString(const char *value);
      OctetString(const unsigned char *value, unsigned int len);
      virtual ~OctetString() { delete [] _data; }

      OctetString(const OctetString &rhs);
      OctetString &operator=(const OctetString &rhs);

      const unsigned char *Value() const { return _data; }
      const unsigned int Length() const  { return _len; }

      operator const char *() const { return (const char *)_data; }

      virtual Object *copy() const { return new OctetString(*this); }
      virtual bool Marshal(unsigned char *&buffer, unsigned int &buflen) const;
      virtual bool IsOctetString() { return true; }

   protected:

      virtual bool demarshal(unsigned char *&buffer, unsigned int &buflen);

      void assign(const unsigned char *data, unsigned int len);

      unsigned char *_data;
      unsigned int _len;
   };

   // **************************************************************************
   // ObjectId
   // **************************************************************************
   class ObjectId: public Object
   {
   public:

      ObjectId() : Object(OBJECTID), _value(NULL), _count(0) {}
      ObjectId(const int oid[]);
      virtual ~ObjectId() { delete [] _value; }

      ObjectId(const ObjectId &rhs);
      ObjectId &operator=(const ObjectId &rhs);
      ObjectId &operator=(const int oid[]);

      bool operator==(const ObjectId &rhs) const;
      bool operator==(const int oid[])     const;
      bool operator!=(const ObjectId &rhs) const { return !(*this == rhs); }
      bool operator!=(const int oid[])     const { return !(*this == oid); }

      bool IsChildOf(const int oid[]);

      virtual Object *copy() const { return new ObjectId(*this); }
      virtual bool Marshal(unsigned char *&buffer, unsigned int &buflen) const;
      virtual bool IsObjectId() { return true; }

   protected:

      virtual bool demarshal(unsigned char *&buffer, unsigned int &buflen);
      void assign(const int oid[]);
      void assign(const int oid[], unsigned int count);

      int *_value;
      unsigned int _count;
   };

   // **************************************************************************
   // Null
   // **************************************************************************
   class Null: public Object
   {
   public:

      Null() : Object(NULLL) {}
      virtual ~Null() {}

      virtual Object *copy() const { return new Null(*this); }
      virtual bool Marshal(unsigned char *&buffer, unsigned int &buflen) const;

   protected:

      virtual bool demarshal(unsigned char *&buffer, unsigned int &buflen);
   };

   // **************************************************************************
   // Sequence
   // **************************************************************************
   class Sequence: public Object
   {
   public:

      Sequence(Identifier type = SEQUENCE);
      virtual ~Sequence();

      Sequence(const Sequence &rhs);
      Sequence &operator=(const Sequence &rhs);

      unsigned int Size() { return _size; }
      Object *operator[](unsigned int idx);
      void Append(Object *obj);

      virtual Object *copy() const { return new Sequence(*this); }
      virtual bool Marshal(unsigned char *&buffer, unsigned int &buflen) const;
      virtual bool IsSequence() { return true; }

   protected:

      virtual bool demarshal(unsigned char *&buffer, unsigned int &buflen);
      void assign(const Sequence &rhs);
      void clear();

      Object **_data;
      unsigned int _size;
   };
};
#endif
