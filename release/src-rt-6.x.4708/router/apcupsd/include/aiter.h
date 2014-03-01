/*
 * aiter.h
 *
 * Iterator functions for aXXX classes.
 */

/*
 * Copyright (C) 2008 Adam Kropelin
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

#ifndef __AITER_H
#define __AITER_H

template <class I, class T>
class const_iterator
{
public:
   const_iterator() {}
   const_iterator(const const_iterator &rhs) : _iter(rhs._iter) {}
   const_iterator(const I &rhs)              : _iter(rhs)       {}

   const_iterator &operator++() { ++_iter; return *this; }
   const_iterator operator++(int) { const_iterator tmp(_iter); ++_iter; return tmp; }
   const_iterator &operator--() { --_iter; return *this; }
   const_iterator operator--(int) { const_iterator tmp(_iter); --_iter; return tmp; }

   const T& operator*() const { return *_iter; }

   bool operator==(const const_iterator &rhs) const { return _iter == rhs._iter; }
   bool operator!=(const const_iterator &rhs) const { return !(*this == rhs); }
   bool operator==(const I &rhs) const { return _iter == rhs; }
   bool operator!=(const I &rhs) const { return !(*this == rhs); }

   const_iterator &operator=(const const_iterator &rhs)
      { if (&rhs != this) _iter = rhs._iter; return *this; }
   const_iterator &operator=(const I &rhs)
      { if (&rhs != &_iter) _iter = rhs; return *this; }

private:
   I _iter;
};

#endif // __AITER_H
