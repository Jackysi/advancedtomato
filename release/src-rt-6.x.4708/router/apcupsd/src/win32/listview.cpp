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
#include <limits.h>
#include "listview.h"

ListView::ListView(HWND hwnd, UINT id, int cols) :
   _cols(cols)
{
   _hwnd = GetDlgItem(hwnd, id);

   LVCOLUMN lvc;
   lvc.mask = LVCF_SUBITEM;
   for (int i = 0; i < cols; i++)
   {
      lvc.iSubItem = i;
      SendMessage(_hwnd, LVM_INSERTCOLUMN, i, (LONG)&lvc);
   }
}

int ListView::AppendItem(const char *text)
{
   LVITEM lvi;
   lvi.mask = LVIF_TEXT;
   lvi.iItem = INT_MAX;
   lvi.iSubItem = 0;
   lvi.pszText = (char*)text;

   return SendMessage(_hwnd, LVM_INSERTITEM, 0, (LONG)&lvi);
}

void ListView::UpdateItem(int item, int sub, const char *text)
{
   char str[256];

   LVITEM lvi;
   lvi.mask = LVIF_TEXT;
   lvi.iItem = item;
   lvi.iSubItem = sub;
   lvi.pszText = str;
   lvi.cchTextMax = sizeof(str);

   int len = SendMessage(_hwnd, LVM_GETITEMTEXT, item, (LONG)&lvi);
   if (len == 0 || strcmp(str, text))
   {
      lvi.pszText = (char*)text;
      SendMessage(_hwnd, LVM_SETITEMTEXT, item, (LONG)&lvi);
   }
}

int ListView::NumItems()
{
   return SendMessage(_hwnd, LVM_GETITEMCOUNT, 0, 0);
}

void ListView::Autosize()
{
   for (int i = 0; i < _cols; i++)
      SendMessage(_hwnd, LVM_SETCOLUMNWIDTH, i, LVSCW_AUTOSIZE);
}

void ListView::DeleteItem(int item)
{
   SendMessage(_hwnd, LVM_DELETEITEM, item, 0);
}

void ListView::UpdateAll(alist<astring>* data[])
{
   // The simple way to update the listview would be to remove all items and
   // then add them again. However, that causes the control to flicker and the
   // scrollbar to reset to the top every time, which makes it pretty much
   // unusable. To prevent that, we update the items in-place, adding new ones
   // and removing unused ones as necessary. That way the scroll position stays
   // put and only the items that change are redrawn.

   // Get current item count and prepare to update the listview
   int num = NumItems();
   int count = 0;

   // Add each line to the listview
   alist<astring>::const_iterator iter;
   for (iter = data[0]->begin(); iter != data[0]->end(); ++iter)
   {
      // Set main item (leftmost column). This will be an insert if there is no 
      // existing item at this position or an update if an item already exists.
      if (count >= num)
         AppendItem(*iter);
      else
         UpdateItem(count, 0, *iter);

      // On to the next item
      count++;
   }

   // Remove any leftover items that are no longer needed.
   while (count < num)
   {
      DeleteItem(count);
      num--;
   }

   // Update remaining columns. This is always an update since main item
   // (leftmost column) is guaranteed to exist by code above.
   for (int i = 1; i < _cols; i++)
   {
      int count = 0;
      alist<astring>::const_iterator iter;
      for (iter = data[i]->begin(); iter != data[i]->end(); ++iter)
         UpdateItem(count++, i, *iter);
   }

   // Autosize listview columns
   Autosize();
}
