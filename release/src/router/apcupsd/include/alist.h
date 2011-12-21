/*
 * alist.h
 *
 * Simple double linked list template class.
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

#ifndef __ALIST_H
#define __ALIST_H

#include <stdlib.h>
#include "aiter.h"

template <class T> class alist
{
private:

   class node
   {
   public:
      node(const T& elem) : _next(NULL), _prev(NULL), _elem(elem) {}

      ~node() {  if (_prev) _prev->_next = _next;
                 if (_next) _next->_prev = _prev; }

      operator T&() { return _elem; }
      T& operator*() { return _elem; }

      void next(node *link) { link->_next = _next;
                              link->_prev = this;
                              if (_next) _next->_prev = link;
                              _next = link; }
      void prev(node *link) { link->_next = this;
                              link->_prev = _prev;
                              if (_prev) _prev->_next = link;
                              _prev = link; }

      node *next() { return _next; }
      node *prev() { return _prev; }

      bool operator==(const node &rhs) const
         { return _next == rhs._next && _prev == rhs._prev; }

   private:
      node *_next, *_prev;
      T _elem;
   };

public:

   alist() : _head(NULL), _tail(NULL), _size(0) {}
   alist(const alist<T> &rhs) : _head(NULL), _tail(NULL), _size(0)
      { append(rhs); }

   virtual ~alist()
   {
      while (!empty()) remove(begin());
      _size = 0;
   }

   T& first() { return *_head; }
   T& last() { return *_tail; }

   bool empty() const { return _size <= 0; }
   unsigned int size() const { return _size; }

   T& append(const T& elem)
   {
      node *nd = new node(elem);
      if (_tail)
         _tail->next(nd);
      else
         _head = nd;
      _tail = nd;
      _size++;
      return *_tail;
   }

   T& prepend(const T& elem)
   {
      node *nd = new node(elem);
      if (_head)
         _head->prev(nd);
      else
         _tail = nd;
      _head = nd;
      _size++;
      return *_head;
   }

   T& append(const alist<T>& rhs)
   {
      if (&rhs != this)
      {
         for(const_iterator iter = rhs.begin();
             iter != rhs.end();
             ++iter)
         {
            append(*iter);
         }
      }
      return *_tail;
   }

   alist<T>& operator+=(const alist<T>& rhs)
   {
      append(rhs);
      return *this;
   }

   alist<T>& operator=(const alist<T>& rhs)
   {
      if (&rhs != this)
      {
         clear();
         append(rhs);
      }
      return *this;
   }

   void remove_first() { if (!empty()) remove(_head); }
   void remove_last()  { if (!empty()) remove(_tail); }
   void clear() { while (!empty()) remove(_head); }

   class iterator
   {
   public:
      iterator() : _node(NULL) {}
      iterator(const iterator &rhs) : _node(rhs._node) {}

      iterator &operator++() { _node = _node->next(); return *this; }
      iterator operator++(int) { iterator tmp(_node); ++(*this); return tmp; }
      iterator &operator--() { _node = _node->prev(); return *this; }
      iterator operator--(int) { iterator tmp(_node); --(*this); return tmp; }

      T& operator*() { return *_node; }
      const T& operator*() const { return *_node; }
      T* operator->() { return &(**_node); }
      const T* operator->() const { return &(**_node); }

      bool operator==(const iterator &rhs) const { return _node == rhs._node; }
      bool operator!=(const iterator &rhs) const { return !(*this == rhs); }

      iterator &operator=(const iterator &rhs)
         { if (&rhs != this) _node = rhs._node; return *this; }

   private:
      friend class alist;
      iterator(node *node) : _node(node) {}
      node *_node;
   };

   typedef ::const_iterator<iterator, T> const_iterator;

   iterator begin() { return iterator(_head); }
   iterator end() { return iterator(NULL); }
   const_iterator begin() const { return iterator(_head); }
   const_iterator end() const { return iterator(NULL); }

   iterator remove(iterator iter) {
      if (iter == _head) _head = iter._node->next();
      if (iter == _tail) _tail = iter._node->prev();
      iterator newiter(iter._node->next());
      delete iter._node;
      _size--;
      return newiter;
   }

   iterator find(const T& needle) {
      iterator iter;
      for (iter = begin(); iter != end(); ++iter)
         if (*iter == needle) break;
      return iter;
   }

   const_iterator find(const T& needle) const {
      const_iterator iter;
      for (iter = begin(); iter != end(); ++iter)
         if (*iter == needle) break;
      return iter;
   }

private:

   node *_head, *_tail;
   unsigned int _size;
};

#endif
