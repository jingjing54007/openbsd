/*	$OpenBSD: src/lib/libmenu/m_item_use.c,v 1.4 1998/07/24 16:39:04 millert Exp $	*/

/****************************************************************************
 * Copyright (c) 1998 Free Software Foundation, Inc.                        *
 *                                                                          *
 * Permission is hereby granted, free of charge, to any person obtaining a  *
 * copy of this software and associated documentation files (the            *
 * "Software"), to deal in the Software without restriction, including      *
 * without limitation the rights to use, copy, modify, merge, publish,      *
 * distribute, distribute with modifications, sublicense, and/or sell       *
 * copies of the Software, and to permit persons to whom the Software is    *
 * furnished to do so, subject to the following conditions:                 *
 *                                                                          *
 * The above copyright notice and this permission notice shall be included  *
 * in all copies or substantial portions of the Software.                   *
 *                                                                          *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS  *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF               *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   *
 * IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,   *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR    *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR    *
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 *                                                                          *
 * Except as contained in this notice, the name(s) of the above copyright   *
 * holders shall not be used in advertising or otherwise to promote the     *
 * sale, use or other dealings in this Software without prior written       *
 * authorization.                                                           *
 ****************************************************************************/

/****************************************************************************
 *   Author: Juergen Pfeifer <Juergen.Pfeifer@T-Online.de> 1995,1997        *
 ****************************************************************************/

/***************************************************************************
* Module m_item_use                                                        *
* Associate application data with menu items                               *
***************************************************************************/

#include "menu.priv.h"

MODULE_ID("$From: m_item_use.c,v 1.8 1998/02/11 12:13:50 tom Exp $")

/*---------------------------------------------------------------------------
|   Facility      :  libnmenu  
|   Function      :  int set_item_userptr(ITEM *item, void *userptr)
|   
|   Description   :  Set the pointer that is reserved in any item to store
|                    application relevant informations.  
|
|   Return Values :  E_OK               - success
+--------------------------------------------------------------------------*/
int set_item_userptr(ITEM * item, void * userptr)
{
  Normalize_Item(item)->userptr = userptr;
  RETURN( E_OK );
}

/*---------------------------------------------------------------------------
|   Facility      :  libnmenu  
|   Function      :  void *item_userptr(const ITEM *item)
|   
|   Description   :  Return the pointer that is reserved in any item to store
|                    application relevant informations.
|
|   Return Values :  Value of the pointer. If no such pointer has been set,
|                    NULL is returned.
+--------------------------------------------------------------------------*/
void *item_userptr(const ITEM * item)
{
  return Normalize_Item(item)->userptr;
}

/* m_item_use.c */
