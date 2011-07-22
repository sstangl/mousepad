/*
 * mousepad.c
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

#define PROGRAM_NAME "mousepad"
#define VERSION_NUMBER "0.3"

#include "config.h"
#include "keyboard.h"
#include "mouse.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <gtk/gtk.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/joystick.h>

#define MODE_MOUSE 0
#define MODE_KEYBOARD 1

int main (int argc, char *argv[])
{
	int mode = MODE_MOUSE;
	char *device = "/dev/input/js0";
	int joyfd;
	struct js_event jevent;
	int njoybtn;
	FILE *configfile;
	
	gtk_init(&argc, &argv);
	
	if (argc > 2) {
		fprintf(stderr, PROGRAM_NAME": Too many arguments.\n"); 
		return 1;
	}

	if (argc == 2) {
		if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
			fprintf(stdout, "Usage: %s [Joystick Device]\n", argv[0]);
			return 0;
		} else {
			device = argv[1];
		}
	}


	/* Initialize joystick. */
	// TODO: Make this blocking and have a separate thread for cursor.
	if ((joyfd = open(device, O_RDONLY | O_NONBLOCK)) < 0) {
		fprintf(stderr, " Could not open joystick device.\n");
		return 1;
	}
		
	/* Retrieve the number of buttons */
	ioctl(joyfd, JSIOCGBUTTONS, &njoybtn);
	if (njoybtn <= 0)
		return 1;

	/* Map from jevent.number to button bitfield. */
	int *joymap = malloc(njoybtn * sizeof(int));


	/* Read in configuration file */
	configfile = config_open();
	if (configfile == NULL) {
		fprintf(stderr, " Couldn't find a pad configuration file.\n"
		                " Run "PROGRAM_NAME"-config to build one.\n");
		return 1;
	}

	if (config_read(configfile, njoybtn, joymap) < 0) {
		fprintf(stderr, " Error parsing configuration file.\n");
		return 1;
	}
	config_close(configfile);
	

	/* Initialize event handlers. */
	Display *display = XOpenDisplay(NULL);
	if (mouse_init(display) < 0) return 1;
	if (keyboard_init(display) < 0) return 1;
	

	while (1) {
		mouse_begin();
		int buttons = 0;

		/* Main loop */
		while (1) {
			/* Update button values */
			read(joyfd, &jevent, sizeof(struct js_event));
			if (errno == ENODEV)
				return 1;

			int changed = 0;
			if ((jevent.type & ~JS_EVENT_INIT) == JS_EVENT_BUTTON) {
				/* If the button has changed value */
				if (((buttons | joymap[jevent.number]) == 0) == (jevent.value == 0)) {
					changed = joymap[jevent.number];
					if (jevent.value)
						buttons |= changed;
					else
						buttons &= ~changed;
				}
			}

			/* Process Events */
			if (mode == MODE_MOUSE) {
				if (changed)
					mouse_event(buttons, changed);
				mouse_tick();
			} else if (mode == MODE_KEYBOARD) {
				if (changed)
					keyboard_event(buttons, changed);
			}

			/* Powernap */
			usleep(5000);
		}
	
		/* Joystick disconnected */
		close(joyfd);
	
		/* Reset default modes */
		mouse_end();
		mode = 0;
		// TODO: keyboard_begin();
	}
	
	free(joymap);
	return 0;
}

