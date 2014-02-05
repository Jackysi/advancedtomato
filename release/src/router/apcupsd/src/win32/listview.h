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

#ifndef __LISTVIEW_H
#define __LISTVIEW_H

#include <windows.h>
#include "astring.h"
#include "alist.h"

class ListView
{
public:
   ListView(HWND hwnd, UINT id, int cols);
   ~ListView() {}

   int AppendItem(const char *text);
   void UpdateItem(int item, int sub, const char *text);
   int NumItems();
   void Autosize();
   void DeleteItem(int item);
   void UpdateAll(alist<astring>* data[]);

private:
   HWND _hwnd;
   int _cols;
};

#endif
