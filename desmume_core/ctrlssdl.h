/* joysdl.h - this file is part of DeSmuME
 *
 * Copyright (C) 2007 Pascal Giard
 *
 * Author: Pascal Giard <evilynux@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef CTRLSSDL_H
#define CTRLSSDL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
//#include <pthread.h>
//#include <SDL.h>
#include "MMU.h"

#include "types.h"

#define ADD_KEY(keypad,key) ( (keypad) |= (key) )
#define RM_KEY(keypad,key) ( (keypad) &= ~(key) )
#define KEYMASK_(k)	(1 << (k))
#define JOY_AXIS_(k)    (((k)+1) << 8)

#define NB_KEYS		14
#define KEY_NONE		0
#define KEY_A			1
#define KEY_B			2
#define KEY_SELECT		3
#define KEY_START		4
#define KEY_RIGHT		5
#define KEY_LEFT		6
#define KEY_UP			7
#define KEY_DOWN		8
#define KEY_R			9
#define KEY_L			10
#define KEY_X			11
#define KEY_Y			12
#define KEY_DEBUG		13
#define KEY_BOOST		14
struct mouse_status
{
  signed long x;
  signed long y;
  BOOL click;
  BOOL down;
};

struct mouse_status mouse;
/* Update NDS keypad */
void update_keypad(u16 keys);
int
process_ctrls_events( u16 *keypad,
                      void (*external_videoResizeFn)( u16 width, u16 height),
                      float nds_screen_size_ratio);

#endif /* CTRLSSDL_H */
