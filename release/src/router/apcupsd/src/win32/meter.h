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

#ifndef __METER_H
#define __METER_H

#include <windows.h>

class Meter
{
public:
   Meter(HWND hwnd, UINT id, int warn, int critical, int level = 0);
   ~Meter() {}

   void Set(int level);

private:
   HWND _hwnd;
   int _warn;
   int _critical;
   int _level;

   static const COLORREF GREEN = RGB(115, 190, 49);
   static const COLORREF RED = RGB(214, 56, 57);
   static const COLORREF YELLOW = RGB(214, 186, 57);
};

#endif
