/*
 * aarray.h
 *
 * Simple dynamic array template class.
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

#ifndef __AARRAY_H
#define __AARRAY_H

template <class T> class aarray
{
public:
   aarray() : _data(NULL), _size(0) {}
   aarray(const aarray<T> &rhs) : _data(NULL), _size(0) { *this = rhs; }
   ~aarray() { delete [] _data; }

   aarray<T> &operator=(const aarray<T> &rhs)
   {
      if (this != &rhs)
      {
         delete [] _data;
         _size = 0;

         if (rhs._size)
         {
            _size = rhs._size;
            _data = new T[_size];
            for (unsigned int i = 0; i < _size; i++)
               _data[i] = rhs._data[i];
         }
      }
      return *this;
   }

   void append(const T& t)
   {
      T *tmp = new T[_size+1];
      for (unsigned int i = 0; i < _size; i++)
         tmp[i] = _data[i];
      delete [] _data;
      _data = tmp;
      _data[_size++] = t;
   }

   unsigned int size() const { return _size; }

   T& operator[](unsigned int idx) { return _data[idx]; }
   const T& operator[](unsigned int idx) const { return _data[idx]; }

private:
   T *_data;
   unsigned int _size;
};

#endif
