/*	$OpenBSD: src/lib/libcurses/Attic/lib_move.c,v 1.4 1998/07/23 21:19:02 millert Exp $	*/

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
 *  Author: Zeyd M. Ben-Halim <zmbenhal@netcom.com> 1992,1995               *
 *     and: Eric S. Raymond <esr@snark.thyrsus.com>                         *
 ****************************************************************************/


/*
**	lib_move.c
**
**	The routine wmove().
**
*/

#include <curses.priv.h>

MODULE_ID("$From: lib_move.c,v 1.8 1998/02/11 12:13:53 tom Exp $")

int
wmove(WINDOW *win, int y, int x)
{
	T((T_CALLED("wmove(%p,%d,%d)"), win, y, x));

	if (win && 
	    x >= 0  &&  x <= win->_maxx  &&
	    y >= 0  &&  y <= win->_maxy)
	{
		win->_curx = (short)x;
		win->_cury = (short)y;

		win->_flags &= ~_WRAPPED;
		win->_flags |= _HASMOVED;
		returnCode(OK);
	} else
		returnCode(ERR);
}
