/*
 * asn.cpp
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

#include "asn.h"
#include <stdlib.h>
#include <stdio.h>

using namespace Asn;

void debug(const char *foo, int indent)
{
   while (indent--)
      printf(" ");
   printf("%s\n", foo);
}

// *****************************************************************************
// Object
// *****************************************************************************

Object *Object::Demarshal(unsigned char *&buffer, unsigned int &buflen)
{
   static int indent = 0;

   // Must have at least one byte to work with
   if (buflen < 1)
      return NULL;

   // Extract type code from stream
   Identifier type = (Identifier)*buffer++;
   buflen--;

   // Create proper object based on type code
   Object *obj;

   switch (type)
   {
   case INTEGER:
   case COUNTER:
   case GAUGE:
   case TIMETICKS:
      debug("INTEGER", indent);
      obj = new Integer(type);
      break;
   case OCTETSTRING:
   case IPADDRESS:
      debug("OCTETSTRING", indent);
      obj = new OctetString();
      break;
   case OBJECTID:
      debug("OBJECTID", indent);
      obj = new ObjectId();
      break;
   case NULLL:
      debug("NULLL", indent);
      obj = new Null();
      break;
   case SEQUENCE:
   case GET_REQ_PDU:
   case GETNEXT_REQ_PDU:
   case GET_RSP_PDU:
   case TRAP_PDU:
      debug("SEQUENCE", indent);
      indent += 2;
      obj = new Sequence(type);
      break;      
   default:
      printf("UNKNOWN ASN type=0x%02x\n", type);
      debug("UNKNOWN", indent);
      obj = NULL;
      break;      
   }

   // Have the object demarshal itself from the stream
   if (obj && !obj->demarshal(buffer, buflen))
   {
      delete obj;
      obj = NULL;
   }

   if (type & CONSTRUCTED)
      indent -= 2;

   return obj;
}

int Object::numbits(unsigned int num) const
{
   if (num == 0)
      return 1;

   int bits = 0;
   while (num)
   {
      bits++;
      num >>= 1;
   }

   return bits;
}

bool Object::marshalLength(
   unsigned int len,
   unsigned char *&buffer,
   unsigned int &buflen) const
{
   // Compute number of bytes required to store length
   unsigned int bits = numbits(len);
   unsigned int datalen;
   if (bits <= 7)
      datalen = 1;
   else
      datalen = (bits + 7) / 8 + 1;

   // If no buffer provided, just return number of bytes required
   if (!buffer)
   {
      buflen += datalen;
      return true;
   }

   // Fail if not enough space remaining in buffer
   if (buflen < datalen)
      return false;

   // If using long form, begin with byte count
   if (datalen > 1)
      *buffer++ = (datalen - 1) | 0x80;

   // Encode length bytes
   switch (datalen)
   {
   case 5:
      *buffer++ = (len >> 24) & 0xff;
   case 4:
      *buffer++ = (len >> 16) & 0xff;
   case 3:
      *buffer++ = (len >>  8) & 0xff;
   case 2:
   case 1:
      *buffer++ = len & 0xff;
      break;
   }

   return true;
}

bool Object::marshalType(unsigned char *&buffer, unsigned int &buflen) const
{
   // If no buffer provided, just return number of bytes required
   if (!buffer)
   {
      buflen++;
      return true;
   }

   // Fail if not enough room for data
   if (buflen < 1)
      return false;
   buflen--;

   // Store type directly in stream
   *buffer++ = _type;
   return true;
}

bool Object::demarshalLength(
   unsigned char *&buffer,
   unsigned int &buflen,
   unsigned int &vallen) const
{
   // Must have at least one byte to work with
   if (buflen < 1)
      return false;
   unsigned int datalen = *buffer++;
   buflen--;

   // Values less than 128 are stored directly in the first (only) byte
   if (datalen < 128)
   {
      vallen = datalen;
      return true;
   }

   // For values > 128, first byte-128 indicates number of bytes to follow
   // Bail if not enough data for additional bytes
   datalen -= 128;
   if (buflen < datalen)
      return false;
   buflen -= datalen;

   // Read data bytes
   vallen = 0;
   while (datalen--)
   {
      vallen <<= 8;
      vallen |= *buffer++;
   }

   return true;
}

// *****************************************************************************
// Integer
// *****************************************************************************

bool Integer::Marshal(unsigned char *&buffer, unsigned int &buflen) const
{
   // Marshal the type code
   if (!marshalType(buffer, buflen))
      return false;

   // Calculate the number of bytes it will require to store the value
   unsigned int datalen = 4;
   unsigned int tmp = _value;
   while (datalen > 1)
   {
      if (( _signed && (tmp & 0xff800000) != 0xff800000) ||
          (!_signed && (tmp & 0xff800000) != 0x00000000))
         break;

       tmp <<= 8;
       datalen--;
   }

   // Special case for unsigned value with MSb set
   if (!_signed && datalen == 4 && (tmp & 0x80000000))
      datalen = 5;

   // Marshal the length
   if (!marshalLength(datalen, buffer, buflen))
      return false;

   // If no buffer provided, just return number of bytes required
   if (!buffer)
   {
      buflen += datalen;
      return true;
   }

   // Fail if not enough room for data
   if (buflen < datalen)
      return false;
   buflen -= datalen;

   // Marshal the value itself
   switch (datalen)
   {
   case 5:
      *buffer++ = 0;
   case 4:
      *buffer++ = (_value >> 24) & 0xff;
   case 3:
      *buffer++ = (_value >> 16) & 0xff;
   case 2:
      *buffer++ = (_value >>  8) & 0xff;
   case 1:
      *buffer++ = _value & 0xff;
   }

   return true;
}

bool Integer::demarshal(unsigned char *&buffer, unsigned int &buflen)
{
   // Unmarshal the data length
   unsigned int datalen;
   if (!demarshalLength(buffer, buflen, datalen) || 
       datalen < 1 || datalen > 4 || buflen < datalen)
   {
      return false;
   }
   buflen -= datalen;

   // Determine signedness
   if (*buffer & 0x80)
   {
      // Start with all 1s so result will be sign-extended
      _value = (unsigned int)-1;
      _signed = true;
   }
   else
   {
      _value = 0;
      _signed = false;
   }

   // Unmarshal the data
   while (datalen--)
   {
      _value <<= 8;
      _value |= *buffer++;
   }

   return true;
}

// *****************************************************************************
// OctetString
// *****************************************************************************

OctetString::OctetString(const char *value) :
   Object(OCTETSTRING),
   _data(NULL)
{
   assign((const unsigned char *)value, strlen(value));
}

OctetString::OctetString(const unsigned char *value, unsigned int len) :
   Object(OCTETSTRING),
   _data(NULL)
{
   assign(value, len);
}

OctetString::OctetString(const OctetString &rhs) :
   Object(OCTETSTRING),
   _data(NULL)
{
   assign(rhs._data, rhs._len);
}

OctetString &OctetString::operator=(const OctetString &rhs)
{
   if (&rhs != this)
      assign(rhs._data, rhs._len);
   return *this;
}

bool OctetString::Marshal(unsigned char *&buffer, unsigned int &buflen) const
{
   // Marshal the type code
   if (!marshalType(buffer, buflen))
      return false;

   // Marshal the length
   if (!marshalLength(_len, buffer, buflen))
      return false;

   // If no buffer provided, just return number of bytes required
   if (!buffer)
   {
      buflen += _len;
      return true;
   }

   // Bail if not enough room for data
   if ((unsigned int)buflen < _len)
      return false;
   buflen -= _len;

   // Marshal data
   memcpy(buffer, _data, _len);
   buffer += _len;

   return true;
}

bool OctetString::demarshal(unsigned char *&buffer, unsigned int &buflen)
{
   // Unmarshal the data length
   unsigned int datalen;
   if (!demarshalLength(buffer, buflen, datalen) || 
       datalen < 1 || buflen < datalen)
   {
      return false;
   }
   buflen -= datalen;

   // Unmarshal the data
   assign(buffer, datalen);
   buffer += datalen;

   return true;
}

void OctetString::assign(const unsigned char *data, unsigned int len)
{
   delete [] _data;
   _data = new unsigned char[len+1];
   memcpy(_data, data, len);
   _data[len] = '\0';
   _len = len;
}

// *****************************************************************************
// ObjectId
// *****************************************************************************

ObjectId::ObjectId(const int oid[]) :
   Object(OBJECTID),
   _value(NULL)
{
   assign(oid);
}

ObjectId::ObjectId(const ObjectId &rhs) :
   Object(OBJECTID),
   _value(NULL)
{
   assign(rhs._value, rhs._count);
}

ObjectId &ObjectId::operator=(const ObjectId &rhs)
{
   if (&rhs != this)
      assign(rhs._value, rhs._count);
   return *this;
}

ObjectId &ObjectId::operator=(const int oid[])
{
   if (oid != _value)
      assign(oid);
   return *this;
}

void ObjectId::assign(const int oid[])
{
   unsigned int count = 0;
   while (oid[count] != -1)
      count++;
   assign(oid, count);
}

void ObjectId::assign(const int oid[], unsigned int count)
{
   delete [] _value;
   _value = NULL;
   _count = count;
   if (_count)
   {
      _value = new int[_count];
      memcpy(_value, oid, _count*sizeof(int));
   }
}

bool ObjectId::operator==(const ObjectId &rhs) const
{
   if (_count != rhs._count)
      return false;

   for (unsigned int i = 0; i < _count; i++)
   {
      if (_value[i] != rhs._value[i])
         return false;
   }

   return true;
}

bool ObjectId::operator==(const int oid[]) const
{
   unsigned int i;
   for (i = 0; i < _count && oid[i] != -1; i++)
   {
      if (_value[i] != oid[i])
         return false;
   }

   return i == _count && oid[i] == -1;
}

// Same as operator== except we're allowed to be longer than the parameter
bool ObjectId::IsChildOf(const int oid[])
{
   unsigned int i;
   for (i = 0; i < _count && oid[i] != -1; i++)
   {
      if (_value[i] != oid[i])
         return false;
   }

   return i < _count && oid[i] == -1;
}

bool ObjectId::Marshal(unsigned char *&buffer, unsigned int &buflen) const
{
   // Protect from trying to marshal an empty object
   if (!_value || _count < 2)
      return false;

   // Marshal the type code
   if (!marshalType(buffer, buflen))
      return false;

   // ASN.1 requires a special case for first two identifiers
   unsigned int cnt = _count-1;
   unsigned int vals[cnt];
   vals[0] = _value[0] * 40 + _value[1];
   for (unsigned int i = 2; i < _count; i++)
      vals[i-1] = _value[i];

   // Calculate number of octets required to store data
   // We can only store 7 bits of data in each octet, so round accordingly
   unsigned int datalen = 0;
   for (unsigned int i = 0; i < cnt; i++)
      datalen += (numbits(vals[i]) + 6) / 7;

   // Marshal the length
   if (!marshalLength(datalen, buffer, buflen))
      return false;

   // If no buffer provided, just return number of bytes required
   if (!buffer)
   {
      buflen += datalen;
      return true;
   }

   // Bail if data bytes will not fit
   if (buflen < datalen)
      return false;
   buflen -= datalen;

   // Write data: 7 data bits per octet, bit 7 set on all but final octet
   for (unsigned int i = 0; i < cnt; i++)
   {
      unsigned int val = vals[i];
      unsigned int noctets = (numbits(val) + 6) / 7;
      switch (noctets)
      {
      case 5:
         *buffer++ = ((val >> 28) & 0x7f) | 0x80;
      case 4:
         *buffer++ = ((val >> 21) & 0x7f) | 0x80;
      case 3:
         *buffer++ = ((val >> 14) & 0x7f) | 0x80;
      case 2:
         *buffer++ = ((val >>  7) & 0x7f) | 0x80;
      case 1:
      case 0:
         *buffer++ = val & 0x7f;
      }
   }

   return true;
}

bool ObjectId::demarshal(unsigned char *&buffer, unsigned int &buflen)
{
   // Unmarshal the data length
   unsigned int datalen;
   if (!demarshalLength(buffer, buflen, datalen) || 
       datalen < 1 || buflen < datalen)
   {
      return false;
   }
   buflen -= datalen;

   // Allocate new value array, sized for worst case. +1 is because of
   // ASN.1 special case of compressing first two ids into one octet.
   delete [] _value;
   _value = new int[datalen+1];

   // Unmarshal identifier values
   unsigned int i = 0;
   while (datalen)
   {
      // Accumulate octets into this identifier
      _value[i] = 0;
      unsigned int val;
      do
      {
         datalen--;
         val = *buffer++;
         _value[i] <<= 7;
         _value[i] |= val & 0x7f;
      }
      while (datalen && val & 0x80);

      // Handle special case for first two ids
      if (i++ == 0)
      {
         _value[1] = _value[0] % 40;
         _value[0] /= 40;
         i++;
      }
   }
   _count = i;

   return true;
}

// *****************************************************************************
// Null
// *****************************************************************************

bool Null::Marshal(unsigned char *&buffer, unsigned int &buflen) const
{
   // Marshal the type code
   if (!marshalType(buffer, buflen))
      return false;

   // Marshal the length
   if (!marshalLength(0, buffer, buflen))
      return false;

   return true;
}

bool Null::demarshal(unsigned char *&buffer, unsigned int &buflen)
{
   // Unmarshal the data length
   unsigned int datalen;
   return demarshalLength(buffer, buflen, datalen) && datalen == 0;
}

// *****************************************************************************
// Sequence
// *****************************************************************************

Sequence::Sequence(Identifier type) : 
   Object(type), 
   _data(NULL), 
   _size(0)
{
}

Sequence::~Sequence()
{
   clear();
}

Sequence::Sequence(const Sequence &rhs) :
   Object(rhs._type),
   _data(NULL)
{
   assign(rhs);
}

void Sequence::clear()
{
   for (unsigned int i = 0; i < _size; i++)
      delete _data[i];
   delete [] _data;
   _size = 0;
}

bool Sequence::Marshal(unsigned char *&buffer, unsigned int &buflen) const
{
   // Marshal the type code
   if (!marshalType(buffer, buflen))
      return false;

   // Need to calculate the length of the marshalled sequence.
   // Do so by passing a NULL buffer to Marshal() which will
   // accumulate total length in buflen parameter.
   unsigned int datalen = 0;
   unsigned char *buf = NULL;
   for (unsigned int i = 0; i < _size; ++i)
   {
      if (!_data[i]->Marshal(buf, datalen))
         return false;
   }

   // Now marshal the length itself
   if (!marshalLength(datalen, buffer, buflen))
      return false;

   // If no buffer provided, just return number of bytes required
   if (!buffer)
   {
      buflen += datalen;
      return true;
   }

   // Marshal all items in the sequence
   for (unsigned int i = 0; i < _size; ++i)
   {
      if (!_data[i]->Marshal(buffer, buflen))
         return false;
   }

   return true;
}

bool Sequence::demarshal(unsigned char *&buffer, unsigned int &buflen)
{
   // Free any existing data
   clear();

   // Unmarshal the sequence data length
   unsigned int datalen;
   if (!demarshalLength(buffer, buflen, datalen) || 
       datalen < 1 || buflen < datalen)
   {
      return false;
   }

   // Unmarshal items from the stream until sequence data is exhausted
   unsigned char *start = buffer;
   while (datalen)
   {
      Object *obj = Object::Demarshal(buffer, datalen);
      if (!obj)
         return false;
      Append(obj);
   }

   buflen -= buffer-start;
   return true;
}

void Sequence::Append(Object *obj)
{
   // realloc ... not efficient, but easy
   Object **tmp = new Object *[_size+1];
   memcpy(tmp, _data, _size * sizeof(_data[0]));
   tmp[_size++] = obj;
   delete [] _data;
   _data = tmp;
}

Object *Sequence::operator[](unsigned int idx)
{
   if (idx >= _size)
      return NULL;
   return _data[idx];
}

Sequence &Sequence::operator=(const Sequence &rhs)
{
   if (this != &rhs)
      assign(rhs);
   return *this;
}

void Sequence::assign(const Sequence &rhs)
{
   clear();

   _type = rhs._type;
   _size = rhs._size;
   _data = new Object *[_size];

   for (unsigned int i = 0; i < _size; i++)
      _data[i] = rhs._data[i]->copy();
}
