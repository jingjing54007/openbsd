/*	$OpenBSD: src/lib/libpanel/p_show.c,v 1.3 1999/11/28 17:49:19 millert Exp $	*/

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
 *  Author: Zeyd M. Ben-Halim <zmbenhal@netcom.com> 1995                    *
 *     and: Eric S. Raymond <esr@snark.thyrsus.com>                         *
 ****************************************************************************/

/* p_show.c
 * Place a panel on top of the stack; may already be in the stack 
 */
#include "panel.priv.h"

MODULE_ID("$From: p_show.c,v 1.7 1999/11/25 13:49:26 juergen Exp $")

int
show_panel(PANEL *pan)
{ 
  int err = OK;

  if(!pan)
    return(ERR);

  if (Is_Top(pan))
    return(OK);

  dBug(("--> show_panel %s", USER_PTR(pan->user)));

  HIDE_PANEL(pan,err,FALSE);

  dStack("<lt%d>",1,pan);
  assert(_nc_bottom_panel == _nc_stdscr_pseudo_panel);

  _nc_top_panel->above = pan;
  pan->below = _nc_top_panel;  
  pan->above = (PANEL *)0;
  _nc_top_panel = pan;
  
  dStack("<lt%d>",9,pan);

  return(OK);
}
