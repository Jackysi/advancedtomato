/*
 * apclist.c
 *
 * UPS linked list functions.
 */

/*
 * Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
 * Copyright (C) 1999-2001 Riccardo Facchetti <riccardo@master.oasi.gpa.it>
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

#include "apc.h"

static UPSINFO *upshead = NULL;

/*
 * The linked list need to be defined in _all_ the forked processes.
 * The syncronization of data into this structure is done with the shared
 * memory area so this is made reentrant by the shm mechanics.
 */

int insertUps(UPSINFO *ups)
{
   UPSINFO *ptr = upshead;

   if (ptr == NULL) {
      upshead = ups;
   } else {
      while (ptr->next)
         ptr = ptr->next;
      ptr->next = ups;
   }

   return SUCCESS;
}

UPSINFO *getNextUps(UPSINFO *ups)
{
   if (ups == NULL)
      return upshead;

   return ups->next;
}

UPSINFO *getUpsByname(char *name)
{
   UPSINFO *ups = NULL;

   for (ups = NULL; (ups = getNextUps(ups)) != NULL;) {
      if (strncmp(name, ups->upsname, strlen(ups->upsname)) == 0)
         return ups;
   }

   return NULL;
}
