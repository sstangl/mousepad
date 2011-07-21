/*
 * mouse.c
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

#include "mouse.h"
#include "keyboard.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>

#define MOTION_DAMP 1.0
#define MOUSE_DELAY_MILLISECONDS 10
#define MOUSE_MAX_VELOCITY 30.0
#define MOUSE_VELOCITY 2
#define MOUSE_ACCELERATION 1

mouse_t mouse;
Display *display;

struct timespec prevtime;  // Time since last mouse_tick().

/* Returns the difference in milliseconds between two times. */
static int millidiff(struct timespec time, struct timespec prev)
{
	return abs(((time.tv_sec  - prev.tv_sec)  * 1000) +
	           ((time.tv_nsec - prev.tv_nsec) / 1000000));
}

/* Mouse initialization. */
int mouse_init(Display *d)
{
	if (d == NULL) return -1;

	display = d;
	clock_gettime(CLOCK_REALTIME, &prevtime);
	return 0;
}

/*
 * Begin mouse mode.
 * Either the program is starting, or the mouse has been switched to.
 */
void mouse_begin()
{
	memset(&mouse, 0x0, sizeof(mouse_t));
}

/* End mouse mode. */
void mouse_end()
{
	return;
}

/* Moves the mouse relatively. */
void mouse_move(int xdelta, int ydelta)
{
	Window w;
	unsigned tmp;
	int tmp2;

	/* Obtain mouse position. */
	XQueryPointer(display, RootWindow(display, DefaultScreen(display)), &w, &w,
	              &mouse.x, &mouse.y, &tmp2, &tmp2, &tmp);

	/* Move mouse. */
	XWarpPointer(display, None, RootWindow(display, DefaultScreen(display)),
	             0, 0, 0, 0, mouse.x + xdelta, mouse.y + ydelta);
	XFlush(display);
}

/* 
 * Causes a mouse click of the specified mouse button.
 * Options are MOUSE_BUTTON_LEFT and MOUSE_BUTTON_RIGHT.
 */
void mouse_click(unsigned button)
{

	XTestFakeButtonEvent(display, button, 1, 0);
	XTestFakeButtonEvent(display, button, 0, 0);

	XFlush(display);
}

/* Closes the currently focused window by sending an XDestroy message. */
void mouse_close_focused_window()
{
	// TODO: Don't use XDestroy.
	Window focused;
	int revert;

	XGetInputFocus(display, &focused, &revert);
	if (focused != None)
		XDestroyWindow(display, focused);
}

/* Handle a tick event by moving the cursor if necessary. */
void mouse_tick()
{
	if (mouse.xa == 0 && mouse.ya == 0)
		return;

	/* Time-based delay. */
	struct timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	if (millidiff(time, prevtime) < MOUSE_DELAY_MILLISECONDS)
		return;

	/* Update velocities. */
	float curve = ((float)millidiff(time, prevtime))/1000.0*MOTION_DAMP;

	if (mouse.xa == 0)
		mouse.xv = 0;
	else
		mouse.xv += mouse.xa * curve;

	if (mouse.ya == 0)
		mouse.yv = 0;
	else
		mouse.yv += mouse.ya * curve;

	/* Cap velocities. */
	if (abs((int)mouse.xv) > MOUSE_MAX_VELOCITY)
		mouse.xv = MOUSE_MAX_VELOCITY;
	if (abs((int)mouse.yv) > MOUSE_MAX_VELOCITY)
		mouse.yv = MOUSE_MAX_VELOCITY;

	/* Handle movement. */
	mouse_move((int)mouse.xv, (int)mouse.yv);
	prevtime = time;
}

/* Handle a mouse event by performing an action or changing mouse state. */
void mouse_event(buttonstate_t buttons, button_t changed)
{
	if (!changed) return;

	/* Press left and right buttons at the same time to left-click. */
	if (buttons == (BUTTON_LEFT | BUTTON_RIGHT) &&
	    (changed & (BUTTON_LEFT | BUTTON_RIGHT))) {
		mouse_click(MOUSE_BUTTON_LEFT);
		return;
	}

	/* Press up and down buttons at the same time to right-click. */
	if (buttons == (BUTTON_UP | BUTTON_DOWN) &&
	    (changed & (BUTTON_UP | BUTTON_DOWN))) {
		mouse_click(MOUSE_BUTTON_RIGHT);
		return;
	}

	/* Press the up-left button (and no other button) to close the window. */
	if (buttons == BUTTON_UPLEFT && changed == BUTTON_UPLEFT) {
		mouse_close_focused_window();
		return;
	}

	/* Press the up-right button (and no other button) to scroll up. */
	if (buttons == BUTTON_UPRIGHT && changed == BUTTON_UPRIGHT) {
		keyboard_press(XK_Page_Up);
		return;
	}

	/* Press the down-right button (and no other button) to scroll down. */
	if (buttons == BUTTON_DOWNRIGHT && changed == BUTTON_DOWNRIGHT) {
		keyboard_press(XK_Page_Down);
		return;
	}
	
	/* 
	 * If a cardinal button is pushed, accelerate in that direction,
	 * or halt motion in that direction. This permits moving diagonally
	 * by pressing multiple cardinal buttons.
	 */
	int accel = 0;
	int vel   = 0;
	if (buttons & changed) {
		accel = MOUSE_ACCELERATION;
		vel   = MOUSE_VELOCITY;
	}

	switch (changed) {
		case BUTTON_LEFT:
			mouse.xa = -accel;
			mouse.xv = -vel;
			break;

		case BUTTON_UP:
			mouse.ya = -accel;
			mouse.yv = -vel;
			break;

		case BUTTON_RIGHT:
			mouse.xa = accel;
			mouse.xv = vel;
			break;

		case BUTTON_DOWN:
			mouse.ya = accel;
			mouse.yv = vel;
			break;

		default:
			break;
	}

	return;
}

