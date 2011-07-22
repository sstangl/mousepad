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
#include "keygtk.h"

Display *display;
int shift = 0; // State of the shift toggle: nonzero if active.

/* Keyboard initialization. */
int keyboard_init(Display *d)
{
	if (d == NULL) return -1;

	display = d;
	return keygtk_init(d);
}

int keyboard_begin()
{
	shift = 0;
	keygtk_set_layout(0x0);
	keygtk_window_show();
	return 0;
}

int keyboard_end()
{
	keygtk_window_hide();
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

/*
 * The keyboard is modal: pressing the first button sets the mode,
 *  modifying keyboard layout; the second button then selects the letter
 *  from those available within that mode.
 */
void keyboard_event(buttonstate_t buttons, button_t changed)
{
	static int layout = 0x0;

	if (!changed) return;

	if (!buttons) {
		layout = 0x0;
		keygtk_set_layout(layout);
		return;
	}

	/* Disregard general releases. */
	if (!(buttons & changed)) {
		return;
	}

	/* If only one button is pressed, set the layout. */
	if (buttons && (buttons & (buttons - 1)) == 0) {
		if (buttons != BUTTON_START && buttons != BUTTON_BACK) {
			layout = buttons;
			keygtk_set_layout(layout);
		}
		return;
	}

	// TODO: Handle double-tapping for arrow key pressing.
	
	/* Otherwise, type the key that was just selected. */
	int k = 0;
	switch (layout) {
		case BUTTON_LEFT: {
			switch (changed) {
				case BUTTON_UPLEFT:    k = XK_a; break;
				case BUTTON_UP:        k = XK_b; break;
				case BUTTON_UPRIGHT:   k = XK_c; break;
				case BUTTON_RIGHT:     k = XK_d; break;
				case BUTTON_DOWNRIGHT: k = XK_e; break;
				case BUTTON_DOWN:      k = XK_f; break;
				case BUTTON_DOWNLEFT:  k = XK_g; break;
				default: return;
			}
			break;
		}

		case BUTTON_UP: {
			switch (changed) {
				case BUTTON_UPRIGHT:   k = XK_h; break;
				case BUTTON_RIGHT:     k = XK_i; break;
				case BUTTON_DOWNRIGHT: k = XK_j; break;
				case BUTTON_DOWN:      k = XK_k; break;
				case BUTTON_DOWNLEFT:  k = XK_l; break;
				case BUTTON_LEFT:      k = XK_m; break;
				case BUTTON_UPLEFT:    k = XK_n; break;
				default: return;
			}
			break;
		}

		case BUTTON_RIGHT: {
			switch (changed) {
				case BUTTON_DOWNRIGHT: k = XK_o; break;
				case BUTTON_DOWN:      k = XK_p; break;
				case BUTTON_DOWNLEFT:  k = XK_q; break;
				case BUTTON_LEFT:      k = XK_r; break;
				case BUTTON_UPLEFT:    k = XK_s; break;
				case BUTTON_UP:        k = XK_t; break;
				case BUTTON_UPRIGHT:   k = XK_u; break;
				default: return;
			}
			break;
		}

		case BUTTON_DOWN: {
			switch (changed) {
				case BUTTON_DOWNLEFT:  k = XK_v; break;
				case BUTTON_LEFT:      k = XK_w; break;
				case BUTTON_UPLEFT:    k = XK_x; break;
				case BUTTON_UP:        k = XK_y; break;
				case BUTTON_UPRIGHT:   k = XK_z; break;
				case BUTTON_RIGHT:     k = XK_BackSpace; break;
				case BUTTON_DOWNRIGHT: k = XK_slash; break;
				default: return;
			}
			break;
		}

		case BUTTON_UPLEFT: {
			switch (changed) {
				case BUTTON_UP:        k = XK_0; break;
				case BUTTON_UPRIGHT:   k = XK_1; break;
				case BUTTON_RIGHT:     k = XK_2; break;
				case BUTTON_DOWNRIGHT: k = XK_3; break;
				case BUTTON_DOWN:      k = XK_4; break;
				case BUTTON_DOWNLEFT:  k = XK_5; break;
				case BUTTON_LEFT:      k = XK_6; break;
				default: return;
			}
			break;
		}

		case BUTTON_UPRIGHT: {
			switch (changed) {
				case BUTTON_RIGHT:     k = XK_7; break;
				case BUTTON_DOWNRIGHT: k = XK_8; break;
				case BUTTON_DOWN:      k = XK_9; break;
				case BUTTON_DOWNLEFT:  k = XK_apostrophe; break;
				case BUTTON_LEFT:      k = XK_question; break;
				case BUTTON_UPLEFT:    k = XK_comma; break;
				case BUTTON_UP:        k = XK_period; break;
				default: return;
			}
			break;
		}

		default: return;
	}

	keyboard_press(k);
}

