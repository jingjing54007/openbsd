/*
 * Author: Tatu Ylonen <ylo@cs.hut.fi>
 * Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
 *                    All rights reserved
 * Macros for storing and retrieving data in msb first and lsb first order.
 *
 * As far as I am concerned, the code I have written for this software
 * can be used freely for any purpose.  Any derived versions of this
 * software must be clearly marked as such, and if the derived work is
 * incompatible with the protocol description in the RFC file, it must be
 * called by a name other than "ssh" or "Secure Shell".
 */

/* RCSID("$OpenBSD: src/usr.bin/ssh/Attic/getput.h,v 1.6 2000/12/19 23:17:56 markus Exp $"); */

#ifndef GETPUT_H
#define GETPUT_H

/*------------ macros for storing/extracting msb first words -------------*/

#define GET_32BIT(cp) (((u_long)(u_char)(cp)[0] << 24) | \
		       ((u_long)(u_char)(cp)[1] << 16) | \
		       ((u_long)(u_char)(cp)[2] << 8) | \
		       ((u_long)(u_char)(cp)[3]))

#define GET_16BIT(cp) (((u_long)(u_char)(cp)[0] << 8) | \
		       ((u_long)(u_char)(cp)[1]))

#define PUT_32BIT(cp, value) do { \
  (cp)[0] = (value) >> 24; \
  (cp)[1] = (value) >> 16; \
  (cp)[2] = (value) >> 8; \
  (cp)[3] = (value); } while (0)

#define PUT_16BIT(cp, value) do { \
  (cp)[0] = (value) >> 8; \
  (cp)[1] = (value); } while (0)

/*------------ macros for storing/extracting lsb first words -------------*/

#define GET_32BIT_LSB_FIRST(cp) \
  (((u_long)(u_char)(cp)[0]) | \
  ((u_long)(u_char)(cp)[1] << 8) | \
  ((u_long)(u_char)(cp)[2] << 16) | \
  ((u_long)(u_char)(cp)[3] << 24))

#define GET_16BIT_LSB_FIRST(cp) \
  (((u_long)(u_char)(cp)[0]) | \
  ((u_long)(u_char)(cp)[1] << 8))

#define PUT_32BIT_LSB_FIRST(cp, value) do { \
  (cp)[0] = (value); \
  (cp)[1] = (value) >> 8; \
  (cp)[2] = (value) >> 16; \
  (cp)[3] = (value) >> 24; } while (0)

#define PUT_16BIT_LSB_FIRST(cp, value) do { \
  (cp)[0] = (value); \
  (cp)[1] = (value) >> 8; } while (0)

#endif				/* GETPUT_H */
