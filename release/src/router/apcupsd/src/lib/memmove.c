/*
 * memmove.c
 *
 * Implementation of memmove(), presumable for platforms that don't
 * already have it.
 */

/*
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

char *memmove(char *dst, register char *src, register int n)
{
   register char *svdst;

   if ((dst > src) && (dst < src + n)) {
      src += n;
      for (svdst = dst + n; n-- > 0;)
         *--svdst = *--src;
   } else {
      for (svdst = dst; n-- > 0;)
         *svdst++ = *src++;
   }

   return dst;
}
