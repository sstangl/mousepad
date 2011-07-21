/*
 * keyboard.c
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

#include "keyboard.h"

#define KEYBOARD_STATE_NONE 0
#define KEYBOARD_STATE_LEFT 1
#define KEYBOARD_STATE_UP 2
#define KEYBOARD_STATE_RIGHT 3
#define KEYBOARD_STATE_DOWN 4
#define KEYBOARD_STATE_UPLEFT 5
#define KEYBOARD_STATE_UPRIGHT 6
#define KEYBOARD_STATE_DOWNRIGHT 7
#define KEYBOARD_STATE_DOWNLEFT 8

Display *display;
int shift = 0; // State of the shift toggle: nonzero if active.

/* Keyboard initialization. */
int keyboard_init(Display *d)
{
	if (d == NULL) return -1;

	display = d;
	return 0;
}

/* Internal wrapper for XTestFakeKeyEvent. */
static inline int keyboard_keyevent(unsigned key, int pushed)
{
	return XTestFakeKeyEvent(display, XKeysymToKeycode(display, key),
	                         pushed, CurrentTime);
}

/* Send a key press event to the current window. */
void keyboard_press(unsigned key)
{
	if (shift)
		keyboard_keyevent(XK_Shift_R, True);

	keyboard_keyevent(key, True);  // press key
	keyboard_keyevent(key, False); // release key

	if (shift)
		keyboard_keyevent(XK_Shift_R, False);
}

