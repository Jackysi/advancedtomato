/*
 * amap.h
 *
 * Simple map template class build on alist.
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

#ifndef __AMAP_H
#define __AMAP_H

#include "alist.h"
#include "aiter.h"

template <class K, class V>
class amap
{
private:

   struct keyval
   {
      keyval() {}
      keyval(const K& key) : _key(key) {}
      bool operator==(const keyval &rhs) const { return _key == rhs._key; }
      K _key;
      V _val;
   };

public:

   amap() {}
   ~amap() {}

   V& operator[](const K& key)
   {
      typename alist<keyval>::iterator iter = _map.find(key);
      return (iter == _map.end()) ? _map.append(key)._val : (*iter)._val;
   }

   bool contains(const K& key) const { return _map.find(key) != _map.end(); }
   bool empty() const { return _map.empty(); }

   class iterator
   {
   public:
      iterator() {}
      iterator(const iterator &rhs) : _iter(rhs._iter) {}

      iterator &operator++() { ++_iter; return *this; }
      iterator operator++(int) { iterator tmp(_iter); ++(*this); return tmp; }
      iterator &operator--() { --_iter; return *this; }
      iterator operator--(int) { iterator tmp(_iter); --(*this); return tmp; }

      V& value() { return (*_iter)._val; }
      K& key() { return (*_iter)._key; }
      V& operator*() { return value(); }

      bool operator==(const iterator &rhs) const { return _iter == rhs._iter; }
      bool operator!=(const iterator &rhs) const { return !(*this == rhs); }

      iterator &operator=(const iterator &rhs)
         { if (&rhs != this) _iter = rhs._iter; return *this;}

   protected:
      friend class amap;
      iterator(const typename alist<keyval>::iterator &iter) : _iter(iter) {}
      typename alist<keyval>::iterator _iter;
   };

   class const_iterator
   {
   public:
      const_iterator() {}
      const_iterator(const const_iterator &rhs) : _iter(rhs._iter) {}

      const_iterator &operator++() { ++_iter; return *this; }
      const_iterator operator++(int) { const_iterator tmp(_iter); ++(*this); return tmp; }
      const_iterator &operator--() { --_iter; return *this; }
      const_iterator operator--(int) { const_iterator tmp(_iter); --(*this); return tmp; }

      const V& value() const { return (*_iter)._val; }
      const K& key() const { return (*_iter)._key; }
      const V& operator*() const { return value(); }

      bool operator==(const const_iterator &rhs) const { return _iter == rhs._iter; }
      bool operator!=(const const_iterator &rhs) const { return !(*this == rhs); }

      const_iterator &operator=(const const_iterator &rhs)
         { if (&rhs != this) _iter = rhs._iter; return *this;}

   protected:
      friend class amap;
      const_iterator(const typename alist<keyval>::const_iterator &iter) : _iter(iter) {}
      typename alist<keyval>::const_iterator _iter;
   };

   iterator end() { return iterator(_map.end()); }
   iterator begin() { return iterator(_map.begin()); }
   const_iterator end() const { return _map.end(); }
   const_iterator begin() const { return _map.begin(); }

   iterator find(const K& key) { return iterator(_map.find(key)); }
   const_iterator find(const K& key) const { return _map.find(key); }

private:

   // Should really use a hash table for efficient random lookups,
   // but for now we'll take the easy route and use alist.
   alist<keyval> _map;

   // Prevent use
   amap(const amap<K,V> &rhs);
   amap<K,V> &operator=(const amap<K,V> &rhs);
};

#endif // __AMAP_H
