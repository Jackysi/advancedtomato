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

#include <windows.h>
#include <commctrl.h>
#include "meter.h"

Meter::Meter(HWND hwnd, UINT id, int warn, int critical, int level) :
   _warn(warn),
   _critical(critical),
   _level(level)
{
   _hwnd = GetDlgItem(hwnd, id);
}

void Meter::Set(int level)
{
   if (level == _level)
      return;

   SendMessage(_hwnd, PBM_SETPOS, level, 0);
   _level = level;

   // Figure out bar color. 
   COLORREF color;
   if (_warn > _critical)
   {
      // Low is critical
      if (level > _warn)
         color = GREEN;
      else if (level > _critical)
         color = YELLOW;
      else
         color = RED;
   }
   else
   {
      // High is critical
      if (level >= _critical)
         color = RED;
      else if (level >= _warn)
         color = YELLOW;
      else
         color = GREEN;
   }

   SendMessage(_hwnd, PBM_SETBARCOLOR, 0, color);
}
