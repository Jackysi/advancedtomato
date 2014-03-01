/*
 * astring.h
 *
 * Simple string management class. Like STL, but lighter.
 */

/*
 * Copyright (C) 2007 Adam Kropelin
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

#ifndef __ASTRING_H
#define __ASTRING_H

#include <string.h>
#include <stdarg.h>

class astring
{
public:

   astring()                   : _data(NULL) { assign(""); }
   astring(const char *str)    : _data(NULL) { assign(str); }
   astring(const astring &str) : _data(NULL) { assign(str._data); }
   ~astring() { delete [] _data; }

   int len() const { return _len; }

   int format(const char *format, ...);
   int vformat(const char *format, va_list args);

   astring &operator=(const astring &rhs);
   astring &operator=(const char *rhs);
   astring &operator=(const char rhs);

   const char &operator[](int index) const;
   char &operator[](int index);

   astring &operator+(const char *rhs);
   astring &operator+=(const char *rhs) { return *this + rhs; }
   astring &operator+(const astring &rhs);
   astring &operator+=(const astring &rhs) { return *this + rhs; }
   astring &operator+(const char rhs);
   astring &operator+=(const char rhs) { return *this + rhs; }

   bool operator==(const char *rhs) const { return !strcmp(_data, rhs); }
   bool operator==(const astring &rhs) const { return *this == rhs._data; }
   bool operator!=(const char *rhs) const { return !(*this == rhs); }
   bool operator!=(const astring &rhs) const { return !(*this == rhs); }

   astring substr(int start, int len = -1) const;
   int strchr(char ch) const;

   operator const char *() const { return _data; }
   const char *str() const { return _data; }

   astring &rtrim();
   astring &ltrim();
   astring &trim() { ltrim(); return rtrim(); }

   bool empty() const { return _len == 0; }

   int compare(const char *rhs) const { return strcmp(_data, rhs); }

private:

   void realloc(unsigned int newlen);
   void assign(const char *str, int len = -1);

   char *_data;
   int _len;
};

inline astring operator+(const char *lhs, const astring &rhs)
   { return astring(lhs) + rhs; }

#endif
