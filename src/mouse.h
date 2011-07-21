/*
 * mouse.h
 * Copyright Sean Stangl <sean.stangl@gmail.com> 2005-2011
 *
 * This file is part of Mousepad.
 *
 * Mousepad is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mousepad is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with mousepad.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __mousepad_mouse_h__
#define __mousepad_mouse_h__

#include "mousepad.h"

#include <X11/X.h>
#include <X11/Xlib.h>

typedef struct
{
	float xv, yv; /* velocities */
	float xa, ya; /* accelerations */
	int   x, y;   /* positions */
} mouse_t;

#define MOUSE_BUTTON_LEFT Button1
#define MOUSE_BUTTON_RIGHT 3

int mouse_init(Display *d);
void mouse_begin();
void mouse_end();
void mouse_move(int xdelta, int ydelta);
void mouse_click(unsigned button);
void mouse_close_focused_window();
void mouse_tick();
void mouse_event(buttonstate_t buttons, button_t changed);

#endif /* __mousepad_mouse_h__ */

