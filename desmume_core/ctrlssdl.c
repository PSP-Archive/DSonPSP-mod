/* joysdl.c - this file is part of DeSmuME
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

#include "ctrlssdl.h"
#ifdef __psp__
	#include <pspctrl.h>
    #include "./psp/FrontEnd.h"
    #include "./psp/SystemButtons.h"
#endif
  
// Horizontal
#ifdef __psp__
const u16 default_psp_cfg_h[NB_KEYS] =
  { PSP_CTRL_CIRCLE,    //A
	PSP_CTRL_CROSS,     //B
	PSP_CTRL_SELECT,	//Select
	PSP_CTRL_START,		//Start
	PSP_CTRL_RIGHT,		//Right
	PSP_CTRL_LEFT,		//Left
	PSP_CTRL_UP,		//Up
	PSP_CTRL_DOWN,		//Down
	PSP_CTRL_RTRIGGER,	//R
	PSP_CTRL_LTRIGGER,	//L
	PSP_CTRL_TRIANGLE,  //X
	PSP_CTRL_SQUARE     //Y
  };
#endif

/* Update NDS keypad */
void update_keypad(u16 keys)
{
  ((u16 *)ARM9Mem.ARM9_REG)[0x130>>1] = ~keys & 0x3FF;
  ((u16 *)MMU.ARM7_REG)[0x130>>1] = ~keys & 0x3FF;
  /* Update X and Y buttons */
  MMU.ARM7_REG[0x136] = ( ~( keys >> 10) & 0x3 ) | (MMU.ARM7_REG[0x136] & ~0x3);
}
/* Manage input events */
int
process_ctrls_events( u16 *keypad,
                      void (*external_videoResizeFn)( u16 width, u16 height),
                      float nds_screen_size_ratio)
{
	  SceCtrlData pad;
	  sceCtrlPeekBufferPositive(&pad, 1); 

	  int i;
	  for(i=0;i<12;i++) {
			if (pad.Buttons & default_psp_cfg_h[i])
				ADD_KEY( *keypad, KEYMASK_(i));
			else
				RM_KEY( *keypad, KEYMASK_(i));
	  }
	  if (pad.Buttons & PSP_CTRL_NOTE) {
//		if (readNoteButton()) {
	  		mouse.click = TRUE;
			mouse.down = FALSE;
	  } else {
	  // holding down note should trigger this
//		  mouse.down = TRUE;

#define xmax 255
#define ymax 192
#define minmaxp(a,b) ((a+=1)<0?0:(a>b?b:a))
#define minmaxm(a,b) ((a-=1)<0?0:(a>b?b:a))

			if (pad.Lx < 10)
				mouse.x = minmaxm(mouse.x, xmax);

			if (pad.Lx > 245)
				mouse.x = minmaxp(mouse.x, xmax);

			if (pad.Ly < 10)
				mouse.y = minmaxm(mouse.x, ymax);
		
			if (pad.Ly > 245)
				mouse.y = minmaxp(mouse.x, ymax);
	  }
#undef xmax
#undef ymax
#undef minmaxp
#undef minmaxm
  return 0;
}

